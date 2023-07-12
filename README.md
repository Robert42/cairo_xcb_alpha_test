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

Final code in [tutorial_01_01_connection.c](tutorial_01_01_connection.c).
Compile with
```sh
gcc tutorial_01_01_connection.c `pkg-config --cflags --libs xcb` -o bin/tutorial_01_01_connection
```

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

Final code in [tutorial_01_02_create_window.c](tutorial_01_02_create_window.c).
Copmile with
```sh
gcc tutorial_01_02_create_window.c `pkg-config --cflags --libs xcb` -o bin/tutorial_01_02_create_window
```

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

Final code in [tutorial_01_03_event_handling.c](tutorial_01_03_event_handling.c).Compile with
```sh
gcc tutorial_01_03_event_handling.c `pkg-config --cflags --libs xcb` -o bin/tutorial_01_03_event_handling
```

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
    [...]
  */
  value_mask,
  value_list
);
```

Now we need to handle the mouse button click. For this, I simply added a switch statement for the `event->response_type` and a boolean flag `running` which I use to exit the main loop once the used clicked into the window.
The `0x7F` mask ignores the most significant bit: As that bit is used to mark, whether the event was send by SendEvent[^most_significant_bit].

Don't forget to `#include <stdbool.h>` for `true` and `false`.

```c
xcb_generic_event_t* event;
bool running = true;
while(running && (event = xcb_wait_for_event(xcon)))
{
  switch(event->response_type & 0x7F)
  {
  case XCB_BUTTON_PRESS:
    running = false;
    break;
  }
  free(event);
}
```

Now we can close our window by clicking into it.

Final code in [tutorial_01_04_mouse_event.c](tutorial_01_04_mouse_event.c).
Compile with
```sh
gcc tutorial_01_04_mouse_event.c `pkg-config --cflags --libs xcb` -o bin/tutorial_01_04_mouse_event
```

[^most_significant_bit]: https://web.archive.org/web/20230404170844/https://www.x.org/releases/current/doc/xproto/x11protocol.html#event_format

# 2 Cairo

## 2.1 draw a rectangle

Cairo is a vector graphics library popular on linux.
It can use xcb directly as backend, by calling

```c
cairo_surface_t* cairo_surface = cairo_xcb_surface_create(
  xcon, // connection
  xwindow, // drawable
  xvisual, // visual
  w, h
);
cairo_t* cairo = cairo_create(cairo_surface);
```
before calling `xcb_map_window`.

This won't compile, as we first need to `#include <cairo-xcb.h>` and get the visual to pass to cairo.

Let's write a helper function for that
```c
xcb_visualtype_t* get_xvisual(xcb_screen_t *screen, uint8_t depth)
{
  xcb_depth_iterator_t i = xcb_screen_allowed_depths_iterator(screen);
  for (; i.rem; xcb_depth_next(&i)) {
    if (i.data->depth != depth)
        continue;

    xcb_visualtype_iterator_t vi;
    vi = xcb_depth_visuals_iterator(i.data);
    for (; vi.rem; xcb_visualtype_next(&vi)) {
        return vi.data;
    }
  }

  errx(-1, "No visual found");
}
```
and call it after setting `xscreen`;
```c
xcb_visualtype_t* xvisual = get_xvisual(xscreen, xscreen->root_depth);
```

Now all we need to do is to listen to the expose event by modifying our `value_list`
```c
uint32_t value_list[] = {
  XCB_EVENT_MASK_EXPOSURE | XCB_EVENT_MASK_BUTTON_PRESS
};
```

and draw a simple rectangle when we got an expose event (in the switch statement, where where we already handle the mouse button event)
```c
  case XCB_EXPOSE:
    // background
    cairo_set_source_rgba(cairo, 0.5, 0.5, 0.5, 1);
    cairo_paint(cairo);

    // rectangle
    cairo_set_source_rgba(cairo, 1, 0.5, 0, 1);
    cairo_rectangle(cairo, 21, 21, 71, 71);
    cairo_fill(cairo);

    xcb_flush(xcon);
    break;
```

Final code in [tutorial_02_01_cairo_draw.c](tutorial_02_01_cairo_draw.c).
Compile with
```sh
gcc tutorial_02_01_cairo_draw.c `pkg-config --cflags --libs cairo-xcb` -o bin/tutorial_02_01_cairo_draw
```

## 2.2 draw a countdown

### Draw Text

Start simple by drawing some text close to the center

```c
  case XCB_EXPOSE:
    /*
      [... rectangle ...]
    */

    cairo_set_font_size(cairo, 64);
    cairo_move_to(cairo, w/2, h/2);
    cairo_set_source_rgba(cairo, 1, 1, 1, 1);
    cairo_show_text(cairo, "10");

    xcb_flush(xcon);
    break;
```

### Timer

