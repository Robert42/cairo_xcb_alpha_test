# very simple - just a test/example case
.PHONY: clean



all: example tutorial

tutorial: tutorial_01.c
	$(CC) tutorial_01.c `pkg-config --cflags --libs cairo xcb` -o bin/tutorial_01

example: example.c
	$(CC) example.c `pkg-config --cflags --libs cairo xcb` -o bin/example

clean:
	rm -f example
