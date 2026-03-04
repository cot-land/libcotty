// treesitter_shim.c — C shim wrapping tree-sitter for Cot FFI.
// Encapsulates markdown's two-parser complexity (block + inline).

#include "treesitter_shim.h"
#include "tree-sitter/lib/include/tree_sitter/api.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// External language functions from grammar parsers
extern const TSLanguage *tree_sitter_markdown(void);
extern const TSLanguage *tree_sitter_markdown_inline(void);

// ============================================================================
// Embedded highlight queries
// ============================================================================

// Block-level highlights (from tree-sitter-markdown/queries/highlights.scm)
static const char BLOCK_HIGHLIGHTS[] =
    "(atx_heading (inline) @text.title)\n"
    "(setext_heading (paragraph) @text.title)\n"
    "[\n"
    "  (atx_h1_marker)\n"
    "  (atx_h2_marker)\n"
    "  (atx_h3_marker)\n"
    "  (atx_h4_marker)\n"
    "  (atx_h5_marker)\n"
    "  (atx_h6_marker)\n"
    "  (setext_h1_underline)\n"
    "  (setext_h2_underline)\n"
    "] @punctuation.special\n"
    "[\n"
    "  (link_title)\n"
    "  (indented_code_block)\n"
    "  (fenced_code_block)\n"
    "] @text.literal\n"
    "(fenced_code_block_delimiter) @punctuation.delimiter\n"
    "(code_fence_content) @none\n"
    "(link_destination) @text.uri\n"
    "(link_label) @text.reference\n"
    "[\n"
    "  (list_marker_plus)\n"
    "  (list_marker_minus)\n"
    "  (list_marker_star)\n"
    "  (list_marker_dot)\n"
    "  (list_marker_parenthesis)\n"
    "  (thematic_break)\n"
    "] @punctuation.special\n"
    "[\n"
    "  (block_continuation)\n"
    "  (block_quote_marker)\n"
    "] @punctuation.special\n"
    "(backslash_escape) @string.escape\n";

// Inline-level highlights (from tree-sitter-markdown-inline/queries/highlights.scm)
static const char INLINE_HIGHLIGHTS[] =
    "[\n"
    "  (code_span)\n"
    "  (link_title)\n"
    "] @text.literal\n"
    "[\n"
    "  (emphasis_delimiter)\n"
    "  (code_span_delimiter)\n"
    "] @punctuation.delimiter\n"
    "(emphasis) @text.emphasis\n"
    "(strong_emphasis) @text.strong\n"
    "[\n"
    "  (link_destination)\n"
    "  (uri_autolink)\n"
    "] @text.uri\n"
    "[\n"
    "  (link_label)\n"
    "  (link_text)\n"
    "  (image_description)\n"
    "] @text.reference\n"
    "[\n"
    "  (backslash_escape)\n"
    "  (hard_line_break)\n"
    "] @string.escape\n";

// ============================================================================
// Context
// ============================================================================

typedef struct {
    TSParser *block_parser;
    TSParser *inline_parser;
    TSQuery *block_query;
    TSQuery *inline_query;
    TSQueryCursor *cursor;  // reusable cursor
    int64_t last_count;     // span count from last highlight call
} CottyTSContext;

// ============================================================================
// Init / Destroy
// ============================================================================

void *cotty_ts_init_markdown(void) {
    CottyTSContext *ctx = calloc(1, sizeof(CottyTSContext));
    if (!ctx) return NULL;

    const TSLanguage *block_lang = tree_sitter_markdown();
    const TSLanguage *inline_lang = tree_sitter_markdown_inline();

    ctx->block_parser = ts_parser_new();
    ctx->inline_parser = ts_parser_new();
    if (!ctx->block_parser || !ctx->inline_parser) goto fail;

    ts_parser_set_language(ctx->block_parser, block_lang);
    ts_parser_set_language(ctx->inline_parser, inline_lang);

    // Create queries
    uint32_t error_offset;
    TSQueryError error_type;

    ctx->block_query = ts_query_new(
        block_lang, BLOCK_HIGHLIGHTS, (uint32_t)strlen(BLOCK_HIGHLIGHTS),
        &error_offset, &error_type);
    if (!ctx->block_query) {
        fprintf(stderr, "treesitter_shim: block query error at offset %u, type %d\n",
                error_offset, error_type);
        goto fail;
    }

    ctx->inline_query = ts_query_new(
        inline_lang, INLINE_HIGHLIGHTS, (uint32_t)strlen(INLINE_HIGHLIGHTS),
        &error_offset, &error_type);
    if (!ctx->inline_query) {
        fprintf(stderr, "treesitter_shim: inline query error at offset %u, type %d\n",
                error_offset, error_type);
        goto fail;
    }

    ctx->cursor = ts_query_cursor_new();
    if (!ctx->cursor) goto fail;

    return ctx;

fail:
    cotty_ts_destroy(ctx);
    return NULL;
}

