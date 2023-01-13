#include <am.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#define BLKSZ 512

static int disk_size = 0;
static FILE *fp = NULL;

void __am_disk_init() {
  const char *diskimg = getenv("diskimg");
  if (diskimg) {
    fp = fopen(diskimg, "r+");
    if (fp) {
      fseek(fp, 0, SEEK_END);
      disk_size = (ftell(fp) + 511) / 512;
      rewind(fp);
    }
  }
}

void __am_disk_config(AM_DISK_CONFIG_T *cfg) {
  cfg->present = (fp != NULL);
  cfg->blksz = BLKSZ;
  cfg->blkcnt = disk_size;
}

void __am_disk_status(AM_DISK_STATUS_T *stat) {
  stat->ready = 1;
}

void __am_disk_blkio(AM_DISK_BLKIO_T *io) {
  if (fp) {
    fseek(fp, io->blkno * BLKSZ, SEEK_SET);
    int ret;
    if (io->write) ret = fwrite(io->buf, io->blkcnt * BLKSZ, 1, fp);
    else ret = fread(io->buf, io->blkcnt * BLKSZ, 1, fp);
    assert(ret == 1);
  }
}
