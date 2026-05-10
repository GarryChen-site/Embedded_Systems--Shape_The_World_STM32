
  .syntax unified
  .cpu cortex-m4
  .thumb


  .global  g_pfnVectors
  .global  Default_Handler
  .global  Reset_Handler


  .word _sidata
  .word _sdata
  .word _edata
  .word _sbss
  .word _ebss


  .section  .isr_vector, "a", %progbits
  .type  g_pfnVectors, %object

g_pfnVectors:
  .word  _estack                /* 0:  Initial Stack Pointer       */
  .word  Reset_Handler          /* 1:  Reset                       */
  .word  NMI_Handler            /* 2:  NMI                         */
  .word  HardFault_Handler      /* 3:  Hard Fault                  */
  .word  MemManage_Handler      /* 4:  MPU Fault                   */
  .word  BusFault_Handler       /* 5:  Bus Fault                   */
  .word  UsageFault_Handler     /* 6:  Usage Fault                 */
  .word  0                      /* 7:  Reserved                    */
  .word  0                      /* 8:  Reserved                    */
  .word  0                      /* 9:  Reserved                    */
  .word  0                      /* 10: Reserved                    */
  .word  SVC_Handler            /* 11: SVCall                      */
  .word  DebugMon_Handler       /* 12: Debug Monitor               */
  .word  0                      /* 13: Reserved                    */
  .word  PendSV_Handler         /* 14: PendSV                      */
  .word  SysTick_Handler        /* 15: SysTick ← used by Lab12     */



  .section  .text.Reset_Handler
  .type  Reset_Handler, %function

Reset_Handler:

  ldr   sp, =_estack      


  /* 
   * uint32_t *src = _sidata;
   * uint32_t *dest = _sdata;
   * while(dest < _edata) { *dest = *src; dest++; src++; }
   */

  ldr r0, =_sdata         
  ldr r1, =_edata         
  ldr r2, =_sidata        

CopyDataInit:
  cmp r0, r1              
  bcc CopyDataLoop        
  b   ZeroBssInit         

CopyDataLoop:
  ldr r3, [r2], #4        
  str r3, [r0], #4        
  b   CopyDataInit        

  /* 
   * uint32_t *dest = _sbss;
   * while(dest < _ebss) { *dest = 0; dest++; }
   */
ZeroBssInit:
  ldr r0, =_sbss          
  ldr r1, =_ebss          
  movs r3, #0             

FillZeroBss:
  cmp r0, r1              
  bcc FillZeroBssLoop     
  b   CallMain            

FillZeroBssLoop:
  str r3, [r0], #4        
  b   FillZeroBss         


CallMain:
  bl  main                


LoopForever:
  b LoopForever


/* =========================================================================
 * Default_Handler — catches any enabled but unimplemented exception.
 * All system handlers below are weak aliases to this loop so the linker
 * uses them unless a strong definition exists elsewhere (e.g. main.c).
 * ========================================================================= */

  .section  .text.Default_Handler, "ax", %progbits
  .type  Default_Handler, %function

Default_Handler:
Infinite_Loop:
  b  Infinite_Loop
  .size  Default_Handler, .-Default_Handler


  .weak  NMI_Handler
  .thumb_set NMI_Handler, Default_Handler

  .weak  HardFault_Handler
  .thumb_set HardFault_Handler, Default_Handler

  .weak  MemManage_Handler
  .thumb_set MemManage_Handler, Default_Handler

  .weak  BusFault_Handler
  .thumb_set BusFault_Handler, Default_Handler

  .weak  UsageFault_Handler
  .thumb_set UsageFault_Handler, Default_Handler

  .weak  SVC_Handler
  .thumb_set SVC_Handler, Default_Handler

  .weak  DebugMon_Handler
  .thumb_set DebugMon_Handler, Default_Handler

  .weak  PendSV_Handler
  .thumb_set PendSV_Handler, Default_Handler

  .weak  SysTick_Handler
  .thumb_set SysTick_Handler, Default_Handler