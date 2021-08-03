#include <am.h>
#include <klib-macros.h>

static void fail(void *buf) { panic("access nonexist register"); }

bool ioe_init() {
  return false;
}

void ioe_read (int reg, void *buf) { fail(buf); }
void ioe_write(int reg, void *buf) { fail(buf); }
