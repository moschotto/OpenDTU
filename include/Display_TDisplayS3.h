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
    uint32_t _lastUpdateMillis = 0;
    bool _initialized = false;
};

extern DisplayTDisplayS3 TDisplay;

#endif
