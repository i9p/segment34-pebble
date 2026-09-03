#include <pebble.h>
#include "dithered_rects.h"
#include "segments.h"
#include "dot_matrix.h"

static Window *s_window;
static Layer *s_time_layer, *s_10h_layer, *s_01h_layer, *s_col_layer, *s_10m_layer, *s_01m_layer;

static GBitmap *s_displaymask_bitmap, *s_segments_bitmap;
static GBitmap *s_segment_bitmap[33];

static GFont s_dateline_font;

static TextLayer *s_weather1_textlayer, *s_weather2_textlayer;

static Layer *s_health_layer;
static TextLayer *s_date_textlayer, *s_notification_textlayer, *s_seconds_textlayer;
static TextLayer *s_steps_textlayer, *s_hr_textlayer, *s_sleep_textlayer;
static Layer *s_stepsval_layer, *s_hrval_layer, *s_sleepval_layer;

static BitmapLayer *s_displaymask_layer;

static GColor bg_color = GColorBlack;
static GColor inactive_color = GColorDarkGray;
static GColor gradient_top = GColorRed;
static GColor gradient_bottom = GColorBlue;
static GColor date_color = GColorCeleste;
static GColor notif_color = GColorWhite;
static GColor weather_color = GColorWhite;
static GColor health_label_color = GColorWhite;
static GColor health_active_color = GColorWhite;

static int time_digits[4] = {0, 0, 0, 0};
static int seconds_timeout = 15;

static const char *const WEEKDAYS[] = {"SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT"};
static const char *const MONTHS[]   = {"JAN", "FEB", "MAR", "APR", "MAY", "JUN", 
                                       "JUL", "AUG", "SEP", "OCT", "NOV", "DEC"};
static struct tm *tick_time;

static void draw_time_layer(Layer *layer, GContext *ctx) {
  draw_gradient_rect(ctx, GRect(0, 1, 198, 76), gradient_top, gradient_bottom, TOP_TO_BOTTOM);
}

static void draw_segment_10h(Layer *layer, GContext *ctx) {
  graphics_context_set_compositing_mode(ctx, GCompOpSet);
  draw_segmented_text(ctx, s_segment_bitmap, time_digits[0]);
}

static void draw_segment_01h(Layer *layer, GContext *ctx) {
  graphics_context_set_compositing_mode(ctx, GCompOpSet);
  draw_segmented_text(ctx, s_segment_bitmap, time_digits[1]);
}

static void draw_segment_col(Layer *layer, GContext *ctx) {
  graphics_context_set_compositing_mode(ctx, GCompOpSet);
  draw_segmented_text(ctx, s_segment_bitmap, 10);
}

static void draw_segment_10m(Layer *layer, GContext *ctx) {
  graphics_context_set_compositing_mode(ctx, GCompOpSet);
  draw_segmented_text(ctx, s_segment_bitmap, time_digits[2]);
}

static void draw_segment_01m(Layer *layer, GContext *ctx) {
  graphics_context_set_compositing_mode(ctx, GCompOpSet);
  draw_segmented_text(ctx, s_segment_bitmap, time_digits[3]);
}

static void draw_steps_value(Layer *layer, GContext *ctx) {
  draw_steps(ctx, health_active_color, inactive_color);
}

static void draw_hr_value(Layer *layer, GContext *ctx) {
  draw_hr(ctx, health_active_color, inactive_color);
}

static void draw_sleep_value(Layer *layer, GContext *ctx) {
  draw_sleep(ctx, health_active_color, inactive_color);
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

    if (health_service_metric_accessible(HealthMetricStepCount, time_start_of_today(), time(NULL))
        == HealthServiceAccessibilityMaskAvailable) {
      fmt_steps(health_service_sum_today(HealthMetricStepCount));
    }

    if (health_service_metric_accessible(HealthMetricHeartRateBPM, time(NULL), time(NULL))
        == HealthServiceAccessibilityMaskAvailable) {
      fmt_hr(health_service_peek_current_value(HealthMetricHeartRateBPM));
    }

    if (health_service_metric_accessible(HealthMetricSleepSeconds, time_start_of_today(), time(NULL))
        == HealthServiceAccessibilityMaskAvailable) {
      fmt_sleep(health_service_sum_today(HealthMetricSleepSeconds));
    }

    layer_mark_dirty(s_stepsval_layer);
    layer_mark_dirty(s_sleepval_layer);
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

  time_t now = time(NULL);
	tick_time = localtime(&now);
	handle_tick(tick_time, SECOND_UNIT | MINUTE_UNIT | HOUR_UNIT);

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

  layer_set_update_proc(s_stepsval_layer, draw_steps_value);
  layer_set_update_proc(s_hrval_layer, draw_hr_value);
  layer_set_update_proc(s_sleepval_layer, draw_sleep_value);
}

