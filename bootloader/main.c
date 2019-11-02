#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/cm3/scb.h>

#define SYSTEM_MEMORY_BASE 0x1FFF0000
// Keep in sync with custom.devices.data
#define MAIN_PROGRAM_BASE 0x08004000

void jump_to_addr(uint32_t addr);

void jump_to_addr(uint32_t addr) {

  // TODO AN2606 instructs for some cleanup to happen before jumping to
  // system memory:
  // 
  // - Disable all peripheral clocks
  // - Disable used PLL
  // - Disable interrupts
  // - Clear pending interrupts
  // 
  // However the code below should suffice as long as the bootloader
  // code does not touch any of these.

  // Set vector table base address.
  SCB_VTOR = MAIN_PROGRAM_BASE;
  // Initialize master stack pointer.
  asm volatile("msr msp, %0"::"g"(*(volatile uint32_t *)addr));
  // Jump to application.
  ((void (*)(void)) *((uint32_t*) (addr + 4)))();
}

int main(void) {
  int run_bootloader = (
    (RCC_CSR & RCC_CSR_PINRSTF) // PIN reset flag
    &&
    !(
      RCC_CSR & (
        RCC_CSR_LPWRRSTF | // Low-power reset flag
        RCC_CSR_WWDGRSTF | // Window watchdog reset flag
        RCC_CSR_IWDGRSTF | // Independent watchdog reset flag
        RCC_CSR_SFTRSTF | // Software reset flag
        RCC_CSR_PORRSTF | // POR/PDR reset flag
        RCC_CSR_BORRSTF // BOR reset flag
      )
    )
  );
  RCC_CSR |= RCC_CSR_RMVF;

  if(run_bootloader) {
    jump_to_addr(SYSTEM_MEMORY_BASE);
  } else {
    jump_to_addr(MAIN_PROGRAM_BASE);
  }
}
