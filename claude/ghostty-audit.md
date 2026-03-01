# Ghostty Parity Audit — Full Gap Analysis

Comprehensive audit comparing Ghostty's Zig implementation against libcotty's Cot implementation.
Conducted 2026-03-02.

---

## Tier 1: Critical (Visibly Broken / Blocks Daily Use)

### 1. `saveCursor`/`restoreCursor` (DECSC/DECRC) doesn't save SGR, charset, origin, pending_wrap
- [ ] Ghostty saves the full style, protected flag, pending_wrap, origin mode, and charset state. libcotty only saves row/col. Programs like vim, tmux, and screen rely on DECSC/DECRC to preserve attributes across alt-screen transitions.
- Ghostty ref: `Screen.SavedCursor` — x, y, style, protected, pending_wrap, origin, charset
- libcotty: `saved_row`, `saved_col` only

### 2. Erase operations ignore current SGR background
- [x] When the current SGR has a non-default bg color, erased cells should show that bg. Ghostty's `clearCells` fills with the cursor's current style bg. libcotty always writes `Cell.init()` → `bg_type = COLOR_NONE`. Visible bug: `printf '\e[44m'; clear` shows black instead of blue.
- Affects: `eraseInLine`, `eraseInDisplay`, `insertLines`, `deleteLines`, `insertBlanks`
- Fixed: Added `blankCell()`, threaded blank cell through all erase/scroll/insert/delete operations

### 3. No wide character support (CJK, emoji)
- [ ] No `wide` field on Cell, no spacer_head/spacer_tail, no double-width handling in `putChar`. Every CJK character and wide emoji renders at 1-column width, corrupting the grid.
- Ghostty ref: `Page.Cell.wide` enum: narrow/wide/spacer_tail/spacer_head

### 4. TERM/COLORTERM/TERM_PROGRAM not injected into child env
- [x] When launched from Finder, `TERM` may be unset. Must inject `TERM=xterm-256color`, `COLORTERM=truecolor`, `TERM_PROGRAM=cotty`.
- Ghostty ref: `Termio.zig` env setup
- Fixed: Custom `buildPtyEnvp()` in pty.cot inherits parent env but overrides TERM, COLORTERM, TERM_PROGRAM

### 5. X10 mouse format (fallback when SGR mode 1006 not set)
- [x] When no mouse format mode is active, libcotty still sends SGR (`ESC [ < ...`). Should send classic X10 (`ESC [ M bxy`). Apps that enable mode 1000 without 1006 get malformed mouse data.
- Ghostty ref: `input/mouse.zig`
- Fixed: `mouseToTerminalBytes` now dispatches based on `mode_mouse_format` — SGR when 1006, X10 classic otherwise

### 6. Ctrl+special char mapping incomplete
- [x] Only handles ctrl+a-z/A-Z. Missing: ctrl+space (0x00), ctrl+2 (0x00), ctrl+3 (0x1B), ctrl+4 (0x1C), ctrl+5 (0x1D), ctrl+6 (0x1E), ctrl+7 (0x1F), ctrl+8 (0x7F), ctrl+@ (0x00), ctrl+\ (0x1C), ctrl+] (0x1D), ctrl+^ (0x1E), ctrl+_ (0x1F), ctrl+? (0x7F).
- Breaks: Emacs (ctrl+space = set mark), zsh vi-mode (ctrl+\)
- Fixed: Added Kitty-compatible ctrl+special mappings from Ghostty's ctrlSeq()

### 7. XTSAVE/XTRESTORE (`CSI ? Ps s` / `CSI ? Ps r`) missing
- [x] vim, neovim, tmux save/restore DEC modes on every launch and exit. Without this, mode state leaks across program boundaries.
- Ghostty ref: `Terminal.zig` save_mode/restore_mode
- Fixed: Added `xtsaved_*` fields, `saveDecMode()`/`restoreDecMode()` methods, parser dispatch for `CSI ? s` / `CSI ? r`

### 8. Mode 47/1047/1049 not distinguished
- [x] All three map to the same enter/exit. Mode 47 copies cursor without clearing; 1047 clears alt on exit; 1049 saves cursor on enter, restores on exit.
- Ghostty ref: `Terminal.zig` switchScreenMode
- Fixed: Split into separate handlers — 47 (switch only), 1047 (erase on exit), 1049 (save/restore cursor)

---

## Tier 2: High (Standard VT Behavior Broken)

### 9. No left/right margin support (DECSLRM)
- [ ] Mode 69 flag exists but no `scroll_left`/`scroll_right` fields. All cursor movement, insert/delete, erase, CR, tab, index, reverse-index ignore left/right margins.

### 10. `carriageReturn` always goes to column 0
- [ ] Should go to `scroll_left` when origin mode is set or cursor is inside left margin.

