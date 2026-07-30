#include <pebble.h>
#include "ui.h"
#include "settings.h"

static void on_settings(void) { face_fonts_changed(); face_poke(); }

int main(void) {
  settings_init(on_settings);
  face_init();
  app_event_loop();
  face_deinit();
}
