# Cotty Editor Architecture — Three-Source Design

How Ghostty, Helix, and Zed each contribute to a world-class editor.
Created 2026-03-03.

---

## Design Philosophy

Cotty is a developer environment built from three best-in-class references:

- **Ghostty** → Application shell, terminal emulator, rendering pipeline
- **Helix** → Editor data model (selection, transactions, undo history)
- **Zed** → Editor UX (movement, display mapping, smart editing, actions)

The separation is clean: Ghostty owns everything the user *sees* (windows, Metal, fonts, terminal). Helix owns how text *changes* (operational transforms, selection algebra, undo DAG). Zed owns how the editor *feels* (column memory, soft wraps, auto-indent, clipboard intelligence).

```
┌─────────────────────────────────────────────────────┐
│                    USER INTERACTION                   │
│         keyboard · mouse · scroll · clipboard        │
└────────────────────────┬────────────────────────────┘
                         │
┌────────────────────────▼────────────────────────────┐
│              SWIFT SHELL (Ghostty-style)             │
│  NSEvent → FFI · Metal rendering · Window/Tab mgmt  │
│  AppDelegate · MetalRenderer · GlyphAtlas · Theme    │
└────────────────────────┬────────────────────────────┘
                         │ FFI boundary (cotty.h)
┌────────────────────────▼────────────────────────────┐
│                  COT BACKEND (libcotty)              │
│                                                      │
│  ┌─────────────────────────────────────────────────┐ │
│  │         EDITOR CORE (Zed-style UX)              │ │
│  │  Actions · Movement · DisplayMap · ScrollAnchor │ │
│  │  AutoIndent · BracketMatch · ClipboardMeta      │ │
│  └──────────────────────┬──────────────────────────┘ │
│                         │                            │
│  ┌──────────────────────▼──────────────────────────┐ │
│  │       DATA MODEL (Helix-style primitives)       │ │
│  │  Selection · Transaction · History · Document   │ │
│  │  Buffer (gap) · Range · ChangeSet               │ │
│  └─────────────────────────────────────────────────┘ │
│                                                      │
│  ┌─────────────────────────────────────────────────┐ │
│  │       TERMINAL (Ghostty-style emulator)         │ │
│  │  VT Parser · Terminal Grid · PTY · Mouse · IO   │ │
│  └─────────────────────────────────────────────────┘ │
└──────────────────────────────────────────────────────┘
```

---

## Source 1: Ghostty — The Shell

**What it provides**: Everything outside the editor's text manipulation. The application *is* a Ghostty-style app with an editor bolted in.

### Already Ported (keep, maintain, extend)

| Component | Cotty Files | Status | Notes |
|---|---|---|---|
| Application lifecycle | `app.cot`, `AppDelegate.swift` | Complete | Singleton app, surface list, mailbox |
| Surface abstraction | `surface.cot`, `CottySurface.swift` | Complete | Editor + Terminal surfaces share grid |
| Terminal emulator | `terminal.cot`, `vt_parser.cot` | ~88% parity | 170K lines. CSI/ESC/OSC/DCS |
| Metal rendering | `MetalRenderer.swift` | Complete | Instanced cell rendering, 3 pipelines |
| Glyph atlas | `GlyphAtlas.swift` | Complete | CoreText, Unicode fallback, styled variants |
| IO threading | `surface.cot` (ioReaderMain) | Complete | Blocking PTY read, mutex, notification pipe |
| Keyboard protocol | `input.cot`, `TerminalView.swift` | Complete | Kitty protocol, macOS natural editing |
| Mouse tracking | `terminal.cot`, `TerminalView.swift` | Complete | 1000/1002/1003/1006 modes, SGR |
| Scrollback | `terminal.cot` | Complete | Grid resize + reflow |
| Selection (terminal) | `selection.cot`, `TerminalView.swift` | Complete | Word/line/char select, copy-on-select |
| Tabs & workspace | `workspace.cot`, `TabBarView.swift` | Complete | Chrome-style tabs, drag-reorder |
| Split panes | `split.cot`, `SplitContainerView.swift` | Complete | Binary tree, resize, focus nav |
| Inspector | `inspector.cot`, `InspectorView.swift` | Complete | 4-tab diagnostic overlay |
| Command palette | `command_palette.cot`, `CommandPaletteView.swift` | Complete | Fuzzy filter, action dispatch |
| Configuration | `config.cot`, `Theme.swift` | Complete | JSON config, reload without restart |
| Semantic prompts | `terminal.cot` (OSC 133) | Complete | Prompt markers, jump-to-prompt |

