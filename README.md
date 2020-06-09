# 3dpkbd2

[![Build Status](https://travis-ci.com/fornellas/3dpkbd2.svg?branch=master)](https://travis-ci.com/fornellas/3dpkbd2)
[![GitHub license](https://img.shields.io/badge/license-MIT-blue.svg)](LICENSE)

A 3D printed ergonomic keyboard project, fully designed and built from the ground up.

![](https://github.com/fornellas/3dpkbd2/raw/master/doc/images/render.png)

This is my third iteration on custom keyboards, here's my [previous project](https://fornellas.github.io/3d_printed_keyboard/).

## Features

## Design

The CAD design is fully open source and available at [OnShape](https://cad.onshape.com/documents/aac63e7135d7da5735b99a17/w/f43890cccdca58cca591f396/e/c1a5989178408103fe3ff512) for free.

- Highly optimized for being printable on desktop FDM / FFF 3D printers.
- Two levels thumb cluster:
  - First row (Ctrl & Alt) is lower than second row (Space / Shift).
  - This allows pressing the second row naturally.
- Two parts split.
  - Allows adjustment of angle to match wrists.
- 46 unique key cap designs.
  - Horizontal and vertical concave surface for both right and left sides.
  - Strategic bump marking at some keys for tactile feedback.
- Highly optimized keycaps printable locking mechanism makes it snap perfectly to Cherry MX switches.
- Solder free hot swappable Cherry MX switches.

## Firmware

- Uses [libopncm3](http://libopencm3.org/).
- Custom DFU bootloader:
  - Enabled by pressing the reset button.
  - Includes status display on OLED screen.
  - Implements standard USB DFU specification from official documents.
- Main firmware:
  - Passes all logical tests from official [USB20CV compliance test tool](https://www.usb.org/usb2tools) for both USB and HID.
  - Supports several USB HID usage pages (Keyboard/Keypad, Generic Desktop & Consumer), enabling several shortcuts.
  - Layered layout support, allowing easy hardware mapping of alternate keyboard layouts (eg: Dvorak).
  - Each key can be mapped to:
    - USB usage page code.
    - Go to next layer.
    - Run C function.
    - Send sequence of key presses.
  - 128x128 color OLED screen
    - Full USB status (reset, configured, suspended, remote wake-up).
    - Full HID status.
      - Keyboard "LEDs" (caps lock, scroll lock etc).
      - HID report requency.
    - Communication status between left / right side.
    - Active layer (eg: qwerty, dvorak).
  - Support for either Boot or report HID protocols: works on all BIOS.
  - Remote wake up support.

## Bill of Materials

- All 3D parts STL files can be obtained from [OnShape](https://cad.onshape.com/documents/aac63e7135d7da5735b99a17/w/f43890cccdca58cca591f396/e/c1a5989178408103fe3ff512) and printed on a desktop FDM / FFF.
- SSD1351 color OLED 128x128 screen.
- STM32F411CEU6 "Black pill" board.
- USB-C Female breakout boards.
- USB-C Male Breakout board.
- Diodes.
- Kailh PCB Sockets.
- Cherry MX Silent Red.
- Screws.
- Keys stabilizers.
- Silicone pads.
- Tactile button.
