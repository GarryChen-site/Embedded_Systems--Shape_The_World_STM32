#include "uart.h"

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