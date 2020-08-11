#include "x86-qemu.h"
#include <klib.h> // TODO: delete

// UART
// ====================================================

#define COM1 0x3f8

static int uart_init() {
  outb(COM1 + 2, 0);
  outb(COM1 + 3, 0x80);
  outb(COM1 + 0, 115200 / 9600);
  outb(COM1 + 1, 0);
  outb(COM1 + 3, 0x03);
  outb(COM1 + 4, 0);
  outb(COM1 + 1, 0x01);
  inb (COM1 + 2);
  inb (COM1 + 0);
  return 0;
}

static void uart_config(AM_UART_CONFIG_T *cfg) {
  cfg->present = true;
}

static void uart_tx(AM_UART_TX_T *send) {
  outb(COM1, send->data);
}

static void uart_rx(AM_UART_RX_T *recv) {
  recv->data = (inb(COM1 + 5) & 0x1) ? inb(COM1) : -1;
}

// Timer
// ====================================================

static AM_TIMER_RTC_T boot_date;
static uint32_t freq_mhz = 2000;
static uint64_t uptsc;
static void timer_rtc(AM_TIMER_RTC_T *rtc);

static inline int read_rtc(int reg) {
  outb(0x70, reg);
  int ret = inb(0x71);
  return (ret & 0xf) + (ret >> 4) * 10;
}

static void read_rtc_async(AM_TIMER_RTC_T *rtc) {
  *rtc = (AM_TIMER_RTC_T) {
    .second = read_rtc(0),
    .minute = read_rtc(2),
    .hour   = read_rtc(4),
    .day    = read_rtc(7),
    .month  = read_rtc(8),
    .year   = read_rtc(9) + 2000,
  };
}

static void wait_sec(AM_TIMER_RTC_T *t1) {
  AM_TIMER_RTC_T t0;
  while (1) {
    read_rtc_async(&t0);
    for (int volatile i = 0; i < 100000; i++) ;
    read_rtc_async(t1);
    if (t0.second != t1->second) {
      return;
    }
  }
}

static uint32_t estimate_freq() {
  AM_TIMER_RTC_T rtc1, rtc2;
  uint64_t tsc1, tsc2, t1, t2;
  wait_sec(&rtc1); tsc1 = rdtsc(); t1 = rtc1.minute * 60 + rtc1.second;
  wait_sec(&rtc2); tsc2 = rdtsc(); t2 = rtc2.minute * 60 + rtc2.second;
  if (t1 >= t2) return estimate_freq(); // passed an hour; try again
  return ((tsc2 - tsc1) >> 20) / (t2 - t1);
}

static void timer_init() {
  freq_mhz = estimate_freq();
  timer_rtc(&boot_date);
  uptsc = rdtsc();
}

static void timer_config(AM_TIMER_CONFIG_T *cfg) {
  cfg->present = cfg->has_rtc = true;
}

static void timer_rtc(AM_TIMER_RTC_T *rtc) {
  int tmp;
  do {
    read_rtc_async(rtc);
    tmp = read_rtc(0);
  } while (tmp != rtc->second);
}

static void timer_uptime(AM_TIMER_UPTIME_T *upt) {
  upt->us = (rdtsc() - uptsc) / freq_mhz;
}

// Input
// ====================================================

