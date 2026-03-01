# libcotty → libghostty Parity Roadmap

Tracks progress toward feature parity with Ghostty's terminal emulation library.
Each item has a checkbox, Ghostty reference file, and the libcotty file it belongs in.

**Legend**: [x] = done, [ ] = not started, [~] = partial

---

## Phase 1 — Daily Driver

**Goal**: Run vim, tmux, htop, git, neovim without glitches.

### 1.1 Mouse Tracking

Ghostty ref: `src/input/mouse.zig`, `src/terminal/Terminal.zig`
libcotty: `terminal.cot`, `ffi.cot`

- [x] Mode 1000 (basic button tracking) — stored in setDecMode
- [x] Mode 1002 (button-event tracking / motion while pressed)
- [x] Mode 1003 (any-event tracking / all motion)
- [x] Mode 1006 (SGR extended mouse format)
- [x] Mode 1007 (mouse alternate scroll)
- [x] FFI: cotty_terminal_mouse_mode() / mouse_format() / mouse_event()
- [ ] X10 mouse encoding (mode 9) — legacy, rarely needed
- [ ] UTF-8 mouse encoding (mode 1005) — legacy, rarely needed
- [ ] urxvt mouse encoding (mode 1015) — legacy, rarely needed
- [ ] Mouse highlight tracking (mode 1001) — very rarely used
- [ ] Pixel-based mouse coordinates (mode 1016)

### 1.2 Bracketed Paste

Ghostty ref: `src/input/paste.zig`
libcotty: `terminal.cot`, `ffi.cot`

- [x] Mode 2004 stored in setDecMode
- [x] FFI: cotty_terminal_bracketed_paste_mode() returns flag
- [ ] Send `\e[200~` before paste and `\e[201~` after paste (Swift-side, needs FFI guidance)

### 1.3 Focus Events

Ghostty ref: `src/terminal/Terminal.zig`
libcotty: `terminal.cot`, `ffi.cot`

- [x] Mode 1004 stored in setDecMode
- [ ] FFI: expose focus_events mode flag to Swift
- [ ] Swift sends `\e[I` on focus-in and `\e[O` on focus-out when mode enabled

### 1.4 Clipboard (OSC 52)

Ghostty ref: `src/terminal/osc.zig` (OSC 52 handler)
libcotty: `vt_parser.cot`

- [ ] Parse OSC 52 sequence (clipboard selection + base64 payload)
- [ ] Decode base64 payload
- [ ] Surface clipboard-set action to Swift via FFI action
- [ ] Surface clipboard-query response (send back current clipboard contents)

### 1.5 Text Selection & Copy

Ghostty ref: `src/terminal/Selection.zig`
libcotty: `terminal.cot`, `ffi.cot`

- [x] selectionStart / selectionUpdate / selectionClear
- [x] markSelection (CELL_SELECTED flags on grid)
- [x] writeSelectedText (extract to buffer)
- [x] FFI: cotty_terminal_selected_text() / selected_text_len()
- [ ] Word selection (double-click) — detect word boundaries
- [ ] Line selection (triple-click)
- [ ] Semantic selection (respect OSC 133 prompt boundaries)
- [ ] Rectangular (block) selection
- [ ] Selection auto-scroll when dragging past viewport edge
- [ ] URL detection within selection

### 1.6 Window Title & Bell

Ghostty ref: `src/terminal/osc.zig`, `src/apprt/action.zig`
libcotty: `terminal.cot`, `vt_parser.cot`, `ffi.cot`

- [x] OSC 0/2 set window title
- [x] FFI: cotty_terminal_title() / title_len()
- [x] Bell: bell_pending flag set on BEL (0x07)
- [x] FFI: cotty_terminal_bell() returns and clears flag
- [ ] OSC 1 (set icon name — separate from title)
- [x] OSC 7 (current working directory)

### 1.7 Device Attributes & Status Reports

Ghostty ref: `src/terminal/Terminal.zig`
libcotty: `terminal.cot`, `vt_parser.cot`

- [x] Primary DA (CSI c) — sends `\e[?62;22c`
- [x] Secondary DA (CSI > c) — sends `\e[>1;10;0c`
- [x] DSR cursor position (CSI 6 n) — sends `\e[row;colR`
- [x] DSR operating status (CSI 5 n) — sends `\e[0n`
- [ ] Tertiary DA (CSI = c) — unit ID report
- [ ] DECRQM (CSI ? Ps $ p) — report mode value
- [ ] DECRQSS (DCS $ q) — report setting value
- [ ] XTVERSION (CSI > 0 q) — terminal version report

