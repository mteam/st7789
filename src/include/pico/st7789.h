/*
 * Copyright (c) 2021 Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 * 
 */

#ifndef _PICO_ST7789_H_
#define _PICO_ST7789_H_

#include "hardware/spi.h"

struct st7789_config {
    spi_inst_t* spi;
    uint gpio_din;
    uint gpio_clk;
    int gpio_cs;
    uint gpio_dc;
    uint gpio_rst;
    uint gpio_bl;
};

void st7789_init(const struct st7789_config* config, uint16_t width, uint16_t height);
void st7789_write(const void* data, size_t len);
void st7789_set_window(uint16_t xs, uint16_t xe, uint16_t ys, uint16_t ye);
void st7789_vertical_scroll(uint16_t row);

#endif
