#include "dot_matrix.h"

void draw_5x7_digit(GContext *ctx, int digit, int start_x, int start_y, int size, int spacing, GColor active, GColor inactive) {
    if (digit < 0 || digit > 13) return;
    
    for (int col = 0; col < 5; col++) {
        uint8_t column_data = FONT_5X7_DIGITS[digit][col];
        for (int row = 0; row < 7; row++) {
          if ((column_data >> row) & 0x01) {
            graphics_context_set_stroke_color(ctx, active);
          } else {
            graphics_context_set_stroke_color(ctx, inactive);
          }
          graphics_draw_rect(ctx, GRect(start_x + col*(size+spacing), start_y + row*(size+spacing), size, size));
        }
    }
}

void fmt_steps(int steps) {
  if (steps < 10000) {
    steps_display[3] = steps % 10;
    steps_display[2] = (steps > 9) ? steps / 10 % 10 : 11;
    steps_display[1] = (steps > 99) ? steps / 100 % 10 : 11;
    steps_display[0] = (steps > 999) ? steps / 1000 % 10 : 11;
  } else {
    steps_display[3] = 12;
    steps_display[2] = steps / 1000 % 10;
    steps_display[1] = steps / 10000 % 10;
    steps_display[0] = (steps > 99999) ? steps / 100000 % 10 : 11; // ???
  }
}

void fmt_hr(int hr) {
  hr_display[2] = hr % 10;
  hr_display[1] = (hr > 9) ? hr / 10 % 10 : 11; // dude i fucking hope lmao
  hr_display[0] = (hr > 99) ? hr / 100 % 10 : 11;
}

void fmt_sleep(int sleep) {
  int sleep_hrs = sleep / 360;
  if (sleep_hrs < 100) {
    sleep_display[3] = 13;
    sleep_display[2] = sleep_hrs % 10;
    sleep_display[1] = 10;
    sleep_display[0] = sleep_hrs / 10 % 10;
  } else {
    sleep_display[3] = 13;
    sleep_display[2] = sleep_hrs / 10 % 10;
    sleep_display[1] = sleep_hrs / 100 % 10;
    sleep_display[0] = 11;
  }
}

void draw_steps(GContext *ctx, GColor active, GColor inactive) {
  for (int i = 0; i < 4; i++) {
    draw_5x7_digit(ctx, steps_display[i], 16*i, 0, 2, 1, active, inactive);
  }
}

void draw_hr(GContext *ctx, GColor active, GColor inactive) {
  for (int i = 0; i < 3; i++) {
    draw_5x7_digit(ctx, hr_display[i], 16*i, 0, 2, 1, active, inactive);
  }
}

void draw_sleep(GContext *ctx, GColor active, GColor inactive) {
  for (int i = 0; i < 4; i++) {
    draw_5x7_digit(ctx, sleep_display[i], 16*i, 0, 2, 1, active, inactive);
  }
}
