

void DisableInterrupts(void);
void EnableInterrupts(void);
void WaitForInterrupt(void);

int main(void) {

  EnableInterrupts();
  while (1) {
  }
}