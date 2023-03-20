#pragma once

#include <furi.h>
#include <furi_hal.h>

#define AVR_ISP_VERSION_APP "0.1"
#define AVR_ISP_DEVELOPED "SkorP"
#define AVR_ISP_GITHUB "https://github.com/flipperdevices/flipperzero-firmware"

#define AVR_ISP_APP_EXTENSION ".avr"

typedef enum {
    //AvrIspViewVariableItemList,
    AvrIspViewSubmenu,
    AvrIspViewProgrammer,
    AvrIspViewReader,
    AvrIspViewWriter,
    AvrIspViewWidget,
    AvrIspViewTextInput,
} AvrIspView;