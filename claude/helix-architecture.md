# Helix Architecture Deep Dive — Cotty Editor Reference

Reference analysis of Helix (`references/helix/`) for porting to Cotty's editor.
Created 2026-03-03.

---

## Overview

Helix is structured as 5 crates. For Cotty, the mapping is:

| Helix Crate | Purpose | Cotty Equivalent |
|---|---|---|
| `helix-core` | Buffer, Selection, Transaction, History, Syntax | `libcotty/src/editor/` (Cot) |
| `helix-view` | Document, View, Editor, Tree, Theme, Gutter | `libcotty/src/editor/` (Cot) |
| `helix-tui` | Cell grid, double-buffering, diff-based flush | Already exists — terminal cell grid + Metal renderer |
| `helix-term` | Compositor, UI components, application loop | Split: Cot (compositor logic) + Swift (rendering) |
| `helix-lsp` | LSP client | Future work |

**Key insight**: Cotty already has a cell grid (`terminal.cot`), Metal renderer, and glyph atlas. Helix's `helix-tui` layer maps directly to what we have — the editor writes to the same cell grid the terminal uses. No new rendering infrastructure needed.

---

## 1. Core Data Structures (helix-core)

### Text Buffer — Rope (ropey crate)

Helix uses the `ropey` crate for O(log n) insert/delete. Cotty currently has a gap buffer (`buffer.cot`).

**Decision point**: Gap buffer is simpler and sufficient for most files. Rope becomes important for very large files (>1MB). Can start with gap buffer and add rope later if needed.

**All positions use character offsets (not byte offsets)**. Conversions happen at rope boundaries.

### Selection Model

**File**: `helix-core/src/selection.rs`

```
Range {
    anchor: usize,    // Stationary side
    head: usize,      // Moving side (extends on Shift+move)
    old_visual_position: Option<(u32, u32)>  // Soft-wrap tracking
}

Selection {
    ranges: SmallVec<[Range; 1]>,  // Multiple cursors
    primary_index: usize,          // Active cursor
}
```

**Key design decisions**:
- **Anchor-head, not start-end** — natural for extending selections. Anchor stays, head moves.
- **Gap indexing** — positions are between characters: `(0, 3)` = `[Som]e text`
- **Direction preserved** — `(0, 3)` is forward, `(3, 0)` is backward
- **Zero-width = cursor** — `(1, 1)` is a cursor between chars 0 and 1
- **SmallVec<1>** — single cursor is inline (no heap alloc), multicursor uses Vec
- **Auto-normalize** — on construction: sort ranges, merge overlaps, grapheme-align

**Invariants** (enforced by `normalize()`):
1. At least one range always exists
2. Ranges sorted by `from()`
3. Overlapping ranges merged
4. Primary index valid

**Selection mapping through edits** — `Selection::map(changes)` transforms all ranges through a ChangeSet. Uses `Assoc` enum to decide cursor stickiness at edit boundaries:
- `Before` / `After` — cursor stays before/after inserted text
- `BeforeSticky` / `AfterSticky` — preserves offset within replacements

### Transaction System

**File**: `helix-core/src/transaction.rs`

```
Operation = Retain(n) | Delete(n) | Insert(text)

ChangeSet {
    changes: Vec<Operation>,
    len: usize,        // Original doc length
    len_after: usize,  // Final doc length
}

Transaction {
    changes: ChangeSet,
    selection: Option<Selection>,
}
```

**Operational Transform style** — edits expressed as sequence of retain/delete/insert ops.

**Core operations**:
- **Compose**: `a.compose(b)` → single changeset for `doc1 → doc3`
- **Invert**: `changeset.invert(original_doc)` → undo changeset (Insert↔Delete)
- **Apply**: `changeset.apply(&mut rope)` — mutates rope in-place
- **Map position**: `changeset.map_pos(pos, assoc)` → new position after edit

