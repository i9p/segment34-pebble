#include "segments.h"

void draw_single_seg(GContext *ctx, GBitmap *bitmap, int segment) {
  graphics_draw_bitmap_in_rect(ctx, bitmap, GRect(segment_pos[segment].x, segment_pos[segment].y, segment_bounds[segment].size.w, segment_bounds[segment].size.h));
}

void draw_segmented_text(GContext *ctx, GBitmap *segment_bitmaps[], int segment) {
  for (int i = 0; i < 33; i++) {
    if (!segment_maps[segment][i]) {
      draw_single_seg(ctx, segment_bitmaps[i], i);
    }
  }
}

