#ifndef OS1_SURFACE_H
#define OS1_SURFACE_H
#include <surface.h>
#include <os1_compositor.h>
/* Hardware resource client; handles may instead be delegated by a supervisor.
 * Notification carries a generation, not a pulse, so wakeups cannot be lost. */
long OS1_surface_events_open(unsigned rights);
long OS1_surface_state(int handle, struct os1_surface_state_v1 *state);
long OS1_surface_wait(int handle, uint64_t generation);
long OS1_surface_invalidate(void);
long OS1_surface_present(struct os1_compositor_present_v1 *present);
long OS1_display_mode_query(uint32_t index, struct os1_display_mode_v1 *mode);
long OS1_display_mode_set(uint32_t index, uint64_t generation);
long OS1_display_workspace_query(struct os1_display_workspace_v1 *workspace);
long OS1_display_workspace_set(uint32_t mode, uint32_t width,
                               uint32_t height, uint32_t zoom_percent);
long OS1_surface_snapshot(struct os1_window_snapshot_v1 *snapshot);
long OS1_surface_move(int id, int x, int y);
/* Declarative plugin: a surface's anchor, edge reservation and overlay Z
 * order are independent of its executable name. Geometry is resolved by the
 * resource provider whenever the logical workspace changes. */
long OS1_surface_layout_apply(int window_id, unsigned anchor, unsigned layer,
                              unsigned order, unsigned margin,
                              unsigned reserved, unsigned width,
                              unsigned height);
long OS1_surface_layout_query(int window_id,
                              struct os1_surface_layout_v1 *layout);
long OS1_input_resource_read(struct os1_raw_input_event_v1 *event,
                             unsigned role, unsigned mode);
#endif
