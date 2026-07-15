// SPDX-License-Identifier: GPL-2.0-or-later
#include "Display_TDisplayS3.h"

#if defined(OPENDTU_LILYGO_T_DISPLAY_S3)

#include "Configuration.h"
#include "Datastore.h"

#include <Hoymiles.h>
#include <LittleFS.h>
#include <MycilaNTP.h>
#include <TFT_eSPI.h>
#include <WiFi.h>
#include <algorithm>
#include <cmath>
#include <cstring>
#include <ctime>
#include <memory>

namespace {
constexpr uint8_t DisplayPowerPin = 15;
constexpr uint8_t ButtonPagePin = BUTTON_1;
constexpr uint8_t ButtonThemePin = BUTTON_2;
constexpr uint8_t PageCount = 7;
constexpr uint8_t ThemeCount = 10;
constexpr uint32_t UpdateIntervalMillis = 1000U;
constexpr uint32_t ButtonDebounceMillis = 35U;
constexpr uint8_t PowerHistoryBuckets = 49;
constexpr uint16_t PowerHistoryStartMinute = 6U * 60U;
constexpr uint16_t PowerHistoryEndMinute = 18U * 60U;
constexpr uint8_t YieldHistoryDays = 7;
constexpr uint32_t YieldHistoryMagic = 0x54445948;
constexpr uint16_t YieldHistoryVersion = 1;
constexpr uint32_t YieldPersistIntervalMillis = 5U * 60U * 1000U;
constexpr char YieldHistoryFilename[] = "/tdisplay_yield_history.bin";
constexpr uint32_t DisplaySettingsMagic = 0x54445345;
constexpr uint16_t DisplaySettingsVersion = 1;
constexpr char DisplaySettingsFilename[] = "/tdisplay_settings.bin";
constexpr int16_t StartupLogoTargetW = 300;
constexpr int16_t StartupLogoTargetH = 58;
constexpr int16_t StartupLogoScale = 2;
constexpr int16_t StartupLogoSourceW = StartupLogoTargetW * StartupLogoScale;
constexpr int16_t StartupLogoSourceH = StartupLogoTargetH * StartupLogoScale;
constexpr uint32_t InverterWaitTimeoutMillis = 60U * 1000U;
constexpr uint32_t InverterWaitFrameMillis = 85U;
constexpr int16_t WaitPanelX = 22;
constexpr int16_t WaitPanelY = 24;
constexpr int16_t WaitPanelW = 276;
constexpr int16_t WaitPanelH = 122;
constexpr int16_t WaitBarX = 58;
constexpr int16_t WaitBarY = 118;
constexpr int16_t WaitBarW = 204;
constexpr int16_t WaitBarH = 12;
constexpr int16_t WaitBarSegmentW = 54;
constexpr int16_t WaitDotsX = 132;
constexpr int16_t WaitDotsY = 146;
constexpr int16_t WaitDotsW = 58;
constexpr int16_t WaitDotsH = 16;

struct Theme {
    uint16_t background;
    uint16_t panel;
    uint16_t panel2;
    uint16_t border;
    uint16_t muted;
    uint16_t text;
    uint16_t primary;
    uint16_t secondary;
    uint16_t success;
    uint16_t warning;
    uint16_t purple;
    uint16_t red;
    uint16_t grid;
    uint16_t inactive;
    uint16_t neutralBar;
    uint16_t glow;
};

const Theme Themes[ThemeCount] = {
    { TFT_BLACK, 0x10C4, 0x1946, 0x29AA, 0x3C1E, 0xF7BF, 0x3C1E, 0xED81, 0x262B, 0xFE62, 0x0BB2, 0xC3C7, 0x2188, 0x0884, 0x63B1, 0x3C1E },
    { TFT_BLACK, 0x0862, 0x10A3, 0x21C7, 0x1407, 0xEFFE, 0x1407, 0x231D, 0x262B, 0xED81, 0x0BB2, 0xEAC1, 0x1946, 0x0842, 0x6B91, 0x1407 },
    { TFT_BLACK, 0x1042, 0x1883, 0x30E4, 0xEAC1, 0xFF9D, 0xEAC1, 0x3C1E, 0x262B, 0xED81, 0x0BB2, 0xC3C7, 0x2966, 0x1041, 0x7B8D, 0xEAC1 },
    { TFT_BLACK, 0x18E3, 0x2124, 0x39E7, 0x633E, 0xF7BF, 0x633E, 0x3C1E, 0x262B, 0xED81, 0x0BB2, 0xEAC1, 0x3186, 0x1082, 0x738F, 0x633E },
    { TFT_BLACK, 0x0084, 0x08E7, 0x114A, 0x0BB2, 0xEFFF, 0x0BB2, 0xED81, 0x262B, 0xF4E1, 0x3C1E, 0xEAC1, 0x1107, 0x0042, 0x63B1, 0x0BB2 },
    { TFT_BLACK, 0x0861, 0x10A2, 0x2925, 0xED81, 0xF7BF, 0xED81, 0x3C1E, 0x262B, 0xFE62, 0x0BB2, 0xEAC1, 0x2104, 0x0841, 0x6B4D, 0xED81 },
    { TFT_BLACK, 0x2042, 0x3083, 0x48A5, 0xC3C7, 0xF73A, 0xC3C7, 0x3C1E, 0x262B, 0xED81, 0x0BB2, 0xEAC1, 0x38A4, 0x1021, 0x7B8D, 0xC3C7 },
    { TFT_BLACK, 0x08A4, 0x1106, 0x21AA, 0x10F4, 0xF7BF, 0x10F4, 0x0BB2, 0x262B, 0xED81, 0x3C1E, 0xEAC1, 0x1188, 0x0083, 0x63B1, 0x10F4 },
    { TFT_BLACK, 0x1046, 0x1888, 0x314D, 0x269D, 0xF7DF, 0x269D, 0xFDA0, 0x15D0, 0xFE62, 0x0BB2, 0xEAC1, 0x294A, 0x0844, 0x73B0, 0x269D },
    { TFT_BLACK, 0x10C4, 0x18E6, 0x31CB, 0xFDA0, 0xFFDE, 0xFDA0, 0x3C1E, 0x262B, 0xED81, 0x0BB2, 0xEAC1, 0x298A, 0x0843, 0x73B3, 0xFDA0 },
};

uint16_t ColorBackground = TFT_BLACK;
uint16_t ColorPanel = 0x0021;
uint16_t ColorPanel2 = 0x0841;
uint16_t ColorPanelBorder = 0x18E3;
uint16_t ColorMuted = 0x8C71;
uint16_t ColorText = TFT_WHITE;
uint16_t ColorCyan = 0x06DF;
uint16_t ColorBlue = 0x047F;
uint16_t ColorGreen = 0x07E8;
uint16_t ColorAmber = 0xFDA0;
uint16_t ColorRed = 0xF986;
uint16_t ColorPurple = 0xA35F;
uint16_t ColorGrid = 0x0841;
uint16_t ColorInactive = 0x1082;
uint16_t ColorNeutralBar = 0x7BEF;
uint16_t ColorGlow = 0x36FF;

TFT_eSPI _tft;
TFT_eSprite* _renderSprite = nullptr;
TFT_eSprite _waitBarSprite(&_tft);
TFT_eSprite _waitDotsSprite(&_tft);
bool _suppressPageDots = false;
bool _waitBarSpriteReady = false;
bool _waitDotsSpriteReady = false;

struct RenderTarget {
    void useSprite(TFT_eSprite* sprite) { _renderSprite = sprite; }
    void useDisplay() { _renderSprite = nullptr; }