**Transaction builders**:
- `Transaction::change(iter)` — from `(from, to, Option<text>)` tuples
- `Transaction::change_by_selection(doc, selection, closure)` — per-range edits
- `Transaction::insert(doc, selection, text)` — insert at each cursor
- `Transaction::delete(doc, selection)` — delete each selection range

### History (Undo/Redo)

**File**: `helix-core/src/history.rs`

```
History {
    revisions: Vec<Revision>,
    current: usize,
}

Revision {
    parent: usize,
    last_child: Option<NonZeroUsize>,  // Redo path
    transaction: Transaction,           // Forward edit
    inversion: Transaction,            // Reverse edit
    timestamp: Instant,
}
```

**Linear stack with redo branch** — not a full undo tree. Each revision points to parent. `last_child` tracks redo path. New edits after undo orphan old redo path.

**Operations**: undo (apply inversion, move to parent), redo (follow last_child, apply forward), jump by count or timestamp.

### Syntax (Tree-sitter)

**File**: `helix-core/src/syntax.rs`

Tree-sitter integration for syntax highlighting. Key concepts:
- Grammar binaries loaded per-language (`.so` files)
- S-expression queries (`.scm` files): `highlights.scm`, `injections.scm`, `textobjects.scm`
- Parse tree cached and incrementally updated via `InputEdit` on buffer changes
- Highlighting yields a stream of `HighlightEvent` (byte ranges → semantic tokens)
- Syntax is per-document, view-agnostic

### Text Objects & Movement

**File**: `helix-core/src/textobject.rs`, `movement.rs`

```
TextObject = Around | Inside | Movement

Movement = Extend | Move
```

Text objects and movements are **pure functions**: `(range, rope, context) → new_range`. No side effects, easily testable.

- Word boundaries via character category (whitespace/punctuation/word)
- Tree-sitter queries for function/class/block objects
- Vertical movement tracks `old_visual_position` for column memory

---

## 2. View Layer (helix-view)

### Document

**File**: `helix-view/src/document.rs` (~2500 lines)

A Document holds:
- `text: Rope` — the buffer
- `selections: HashMap<ViewId, Selection>` — per-view cursors
- `syntax: Option<Syntax>` — tree-sitter AST
- `history: Cell<History>` — undo tree
- `changes: ChangeSet` — pending uncommitted changes
- `path: Option<PathBuf>` — file path
- `diagnostics: Vec<Diagnostic>` — LSP diagnostics
- `language_servers: HashMap<Name, Arc<Client>>` — active LSP connections
- `indent_style`, `line_ending`, `encoding` — file format

**Lifecycle**: Open (read file, detect language/indent/encoding) → Modify (`apply(transaction, view_id)`) → Save (encode to file, atomic write) → Close.

**Key**: `apply()` updates ALL views' selections through the changeset, updates syntax tree, maps diagnostic positions.

**Per-view state stored IN document**: `HashMap<ViewId, Selection>` and `HashMap<ViewId, ViewData>`. This allows multiple views of the same document with independent cursors/viewports.

### View

**File**: `helix-view/src/view.rs` (~1000 lines)

```
View {
    id: ViewId,
    area: Rect,           // Screen rectangle
    doc: DocumentId,      // Which document
    jumps: JumpList,      // Navigation history
    gutters: GutterConfig,
}

ViewPosition {
    anchor: usize,           // Char position of viewport top-left
    horizontal_offset: usize,
    vertical_offset: usize,
}
```

**Scrolling**: After every cursor movement, `offset_coords_to_in_view_center()` checks if cursor is within `scrolloff` margin and adjusts viewport anchor.

**Coordinate transforms**:
- `pos_at_screen_coords(row, col)` → char index in document (screen → doc)
- `screen_coords_at_pos(char_idx)` → screen Position (doc → screen)

### Editor

**File**: `helix-view/src/editor.rs` (~2500 lines)

