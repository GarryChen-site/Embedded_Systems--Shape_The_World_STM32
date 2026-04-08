#include <stdio.h>    // printf, scanf — redirected to USART2 via syscalls.c
#include "uart.h"     // UART_Init(), UART_InChar(), UART_OutChar()

// Function prototype
unsigned long Calc_Area(unsigned long l, unsigned long w);

int main(void) {

    UART_Init();    // initialize USART2 at 115200 baud, 8N1

    setvbuf(stdout, NULL, _IONBF, 0); 
    setvbuf(stdin, NULL, _IONBF, 0);

    unsigned long length, width, area;
    printf("\r\nThis program calculates areas of rectangular rooms.\r\n");

    while (1) {
        printf("\r\nGive length: "); 
        scanf("%ld", &length);  
        
        printf("\r\nGive width: ");  
        scanf("%ld", &width);   
        
        area = Calc_Area(length, width);
        printf("\r\nArea of the room = %ld\r\n", area);
    }
}


unsigned long Calc_Area(unsigned long l, unsigned long w) {
    if (l < 3 || l > 20 || w < 3 || w > 20) {
        return 0;
    }
    return l * w;
}
