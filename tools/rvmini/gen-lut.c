#include <stdio.h>
#include <assert.h>

#define LUT_BIN "lut.bin"

int main() {
  FILE *fp = fopen(LUT_BIN, "w");
  assert(fp != NULL);

#define gen_table(name, row, col, entry_size, expr) do { \
  for (int j = 0; j < row; j ++) { \
    for (int i = 0; i < col; i ++) { \
      unsigned res = (expr); \
      fwrite(&res, entry_size, 1, fp); \
    } \
  } \
} while (0)

  gen_table(_and8_table, 256, 256, 1, j & i);
  gen_table(_or8_table,  256, 256, 1, j | i);
  gen_table(_xor8_table, 256, 256, 1, j ^ i);
  gen_table(_sll8_table, 32, 256, 4, (unsigned)i << j);
  gen_table(_srl8_table, 32, 256, 4, ((unsigned)i << 24) >> j);
  gen_table(_sra8_table, 32, 256, 4, (int)((unsigned)i << 24) >> j);

  fclose(fp);

  return 0;
}
