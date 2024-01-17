#include <am.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <assert.h>
#include <errno.h>

void __am_uart_init() {
  int ret = fcntl(STDIN_FILENO, F_GETFL);
  assert(ret != -1);
  int flag = ret | O_NONBLOCK;
  ret = fcntl(STDIN_FILENO, F_SETFL, flag);
  assert(ret != -1);
}

void __am_uart_config(AM_UART_CONFIG_T *cfg) {
  cfg->present = true;
}

void __am_uart_tx(AM_UART_TX_T *uart) {
  putchar(uart->data);
}

void __am_uart_rx(AM_UART_RX_T *uart) {
  int ret = fgetc(stdin);
  if (ret == EOF) ret = -1;
  uart->data = ret;
}
