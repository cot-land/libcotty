# Editor Feature Roadmap

Tracks progress toward a production-ready text editor in Cotty.
All logic lives in Cot (libcotty). Swift is only platform bindings.

**Legend**: [x] = done, [ ] = not started, [~] = partial

---

## Phase 1 — Core Editing (Daily Use)

**Goal**: Edit files comfortably without reaching for another editor.

### 1.1 Text Manipulation

- [x] Insert/delete characters
- [x] Newline with auto-indent (inherits whitespace, extra indent after `{([`)
- [x] Auto-closing brackets/quotes with selection surrounding
- [x] Undo/redo with history tree
- [x] Cut/copy/paste with system clipboard
- [x] Cut/copy whole line when no selection
- [x] Smart paste with indent adjustment
- [x] Tab inserts 4 spaces, indents selection when active
- [x] Indent selection (Tab) / outdent selection (Shift+Tab)
- [x] Duplicate line (Cmd+Shift+D)
- [x] Move line up/down (Alt+Up/Alt+Down)
- [x] Delete word backward (Alt+Backspace)
- [x] Delete word forward (Alt+Delete)
- [x] Delete entire line (Cmd+Shift+K)
- [x] Join lines (Ctrl+J)

### 1.2 Navigation

- [x] Arrow keys, Home/End, Page Up/Down
- [x] Cmd+Arrow: line start/end, doc start/end
- [x] Alt+Arrow: word forward/backward
- [x] Go to matching bracket (bracket match highlighting exists)
- [x] Line numbers in gutter
- [x] Go to line (Cmd+G dialog)
- [ ] Scroll past end (visual overscroll)

### 1.3 Selection

- [x] Shift+Arrow/Home/End/PageUp/PageDown extends selection
- [x] Cmd+Shift+Arrow: select to line start/end, doc start/end
- [x] Alt+Shift+Arrow: select word
- [x] Select all (Cmd+A)
- [x] Mouse drag to select
- [x] Double-click word select, triple-click line select
- [x] Multiple cursors (Cmd+click)
- [x] Add next occurrence (Cmd+D)
- [x] Select line (Cmd+L)
- [ ] Highlight all occurrences of selected word

### 1.4 Search

- [x] Find (Cmd+F) with overlay
- [x] Replace (Cmd+Shift+H)
- [x] Next/prev match (Cmd+G / Cmd+Shift+G)
- [x] Case-sensitive toggle
- [ ] Regex search
- [ ] Find in selection
- [ ] Preserve case on replace

---

## Phase 2 — Language Editing

**Goal**: Language-aware editing features that make coding efficient.

### 2.1 Syntax Highlighting

- [x] Tree-sitter markdown highlighting
- [x] Per-surface token arrays (Surface.hl_tokens)
- [ ] Tree-sitter grammar for Cot language
- [ ] Tree-sitter grammar for common languages (JSON, TOML, Swift, Zig)
- [ ] Theme-aware token colors (use config palette, not hardcoded)

### 2.2 Code Intelligence

- [ ] Comment toggle (Cmd+/) — language-aware line/block comment
- [ ] Auto-detect indent style (tabs vs spaces, indent width)
- [ ] Trim trailing whitespace on save
- [ ] Insert final newline on save
- [ ] Show whitespace (toggle visible spaces/tabs/newlines)

### 2.3 LSP Integration

- [ ] LSP client in Cot
- [ ] Go to definition
- [ ] Find references
- [ ] Hover info
- [ ] Diagnostics (inline errors/warnings)
- [ ] Code actions / quick fixes
- [ ] Autocomplete

---

## Phase 3 — File Management

**Goal**: Reliable file operations and project awareness.

### 3.1 File Operations

- [x] Open file
- [x] Save (Cmd+S)
- [x] Save As (Cmd+Shift+S)
- [x] Modified file indicator (dirty dot on tab)
- [ ] Revert file to saved state
- [ ] Auto-save (configurable interval)
- [x] Warn on close with unsaved changes
- [ ] Watch file for external changes and prompt reload
- [ ] Recent files list

### 3.2 Project Features

- [x] File tree sidebar with expand/collapse
- [x] File tree auto-populate from PWD
- [ ] File tree: rename/delete/new file
- [ ] File tree: file icons by extension
- [ ] Fuzzy file finder (Cmd+P)
- [ ] Project-wide search (Cmd+Shift+F)

---

## Phase 4 — Polish

**Goal**: Small features that add up to a refined editing experience.

### 4.1 Visual Enhancements

- [x] Bracket match highlighting
- [x] Cursor blink
- [x] Soft word wrap
- [x] Status bar (line, column, mode)
- [ ] Current line highlight
- [ ] Indent guides (vertical lines at indent levels)
- [ ] Minimap
- [ ] Smooth scroll animation

### 4.2 Configurability

- [ ] Configurable tab size
- [ ] Configurable indent style (tabs vs spaces)
- [ ] Editor-specific keybinding overrides
- [ ] Word wrap toggle (soft wrap on/off)

---

## Progress Summary

| Phase | Total | Done | Remaining |
|-------|-------|------|-----------|
| 1 — Core Editing | 33 | 31 | 2 |
| 2 — Language Editing | 12 | 2 | 10 |
| 3 — File Management | 14 | 6 | 8 |
| 4 — Polish | 8 | 4 | 4 |
| **Total** | **67** | **43** | **24** |

**~63% complete.** Phase 1 is nearly done — only 2 items remain:
highlight all occurrences, and scroll past end (both need render-layer work).

### Phase 1 Remaining

1. Highlight all occurrences of selection
2. Scroll past end (visual overscroll)