### Still Needed from Ghostty

| Component | Ghostty Reference | Priority | Why |
|---|---|---|---|
| **VSync rendering** | `renderer/Metal.zig` (CVDisplayLink) | P0 | Frame pacing. Currently renders on every notification. |
| **Damage tracking** | `renderer/generic.zig` (dirty rects) | P1 | Skip unchanged cells. Currently full-grid every frame. |
| **Text decorations GPU** | `renderer/generic.zig` (underline paths) | P1 | Underline/strikethrough parsed but not rendered in Metal. |
| **Grapheme clusters** | `terminal/page.zig` (multi-codepoint cells) | P1 | Currently 1 codepoint per cell. Breaks emoji/ZWJ. |
| **URL detection** | `input/Link.zig` (regex link matching) | P2 | Clickable URLs in terminal output. |
| **Search** | `terminal/search/` | P2 | Cmd+F scrollback search with regex. |
| **Shell integration** | `shell-integration/` | P2 | Auto-source RC for OSC 133 markers. |
| **Hyperlinks (OSC 8)** | Terminal per-cell URI storage | P3 | Explicit hyperlinks from apps. |
| **Sixel/Kitty images** | `terminal/kitty_graphics.zig` | P3 | Inline image display. |

### NOT Porting from Ghostty

- 405K `Config.zig` — keep JSON simple
- 164K keybinding engine — build lighter in Cot
- 3-thread model — 2-thread + CVDisplayLink is cleaner for macOS
- Font discovery system — CoreText handles this natively in Swift

---

## Source 2: Helix — The Data Model

**What it provides**: The mathematical foundation for text editing. Selection algebra, operational transforms, undo DAG. These are well-defined, well-tested abstractions that don't change based on UX style.

### Already Ported (keep as-is)

| Component | Cotty File | Lines | Tests | Helix File | Status |
|---|---|---|---|---|---|
| **Selection** | `selection.cot` | 532 | 26 | `helix-core/selection.rs` | 95% complete |
| **Transaction** | `transaction.cot` | 589 | 18 | `helix-core/transaction.rs` | 90% complete |
| **History** | `history.cot` | 288 | 7 | `helix-core/history.rs` | 85% complete |
| **Document** | `document.cot` | 164 | 7 | `helix-view/document.rs` | Simplified, correct |
| **Gap Buffer** | `buffer.cot` | 410 | 0* | Custom (not Helix rope) | Complete |

*Buffer tested indirectly via transaction/document tests.

**Why these stay Helix**: The data model is pure math. Anchor-head selection, retain/delete/insert OT, undo DAG with redo branching — these are the same regardless of whether the editor is modal or standard. Helix got these right, they're well-tested in Cot, and there's no reason to change them.

### Remaining Helix Work

| Component | What's Missing | Priority | Effort |
|---|---|---|---|
| **Transaction grouping** | Every keystroke = separate undo step. Need time-based grouping (Helix: group edits within same "thought"). | P0 | Small |
| **ChangeSet.compose()** | Merge two changesets into one. Needed for macro replay, grouped undo. | P1 | Medium |
| **Grapheme-aware positions** | Currently byte offsets. Need grapheme boundary detection for proper Unicode movement. | P1 | Medium |
| **Selection.map() in Selection** | Currently in Document. Should live in Selection for cleaner architecture. | P2 | Small |

### NOT Continuing from Helix

These were Helix concepts that don't fit Cotty's standard-editor-first approach:

| Helix Concept | Why Not | Replacement |
|---|---|---|
| **Modal editing as default** | Standard editor by default. Vim is opt-in extension. | Zed-style input dispatch |
| **Helix keymaps** (`keymap.rs`) | Helix-specific modal keymaps. | Zed-style action system |
| **Helix commands** (`commands.rs`) | Tied to modal operator-motion grammar (d3w). | Zed-style action handlers |
| **Text objects** (`textobject.rs`) | `iw`, `a(` etc. are vim concepts. | Keep for vim extension, not default path. |
| **Helix compositor** | TUI-oriented (cells + diff flush). | Ghostty Metal rendering (already have it). |
| **Registers** (a-z yank buffers) | Vim concept. | System clipboard by default. |

---

