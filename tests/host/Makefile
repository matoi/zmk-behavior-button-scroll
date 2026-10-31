CC ?= cc
CFLAGS := -std=c11 -Wall -Wextra -Werror -Wno-unused-parameter
CPPFLAGS := -I. -Iinclude

.PHONY: test clean

test: build/test_button_scroll
	./build/test_button_scroll

build/test_button_scroll: test_button_scroll.c stubs.h ../../src/behavior_button_scroll.c
	mkdir -p build
	$(CC) $(CPPFLAGS) $(CFLAGS) $< -o $@

clean:
	rm -rf build