Top-level state:
```
Editor {
    mode: Mode,                    // Normal | Insert | Select
    tree: Tree,                    // Split tree of Views
    documents: BTreeMap<DocumentId, Document>,
    theme: Theme,
    count: Option<NonZeroUsize>,   // Vim count prefix
    selected_register: Option<char>,
    registers: Registers,          // Yank/paste buffers
    macro_recording: Option<(char, Vec<KeyEvent>)>,
    language_servers: Registry,    // LSP singleton
    diagnostics: Diagnostics,     // Global diagnostic aggregator
    needs_redraw: bool,
}
```

**Focus**: Editor owns a `Tree` (split tree). `current_ref!()` macro gets `(View, Document)` of focused view.

### Split Tree

**File**: `helix-view/src/tree.rs`

```
Tree {
    root: ViewId,
    focus: ViewId,
    nodes: SlotMap<ViewId, Node>,
}

Node = View(Box<View>) | Container(Box<Container>)

Container {
    layout: Horizontal | Vertical,
    children: Vec<ViewId>,
}
```

Recursive split layout. `recalculate()` walks DFS, dividing area equally among children. Navigation: `find_split_in_direction()` walks up tree to find sibling in direction.

**Already implemented in Cotty** as `split.cot` — flat node array with similar semantics.

### Theme

**File**: `helix-view/src/theme.rs`

```
Theme {
    styles: HashMap<String, Style>,  // "ui.linenr" → Style
    scopes: Vec<String>,             // Tree-sitter scope names
    highlights: Vec<Style>,          // Indexed by Highlight
}
```

Scope lookup is hierarchical: `"ui.text.focus"` falls back to `"ui.text"`, then `"ui"`.

### Gutter

**File**: `helix-view/src/gutter.rs`

Default layout: `[Diagnostics, Spacer, LineNumbers, Spacer, Diff]`

Each gutter type is a per-line render function: `(line, selected, first_visual_line) → Option<Style>`.

---

## 3. TUI & Compositor (helix-tui, helix-term)

### Cell Grid (helix-tui)

```
Cell {
    symbol: String,           // Grapheme (multi-width OK)
    fg: Color,
    bg: Color,
    underline_color: Color,
    underline_style: UnderlineStyle,
    modifier: Modifier,       // Bold | Italic | Dim | etc.
}

Buffer {
    area: Rect,
    content: Vec<Cell>,       // Flat 1D grid
}
```

**Double-buffering**: `Terminal` holds `[Buffer; 2]`. Write to current, diff against previous, flush only changed cells, swap.

**Maps directly to Cotty**: Our `CellData` struct + Metal renderer replaces this. Editor writes to the same cell grid terminal uses.

### Compositor

**File**: `helix-term/src/compositor.rs`

```
Component trait {
    handle_event(&mut self, event, ctx) → EventResult
    render(&mut self, area, surface, ctx)
    cursor(&self, area, editor) → (Option<Position>, CursorKind)
    required_size(&mut self, viewport) → Option<(u16, u16)>
}

Compositor {
    layers: Vec<Box<dyn Component>>,
}
```

**Event propagation**: Top-to-bottom (reverse iterate). First layer to consume stops propagation. Deferred callbacks execute after propagation.

**Rendering**: Bottom-to-top. Each layer draws to same surface. Later layers overwrite earlier.

**Cursor**: Top-to-bottom. First layer reporting a cursor wins.

### UI Components

| Component | Purpose |
|---|---|
| `EditorView` | Main editor — syntax, selections, gutter, statusline |
| `TextRenderer` | Low-level grapheme rendering, decorations |
| `Prompt` | Command line (`:`, `/`, `?`) |
| `Completion` | LSP completion menu |
| `Picker` | Fuzzy file/symbol picker |
| `Popup` | Floating overlay (hover, signature help) |
| `Markdown` | Render markdown for docs |
| `StatusLine` | Mode, file info, diagnostics, position |

### Application Loop

```
loop {
    tokio::select! {
        signal => handle_signals(),
        terminal_event => handle_terminal_events(),  // Key/mouse input
        job_callback => handle_callback(),           // Async results
        editor_event => handle_editor_event(),       // LSP, idle timer
    }
    render();
}
```