## Source 3: Zed — The Editor UX

**What it provides**: Everything that makes an editor feel *good*. This is the largest body of new work and the main path to "world-class."

Zed's editor crate is ~30K lines of Rust. We don't port all of it — we port the **patterns and algorithms** that make editing feel professional.

### P0 — Must Have (editor feels broken without these)

#### 1. SelectionGoal — Column Memory for Vertical Movement

**Zed**: `Selection<T>` has `goal: SelectionGoal` — stores visual X position.
**Current Cotty**: Moving down through a short line loses your column.

```
long line with text here|    ← cursor at col 24
short|                       ← cursor jumps to col 5
another long line here       ← cursor stays at col 5 (WRONG — should go back to 24)
```

**Port**: Add `goal_col: int` field to Cursor or Selection. When moving vertically, preserve goal. When moving horizontally or typing, clear goal.

**Zed ref**: `crates/text/src/selection.rs` lines 18-64, `crates/editor/src/movement.rs` (up/down functions).

**Effort**: Small. ~30 lines in cursor.cot + movement logic.

#### 2. Transaction Grouping — Undo by "Thought"

**Zed**: Edits within 300ms grouped into one undo step.
**Current Cotty**: Each keystroke is a separate undo revision.

**Port**: Add `group_interval` to History. On commit, check if time since last edit < interval. If so, merge into same revision instead of creating new one.

**Zed ref**: `crates/text/src/text.rs` lines 135-145 (History struct with group_interval).
**Helix ref**: `helix-core/src/history.rs` (same concept, `with_changes` grouping).

**Effort**: Small. ~40 lines in history.cot.

#### 3. Smart Movement Primitives

**Zed**: Movement functions are pure: `(snapshot, point, goal) → (point, goal)`.
**Current Cotty**: Movement mutates cursor in-place inside Surface. Not testable, not reusable.

**Port**: Extract movement into standalone functions in a new `movement.cot`:

```
fn moveLeft(buf: *Buffer, offset: int) int
fn moveRight(buf: *Buffer, offset: int) int
fn moveUp(buf: *Buffer, line: int, col: int, goal: int) MoveResult
fn moveDown(buf: *Buffer, line: int, col: int, goal: int) MoveResult
fn moveWordForward(buf: *Buffer, offset: int) int
fn moveWordBackward(buf: *Buffer, offset: int) int
fn moveLineStart(buf: *Buffer, line: int, first_non_whitespace: bool) int
fn moveLineEnd(buf: *Buffer, line: int) int
fn moveToBufferStart() int
fn moveToBufferEnd(buf: *Buffer) int
```

**Zed ref**: `crates/editor/src/movement.rs` (400+ lines of pure functions).

**Effort**: Medium. ~200 lines. Mostly extracting existing logic from surface.cot.

#### 4. Selection-Aware Editing (Type-Replaces-Selection)

**Status**: Just implemented (this session). Standard editor behavior where:
- Typing with selection replaces it
- Backspace/Delete with selection deletes it
- Movement collapses selection
- Shift+movement extends selection

**Remaining**: Needs testing and polish.

### P1 — Should Have (editor feels amateurish without these)

#### 5. Auto-Indent on Newline

**Zed**: When pressing Enter, new line inherits indentation of current line. If current line ends with `{`, add one indent level.
**Current Cotty**: Enter inserts bare `\n` at column 0.

**Port**: After inserting newline:
1. Scan previous line for leading whitespace
2. Copy that whitespace to new line
3. If previous line ends with `{`, `(`, `[`, `:`  → add one indent level
4. If next char after cursor is `}`, `)`, `]` → add closing line with dedent

**Zed ref**: `crates/editor/src/editor.rs` lines 5119-5259 (newline_with_auto_indent).

**Effort**: Medium. ~100 lines in surface.cot or a new `indent.cot`.

#### 6. Auto-Close Brackets

**Zed**: Typing `(` inserts `()` with cursor between. Typing `)` when next char is `)` just moves cursor right.
**Current Cotty**: Typing `(` inserts `(`.

**Port**: Bracket pair table: `( )`, `[ ]`, `{ }`, `" "`, `' '`, `` ` ` ``.
On insert of opening bracket → insert pair, cursor between.
On insert of closing bracket → if next char matches, skip instead of insert.
On backspace of opening bracket → delete pair if closing bracket is immediately after.

