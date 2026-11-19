/*
 * NexsOS userland base graphics primitives.
 *
 * This layer owns process-local pixel surfaces, alpha composition and bounded
 * raster operations. It contains no widget, theme, window-chrome or desktop
 * policy. Resource-backed buffers and presentation are provided separately by
 * the graphics extension clients.
 */
#ifndef OS1_GFX_H
#define OS1_GFX_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

struct os1_gfx_surface {
  uint32_t *pixels;
  size_t width;
  size_t height;
  size_t stride;   /* pixels per row */
  size_t capacity; /* total allocated pixels */
};

struct os1_gfx_rect {
  int x;
  int y;
  int width;
  int height;
};

/* ARGB8888 straight-alpha storage remains the existing surface contract.
 * Premultiplied operations are explicit; both variants retain output alpha.
 * Integer channel calculations round down; opaque destinations retain the
 * historical raster results exactly. */
uint32_t os1_gfx_argb_over(uint32_t source, uint32_t destination);
uint32_t os1_gfx_argb_premultiply(uint32_t source);
uint32_t os1_gfx_premult_over(uint32_t source, uint32_t destination);

int os1_gfx_surface_valid(const struct os1_gfx_surface *surface);
void os1_gfx_clear(struct os1_gfx_surface *surface, uint32_t argb);
void os1_gfx_fill_rect(struct os1_gfx_surface *surface,
                       const struct os1_gfx_rect *rect, uint32_t argb);
void os1_gfx_composite_over(struct os1_gfx_surface *destination,
                            const struct os1_gfx_surface *source, int x,
                            int y);
void os1_gfx_composite_scaled(struct os1_gfx_surface *destination,
                              const struct os1_gfx_surface *source,
                              int x, int y, int width, int height);

#ifdef __cplusplus
}
#endif

#endif /* OS1_GFX_H */
