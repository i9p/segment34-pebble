#include <pebble.h>
#include <stdint.h>
#include "dithered_rects.h"
#include "segments.h"

static Window *s_window;
static Layer *s_time_layer, *s_10h_layer, *s_01h_layer, *s_col_layer, *s_10m_layer, *s_01m_layer;

static GBitmap *s_displaymask_bitmap, *s_segments_bitmap;
static GBitmap *s_segment_bitmap[33];

static GFont s_dateline_font;

static TextLayer *s_date_textlayer, *s_notification_textlayer, *s_seconds_textlayer;

static BitmapLayer *s_displaymask_layer;

static GColor bg_color = GColorWhite;
static GColor inactive_color = GColorLightGray;
static GColor gradient_top = GColorBlack;
static GColor gradient_bottom = GColorDarkGray;
static GColor date_color = GColorBlack;
static GColor notif_color = GColorBlack;

static int time_digits[4] = {0, 0, 0, 0};
static int seconds_timeout = 15;

static const char *const WEEKDAYS[] = {"SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT"};
static const char *const MONTHS[]   = {"JAN", "FEB", "MAR", "APR", "MAY", "JUN", 
                                       "JUL", "AUG", "SEP", "OCT", "NOV", "DEC"};
static struct tm *tick_time;

static void draw_time_layer(Layer *layer, GContext *ctx) {
  draw_gradient_rect(ctx, GRect(0, 1, 198, 76), gradient_top, gradient_bottom, TOP_TO_BOTTOM);
}

static void draw_single_seg(GContext *ctx, int segment) {
  graphics_draw_bitmap_in_rect(ctx, s_segment_bitmap[segment], GRect(segment_pos[segment].x, segment_pos[segment].y, segment_bounds[segment].size.w, segment_bounds[segment].size.h));
}

static void draw_segmented_text(GContext *ctx, int segment) {
  for (int i = 0; i < 33; i++) {
    if (!segment_maps[segment][i]) {
      draw_single_seg(ctx, i);
    }
  }
}

static void draw_segment_10h(Layer *layer, GContext *ctx) {
  graphics_context_set_compositing_mode(ctx, GCompOpSet);
  draw_segmented_text(ctx, time_digits[0]);
}

static void draw_segment_01h(Layer *layer, GContext *ctx) {
  graphics_context_set_compositing_mode(ctx, GCompOpSet);
  draw_segmented_text(ctx, time_digits[1]);
}

static void draw_segment_col(Layer *layer, GContext *ctx) {
  graphics_context_set_compositing_mode(ctx, GCompOpSet);
  draw_segmented_text(ctx, 10);
}

static void draw_segment_10m(Layer *layer, GContext *ctx) {
  graphics_context_set_compositing_mode(ctx, GCompOpSet);
  draw_segmented_text(ctx, time_digits[2]);
}

static void draw_segment_01m(Layer *layer, GContext *ctx) {
  graphics_context_set_compositing_mode(ctx, GCompOpSet);
  draw_segmented_text(ctx, time_digits[3]);
}

static void handle_tick(struct tm* current_time, TimeUnits units_changed) {
  static char date_buffer[20];
  static char sec_buffer[3];
  if (units_changed & HOUR_UNIT) {
    if (clock_is_24h_style()) {
      time_digits[0] = current_time->tm_hour / 10;
      time_digits[1] = current_time->tm_hour % 10;
    } else {
      int hour12 = current_time->tm_hour % 12;
      if (hour12 == 0) hour12 += 12;
      time_digits[0] = hour12 / 10;
      time_digits[1] = hour12 % 10;
    }
    layer_mark_dirty(s_10h_layer);
    layer_mark_dirty(s_01h_layer);

    snprintf(date_buffer, sizeof(date_buffer), "%s, %d %s %d",
             WEEKDAYS[current_time->tm_wday],
             current_time->tm_mday,
             MONTHS[current_time->tm_mon],
             current_time->tm_year + 1900);
    text_layer_set_text(s_date_textlayer, date_buffer);
  }

  if (units_changed & MINUTE_UNIT) {
    time_digits[2] = current_time->tm_min / 10;
    time_digits[3] = current_time->tm_min % 10;

    layer_mark_dirty(s_10m_layer);
    layer_mark_dirty(s_01m_layer);
  }

  if (seconds_timeout > 0) {
    strftime(sec_buffer, sizeof(sec_buffer), "%S", current_time);
    text_layer_set_text(s_seconds_textlayer, sec_buffer);
    seconds_timeout--;
  } else if (seconds_timeout == 0) {

    text_layer_set_text(s_seconds_textlayer, "--");
    seconds_timeout--;
    tick_timer_service_unsubscribe();
    tick_timer_service_subscribe(MINUTE_UNIT, &handle_tick);
  }
}

static void handle_accel_tap(AccelAxisType axis, int32_t direction) {
  seconds_timeout = 15;

  tick_timer_service_unsubscribe();
  tick_timer_service_subscribe(SECOND_UNIT, &handle_tick);
}

