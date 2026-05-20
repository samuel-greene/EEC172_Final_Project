/*
 * movement_detection.cpp
 *
 *  Created on: May 18, 2026
 *      Author: samcf
 */
// Simplelink includes
#include "simplelink.h"

//Driverlib includes
#include "hw_types.h"
#include "hw_ints.h"
#include "hw_memmap.h"
#include "rom.h"
#include "rom_map.h"
#include "interrupt.h"
#include "prcm.h"
#include "utils.h"
#include "uart.h"

//Common interface includes
#include "pinmux.h"
#include "gpio_if.h"
#include "common.h"
#include "uart_if.h"

#include "movement_detection.h"
#include "i2c_if.h"

int theft_detected(int z, int safe_value) {
    UART_PRINT("Safe Value: %d, Current Value: %d\n\r", safe_value, z);

    if (z > (safe_value + 10) || z < (safe_value - 10)) {
        return 1;
    } else return 0;
}
