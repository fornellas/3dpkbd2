#include <libopencm3/stm32/rcc.h>

#define PIN_RESET ( \
  (RCC_CSR & RCC_CSR_PINRSTF) \
  && \
  !( \
    RCC_CSR & ( \
      RCC_CSR_LPWRRSTF | \
      RCC_CSR_WWDGRSTF | \
      RCC_CSR_IWDGRSTF | \
      RCC_CSR_SFTRSTF | \
      RCC_CSR_PORRSTF | \
      RCC_CSR_BORRSTF \
    ) \
  ) \
)