static void set_layers_update_procs() {
  layer_set_update_proc(s_time_layer, draw_time_layer);
  layer_set_update_proc(s_10h_layer, draw_segment_10h);
  layer_set_update_proc(s_01h_layer, draw_segment_01h);
  layer_set_update_proc(s_col_layer, draw_segment_col);
  layer_set_update_proc(s_10m_layer, draw_segment_10m);
  layer_set_update_proc(s_01m_layer, draw_segment_01m);
}

static void layers_add_children(Layer *window_layer) {
  layer_add_child(window_layer, s_time_layer);
  layer_add_child(s_time_layer, s_10h_layer);
  layer_add_child(s_time_layer, s_01h_layer);
  layer_add_child(s_time_layer, s_col_layer);
  layer_add_child(s_time_layer, s_10m_layer);
  layer_add_child(s_time_layer, s_01m_layer);
  layer_add_child(s_time_layer, bitmap_layer_get_layer(s_displaymask_layer));
  layer_add_child(s_time_layer, text_layer_get_layer(s_date_textlayer));
  layer_add_child(s_time_layer, text_layer_get_layer(s_seconds_textlayer));
}

static void set_dateline_style(TextLayer *text_layer, GColor textlayer_color) {
  text_layer_set_font(text_layer, fonts_load_custom_font(resource_get_handle(RESOURCE_ID_TERMINUS_16)));
  text_layer_set_text_color(text_layer, textlayer_color);
  text_layer_set_background_color(text_layer, GColorClear);
}

static void prv_window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  window_set_background_color(window, bg_color);
  s_displaymask_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_DISPLAY_MASK);
  s_segments_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_SEGMENTS);

  s_time_layer = layer_create(GRect(1, 72, 198, 76+18));
  s_displaymask_layer = bitmap_layer_create(GRect(0, 0, 198, 76));

  s_10h_layer = layer_create(GRect(0, 0, 38, 76));
  s_01h_layer = layer_create(GRect(40, 0, 38, 76));
  s_col_layer = layer_create(GRect(80, 0, 38, 76));
  s_10m_layer = layer_create(GRect(120, 0, 38, 76));
  s_01m_layer = layer_create(GRect(160, 0, 38, 76));

  s_date_textlayer = text_layer_create(GRect(2, 76, 148, 18));
  set_dateline_style(s_date_textlayer, date_color);
  text_layer_set_text(s_date_textlayer, "ERROR!");

  s_seconds_textlayer = text_layer_create(GRect(180, 76, 16, 18));
  set_dateline_style(s_seconds_textlayer, date_color);
  text_layer_set_text(s_seconds_textlayer, "--");

  GColor *palette = gbitmap_get_palette(s_displaymask_bitmap);
  palette[1] = bg_color;
  GColor *palette2 = gbitmap_get_palette(s_segments_bitmap);
  palette2[1] = inactive_color;
  bitmap_layer_set_compositing_mode(s_displaymask_layer, GCompOpSet);
  bitmap_layer_set_bitmap(s_displaymask_layer, s_displaymask_bitmap);

  for (int i = 0; i < 33; i++) {
    s_segment_bitmap[i] = gbitmap_create_as_sub_bitmap(s_segments_bitmap, segment_bounds[i]);
  }

  set_layers_update_procs();

  layers_add_children(window_layer);

  time_t now = time(NULL);
	tick_time = localtime(&now);
	handle_tick(tick_time, SECOND_UNIT | MINUTE_UNIT | HOUR_UNIT);
}

static void prv_window_unload(Window *window) {
  tick_timer_service_unsubscribe();
  accel_tap_service_unsubscribe();
  
  for (int i = 0; i < 33; i++) {
    gbitmap_destroy(s_segment_bitmap[i]);
  }

  gbitmap_destroy(s_segments_bitmap);
  gbitmap_destroy(s_displaymask_bitmap);

  bitmap_layer_destroy(s_displaymask_layer);

  text_layer_destroy(s_date_textlayer);
  text_layer_destroy(s_seconds_textlayer);
  layer_destroy(s_10h_layer);
  layer_destroy(s_01h_layer);
  layer_destroy(s_col_layer);
  layer_destroy(s_10m_layer);
  layer_destroy(s_01m_layer);
  layer_destroy(s_time_layer);
}

static void prv_init(void) {
  s_window = window_create();

  s_dateline_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_TERMINUS_16));

  tick_timer_service_subscribe(SECOND_UNIT, &handle_tick);
  accel_tap_service_subscribe(handle_accel_tap);

  window_set_window_handlers(s_window, (WindowHandlers) {
    .load = prv_window_load,
    .unload = prv_window_unload,
  });
  const bool animated = true;
  window_stack_push(s_window, animated);
}

static void prv_deinit(void) {
  
  window_destroy(s_window);
}

int main(void) {
  prv_init();

  APP_LOG(APP_LOG_LEVEL_DEBUG, "Done initializing, pushed window: %p", s_window);

  app_event_loop();
  prv_deinit();
}