**Zed ref**: `crates/editor/src/editor.rs` (use_autoclose flag, bracket_pair config).

**Effort**: Medium. ~80 lines.

#### 7. Auto-Surround Selection

**Zed**: With text selected, typing `(` wraps it: `(selected text)`.
**Current Cotty**: With text selected, typing `(` replaces selection with `(`.

**Port**: If selection is non-empty and typed char is an opening bracket, wrap instead of replace.

**Zed ref**: `crates/editor/src/editor.rs` (use_auto_surround flag).

**Effort**: Small. ~30 lines on top of bracket pairs.

#### 8. Clipboard Intelligence

**Zed**: Copy preserves selection metadata (indentation, line boundaries). Paste restores it.
**Current Cotty**: Copy/paste is plain text.

**Port**:
- **Line-wise copy**: If selection covers full lines, paste inserts above/below cursor (not inline).
- **Indentation normalization**: On paste, adjust indentation to match target context.
- **Multi-cursor paste**: If copying from N cursors, pasting into N cursors distributes one selection per cursor.

**Zed ref**: `crates/editor/src/editor.rs` lines 13668+ (copy), 13803+ (paste), `ClipboardSelection` struct.

**Effort**: Medium. ~120 lines across surface.cot + ffi.cot.

#### 9. Matching Bracket Highlight

**Zed**: When cursor is on `(`, highlight the matching `)`.
**Current Cotty**: No bracket matching.

**Port**: Scan forward/backward from cursor, counting nesting depth. When matching bracket found, mark it with a highlight flag in the render grid.

Does NOT require tree-sitter for basic implementation (just char scanning with nesting). Tree-sitter makes it accurate for strings/comments.

**Effort**: Small. ~60 lines.

#### 10. Scroll Anchor

**Zed**: Scroll position is anchored to a buffer position + pixel offset. Edits above the viewport don't cause visual jumping.
**Current Cotty**: Scroll position is a line number. Inserting lines above viewport shifts content.

**Port**: Store `scroll_anchor: int` (byte offset of first visible line) + `scroll_pixel_offset: int`. After edits, map anchor through changeset.

**Zed ref**: `crates/editor/src/scroll.rs` (ScrollAnchor struct).

**Effort**: Small. ~40 lines.

### P2 — Nice to Have (editor feels complete with these)

#### 11. DisplayMap — Soft Wraps & Folds

**Zed**: Layers: Buffer → Folds → Inlays → Tabs → Wraps → Display.
**Current Cotty**: Buffer position = screen position (no wrapping, no folding).

**Port (simplified)**:
- **Phase 1**: Soft wrap at editor width. Wrap map: logical line → N display lines.
- **Phase 2**: Code folding. Fold map: collapsed ranges.
- **Phase 3**: Inlays (type hints, ghost text).

This is the biggest architectural addition. Requires `DisplayPoint` vs `Point` distinction throughout movement and rendering.

**Zed ref**: `crates/editor/src/display_map.rs` (1000+ lines).

**Effort**: Large. ~500 lines for soft wrap alone.

#### 12. Action System

**Zed**: Actions are serializable structs dispatched by name. Keybindings map to action names.
**Current Cotty**: `InputAction` enum with hardcoded `keyToAction()`.

**Port**: Keep the enum for now (simpler in Cot). When adding configurable keybindings, evolve to an action registry where keybindings map to action names.

**Effort**: Medium when needed. Not blocking.

#### 13. Multi-Cursor

**Zed**: `SelectionsCollection` with disjoint + pending selections. Each has unique ID.
**Current Cotty**: Selection supports multiple ranges (from Helix port) but no UI to create them.

**Port**:
- Cmd+D → select next occurrence of current word
- Cmd+Click → add cursor
- Operations apply to all cursors simultaneously

The data model already supports this (Selection has `List(Range)`). Need input dispatch + render support.

**Effort**: Medium. ~150 lines for Cmd+D and Cmd+Click.

#### 14. Syntax Highlighting (Tree-sitter)

**Both Helix and Zed** use tree-sitter. This is the single biggest feature gap.

**Port**:
- Load tree-sitter grammar .so files
- Parse buffer on load + incremental reparse on edit
- Run highlight queries → scope → theme color
- Render colored cells in grid

**Effort**: Very Large. ~800+ lines. Requires FFI to tree-sitter C library.

#### 15. Find & Replace

**Zed**: Cmd+F opens search bar, regex support, replace with capture groups.

