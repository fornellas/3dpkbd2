#include "lib/display_hal.h"
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/spi.h>
#include <libopencm3/stm32/common/spi_common_all.h>
#include "lib/systick.h"

#define RCC_RST   RCC_GPIOA
#define RST_PORT  GPIOA
#define RST_GPIO  GPIO4

#define RCC_CD    RCC_GPIOA
#define CD_PORT   GPIOA
#define CD_GPIO   GPIO3

#define RCC_CS    RCC_GPIOA
#define CS_PORT   GPIOA
#define CS_GPIO   GPIO2

#define RCC_SPI         RCC_SPI1
#define SPI_PERIPHERAL  SPI1

#define RCC_SCK         RCC_GPIOA
#define SCK_PORT        GPIOA
#define SCK_GPIO        GPIO5

#define RCC_MOSI        RCC_GPIOA
#define MOSI_PORT       GPIOA
#define MOSI_GPIO       GPIO7

static inline void spi_wait_not_busy(void);

static inline void spi_wait_not_busy() {
  while (SPI_SR(SPI_PERIPHERAL) & SPI_SR_BSY);
}

void ucg_com_cm3_4wire_HW_SPI_power_up(ucg_t *ucg, ucg_com_info_t *ucg_com_info) {
  uint32_t serial_clk_hz;
  uint32_t spi_cr1_baudrate;

  (void)ucg;

  rcc_periph_clock_enable(RCC_RST);
  gpio_mode_setup(RST_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, RST_GPIO);

  rcc_periph_clock_enable(RCC_CD);
  gpio_mode_setup(CD_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, CD_GPIO);

  rcc_periph_clock_enable(RCC_CS);
  gpio_mode_setup(CS_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, CS_GPIO);

  rcc_periph_clock_enable(RCC_SCK);
  gpio_mode_setup(SCK_PORT, GPIO_MODE_AF, GPIO_PUPD_NONE, SCK_GPIO);
  gpio_set_af(SCK_PORT, GPIO_AF5, SCK_GPIO);

  rcc_periph_clock_enable(RCC_MOSI);
  gpio_mode_setup(MOSI_PORT, GPIO_MODE_AF, GPIO_PUPD_NONE, MOSI_GPIO);
  gpio_set_af(MOSI_PORT, GPIO_AF5, MOSI_GPIO);

  rcc_periph_clock_enable(RCC_SPI);
  serial_clk_hz = 1000000000UL / (ucg_com_info->serial_clk_speed);
  if(serial_clk_hz < (rcc_ahb_frequency / 128))
    spi_cr1_baudrate = SPI_CR1_BAUDRATE_FPCLK_DIV_256;
  else if(serial_clk_hz < (rcc_ahb_frequency / 64))
    spi_cr1_baudrate = SPI_CR1_BAUDRATE_FPCLK_DIV_128;
  else if(serial_clk_hz < (rcc_ahb_frequency / 32))
    spi_cr1_baudrate = SPI_CR1_BAUDRATE_FPCLK_DIV_64;
  else if(serial_clk_hz < (rcc_ahb_frequency / 16))
    spi_cr1_baudrate = SPI_CR1_BAUDRATE_FPCLK_DIV_32;
  else if(serial_clk_hz < (rcc_ahb_frequency / 8))
    spi_cr1_baudrate = SPI_CR1_BAUDRATE_FPCLK_DIV_16;
  else if(serial_clk_hz < (rcc_ahb_frequency / 4))
    spi_cr1_baudrate = SPI_CR1_BAUDRATE_FPCLK_DIV_8;
  else if(serial_clk_hz < (rcc_ahb_frequency / 2))
    spi_cr1_baudrate = SPI_CR1_BAUDRATE_FPCLK_DIV_4;
  else
    spi_cr1_baudrate = SPI_CR1_BAUDRATE_FPCLK_DIV_2;
  
  spi_init_master(
    SPI_PERIPHERAL,
    spi_cr1_baudrate,
    SPI_CR1_CPOL_CLK_TO_0_WHEN_IDLE,
    SPI_CR1_CPHA_CLK_TRANSITION_1,
    SPI_CR1_DFF_8BIT,
    SPI_CR1_MSBFIRST
  );
  spi_enable(SPI_PERIPHERAL);
}

