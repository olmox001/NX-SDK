/*
 * Temporary NexsOS Gnulib FTS compatibility stub.
 *
 * Real FTS traversal requires fd-relative directory primitives that the
 * current NX-SDK does not yet provide completely. Keep the public Gnulib
 * contract linkable for this checkpoint, but fail every traversal operation
 * explicitly with ENOSYS. The real implementation is deferred to the next
 * portability gate; this file must not be treated as functional FTS support.
 */
#include <errno.h>
#include "fts_.h"

FTS *fts_open(char *const *paths, int options,
              int (*compare)(const FTSENT **, const FTSENT **)) {
  (void)paths;
  (void)options;
  (void)compare;
  errno = ENOSYS;
  return NULL;
}

FTSENT *fts_read(FTS *stream) {
  (void)stream;
  errno = ENOSYS;
  return NULL;
}

FTSENT *fts_children(FTS *stream, int instruction) {
  (void)stream;
  (void)instruction;
  errno = ENOSYS;
  return NULL;
}

int fts_set(FTS *stream, FTSENT *entry, int instruction) {
  (void)stream;
  (void)entry;
  (void)instruction;
  errno = ENOSYS;
  return -1;
}

int fts_close(FTS *stream) {
  (void)stream;
  errno = ENOSYS;
  return -1;
}
