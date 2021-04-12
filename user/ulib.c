#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/fcntl.h"
#include "user.h"

char *
strcpy(char *s, const char *t) {
  char *os;

  os = s;
  while ((*s++ = *t++) != 0);
  return os;
}

int
strcmp(const char *p, const char *q) {
  while (*p && *p == *q)
    p++, q++;
  return (uchar) * p - (uchar) * q;
}

uint
strlen(const char *s) {
  int n;

  for (n = 0; s[n]; n++);
  return n;
}

void *
memset(void *dst, int c, uint n) {
  char *cdst = (char *) dst;
  int i;
  for (i = 0; i < n; i++) {
    cdst[i] = c;
  }
  return dst;
}

char *
strchr(const char *s, char c) {
  for (; *s; s++)
    if (*s == c)
      return (char *) s;
  return 0;
}

/**
 * Read from file descriptor until a '\n' is reached.
 * keep reading one character.
 * @param fd  The file descriptor to read.
 * @param buf The buffer to put the contents read by the function.
 * @param max_size How many bytes were read at most by the function.
 * @return A space allocated by malloc function.
 */
int
readline(int fd, char *buf, int max_size) {
  char *p = buf;
  int count = 0;
  while (read(fd, p, 1) > 0) {
    count += 1;
    if (*p == '\n') {
      if (count <= max_size - 1) {
        *(p + 1) = '\0';
        return count;
      } else {
        // Not null-terminated.
        return -1;
      }
    } else if (count == max_size) {
      return -1;
    }
    p++;
  }
  return count;
}

char *
gets(char *buf, int max) {
  int i, cc;
  char c;

  for (i = 0; i + 1 < max;) {
    cc = read(0, &c, 1);
    if (cc < 1)
      break;
    buf[i++] = c;
    if (c == '\n' || c == '\r')
      break;
  }
  buf[i] = '\0';
  return buf;
}

int
stat(const char *n, struct stat *st) {
  int fd;
  int r;

  fd = open(n, O_RDONLY);
  if (fd < 0)
    return -1;
  r = fstat(fd, st);
  close(fd);
  return r;
}

int
atoi(const char *s) {
  int n;

  n = 0;
  while ('0' <= *s && *s <= '9')
    n = n * 10 + *s++ - '0';
  return n;
}

/**
 *A version of atoi, which can record whether the passed char * is a valid int or not.
 *If the number is not valid, then *result is set to 0 and return 0.
 *Otherwise, return the number.*/
int atoi_safe(const char *s, int *result) {
  int n = 0;
  while (*s != '\0') {
    // Check if *s is between '0' and '9'.
    if (*s >= '0' && *s <= '9') {
      n = n * 10 + *s++ - '0';
    } else {
      // Invalid.
      *result = 0;
      return 0;
    }
  }
  *result = 1;
  return n;
}

void *
memmove(void *vdst, const void *vsrc, int n) {
  char *dst;
  const char *src;

  dst = vdst;
  src = vsrc;
  if (src > dst) {
    while (n-- > 0)
      *dst++ = *src++;
  } else {
    dst += n;
    src += n;
    while (n-- > 0)
      *--dst = *--src;
  }
  return vdst;
}

int
memcmp(const void *s1, const void *s2, uint n) {
  const char *p1 = s1, *p2 = s2;
  while (n-- > 0) {
    if (*p1 != *p2) {
      return *p1 - *p2;
    }
    p1++;
    p2++;
  }
  return 0;
}

void *
memcpy(void *dst, const void *src, uint n) {
  return memmove(dst, src, n);
}
