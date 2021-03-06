#include "lib/usb.h"
#include "descriptors.h"

#define MAX(h,i) ((h) > (i) ? (h) : (i))

// Must be as big as the biggest descriptor
extern uint8_t usbd_control_buffer[
	MAX(
		MAX(
			MAX(
				MAX(
					MAX(
						MAX(
							MAX(
								MAX(
									MAX(
										sizeof(struct usb_device_descriptor),
										sizeof(struct usb_config_descriptor)
									),
									sizeof(struct usb_interface_descriptor)
								),
								sizeof(struct usb_endpoint_descriptor)
							),
 							sizeof(struct usb_hid_function)
 						),
 						sizeof(hid_report_descriptor_boot)
 					),
 					sizeof(struct hid_in_report_boot_t)
 				),
 				sizeof(hid_out_report_boot_t)
 			),
 			sizeof(hid_report_descriptor_extra)
 		),
 		sizeof(struct hid_in_report_extra_t)
 	)
];

usbd_device *usbd_setup(void);