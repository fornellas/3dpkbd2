#include <libopencm3/usb/usbd.h>

extern uint8_t status;
extern uint8_t state;
extern uint32_t current_address;
extern uint32_t current_bytes;
extern uint16_t current_block_num;

void dfu_set_config_callback(usbd_device *dev);