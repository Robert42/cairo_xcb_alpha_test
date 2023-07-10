# very simple - just a test/example case
.PHONY: clean



all: example tutorial

tutorial:
	gcc tutorial_01_01_connection.c `pkg-config --cflags --libs xcb` -o bin/tutorial_01_01_connection
	gcc tutorial_01_02_create_window.c `pkg-config --cflags --libs xcb` -o bin/tutorial_01_02_create_window
	gcc tutorial_01_03_event_handling.c `pkg-config --cflags --libs xcb` -o bin/tutorial_01_03_event_handling
	gcc tutorial_01_04_mouse_event.c `pkg-config --cflags --libs xcb` -o bin/tutorial_01_04_mouse_event
	gcc tutorial_02_01_cairo_draw.c `pkg-config --cflags --libs cairo-xcb` -o bin/tutorial_02_01_cairo_draw

example: example.c
	gcc example.c `pkg-config --cflags --libs cairo xcb` -o bin/example

clean:
	rm -f example
