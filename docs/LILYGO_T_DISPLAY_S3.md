# LILYGO T-Display-S3

## Board

- Board: LILYGO T-Display-S3
- MCU: ESP32-S3
- Flash: 16 MB module, OpenDTU partition table remains `partitions_custom_4mb.csv`
- PlatformIO environment: `lilygo_t_display_s3`
- PlatformIO board ID: `lilygo-t-display-s3`

## Display

- Controller: ST7789
- Resolution: 170 x 320 px
- Bus: 8-bit parallel
- TFT_eSPI setup source: LilyGO T-Display-S3 Setup 206 definitions copied into `include/TDisplayS3_TFT_Setup.h`
- Display power: GPIO 15
- Backlight: GPIO 38

The generic OpenDTU display configuration stays disabled:

```json
"display": {
  "type": 0,
  "data": -1,
  "clk": -1,
  "cs": -1,
  "reset": -1
}
```

The internal LilyGO display is driven separately by `Display_TDisplayS3`.

## CMT2300A

The working CMT mapping is unchanged:

```text
CMT SDIO: GPIO 11
CMT CLK:  GPIO 12
CMT CS:   GPIO 10
CMT FCS:  GPIO 13
```

```json
"cmt": {
  "sdio": 11,
  "clk": 12,
  "cs": 10,
  "fcs": 13,
  "gpio2": -1,
  "gpio3": -1
}
```

## Commands

```bash
pio run -e lilygo_t_display_s3
pio run -e lilygo_t_display_s3 -t upload
pio device monitor -b 115200
```

## Known Limitations

- No touch support.
- No button handling.
- No LVGL.
- No web interface changes.
- The display update is limited to once per second and uses no periodic `delay()`.
