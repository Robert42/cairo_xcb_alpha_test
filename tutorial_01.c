#include <xcb/xcb.h>
#include <assert.h>

int main(int, char**)
{
  xcb_connection_t *xcon;

  xcon = xcb_connect(NULL, NULL);
  assert(!xcb_connection_has_error(xcon));

  xcb_disconnect(xcon);
}