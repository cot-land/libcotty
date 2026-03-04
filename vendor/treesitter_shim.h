// treesitter_shim.h — C shim wrapping tree-sitter for Cot FFI.
// Tree-sitter returns TSNode by value (32 bytes), which Cot can't handle.
// This shim flattens the API to pointer + integer types.

#ifndef TREESITTER_SHIM_H
#define TREESITTER_SHIM_H

#include <stdint.h>

// Highlight span: byte range + capture index + parser origin
typedef struct {
    uint32_t start_byte;
    uint32_t end_byte;
    uint32_t capture_index;
    uint32_t is_inline;  // 0=block, 1=inline
} CottyTSSpan;

// Initialize markdown context (block + inline parsers + queries).
// Returns opaque context pointer, or NULL on failure.
void *cotty_ts_init_markdown(void);

// Parse text and return highlight spans.
// Returns pointer to span array. Call cotty_ts_last_count() for span count.
// Caller must free with cotty_ts_free_spans().
CottyTSSpan *cotty_ts_highlight(void *ctx, const uint8_t *text, int64_t text_len);

// Get span count from most recent cotty_ts_highlight call.
int64_t cotty_ts_last_count(void *ctx);

// Span accessors
uint32_t cotty_ts_span_start(const CottyTSSpan *spans, int64_t idx);
uint32_t cotty_ts_span_end(const CottyTSSpan *spans, int64_t idx);
uint32_t cotty_ts_span_capture(const CottyTSSpan *spans, int64_t idx);
uint32_t cotty_ts_span_is_inline(const CottyTSSpan *spans, int64_t idx);

// Query capture name for a given index.
// Returns pointer to name string (owned by query, do not free).
const char *cotty_ts_capture_name(void *ctx, int64_t capture_idx, int64_t is_inline);

// Get length of capture name string.
int64_t cotty_ts_capture_name_len(void *ctx, int64_t capture_idx, int64_t is_inline);

// Number of captures in block or inline query.
int64_t cotty_ts_capture_count(void *ctx, int64_t is_inline);

// Free span array returned by cotty_ts_highlight.
void cotty_ts_free_spans(CottyTSSpan *spans);

// Destroy context and all internal resources.
void cotty_ts_destroy(void *ctx);

#endif
