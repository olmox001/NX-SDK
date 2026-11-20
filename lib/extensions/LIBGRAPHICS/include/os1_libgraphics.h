#ifndef OS1_LIBGRAPHICS_H
#define OS1_LIBGRAPHICS_H

/*
 * NexsOS LIBGRAPHICS visual UI primitives.
 *
 * LIBGRAPHICS is layered on top of the base process-local graphics surface.
 * It owns visual/theme/chrome primitives and, in later migration blocks,
 * Material UI components and DPI-aware layout. It does not own kernel surface
 * lifetime, hardware resources, window authority or compositor scene policy.
 */

#include <os1_gfx.h>
#include <os1_theme.h>

struct os1_gfx_button_geometry {
  int size;
  int top;
  int close_cx;
  int bg_cx;
};

enum os1_gfx_button_hit {
  OS1_GFX_BUTTON_NONE = 0,
  OS1_GFX_BUTTON_CLOSE = 1,
  OS1_GFX_BUTTON_BACKGROUND = 2,
};

int os1_gfx_rrect_contains(int x, int y, int width, int height, int radius);
unsigned os1_gfx_rrect_coverage(int x, int y, int width, int height,
                                int radius);
void os1_gfx_window_decoration(struct os1_gfx_surface *frame,
    const struct os1_gfx_rect *content, const os1_compositor_style_t *style,
    const os1_compositor_theme_t *theme, int focused, int protected_window);
void os1_gfx_composite_rounded(struct os1_gfx_surface *destination,
    const struct os1_gfx_surface *source, const struct os1_gfx_rect *content,
    int title_height, int radius);
void os1_gfx_button_geometry(int frame_x, int frame_width, int titlebar_y,
                             int titlebar_height, int shape, int side,
                             struct os1_gfx_button_geometry *out);
int os1_gfx_button_hit(const struct os1_gfx_button_geometry *geometry,
                       int x, int y);
uint32_t os1_gfx_titlebar_tint(int shadow_type, int y, int titlebar_height,
                               uint32_t base);
uint32_t os1_gfx_button_pixel(const struct os1_gfx_button_geometry *geometry,
                              int shape, int x, int y, uint32_t close_argb,
                              uint32_t background_argb, int drop_shadow,
                              uint32_t base);

#endif /* OS1_LIBGRAPHICS_H */
