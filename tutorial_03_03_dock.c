#include <xcb/xcb.h>
#include <cairo-xcb.h>
#include <err.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <signal.h>
#include <string.h>

#define TRANSPARENCY 1

#define SPLASHSCREEN 1

const int x=16, y=16, w=256, h=256;
xcb_connection_t *xcon = NULL;
xcb_drawable_t xwindow = 0;

int countdown = 10;

void handle_timeout(int);
xcb_visualtype_t* get_xvisual(xcb_screen_t *screen, uint8_t depth);
xcb_atom_t atom(const char* name);
void set_property_uint32(xcb_atom_t prop_name, xcb_atom_t prop_type, uint32_t value);

int main(int, char**)
{
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

#if TRANSPARENCY
  const uint8_t depth = 32;
#else
  const uint8_t depth = xscreen->root_depth;
#endif
  xcb_visualtype_t* xvisual = get_xvisual(xscreen, depth);

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
    xcon, // connection
    depth,
    xwindow, // window is
    xscreen->root,
    x, y, w, h,
    0,  // border width
    XCB_WINDOW_CLASS_INPUT_OUTPUT, // _class
    xvisual->visual_id,
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
#if SPLASHSCREEN
  set_property_uint32(atom("_NET_WM_WINDOW_TYPE"), XCB_ATOM_ATOM, atom("_NET_WM_WINDOW_TYPE_SPLASH"));
#endif

  xcb_map_window(xcon, xwindow);

  xcb_flush(xcon);

  if(signal(SIGALRM, handle_timeout) == SIG_ERR) errx(-1, "Cout not set up timer");
  alarm(1);

  xcb_generic_event_t* event;
  bool running = true;
  while(running && (event = xcb_wait_for_event(xcon)))
  {
    running = countdown > 0;
    switch(event->response_type & 0x7F)
    {
    case XCB_BUTTON_PRESS:
      running = false;
      break;
    case XCB_EXPOSE:
      char countdown_text[3] = {};
      snprintf(countdown_text, sizeof(countdown_text), "%i", countdown);

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

      // rectangle
      cairo_set_source_rgba(cairo, 1, 0.5, 0, 0.5);
      cairo_rectangle(cairo, 21, 21, 71, 71);
      cairo_fill(cairo);

      cairo_set_font_size(cairo, 64);
      cairo_move_to(cairo, w/2, h/2);
      cairo_set_source_rgba(cairo, 1, 1, 1, 1);
      cairo_show_text(cairo, countdown_text);

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

  errx(-1, "No visual found");
}


void handle_timeout(int)
{
  countdown--;
  alarm(1);
  xcb_expose_event_t event = {
    .response_type = XCB_EXPOSE,
    .width = w,
    .height = h,
  };
  xcb_send_event(xcon, false, xwindow, XCB_EVENT_MASK_EXPOSURE, (char*)&event);
  xcb_flush(xcon);
}

xcb_atom_t atom(const char* name)
{
  xcb_intern_atom_cookie_t xcookie = xcb_intern_atom(xcon, 0, strlen(name), name);
  xcb_intern_atom_reply_t* xatom_reply = xcb_intern_atom_reply(xcon, xcookie, NULL);
  if(!xatom_reply)
    errx(1, "xcb atom reply failed for %s", name);

  xcb_atom_t atom = xatom_reply->atom;
  free(xatom_reply);
  return atom;
}

void set_property_uint32(xcb_atom_t prop_name, xcb_atom_t prop_type, uint32_t value)
{
  xcb_change_property(xcon, XCB_PROP_MODE_REPLACE, xwindow,
    prop_name,
    prop_type,
    32, 1, &value// list of 32 bit atoms with the length 1
  );
}