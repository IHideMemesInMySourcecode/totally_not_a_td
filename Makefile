.PHONY: all

all:
	clang++ td.cpp -o td `pkg-config --libs --cflags raylib`