static void layers_add_children(Layer *window_layer) {
  layer_add_child(window_layer, s_time_layer);
  layer_add_child(window_layer, text_layer_get_layer(s_weather1_textlayer));
  layer_add_child(window_layer, text_layer_get_layer(s_weather2_textlayer));

  layer_add_child(window_layer, s_health_layer);
  layer_add_child(s_health_layer, text_layer_get_layer(s_steps_textlayer));
  layer_add_child(s_health_layer, text_layer_get_layer(s_hr_textlayer));
  layer_add_child(s_health_layer, text_layer_get_layer(s_sleep_textlayer));

  layer_add_child(s_health_layer, s_stepsval_layer);
  layer_add_child(s_health_layer, s_hrval_layer);
  layer_add_child(s_health_layer, s_sleepval_layer);

  layer_add_child(s_time_layer, s_10h_layer);
  layer_add_child(s_time_layer, s_01h_layer);
  layer_add_child(s_time_layer, s_col_layer);
  layer_add_child(s_time_layer, s_10m_layer);
  layer_add_child(s_time_layer, s_01m_layer);
  layer_add_child(s_time_layer, bitmap_layer_get_layer(s_displaymask_layer));
  layer_add_child(s_time_layer, text_layer_get_layer(s_date_textlayer));
  layer_add_child(s_time_layer, text_layer_get_layer(s_seconds_textlayer));
}

static void set_text_style(TextLayer *text_layer, GColor textlayer_color, int font_id) {
  text_layer_set_font(text_layer, fonts_load_custom_font(resource_get_handle(font_id)));
  text_layer_set_text_color(text_layer, textlayer_color);
  text_layer_set_background_color(text_layer, GColorClear);
}

static void prv_window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  window_set_background_color(window, bg_color);
  s_displaymask_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_DISPLAY_MASK);
  s_segments_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_SEGMENTS);

  GColor *palette = gbitmap_get_palette(s_displaymask_bitmap);
  palette[1] = bg_color;
  GColor *palette2 = gbitmap_get_palette(s_segments_bitmap);
  palette2[1] = inactive_color;

  s_time_layer = layer_create(GRect(1, 72, 198, 76+18));

  s_displaymask_layer = bitmap_layer_create(GRect(0, 0, 198, 76));
  bitmap_layer_set_compositing_mode(s_displaymask_layer, GCompOpSet);
  bitmap_layer_set_bitmap(s_displaymask_layer, s_displaymask_bitmap);

  s_10h_layer = layer_create(GRect(0, 0, 38, 76));
  s_01h_layer = layer_create(GRect(40, 0, 38, 76));
  s_col_layer = layer_create(GRect(80, 0, 38, 76));
  s_10m_layer = layer_create(GRect(120, 0, 38, 76));
  s_01m_layer = layer_create(GRect(160, 0, 38, 76));

  s_weather1_textlayer = text_layer_create(GRect(0, 24, 200, 24));
  set_text_style(s_weather1_textlayer, weather_color, RESOURCE_ID_TERMINUS_BOLD_22);
  text_layer_set_text_alignment(s_weather1_textlayer, GTextAlignmentCenter);
  text_layer_set_text(s_weather1_textlayer, "13C 13km/h 84%");

  s_weather2_textlayer = text_layer_create(GRect(0, 44, 200, 24));
  set_text_style(s_weather2_textlayer, weather_color, RESOURCE_ID_TERMINUS_BOLD_22);
  text_layer_set_text_alignment(s_weather2_textlayer, GTextAlignmentCenter);
  text_layer_set_text(s_weather2_textlayer, "HEAVY SHOWERS");

  s_date_textlayer = text_layer_create(GRect(2, 76, 148, 18));
  set_text_style(s_date_textlayer, date_color, RESOURCE_ID_TERMINUS_16);
  text_layer_set_text(s_date_textlayer, "ERROR!");

  s_seconds_textlayer = text_layer_create(GRect(180, 76, 16, 18));
  set_text_style(s_seconds_textlayer, date_color, RESOURCE_ID_TERMINUS_16);
  text_layer_set_text(s_seconds_textlayer, "--");

  s_health_layer = layer_create(GRect(0, 170, 200, 34));

  s_steps_textlayer = text_layer_create(GRect(6, 0, 53, 14));
  set_text_style(s_steps_textlayer, health_label_color, RESOURCE_ID_TERMINUS_12);
  text_layer_set_text(s_steps_textlayer, "STEPS:");
  s_stepsval_layer = layer_create(GRect(6, 14, 16*5, 20));

  s_hr_textlayer = text_layer_create(GRect(76, 0, 53, 14));
  set_text_style(s_hr_textlayer, health_label_color, RESOURCE_ID_TERMINUS_12);
  text_layer_set_text(s_hr_textlayer, "HR:");
  s_hrval_layer = layer_create(GRect(76, 14, 16*3, 20));

  s_sleep_textlayer = text_layer_create(GRect(130, 0, 53, 14));
  set_text_style(s_sleep_textlayer, health_label_color, RESOURCE_ID_TERMINUS_12);
  text_layer_set_text(s_sleep_textlayer, "SLEEP:");
  s_sleepval_layer = layer_create(GRect(130, 14, 16*5, 20));


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
  
  bitmap_layer_destroy(s_displaymask_layer);

  for (int i = 0; i < 33; i++) {
    gbitmap_destroy(s_segment_bitmap[i]);
  }

  gbitmap_destroy(s_segments_bitmap);
  gbitmap_destroy(s_displaymask_bitmap);

  text_layer_destroy(s_weather1_textlayer);
  text_layer_destroy(s_weather2_textlayer);

  text_layer_destroy(s_steps_textlayer);
  layer_destroy(s_stepsval_layer);

  text_layer_destroy(s_hr_textlayer);

  text_layer_destroy(s_sleep_textlayer);

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
