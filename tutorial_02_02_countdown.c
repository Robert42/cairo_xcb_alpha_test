#include <xcb/xcb.h>
#include <cairo-xcb.h>
#include <err.h>
#include <stdlib.h>
#include <stdbool.h>


xcb_visualtype_t* get_xvisual(xcb_screen_t *screen, uint8_t depth);

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

  xcb_visualtype_t* xvisual = get_xvisual(xscreen, xscreen->root_depth);

  xcb_drawable_t xwindow = xcb_generate_id(xcon);
  int x=16, y=16, w=256, h=256;
  uint32_t value_mask = XCB_CW_EVENT_MASK;
  uint32_t value_list[] = {
    XCB_EVENT_MASK_BUTTON_PRESS | XCB_EVENT_MASK_EXPOSURE
  };
  xcb_create_window(
    xcon, // connection
    xscreen->root_depth, // depth
    xwindow, // window is
    xscreen->root,
    x, y, w, h,
    0,  // border width
    XCB_WINDOW_CLASS_INPUT_OUTPUT, // _class
    xscreen->root_visual,
    value_mask,
    value_list
  );

  cairo_surface_t* cairo_surface = cairo_xcb_surface_create(
    xcon, // connection
    xwindow, // drawable
    xvisual, // visual
    w, h
  );
  cairo_t* cairo = cairo_create(cairo_surface);

  xcb_map_window(xcon, xwindow);

  xcb_flush(xcon);

  xcb_generic_event_t* event;
  bool running = true;
  while(running && (event = xcb_wait_for_event(xcon)))
  {
    switch(event->response_type & 0x7F)
    {
    case XCB_BUTTON_PRESS:
      running = false;
      break;
    case XCB_EXPOSE:
      cairo_set_source_rgba(cairo, 1, 0.5, 0, 1);
      cairo_rectangle(cairo, 16, 16, 32, 32);
      cairo_fill(cairo);

      cairo_set_font_size(cairo, 64);
      cairo_move_to(cairo, w/2, h/2);
      cairo_set_source_rgba(cairo, 1, 1, 1, 1);
      cairo_show_text(cairo, "10");

      xcb_flush(xcon);
      break;
    }
    free(event);
  }

  xcb_disconnect(xcon);
}

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

  return NULL;
}