### 11. `eraseDisplay` mode 3 (erase scrollback) missing
- [ ] `CSI 3 J` is widely used (macOS Cmd+K). libcotty cannot erase scrollback.

### 12. LNM mode (ANSI mode 20) missing
- [ ] When set, LF also does CR. Some programs rely on this.

### 13. `insertLines`/`deleteLines` don't move cursor to left margin
- [ ] Ghostty moves cursor to `scroll_left` after IL/DL. libcotty leaves cursor in place.

### 14. Colon-separated SGR sub-parameters entirely absent
- [ ] No `4:3` (curly underline), no `38:2:r:g:b` (modern colon color format). Parser has no colon vs semicolon separator concept.

### 15. No underline/strikethrough/overline rendering
- [ ] SGR flags parsed and stored but renderer draws nothing for text decorations. `CELL_HIDDEN` not checked.

### 16. Bold-as-bright logic is in Swift, not Cot
- [ ] Per CLAUDE.md, all logic must be in Cot. Should move to terminal.cot or cell.cot.

### 17. Backspace/Enter/Tab with modifiers not encoded
- [ ] BS always \x7f, Enter always \r, Tab only shift. Missing alt+BS, ctrl+BS, ctrl+enter, alt+enter, ctrl+tab.

### 18. No `fullReset` (RIS, ESC c)
- [ ] softReset exists but no hard reset. RIS should clear everything.

### 19. XTGETTCAP only handles 3 capabilities
- [ ] Only TN, Co, RGB. neovim requests smcup, rmcup, setaf, sgr, kcuu1 and many others.

### 20. Child process exit doesn't auto-close tab
- [ ] Notify pipe EOF detected but tab stays open as dead session.

---

## Tier 3: Important (Protocol Completeness / Modern Apps)

### 21. No OSC 8 hyperlinks
- [ ] Modern CLIs (gh, cargo, delta) emit hyperlinks. No cell-level hyperlink tracking.

### 22. `modifyOtherKeys` (`CSI > Ps m`) missing
- [ ] neovim sets modifyOtherKeys level 2 for extended key encoding.

### 23. Super/Cmd never included in modifier parameter
- [ ] Swift never passes Cmd in mods. Kitty mods also miss caps_lock, num_lock, hyper, meta.

### 24. Kitty `report_alternates` and `report_associated` text not implemented
- [ ] Flags 2 and 4 of Kitty keyboard protocol.

### 25. No Insert key mapping
- [ ] No KEY_INSERT constant, no keyCode mapping, no `ESC [ 2 ~`.

### 26. macOS option-as-alt not configurable
- [ ] Option always treated as Alt. Users wanting Option+e → é get ESC e instead.

### 27. Mode 2031 (report_color_scheme) missing
- [ ] neovim uses this for dark/light theme detection.

### 28. Selection broken across scrollback
- [ ] sel_start_row/sel_end_row are active-grid-relative, don't account for scrollback offset.

### 29. No grapheme cluster / zero-width character support
- [ ] Mode 2027 flag exists but putChar has no width=0 path or grapheme break detection.

### 30. Mouse modifier bits not encoded in button code
- [ ] Shift(+4), alt(+8), ctrl(+16) never added to mouse button code.

### 31. No font variants (bold/italic font faces)
- [ ] GlyphAtlas uses single CTFont. CELL_BOLD/CELL_ITALIC ignored by renderer.

### 32. CAN (0x18) / SUB (0x1A) don't abort sequences
- [ ] Should cancel in-progress escape sequence and return to ground state.

### 33. No configurable shell command
- [ ] Hardcoded to /bin/zsh. No `command` config option.

### 34. No shell integration injection
- [ ] OSC 133 parsing works but shell must be manually configured.

---

## Tier 4: Nice-to-Have (Niche / Polish)

- [ ] 35. No search (Ctrl+F)
- [ ] 36. No block/rectangular selection
- [ ] 37. No eraseDisplay mode 22 (scroll-clear)
- [ ] 38. DECALN (ESC # 8)
- [ ] 39. ESC E (NEL — next line)
- [ ] 40. No emoji/color glyph atlas
- [ ] 41. No per-row dirty tracking
- [ ] 42. No mouse-hide-while-typing
- [ ] 43. No minimum-contrast
- [ ] 44. No background-opacity/transparency
- [ ] 45. Numpad keys, F13-F25, modifier keys as events
- [ ] 46. No bare `CSI u` for xterm restore-cursor
- [ ] 47. ANSI modes 2 (KAM), 12 (SRM)
- [ ] 48. No DECSCA / SPA/EPA (cell protection)
- [ ] 49. OSC 9, OSC 1337, OSC 22
- [ ] 50. No copy-on-select
