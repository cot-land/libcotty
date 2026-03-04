CC = clang
CFLAGS = -O2 -I tree-sitter/lib/include -I tree-sitter/lib/src

OBJECTS = tree-sitter/lib/src/lib.o \
    tree-sitter-markdown/tree-sitter-markdown/src/parser.o \
    tree-sitter-markdown/tree-sitter-markdown/src/scanner.o \
    tree-sitter-markdown/tree-sitter-markdown-inline/src/parser.o \
    tree-sitter-markdown/tree-sitter-markdown-inline/src/scanner.o \
    treesitter_shim.o

libtreesitter_shim.dylib: $(OBJECTS)
	$(CC) -dynamiclib -o $@ $^ -install_name @rpath/libtreesitter_shim.dylib

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(OBJECTS) libtreesitter_shim.dylib

.PHONY: clean
