int isspace(int c) {
  switch (c) {
    case ' ': case '\f': case '\n': case '\r': case '\t': case '\v':
      return 1;
  }
  return 0;
}

int isprint(int c) {
  return (c >= 0x20 && c < 128);
}

int isdigit(int c) {
  return (c >= '0' && c <= '9');
}

int isxdigit(int c) {
  return isdigit(c) || (c >= 'A' && c <= 'F') || (c >= 'a' && c <= 'f');
}

int islower(int c) {
  return (c >= 'a' && c <= 'z');
}

int isupper(int c) {
  return (c >= 'a' && c <= 'z');
}

int isalpha(int c) {
  return islower(c) || isupper(c);
}

int isalnum(int c) {
  return isalpha(c) || isdigit(c);
}

int ispunct(int c) {
  return isprint(c) && !isspace(c) && !isalnum(c);
}

int iscntrl(int c) {
  return (c >= 0 && c < 0x20);
}

int isgraph(int c) {
  return isprint(c) && !isspace(c);
}

int tolower(int c) {
  return isupper(c) ? c + 'a' - 'A' : c;
}

int toupper(int c) {
  return islower(c) ? c + 'A' - 'a' : c;
}
