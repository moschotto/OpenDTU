// SPDX-License-Identifier: GPL-2.0-or-later
#include "Display_TDisplayS3.h"

#if defined(OPENDTU_LILYGO_T_DISPLAY_S3)

#include <TFT_eSPI.h>
#include <WiFi.h>

namespace {
constexpr uint8_t DisplayPowerPin = 15;

TFT_eSPI _tft;

void drawStaticLayout()
{
    _tft.fillScreen(TFT_BLACK);
    _tft.setTextColor(TFT_WHITE, TFT_BLACK);
    _tft.setTextDatum(TL_DATUM);
    _tft.drawString("OpenDTU", 8, 8, 4);
    _tft.setTextDatum(MC_DATUM);
    _tft.drawString("LILYGO T-Display-S3", 160, 84, 4);
    _tft.setTextDatum(TL_DATUM);
    _tft.drawString("CMT: configured", 8, 136, 2);
}

void formatUptime(char* buffer, const size_t size)
{
    const uint32_t totalSeconds = millis() / 1000U;
    const uint32_t days = totalSeconds / 86400U;
    const uint32_t hours = (totalSeconds % 86400U) / 3600U;
    const uint32_t minutes = (totalSeconds % 3600U) / 60U;
    const uint32_t seconds = totalSeconds % 60U;

    snprintf(buffer, size, "Uptime: %02" PRIu32 "d %02" PRIu32 ":%02" PRIu32 ":%02" PRIu32,
        days, hours, minutes, seconds);
}
}

DisplayTDisplayS3 TDisplay;

void DisplayTDisplayS3::init()
{
    if (_initialized) {
        return;
    }

    pinMode(DisplayPowerPin, OUTPUT);
    digitalWrite(DisplayPowerPin, HIGH);

    _tft.init();
    _tft.setRotation(1);
    _tft.setTextPadding(0);
    drawStaticLayout();
    _tft.setTextDatum(TL_DATUM);
    _tft.drawString("Display OK", 8, 112, 2);

    _initialized = true;
}

void DisplayTDisplayS3::loop()
{
    if (!_initialized) {
        return;
    }

    const uint32_t now = millis();
    if (now - _lastUpdateMillis < 1000U) {
        return;
    }
    _lastUpdateMillis = now;

    _tft.setTextDatum(TR_DATUM);
    _tft.setTextColor(TFT_WHITE, TFT_BLACK);
    _tft.fillRect(185, 8, 127, 22, TFT_BLACK);
    if (WiFi.isConnected()) {
        char wifiText[24];
        snprintf(wifiText, sizeof(wifiText), "WiFi: %d dBm", WiFi.RSSI());
        _tft.drawString(wifiText, 312, 8, 2);
    } else {
        _tft.drawString("WiFi: offline", 312, 8, 2);
    }

    char uptimeText[32];
    formatUptime(uptimeText, sizeof(uptimeText));
    _tft.setTextDatum(TL_DATUM);
    _tft.fillRect(8, 112, 220, 18, TFT_BLACK);
    _tft.drawString(uptimeText, 8, 112, 2);
}

bool DisplayTDisplayS3::isInitialized() const
{
    return _initialized;
}

#endif
