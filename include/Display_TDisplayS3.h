// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include <Arduino.h>

#if defined(OPENDTU_LILYGO_T_DISPLAY_S3)

class DisplayTDisplayS3 final {
public:
    void init();
    void loop();

    bool isInitialized() const;

private:
    void handleButtons();
    void changePage(int8_t direction);
    void changeTheme();
    void dismissInverterWait();

    uint32_t _lastUpdateMillis = 0;
    uint32_t _waitStartMillis = 0;
    uint32_t _lastWaitFrameMillis = 0;
    uint32_t _button1LastChangeMillis = 0;
    uint32_t _button2LastChangeMillis = 0;
    bool _initialized = false;
    bool _layoutDrawn = false;
    bool _pageRedrawPending = true;
    bool _transitionActive = false;
    bool _waitingForInverterData = false;
    bool _button1LastReading = true;
    bool _button2LastReading = true;
    bool _button1Stable = true;
    bool _button2Stable = true;
    uint8_t _currentPage = 0;
    uint8_t _previousPage = 0;
    uint8_t _themeIndex = 0;
    uint8_t _waitFrame = 0;
    bool _waitLayoutDrawn = false;
};

extern DisplayTDisplayS3 TDisplay;

#endif