For the countdown to actually work, we will need a timer.
For this tutorial we will use signals.
By calling `alarm(1)` we can tell the operating system to send the `SIGALRM` signal after one second.
So add asignal handler (don't forget to `#include <stdio.h>`)
```c
void handle_timeout(int)
{
  printf("TIMEOUT!\n");
}
```
and right before the event loop, set our new function as the `SIGALRM` signal handler (don't forget to `#include <signal.h>`).
```c
if(signal(SIGALRM, handle_timeout) == SIG_ERR) errx(-1, "Cout not set up timer");
```
And after that, we can start our timer
```c
alarm(1);
```
Now when you start the program, after a second, you should see `TIMEOUT!` being printed to the console.

### Redraw
To redraw, we need to send the expose event ourselves from the timeout handler.
```c
void handle_timeout(int)
{
  xcb_expose_event_t event = {
    .response_type = XCB_EXPOSE,
    .width = w,
    .height = h,
  };
  xcb_send_event(xcon, false, xwindow, XCB_EVENT_MASK_EXPOSURE, (char*)&event);
  xcb_flush(xcon);
}
```



To do so, our timout handler needs the `xcon` `xwindow` and `w` and `h`.
So lets turn those variables into global variables.
```c
const int x=16, y=16, w=256, h=256;
xcb_connection_t *xcon = NULL;
xcb_drawable_t xwindow = 0;
```

In order to see the countdown, let's add another global
```c
int countdown = 10;
```

and decrease it in our timeout handler before sending the event
```c
void handle_timeout(int)
{
  countdown--;
  // ...
}
```

Now we need to draw that value
```c
char countdown_text[3] = {};
snprintf(countdown_text, sizeof(countdown_text), "%i", countdown);
// ...
cairo_show_text(cairo, countdown_text);
```

### Countdown and exit

Awesome, now we see our countdown.
Now all we need to do is the send another timeout in our timeout handler.
```c
void handle_timeout(int)
{
  countdown--;
  alarm(1);
  // ...
}
```

And exit our program, when the countdown reached negative numbers by adding
```c
running = countdown > 0;
```
somewhere in the main loop (I've put it right before the switch).

Now we have a program, that shows a countdown for shutting itself down.

Final code in [tutorial_02_02_countdown.c](tutorial_02_02_countdown.c).
Compile with
```sh
gcc tutorial_02_02_countdown.c `pkg-config --cflags --libs cairo-xcb` -o bin/tutorial_02_02_countdown
```

## 2.3 Style

Currently, we have an opaque orange rectangle and an opaque white countdown.
The goal of this tutorial is to demonstrate some transparency.
Se we need to make our rendering more interesting to demonstrate those aspects.

Let's start by replacing the solid grey background with an opaque checkerboard, in some way as preview before later making the window actually transparent.

## Checkerboard

To draw the checkerboard, we create an image containing the checkreboard and then paint a patter of repeating that image.

First, we craete the checkerboard pattern

```c
  // Important this buffer must stay valid as long as the pattern is used by cairo.
  // Cairo won't copy this buffer and will instead reference this original.
  const uint8_t tile_brightness_center = 0xFF/2;
  const uint8_t tile_brightness_offset = 0xFF/8;
  const uint8_t dark_tile = tile_brightness_center - tile_brightness_offset;
  const uint8_t light_tile = tile_brightness_center + tile_brightness_offset;
  const uint32_t checkerboard_data[] = {0x010101*dark_tile, 0x010101*light_tile, 0x010101*light_tile, 0x010101*dark_tile};
  // create checkboard pattern
  cairo_pattern_t* checkerboard = ({
    // create image containing the tiles
    cairo_surface_t* img = cairo_image_surface_create_for_data((char*)checkerboard_data, CAIRO_FORMAT_RGB24, 2, 2, 2*4);
    // and create a patter showing this image
    cairo_pattern_t* pattern = cairo_pattern_create_for_surface(img);
    // repeated
    cairo_pattern_set_extend(pattern, CAIRO_EXTEND_REPEAT);
    // and scaled up to 8x8 px tiles
    cairo_matrix_t each_tile_8px;
    cairo_matrix_init_scale(&each_tile_8px, 1./8, 1./8);
    cairo_pattern_set_matrix(pattern, &each_tile_8px);
    // while using nearest pixel interpolation
    cairo_pattern_set_filter(pattern, CAIRO_FILTER_NEAREST);
    pattern;
  });
```

Then we can use this pattern as source of the background, instead of the old solid color
```c
cairo_set_source(cairo, checkerboard);
cairo_paint(cairo);
```

## transparent rectangle

Now that we have a heterogeneus background, why not add some transparency?

Let's start simple with a transparent rectangle
```c
// rectangle
cairo_set_source_rgba(cairo, 1, 0.5, 0, 0.5);
cairo_rectangle(cairo, 21, 21, 71, 71);
cairo_fill(cairo);
```

Final code in [tutorial_02_03_style.c](tutorial_02_03_style.c).
Compile with
```sh
gcc tutorial_02_03_style.c `pkg-config --cflags --libs cairo-xcb` -o bin/tutorial_02_03_style
```

# 2.4 Clean up

Now that our rendering is working, but we can clean our code up a little.

We have a code duplication: `get_xvisual` and `xcb_create_window` both use the same depth `xscreen->root_depth`, so why not store it in one variable?
```c
  const uint8_t depth = xscreen->root_depth;
  xcb_visualtype_t* xvisual = get_xvisual(xscreen, depth);

  // ...

  xcb_create_window(
    xcon, // connection
    depth,
    // ...
```

Also, we are using the same visual type for our window as we do for cairo, so let's replace `xscreen->root_visual` in our `xcb_create_window` call with `xvisual->visual_id`
```c
  xcb_create_window(
    // ...
    xvisual->visual_id,
    value_mask,
    value_list
  );
```

Final code in [tutorial_02_04_clean_up.c](tutorial_02_04_clean_up.c).
Compile with
```sh
gcc tutorial_02_04_clean_up.c `pkg-config --cflags --libs cairo-xcb` -o bin/tutorial_02_04_clean_up
```

# 3 Transparent Window

Now we're ready to start with the interesting part of the tutorial: drawing transparent windows.

## 3.1 Transparency

From now on, I will make new features optional using a macro
```c
#define TRANSPARENCY 1
```

### Depth
As we are drawing with an alpha channel, we need 32 bits instead of the `xscreen->root_depth`.
```c
#if TRANSPARENCY
  const uint8_t depth = 32;
#else
  const uint8_t depth = xscreen->root_depth;
#endif
```

### Borderpixel & Colormap
But now, that we've changed the depth, [we also need to change the border pixel and colormap of our window](https://stackoverflow.com/questions/3645632/how-to-create-a-window-with-a-bit-depth-of-32) for our visual.

```c
  xcb_colormap_t colormap = xcb_generate_id(xcon);
  xcb_create_colormap(xcon, XCB_COLORMAP_ALLOC_NONE, colormap, xscreen->root, xvisual->visual_id);

  xwindow = xcb_generate_id(xcon);
  uint32_t value_mask =
#if TRANSPARENCY
    XCB_CW_BORDER_PIXEL | XCB_CW_COLORMAP |
#endif
    XCB_CW_EVENT_MASK;
  uint32_t value_list[] = {
#if TRANSPARENCY
    0,
#endif
    XCB_EVENT_MASK_BUTTON_PRESS | XCB_EVENT_MASK_EXPOSURE,
#if TRANSPARENCY
    colormap,
#endif
  };
  xcb_create_window(
    // ...
```

### Clear Buffer
Now we would not see any change, because we draw an opaque checkerboard over everything behind the window.
Instead of drawing the checkerboard, we need to clear the background
```c
#if TRANSPARENCY
      // clear background
      cairo_set_operator(cairo, CAIRO_OPERATOR_SOURCE); // overwrite everything in the buffer ...
      cairo_set_source_rgba(cairo, 0, 0, 0, 0); // .. with transparent black
      cairo_paint(cairo);
      cairo_set_operator(cairo, CAIRO_OPERATOR_OVER); // Now wer're regulary drawing again
#else
      // opaque checkerboard background
      cairo_set_source(cairo, checkerboard);
      cairo_paint(cairo);
#endif
```

Final code in [tutorial_03_01_transparency.c](tutorial_03_01_transparency.c).
Compile with
```sh
gcc tutorial_03_01_transparency.c `pkg-config --cflags --libs cairo-xcb` -o bin/tutorial_03_01_transparency
```

## 3.2 Make it a splashscreen xor a dock on all desktops

The window is transparent, but it still has a border. If that's what you want, great!
But you may have other plans.
For example you might be interested in having a splashscreen for another application.

Then you would set the `_NET_WM_WINDOW_TYPE` property[^_NET_WM_WINDOW_TYPE] to to `_NET_WM_WINDOW_TYPE_SPLASH`.

Or you are writing a dock, that should also have no frames and be above over all other windows.
Then you could set the `_NET_WM_WINDOW_TYPE` to `_NET_WM_WINDOW_TYPE_DOCK`.
If you want the dock to appear on all desktops, you can set `_NET_WM_DESKTOP`[^_NET_WM_DESKTOP] to `0xffffffff`.

[^_NET_WM_WINDOW_TYPE]: https://web.archive.org/web/20230528202859/https://specifications.freedesktop.org/wm-spec/wm-spec-latest.html#idm45894598049680
[^_NET_WM_DESKTOP]: https://web.archive.org/web/20230528202859/https://specifications.freedesktop.org/wm-spec/wm-spec-latest.html#idm45894598055552
Final code in [tutorial_03_01_transparency.c](tutorial_03_01_transparency.c).
Compile with
```sh
gcc tutorial_03_01_transparency.c `pkg-config --cflags --libs cairo-xcb` -o bin/tutorial_03_01_transparency
```