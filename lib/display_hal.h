#ifndef DISPLAY_HAL_H
#define DISPLAY_HAL_H

#include <stdint.h>
#include <ucg.h>

void ucg_com_cm3_4wire_HW_SPI_power_up(ucg_t *ucg, ucg_com_info_t *ucg_com_info);

void ucg_com_cm3_4wire_HW_SPI_power_down(ucg_t *ucg);

void ucg_com_cm3_4wire_HW_SPI_delay(ucg_t *ucg, uint16_t microseconds);

void ucg_com_cm3_4wire_HW_SPI_change_reset_line(ucg_t *ucg, uint8_t state);

void ucg_com_cm3_4wire_HW_SPI_change_cd_line(ucg_t *ucg, uint8_t state);

void ucg_com_cm3_4wire_HW_SPI_change_cs_line(ucg_t *ucg, uint8_t state);

void ucg_com_cm3_4wire_HW_SPI_send_byte(ucg_t *ucg, uint8_t byte);

void ucg_com_cm3_4wire_HW_SPI_repeat_1_byte(ucg_t *ucg, uint16_t repeat, uint8_t byte);

void ucg_com_cm3_4wire_HW_SPI_repeat_2_bytes(ucg_t *ucg, uint16_t repeat, uint8_t bytes[2]);

void ucg_com_cm3_4wire_HW_SPI_repeat_3_bytes(ucg_t *ucg, uint16_t repeat, uint8_t bytes[3]);

void ucg_com_cm3_4wire_HW_SPI_send_str(ucg_t * ucg, uint16_t length, uint8_t bytes[]);

void ucg_com_cm3_4wire_HW_SPI_send_cd_data_sequence(ucg_t *ucg, uint16_t count, uint8_t bytes[]);

int16_t ucg_com_cm3_4wire_HW_SPI(ucg_t *, int16_t, uint16_t, uint8_t *);

#endif