#include <libopencm3/usb/dfu.h>
#include <libopencm3/usb/usbd.h>

extern uint8_t dfu_status;
extern uint8_t dfu_state;
extern uint32_t dfu_address;
extern uint32_t dfu_bytes;
extern uint16_t dfu_block_num;

void dfu_set_config_callback(usbd_device *dev);