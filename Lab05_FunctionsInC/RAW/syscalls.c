#include "uart.h"
#include <sys/stat.h>   

int _write(int file, char *ptr, int len) {
    (void)file;
    for (int i=0; i < len; i++) {
        if(ptr[i] == '\n') {
            UART_OutChar('\r');
        }
        UART_OutChar(ptr[i]);
    }
    return len;
}

int _read(int file, char *ptr, int len) {
    (void)file;
    (void)len;

    char ch = UART_InChar();
    
    if(ch == '\r') {
        UART_OutChar('\r');
        UART_OutChar('\n');
        ch = '\n';
    } else {
        UART_OutChar(ch);
    }
    
    *ptr = ch;
    return 1;
}


// Required stubs
int _fstat(int file, struct stat *st) {
    (void) file;
    st->st_mode = S_IFCHR;
    return 0;
}

int _isatty(int file){
    (void) file;
    return 1;
}

int _lseek(int file, int offset, int whence){
    (void) file;
    (void) offset;
    (void) whence;
    return 0;
}

int _close(int file){
    (void) file;
    return 0;
}