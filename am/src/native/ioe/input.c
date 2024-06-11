#include <am.h>
#include <SDL.h>

#define KEYDOWN_MASK 0x8000

#define KEY_QUEUE_LEN 1024
static int key_queue[KEY_QUEUE_LEN] = {};
static int key_f = 0, key_r = 0;
static SDL_mutex *key_queue_lock = NULL;

#define XX(k) [SDL_SCANCODE_##k] = AM_KEY_##k,
static int keymap[256] = {
  AM_KEYS(XX)
};

static int event_thread(void *args) {
  SDL_Event event;
  while (1) {
    SDL_WaitEvent(&event);
    switch (event.type) {
      case SDL_QUIT: halt(0);
      case SDL_KEYDOWN:
      case SDL_KEYUP: {
        SDL_Keysym k = event.key.keysym;
        int keydown = event.key.type == SDL_KEYDOWN;
        int scancode = k.scancode;
        if (keymap[scancode] != 0) {
          int am_code = keymap[scancode] | (keydown ? KEYDOWN_MASK : 0);
          SDL_LockMutex(key_queue_lock);
          key_queue[key_r] = am_code;
          key_r = (key_r + 1) % KEY_QUEUE_LEN;
          SDL_UnlockMutex(key_queue_lock);
          void __am_send_kbd_intr();
          __am_send_kbd_intr();
        }
        break;
      }
    }
  }
}

void __am_input_init() {
  key_queue_lock = SDL_CreateMutex();
  SDL_CreateThread(event_thread, "event thread", NULL);
}

void __am_input_config(AM_INPUT_CONFIG_T *cfg) {
  cfg->present = true;
}

void __am_input_keybrd(AM_INPUT_KEYBRD_T *kbd) {
  int k = AM_KEY_NONE;

  SDL_LockMutex(key_queue_lock);
  if (key_f != key_r) {
    k = key_queue[key_f];
    key_f = (key_f + 1) % KEY_QUEUE_LEN;
  }
  SDL_UnlockMutex(key_queue_lock);

  kbd->keydown = (k & KEYDOWN_MASK ? true : false);
  kbd->keycode = k & ~KEYDOWN_MASK;
}
