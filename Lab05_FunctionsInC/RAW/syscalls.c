// syscalls.c
// Newlib I/O syscall stubs — redirect printf/scanf to USART2
//
// -------------------------------------------------------------------------
// WHY THIS FILE EXISTS (Linux analogy):
//
//   On Linux, printf() → write() → kernel system call → tty driver → terminal
//
//   On bare-metal STM32 with GCC/newlib:
//   printf() → newlib's printf → newlib calls _write(fd, buf, len)
//                                           └── this file implements _write
//                                               │
//                                               ▼
//                                           UART_OutChar() → USART2 → VCP
//
//   Newlib is a C library for embedded systems. It provides printf, scanf,
//   malloc etc., but leaves I/O hooks (_write, _read) for you to implement.
//   This is the "retargeting" concept.
//
// TM4C APPROACH (original lab):
//   The TM4C UART.c overrode fputc() and fgetc() — Keil/ARM compiler hooks.
//   GCC/newlib uses _write() and _read() instead.
//   Same concept, different hook names.
//
// HOW scanf WORKS:
//   scanf() → _read() → UART_InChar() (blocks until key pressed)
//   We echo the character back so you see what you typed.
//   '\r' (Enter) signals end of input to scanf's %ld parser.
// -------------------------------------------------------------------------

#include "uart.h"
#include <sys/stat.h>

// =========================================================================
// _write — called by printf/puts/putchar
//
// Parameters:
//   file — file descriptor (1=stdout, 2=stderr; we handle all the same)
//   ptr  — pointer to the character buffer
//   len  — number of characters to write
// Returns: number of characters actually written
// =========================================================================
int _write(int file, char *ptr, int len) {
  (void)file; // ignore the file descriptor - everything goes to USART2
  for (int i = 0; i < len; i++) {
    // Convert '\n' to '\r\n'
    if (ptr[i] == '\n') {
      UART_OutChar('\r');
    }
    UART_OutChar((unsigned char)ptr[i]);
  }
  return len;
}

// =========================================================================
// _read — called by scanf/getchar/fgets
//
// Parameters:
//   file — file descriptor (0=stdin)
//   ptr  — buffer to write into
//   len  — max characters to read
// Returns: number of characters actually read
//
// We read one character at a time and echo it back (same as original
// TM4C fgetc which called UART_OutChar(ch) after UART_InChar()).
// =========================================================================
int _read(int file, char *ptr, int len) {
  (void)file;
  (void)len;

  char ch = (char)UART_InChar();

  if (ch == '\r') {
    UART_OutChar('\r');
    UART_OutChar('\n');
    ch = '\n';
  } else {
    UART_OutChar((unsigned char)ch);
  }

  *ptr = ch;
  return 1;
}

// =========================================================================
// Required stubs — newlib needs these to link even if unused
// =========================================================================
int _fstat(int file, struct stat *st) {
  (void)file;
  st->st_mode = S_IFCHR;
  return 0;
}

int _isatty(int file) {
  (void)file;
  return 1;
}

int _lseek(int file, int offset, int whence) {
  (void)file;
  (void)offset;
  (void)whence;
  return 0;
}

int _close(int file) {
  (void)file;
  return -1;
}