**Port**: New `search.cot` module. Overlay UI in command palette style.

**Effort**: Large. ~400 lines.

#### 16. LSP Integration

**Both Helix and Zed** have full LSP. This is future work.

- Diagnostics (inline errors/warnings)
- Completions (popup menu)
- Hover (tooltip)
- Go-to-definition / references
- Rename

**Effort**: Very Large. Separate project phase.

---

## File Organization

Current and planned files in `libcotty/src/`:

```
# ─── HELIX DATA MODEL (keep) ────────────────────────────
selection.cot         # Range, Selection (anchor-head, multi-cursor)     ✓ done
transaction.cot       # ChangeSet, Transaction (retain/delete/insert)    ✓ done
history.cot           # History, Revision (undo DAG, redo branch)        ✓ done  → add grouping
document.cot          # Document (buffer + history + selection)          ✓ done

# ─── BUFFER ──────────────────────────────────────────────
buffer.cot            # Gap buffer (insert, delete, line queries)        ✓ done

# ─── ZED EDITOR UX (build) ──────────────────────────────
movement.cot          # Pure movement functions (left/right/up/down/word)  → NEW
indent.cot            # Auto-indent, bracket pairs, surround               → NEW
display_map.cot       # Soft wrap, fold map, display coordinates            → NEW (P2)
scroll.cot            # Scroll anchor, autoscroll strategies                → NEW

# ─── INPUT & ACTIONS ────────────────────────────────────
input.cot             # InputAction enum, keyToAction (standard editor)  ✓ done
actions.cot           # Action dispatch, parameterized commands            → NEW (P2)

# ─── SURFACE & RENDERING ───────────────────────────────
surface.cot           # Editor + Terminal surface, grid rendering        ✓ done  → refactor
cursor.cot            # Cursor position tracking                         ✓ done  → add goal_col

# ─── TERMINAL (Ghostty) ────────────────────────────────
terminal.cot          # Terminal state machine                           ✓ done
vt_parser.cot         # VT escape sequence parser                        ✓ done
pty.cot               # Pseudo-terminal spawning                         ✓ done
cell.cot              # Cell data structure                              ✓ done

# ─── APPLICATION ────────────────────────────────────────
app.cot               # App lifecycle, surface management                ✓ done
workspace.cot         # Tab/workspace state                              ✓ done
split.cot             # Split pane tree                                  ✓ done
config.cot            # Configuration loading                            ✓ done
inspector.cot         # Inspector overlay                                ✓ done
ffi.cot               # FFI export layer                                 ✓ done
```

---

## Implementation Roadmap

### Phase 1: Editor Fundamentals (make it feel right)

**Goal**: A developer would choose Cotty's editor over TextEdit.

