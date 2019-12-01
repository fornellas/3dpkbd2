#include "display.h"
#include "lib/systick.h"
#include <ucg.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/spi.h>
#include <libopencm3/stm32/common/spi_common_all.h>

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

static ucg_t ucg;

static inline void spi_wait_not_busy(void);

static inline void spi_wait_not_busy() {
  while (SPI_SR(SPI_PERIPHERAL) & SPI_SR_BSY);
}

static int16_t ucg_com_cm3_4wire_HW_SPI(ucg_t *, int16_t, uint16_t, uint8_t *);

static int16_t ucg_com_cm3_4wire_HW_SPI(
    ucg_t *_ucg,
    int16_t msg,
    uint16_t arg,
    uint8_t *data
) {
  uint32_t start_ms;
  (void)_ucg;

  switch(msg) {
    case UCG_COM_MSG_POWER_UP:
      /* "data" is a pointer to ucg_com_info_t structure with the following information: */
      /*  ((ucg_com_info_t *)data)->serial_clk_speed value in nanoseconds */
      /*  ((ucg_com_info_t *)data)->parallel_clk_speed value in nanoseconds */
      /* "arg" is not used */

      /* This message is sent once at the uC startup and for power up. */
      /* setup i/o or do any other setup */

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
      spi_init_master(
        SPI_PERIPHERAL,
        SPI_CR1_BAUDRATE_FPCLK_DIV_8,
        SPI_CR1_CPOL_CLK_TO_0_WHEN_IDLE,
        SPI_CR1_CPHA_CLK_TRANSITION_1,
        SPI_CR1_DFF_8BIT,
        SPI_CR1_MSBFIRST
      );
      spi_enable(SPI_PERIPHERAL);

      break;
    case UCG_COM_MSG_POWER_DOWN:
      /* "data" and "arg" are not used*/
      /* This message is sent for a power down request */
      spi_disable(SPI_PERIPHERAL);
      break;
    case UCG_COM_MSG_DELAY:
      /* "data" is not used */
      /* "arg" contains the number of microseconds for the delay */
      /* By receiving this message, the following code should delay by */
      /* "arg" microseconds. One microsecond is 0.000001 second */
      // TODO
      break;
    case UCG_COM_MSG_CHANGE_RESET_LINE:
      /* "data" is not used */
      /* "arg" = 1: set the reset output line to 1 */
      /* "arg" = 0: set the reset output line to 0 */
      if(arg)
        gpio_set(RST_PORT, RST_GPIO);
      else
        gpio_clear(RST_PORT, RST_GPIO);
      break;
    case UCG_COM_MSG_CHANGE_CD_LINE:
      /* "ucg->com_status"  bit 0 contains the old level for the CD line */
      /* "data" is not used */
      /* "arg" = 1: set the command/data (a0) output line to 1 */
      /* "arg" = 0: set the command/data (a0) output line to 0 */
      spi_wait_not_busy();
      if(arg)
        gpio_set(CD_PORT, CD_GPIO);
      else
        gpio_clear(CD_PORT, CD_GPIO);
      break;
    case UCG_COM_MSG_CHANGE_CS_LINE:
      /* "ucg->com_status"  bit 1 contains the old level for the CS line */
      /* "data" is not used */
      /* "arg" = 1: set the chipselect output line to 1 */
      /* "arg" = 0: set the chipselect output line to 0 */
      spi_wait_not_busy();
      if(arg)
        gpio_set(CS_PORT, CS_GPIO);
      else
        gpio_clear(CS_PORT, CS_GPIO);
      break;
    case UCG_COM_MSG_SEND_BYTE:
      /* "data" is not used */
      /* "arg" contains one byte, which should be sent to the display */
      /* The current status of the CD line is available */
      /* in bit 0 of u8g->com_status */
      spi_write(SPI_PERIPHERAL, arg);
      break;
    case UCG_COM_MSG_REPEAT_1_BYTE:
      /* "data[0]" contains one byte */
      /* repeat sending the byte in data[0] "arg" times */
      /* The current status of the CD line is available */
      /* in bit 0 of u8g->com_status */
      while( arg > 0 ) {
        spi_send(SPI_PERIPHERAL, data[0]);
        arg--;
      }
      break;
    case UCG_COM_MSG_REPEAT_2_BYTES:
      /* "data[0]" contains first byte */
      /* "data[1]" contains second byte */
      /* repeat sending the two bytes "arg" times */
      /* The current status of the CD line is available */
      /* in bit 0 of u8g->com_status */
      while( arg > 0 ) {
        spi_send(SPI_PERIPHERAL, data[0]);
        spi_send(SPI_PERIPHERAL, data[1]);
        arg--;
      }
      break;
    case UCG_COM_MSG_REPEAT_3_BYTES:
      /* "data[0]" contains first byte */
      /* "data[1]" contains second byte */
      /* "data[2]" contains third byte */
      /* repeat sending the three bytes "arg" times */
      /* The current status of the CD line is available */
      /* in bit 0 of u8g->com_status */
      while( arg > 0 ) {
        spi_send(SPI_PERIPHERAL, data[0]);
        spi_send(SPI_PERIPHERAL, data[1]);
        spi_send(SPI_PERIPHERAL, data[2]);
        arg--;
      }
      break;
    case UCG_COM_MSG_SEND_STR:
      /* "data" is an array with "arg" bytes */
      /* send "arg" bytes to the display */
      while( arg > 0 ) {
        spi_send(SPI_PERIPHERAL, *data);
        data++;
        arg--;
      }
      break;
    case UCG_COM_MSG_SEND_CD_DATA_SEQUENCE:
      /* "data" is a pointer to two bytes, which contain the cd line */
      /* status and display data */
      /* "arg" contains the number of these two byte tuples which need to */
      /* be analysed and sent. Bellow is a example sequence */
      /* The content of bit 0 in u8g->com_status is undefined for this message */
      while(arg > 0) {
        if ( *data != 0 ) {
          if ( *data == 1 ) {
            /* set CD (=D/C=A0) line to low */
            spi_wait_not_busy();
            gpio_clear(CD_PORT, CD_GPIO);
          } else {
            /* set CD (=D/C=A0) line to high */
            spi_wait_not_busy();
            gpio_set(CD_PORT, CD_GPIO);
          }
        }
        data++;
        /* send *data to the display */
        spi_send(SPI_PERIPHERAL, *data);
        data++;
        arg--;
      }
      break;
  }
  return 1;
}

ucg_t *display_setup_base(void) {
  // Sleep a bit to allow the voltage regulator to stabilize
  delay_ms(50);

  ucg_InitBuffer(
    &ucg,
    ucg_dev_ssd1351_18x128x128_ilsoft,
    ucg_ext_ssd1351_18,
    ucg_com_cm3_4wire_HW_SPI
  );
  ucg_SendBuffer(&ucg);

  return &ucg;
}