void cotty_ts_destroy(void *ptr) {
    if (!ptr) return;
    CottyTSContext *ctx = (CottyTSContext *)ptr;
    if (ctx->cursor) ts_query_cursor_delete(ctx->cursor);
    if (ctx->block_query) ts_query_delete(ctx->block_query);
    if (ctx->inline_query) ts_query_delete(ctx->inline_query);
    if (ctx->block_parser) ts_parser_delete(ctx->block_parser);
    if (ctx->inline_parser) ts_parser_delete(ctx->inline_parser);
    free(ctx);
}

// ============================================================================
// Highlight
// ============================================================================

// Collect inline content ranges from block tree for inline parser
static TSRange *collect_inline_ranges(TSNode root, uint32_t *out_count) {
    // Walk the tree looking for "inline" nodes
    uint32_t cap = 64;
    uint32_t count = 0;
    TSRange *ranges = malloc(cap * sizeof(TSRange));
    if (!ranges) { *out_count = 0; return NULL; }

    TSTreeCursor walker = ts_tree_cursor_new(root);
    bool done = false;

    // DFS traversal
    while (!done) {
        TSNode node = ts_tree_cursor_current_node(&walker);
        const char *type = ts_node_type(node);

        if (type && strcmp(type, "inline") == 0) {
            if (count >= cap) {
                cap *= 2;
                ranges = realloc(ranges, cap * sizeof(TSRange));
            }
            ranges[count].start_point = ts_node_start_point(node);
            ranges[count].end_point = ts_node_end_point(node);
            ranges[count].start_byte = ts_node_start_byte(node);
            ranges[count].end_byte = ts_node_end_byte(node);
            count++;

            // Don't descend into inline nodes
            if (!ts_tree_cursor_goto_next_sibling(&walker)) {
                while (ts_tree_cursor_goto_parent(&walker)) {
                    if (ts_tree_cursor_goto_next_sibling(&walker)) goto continue_walk;
                }
                done = true;
            }
            continue_walk:
            continue;
        }

        // Try to descend
        if (ts_tree_cursor_goto_first_child(&walker)) continue;
        if (ts_tree_cursor_goto_next_sibling(&walker)) continue;
        while (ts_tree_cursor_goto_parent(&walker)) {
            if (ts_tree_cursor_goto_next_sibling(&walker)) goto continue_walk2;
        }
        done = true;
        continue_walk2:;
    }

    ts_tree_cursor_delete(&walker);
    *out_count = count;
    return ranges;
}

// Compare function for qsort
static int span_compare(const void *a, const void *b) {
    const CottyTSSpan *sa = (const CottyTSSpan *)a;
    const CottyTSSpan *sb = (const CottyTSSpan *)b;
    if (sa->start_byte != sb->start_byte)
        return (sa->start_byte < sb->start_byte) ? -1 : 1;
    if (sa->end_byte != sb->end_byte)
        return (sa->end_byte < sb->end_byte) ? -1 : 1;
    return 0;
}

