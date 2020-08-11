#include <klib.h>
#include <SDL2/SDL.h>

#define SBUF_SIZE_MAX 65536
static uint8_t sbuf [SBUF_SIZE_MAX] = {};
static int sbuf_size = 0;
static int head = 0, tail = 0;
static volatile int count = 0;

static void audio_play(void *userdata, uint8_t *stream, int len) {
  int nread = len;
  if (count < len) nread = count;

  if (nread + tail < sbuf_size) {
    memcpy(stream, sbuf + tail, nread);
    tail += nread;
  } else {
    int first_cpy_len = sbuf_size - tail;
    memcpy(stream, sbuf + tail, first_cpy_len);
    memcpy(stream + first_cpy_len, sbuf, nread - first_cpy_len);
    tail = nread - first_cpy_len;
  }
  count -= nread;
  if (len > nread) memset(stream + nread, 0, len - nread);
}

static int audio_write(uint8_t *buf, int len) {
  int free = sbuf_size - count;
  int nwrite = len;
  if (free < len) nwrite = free;

  if (nwrite + head < sbuf_size) {
    memcpy(sbuf + head, buf, nwrite);
    head += nwrite;
  } else {
    int first_cpy_len = sbuf_size - head;
    memcpy(sbuf + head, buf, first_cpy_len);
    memcpy(sbuf, buf + first_cpy_len, nwrite - first_cpy_len);
    head = nwrite - first_cpy_len;
  }
  count += nwrite;
  return nwrite;
}

void __am_audio_ctrl(AM_AUDIO_CTRL_T *ctrl) {
  SDL_AudioSpec s = {};
  s.freq = ctrl->freq;
  s.format = AUDIO_S16SYS;
  s.channels = ctrl->channels;
  s.samples = ctrl->samples;
  s.callback = audio_play;
  s.userdata = NULL;
  sbuf_size = ctrl->bufsize;
  assert(sbuf_size <= SBUF_SIZE_MAX);

  head = tail = 0;
  count = 0;
  int ret = SDL_InitSubSystem(SDL_INIT_AUDIO);
  if (ret == 0) {
    SDL_OpenAudio(&s, NULL);
    SDL_PauseAudio(0);
  }
}

void __am_audio_status(AM_AUDIO_STATUS_T *stat) {
  stat->count = count;
}

void __am_audio_play(AM_AUDIO_PLAY_T *ctl) {
  int len = ctl->buf.end - ctl->buf.start;
  assert(len <= sbuf_size);
  while (sbuf_size - count < len);
  audio_write(ctl->buf.start, len);
}
