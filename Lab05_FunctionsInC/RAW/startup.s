
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
  .word  _estack         
  .word  Reset_Handler   



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

/* Call static constructors */
CallLibcInit:
  bl  __libc_init_array


CallMain:
  bl  main                


LoopForever:
  b LoopForever