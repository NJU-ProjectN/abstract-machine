#include <am.h>
#include <SDL2/SDL.h>

//#define MODE_800x600
#ifdef MODE_800x600
# define W    800
# define H    600
#else
# define W    400
# define H    300
#endif

#define FPS   60

static SDL_Window *window = NULL;
static SDL_Renderer *renderer = NULL;

static SDL_Texture *texture = NULL;
static uint32_t fb[W * H] = {};

static inline int min(int x, int y) { return (x < y) ? x : y; }

static Uint32 texture_sync(Uint32 interval, void *param) {
  SDL_UpdateTexture(texture, NULL, fb, W * sizeof(Uint32));
  SDL_RenderClear(renderer);
  SDL_RenderCopy(renderer, texture, NULL, NULL);
  SDL_RenderPresent(renderer);
  return interval;
}

void __am_gpu_init() {
  SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER);
  SDL_CreateWindowAndRenderer(
#ifdef MODE_800x600
      W, H,
#else
      W * 2, H * 2,
#endif
      0, &window, &renderer);
  SDL_SetWindowTitle(window, "Native Application");
  texture = SDL_CreateTexture(renderer,
    SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STATIC, W, H);
  memset(fb, 0, W * H * sizeof(uint32_t));
  SDL_AddTimer(1000 / FPS, texture_sync, NULL);
}

void __am_gpu_config(AM_GPU_CONFIG_T *cfg) {
  *cfg = (AM_GPU_CONFIG_T) {
    .present = true, .has_accel = false,
    .width = W, .height = H,
    .vmemsz = 0
  };
}

void __am_gpu_status(AM_GPU_STATUS_T *stat) {
  stat->ready = true;
}

void __am_gpu_fbdraw(AM_GPU_FBDRAW_T *ctl) {
  int x = ctl->x, y = ctl->y, w = ctl->w, h = ctl->h;
  uint32_t *pixels = ctl->pixels;
  int cp_bytes = sizeof(uint32_t) * min(w, W - x);
  for (int j = 0; j < h && y + j < H; j ++) {
    memcpy(&fb[(y + j) * W + x], pixels, cp_bytes);
    pixels += w;
  }
}
