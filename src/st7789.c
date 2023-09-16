/*
 * Copyright (c) 2021 Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 * 
 */

#include <string.h>
#include "pico/stdlib.h"
#include "pico/st7789.h"

const uint8_t madctl_rotation[] = {0x00, 0x60};

static struct st7789_config st7789_cfg;
static uint16_t st7789_width;
static uint16_t st7789_height;
static bool st7789_data_mode = false;

static void st7789_cmd(uint8_t cmd, const uint8_t* data, size_t len)
{
    if (st7789_cfg.gpio_cs > -1) {
        spi_set_format(st7789_cfg.spi, 8, SPI_CPOL_0, SPI_CPHA_0, SPI_MSB_FIRST);
    } else {
        spi_set_format(st7789_cfg.spi, 8, SPI_CPOL_1, SPI_CPHA_1, SPI_MSB_FIRST);
    }
    st7789_data_mode = false;

    sleep_us(1);
    if (st7789_cfg.gpio_cs > -1) {
        gpio_put(st7789_cfg.gpio_cs, 0);
    }
    gpio_put(st7789_cfg.gpio_dc, 0);
    sleep_us(1);
    
    spi_write_blocking(st7789_cfg.spi, &cmd, sizeof(cmd));
    
    if (len) {
        sleep_us(1);
        gpio_put(st7789_cfg.gpio_dc, 1);
        sleep_us(1);
        
        spi_write_blocking(st7789_cfg.spi, data, len);
    }

    sleep_us(1);
    if (st7789_cfg.gpio_cs > -1) {
        gpio_put(st7789_cfg.gpio_cs, 1);
    }
    gpio_put(st7789_cfg.gpio_dc, 1);
    sleep_us(1);
}

void st7789_caset(uint16_t xs, uint16_t xe)
{
    uint8_t data[] = {
        xs >> 8,
        xs & 0xff,
        xe >> 8,
        xe & 0xff,
    };

    // CASET (2Ah): Column Address Set
    st7789_cmd(0x2a, data, sizeof(data));
}

void st7789_raset(uint16_t ys, uint16_t ye)
{
    uint8_t data[] = {
        ys >> 8,
        ys & 0xff,
        ye >> 8,
        ye & 0xff,
    };

    // RASET (2Bh): Row Address Set
    st7789_cmd(0x2b, data, sizeof(data));
}

void st7789_init(const struct st7789_config* config, uint16_t width, uint16_t height, enum st7789_rotation rotation)
{
    memcpy(&st7789_cfg, config, sizeof(st7789_cfg));
    st7789_width = width;
    st7789_height = height;

    spi_init(st7789_cfg.spi, 125 * 1000 * 1000);
    if (st7789_cfg.gpio_cs > -1) {
        spi_set_format(st7789_cfg.spi, 8, SPI_CPOL_0, SPI_CPHA_0, SPI_MSB_FIRST);
    } else {
        spi_set_format(st7789_cfg.spi, 8, SPI_CPOL_1, SPI_CPHA_1, SPI_MSB_FIRST);
    }

    gpio_set_function(st7789_cfg.gpio_din, GPIO_FUNC_SPI);
    gpio_set_function(st7789_cfg.gpio_clk, GPIO_FUNC_SPI);

    if (st7789_cfg.gpio_cs > -1) {
        gpio_init(st7789_cfg.gpio_cs);
    }
    gpio_init(st7789_cfg.gpio_dc);
    gpio_init(st7789_cfg.gpio_rst);
    gpio_init(st7789_cfg.gpio_bl);

    if (st7789_cfg.gpio_cs > -1) {
        gpio_set_dir(st7789_cfg.gpio_cs, GPIO_OUT);
    }
    gpio_set_dir(st7789_cfg.gpio_dc, GPIO_OUT);
    gpio_set_dir(st7789_cfg.gpio_rst, GPIO_OUT);
    gpio_set_dir(st7789_cfg.gpio_bl, GPIO_OUT);

    if (st7789_cfg.gpio_cs > -1) {
        gpio_put(st7789_cfg.gpio_cs, 1);
    }
    gpio_put(st7789_cfg.gpio_dc, 1);
    gpio_put(st7789_cfg.gpio_rst, 1);
    sleep_ms(100);
    
    // SWRESET (01h): Software Reset
    st7789_cmd(0x01, NULL, 0);
    sleep_ms(150);

    // SLPOUT (11h): Sleep Out
    st7789_cmd(0x11, NULL, 0);
    sleep_ms(50);

    // COLMOD (3Ah): Interface Pixel Format
    // - RGB interface color format     = 65K of RGB interface
    // - Control interface color format = 16bit/pixel
    st7789_cmd(0x3a, (uint8_t[]){ 0x55 }, 1);
    sleep_ms(10);

    // MADCTL (36h): Memory Data Access Control
    st7789_cmd(0x36, (uint8_t[]){ madctl_rotation[rotation] }, 1);
   
    st7789_caset(0, width - 1);
    st7789_raset(0, height - 1);

    // INVON (21h): Display Inversion On
    st7789_cmd(0x21, NULL, 0);
    sleep_ms(10);

    // NORON (13h): Normal Display Mode On
    st7789_cmd(0x13, NULL, 0);
    sleep_ms(10);

    // DISPON (29h): Display On
    st7789_cmd(0x29, NULL, 0);
    sleep_ms(10);

    gpio_put(st7789_cfg.gpio_bl, 1);
}

void st7789_ramwr()
{
    sleep_us(1);
    if (st7789_cfg.gpio_cs > -1) {
        gpio_put(st7789_cfg.gpio_cs, 0);
    }
    gpio_put(st7789_cfg.gpio_dc, 0);
    sleep_us(1);

    // RAMWR (2Ch): Memory Write
    uint8_t cmd = 0x2c;
    spi_write_blocking(st7789_cfg.spi, &cmd, sizeof(cmd));

    sleep_us(1);
    if (st7789_cfg.gpio_cs > -1) {
        gpio_put(st7789_cfg.gpio_cs, 0);
    }
    gpio_put(st7789_cfg.gpio_dc, 1);
    sleep_us(1);
}

void st7789_write(const void* data, size_t len)
{
    if (!st7789_data_mode) {
        st7789_ramwr();

        if (st7789_cfg.gpio_cs > -1) {
            spi_set_format(st7789_cfg.spi, 16, SPI_CPOL_0, SPI_CPHA_0, SPI_MSB_FIRST);
        } else {
            spi_set_format(st7789_cfg.spi, 16, SPI_CPOL_1, SPI_CPHA_1, SPI_MSB_FIRST);
        }

        st7789_data_mode = true;
    }

    spi_write16_blocking(st7789_cfg.spi, data, len / 2);
}

void st7789_set_window(uint16_t xs, uint16_t xe, uint16_t ys, uint16_t ye)
{
    st7789_caset(xs, xe);
    st7789_raset(ys, ye);
}

void st7789_vertical_scroll(uint16_t row)
{
    uint8_t data[] = {
        (row >> 8) & 0xff,
        row & 0x00ff
    };

    // VSCSAD (37h): Vertical Scroll Start Address of RAM 
    st7789_cmd(0x37, data, sizeof(data));
}