static int keylut[128] = {
  [0x01] = AM_KEY_ESCAPE,               [0x02] = AM_KEY_1, [0x03] = AM_KEY_2,
  [0x04] = AM_KEY_3, [0x05] = AM_KEY_4, [0x06] = AM_KEY_5, [0x07] = AM_KEY_6,
  [0x08] = AM_KEY_7, [0x09] = AM_KEY_8, [0x0a] = AM_KEY_9, [0x0b] = AM_KEY_0,
  [0x0c] = AM_KEY_MINUS,                [0x0d] = AM_KEY_EQUALS,
  [0x0e] = AM_KEY_BACKSPACE,            [0x0f] = AM_KEY_TAB,
  [0x10] = AM_KEY_Q, [0x11] = AM_KEY_W, [0x12] = AM_KEY_E, [0x13] = AM_KEY_R,
  [0x14] = AM_KEY_T, [0x15] = AM_KEY_Y, [0x16] = AM_KEY_U, [0x17] = AM_KEY_I,
  [0x18] = AM_KEY_O, [0x19] = AM_KEY_P, [0x1a] = AM_KEY_LEFTBRACKET,
  [0x1b] = AM_KEY_RIGHTBRACKET,         [0x1c] = AM_KEY_RETURN,
  [0x1d] = AM_KEY_LCTRL,                [0x1e] = AM_KEY_A, [0x1f] = AM_KEY_S,
  [0x20] = AM_KEY_D, [0x21] = AM_KEY_F, [0x22] = AM_KEY_G, [0x23] = AM_KEY_H,
  [0x24] = AM_KEY_J, [0x25] = AM_KEY_K, [0x26] = AM_KEY_L,
  [0x27] = AM_KEY_SEMICOLON,            [0x28] = AM_KEY_APOSTROPHE,
  [0x29] = AM_KEY_GRAVE,                [0x2a] = AM_KEY_LSHIFT,
  [0x2b] = AM_KEY_BACKSLASH,            [0x2c] = AM_KEY_Z, [0x2d] = AM_KEY_X,
  [0x2e] = AM_KEY_C, [0x2f] = AM_KEY_V, [0x30] = AM_KEY_B, [0x31] = AM_KEY_N,
  [0x32] = AM_KEY_M,     [0x33] = AM_KEY_COMMA,  [0x34] = AM_KEY_PERIOD,
  [0x35] = AM_KEY_SLASH, [0x36] = AM_KEY_RSHIFT, [0x38] = AM_KEY_LALT,
  [0x38] = AM_KEY_RALT,  [0x39] = AM_KEY_SPACE,  [0x3a] = AM_KEY_CAPSLOCK,
  [0x3b] = AM_KEY_F1,    [0x3c] = AM_KEY_F2,     [0x3d] = AM_KEY_F3,
  [0x3e] = AM_KEY_F4,    [0x3f] = AM_KEY_F5,     [0x40] = AM_KEY_F6,
  [0x41] = AM_KEY_F7,    [0x42] = AM_KEY_F8,     [0x43] = AM_KEY_F9,
  [0x44] = AM_KEY_F10,   [0x48] = AM_KEY_INSERT,
  [0x4b] = AM_KEY_HOME,  [0x4d] = AM_KEY_END,    [0x50] = AM_KEY_DELETE,
  [0x57] = AM_KEY_F11,   [0x58] = AM_KEY_F12,    [0x5b] = AM_KEY_APPLICATION,
};

static void input_config(AM_INPUT_CONFIG_T *cfg) {
  cfg->present = true;
}

static void input_keybrd(AM_INPUT_KEYBRD_T *ev) {
  if (inb(0x64) & 0x1) {
    int code = inb(0x60) & 0xff;
    ev->keydown = code < 128;
    ev->keycode = keylut[code & 0x7f];
  } else {
    ev->keydown = false;
    ev->keycode = AM_KEY_NONE;
  }
}

// GPU (Frame Buffer and 2D Accelerated Graphics)
// ====================================================

#define VMEM_SIZE (512 << 10)

struct vbe_info {
  uint8_t  ignore[18];
  uint16_t width;
  uint16_t height;
  uint8_t  ignore1[18];
  uint32_t framebuffer;
} __attribute__ ((packed));

static inline uint8_t R(uint32_t p) { return p >> 16; }
static inline uint8_t G(uint32_t p) { return p >> 8; }
static inline uint8_t B(uint32_t p) { return p; }

struct pixel {
  uint8_t b, g, r;
} __attribute__ ((packed));

static struct pixel *fb;
static uint8_t vmem[VMEM_SIZE], vbuf[VMEM_SIZE], *vbuf_head;