**Render cycle**:
1. Check resize
2. Get write buffer
3. `compositor.render(area, surface, ctx)` — all layers draw
4. `previous.diff(current)` — compute changed cells
5. Flush deltas to terminal
6. Swap buffers

---

## 4. Mapping to Cotty

### What Already Exists

| Helix Concept | Cotty Equivalent |
|---|---|
| Cell grid + rendering | `terminal.cot` cell grid + `MetalRenderer.swift` |
| Split tree | `split.cot` + `workspace.cot` |
| Glyph atlas | `GlyphAtlas.swift` |
| Surface abstraction | `surface.cot` (terminal + editor surfaces) |
| FFI boundary | `ffi.cot` + `cotty.h` |
| Theme | `Theme.swift` + config loading |
| Gap buffer | `buffer.cot` |
| Cursor | `cursor.cot` |

### Completed

| Component | Cotty File | Notes |
|---|---|---|
| **Selection model** (anchor-head, multi-cursor) | `src/selection.cot` | Range, Selection, normalize, map through edits |
| **Transaction system** (retain/delete/insert ops) | `src/transaction.cot` | Operation, ChangeSet, compose, invert, apply |
| **History** (undo/redo with inversion) | `src/history.cot` | Linear stack with redo branch, cursor restore |
| **Document** (buffer + selection + history) | `src/document.cot` | Buffer + History + Selection + filepath + dirty |
| **Mode system** (Normal/Insert/Select) | `src/input.cot` + `src/surface.cot` | Mode enum, keyToActionInMode, vim-style Normal mode |
| **Word movement** (w/b/e) | `src/surface.cot` | charCategory, moveWordForward/Backward/End |

### What Needs Building

| Component | Helix Ref | Priority | Effort |
|---|---|---|---|
| **View** (viewport, scrolling, coord transforms) | `view.rs` | P0 | Medium |
| **Editor state** (mode, registers, documents, focus) | `editor.rs` | P0 | Large |
| **Compositor** (layered components, event propagation) | `compositor.rs` | P1 | Medium |
| **EditorView** (syntax render, gutter, statusline) | `ui/editor.rs` | P1 | Large |
| **Prompt** (command line) | `ui/prompt.rs` | P1 | Medium |
| **Keymaps** (modal input, vim-like commands) | `keymap.rs` | P1 | Large |
| **Syntax highlighting** (tree-sitter integration) | `syntax.rs` | P2 | Large |
| **LSP client** | `helix-lsp/` | P2 | Very Large |
| **Completion** | `ui/completion.rs` | P2 | Medium |
| **Picker** (fuzzy finder) | `ui/picker.rs` | P2 | Large |
| **Diagnostics** (inline errors) | gutter + decorations | P2 | Medium |

### Rendering Strategy

The editor renders to the **same cell grid** as the terminal. An editor surface writes cells with:
- Syntax-highlighted text (fg color from theme scope)
- Selection highlighting (bg color)
- Cursor (block/bar/underline)
- Gutter (line numbers, diagnostics)
- Statusline (mode, file info)
- Overlays (completion popup, hover)

Metal renderer already handles all of this — it's just cells with colors and attributes.

### File Organization

Files are flat in `src/` (no subdirectory):

```
libcotty/src/
  selection.cot     # Range, Selection (port of selection.rs)        ✓ done
  transaction.cot   # Operation, ChangeSet, Transaction              ✓ done
  history.cot       # History, Revision (port of history.rs)         ✓ done
  document.cot      # Document (buffer + selection + history)        ✓ done
  input.cot         # Mode enum, keyToActionInMode, InputAction      ✓ done
  surface.cot       # Editor surface, mode dispatch, word movement   ✓ done
  view.cot          # View (viewport, scrolling, coord transforms)   TODO
  editor.cot        # Editor (top-level state, mode, registers)      TODO
  keymap.cot        # Modal input handling                           TODO
  commands.cot      # Editor commands (movement, edit, etc.)         TODO
  render.cot        # Editor → cell grid rendering                   TODO
```
