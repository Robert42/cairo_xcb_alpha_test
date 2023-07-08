# very simple - just a test/example case
.PHONY: clean

all: example

example:
	$(CC) example.c `pkg-config --cflags --libs cairo xcb` -o example

clean:
	rm -f example