| # | Task | Source | File | Effort |
|---|---|---|---|---|
| 1 | Transaction grouping (300ms) | Helix | `history.cot` | S |
| 2 | Column memory (goal_col) | Zed | `cursor.cot`, `surface.cot` | S |
| 3 | Extract pure movement functions | Zed | `movement.cot` (new) | M |
| 4 | Auto-indent on newline | Zed | `surface.cot` | M |
| 5 | Auto-close brackets | Zed | `surface.cot` | M |
| 6 | Matching bracket highlight | Zed | `surface.cot` render | S |
| 7 | Scroll anchor (edits don't shift viewport) | Zed | `surface.cot` | S |

**Verification**: Open a .cot file, type code, undo feels natural, indentation works, brackets pair, vertical navigation preserves column.

### Phase 2: Editor Polish (make it feel professional)

**Goal**: A developer would consider Cotty over VS Code for quick editing.

| # | Task | Source | File | Effort |
|---|---|---|---|---|
| 8 | Auto-surround selection | Zed | `surface.cot` | S |
| 9 | Clipboard intelligence (line-wise, indent) | Zed | `surface.cot`, `ffi.cot` | M |
| 10 | Find & replace (Cmd+F) | Zed | `search.cot` (new) | L |
| 11 | Multi-cursor (Cmd+D, Cmd+Click) | Zed | `surface.cot`, `selection.cot` | M |
| 12 | Grapheme-aware movement | Helix | `buffer.cot`, `movement.cot` | M |
| 13 | VSync rendering | Ghostty | Swift + `surface.cot` | M |
| 14 | Damage tracking | Ghostty | `MetalRenderer.swift` | M |

**Verification**: Edit real code. Undo groups feel right. Multi-cursor works. Search finds things. Terminal renders at smooth 120fps.

### Phase 3: Syntax & Intelligence (make it a real IDE)

**Goal**: Cotty is a daily-driver for Cot development.

| # | Task | Source | File | Effort |
|---|---|---|---|---|
| 15 | Tree-sitter integration | Both | `syntax.cot` (new) | XL |
| 16 | Syntax highlighting (highlight queries) | Both | `surface.cot` render | L |
| 17 | Soft wrapping | Zed | `display_map.cot` (new) | L |
| 18 | Code folding | Zed | `display_map.cot` | L |
| 19 | LSP client | Both | `lsp.cot` (new) | XL |
| 20 | Diagnostics (inline errors) | Both | `surface.cot` render | M |
| 21 | Completions | Both | `completion.cot` (new) | L |
| 22 | Go-to-definition | Both | `lsp.cot` | M |

**Verification**: Open a Cot project. Syntax is colored. Errors are underlined. Completions appear. Navigate to definitions.

### Phase 4: Vim Extension (opt-in modal editing)

**Goal**: Vim users can enable modal editing as a preference.

| # | Task | Source | File | Effort |
|---|---|---|---|---|
| 23 | Restore modal dispatch (Normal/Insert/Select) | Helix | `input.cot` | M |
| 24 | Operator-motion grammar (d3w, ci") | Helix | `commands.cot` (new) | L |
| 25 | Text objects (iw, a(, etc.) | Helix | `textobject.cot` (new) | L |
| 26 | Registers (a-z yank buffers) | Helix | `registers.cot` (new) | M |
| 27 | Macro recording (q) | Helix | `macro.cot` (new) | M |

**Verification**: Enable vim mode in config. hjkl works. d2w deletes two words. ciw changes inner word. Macros replay.

---

## Reference File Map

When implementing a feature, consult these files in order:

| Feature | Zed Reference | Helix Reference | Ghostty Reference |
|---|---|---|---|
| Selection model | `crates/text/src/selection.rs` | `helix-core/src/selection.rs` | — |
| Multi-cursor | `crates/editor/src/selections_collection.rs` | `helix-core/src/selection.rs` | — |
| Transactions | `crates/text/src/text.rs` | `helix-core/src/transaction.rs` | — |
| Undo/redo | `crates/text/src/text.rs` (History) | `helix-core/src/history.rs` | — |
| Movement | `crates/editor/src/movement.rs` | `helix-core/src/movement.rs` | — |
| Text objects | — | `helix-core/src/textobject.rs` | — |
| Auto-indent | `crates/editor/src/editor.rs:5119` | — | — |
| Bracket pairs | `crates/editor/src/editor.rs` (autoclose) | — | — |
| Clipboard | `crates/editor/src/editor.rs:13668` | — | — |
| Scroll | `crates/editor/src/scroll.rs` | `helix-view/src/view.rs` | — |
| Soft wrap | `crates/editor/src/display_map.rs` | — | — |
| Syntax highlight | `crates/editor/src/element.rs` | `helix-core/src/syntax.rs` | — |
| Find/replace | `crates/search/` | — | `terminal/search/` |
| Terminal grid | — | — | `terminal/Terminal.zig` |
| Metal rendering | — | — | `renderer/Metal.zig` |
| Glyph atlas | — | — | `font/Atlas.zig` |
| VT parsing | — | — | `terminal/Parser.zig` |
| PTY | — | — | `Pty.zig` |
| Mouse tracking | — | — | `surface_mouse.zig` |
| Window management | — | — | `App.zig`, `Surface.zig` |
| Config | — | — | `config/Config.zig` |

---

## Current State Summary

| Layer | Source | Lines of Cot | Tests | Assessment |
|---|---|---|---|---|
| **Terminal emulator** | Ghostty | ~180K | comprehensive | Production quality |
| **Editor data model** | Helix | ~2,000 | 58 | Solid foundation |
| **Editor UX** | Zed | ~800 | 0 | Functional but basic |
| **Application shell** | Ghostty | ~1,500 | 0 | Complete |
| **Swift rendering** | Ghostty | ~6,600 (Swift) | 0 | Production quality |

**Bottom line**: The terminal is strong. The editor data model is strong. What's missing is the Zed-style UX layer that makes editing feel polished — and that's exactly where the roadmap above focuses.
