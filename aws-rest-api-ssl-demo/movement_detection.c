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

int theft_detected() {
    unsigned char ucDevAddr, ucRegOffset, ucRdLen;
    ucDevAddr = 0x18;
    ucRegOffset = 0x2;
    ucRdLen = 6;
    unsigned char aucRdDataBuf[256];
    I2C_IF_Write(ucDevAddr, &ucRegOffset, 1, 0);
    I2C_IF_Read(ucDevAddr, &aucRdDataBuf[0], ucRdLen);
    int x = (signed char)aucRdDataBuf[1];
    int y = (signed char)aucRdDataBuf[3];
    int z = (signed char)aucRdDataBuf[5];

    UART_PRINT("X: %d; Y: %d, Z: %d\n\r", x, y, z);

    return 0;
}