static struct gpu_canvas display;
static inline void *to_host(gpuptr_t ptr) { return ptr == AM_GPU_NULL ? NULL : vmem + ptr; }

static void gpu_init() {
  struct vbe_info *info = (struct vbe_info *)0x00004000;
  display.w = info->width;
  display.h = info->height;
  fb = (void *)((intptr_t)(info->framebuffer));
}

static void gpu_config(AM_GPU_CONFIG_T *cfg) {
  *cfg = (AM_GPU_CONFIG_T) {
    .present = true,
    .width = display.w, .height = display.h,
    .vmemsz  = sizeof(vmem),
  };
}

static void gpu_fbdraw(AM_GPU_FBDRAW_T *draw) {
  int x = draw->x, y = draw->y, w = draw->w, h = draw->h;
  int W = display.w, H = display.h;
  uint32_t *pixels = draw->pixels;
  int len = (x + w >= W) ? W - x : w;
  for (int j = 0; j < h; j ++, pixels += w) {
    if (y + j < H) {
      struct pixel *px = &fb[x + (j + y) * W];
      for (int i = 0; i < len; i ++, px ++) {
        uint32_t p = pixels[i];
        *px = (struct pixel) { .r = R(p), .g = G(p), .b = B(p) };
      }
    }
  }
}

static void gpu_status(AM_GPU_STATUS_T *stat) {
  stat->ready = true;
}

static void gpu_memcpy(AM_GPU_MEMCPY_T *params) {
  char *src = params->src, *dst = to_host(params->dest);
  for (int i = 0; i < params->size; i++)
    dst[i] = src[i];
}

static void *vbuf_alloc(int size) {
  void *ret = vbuf_head;
  vbuf_head += size;
  panic_on(vbuf_head > vbuf + sizeof(vbuf), "no memory");
  for (int i = 0; i < size; i++)
    ((char *)ret)[i] = 0;
  return ret;
}

static struct pixel *render(struct gpu_canvas *cv, struct gpu_canvas *parent, struct pixel *px) {
  struct pixel *px_local;
  int W = parent->w, w, h;

  switch (cv->type) {
    case AM_GPU_TEXTURE: {
      w = cv->texture.w; h = cv->texture.h;
      px_local = to_host(cv->texture.pixels);
      break;
    }
    case AM_GPU_SUBTREE: {
      w = cv->w; h = cv->h;
      px_local = vbuf_alloc(w * h * sizeof(struct pixel));
      for (struct gpu_canvas *ch = to_host(cv->child); ch; ch = to_host(ch->sibling)) {
        render(ch, cv, px_local);
      }
      break;
    }
    default:
      panic("invalid node");
  }

  // draw local canvas (w * h) -> px (x1, y1) - (x1 + w1, y1 + h1)
  for (int i = 0; i < cv->w1; i++)
    for (int j = 0; j < cv->h1; j++) {
      int x = cv->x1 + i, y = cv->y1 + j;
      px[W * y + x] = px_local[w * (j * h / cv->h1) + (i * w / cv->w1)];
    }
  return 0;
}

static void gpu_render(AM_GPU_RENDER_T *ren) {
  vbuf_head = vbuf;
  render(to_host(ren->root), &display, fb);
}

// Disk (ATA0)
// ====================================================

#define BLKSZ  512
#define DISKSZ (64 << 20)

static void disk_config(AM_DISK_CONFIG_T *cfg) {
  cfg->present = true;
  cfg->blksz   = BLKSZ;
  cfg->blkcnt  = DISKSZ / BLKSZ;
}

static void disk_status(AM_DISK_STATUS_T *status) {
  status->ready = true;
}

static inline void wait_disk(void) {
  while ((inb(0x1f7) & 0xc0) != 0x40);
}