void ucg_com_cm3_4wire_HW_SPI_power_down(ucg_t *ucg) {
  (void)ucg;

  spi_disable(SPI_PERIPHERAL);
}

void ucg_com_cm3_4wire_HW_SPI_delay(ucg_t *ucg, uint16_t microseconds) {
  (void) ucg;

  uint16_t milliseconds;
  spi_wait_not_busy();
  if(microseconds < 1000)
    // FIXME we're delaying 1ms for any value < 1000us
    milliseconds = 1;
  else
    milliseconds = microseconds / 1000;
  delay_ms(milliseconds);
}

void ucg_com_cm3_4wire_HW_SPI_change_reset_line(ucg_t *ucg, uint8_t state) {
  (void) ucg;

  spi_wait_not_busy();
  if(state)
    gpio_set(RST_PORT, RST_GPIO);
  else
    gpio_clear(RST_PORT, RST_GPIO);
}

void ucg_com_cm3_4wire_HW_SPI_change_cd_line(ucg_t *ucg, uint8_t state) {
  (void) ucg;

  spi_wait_not_busy();
  if(state)
    gpio_set(CD_PORT, CD_GPIO);
  else
    gpio_clear(CD_PORT, CD_GPIO);
}

void ucg_com_cm3_4wire_HW_SPI_change_cs_line(ucg_t *ucg, uint8_t state) {
  (void) ucg;

  spi_wait_not_busy();
  if(state)
    gpio_set(CS_PORT, CS_GPIO);
  else
    gpio_clear(CS_PORT, CS_GPIO);
}

void ucg_com_cm3_4wire_HW_SPI_send_byte(ucg_t *ucg, uint8_t byte) {
  (void) ucg;

  spi_send(SPI_PERIPHERAL, byte);
}

void ucg_com_cm3_4wire_HW_SPI_repeat_1_byte(ucg_t *ucg, uint16_t repeat, uint8_t byte) {
  (void) ucg;

  while( repeat > 0 ) {
    spi_send(SPI_PERIPHERAL, byte);
    repeat--;
  }
}

void ucg_com_cm3_4wire_HW_SPI_repeat_2_bytes(ucg_t *ucg, uint16_t repeat, uint8_t bytes[2]) {
  (void) ucg;

  while( repeat > 0 ) {
    spi_send(SPI_PERIPHERAL, bytes[0]);
    spi_send(SPI_PERIPHERAL, bytes[1]);
    repeat--;
  }
}

void ucg_com_cm3_4wire_HW_SPI_repeat_3_bytes(ucg_t *ucg, uint16_t repeat, uint8_t bytes[3]) {
  (void) ucg;

  while( repeat > 0 ) {
    spi_send(SPI_PERIPHERAL, bytes[0]);
    spi_send(SPI_PERIPHERAL, bytes[1]);
    spi_send(SPI_PERIPHERAL, bytes[2]);
    repeat--;
  }
}

void ucg_com_cm3_4wire_HW_SPI_send_str(ucg_t * ucg, uint16_t length, uint8_t bytes[]) {
  (void)ucg;

  while( length > 0 ) {
    spi_send(SPI_PERIPHERAL, *bytes); 
    bytes++;
    length--;
  }
}

void ucg_com_cm3_4wire_HW_SPI_send_cd_data_sequence(ucg_t *ucg, uint16_t count, uint8_t bytes[]) {
  (void)ucg;

  while(count > 0) {
    if ( *bytes != 0 ) {
      if ( *bytes == 1 ) {
        /* set CD (=D/C=A0) line to low */
        spi_wait_not_busy();
        gpio_clear(CD_PORT, CD_GPIO);
      } else {
        /* set CD (=D/C=A0) line to high */
        spi_wait_not_busy();
        gpio_set(CD_PORT, CD_GPIO);
      }
    }
    bytes++;
    spi_send(SPI_PERIPHERAL, *bytes);
    bytes++;
    count--;
  }
}

