# very simple - just a test/example case
.PHONY: clean



all: example tutorial

tutorial:
	$(CC) tutorial_01_01_connection.c `pkg-config --cflags --libs cairo xcb` -o bin/tutorial_01_01_connection
	$(CC) tutorial_01_02_create_window.c `pkg-config --cflags --libs cairo xcb` -o bin/tutorial_01_02_create_window

example: example.c
	$(CC) example.c `pkg-config --cflags --libs cairo xcb` -o bin/example

clean:
	rm -f example