static void disk_blkio(AM_DISK_BLKIO_T *bio) {
  uint32_t blkno = bio->blkno, remain = bio->blkcnt;
  uint32_t *ptr = bio->buf;
  for (remain = bio->blkcnt; remain; remain--, blkno++) {
    wait_disk();
    outb(0x1f2, 1);
    outb(0x1f3, blkno);
    outb(0x1f4, blkno >> 8);
    outb(0x1f5, blkno >> 16);
    outb(0x1f6, (blkno >> 24) | 0xe0);
    outb(0x1f7, bio->write? 0x30 : 0x20);
    wait_disk();
    if (bio->write) {
      for (int i = 0; i < BLKSZ / 4; i ++)
        outl(0x1f0, *ptr++);
    } else {
      for (int i = 0; i < BLKSZ / 4; i ++)
        *ptr++ = inl(0x1f0);
    }
  }
}

// ====================================================

static void audio_config(AM_AUDIO_CONFIG_T *cfg) { cfg->present = false; }
static void net_config(AM_NET_CONFIG_T *cfg) { cfg->present = false; }
static void fail(void *buf) { panic("access nonexist register"); }

typedef void (*handler_t)(void *buf);
static void *lut[128] = {
  [AM_UART_CONFIG ] = uart_config,
  [AM_UART_TX     ] = uart_tx,
  [AM_UART_RX     ] = uart_rx,
  [AM_TIMER_CONFIG] = timer_config,
  [AM_TIMER_RTC   ] = timer_rtc,
  [AM_TIMER_UPTIME] = timer_uptime,
  [AM_INPUT_CONFIG] = input_config,
  [AM_INPUT_KEYBRD] = input_keybrd,
  [AM_GPU_CONFIG  ] = gpu_config,
  [AM_GPU_FBDRAW  ] = gpu_fbdraw,
  [AM_GPU_STATUS  ] = gpu_status,
  [AM_GPU_MEMCPY  ] = gpu_memcpy,
  [AM_GPU_RENDER  ] = gpu_render,
  [AM_AUDIO_CONFIG] = audio_config,
  [AM_DISK_CONFIG ] = disk_config,
  [AM_DISK_STATUS ] = disk_status,
  [AM_DISK_BLKIO  ] = disk_blkio,
  [AM_NET_CONFIG  ] = net_config,
};


bool ioe_init() {
  panic_on(cpu_current() != 0, "init IOE in non-bootstrap CPU");

  for (int i = 0; i < LENGTH(lut); i++)
    if (!lut[i]) lut[i] = fail;

  uart_init();
  timer_init();
  gpu_init();

  return true;
}

void ioe_read (int reg, void *buf) { ((handler_t)lut[reg])(buf); }
void ioe_write(int reg, void *buf) { ((handler_t)lut[reg])(buf); }

// LAPIC/IOAPIC (from xv6)

#define ID      (0x0020/4)   // ID
#define VER     (0x0030/4)   // Version
#define TPR     (0x0080/4)   // Task Priority
#define EOI     (0x00B0/4)   // EOI
#define SVR     (0x00F0/4)   // Spurious Interrupt Vector
  #define ENABLE     0x00000100   // Unit Enable
#define ESR     (0x0280/4)   // Error Status
#define ICRLO   (0x0300/4)   // Interrupt Command
  #define INIT       0x00000500   // INIT/RESET
  #define STARTUP    0x00000600   // Startup IPI
  #define DELIVS     0x00001000   // Delivery status
  #define ASSERT     0x00004000   // Assert interrupt (vs deassert)
  #define DEASSERT   0x00000000
  #define LEVEL      0x00008000   // Level triggered
  #define BCAST      0x00080000   // Send to all APICs, including self.
  #define BUSY       0x00001000
  #define FIXED      0x00000000
#define ICRHI   (0x0310/4)   // Interrupt Command [63:32]
#define TIMER   (0x0320/4)   // Local Vector Table 0 (TIMER)
  #define X1         0x0000000B   // divide counts by 1
  #define PERIODIC   0x00020000   // Periodic
#define PCINT   (0x0340/4)   // Performance Counter LVT
#define LINT0   (0x0350/4)   // Local Vector Table 1 (LINT0)
#define LINT1   (0x0360/4)   // Local Vector Table 2 (LINT1)
#define ERROR   (0x0370/4)   // Local Vector Table 3 (ERROR)
  #define MASKED     0x00010000   // Interrupt masked