### 1.8 Complete DECSET/DECRST Modes

Ghostty ref: `src/terminal/modes.zig` (50+ modes)
libcotty: `terminal.cot` setDecMode()

**Currently handled (16 modes):**
- [x] 1 — Application cursor keys (DECCKM)
- [x] 6 — Origin mode (DECOM)
- [x] 7 — Wraparound mode (DECAWM)
- [x] 25 — Cursor visible (DECTCEM)
- [x] 47 — Alt screen (legacy)
- [x] 1000 — Mouse basic
- [x] 1002 — Mouse button-event
- [x] 1003 — Mouse any-event
- [x] 1004 — Focus events
- [x] 1006 — SGR mouse format
- [x] 1007 — Mouse alternate scroll
- [x] 1047 — Alt screen
- [x] 1048 — Save/restore cursor
- [x] 1049 — Alt screen + cursor save
- [x] 2004 — Bracketed paste
- [x] 2026 — Synchronized output

**Missing modes needed for daily-driver programs:**
- [ ] 3 — DECCOLM (132-column mode) — rarely used, but some programs query it
- [ ] 4 — Insert/Replace mode (IRM) — used by some editors
- [ ] 5 — Reverse video (DECSCNM) — swap fg/bg for entire screen
- [ ] 12 — Cursor blink (att610)
- [ ] 45 — Reverse wrap mode — backspace across soft-wrapped lines
- [ ] 66 — Application keypad mode (DECNKM)
- [ ] 67 — Backspace sends BS vs DEL (DECBKM)
- [ ] 69 — Left/right margin mode (DECLRMM)
- [ ] 80 — Sixel scrolling (DECSDM)
- [ ] 1005 — UTF-8 mouse encoding
- [ ] 1015 — urxvt mouse encoding
- [ ] 1016 — Pixel mouse coordinates
- [ ] 2027 — Grapheme clustering

---

## Phase 2 — Terminal Correctness

**Goal**: Pass vttest basic tests, handle edge cases that trip up programs.

### 2.1 CSI Sequences

Ghostty ref: `src/terminal/Terminal.zig`, `src/terminal/stream.zig`
libcotty: `vt_parser.cot` dispatchCsi()

**Currently handled:**
- [x] CUU/CUD/CUF/CUB (A/B/C/D) — cursor movement
- [x] CUP/HVP (H/f) — cursor position
- [x] CHA (G) — cursor horizontal absolute
- [x] VPA (d) — cursor vertical absolute
- [x] ED (J) — erase in display
- [x] EL (K) — erase in line
- [x] IL/DL (L/M) — insert/delete lines
- [x] ICH/DCH (@/P) — insert/delete chars
- [x] SGR (m) — attributes
- [x] DECSTBM (r) — set scroll region
- [x] SU/SD (S/T) — scroll up/down
- [x] ECH (X) — erase characters
- [x] DSR (n) — device status report
- [x] DA (c) — device attributes
- [x] TBC (g) — tab clear
- [x] CHT/CBT (I/Z) — forward/backward tab
- [x] REP (b) — repeat preceding character
- [x] DECSCUSR (q with space) — cursor style
- [x] DECSTR (p with !) — soft reset
- [x] SM/RM (h/l) — set/reset mode

