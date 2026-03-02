# Ghostty vs Cotty — Terminal Parity Audit

Comprehensive audit comparing Ghostty's implementation against Cotty (libcotty + macos/).
Updated 2026-03-03.

---

## VT Parsing & Escape Sequences

### CSI Sequences — ~88% coverage

#### Cursor Movement — ALL DONE
- [x] CUU (A) — Cursor Up
- [x] CUD (B) — Cursor Down
- [x] CUF (C) — Cursor Forward
- [x] CUB (D) — Cursor Back
- [x] CNL (E) — Cursor Next Line
- [x] CPL (F) — Cursor Previous Line
- [x] CHA/HPA (G, `) — Cursor Horizontal Absolute
- [x] CUP/HVP (H, f) — Cursor Position
- [x] HPR (a) — Cursor Horizontal Relative
- [x] VPA (d) — Cursor Vertical Absolute
- [x] VPR (e) — Cursor Vertical Relative
- [x] CHT (I) — Cursor Horizontal Tab Forward
- [x] CBT (Z) — Cursor Backward Tabulation

#### Erase Operations — ALL DONE
- [x] ED (J) — Erase in Display (below/above/complete/scrollback)
- [x] EL (K) — Erase in Line (right/left/complete)
- [x] ECH (X) — Erase Characters

#### Line/Character Operations — ALL DONE
- [x] IL (L) — Insert Lines
- [x] DL (M) — Delete Lines
- [x] ICH (@) — Insert Blanks
- [x] DCH (P) — Delete Characters

#### Scrolling — ALL DONE
- [x] SU (S) — Scroll Up
- [x] SD (T) — Scroll Down

#### Tab Operations — ALL DONE
- [x] TBC (g) — Tab Clear
- [x] HTS (ESC H) — Horizontal Tab Set

#### Attributes — ALL DONE
- [x] SGR (m) — Full Select Graphic Rendition (bold, italic, underline, double underline, strikethrough, overline, inverse, blink, dim, hidden, colors)
- [x] Colon-separated extended colors (38:2:R:G:B, 48:2:R:G:B)

#### Margins — ALL DONE
- [x] DECSTBM (r) — Set Top/Bottom Margins
- [x] DECSLRM (s) — Set Left/Right Margins

#### Cursor Style & Protection — ALL DONE
- [x] DECSCUSR (space q) — Set Cursor Style (block/underline/bar, blink/steady)
- [x] DECSCA (" q) — Select Character Attribute (protection)

#### Device Control — ALL DONE
- [x] DA (c) — Device Attributes (primary/secondary/tertiary)
- [x] DSR (n) — Device Status Report
- [x] XTVERSION (> q) — Report Terminal Version

#### Keyboard Protocol — ALL DONE
- [x] CSI > m — Modify Other Keys
- [x] CSI ? u — Kitty Keyboard Protocol (query/push/pop/set)

#### Misc — DONE
- [x] REP (b) — Repeat Preceding Character
- [x] DECSTR (! p) — Soft Terminal Reset

#### Missing CSI
- [ ] DECRQM ($ p) — Request Mode Status
- [ ] DECSASD ($ }) — Select Active Status Display
- [ ] XTWINOPS (t) — Only size reports done, missing 14/16/21/22/23 variants

---

### Escape Sequences — ~87% coverage

#### ALL DONE
- [x] ESC 7 / ESC 8 — DECSC/DECRC Save/Restore Cursor (full: SGR, charset, origin, pending_wrap)
- [x] ESC D — IND (Index / scroll down)
- [x] ESC E — NEL (Next Line)
- [x] ESC H — HTS (Set Tab)
- [x] ESC M — RI (Reverse Index / scroll up)
- [x] ESC c — RIS (Full Reset)
- [x] ESC ( / ) / * / + — Charset Designation (ASCII, DEC, UK, etc.)
- [x] ESC # 8 — DECALN (Screen Alignment Pattern)
- [x] ESC = / ESC > — DECKPAM/DECKPNM (Keypad Modes)
- [x] ESC N / ESC O — SS2/SS3 (Single Shift G2/G3)
- [x] ESC V / ESC W — SPA/EPA (Protected Area)
- [x] ESC Z — DECID (Send Terminal ID)
- [x] ESC n / ESC o — LS2/LS3 (Locking Shift G2/G3)

#### Missing
- [ ] LS1R/LS2R/LS3R (ESC ~ / } / |) — Right-side locking shifts (stubs in Ghostty too)

---

### OSC Sequences — ~57% coverage

#### DONE
- [x] OSC 0/1/2 — Window/icon title
- [x] OSC 4 — Color palette set/query
- [x] OSC 7 — Working directory (CWD)
- [x] OSC 9 — iTerm2 notification / bell
- [x] OSC 10/11/12 — FG/BG/cursor color query/set
- [x] OSC 52 — Clipboard access (base64)
- [x] OSC 104 — Reset color palette entry
- [x] OSC 110/111/112 — Reset FG/BG/cursor color
- [x] OSC 133 — Semantic prompt marks (A/B/C/D)

#### Partial
- [~] OSC 22 — Mouse cursor shape (parsed, no-op)
- [~] OSC 1337 — iTerm2 protocol (CurrentDir only)

#### Missing
- [ ] OSC 8 — Hyperlinks
- [ ] OSC 21 — Kitty color protocol
- [ ] OSC 9;1-11 — ConEmu extensions
- [ ] OSC 66 — Kitty text sizing
- [ ] OSC 105/113-119 — Extended color resets
- [ ] OSC 777 — Desktop notification (alternative)

---

### DCS Sequences — 67% coverage

- [x] DECRQSS ($ q) — Request Status String
- [x] XTGETTCAP (+ q) — Request Terminal Capabilities (14 caps)
- [ ] Sixel Graphics — Missing (also missing in Ghostty macOS)

---

### Terminal Modes (DECSET/DECRST) — ~82% coverage

#### DONE
- [x] 1 — DECCKM (Application Cursor Keys)
- [x] 3 — DECCOLM (132/80 Column Mode, stub)
- [x] 5 — DECSCNM (Reverse Video)
- [x] 6 — DECOM (Origin Mode)
- [x] 7 — DECAWM (Wraparound)
- [x] 12 — Cursor Blinking
- [x] 25 — DECTCEM (Cursor Visible)
- [x] 45 — Reverse Wrap
- [x] 47 — Alt Screen (legacy)
- [x] 66 — DECNKM (Application Keypad)
- [x] 67 — Backspace Sends BS
- [x] 69 — Left/Right Margin Mode
- [x] 80 — Sixel Scrolling (stub)
- [x] 1000 — Mouse Reporting (X10 compatible)
- [x] 1002 — Button Event Tracking
- [x] 1003 — Any Event Tracking
- [x] 1004 — Focus Events
- [x] 1006 — SGR Mouse Format
- [x] 1007 — Alternate Scroll
- [x] 1047 — Alt Screen (erase on exit)
- [x] 1048 — Save/Restore Cursor
- [x] 1049 — Alt Screen + Save/Restore
- [x] 2004 — Bracketed Paste
- [x] 2026 — Synchronized Output
- [x] 2027 — Grapheme Cluster
- [x] 2031 — Report Color Scheme
- [x] ANSI modes: 2 (KAM), 4 (IRM), 12 (SRM), 20 (LNM)

#### Missing
- [ ] 1005 — UTF-8 Mouse Format
- [ ] 1015 — urxvt Mouse Format
- [ ] 1016 — SGR Pixels Mouse Format
- [ ] 1035/1036/1039 — Numlock/Alt handling variants
- [ ] 2048 — In-Band Size Reports (Kitty)

---

## Rendering & Grid

### Cell Model
- [x] Cell struct (codepoint + fg/bg type/value + flags)
- [x] All SGR attributes: bold, italic, underline, double underline, strikethrough, overline, inverse, blink, dim, hidden
- [x] Custom underline color (ul_type/ul_val)
- [x] 256-color palette + 24-bit RGB
- [x] Wide character support (CJK/emoji, CELL_WIDE + CELL_SPACER)
- [x] Character protection (CELL_PROTECTED)
- [x] Selection flag (CELL_SELECTED)
- [ ] Multi-codepoint graphemes (combining marks, ZWJ, skin tones, variation selectors) — single i64 per cell
- [ ] Hyperlink ID per cell (OSC 8)

### Metal Rendering
- [x] Instanced rendering pipeline (4 verts per cell, triangleStrip)
- [x] Glyph atlas with texture caching (GlyphAtlas.swift)
- [x] Pre-rendered ASCII glyphs (32-126)
- [x] On-demand Unicode glyph rasterization
- [x] Orthographic projection (2D top-left origin)
- [x] Cursor shapes: block, underline, bar + hollow outline (unfocused)
- [x] Cursor blink animation (timer-based)
- [x] Text decorations: underline, double underline, strikethrough, overline
- [x] Wide character 2x rendering
- [x] Inverse video rendering
- [x] Dim/faint alpha rendering
- [x] Background opacity (configurable)
- [x] Selection highlight color
- [x] Premultiplied alpha blending
- [ ] Damage tracking / partial redraws
- [ ] MSAA / subpixel AA
- [ ] Custom shaders (shadertoy)

### Font Rendering
- [x] Primary font loading (CTFont)
- [x] Bold / italic / bold-italic font variants (CTFontCreateCopyWithSymbolicTraits)
- [x] Styled glyph caching (style-encoded keys)
- [x] Font fallback (CTFontCreateForString)
- [x] Font size configuration
- [x] DPI awareness (scaleFactor)
- [x] Character width detection (charWidth with East Asian Width tables)
- [ ] Font shaping / ligatures (HarfBuzz) — direct glyph lookup only
- [ ] Colored emoji (COLR/CPAL, SVG-in-OpenType)

### Scrollback
- [x] Scrollback buffer with max lines + trimming
- [x] Full text reflow on resize (soft-wrap tracking)
- [x] Scrollback preservation on resize
- [x] Alt screen (separate grid, save/restore)
- [x] Viewport tracking
- [ ] Page-based memory allocation (Ghostty uses PageList)
- [ ] Serialization / persistence

### Selection
- [x] Character selection (mouse drag)
- [x] Word selection (double-click, isWordBoundary)
- [x] Selection rendering (CELL_SELECTED flag)
- [x] Copy-on-select
- [x] Grid-absolute selection coordinates
- [ ] Line selection (triple-click)
- [ ] Rectangle / block selection
- [ ] Selection survives reflow (Pin system)

### Resize & Reflow
- [x] Terminal resize → SIGWINCH
- [x] Soft-wrap tracking per row
- [x] Reflow on shrink (wrap long lines)
- [x] Reflow on grow (unwrap soft-wrapped lines)
- [x] Cursor clamping to new bounds
- [x] Alt-screen skip reflow
- [x] Scrollback preservation during resize

---

## Key Encoding

### Legacy xterm
- [x] ASCII printable pass-through
- [x] Ctrl+key encoding (a-z, special chars: space, @, [, \, ], ^, _, ?)
- [x] Arrow keys (app cursor / normal mode)
- [x] Function keys (F1-F20)
- [x] Numpad keys (KP 0-9, Enter, +, -, *, /, .)
- [x] Backspace/Enter/Tab with modifiers (Alt+BS, Ctrl+BS, Alt+Enter, etc.)
- [x] Alt+key ESC prefix
- [x] Insert/Delete/Home/End/PageUp/PageDown
- [x] MOD_SUPER (Cmd) in modifier param

### Kitty Keyboard Protocol
- [x] Mode negotiation (push/pop/query/set)
- [x] Flag stack (kitty_kbd_flags_0-7)
- [x] Key release events (eventType 1)
- [ ] Unshifted codepoint reporting
- [ ] Report alternates / report associated text (flags 2, 4)

### macOS Natural Text Editing (Ghostty Config.zig:6967-6996)
- [x] Cmd+Left → Ctrl-A (beginning of line)
- [x] Cmd+Right → Ctrl-E (end of line)
- [x] Cmd+Backspace → Ctrl-U (kill line)
- [x] Option+Left → ESC b (backward word)
- [x] Option+Right → ESC f (forward word)
- [x] Option-as-Alt toggle (config)

---

## Mouse Protocol

- [x] Mode 1000/1002/1003 tracking
- [x] SGR mouse format (mode 1006)
- [x] X10 classic format (fallback)
- [x] Mouse modifier bits (shift/alt/ctrl in button code)
- [x] Scroll wheel events
- [x] Alt-screen scroll mode (mode 1007)
- [x] Focus events (mode 1004)
- [ ] UTF-8 mouse format (mode 1005)
- [ ] urxvt mouse format (mode 1015)
- [ ] SGR pixel mouse (mode 1016)
- [ ] Inertial / high-precision scrolling

---

## Application Features

### DONE
- [x] Multiple windows
- [x] Tabs (create/close/select, Cmd+1-9)
- [x] Tab titles (dynamic from shell via OSC 0/2)
- [x] Split panes (horizontal/vertical, focus nav, divider resize)
- [x] Unfocused split dimming (Ghostty style, 0.75 opacity)
- [x] Split focus sync (Swift ↔ Cot via FFI)
- [x] Hollow cursor for unfocused panes (no blink)
- [x] Inspector overlay (Cmd+Opt+I)
- [x] Command palette
- [x] Sidebar / file tree
- [x] Theme switching (Cmd+Shift+T, 2 built-in themes)
- [x] JSON config loading (~/.config/cotty/config.json)
- [x] Copy/paste (Cmd+C/V)
- [x] OSC 52 clipboard
- [x] Copy-on-select
- [x] OSC 133 prompt marks + jump to prompt (Cmd+Up/Down)
- [x] Natural text editing keybinds
- [x] Option-as-alt toggle
- [x] CVDisplayLink VSync rendering
- [x] 2-thread model (main + IO per surface)
- [x] Bracketed paste
- [x] Background opacity
- [x] Child process exit auto-closes tab/pane
- [x] TERM/COLORTERM/TERM_PROGRAM env injection

### MISSING
- [ ] Terminal search (Cmd+F)
- [ ] URL detection / clickable links
- [ ] Fullscreen (native + non-native)
- [ ] Quick terminal (global hotkey dropdown)
- [ ] Desktop notifications (NSUserNotification)
- [ ] Session / workspace restoration
- [ ] Custom keybinding tables
- [ ] Equalize splits command
- [ ] 3rd renderer thread (VSync decoupled from main)
- [ ] Damage tracking / partial redraws
- [ ] Image protocols (Sixel, Kitty graphics, iTerm2 inline)
- [ ] Font shaping / ligatures
- [ ] Multi-codepoint grapheme storage
- [ ] Hyperlink rendering (OSC 8)

---

## Summary

| Category | Done | Missing | Coverage |
|---|---|---|---|
| CSI sequences | 23 | 3 | ~88% |
| Escape sequences | 20 | 3 | ~87% |
| OSC sequences | 12 | 6 | ~57% |
| DCS sequences | 2 | 1 | ~67% |
| Terminal modes | 27 | 6 | ~82% |
| Rendering/grid | 21 | 5 | ~81% |
| Key encoding | 18 | 2 | ~90% |
| Mouse protocol | 7 | 4 | ~64% |
| App features | 23 | 14 | ~62% |
| **Total** | **~153** | **~44** | **~78%** |

Core terminal emulation is solid. Main gaps are modern extensions (graphemes, ligatures, image protocols, hyperlinks) and app-level features (search, URL detection, fullscreen). Ready to pivot to editor work.
