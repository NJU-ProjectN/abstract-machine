#include <klib.h>

#if !defined(__ISA_NATIVE__) || defined(__NATIVE_USE_KLIB__)

int printf(const char *fmt, ...) {
  panic("Not implemented");
}

int vprintf(const char *fmt, va_list ap) {
  panic("Not implemented");
}

int vsprintf(char *out, const char *fmt, va_list ap) {
  panic("Not implemented");
}

int sprintf(char *out, const char *fmt, ...) {
  panic("Not implemented");
}

int snprintf(char *out, size_t n, const char *fmt, ...) {
  panic("Not implemented");
}

int vsnprintf(char *out, size_t n, const char *fmt, va_list ap) {
  panic("Not implemented");
}

int __am_vsscanf_internal(const char *str, const char **end_pstr, const char *fmt, va_list ap) {
  const char *pstr = str;
  const char *pfmt = fmt;
  int item = -1;
  while (*pfmt) {
    char ch = *pfmt ++;
    if (isspace(ch)) {
      for (ch = *pfmt; isspace(ch); ch = *(++ pfmt));
      for (ch = *pstr; isspace(ch); ch = *(++ pstr));
      item ++;
      continue;
    }
    switch (ch) {
      case '%': break;
      default:
        if (*pstr == ch) { // match
          pstr ++;
          item ++;
          continue;
        }
        goto end; // fail
    }

    char *p;
    ch = *pfmt ++;
    switch (ch) {
      // conversion specifier
      case 'd':
        *(va_arg(ap, int *)) = strtol(pstr, &p, 10);
        if (p == pstr) goto end; // fail
        pstr = p;
        item ++;
        break;

      case 'c':
        *(va_arg(ap, char *)) = *pstr ++;
        item ++;
        break;

      default:
        printf("Unsupported conversion specifier '%c'\n", ch);
        assert(0);
    }
  }

end:
  if (end_pstr) {
    *end_pstr = pstr;
  }
  return item;
}

int vsscanf(const char *str, const char *fmt, va_list ap) {
  return __am_vsscanf_internal(str, NULL, fmt, ap);
}

int sscanf(const char *str, const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  int r = vsscanf(str, fmt, ap);
  va_end(ap);
  return r;
}

int __isoc99_sscanf(const char *str, const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  int r = vsscanf(str, fmt, ap);
  va_end(ap);
  return r;
}

#endif
