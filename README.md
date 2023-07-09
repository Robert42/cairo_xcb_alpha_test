# cairo-xcb-alpha tutorial

> This tutorial's source code is forked from https://github.com/ryanflannery/cairo_xcb_alpha_test

If you want to write a gui application for linux, there are many options to choose: Gtk, Qt, librocket.
If you want to draw the ui yourself, then there are also many options: cairo, blend2d, nanovg, vkvg, thorvg.
For each of these, you will need to tell the operating systems what to open a window for you.
There are some libraries that will do that for you (and even initialize OpenGL for you): glfw, sdl2, sfml.

But every of this libraries is putting an abstraction layer between you and the operating system.
For some effects those libraries weren't made for, you will need to takl directly with the window system -- this libraries have ways to give you pointers/handle to windows to talk directly with the window manager.

On linux, this will be most likely X11 or Wayland.

This tutorial serves an a small introduction into creating a transparent window with xcb and drawing with cairo into it.

# Step 1: Create XCB Window

XCB is a thin wrapper over the X11 protocol.
The X11 protocol is a client/server model, where the operating systems window manager is the server and your application the client.
So unsurprisingly the connection to the X11 server is central aspect of the API.

Let's create a connection to the X11 server:

```c
xcb_connection_t *xcon;

xcon = xcb_connect(NULL, NULL);
assert(!xcb_connection_has_error(xcon));

// New code will come here

xcb_disconnect(xcon);
```

You will also need to `#include <xcb/xcb.h>` and `#include <assert.h>` for simple error testing. In a real application, you would of course need some proper error handling.


