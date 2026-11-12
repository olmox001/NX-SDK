#ifndef NEXSOS_USERKIT_SYSTEM_IMAGE_H
#define NEXSOS_USERKIT_SYSTEM_IMAGE_H

#include <stdint.h>

#define NXSYSTEM_BOOT_INIT_IMAGE_KEY "system.boot.init_image"
#define NXSYSTEM_IMAGE_CLASS_INIT "init"
#define NXSYSTEM_IMAGE_CLASS_SERVICE "service"
#define NXSYSTEM_IMAGE_NAME_MAX 24U
#define NXSYSTEM_IMAGE_PATH_MAX 96U

int nxsystem_image_identity_valid(const char *identity);
long nxsystem_image_spawn(const char *image_class, const char *identity,
                          int level, unsigned long caps);
long nxsystem_image_spawn_args(const char *image_class, const char *identity,
                               int argc, char *const argv[]);

#endif