#define TICR    (0x0380/4)   // Timer Initial Count
#define TCCR    (0x0390/4)   // Timer Current Count
#define TDCR    (0x03E0/4)   // Timer Divide Configuration

#define IOAPIC_ADDR  0xFEC00000   // Default physical address of IO APIC
#define REG_ID     0x00  // Register index: ID
#define REG_VER    0x01  // Register index: version
#define REG_TABLE  0x10  // Redirection table base

#define INT_DISABLED   0x00010000  // Interrupt disabled
#define INT_LEVEL      0x00008000  // Level-triggered (vs edge-)
#define INT_ACTIVELOW  0x00002000  // Active low (vs high)
#define INT_LOGICAL    0x00000800  // Destination is CPU id (vs APIC ID)

volatile unsigned int *__am_lapic = NULL;  // Initialized in mp.c
struct IOAPIC {
    uint32_t reg, pad[3], data;
} __attribute__((packed));
typedef struct IOAPIC IOAPIC;

static volatile IOAPIC *ioapic;

static void lapicw(int index, int value) {
  __am_lapic[index] = value;
  __am_lapic[ID];
}

void __am_percpu_initlapic(void) {
  lapicw(SVR, ENABLE | (T_IRQ0 + IRQ_SPURIOUS));
  lapicw(TDCR, X1);
  lapicw(TIMER, PERIODIC | (T_IRQ0 + IRQ_TIMER));
  lapicw(TICR, 10000000);
  lapicw(LINT0, MASKED);
  lapicw(LINT1, MASKED);
  if (((__am_lapic[VER]>>16) & 0xFF) >= 4)
    lapicw(PCINT, MASKED);
  lapicw(ERROR, T_IRQ0 + IRQ_ERROR);
  lapicw(ESR, 0);
  lapicw(ESR, 0);
  lapicw(EOI, 0);
  lapicw(ICRHI, 0);
  lapicw(ICRLO, BCAST | INIT | LEVEL);
  while(__am_lapic[ICRLO] & DELIVS) ;
  lapicw(TPR, 0);
}

void __am_lapic_eoi(void) {
  if (__am_lapic)
    lapicw(EOI, 0);
}

void __am_lapic_bootap(uint32_t apicid, void *addr) {
  int i;
  uint16_t *wrv;
  outb(0x70, 0xF);
  outb(0x71, 0x0A);
  wrv = (unsigned short*)((0x40<<4 | 0x67));
  wrv[0] = 0;
  wrv[1] = (uintptr_t)addr >> 4;

  lapicw(ICRHI, apicid<<24);
  lapicw(ICRLO, INIT | LEVEL | ASSERT);
  lapicw(ICRLO, INIT | LEVEL);

  for (i = 0; i < 2; i++){
    lapicw(ICRHI, apicid<<24);
    lapicw(ICRLO, STARTUP | ((uintptr_t)addr>>12));
  }
}

static unsigned int ioapicread(int reg) {
  ioapic->reg = reg;
  return ioapic->data;
}

static void ioapicwrite(int reg, unsigned int data) {
  ioapic->reg = reg;
  ioapic->data = data;
}

void __am_ioapic_init(void) {
  int i, maxintr;

  ioapic = (volatile IOAPIC*)IOAPIC_ADDR;
  maxintr = (ioapicread(REG_VER) >> 16) & 0xFF;

  for (i = 0; i <= maxintr; i++){
    ioapicwrite(REG_TABLE+2*i, INT_DISABLED | (T_IRQ0 + i));
    ioapicwrite(REG_TABLE+2*i+1, 0);
  }
}

void __am_ioapic_enable(int irq, int cpunum) {
  ioapicwrite(REG_TABLE+2*irq, T_IRQ0 + irq);
  ioapicwrite(REG_TABLE+2*irq+1, cpunum << 24);
}
