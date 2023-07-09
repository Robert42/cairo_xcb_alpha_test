# cairo-xcb-alpha tutorial

> This tutorial's source code is forked from https://github.com/ryanflannery/cairo_xcb_alpha_test.

| Info |
| ---  |
| Work in progess, not complete |

If you want to write a gui application for linux, there are many options to choose: Gtk, Qt, librocket.
If you want to draw the ui yourself, then there are also many options: cairo, blend2d, nanovg, vkvg, thorvg.
For each of these, you will need to tell the operating systems what to open a window for you.
There are some libraries that will do that for you (and even initialize OpenGL for you): glfw, sdl2, sfml.

But every of this libraries is putting an abstraction layer between you and the operating system.
For some effects those libraries weren't made for, you will need to takl directly with the window system -- this libraries have ways to give you pointers/handle to windows to talk directly with the window manager.

On linux, this will be most likely X11 or Wayland.

This tutorial serves an a small introduction into creating a transparent window with xcb and drawing with cairo into it.

# 1 Create XCB Window

## 1.1 Connect to X11 server

XCB is a thin wrapper over the X11 protocol.
The X11 protocol is a client/server model, where the operating systems window manager is the server and your application the client.
So unsurprisingly the connection to the X11 server is central aspect of the API.

Let's create a connection to the X11 server:

```c
xcb_connection_t *xcon;

xcon = xcb_connect(NULL, NULL);
if(xcb_connection_has_error(xcon))
{
  xcb_disconnect(xcon);
  errx(-1, "Failed to establish connection to X");
}

// New code will come here

xcb_disconnect(xcon);
```

You will also need to `#include <xcb/xcb.h>` and `#include <err.h>` for simple error handling. In a real application, you would of course need some proper error handling. For this tutorial, `errx` is enough and will print the message and immediately exit the application.

It's noteworthy, that `xcb_connect` never returns `NULL`, not even when an error happens[^xcb_connect]. We need to use `xcb_connection_has_error` to find out, whether we successfully established a connection.


[^xcb_connect]: https://web.archive.org/web/20230608232234/https://xcb.freedesktop.org/manual/group__XCB__Core__API.html#ga094470586356d1764e69c9a1882966c3

[tutorial_01_01_connection.c](tutorial_01_01_connection.c)
