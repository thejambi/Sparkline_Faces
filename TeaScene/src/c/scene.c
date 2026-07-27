#include "scene.h"
#include "scene_data.h"

// Resource layout (little-endian, written by tools/make_scene.py):
//   u16 w, u16 h, u16 off[h], u16 len[h], then the row streams.
// A row stream is hybrid RLE: a byte with the high bit clear is a repeat of
// (b+1) pixels of the next byte's palette index; with the high bit set it is
// a literal run of ((b & 0x7F) + 1) indices.
#define HDR 4
#define MAX_ROW 320

static ResHandle s_res;
static uint16_t s_off[SCENE_H];
static uint16_t s_len[SCENE_H];
static uint32_t s_body;

void scene_init(void) {
  s_res = resource_get_handle(RESOURCE_ID_SCENE);
  resource_load_byte_range(s_res, HDR, (uint8_t *)s_off, sizeof s_off);
  resource_load_byte_range(s_res, HDR + sizeof s_off, (uint8_t *)s_len,
                           sizeof s_len);
  s_body = HDR + sizeof s_off + sizeof s_len;
}

void scene_draw(GContext *ctx, int top, int variant) {
  if (variant < 0 || variant > 3) variant = 1;
  const uint8_t *pal = SCENE_PAL[variant];
  GBitmap *fb = graphics_capture_frame_buffer(ctx);
  if (!fb) return;
  static uint8_t buf[MAX_ROW];

  for (int y = 0; y < SCENE_H; y++) {
    GBitmapDataRowInfo info = gbitmap_get_data_row_info(fb, top + y);
    uint8_t *dst = info.data;
    int xmax = info.max_x < SCENE_W - 1 ? info.max_x : SCENE_W - 1;
    int n = s_len[y];
    if (n > MAX_ROW) n = MAX_ROW;
    resource_load_byte_range(s_res, s_body + s_off[y], buf, n);

    int x = info.min_x, i = 0;
    while (i < n && x <= xmax) {
      uint8_t b = buf[i++];
      int run = (b & 0x7F) + 1;
      if (b & 0x80) {
        while (run-- > 0 && i < n && x <= xmax) dst[x++] = pal[buf[i++]];
      } else {
        if (i >= n) break;
        uint8_t c = pal[buf[i++]];
        while (run-- > 0 && x <= xmax) dst[x++] = c;
      }
    }
  }
  graphics_release_frame_buffer(ctx, fb);
}

// Morning starts when the room does; night when the lamp is the only light.
int scene_variant_for_hour(int hour) {
  if (hour >= 5 && hour < 11) return 0;
  if (hour >= 11 && hour < 17) return 1;
  if (hour >= 17 && hour < 21) return 2;
  return 3;
}
