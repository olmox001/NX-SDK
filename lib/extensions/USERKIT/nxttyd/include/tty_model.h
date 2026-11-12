#ifndef NEXSOS_USERKIT_TTY_MODEL_H
#define NEXSOS_USERKIT_TTY_MODEL_H
#include <stddef.h>
#include <stdint.h>

#define OS1_TTY_COLS_MAX 256
#define OS1_TTY_ROWS_MAX 128
#define OS1_TTY_SURFACE_W_MAX 1024
#define OS1_TTY_SURFACE_H_MAX 768
#define OS1_TTY_CSI_MAX 32
#define OS1_TTY_FEED_MAX 4096U
#define OS1_TTY_HISTORY_BYTES 65536U
#define OS1_TTY_CELL_FG_DEFAULT 1U
#define OS1_TTY_CELL_BG_DEFAULT 2U
#define OS1_TTY_CELL_REVERSED 4U

struct os1_tty_cell {
  uint32_t fg;
  uint32_t bg;
  uint8_t ch;
  uint8_t defaults;
};

struct os1_tty_model {
  struct os1_tty_cell cells[OS1_TTY_ROWS_MAX][OS1_TTY_COLS_MAX];
  uint8_t dirty[OS1_TTY_ROWS_MAX];
  int cols;
  int rows;
  int x;
  int y;
  int saved_x;
  int saved_y;
  int cursor_visible;
  int escape_mode;
  int escape_len;
  int bold;
  int reverse;
  int fg_uses_default;
  int bg_uses_default;
  char escape[OS1_TTY_CSI_MAX];
  uint32_t fg;
  uint32_t bg;
  uint32_t default_fg;
  uint32_t default_bg;
  size_t history_len;
  int replaying;
  char history[OS1_TTY_HISTORY_BYTES];
};

int os1_tty_grid_from_pixels(int width, int height, int cell_width,
                             int cell_height, int *cols, int *rows);
int os1_tty_init(struct os1_tty_model *model, int cols, int rows,
                 uint32_t fg, uint32_t bg);
int os1_tty_resize(struct os1_tty_model *model, int cols, int rows);
int os1_tty_set_palette(struct os1_tty_model *model,
                        uint32_t default_fg, uint32_t default_bg);
int os1_tty_feed(struct os1_tty_model *model, const char *bytes, size_t count);
#endif
