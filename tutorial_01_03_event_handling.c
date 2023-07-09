#include <xcb/xcb.h>
#include <err.h>
#include <stdlib.h>

int main(int, char**)
{
  xcb_connection_t *xcon;

  xcon = xcb_connect(NULL, NULL);
  if(xcb_connection_has_error(xcon))
  {
    xcb_disconnect(xcon);
    errx(-1, "Failed to establish connection to X");
  }

  xcb_screen_t* xscreen;
  {
    xcb_screen_iterator_t xscreen_iter = xcb_setup_roots_iterator(xcb_get_setup(xcon));
    if(xscreen_iter.rem == 0) errx(-1, "No XScreen found");
    xscreen = xscreen_iter.data;
  }

  xcb_drawable_t xwindow = xcb_generate_id(xcon);
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

  xcb_map_window(xcon, xwindow);

  xcb_flush(xcon);

  xcb_generic_event_t* event;
  while((event = xcb_wait_for_event(xcon)))
  {
    free(event);
  }

  xcb_disconnect(xcon);
}