# OpenDTU **LilyGo T-Display-S3 Dashboard** v1.0

Initial public release of the OpenDTU LilyGo T-Display-S3 dashboard fork.

This release is based on OpenDTU and adds a modern local display dashboard for the LilyGo T-Display-S3 / ESP32-S3 board, plus an optional modern web dashboard at `/modern`.

## Features

- Modern seven-page dashboard for the 320 x 170 LilyGo T-Display-S3
- Dark dashboard UI with rounded tiles, gauges, charts, and clean typography
- Current power overview with 180 degree gauge
- Per-panel power chart for up to four DC panel/string channels
- Daily power history from 06:00 to 18:00 with average line
- Seven-day rolling daily yield buffer stored on the ESP filesystem
- Energy statistics screen for savings, total yield, and estimated CO2 savings
- Inverter, OpenDTU, and network status screens
- Animated OpenDTU boot screen
- Waiting screen until inverter data is available, with button skip and timeout
- Smooth horizontal slide animation between display pages
- 10 selectable color themes
- Last selected display page and theme are restored after reboot
- Modern web dashboard available at `http://<opendtu-ip>/modern`
- Link to the modern dashboard from the original OpenDTU navigation bar

## Inverter Compatibility

This fork keeps the original OpenDTU inverter support. In general, every inverter that works with the official OpenDTU firmware should also work with this release, because inverter communication, configuration, Web API, and core OpenDTU logic are inherited from upstream OpenDTU.

The added LilyGo display dashboard is visually optimized for one inverter and up to four DC panel/string channels. Inverters with fewer channels still work; unused panel positions are displayed as empty or zero-value channels on the dashboard.

For the full compatibility list, see the official OpenDTU inverter overview:

https://www.opendtu.solar/hardware/inverter_overview/

## License

This release follows the original OpenDTU license:

- SPDX-License-Identifier: `GPL-2.0-or-later`
- Full license text: [LICENSE](https://github.com/moschotto/OpenDTU/blob/feature/lilygo-t-display-s3/LICENSE)
- Original license notice: [COPYING](https://github.com/moschotto/OpenDTU/blob/feature/lilygo-t-display-s3/COPYING)

## Screenshots

Current dashboard layout including boot, waiting, and dashboard screens.

![LilyGo T-Display-S3 page slide demo](https://raw.githubusercontent.com/moschotto/OpenDTU/feature/lilygo-t-display-s3/docs/lilygo-t-display-s3/screenshots/dashboard-slide-demo.gif)


![LilyGo T-Display-S3 dashboard screens](https://raw.githubusercontent.com/moschotto/OpenDTU/feature/lilygo-t-display-s3/docs/lilygo-t-display-s3/screenshots/dashboard-screens-overview.png)

## Wiring

The LilyGo T-Display-S3 profile uses direct CMT2300A pads:

![LilyGo T-Display-S3 CMT2300A wiring](https://raw.githubusercontent.com/moschotto/OpenDTU/feature/lilygo-t-display-s3/docs/lilygo-t-display-s3/wiring/lilygo-t-display-s3-cmt2300a-wiring.png)

Connections:

- `3V3` to `3.3V`
- `GND` to `GND`
- `GPIO10` to `CS`
- `GPIO11` to `SDIO`
- `GPIO12` to `CLK`
- `GPIO13` to `FCS`

Remove the jumper for SPI operation. Use 3.3 V only.

## Release Assets

The release includes firmware binaries for the `lilygo_t_display_s3` PlatformIO environment:

- `firmware.factory.bin`
- `firmware.bin`

## Which Binary Should I Flash?

Use the same flashing approach as the original OpenDTU project. The LilyGo T-Display-S3 fork does not require a special flashing process.

### Fresh Install / First Flash

Use `firmware.factory.bin` for a clean first installation or when flashing the ESP32-S3 from scratch.

`firmware.factory.bin` is the combined factory image. It contains:

- bootloader
- partition table
- boot app
- OpenDTU application firmware

This is the safest choice when the board is empty, freshly erased, has an unknown firmware state, or when you use the ESP Web Flasher for an initial installation.

### Update an Existing OpenDTU Installation

Use `firmware.bin` when OpenDTU is already installed and you update through an existing OpenDTU-compatible setup, for example:

- OpenDTU web update
- OTA update
- PlatformIO upload to an already correctly partitioned device

`firmware.bin` contains only the application firmware and assumes that bootloader and partition table are already present and compatible.

### ESP Web Flasher

For ESP Web Flasher usage, use a Chromium-based browser and open the ESP Web Tools flasher:

https://web.esphome.io/

Follow the original OpenDTU flashing documentation for the general flashing process. The flashing procedure is the same for this LilyGo T-Display-S3 release:

https://www.opendtu.solar/firmware/firmware/

The recommended file for a first flash via ESP Web Flasher is:

```text
firmware.factory.bin
```

If the device already runs OpenDTU and the web flasher/update tool explicitly expects an application image, use:

```text
firmware.bin
```

## Local Build and Upload

Build locally:

```bash
~/.platformio/penv/bin/pio run -e lilygo_t_display_s3
```

Upload locally:

```bash
~/.platformio/penv/bin/pio run -e lilygo_t_display_s3 -t upload --upload-port /dev/cu.usbmodem21201
```

Use the actual serial port of your board if it differs.

## Notes

- Back up your OpenDTU configuration before replacing an existing installation.
- The display UI is currently tailored for one inverter.
- The display panel screen is fixed to four PV channels.
- The daily yield history starts empty after first flash or filesystem reset.
- Savings and CO2 values are estimates derived from total generated energy.
- The power history graph is kept in RAM and starts fresh after reboot.
- Missing inverter values are displayed as `--`.
