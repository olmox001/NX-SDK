/* SDK-owned keyboard layouts. The OS selects the active layout through
 * OS1_KEYBOARD_LAYOUT_REGISTRY_KEY; nxkeyboardd only routes translated input. */
#ifndef NXS_KEYBOARD_LAYOUT_H
#define NXS_KEYBOARD_LAYOUT_H

#include <os1_keyboard_layout_provider.h>

#define NXS_KBD_MAC19ITA 1
#define NXS_KBD_US 2
#define NXS_KBD_DEFAULT "mac19ita"

#endif
