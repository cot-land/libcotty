# Cotty Architecture: Threading Model vs Ghostty

## Source
Mitchell Hashimoto interview (Feb 2025) describing Ghostty's architecture.

## Ghostty's 3-Thread Model

1. **UI thread** — draws windows, handles keyboard/mouse input
2. **IO thread** — reads/writes PTY bytes to/from the shell process
3. **Renderer thread** — runs on VSync clock (30/60/120 Hz), samples terminal state and draws

Key insight from Mitchell: "The best way to think about it is it's on a VSync clock... it's just sampling what the terminal state is and then drawing it."

Ghostty's renderer thread (`renderer/Thread.zig`) runs an event loop with timers:
- `DRAW_INTERVAL = 8` (120 FPS target)
- `CURSOR_BLINK_INTERVAL = 600`
- Uses `xev` async events + timer-based render loop
- IO thread wakes renderer via mailbox when state changes
- Renderer coalesces multiple wakeups into single frame draws

## Cotty's Current Model: 2-Thread (IO Thread + Main Queue)

Cotty has a dedicated background IO thread per terminal surface. All rendering and input handling happens on the main thread.

| Component | Thread | Trigger |
|-----------|--------|---------|
| PTY read | IO thread (Cot) | Blocking `fd_read()` loop in `ioReaderMain()` |
| VT parsing | IO thread (Cot) | Fed byte-by-byte under `terminal_mutex` lock |
| Terminal state mutation | IO thread (Cot) | `VtParser.feed()` → `TerminalState.putChar()` etc. |
| Metal rendering | Main (Swift) | `DispatchSourceRead` on notification pipe |
| Cursor blink | Main (Swift) | `Timer.scheduledTimer()` |
| Keyboard/mouse input | Main (Swift) | `NSView.keyDown()` / `mouseDown()` |

### How It Works

**IO thread (Cot, `surface.cot:ioReaderMain()`):**
1. Blocking `fd_read(master_fd, buf, 4096)` in a loop
2. Acquires `terminal_mutex`
3. Feeds each byte through VT parser → mutates TerminalState
4. Releases `terminal_mutex`
5. Writes `"!"` to notification pipe to signal main thread
6. Exits when PTY returns EOF (child exited)

**Main thread (Swift, `TerminalView.swift`):**
1. `DispatchSource.makeReadSource()` monitors notification pipe on `.main` queue
2. When signaled: drains pipe, calls `renderFrame()`
3. `renderFrame()` acquires `terminal_mutex`, reads grid/cursor/title, releases, renders via Metal
4. All keyboard/mouse input also acquires lock before touching terminal state

**Synchronization:**
- `Mutex` in Cot (`std/thread`) protects terminal state + VT parser
- Notification pipe (`pipe()`) signals IO→Main direction
- Lock held briefly: IO thread holds during parse batch, main thread holds during state read
- Lock is released BEFORE Metal rendering (GPU work doesn't need the lock)

### Where It Still Diverges from Ghostty

#### 1. No VSync Rendering (the missing 3rd thread)
Ghostty's renderer samples terminal state on a timer-driven loop (120 FPS target). Cotty renders **reactively** — every IO notification triggers a render. During fast output (`cat large_file`), this means potentially hundreds of render calls per second with no throttling. During idle, zero unnecessary renders (which is fine).

#### 2. No Render Throttling / Frame Coalescing
Ghostty coalesces multiple IO wakeups into single frame draws. Cotty renders after every pipe signal. Multiple rapid IO batches = multiple renders when one would suffice.

## Threading Primitives Available in Cot

- `Thread.spawn(func, arg)`, `.join()`, `.detach()` (wraps pthreads)
- `Mutex.init()`, `.lock()`, `.unlock()`, `.tryLock()`, `.destroy()`
- `Condition.init()`, `.wait(mutex)`, `.signal()`, `.broadcast()`, `.destroy()`
- `@atomicLoad`, `@atomicStore`, `@atomicAdd`, `@atomicCAS`, `@atomicExchange`
- `Channel(T)` — SPSC ring buffer with send/recv/close

## Completed Milestones

### ~~Phase 1: IO Reader Thread~~ ✅ DONE
- Dedicated Cot thread per surface (`ioReaderMain` in `surface.cot`)
- Blocking PTY read loop with 4K buffer
- VT parsing under mutex on IO thread
- Notification pipe (IO→Main) for render signaling
- Clean shutdown: `kill(SIGHUP)` → `waitpid` → `io_thread.join()` → cleanup
- All logic in Cot — Swift only monitors the pipe and renders

## Next: Phase 2 — VSync Rendering (the 3rd thread)

See design section below.

## Mitchell's Key Quotes

- "It's like a browser but for text content"
- "30% terminal, 70% font renderer"
- "The hardest part is actually maintaining the terminal state"
- "That terminal screen you see is like you're drawing on a canvas"
- Renderer submission: ~9 microseconds per frame (down from 800)
- "A 120 Hz frame is 8,333 microseconds. So if you have nine... you're leaving a lot of options"

## Performance Note

Mitchell benchmarks `cat` of large files as the obvious speed test. Modern terminals (Ghostty, Kitty, Alacritty) all significantly outperform Terminal.app. Cotty's 2-thread model handles this much better than single-threaded (IO doesn't block the UI), but the reactive rendering still causes excessive GPU work during output floods. VSync rendering would cap this at display refresh rate.