**Missing:**
- [ ] CNL/CPL (E/F) — cursor next/previous line
- [ ] HPA (`) — horizontal position absolute
- [ ] VPR (e) — vertical position relative
- [ ] HPR (a) — horizontal position relative
- [ ] DECIC/DECDC — insert/delete columns
- [ ] DECRQM ($ p) — request mode value
- [ ] DECSCL — set conformance level
- [ ] DECCARA/DECRARA — change/reverse attributes in rectangular area
- [ ] DECFRA — fill rectangular area
- [ ] DECERA — erase rectangular area
- [ ] DECSERA — selective erase rectangular area
- [ ] DECCRA — copy rectangular area
- [ ] DECSACE — select attribute change extent
- [ ] XTWINOPS (t) — window manipulation (resize, report size, etc.)
- [ ] DECSLRM (s when DECLRMM active) — set left/right margins

### 2.2 ESC Sequences

Ghostty ref: `src/terminal/Terminal.zig`
libcotty: `vt_parser.cot` (Escape state handler)

**Currently handled:**
- [x] ESC 7/8 — save/restore cursor (DECSC/DECRC)
- [x] ESC D — index (IND)
- [x] ESC E — next line (NEL) — check: may be swapped with D
- [x] ESC M — reverse index (RI)
- [x] ESC ( / ) / * / + — charset designation
- [x] ESC [ — CSI introducer
- [x] ESC ] — OSC introducer
- [x] ESC > / = — application/normal keypad mode
- [x] ESC # 8 — DECALN (alignment test)
- [x] ESC c — RIS (full reset)

**Missing:**
- [ ] ESC H — set tab stop at current column (HTS)
- [ ] ESC N — single shift G2 (SS2)
- [ ] ESC O — single shift G3 (SS3)
- [ ] ESC P — DCS introducer
- [ ] ESC Z — return terminal ID (DECID, obsolete)
- [ ] ESC \ — string terminator (ST) — may already be handled in OSC/DCS context
- [ ] ESC 6 — back index (DECBI)
- [ ] ESC 9 — forward index (DECFI)

### 2.3 OSC Sequences

Ghostty ref: `src/terminal/osc.zig` (23K — comprehensive)
libcotty: `vt_parser.cot` dispatchOsc()

**Currently handled:**
- [x] OSC 0 — set icon name and title
- [x] OSC 2 — set window title
- [x] OSC 7 — current working directory

**Missing:**
- [ ] OSC 1 — set icon name (separate from title)
- [ ] OSC 4 — set/query color palette entry
- [ ] OSC 8 — hyperlinks (id + URI)
- [ ] OSC 9 — desktop notification (iTerm2)
- [ ] OSC 10 — set/query foreground color
- [ ] OSC 11 — set/query background color
- [ ] OSC 12 — set/query cursor color
- [ ] OSC 13-19 — set/query other colors (mouse fg/bg, highlight, etc.)
- [ ] OSC 22 — set mouse cursor shape
- [ ] OSC 50 — set font
- [ ] OSC 52 — clipboard access (read/write)
- [ ] OSC 104 — reset color palette entry
- [ ] OSC 110-119 — reset dynamic colors
- [ ] OSC 133 — semantic prompt markers (FinalTerm)
- [ ] OSC 176 — set current working directory (iTerm2 variant)
- [ ] OSC 777 — desktop notification (rxvt-unicode)
- [ ] OSC 1337 — iTerm2 proprietary sequences

### 2.4 DCS (Device Control String)

Ghostty ref: `src/terminal/dcs.zig`
libcotty: `vt_parser.cot` — NOT IMPLEMENTED

- [ ] DCS state machine in parser (DCS → params → passthrough → ST)
- [ ] DECRQSS (DCS $ q) — request setting status
- [ ] XTGETTCAP (DCS + q) — request terminfo capability
- [ ] DECSIXEL (DCS Ps ; Ps q) — sixel graphics (low priority)

### 2.5 C0 Control Characters

Ghostty ref: `src/terminal/Terminal.zig`
libcotty: `vt_parser.cot` Ground state

**Currently handled:**
- [x] BEL (0x07) — bell
- [x] BS (0x08) — backspace
- [x] HT (0x09) — horizontal tab
- [x] LF (0x0A) — line feed
- [x] VT (0x0B) — vertical tab (as LF)
- [x] FF (0x0C) — form feed (as LF)
- [x] CR (0x0D) — carriage return
- [x] SO (0x0E) — shift out (G1)
- [x] SI (0x0F) — shift in (G0)

**Missing:**
- [ ] ENQ (0x05) — return answerback message
- [ ] DEL (0x7F) — handled in input but verify parser ignores it in ground state

### 2.6 SGR (Select Graphic Rendition)

Ghostty ref: `src/terminal/SGR.zig`, `src/terminal/Style.zig`
libcotty: `terminal.cot` sgr()

**Currently handled:**
- [x] 0 — reset
- [x] 1 — bold
- [x] 2 — dim/faint
- [x] 3 — italic
- [x] 4 — underline
- [x] 7 — inverse/reverse video
- [x] 9 — strikethrough
- [x] 22 — normal intensity (unbold/undim)
- [x] 23 — not italic
- [x] 24 — not underlined
- [x] 27 — not inverse
- [x] 29 — not strikethrough
- [x] 30-37 — foreground basic colors
- [x] 38;2;r;g;b — foreground RGB
- [x] 38;5;n — foreground 256-color
- [x] 39 — default foreground
- [x] 40-47 — background basic colors
- [x] 48;2;r;g;b — background RGB
- [x] 48;5;n — background 256-color
- [x] 49 — default background
- [x] 90-97 — bright foreground
- [x] 100-107 — bright background

**Missing:**
- [ ] 5 — slow blink
- [ ] 6 — rapid blink
- [ ] 8 — hidden/invisible
- [ ] 21 — double underline
- [ ] 28 — not hidden
- [ ] 53 — overline
- [ ] 55 — not overline
- [ ] 58;2;r;g;b — underline color (Kitty extension)
- [ ] 58;5;n — underline 256-color (Kitty extension)
- [ ] 59 — default underline color

### 2.7 Character Sets

Ghostty ref: `src/terminal/charsets.zig` (3.2K)
libcotty: `terminal.cot` decSpecialMap(), `vt_parser.cot`

- [x] DEC Special Graphics (G0/G1 with `(0` / `)0`)
- [x] ASCII reset (`(B` / `)B`)
- [x] SO/SI switching between G0/G1
- [ ] UK character set (`(A`)
- [ ] G2/G3 designation (`*` / `+` intermediates)
- [ ] SS2/SS3 single shifts
- [ ] Locking shifts (LS2, LS3, LS1R, LS2R, LS3R)

---

## Phase 3 — Modern Protocols

**Goal**: Support neovim, kitty-based tools, modern terminal programs.

### 3.1 Kitty Keyboard Protocol

Ghostty ref: `src/terminal/kitty/key.zig`
libcotty: NOT IMPLEMENTED

This is a large feature. Programs detect support via DA or mode query.

- [ ] CSI ? u — query keyboard protocol flags
- [ ] CSI > flags u — push keyboard mode
- [ ] CSI < u — pop keyboard mode
- [ ] Enhanced key encoding (disambiguate, report events, report alternates, report all keys)
- [ ] CSI unicode-codepoint u — key event encoding
- [ ] Associated text reporting
- [ ] Modifier encoding in CSI u format

### 3.2 Kitty Graphics Protocol

Ghostty ref: `src/terminal/kitty/graphics_*.zig` (52K, 6 files)
libcotty: NOT IMPLEMENTED

Very large feature. Low priority unless image display is needed.

- [ ] APC Gq — graphics command parsing
- [ ] Transmission: direct, file, temp file, shared memory
- [ ] Display: absolute, relative, virtual placement
- [ ] Animation frames
- [ ] Image storage management
- [ ] Unicode placeholder integration

### 3.3 Hyperlinks (OSC 8)

Ghostty ref: `src/terminal/Hyperlink.zig` (7.6K)
libcotty: NOT IMPLEMENTED

- [ ] Parse OSC 8 ; params ; URI ST
- [ ] Store hyperlink ID + URI per cell range
- [ ] Surface hyperlink info via FFI for click handling
- [ ] Hover detection (highlight linked cells)
- [ ] Open URL on click (FFI action to Swift)

### 3.4 Semantic Prompts (OSC 133)

Ghostty ref: `src/terminal/osc.zig`, `src/shell-integration/`
libcotty: NOT IMPLEMENTED

- [ ] OSC 133 ; A — prompt start
- [ ] OSC 133 ; B — command start
- [ ] OSC 133 ; C — command output start
- [ ] OSC 133 ; D ; exitcode — command complete
- [ ] Store prompt boundaries in grid metadata
- [ ] Jump-to-prompt navigation (Cmd+Up/Down)
- [ ] Select command output region

### 3.5 Synchronized Output

Ghostty ref: `src/terminal/Terminal.zig`
libcotty: `terminal.cot`

- [x] Mode 2026 stored in setDecMode
- [ ] Actually batch rendering: suppress redraws while sync mode is on
- [ ] Timeout to prevent hang if app forgets to disable sync mode

### 3.6 Cursor Shapes

Ghostty ref: `src/terminal/Terminal.zig`
libcotty: `terminal.cot`, `vt_parser.cot`

- [x] DECSCUSR (CSI Ps SP q) — parsed, cursor_shape stored
- [x] cursor_shape field (0=block, 1=underline, 2=bar)
- [ ] FFI: surface cursor_shape to Swift for rendering
- [ ] Blinking vs steady variants (shapes 0-6)

---

## Phase 4 — Keybindings & Configuration

**Goal**: User-configurable terminal experience.

### 4.1 Keybinding Engine

Ghostty ref: `src/input/Binding.zig` (164K)
libcotty: NOT IMPLEMENTED (hardcoded in `input.cot`)

- [ ] Binding data structure (key + mods → action)
- [ ] Config file keybinding syntax
- [ ] Binding set (default + user overrides)
- [ ] Action dispatch (close_surface, new_tab, copy, paste, scroll_*, etc.)
- [ ] Leader key / chord sequences
- [ ] Physical vs logical key binding
- [ ] Performant keybinding lookup

### 4.2 Configuration System

Ghostty ref: `src/config/Config.zig` (405K)
libcotty: `config.cot` (397 lines — basic JSON)

- [x] Font name / size
- [x] Padding
- [x] Background / foreground / cursor / selection colors
- [x] 16-color ANSI palette
- [x] Load from JSON file
- [ ] TOML or key=value config format (match Ghostty format)
- [ ] Config reload without restart
- [ ] Per-surface configuration overrides
- [ ] Config validation with error messages
- [ ] CLI flag overrides
- [ ] Config dump command

### 4.3 Theme System

Ghostty ref: `src/config/theme.zig`
libcotty: hardcoded Monokai Remastered in `config.cot`

- [x] Default theme (Monokai Remastered)
- [ ] Theme file loading (separate from main config)
- [ ] Light/dark theme variants
- [ ] Auto-switch based on system appearance
- [ ] Theme directory scanning

---

## Phase 5 — Infrastructure & Polish

**Goal**: Production-quality terminal library.

### 5.1 Search

Ghostty ref: `src/terminal/search/` (6 files)
libcotty: NOT IMPLEMENTED

- [ ] Text search in active screen
- [ ] Text search in scrollback
- [ ] Regex search
- [ ] Search highlighting (mark matching cells)
- [ ] Next/previous match navigation
- [ ] Case-sensitive/insensitive toggle
- [ ] FFI: search API for Swift UI

### 5.2 Shell Integration

Ghostty ref: `src/shell-integration/`
libcotty: NOT IMPLEMENTED

- [ ] Inject shell function on PTY spawn (bash, zsh, fish)
- [ ] OSC 133 markers sent by injected shell code
- [ ] Current directory tracking (OSC 7)
- [ ] Command completion status tracking

### 5.3 URL Detection

Ghostty ref: `src/input/Link.zig`
libcotty: NOT IMPLEMENTED

- [ ] Regex-based URL detection in terminal grid
- [ ] Visual highlight on hover (underline + color change)
- [ ] Click-to-open via FFI action
- [ ] Cmd+click modifier support

### 5.4 Desktop Notifications

Ghostty ref: `src/os/desktop.zig`
libcotty: NOT IMPLEMENTED

- [ ] OSC 9 (iTerm2 notification)
- [ ] OSC 777 (rxvt-unicode notification)
- [ ] Surface notification via FFI action to Swift

### 5.5 Threading & Performance

Ghostty ref: `src/renderer/Thread.zig`, architecture docs
libcotty: `claude/architecture-threading.md`

- [x] IO reader thread (background PTY reads)
- [x] Mutex for terminal state
- [x] Notify pipe (IO thread → main)
- [ ] VSync-driven rendering (CVDisplayLink)
- [ ] Render throttling (skip frames if PTY is flooding)
- [ ] Dirty region tracking (only re-render changed cells)

### 5.6 Unicode

Ghostty ref: `src/unicode/` (many files)
libcotty: basic UTF-8 decode in `vt_parser.cot`

- [x] UTF-8 decode (2/3/4-byte sequences)
- [ ] Grapheme cluster detection (multi-codepoint characters)
- [ ] Wide character handling (CJK double-width)
- [ ] Emoji presentation (VS15/VS16)
- [ ] Zero-width joiners (ZWJ sequences)
- [ ] Bidirectional text (BiDi)
- [ ] Codepoint width lookup table (wcwidth)

---

## Progress Summary

| Phase | Total Items | Done | Remaining |
|-------|------------|------|-----------|
| 1 — Daily Driver | ~35 | ~22 | ~13 |
| 2 — Correctness | ~65 | ~40 | ~25 |
| 3 — Modern Protocols | ~25 | ~3 | ~22 |
| 4 — Config | ~15 | ~7 | ~8 |
| 5 — Infrastructure | ~25 | ~4 | ~21 |
| **Total** | **~165** | **~76** | **~89** |

libcotty is at roughly **46% checkbox completion** — much stronger than the line-count comparison suggests. The VT parser, grid, SGR, DA/DSR, mouse FFI, selection, alt screen, and reflow are all solid. The biggest gaps are in modern protocols (Kitty keyboard/graphics), search, keybindings, and Unicode width handling.