int16_t ucg_com_cm3_4wire_HW_SPI(
    ucg_t *ucg,
    int16_t msg,
    uint16_t arg,
    uint8_t *data
) {
  uint32_t start_ms;

  switch(msg) {
    case UCG_COM_MSG_POWER_UP:
      /* "data" is a pointer to ucg_com_info_t structure with the following information: */
      /*  ((ucg_com_info_t *)data)->serial_clk_speed value in nanoseconds */
      /*  ((ucg_com_info_t *)data)->parallel_clk_speed value in nanoseconds */
      /* "arg" is not used */

      /* This message is sent once at the uC startup and for power up. */
      /* setup i/o or do any other setup */

      ucg_com_cm3_4wire_HW_SPI_power_up(ucg, (ucg_com_info_t *)data);
      break;
    case UCG_COM_MSG_POWER_DOWN:
      /* "data" and "arg" are not used*/
      /* This message is sent for a power down request */
      ucg_com_cm3_4wire_HW_SPI_power_down(ucg);
      break;
    case UCG_COM_MSG_DELAY:
      /* "data" is not used */
      /* "arg" contains the number of microseconds for the delay */
      /* By receiving this message, the following code should delay by */
      /* "arg" microseconds. One microsecond is 0.000001 second */
      ucg_com_cm3_4wire_HW_SPI_delay(ucg, arg);
      break;
    case UCG_COM_MSG_CHANGE_RESET_LINE:
      /* "data" is not used */
      /* "arg" = 1: set the reset output line to 1 */
      /* "arg" = 0: set the reset output line to 0 */
      ucg_com_cm3_4wire_HW_SPI_change_reset_line(ucg, arg);
      break;
    case UCG_COM_MSG_CHANGE_CD_LINE:
      /* "ucg->com_status"  bit 0 contains the old level for the CD line */
      /* "data" is not used */
      /* "arg" = 1: set the command/data (a0) output line to 1 */
      /* "arg" = 0: set the command/data (a0) output line to 0 */
      ucg_com_cm3_4wire_HW_SPI_change_cd_line(ucg, arg);
      break;
    case UCG_COM_MSG_CHANGE_CS_LINE:
      /* "ucg->com_status"  bit 1 contains the old level for the CS line */
      /* "data" is not used */
      /* "arg" = 1: set the chipselect output line to 1 */
      /* "arg" = 0: set the chipselect output line to 0 */
      ucg_com_cm3_4wire_HW_SPI_change_cs_line(ucg, arg);
      break;
    case UCG_COM_MSG_SEND_BYTE:
      /* "data" is not used */
      /* "arg" contains one byte, which should be sent to the display */
      /* The current status of the CD line is available */
      /* in bit 0 of u8g->com_status */
      ucg_com_cm3_4wire_HW_SPI_send_byte(ucg, arg);
      break;
    case UCG_COM_MSG_REPEAT_1_BYTE:
      /* "data[0]" contains one byte */
      /* repeat sending the byte in data[0] "arg" times */
      /* The current status of the CD line is available */
      /* in bit 0 of u8g->com_status */
      ucg_com_cm3_4wire_HW_SPI_repeat_1_byte(ucg, arg, data[0]);
      break;
    case UCG_COM_MSG_REPEAT_2_BYTES:
      /* "data[0]" contains first byte */
      /* "data[1]" contains second byte */
      /* repeat sending the two bytes "arg" times */
      /* The current status of the CD line is available */
      /* in bit 0 of u8g->com_status */
      ucg_com_cm3_4wire_HW_SPI_repeat_2_bytes(ucg, arg, data);
      break;
    case UCG_COM_MSG_REPEAT_3_BYTES:
      /* "data[0]" contains first byte */
      /* "data[1]" contains second byte */
      /* "data[2]" contains third byte */
      /* repeat sending the three bytes "arg" times */
      /* The current status of the CD line is available */
      /* in bit 0 of u8g->com_status */
      ucg_com_cm3_4wire_HW_SPI_repeat_3_bytes(ucg, arg, data);
      break;
    case UCG_COM_MSG_SEND_STR:
      /* "data" is an array with "arg" bytes */
      /* send "arg" bytes to the display */
      ucg_com_cm3_4wire_HW_SPI_send_str(ucg, arg, data);
      break;
    case UCG_COM_MSG_SEND_CD_DATA_SEQUENCE:
      /* "data" is a pointer to two bytes, which contain the cd line */
      /* status and display data */
      /* "arg" contains the number of these two byte tuples which need to */
      /* be analysed and sent. Bellow is a example sequence */
      /* The content of bit 0 in u8g->com_status is undefined for this message */
      ucg_com_cm3_4wire_HW_SPI_send_cd_data_sequence(ucg, arg, data);
      break;
  }
  return 1;
}