    void init() { _tft.init(); }
    void setRotation(const uint8_t rotation) { _tft.setRotation(rotation); }
    void setTextPadding(const uint16_t padding)
    {
        if (_renderSprite != nullptr) {
            _renderSprite->setTextPadding(padding);
        } else {
            _tft.setTextPadding(padding);
        }
    }
    void setTextDatum(const uint8_t datum)
    {
        if (_renderSprite != nullptr) {
            _renderSprite->setTextDatum(datum);
        } else {
            _tft.setTextDatum(datum);
        }
    }
    void setTextColor(const uint16_t foreground, const uint16_t background)
    {
        if (_renderSprite != nullptr) {
            _renderSprite->setTextColor(foreground, background);
        } else {
            _tft.setTextColor(foreground, background);
        }
    }
    void setTextSize(const uint8_t size)
    {
        if (_renderSprite != nullptr) {
            _renderSprite->setTextSize(size);
        } else {
            _tft.setTextSize(size);
        }
    }
    void fillScreen(const uint32_t color)
    {
        if (_renderSprite != nullptr) {
            _renderSprite->fillSprite(color);
        } else {
            _tft.fillScreen(color);
        }
    }
    void fillRect(const int32_t x, const int32_t y, const int32_t w, const int32_t h, const uint32_t color)
    {
        if (_renderSprite != nullptr) {
            _renderSprite->fillRect(x, y, w, h, color);
        } else {
            _tft.fillRect(x, y, w, h, color);
        }
    }
    void drawRect(const int32_t x, const int32_t y, const int32_t w, const int32_t h, const uint32_t color)
    {
        if (_renderSprite != nullptr) {
            _renderSprite->drawRect(x, y, w, h, color);
        } else {
            _tft.drawRect(x, y, w, h, color);
        }
    }
    void fillRoundRect(const int32_t x, const int32_t y, const int32_t w, const int32_t h, const int32_t radius, const uint32_t color)
    {
        if (_renderSprite != nullptr) {
            _renderSprite->fillRoundRect(x, y, w, h, radius, color);
        } else {
            _tft.fillRoundRect(x, y, w, h, radius, color);
        }
    }
    void drawRoundRect(const int32_t x, const int32_t y, const int32_t w, const int32_t h, const int32_t radius, const uint32_t color)
    {
        if (_renderSprite != nullptr) {
            _renderSprite->drawRoundRect(x, y, w, h, radius, color);
        } else {
            _tft.drawRoundRect(x, y, w, h, radius, color);
        }
    }
    void drawArc(const int32_t x, const int32_t y, const int32_t r, const int32_t ir, const uint32_t startAngle, const uint32_t endAngle, const uint32_t fg, const uint32_t bg, const bool smooth = true)
    {
        if (_renderSprite != nullptr) {
            _renderSprite->drawArc(x, y, r, ir, startAngle, endAngle, fg, bg, smooth);
        } else {
            _tft.drawArc(x, y, r, ir, startAngle, endAngle, fg, bg, smooth);
        }
    }
    void drawFastHLine(const int32_t x, const int32_t y, const int32_t w, const uint32_t color)
    {
        if (_renderSprite != nullptr) {
            _renderSprite->drawFastHLine(x, y, w, color);
        } else {
            _tft.drawFastHLine(x, y, w, color);
        }
    }
    void drawFastVLine(const int32_t x, const int32_t y, const int32_t h, const uint32_t color)
    {
        if (_renderSprite != nullptr) {
            _renderSprite->drawFastVLine(x, y, h, color);
        } else {
            _tft.drawFastVLine(x, y, h, color);
        }
    }
    void drawLine(const int32_t x0, const int32_t y0, const int32_t x1, const int32_t y1, const uint32_t color)
    {
        if (_renderSprite != nullptr) {
            _renderSprite->drawLine(x0, y0, x1, y1, color);
        } else {
            _tft.drawLine(x0, y0, x1, y1, color);
        }
    }
    void drawWideLine(const float ax, const float ay, const float bx, const float by, const float width, const uint32_t color, const uint32_t background)
    {
        if (_renderSprite != nullptr) {
            _renderSprite->drawWideLine(ax, ay, bx, by, width, color, background);
        } else {
            _tft.drawWideLine(ax, ay, bx, by, width, color, background);
        }
    }
    void drawPixel(const int32_t x, const int32_t y, const uint32_t color)
    {
        if (_renderSprite != nullptr) {
            _renderSprite->drawPixel(x, y, color);
        } else {
            _tft.drawPixel(x, y, color);
        }
    }
    void drawCircle(const int32_t x, const int32_t y, const int32_t radius, const uint32_t color)
    {
        if (_renderSprite != nullptr) {
            _renderSprite->drawCircle(x, y, radius, color);
        } else {
            _tft.drawCircle(x, y, radius, color);
        }
    }
    void fillCircle(const int32_t x, const int32_t y, const int32_t radius, const uint32_t color)
    {
        if (_renderSprite != nullptr) {
            _renderSprite->fillCircle(x, y, radius, color);
        } else {
            _tft.fillCircle(x, y, radius, color);
        }
    }
    void fillEllipse(const int16_t x, const int16_t y, const int32_t rx, const int32_t ry, const uint16_t color)
    {
        if (_renderSprite != nullptr) {
            _renderSprite->fillEllipse(x, y, rx, ry, color);
        } else {
            _tft.fillEllipse(x, y, rx, ry, color);
        }
    }
    void fillTriangle(const int32_t x0, const int32_t y0, const int32_t x1, const int32_t y1, const int32_t x2, const int32_t y2, const uint32_t color)
    {
        if (_renderSprite != nullptr) {
            _renderSprite->fillTriangle(x0, y0, x1, y1, x2, y2, color);
        } else {
            _tft.fillTriangle(x0, y0, x1, y1, x2, y2, color);
        }
    }
    int16_t drawString(const char* string, const int32_t x, const int32_t y, const uint8_t font)
    {
        return _renderSprite != nullptr
            ? _renderSprite->drawString(string, x, y, font)
            : _tft.drawString(string, x, y, font);
    }
    int16_t drawString(const String& string, const int32_t x, const int32_t y, const uint8_t font)
    {
        return _renderSprite != nullptr
            ? _renderSprite->drawString(string, x, y, font)
            : _tft.drawString(string, x, y, font);
    }
    int16_t textWidth(const char* string, const uint8_t font)
    {
        return _renderSprite != nullptr
            ? _renderSprite->textWidth(string, font)
            : _tft.textWidth(string, font);
    }
};

RenderTarget _gfx;
float _powerHistory[PowerHistoryBuckets] = {};
uint16_t _powerHistorySamples[PowerHistoryBuckets] = {};
uint32_t _yieldHistoryDates[YieldHistoryDays] = {};
float _yieldHistoryWh[YieldHistoryDays] = {};
bool _yieldHistoryLoaded = false;
uint32_t _yieldHistoryLastDate = 0;
uint32_t _yieldHistoryLastPersistMillis = 0;
float _yieldHistoryLastSavedTodayWh = -1.0f;

void drawBoldString(const char* string, const int32_t x, const int32_t y, const uint8_t font)
{
    _gfx.drawString(string, x, y, font);
    _gfx.drawString(string, x + 1, y, font);
}

struct YieldHistoryStore {
    uint32_t magic;
    uint16_t version;
    uint16_t reserved;
    uint32_t dates[YieldHistoryDays];
    float wh[YieldHistoryDays];
};

struct DisplaySettingsStore {
    uint32_t magic;
    uint16_t version;
    uint8_t themeIndex;
    uint8_t pageIndex;
};

void applyTheme(const uint8_t index)
{
    const Theme& theme = Themes[index % ThemeCount];
    ColorBackground = TFT_BLACK;
    ColorPanel = theme.panel;
    ColorPanel2 = theme.panel2;
    ColorPanelBorder = theme.border;
    ColorMuted = theme.primary;
    ColorText = theme.text;
    ColorBlue = theme.primary;
    ColorCyan = theme.secondary;
    ColorGreen = theme.success;
    ColorAmber = theme.warning;
    ColorPurple = theme.purple;
    ColorRed = theme.red;
    ColorGrid = theme.grid;
    ColorInactive = 0x4208;
    ColorNeutralBar = theme.neutralBar;
    ColorGlow = theme.glow;
}

void saveDisplaySettings(const uint8_t themeIndex, const uint8_t pageIndex)
{
    DisplaySettingsStore store = {};
    store.magic = DisplaySettingsMagic;
    store.version = DisplaySettingsVersion;
    store.themeIndex = themeIndex % ThemeCount;
    store.pageIndex = pageIndex % PageCount;

    File file = LittleFS.open(DisplaySettingsFilename, "w");
    if (!file) {
        return;
    }
    file.write(reinterpret_cast<const uint8_t*>(&store), sizeof(store));
    file.close();
}

bool loadDisplaySettings(uint8_t& themeIndex, uint8_t& pageIndex)
{
    if (!LittleFS.exists(DisplaySettingsFilename)) {
        return false;
    }

    File file = LittleFS.open(DisplaySettingsFilename, "r", false);
    if (!file || file.size() != sizeof(DisplaySettingsStore)) {
        return false;
    }

    DisplaySettingsStore store = {};
    if (file.read(reinterpret_cast<uint8_t*>(&store), sizeof(store)) != sizeof(store)) {
        return false;
    }

    if (store.magic != DisplaySettingsMagic || store.version != DisplaySettingsVersion || store.themeIndex >= ThemeCount || store.pageIndex >= PageCount) {
        return false;
    }

    themeIndex = store.themeIndex;
    pageIndex = store.pageIndex;
    return true;
}

std::shared_ptr<InverterAbstract> firstInverter()
{
    return Hoymiles.getNumInverters() > 0 ? Hoymiles.getInverterByPos(0) : nullptr;
}

bool getField(const ChannelType_t type, const ChannelNum_t channel, const FieldId_t field, float& value)
{
    const auto inv = firstInverter();
    if (inv == nullptr || !inv->Statistics()->hasChannelFieldValue(type, channel, field)) {
        return false;
    }

    value = inv->Statistics()->getChannelFieldValue(type, channel, field);
    return true;
}

bool inverterDataAvailable()
{
    const auto inv = firstInverter();
    return inv != nullptr && inv->Statistics()->getLastUpdate() > 0;
}

bool currentLocalDate(uint32_t& date)
{
    if (!Mycila::NTP.isSynced()) {
        return false;
    }

    tm timeInfo = {};
    if (!getLocalTime(&timeInfo, 0)) {
        return false;
    }
    date = static_cast<uint32_t>((timeInfo.tm_year + 1900) * 10000 + (timeInfo.tm_mon + 1) * 100 + timeInfo.tm_mday);
    return true;
}

bool localDateDaysAgo(const uint8_t daysAgo, uint32_t& date)
{
    if (!Mycila::NTP.isSynced()) {
        return false;
    }

    tm timeInfo = {};
    if (!getLocalTime(&timeInfo, 0)) {
        return false;
    }
    timeInfo.tm_hour = 12;
    timeInfo.tm_min = 0;
    timeInfo.tm_sec = 0;
    timeInfo.tm_mday -= daysAgo;
    mktime(&timeInfo);
    date = static_cast<uint32_t>((timeInfo.tm_year + 1900) * 10000 + (timeInfo.tm_mon + 1) * 100 + timeInfo.tm_mday);
    return true;
}

int8_t findYieldHistoryDate(const uint32_t date)
{
    for (uint8_t i = 0; i < YieldHistoryDays; i++) {
        if (_yieldHistoryDates[i] == date) {
            return i;
        }
    }
    return -1;
}

void saveYieldHistory()
{
    YieldHistoryStore store = {};
    store.magic = YieldHistoryMagic;
    store.version = YieldHistoryVersion;
    for (uint8_t i = 0; i < YieldHistoryDays; i++) {
        store.dates[i] = _yieldHistoryDates[i];
        store.wh[i] = _yieldHistoryWh[i];
    }

    File file = LittleFS.open(YieldHistoryFilename, "w");
    if (!file) {
        return;
    }
    file.write(reinterpret_cast<const uint8_t*>(&store), sizeof(store));
    file.close();
}

void loadYieldHistory()
{
    if (_yieldHistoryLoaded) {
        return;
    }

    _yieldHistoryLoaded = true;
    if (!LittleFS.exists(YieldHistoryFilename)) {
        return;
    }

    File file = LittleFS.open(YieldHistoryFilename, "r", false);
    if (!file || file.size() != sizeof(YieldHistoryStore)) {
        return;
    }

    YieldHistoryStore store = {};
    if (file.read(reinterpret_cast<uint8_t*>(&store), sizeof(store)) != sizeof(store)) {
        return;
    }

    if (store.magic != YieldHistoryMagic || store.version != YieldHistoryVersion) {
        return;
    }

    for (uint8_t i = 0; i < YieldHistoryDays; i++) {
        _yieldHistoryDates[i] = store.dates[i];
        _yieldHistoryWh[i] = std::max(0.0f, store.wh[i]);
    }
}

void alignYieldHistoryToToday(const uint32_t today)
{
    if (_yieldHistoryLastDate == today) {
        return;
    }

    uint32_t oldDates[YieldHistoryDays] = {};
    float oldWh[YieldHistoryDays] = {};
    for (uint8_t i = 0; i < YieldHistoryDays; i++) {
        oldDates[i] = _yieldHistoryDates[i];
        oldWh[i] = _yieldHistoryWh[i];
    }

    for (uint8_t i = 0; i < YieldHistoryDays; i++) {
        uint32_t date = 0;
        if (!localDateDaysAgo(YieldHistoryDays - 1 - i, date)) {
            return;
        }
        _yieldHistoryDates[i] = date;
        _yieldHistoryWh[i] = 0.0f;
        for (uint8_t j = 0; j < YieldHistoryDays; j++) {
            if (oldDates[j] == date) {
                _yieldHistoryWh[i] = oldWh[j];
                break;
            }
        }
    }

    _yieldHistoryLastDate = today;
}

void updateYieldHistory()
{
    loadYieldHistory();

    const float todayWh = std::max(0.0f, Datastore.getTotalAcYieldDayEnabled());
    uint32_t today = 0;
    if (!currentLocalDate(today)) {
        return;
    }

    const bool dayChanged = _yieldHistoryLastDate != 0 && _yieldHistoryLastDate != today;
    alignYieldHistoryToToday(today);
    const int8_t todayIndex = findYieldHistoryDate(today);
    if (todayIndex >= 0) {
        _yieldHistoryWh[todayIndex] = todayWh;
    }

    const uint32_t now = millis();
    const bool firstSave = _yieldHistoryLastSavedTodayWh < 0.0f;
    const bool intervalDue = now - _yieldHistoryLastPersistMillis >= YieldPersistIntervalMillis;
    const bool meaningfulChange = std::fabs(todayWh - _yieldHistoryLastSavedTodayWh) >= 1.0f;
    if (firstSave || dayChanged || (intervalDue && meaningfulChange)) {
        saveYieldHistory();
        _yieldHistoryLastPersistMillis = now;
        _yieldHistoryLastSavedTodayWh = todayWh;
    }
}

void recordPowerSample()
{
    if (!Mycila::NTP.isSynced()) {
        return;
    }

    tm timeInfo = {};
    if (!getLocalTime(&timeInfo, 0)) {
        return;
    }
    const uint16_t minuteOfDay = static_cast<uint16_t>(timeInfo.tm_hour * 60 + timeInfo.tm_min);
    if (minuteOfDay < PowerHistoryStartMinute || minuteOfDay > PowerHistoryEndMinute) {
        return;
    }

    const uint16_t range = PowerHistoryEndMinute - PowerHistoryStartMinute;
    const uint8_t bucket = std::min<uint16_t>(
        PowerHistoryBuckets - 1,
        static_cast<uint32_t>(minuteOfDay - PowerHistoryStartMinute) * (PowerHistoryBuckets - 1) / range);
    const float power = Datastore.getTotalAcPowerEnabled();
    const uint16_t samples = _powerHistorySamples[bucket];
    _powerHistory[bucket] = samples == 0
        ? power
        : (_powerHistory[bucket] * static_cast<float>(samples) + power) / static_cast<float>(samples + 1);
    if (_powerHistorySamples[bucket] < UINT16_MAX) {
        _powerHistorySamples[bucket]++;
    }
}

float averagePower()
{
    float sum = 0.0f;
    uint8_t count = 0;
    for (uint8_t i = 0; i < PowerHistoryBuckets; i++) {
        if (_powerHistorySamples[i] == 0) {
            continue;
        }
        sum += _powerHistory[i];
        count++;
    }

    if (count == 0) {
        return Datastore.getTotalAcPowerEnabled();
    }

    return sum / static_cast<float>(count);
}

void formatPower(char* value, const size_t valueSize, char* unit, const size_t unitSize, const float watts)
{
    if (watts >= 1000.0f) {
        snprintf(value, valueSize, "%.1f", watts / 1000.0f);
        snprintf(unit, unitSize, "kW");
        return;
    }

    snprintf(value, valueSize, "%.0f", watts);
    snprintf(unit, unitSize, "W");
}

void formatEnergy(char* value, const size_t valueSize, char* unit, const size_t unitSize, const float wattHours)
{
    if (wattHours >= 10000.0f) {
        snprintf(value, valueSize, "%.2f", wattHours / 1000.0f);
        snprintf(unit, unitSize, "kWh");
        return;
    }

    snprintf(value, valueSize, "%.0f", wattHours);
    snprintf(unit, unitSize, "Wh");
}

void formatKiloWattHours(char* value, const size_t valueSize, const float wattHours)
{
    snprintf(value, valueSize, "%.1f", wattHours / 1000.0f);
}

void formatLimit(char* buffer, const size_t size)
{
    const auto inv = firstInverter();
    if (inv == nullptr) {
        snprintf(buffer, size, "--");
        return;
    }

    const float percent = constrain(inv->SystemConfigPara()->getLimitPercent(), 0.0f, 100.0f);
    snprintf(buffer, size, "%.0f%%", percent);
}

int16_t easeInOut(const uint8_t frame, const uint8_t frames, const int16_t distance)
{
    const float t = static_cast<float>(frame) / static_cast<float>(frames);
    const float eased = t < 0.5f ? 2.0f * t * t : 1.0f - std::pow(-2.0f * t + 2.0f, 2.0f) / 2.0f;
    return static_cast<int16_t>(distance * eased);
}

void drawPageDots(const uint8_t page)
{
    constexpr int16_t DotSpacing = 13;
    constexpr int16_t DotY = 164;
    const int16_t startX = (320 - (PageCount - 1) * DotSpacing) / 2;
    for (uint8_t i = 0; i < PageCount; i++) {
        const int16_t x = startX + i * DotSpacing;
        _gfx.drawCircle(x, DotY, 3, ColorMuted);
        if (i == page) {
            _gfx.fillCircle(x, DotY, 4, ColorBlue);
        }
    }
}

void drawTile(const int16_t x, const int16_t y, const int16_t w, const int16_t h)
{
    constexpr int16_t Radius = 6;
    _gfx.fillRoundRect(x, y, w, h, Radius, ColorPanel);
    _gfx.drawRoundRect(x, y, w, h, Radius, ColorPanelBorder);
    if (w > 18 && h > 18) {
        _gfx.drawRoundRect(x + 1, y + 1, w - 2, h - 2, Radius - 1, ColorPanel2);
    }
}

void drawMetricTile(const int16_t x, const int16_t y, const int16_t w, const int16_t h, const char* title, const char* value, const uint16_t titleColor, const uint16_t valueColor, const uint8_t valueFont = 2)
{
    (void)titleColor;
    (void)valueColor;
    (void)valueFont;
    drawTile(x, y, w, h);
    const int16_t cx = x + w / 2;
    _gfx.setTextDatum(MC_DATUM);
    _gfx.setTextColor(ColorMuted, ColorPanel);
    if (h < 30) {
        drawBoldString(title, cx, y + 6, 1);
        _gfx.setTextColor(ColorText, ColorPanel);
        _gfx.drawString(value, cx, y + 16, 1);
        return;
    }

    drawBoldString(title, cx, y + (h >= 60 ? 14 : 13), 2);
    _gfx.setTextColor(ColorText, ColorPanel);
    _gfx.drawString(value, cx, y + (h >= 60 ? 45 : 32), 2);
}

void formatClock(char* buffer, const size_t size)
{
    if (!Mycila::NTP.isSynced()) {
        snprintf(buffer, size, "--:-- --");
        return;
    }

    tm timeInfo = {};
    if (!getLocalTime(&timeInfo, 0)) {
        snprintf(buffer, size, "--:-- --");
        return;
    }

    const bool pm = timeInfo.tm_hour >= 12;
    int hour12 = timeInfo.tm_hour % 12;
    if (hour12 == 0) {
        hour12 = 12;
    }
    snprintf(buffer, size, "%02d:%02d %s", hour12, timeInfo.tm_min, pm ? "PM" : "AM");
}

void drawDashboardFrame(const uint8_t page, const char* title)
{
    (void)title;
    _gfx.fillScreen(ColorBackground);
    if (!_suppressPageDots) {
        drawPageDots(page);
    }
}

void drawSolidArc(const int16_t cx, const int16_t cy, const int16_t r, const int16_t thickness, const float startDeg, const float sweepDeg, const float ratio, const uint16_t color)
{
    const float clamped = constrain(ratio, 0.0f, 1.0f);
    if (clamped <= 0.0f) {
        return;
    }

    const uint32_t startAngle = 90U + static_cast<uint32_t>(std::round((startDeg - 180.0f) * sweepDeg / 180.0f));
    const uint32_t endAngle = std::min<uint32_t>(270U, startAngle + static_cast<uint32_t>(std::round(180.0f * clamped)));
    if (endAngle <= startAngle) {
        return;
    }

    _gfx.drawArc(cx, cy, r, r - thickness, startAngle, endAngle, color, ColorPanel, true);
}

void drawPowerGauge(const int16_t cx, const int16_t cy, const int16_t r, const int16_t thickness, const float watts, const uint16_t active)
{
    drawSolidArc(cx, cy, r, thickness, 180.0f, 180.0f, 1.0f, ColorInactive);
    drawSolidArc(cx, cy, r, thickness, 180.0f, 180.0f, watts / 1600.0f, active);
}

void drawOverallIcon(const int16_t cx, const int16_t cy, const uint16_t color)
{
    _gfx.drawArc(cx, cy, 26, 24, 0, 360, color, ColorPanel, true);
    _gfx.fillRoundRect(cx - 10, cy + 3, 4, 9, 1, color);
    _gfx.fillRoundRect(cx - 2, cy - 4, 4, 16, 1, color);
    _gfx.fillRoundRect(cx + 6, cy - 11, 4, 23, 1, color);
    _gfx.drawWideLine(cx - 12, cy + 12, cx + 13, cy + 12, 2.0f, color, ColorPanel);
}

void drawLeafIcon(const int16_t cx, const int16_t cy, const uint16_t color)
{
    _gfx.drawArc(cx, cy, 26, 24, 0, 360, color, ColorPanel, true);
    _gfx.fillEllipse(cx + 2, cy - 1, 8, 12, color);
    _gfx.fillTriangle(cx - 9, cy + 8, cx + 5, cy - 16, cx + 11, cy - 2, color);
    _gfx.fillTriangle(cx - 9, cy + 8, cx + 10, cy + 1, cx + 1, cy + 12, color);
    _gfx.drawWideLine(cx - 8, cy + 14, cx - 5, cy + 7, 3.0f, color, ColorPanel);
    _gfx.drawWideLine(cx - 6, cy + 7, cx + 8, cy - 10, 2.0f, ColorPanel, color);
}

bool useEuroCurrency()
{
    return strncmp(Configuration.get().Ntp.TimezoneDescr, "Europe/", 7) == 0;
}

const char* currencyCode()
{
    return useEuroCurrency() ? "EUR" : "USD";
}

void drawCurrencyIcon(const int16_t cx, const int16_t cy, const uint16_t color)
{
    _gfx.drawArc(cx, cy, 26, 24, 0, 360, color, ColorPanel, true);
    _gfx.setTextDatum(MC_DATUM);
    _gfx.setTextColor(color, ColorPanel);
    if (useEuroCurrency()) {
        _gfx.drawWideLine(cx + 8, cy - 10, cx - 4, cy - 10, 3.0f, color, ColorPanel);
        _gfx.drawWideLine(cx - 4, cy - 10, cx - 11, cy - 4, 3.0f, color, ColorPanel);
        _gfx.drawWideLine(cx - 11, cy - 4, cx - 11, cy + 4, 3.0f, color, ColorPanel);
        _gfx.drawWideLine(cx - 11, cy + 4, cx - 4, cy + 10, 3.0f, color, ColorPanel);
        _gfx.drawWideLine(cx - 4, cy + 10, cx + 8, cy + 10, 3.0f, color, ColorPanel);
        _gfx.drawWideLine(cx - 12, cy - 3, cx + 6, cy - 3, 2.0f, color, ColorPanel);
        _gfx.drawWideLine(cx - 12, cy + 4, cx + 5, cy + 4, 2.0f, color, ColorPanel);
    } else {
        _gfx.drawString("$", cx, cy + 1, 2);
    }
}

void drawOverviewPage()
{
    const float power = Datastore.getTotalAcPowerEnabled();
    char pv[12], pu[5], dayKWh[12], total[12], limit[18];
    formatPower(pv, sizeof(pv), pu, sizeof(pu), power);
    formatKiloWattHours(dayKWh, sizeof(dayKWh), Datastore.getTotalAcYieldDayEnabled());
    snprintf(total, sizeof(total), "%.1f", Datastore.getTotalAcYieldTotalEnabled());
    formatLimit(limit, sizeof(limit));

    drawDashboardFrame(0, "OVERVIEW");

    drawTile(6, 5, 210, 148);
    drawPowerGauge(111, 116, 88, 12, power, ColorBlue);
    _gfx.setTextDatum(MC_DATUM);
    _gfx.setTextColor(ColorText, ColorPanel);
    _gfx.drawString(pv, 111, 103, 6);
    _gfx.setTextColor(ColorBlue, ColorPanel);
    _gfx.drawString(pu, 111, 139, 4);
    _gfx.setTextDatum(TL_DATUM);
    _gfx.setTextColor(ColorText, ColorPanel);
    _gfx.drawString("0", 17, 132, 2);
    _gfx.drawString("800", 97, 10, 2);
    _gfx.drawString("1600", 169, 132, 2);

    drawMetricTile(224, 5, 90, 45, "TODAY kWh", dayKWh, ColorAmber, ColorText, 2);
    drawMetricTile(224, 56, 90, 45, "TOTAL kWh", total, ColorPurple, ColorCyan, 2);
    drawMetricTile(224, 107, 90, 45, "OUTPUT", limit, ColorGreen, ColorText, 1);
}

void drawPanelPowerPage()
{
    drawDashboardFrame(1, "SOLAR - POWER PER PANEL");
    char text[20];
    const ChannelNum_t channels[] = { CH0, CH1, CH2, CH3 };
    const char* labels[] = { "P1", "P2", "P3", "P4" };
    float maxPanel = 1.0f;
    float powers[4] = {};
    for (uint8_t i = 0; i < 4; i++) {
        getField(TYPE_DC, channels[i], FLD_PDC, powers[i]);
        maxPanel = std::max(maxPanel, powers[i]);
    }

    char pv[12], pu[5];
    formatPower(pv, sizeof(pv), pu, sizeof(pu), Datastore.getTotalAcPowerEnabled());
    float voltage = 0.0f;
    float current = 0.0f;
    float frequency = 0.0f;
    getField(TYPE_AC, CH0, FLD_UAC, voltage);
    getField(TYPE_AC, CH0, FLD_IAC, current);
    getField(TYPE_AC, CH0, FLD_F, frequency);

    const char* metricLabels[] = { "VOLTAGE", "CURRENT", "FREQUENCY" };
    char metricValues[3][18];
    snprintf(metricValues[0], sizeof(metricValues[0]), "%.1f V", voltage);
    snprintf(metricValues[1], sizeof(metricValues[1]), "%.2f A", current);
    snprintf(metricValues[2], sizeof(metricValues[2]), "%.2f Hz", frequency);

    for (uint8_t i = 0; i < 3; i++) {
        const uint16_t valueColors[] = { ColorBlue, ColorCyan, ColorGreen };
        drawMetricTile(228, 4 + i * 50, 88, 45, metricLabels[i], metricValues[i], ColorMuted, valueColors[i]);
    }

    drawTile(4, 4, 218, 145);
    _gfx.setTextDatum(TL_DATUM);
    _gfx.setTextColor(ColorMuted, ColorPanel);
    drawBoldString("Power per Panel (W)", 50, 10, 2);
    const int16_t chartX = 34;
    const int16_t chartY = 42;
    const int16_t chartW = 174;
    const int16_t chartH = 83;
    const float scaleMax = std::max(300.0f, std::ceil(maxPanel / 100.0f) * 100.0f);
    _gfx.drawFastVLine(chartX, chartY, chartH, ColorPanelBorder);
    _gfx.drawFastHLine(chartX, chartY + chartH, chartW, ColorPanelBorder);
    for (uint8_t tick = 0; tick <= 3; tick++) {
        const int16_t y = chartY + chartH - tick * (chartH / 3);
        _gfx.drawFastHLine(chartX, y, chartW, ColorGrid);
        snprintf(text, sizeof(text), "%.0f", scaleMax * tick / 3.0f);
        _gfx.setTextColor(ColorText, ColorPanel);
        _gfx.drawString(text, 12, y - 5, 1);
    }
    for (uint8_t i = 0; i < 4; i++) {
        const int16_t x = chartX + 18 + i * 40;
        const int16_t h = static_cast<int16_t>(constrain(powers[i] / scaleMax, 0.0f, 1.0f) * (chartH - 5));
        _gfx.fillRect(x, chartY + chartH - h, 25, h, ColorBlue);
        _gfx.drawRect(x, chartY + chartH - h, 25, h, ColorGlow);
        snprintf(text, sizeof(text), "%.0f", powers[i]);
        _gfx.setTextDatum(MC_DATUM);
        _gfx.setTextColor(ColorText, ColorPanel);
        _gfx.drawString(text, x + 12, max<int16_t>(chartY + 10, chartY + chartH - h - 14), 2);
        _gfx.setTextDatum(TL_DATUM);
        _gfx.setTextColor(ColorText, ColorPanel);
        drawBoldString(labels[i], x + 5, chartY + chartH + 3, 2);
    }
}

void drawPowerHistoryPage()
{
    drawDashboardFrame(2, "POWER / HISTORY");
    drawTile(4, 4, 232, 145);
    drawTile(240, 4, 76, 145);
    const int16_t chartX = 51;
    const int16_t chartY = 8;
    const int16_t chartW = 166;
    const int16_t chartH = 110;
    const float currentPower = Datastore.getTotalAcPowerEnabled();
    const float avgPower = averagePower();
    float maxPower = std::max(1200.0f, std::max(currentPower, avgPower));
    for (uint8_t i = 0; i < PowerHistoryBuckets; i++) {
        if (_powerHistorySamples[i] > 0) {
            maxPower = std::max(maxPower, _powerHistory[i]);
        }
    }
    maxPower = std::ceil(maxPower / 400.0f) * 400.0f;

    _gfx.drawFastVLine(chartX, chartY, chartH, ColorPanelBorder);
    _gfx.drawFastHLine(chartX, chartY + chartH, chartW, ColorPanelBorder);
    _gfx.setTextDatum(TL_DATUM);
    for (uint8_t tick = 0; tick <= 3; tick++) {
        const int16_t y = chartY + chartH - tick * (chartH / 3);
        _gfx.drawFastHLine(chartX, y, chartW, ColorGrid);
        char label[12];
        const float watts = maxPower * tick / 3.0f;
        snprintf(label, sizeof(label), watts >= 1000.0f ? "%.1fkW" : "%.0fW", watts >= 1000.0f ? watts / 1000.0f : watts);
        _gfx.setTextColor(ColorText, ColorPanel);
        _gfx.drawString(label, 8, y - 8, 2);
    }

    const int16_t avgY = chartY + chartH - static_cast<int16_t>(constrain(avgPower / maxPower, 0.0f, 1.0f) * chartH);
    _gfx.drawFastHLine(chartX, avgY, chartW, ColorAmber);
    int16_t lastX = chartX;
    int16_t lastY = chartY + chartH;
    bool hasLast = false;
    for (uint8_t i = 0; i < PowerHistoryBuckets; i++) {
        if (_powerHistorySamples[i] == 0) {
            hasLast = false;
            continue;
        }
        const float sample = _powerHistory[i];
        const int16_t x = chartX + static_cast<int16_t>(i * chartW / (PowerHistoryBuckets - 1));
        const int16_t y = chartY + chartH - static_cast<int16_t>(constrain(sample / maxPower, 0.0f, 1.0f) * chartH);
        if (hasLast) {
            _gfx.drawLine(lastX, lastY, x, y, ColorBlue);
            _gfx.drawLine(lastX, lastY + 1, x, y + 1, ColorCyan);
        }
        _gfx.fillCircle(x, y, 1, ColorCyan);
        lastX = x;
        lastY = y;
        hasLast = true;
    }
    const char* times[] = { "06", "09", "12", "15", "18" };
    for (uint8_t i = 0; i < 5; i++) {
        _gfx.setTextColor(ColorText, ColorPanel);
        _gfx.drawString(times[i], chartX - 5 + i * 40, chartY + chartH + 9, 1);
    }
    char pv[12], pu[5];
    char av[12], au[5];
    formatPower(pv, sizeof(pv), pu, sizeof(pu), currentPower);
    formatPower(av, sizeof(av), au, sizeof(au), avgPower);
    _gfx.setTextDatum(MC_DATUM);
    _gfx.setTextColor(ColorText, ColorPanel);
    _gfx.drawString(pv, 278, 44, 4);
    _gfx.setTextColor(ColorGreen, ColorPanel);
    _gfx.drawString(pu, 278, 66, 2);
    _gfx.setTextColor(ColorMuted, ColorPanel);
    drawBoldString("AVERAGE", 278, 92, 2);
    _gfx.setTextColor(ColorGreen, ColorPanel);
    _gfx.drawString(av, 278, 113, 2);
    _gfx.drawString(au, 278, 130, 1);
}

void drawYieldPage()
{
    drawDashboardFrame(3, "DAILY YIELD");

    char day[12], dayU[5], yesterday[12], yesterdayU[5], beforeYesterday[12], beforeYesterdayU[5];
    formatEnergy(day, sizeof(day), dayU, sizeof(dayU), _yieldHistoryWh[6]);
    formatEnergy(yesterday, sizeof(yesterday), yesterdayU, sizeof(yesterdayU), _yieldHistoryWh[5]);
    formatEnergy(beforeYesterday, sizeof(beforeYesterday), beforeYesterdayU, sizeof(beforeYesterdayU), _yieldHistoryWh[4]);
    const char* labels[] = { "TODAY", "YESTERDAY", "2 DAYS AGO" };
    const char* vals[] = { day, yesterday, beforeYesterday };
    const char* units[] = { dayU, yesterdayU, beforeYesterdayU };
    char combinedVals[3][18];
    for (uint8_t i = 0; i < 3; i++) {
        snprintf(combinedVals[i], sizeof(combinedVals[i]), "%s %s", vals[i], units[i]);
    }
    for (uint8_t i = 0; i < 3; i++) {
        drawMetricTile(214, 4 + i * 50, 102, 45, labels[i], combinedVals[i], ColorAmber, i == 0 ? ColorText : ColorCyan);
    }
    drawTile(4, 4, 204, 145);
    _gfx.setTextDatum(TL_DATUM);
    _gfx.setTextColor(ColorMuted, ColorPanel);
    drawBoldString("Daily Yield (kWh)", 39, 10, 2);
    float bars[YieldHistoryDays] = {};
    float maxKWh = 0.0f;
    for (uint8_t i = 0; i < YieldHistoryDays; i++) {
        bars[i] = _yieldHistoryWh[i] / 1000.0f;
        maxKWh = std::max(maxKWh, bars[i]);
    }
    const float scaleMax = std::max(10.0f, std::ceil(maxKWh / 5.0f) * 5.0f);
    const float scaleMid = scaleMax / 2.0f;
    const int16_t chartX = 33;
    const int16_t chartY = 40;
    const int16_t chartW = 157;
    const int16_t chartH = 86;
    _gfx.drawFastVLine(chartX, chartY, chartH, ColorPanelBorder);
    _gfx.drawFastHLine(chartX, chartY + chartH, chartW, ColorPanelBorder);
    for (uint8_t tick = 0; tick <= 2; tick++) {
        const int16_t y = chartY + chartH - tick * (chartH / 2);
        _gfx.drawFastHLine(chartX, y, chartW, ColorGrid);
        char scaleLabel[8];
        snprintf(scaleLabel, sizeof(scaleLabel), tick == 0 ? "0" : "%.0f", tick == 1 ? scaleMid : scaleMax);
        _gfx.setTextColor(ColorText, ColorPanel);
        _gfx.drawString(scaleLabel, 13, y - 5, 1);
    }
    for (uint8_t i = 0; i < YieldHistoryDays; i++) {
        const int16_t x = chartX + 12 + i * 20;
        const int16_t h = static_cast<int16_t>(constrain(bars[i] / scaleMax, 0.0f, 1.0f) * (chartH - 3));
        const int16_t barTop = chartY + chartH - h;
        const uint16_t barColor = i == 6 ? ColorAmber : ColorNeutralBar;
        _gfx.fillRect(x, barTop, 14, h, barColor);
        char valueLabel[8];
        snprintf(valueLabel, sizeof(valueLabel), "%.1f", bars[i]);
        _gfx.setTextDatum(MC_DATUM);
        _gfx.setTextColor(i == 6 ? ColorAmber : ColorText, ColorPanel);
        const int16_t valueY = h > 0 ? max<int16_t>(chartY + 8, barTop - 8) : chartY + chartH - 8;
        _gfx.drawString(valueLabel, x + 7, valueY, 1);
        char label[8];
        snprintf(label, sizeof(label), i == 6 ? "0" : "-%u", 6 - i);
        _gfx.setTextDatum(TL_DATUM);
        _gfx.setTextColor(ColorText, ColorPanel);
        _gfx.drawString(label, x - 1, chartY + chartH + 7, 1);
    }
}

void drawEnergyStatsPage()
{
    drawDashboardFrame(4, "ENERGY / STATS");
    char saved[12], total[12], co2[12], totalValue[12], totalUnit[5];
    const float totalKWh = Datastore.getTotalAcYieldTotalEnabled();
    const float co2Kg = totalKWh * 0.6f;
    const float savedCurrency = totalKWh * 0.32f;
    snprintf(saved, sizeof(saved), "%.1f", savedCurrency);
    snprintf(co2, sizeof(co2), "%.1f", co2Kg);
    if (totalKWh >= 1000.0f) {
        snprintf(totalValue, sizeof(totalValue), "%.2f", totalKWh / 1000.0f);
        snprintf(totalUnit, sizeof(totalUnit), "MWh");
    } else {
        snprintf(totalValue, sizeof(totalValue), "%.1f", totalKWh);
        snprintf(totalUnit, sizeof(totalUnit), "kWh");
    }
    snprintf(total, sizeof(total), "%s", totalValue);
    const uint16_t iconColors[] = { 0x3C1E, 0xED81, 0x262B };
    constexpr int16_t TileY = 4;
    constexpr int16_t TileH = 150;
    constexpr int16_t IconY = 44;
    constexpr int16_t LabelY = 92;
    constexpr int16_t ValueY = 128;
    char savingsLabel[16];
    snprintf(savingsLabel, sizeof(savingsLabel), "SAVINGS %s", currencyCode());
    char totalLabel[16];
    snprintf(totalLabel, sizeof(totalLabel), "OVERALL %s", totalUnit);
    const char* labels[] = { savingsLabel, totalLabel, "CO2 kg" };
    const char* vals[] = { saved, total, co2 };
    for (uint8_t i = 0; i < 3; i++) {
        const int16_t x = 4 + i * 106;
        const int16_t cx = x + 50;
        drawTile(x, TileY, 100, TileH);
        if (i == 0) {
            drawCurrencyIcon(cx, IconY, iconColors[i]);
        } else if (i == 1) {
            drawOverallIcon(cx, IconY, iconColors[i]);
        } else {
            drawLeafIcon(cx, IconY, iconColors[i]);
        }
        _gfx.setTextDatum(MC_DATUM);
        _gfx.setTextColor(ColorMuted, ColorPanel);
        drawBoldString(labels[i], cx, LabelY, 2);
        _gfx.setTextColor(ColorText, ColorPanel);
        _gfx.drawString(vals[i], cx, ValueY, 4);
    }
}

void drawEnvironmentPage()
{
    drawDashboardFrame(5, "OPEN DTU / INVERTER");
    char serialText[18], dcPower[18], tempText[18], effText[18], powerFactorText[18], reactivePowerText[18];
    char value[12], unit[5];
    float temp = 0.0f;
    float efficiency = 0.0f;
    float powerFactor = 0.0f;
    float reactivePower = 0.0f;
    const auto inv = firstInverter();
    if (inv != nullptr) {
        snprintf(serialText, sizeof(serialText), "%s", inv->serialString().c_str());
    } else {
        snprintf(serialText, sizeof(serialText), "--");
    }
    formatPower(value, sizeof(value), unit, sizeof(unit), Datastore.getTotalDcPowerEnabled());
    snprintf(dcPower, sizeof(dcPower), "%s %s", value, unit);
    if (getField(TYPE_INV, CH0, FLD_T, temp)) {
        snprintf(tempText, sizeof(tempText), "%.1f C", temp);
    } else {
        snprintf(tempText, sizeof(tempText), "--");
    }
    if (getField(TYPE_INV, CH0, FLD_EFF, efficiency)) {
        snprintf(effText, sizeof(effText), "%.1f %%", efficiency);
    } else {
        snprintf(effText, sizeof(effText), "--");
    }
    if (getField(TYPE_AC, CH0, FLD_PF, powerFactor)) {
        snprintf(powerFactorText, sizeof(powerFactorText), "%.3f", powerFactor);
    } else {
        snprintf(powerFactorText, sizeof(powerFactorText), "--");
    }
    if (getField(TYPE_AC, CH0, FLD_Q, reactivePower)) {
        snprintf(reactivePowerText, sizeof(reactivePowerText), "%.1f var", reactivePower);
    } else {
        snprintf(reactivePowerText, sizeof(reactivePowerText), "--");
    }
    const char* names[] = { "SERIAL", "DC POWER", "TEMP", "EFFICIENCY", "POWER FACTOR", "REACTIVE" };
    const char* vals[] = { serialText, dcPower, tempText, effText, powerFactorText, reactivePowerText };
    for (uint8_t i = 0; i < 6; i++) {
        const int16_t x = i < 3 ? 4 : 160;
        const int16_t y = 4 + (i % 3) * 49;
        const uint16_t titleColors[] = { ColorCyan, ColorCyan, ColorAmber, ColorAmber, ColorPurple, ColorGreen };
        const uint16_t valueColors[] = { ColorBlue, ColorCyan, ColorText, ColorGreen, ColorAmber, ColorText };
        drawMetricTile(x, y, 152, 44, names[i], vals[i], titleColors[i], valueColors[i]);
    }
}

void drawNetworkPage()
{
    drawDashboardFrame(6, "NETWORK");
    char rssi[18];
    char clock[10];
    snprintf(rssi, sizeof(rssi), WiFi.isConnected() ? "%d dBm" : "-- dBm", WiFi.RSSI());
    formatClock(clock, sizeof(clock));
    String ip = WiFi.localIP().toString();
    const char* names[] = { "WLAN STATUS", "SIGNAL", "IP ADDRESS", "OPEN DTU", "TIME", "MAC ADDRESS" };
    const char* vals[] = { WiFi.isConnected() ? "Connected" : "Offline", rssi, ip.c_str(), "Online", clock, "DC:4F:22:11:33:44" };
    for (uint8_t i = 0; i < 6; i++) {
        const int16_t x = i < 3 ? 4 : 160;
        const int16_t y = 4 + (i % 3) * 49;
        const uint16_t titleColors[] = { ColorGreen, ColorGreen, ColorCyan, ColorGreen, ColorAmber, ColorPurple };
        const uint16_t valueColors[] = { ColorText, ColorGreen, ColorBlue, ColorText, ColorAmber, ColorCyan };
        drawMetricTile(x, y, 152, 44, names[i], vals[i], titleColors[i], valueColors[i]);
    }
}

void drawPage(const uint8_t page)
{
    switch (page) {
    case 1:
        drawPanelPowerPage();
        break;
    case 2:
        drawPowerHistoryPage();
        break;
    case 3:
        drawYieldPage();
        break;
    case 4:
        drawEnergyStatsPage();
        break;
    case 5:
        drawEnvironmentPage();
        break;
    case 6:
        drawNetworkPage();
        break;
    default:
        drawOverviewPage();
        break;
    }
}

bool renderPageToSprite(TFT_eSprite& sprite, const uint8_t page)
{
    sprite.setColorDepth(16);
    if (sprite.createSprite(320, 170) == nullptr) {
        return false;
    }

    sprite.setTextPadding(0);
    sprite.setTextFont(1);
    _suppressPageDots = true;
    _gfx.useSprite(&sprite);
    drawPage(page);
    _gfx.useDisplay();
    _suppressPageDots = false;
    return true;
}

void pushSpriteClipped(TFT_eSprite& sprite, const int16_t x)
{
    constexpr int16_t Width = 320;
    constexpr int16_t Height = 170;
    if (x <= -Width || x >= Width) {
        return;
    }

    const int16_t sourceX = x < 0 ? -x : 0;
    const int16_t targetX = x < 0 ? 0 : x;
    const int16_t drawWidth = Width - abs(x);
    if (drawWidth <= 0) {
        return;
    }

    sprite.pushSprite(targetX, 0, sourceX, 0, drawWidth, Height);
}

bool animatePageSlide(const uint8_t oldPage, const uint8_t newPage, const int8_t direction)
{
    constexpr int16_t Width = 320;
    constexpr uint8_t Frames = 18;
    constexpr uint8_t FrameDelayMillis = 2;

    TFT_eSprite oldSprite(&_tft);
    TFT_eSprite newSprite(&_tft);
    if (!renderPageToSprite(oldSprite, oldPage) || !renderPageToSprite(newSprite, newPage)) {
        _gfx.useDisplay();
        oldSprite.deleteSprite();
        newSprite.deleteSprite();
        return false;
    }

    for (uint8_t i = 0; i <= Frames; i++) {
        const int16_t offset = easeInOut(i, Frames, Width);
        if (direction > 0) {
            pushSpriteClipped(oldSprite, -offset);
            pushSpriteClipped(newSprite, Width - offset);
        } else {
            pushSpriteClipped(oldSprite, offset);
            pushSpriteClipped(newSprite, -Width + offset);
        }
        drawPageDots(newPage);
        delay(FrameDelayMillis);
    }

    oldSprite.deleteSprite();
    newSprite.deleteSprite();
    return true;
}

bool renderStartupLogoSprite(TFT_eSprite& sprite)
{
    constexpr uint16_t LogoBlue = 0x047F;
    constexpr uint16_t LogoText = 0xF7DF;
    sprite.setColorDepth(16);
    if (sprite.createSprite(StartupLogoSourceW, StartupLogoSourceH) == nullptr) {
        return false;
    }
    sprite.fillSprite(ColorBackground);
    sprite.setTextDatum(TL_DATUM);
    sprite.setTextSize(StartupLogoScale * 2);
    const int16_t openWidth = sprite.textWidth("Open", 4);
    const int16_t dtuWidth = sprite.textWidth("DTU", 4);
    const int16_t startX = std::max<int16_t>(0, (StartupLogoSourceW - openWidth - dtuWidth) / 2);
    const int16_t startY = std::max<int16_t>(0, (StartupLogoSourceH - 54 * StartupLogoScale) / 2);
    sprite.setTextColor(LogoText, ColorBackground);
    sprite.drawString("Open", startX, startY, 4);
    sprite.drawString("Open", startX + StartupLogoScale, startY, 4);
    sprite.setTextColor(LogoBlue, ColorBackground);
    sprite.drawString("DTU", startX + openWidth, startY, 4);
    sprite.drawString("DTU", startX + openWidth + StartupLogoScale, startY, 4);
    sprite.setTextSize(1);
    return true;
}

uint16_t sampleLogoPixel(TFT_eSprite& logo, const int16_t sx0, const int16_t sx1, const int16_t sy0, const int16_t sy1)
{
    for (int16_t y = sy0; y <= sy1; y++) {
        for (int16_t x = sx0; x <= sx1; x++) {
            const uint16_t color = logo.readPixel(std::min<int16_t>(x, StartupLogoSourceW - 1), std::min<int16_t>(y, StartupLogoSourceH - 1));
            if (color != ColorBackground) {
                return color;
            }
        }
    }
    return ColorBackground;
}

void drawScaledLogoToFrame(TFT_eSprite& frame, TFT_eSprite& logo, const int16_t centerX, const int16_t centerY, const uint8_t percent)
{
    const int16_t w = StartupLogoTargetW * percent / 100;
    const int16_t h = StartupLogoTargetH * percent / 100;
    const int16_t x0 = centerX - w / 2;
    const int16_t y0 = centerY - h / 2;
    for (int16_t y = 0; y < h; y++) {
        const int16_t sy0 = y * StartupLogoSourceH / h;
        const int16_t sy1 = ((y + 1) * StartupLogoSourceH - 1) / h;
        for (int16_t x = 0; x < w; x++) {
            const int16_t sx0 = x * StartupLogoSourceW / w;
            const int16_t sx1 = ((x + 1) * StartupLogoSourceW - 1) / w;
            const uint16_t color = sampleLogoPixel(logo, sx0, sx1, sy0, sy1);
            if (color != ColorBackground) {
                frame.drawPixel(x0 + x, y0 + y, color);
            }
        }
    }
}

void drawScaledLogoToDisplay(TFT_eSprite& logo, const int16_t centerX, const int16_t centerY, const uint8_t percent)
{
    const int16_t w = StartupLogoTargetW * percent / 100;
    const int16_t h = StartupLogoTargetH * percent / 100;
    const int16_t x0 = centerX - w / 2;
    const int16_t y0 = centerY - h / 2;
    for (int16_t y = 0; y < h; y++) {
        const int16_t sy0 = y * StartupLogoSourceH / h;
        const int16_t sy1 = ((y + 1) * StartupLogoSourceH - 1) / h;
        for (int16_t x = 0; x < w; x++) {
            const int16_t sx0 = x * StartupLogoSourceW / w;
            const int16_t sx1 = ((x + 1) * StartupLogoSourceW - 1) / w;
            const uint16_t color = sampleLogoPixel(logo, sx0, sx1, sy0, sy1);
            if (color != ColorBackground) {
                _gfx.drawPixel(x0 + x, y0 + y, color);
            }
        }
    }
}

void showStartupScreen()
{
    constexpr uint8_t ZoomFrames = 18;
    TFT_eSprite logoSprite(&_tft);
    TFT_eSprite frameSprite(&_tft);
    if (!renderStartupLogoSprite(logoSprite)) {
        return;
    }
    frameSprite.setColorDepth(16);
    const bool hasFrameSprite = frameSprite.createSprite(320, 170) != nullptr;
    _gfx.setTextPadding(0);
    _gfx.fillScreen(ColorBackground);
    for (uint8_t frame = 0; frame < ZoomFrames; frame++) {
        const uint8_t percent = 40 + static_cast<uint8_t>(60.0f * (1.0f - std::pow(1.0f - static_cast<float>(frame + 1) / ZoomFrames, 3.0f)));
        if (hasFrameSprite) {
            frameSprite.fillSprite(ColorBackground);
            drawScaledLogoToFrame(frameSprite, logoSprite, 160, 85, percent);
            frameSprite.pushSprite(0, 0);
        } else {
            _gfx.fillScreen(ColorBackground);
            drawScaledLogoToDisplay(logoSprite, 160, 85, percent);
        }
        delay(42);
    }
    delay(650);
    frameSprite.deleteSprite();
    logoSprite.deleteSprite();
}

void drawInverterWaitLayout()
{
    _gfx.setTextPadding(0);
    _gfx.setTextSize(2);
    _gfx.setTextDatum(MC_DATUM);
    _gfx.fillScreen(ColorBackground);

    _gfx.fillRoundRect(WaitPanelX, WaitPanelY, WaitPanelW, WaitPanelH, 10, ColorPanel);
    _gfx.drawRoundRect(WaitPanelX, WaitPanelY, WaitPanelW, WaitPanelH, 10, ColorPanelBorder);

    _gfx.setTextColor(ColorMuted, ColorPanel);
    drawBoldString("WAITING FOR", 160, 58, 2);
    _gfx.setTextColor(ColorText, ColorPanel);
    drawBoldString("INVERTER DATA", 160, 88, 2);
    _gfx.setTextSize(1);

    _gfx.fillRoundRect(WaitBarX, WaitBarY, WaitBarW, WaitBarH, 6, ColorInactive);
    _gfx.drawRoundRect(WaitBarX, WaitBarY, WaitBarW, WaitBarH, 6, ColorPanelBorder);
}

bool ensureWaitAnimationSprites()
{
    if (!_waitBarSpriteReady) {
        _waitBarSprite.setColorDepth(16);
        _waitBarSpriteReady = _waitBarSprite.createSprite(WaitBarW, WaitBarH) != nullptr;
    }

    if (!_waitDotsSpriteReady) {
        _waitDotsSprite.setColorDepth(16);
        _waitDotsSpriteReady = _waitDotsSprite.createSprite(WaitDotsW, WaitDotsH) != nullptr;
    }

    return _waitBarSpriteReady && _waitDotsSpriteReady;
}

int16_t waitBarSegmentOffset(const uint8_t frame)
{
    const int16_t travel = WaitBarW - WaitBarSegmentW - 4;
    const uint8_t phase = frame % 36;
    const float normalized = phase < 18
        ? static_cast<float>(phase) / 17.0f
        : static_cast<float>(35 - phase) / 17.0f;
    const float eased = 0.5f - 0.5f * std::cos(normalized * static_cast<float>(M_PI));
    return 2 + static_cast<int16_t>(travel * eased);
}

void drawInverterWaitAnimation(const uint8_t frame)
{
    if (!ensureWaitAnimationSprites()) {
        return;
    }

    _waitBarSprite.fillSprite(ColorPanel);
    _waitBarSprite.fillRoundRect(0, 0, WaitBarW, WaitBarH, 6, ColorInactive);
    _waitBarSprite.drawRoundRect(0, 0, WaitBarW, WaitBarH, 6, ColorPanelBorder);
    _waitBarSprite.fillRoundRect(waitBarSegmentOffset(frame), 2, WaitBarSegmentW, WaitBarH - 4, 4, ColorBlue);
    _waitBarSprite.pushSprite(WaitBarX, WaitBarY);

    _waitDotsSprite.fillSprite(ColorBackground);
    for (uint8_t i = 0; i < 3; i++) {
        const bool active = ((frame / 4) % 3) == i;
        _waitDotsSprite.fillCircle(10 + i * 18, 8, active ? 4 : 3, active ? ColorBlue : ColorInactive);
    }
    _waitDotsSprite.pushSprite(WaitDotsX, WaitDotsY);
}

void drawInverterWaitScreen(const uint8_t frame)
{
    drawInverterWaitLayout();
    drawInverterWaitAnimation(frame);
}

void updateInverterWaitScreen(const uint8_t frame)
{
    drawInverterWaitAnimation(frame);
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
    pinMode(ButtonPagePin, INPUT_PULLUP);
    pinMode(ButtonThemePin, INPUT_PULLUP);

    _button1LastReading = digitalRead(ButtonPagePin);
    _button2LastReading = digitalRead(ButtonThemePin);
    _button1Stable = _button1LastReading;
    _button2Stable = _button2LastReading;

    _gfx.init();
    _gfx.setRotation(1);
    _gfx.setTextPadding(0);
    loadDisplaySettings(_themeIndex, _currentPage);
    applyTheme(_themeIndex);
    loadYieldHistory();
    showStartupScreen();
    if (!inverterDataAvailable()) {
        _waitingForInverterData = true;
        _waitStartMillis = millis();
        _lastWaitFrameMillis = 0;
        _waitFrame = 0;
        drawInverterWaitScreen(_waitFrame);
        _waitLayoutDrawn = true;
        _layoutDrawn = false;
    } else {
        drawPage(_currentPage);
        _layoutDrawn = true;
    }

    _initialized = true;
}

void DisplayTDisplayS3::handleButtons()
{
    const uint32_t now = millis();

    const bool button1Reading = digitalRead(ButtonPagePin);
    if (button1Reading != _button1LastReading) {
        _button1LastReading = button1Reading;
        _button1LastChangeMillis = now;
    }
    if ((now - _button1LastChangeMillis) > ButtonDebounceMillis && button1Reading != _button1Stable) {
        _button1Stable = button1Reading;
        if (!_button1Stable) {
            if (_waitingForInverterData) {
                dismissInverterWait();
            } else {
                changePage(1);
            }
        }
    }

    const bool button2Reading = digitalRead(ButtonThemePin);
    if (button2Reading != _button2LastReading) {
        _button2LastReading = button2Reading;
        _button2LastChangeMillis = now;
    }
    if ((now - _button2LastChangeMillis) > ButtonDebounceMillis && button2Reading != _button2Stable) {
        _button2Stable = button2Reading;
        if (!_button2Stable) {
            changeTheme();
        }
    }
}

void DisplayTDisplayS3::dismissInverterWait()
{
    if (!_waitingForInverterData) {
        return;
    }

    _waitingForInverterData = false;
    _waitLayoutDrawn = false;
    _pageRedrawPending = true;
    drawPage(_currentPage);
    _lastUpdateMillis = millis();
    _layoutDrawn = true;
    _pageRedrawPending = false;
}

void DisplayTDisplayS3::changeTheme()
{
    _themeIndex = (_themeIndex + 1) % ThemeCount;
    applyTheme(_themeIndex);
    saveDisplaySettings(_themeIndex, _currentPage);
    _pageRedrawPending = true;
    if (_waitingForInverterData) {
        drawInverterWaitScreen(_waitFrame);
        _waitLayoutDrawn = true;
    } else {
        drawPage(_currentPage);
    }
    _lastUpdateMillis = millis();
    _pageRedrawPending = false;
}

void DisplayTDisplayS3::changePage(const int8_t direction)
{
    constexpr int16_t Width = 320;
    constexpr int16_t Height = 170;
    constexpr uint8_t Frames = 18;
    constexpr uint8_t FrameDelayMillis = 2;

    _previousPage = _currentPage;
    const uint8_t nextPage = direction < 0
        ? (_currentPage + PageCount - 1) % PageCount
        : (_currentPage + 1) % PageCount;

    _transitionActive = true;
    if (!animatePageSlide(_currentPage, nextPage, direction)) {
        for (uint8_t i = 0; i <= Frames; i++) {
            const int16_t width = easeInOut(i, Frames, Width);
            if (direction > 0) {
                _gfx.fillRect(Width - width, 0, width, Height, ColorBackground);
            } else {
                _gfx.fillRect(0, 0, width, Height, ColorBackground);
            }
            delay(FrameDelayMillis);
        }
        drawPage(nextPage);
    }
    _transitionActive = false;

    _currentPage = nextPage;
    saveDisplaySettings(_themeIndex, _currentPage);

    _lastUpdateMillis = millis();
    _pageRedrawPending = false;
}

void DisplayTDisplayS3::loop()
{
    if (!_initialized) {
        return;
    }

    handleButtons();
    if (_transitionActive) {
        return;
    }

    const uint32_t now = millis();
    if (_waitingForInverterData) {
        if (inverterDataAvailable() || now - _waitStartMillis >= InverterWaitTimeoutMillis) {
            dismissInverterWait();
            return;
        }
        if (now - _lastWaitFrameMillis >= InverterWaitFrameMillis) {
            _lastWaitFrameMillis = now;
            _waitFrame++;
            if (!_waitLayoutDrawn) {
                drawInverterWaitScreen(_waitFrame);
                _waitLayoutDrawn = true;
            } else {
                updateInverterWaitScreen(_waitFrame);
            }
        }
        return;
    }

    if (!_pageRedrawPending && now - _lastUpdateMillis < UpdateIntervalMillis) {
        return;
    }
    _lastUpdateMillis = now;
    recordPowerSample();
    updateYieldHistory();

    if (!_layoutDrawn) {
        _pageRedrawPending = true;
        _layoutDrawn = true;
    }

    drawPage(_currentPage);
    _pageRedrawPending = false;
}

bool DisplayTDisplayS3::isInitialized() const
{
    return _initialized;
}

#endif
