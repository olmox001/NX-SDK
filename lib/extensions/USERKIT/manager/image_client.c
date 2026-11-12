#include <nxsystem_image.h>
#include <os1.h>
#include <stdio.h>
#include <string.h>

int nxsystem_image_identity_valid(const char *identity) {
  if (identity == NULL || identity[0] == '\0')
    return 0;
  for (unsigned i = 0U; i < NXSYSTEM_IMAGE_NAME_MAX; ++i) {
    const char ch = identity[i];
    if (ch == '\0')
      return 1;
    if (!((ch >= 'a' && ch <= 'z') || (ch >= '0' && ch <= '9') ||
          ch == '_' || ch == '-'))
      return 0;
  }
  return 0;
}

static int nxsystem_image_resolve(const char *image_class,
                                  const char *identity,
                                  char path[NXSYSTEM_IMAGE_PATH_MAX]) {
  if (image_class == NULL || path == NULL ||
      !nxsystem_image_identity_valid(identity))
    return -EINVAL;
  if (strcmp(image_class, NXSYSTEM_IMAGE_CLASS_INIT) != 0 &&
      strcmp(image_class, NXSYSTEM_IMAGE_CLASS_SERVICE) != 0)
    return -EINVAL;

  char key[64] = {0};
  int count = snprintf(key, sizeof(key), "system.boot.image.%s.%s",
                       image_class, identity);
  if (count <= 0 || count >= (int)sizeof(key))
    return -EOVERFLOW;
  int status = OS1_registry_object_get(key, path, NXSYSTEM_IMAGE_PATH_MAX);
  if (status != 0 || path[0] == '\0')
    return status != 0 ? status : -ENOENT;
  return 0;
}

long nxsystem_image_spawn(const char *image_class, const char *identity,
                          int level, unsigned long caps) {
  char path[NXSYSTEM_IMAGE_PATH_MAX] = {0};
  int status = nxsystem_image_resolve(image_class, identity, path);
  return status == 0 ? spawn_caps(path, level, caps) : status;
}

long nxsystem_image_spawn_args(const char *image_class, const char *identity,
                               int argc, char *const argv[]) {
  if (argc <= 0 || argc > 3 || argv == NULL)
    return -EINVAL;
  char path[NXSYSTEM_IMAGE_PATH_MAX] = {0};
  int status = nxsystem_image_resolve(image_class, identity, path);
  if (status != 0)
    return status;
  char *resolved[3] = {path, NULL, NULL};
  for (int i = 1; i < 3 && i < argc; ++i)
    resolved[i] = argv[i];
  return spawn_args(path, argc, resolved);
}
