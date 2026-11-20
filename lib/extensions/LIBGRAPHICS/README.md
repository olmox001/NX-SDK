# LIBGRAPHICS

LIBGRAPHICS is the NX-SDK visual UI extension layered above `lib/gfx`.

Ownership:
- `lib/gfx`: process-local surfaces, alpha/raster primitives and the future
  process-local surface graph/resource-facing common graphics logic.
- `lib/extensions/graphics`: resource/buffer/surface client adapters.
- `lib/extensions/LIBGRAPHICS`: visual primitives, themes, chrome, Material UI
  tokens/layout/components, DPI/density and orientation-aware UI behavior.

LIBGRAPHICS must not own kernel object lifetime, display hardware, compositor
scene authority or device policy. The first migration block is behavior
preserving: it moves existing userland visual implementations here without
duplicating them.

C callers include `os1_libgraphics.h`. C++ callers include
`os1_libgraphics_cxx.h` for C linkage to the same SDK symbols.
