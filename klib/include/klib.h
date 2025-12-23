#ifndef KLIB_H__
#define KLIB_H__

#include <am.h>
#include <stddef.h>
#include <stdarg.h>
#include <klib-macros.h>

#ifdef __cplusplus
extern "C" {
#endif

//#define __NATIVE_USE_KLIB__

// string.h
void  *memset    (void *s, int c, size_t n);
void  *memcpy    (void *dst, const void *src, size_t n);
void  *memmove   (void *dst, const void *src, size_t n);
int    memcmp    (const void *s1, const void *s2, size_t n);
size_t strlen    (const char *s);
char  *strcat    (char *dst, const char *src);
char  *strcpy    (char *dst, const char *src);
char  *strncpy   (char *dst, const char *src, size_t n);
int    strcmp    (const char *s1, const char *s2);
int    strncmp   (const char *s1, const char *s2, size_t n);
char  *strchr    (const char *s, int c);
char  *strrchr   (const char *s, int c);

// stdlib.h
void   srand     (unsigned int seed);
int    rand      (void);
int    abs       (int x);
              long strtol   (const char *nptr, char **endptr, int base);
unsigned      long strtoul  (const char *nptr, char **endptr, int base);
         long long strtoll  (const char *nptr, char **endptr, int base);
unsigned long long strtoull (const char *nptr, char **endptr, int base);
int    atoi      (const char *nptr);
void  *malloc    (size_t size);
void  *calloc    (size_t nmemb, size_t size);
void  *realloc   (void *ptr, size_t size);
void   free      (void *ptr);
void   exit      (int status);

// ctypes.h
int    isspace   (int c);
int    isprint   (int c);
int    isdigit   (int c);
int    isxdigit  (int c);
int    islower   (int c);
int    isupper   (int c);
int    isalpha   (int c);
int    isalnum   (int c);
int    ispunct   (int c);
int    iscntrl   (int c);
int    isgraph   (int c);
int    tolower   (int c);
int    toupper   (int c);

// stdio.h
int    printf    (const char *format, ...);
int    vprintf   (const char *fmt, va_list ap);
int    sprintf   (char *str, const char *format, ...);
int    snprintf  (char *str, size_t size, const char *format, ...);
int    vsprintf  (char *str, const char *format, va_list ap);
int    vsnprintf (char *str, size_t size, const char *format, va_list ap);
int    vsscanf   (const char *str, const char *fmt, va_list ap);
int    sscanf    (const char *str, const char *fmt, ...);

// assert.h
#ifdef NDEBUG
  #define assert(ignore) ((void)0)
#else
  #define assert(cond) panic_on(!(cond), "Assertion `" TOSTRING(cond) "' failed")
#endif

#ifdef __cplusplus
}
#endif

#endif
