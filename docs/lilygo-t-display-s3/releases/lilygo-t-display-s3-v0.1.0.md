# LilyGo T-Display-S3 Dashboard v0.1.0

Initial public release of the OpenDTU LilyGo T-Display-S3 dashboard fork.

This release is based on OpenDTU and adds a modern local display dashboard for the LilyGo T-Display-S3 / ESP32-S3 board, plus an optional modern web dashboard at `/modern`.

## Highlights

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

## Screenshots

![LilyGo T-Display-S3 dashboard screens](https://raw.githubusercontent.com/moschotto/OpenDTU/feature/lilygo-t-display-s3/docs/lilygo-t-display-s3/screenshots/dashboard-screens-overview.png)

## Release Assets

The release includes firmware binaries for the `lilygo_t_display_s3` PlatformIO environment:

- `firmware.bin` - application firmware image
- `firmware.factory.bin` - combined factory image including bootloader, partition table, boot app, and firmware

## Flashing

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

- The display UI is currently tailored for one inverter.
- The display panel screen is fixed to four PV channels.
- The daily yield history starts empty after first flash or filesystem reset.
- Savings and CO2 values are estimates derived from total generated energy.
- The power history graph is kept in RAM and starts fresh after reboot.
- Missing inverter values are displayed as `--`.
