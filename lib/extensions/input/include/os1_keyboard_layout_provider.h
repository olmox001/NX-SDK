#ifndef OS1_KEYBOARD_LAYOUT_PROVIDER_H
#define OS1_KEYBOARD_LAYOUT_PROVIDER_H

#define OS1_KEYBOARD_LAYOUT_REGISTRY_KEY "keyboard.layout"

struct os1_keyboard_state {
  unsigned shift_keys;
  unsigned ctrl_keys;
  int caps;
};

struct os1_keyboard_key {
  unsigned code;
  int state;
  unsigned char key;
  char utf8[4];
};

/* Build-selected keyboard provider contract. No layout policy lives in SDK. */
int os1_keyboard_layout_default(void);
int os1_keyboard_layout_id(const char *name);
const char *os1_keyboard_layout_utf8(int layout, unsigned code, int shift);
int os1_keyboard_translate(struct os1_keyboard_state *state, unsigned code,
                           int value, int layout,
                           struct os1_keyboard_key *out);

#endif