---

## Phase 2 Design: VSync-Driven Rendering

### Goal
Replace reactive render-on-every-IO-signal with a VSync-clocked render loop. The renderer checks a dirty flag on each VSync tick and only draws when content has changed. This caps render work at display refresh rate (60/120 Hz) regardless of how fast the IO thread processes PTY data.

### Ghostty's Approach (for reference)
Ghostty uses a separate **renderer thread** running an event loop (`xev`) with:
- A timer-based draw interval (8ms = 120 FPS)
- Async wakeup from IO thread via mailbox
- `drawFrame()` calls the Metal/OpenGL renderer
- On macOS, `must_draw_from_app_thread = true` — so Ghostty actually sends a `redraw_surface` message back to the app thread for the actual draw call

**Key detail**: On macOS, even Ghostty draws on the main thread. The renderer thread manages timing and state preparation, but the actual Metal `drawFrame()` is dispatched to the app thread. This is because macOS requires UI work on the main thread.

### Cotty's Approach: CVDisplayLink + Dirty Flag

Since even Ghostty draws on the main thread on macOS, Cotty can achieve the same result more simply:

**Option A: CVDisplayLink on main thread (simplest)**
1. Add `CVDisplayLink` (or `CADisplayLink` on macOS 14+) to `TerminalView`
2. IO thread sets an atomic dirty flag after each parse batch (instead of/in addition to pipe write)
3. On each VSync callback, check dirty flag → if set, clear it and call `renderFrame()`
4. Remove the reactive `DispatchSourceRead` render trigger (keep pipe for other signals like title/bell)
5. Cursor blink timer drives its own dirty flag

**Option B: Dedicated render thread with CVDisplayLink**
1. Spawn a render thread that owns the CVDisplayLink
2. On VSync: acquire terminal mutex, snapshot grid state, release mutex, render
3. More complex synchronization but closer to Ghostty's architecture
4. Allows render prep (building vertex buffers) to happen off main thread

### Recommended: Option A (CVDisplayLink on main thread)

This is the pragmatic choice because:
- macOS requires Metal rendering on the main thread anyway
- No additional thread synchronization complexity
- Same effective result as Ghostty on macOS (which also draws on main)
- Atomic dirty flag is trivial to implement in Cot
- CVDisplayLink fires at display refresh rate automatically

### Implementation Plan

**Cot changes (`surface.cot`):**
- Add `dirty: i64` field to Surface (atomic flag)
- IO thread: `@atomicStore(&self.dirty, 1)` after each parse batch
- FFI export: `cotty_terminal_is_dirty(surface) → i64` reads and clears the flag

**Swift changes (`TerminalView.swift`):**
- Add `CADisplayLink` (macOS 14+) or `CVDisplayLink` callback
- On VSync: `if surface.isDirty { renderFrame() }`
- Keep notification pipe for non-render signals (title changes, bell, child exit)
- Remove render call from `DispatchSourceRead` handler
- Cursor blink: set dirty flag instead of calling `renderFrame()` directly

**Expected result:**
- During `cat large_file`: IO thread processes bytes at full speed, renderer draws at 60/120 Hz
- During idle: no renders (dirty flag stays 0)
- Cursor blink: renders at blink rate (0.5s) via dirty flag
- Input handling: set dirty flag after key/mouse events, draws on next VSync
