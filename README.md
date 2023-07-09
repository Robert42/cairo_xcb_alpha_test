# cairo-xcb-alpha tutorial

> This tutorial's source code is forked from https://github.com/ryanflannery/cairo_xcb_alpha_test.
> All rights of the original source code are reserved to the original author

License for my (Robert Hidlebrandt) modifications and this tutorial:
```
Zero-Clause BSD
=============
Copyright (C) 2023 by Robert Hildebrandt

Permission to use, copy, modify, and/or distribute this software for
any purpose with or without fee is hereby granted.

THE SOFTWARE IS PROVIDED “AS IS” AND THE AUTHOR DISCLAIMS ALL
WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES
OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE
FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY
DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN
AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT
OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
```
**Note that this license is only for my modifications and not the original code.**

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

## 1.2 Create Window

First we need to pick the screen we're going to use. X11 might have multiple screens, so normally, when connecting to the xserver, the server will tell us the id of the preferred screen (via the second argument of `xcb_connect`).
In order to keep the code to a minimum, I've just passed `NULL` and am now (according to the documentation[^xcb_connect]) expected to use the screen with the index 0, which is simply the first one returned by `xcb_setup_roots_iterator`

```c
xcb_screen_t* xscreen;
{
  xcb_screen_iterator_t xscreen_iter = xcb_setup_roots_iterator(xcb_get_setup(xcon));
  if(xscreen_iter.rem == 0) errx(-1, "No XScreen found");
  xscreen = xscreen_iter.data;
}
```

With a pointer to the xscreen, we can now create the window. First we generate an id to the new window

```c
xcb_drawable_t xwindow = xcb_generate_id(xcon);
```

This id alone has no effect, but we are now ale to craete the window itself telling it to use that id.

```c
int x=16, y=16, w=256, h=256;
xcb_create_window(
  xcon, // connection
  xscreen->root_depth, // depth
  xwindow, // window is
  xscreen->root,
  x, y, w, h,
  0,  // border width
  XCB_WINDOW_CLASS_INPUT_OUTPUT, // _class
  xscreen->root_visual,
  0, // value_mask
  NULL // value_list
);
```

Now that we have create a window, we can make it visible using `xcb_map_window`[^xcb_map_window]

```c
xcb_map_window(xcon, xwindow);
```

Remember, that wae using a client/server protocol?
We need to send all instructions to the server

```c
xcb_flush(xcon);
```

But we won't see anything, because the application will exit before anythong could've been shown. So we just add a `sleep` (don't forget to `#include <unistd.h>`)

```c
sleep(2);
```

[tutorial_01_02_create_window.c](tutorial_01_02_create_window.c)

[^xcb_map_window]: https://web.archive.org/web/20230226185624/https://xcb.freedesktop.org/manual/group__XCB____API.html#ga63b6126c8f732a339eff596202bcb5eb

## 1.3 Event Handling

Instead of freezig the program for two seconds, let's actually react to events.
Let's simply replace our `sleep(2)` with an event loop and let's `#include <stdlib.h>` for `free`.

```c
xcb_generic_event_t* event;
while((event = xcb_wait_for_event(xcon)))
{
  free(event);
}
```

This loop will stop, after we close our window.

[tutorial_01_03_event_handling](tutorial_01_03_event_handling)

## 1.4 Mouse Event

Let's also close our window when clicking on it.

Remember our window call?
We'v simple passed zero for the last two arguments, `value_mask` and `value_list`.
This two arguments can be used to customize our window in multiple ways.
In a way, those are optional arguments for `xcb_create_window`.
With `value_mask` you or all optional arguments you want to pass in `value_list`, which is an array of `uint32_t` storing all values. The order is implicitely defined.

In order to react to a mouse click, we need to tell X11 that we want to listen to the `XCB_EVENT_MASK_BUTTON_PRESS` event. So we simply add the optional argument `XCB_CW_EVENT_MASK` and set its value to `XCB_EVENT_MASK_BUTTON_PRESS`.
Don't forget to actually pass those values to `xcb_create_window`.

```c
uint32_t value_mask = XCB_CW_EVENT_MASK;
uint32_t value_list[] = {
  XCB_EVENT_MASK_BUTTON_PRESS
};
xcb_create_window(
  /*
    ...
  */
  value_mask,
  value_list
);
```

Now we need to handle the mouse button click. For this, I simply added a switch statement for the `event->response_type` and a boolean flag `running` which I use to exit the main loop once the used clicked into the window. Don't forget to `#include <stdbool.h>` for `true` and `false`.

```c
xcb_generic_event_t* event;
bool running = true;
while(running && (event = xcb_wait_for_event(xcon)))
{
  switch(event->response_type)
  {
  case XCB_BUTTON_PRESS:
    running = false;
    break;
  }
  free(event);
}
```

Now we can close our window by clicking into it.

[tutorial_01_03_event_handling](tutorial_01_03_event_handling)