CottyTSSpan *cotty_ts_highlight(void *ptr, const uint8_t *text, int64_t text_len) {
    CottyTSContext *ctx = (CottyTSContext *)ptr;
    if (!ctx || !text || text_len <= 0) {
        if (ctx) ctx->last_count = 0;
        return NULL;
    }

    // 1. Block parse
    TSTree *block_tree = ts_parser_parse_string(
        ctx->block_parser, NULL, (const char *)text, (uint32_t)text_len);
    if (!block_tree) {
        ctx->last_count = 0;
        return NULL;
    }

    TSNode block_root = ts_tree_root_node(block_tree);

    // Collect spans into dynamic array
    uint32_t cap = 256;
    uint32_t count = 0;
    CottyTSSpan *spans = malloc(cap * sizeof(CottyTSSpan));

    // 2. Query block tree
    ts_query_cursor_exec(ctx->cursor, ctx->block_query, block_root);
    TSQueryMatch match;
    uint32_t capture_idx;
    while (ts_query_cursor_next_capture(ctx->cursor, &match, &capture_idx)) {
        TSQueryCapture cap_data = match.captures[capture_idx];
        uint32_t start = ts_node_start_byte(cap_data.node);
        uint32_t end = ts_node_end_byte(cap_data.node);
        if (count >= cap) {
            cap *= 2;
            spans = realloc(spans, cap * sizeof(CottyTSSpan));
        }
        spans[count].start_byte = start;
        spans[count].end_byte = end;
        spans[count].capture_index = cap_data.index;
        spans[count].is_inline = 0;
        count++;
    }

    // 3. Collect inline ranges from block tree
    uint32_t inline_range_count = 0;
    TSRange *inline_ranges = collect_inline_ranges(block_root, &inline_range_count);

    if (inline_ranges && inline_range_count > 0) {
        // 4. Inline parse
        ts_parser_set_included_ranges(ctx->inline_parser, inline_ranges, inline_range_count);
        TSTree *inline_tree = ts_parser_parse_string(
            ctx->inline_parser, NULL, (const char *)text, (uint32_t)text_len);

        if (inline_tree) {
            TSNode inline_root = ts_tree_root_node(inline_tree);

            // 5. Query inline tree
            ts_query_cursor_exec(ctx->cursor, ctx->inline_query, inline_root);
            while (ts_query_cursor_next_capture(ctx->cursor, &match, &capture_idx)) {
                TSQueryCapture cap_data = match.captures[capture_idx];
                uint32_t start = ts_node_start_byte(cap_data.node);
                uint32_t end = ts_node_end_byte(cap_data.node);
                if (count >= cap) {
                    cap *= 2;
                    spans = realloc(spans, cap * sizeof(CottyTSSpan));
                }
                spans[count].start_byte = start;
                spans[count].end_byte = end;
                spans[count].capture_index = cap_data.index;
                spans[count].is_inline = 1;
                count++;
            }

            ts_tree_delete(inline_tree);
        }
    }

    free(inline_ranges);
    ts_tree_delete(block_tree);

    // Sort spans by start_byte
    if (count > 1) {
        qsort(spans, count, sizeof(CottyTSSpan), span_compare);
    }

    ctx->last_count = (int64_t)count;
    return spans;
}

int64_t cotty_ts_last_count(void *ptr) {
    CottyTSContext *ctx = (CottyTSContext *)ptr;
    return ctx ? ctx->last_count : 0;
}

// ============================================================================
// Span accessors
// ============================================================================

uint32_t cotty_ts_span_start(const CottyTSSpan *spans, int64_t idx) {
    return spans[idx].start_byte;
}

uint32_t cotty_ts_span_end(const CottyTSSpan *spans, int64_t idx) {
    return spans[idx].end_byte;
}

uint32_t cotty_ts_span_capture(const CottyTSSpan *spans, int64_t idx) {
    return spans[idx].capture_index;
}

uint32_t cotty_ts_span_is_inline(const CottyTSSpan *spans, int64_t idx) {
    return spans[idx].is_inline;
}

// ============================================================================
// Capture name lookup
// ============================================================================

const char *cotty_ts_capture_name(void *ptr, int64_t capture_idx, int64_t is_inline) {
    CottyTSContext *ctx = (CottyTSContext *)ptr;
    TSQuery *query = is_inline ? ctx->inline_query : ctx->block_query;
    uint32_t len;
    return ts_query_capture_name_for_id(query, (uint32_t)capture_idx, &len);
}

int64_t cotty_ts_capture_name_len(void *ptr, int64_t capture_idx, int64_t is_inline) {
    CottyTSContext *ctx = (CottyTSContext *)ptr;
    TSQuery *query = is_inline ? ctx->inline_query : ctx->block_query;
    uint32_t len;
    ts_query_capture_name_for_id(query, (uint32_t)capture_idx, &len);
    return (int64_t)len;
}

int64_t cotty_ts_capture_count(void *ptr, int64_t is_inline) {
    CottyTSContext *ctx = (CottyTSContext *)ptr;
    TSQuery *query = is_inline ? ctx->inline_query : ctx->block_query;
    return (int64_t)ts_query_capture_count(query);
}

// ============================================================================
// Memory management
// ============================================================================

void cotty_ts_free_spans(CottyTSSpan *spans) {
    free(spans);
}
