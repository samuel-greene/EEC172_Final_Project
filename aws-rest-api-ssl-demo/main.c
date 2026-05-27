#include <stdio.h>

// Simplelink includes
#include "simplelink.h"

//Driverlib includes
#include "hw_types.h"
#include "hw_ints.h"
#include "hw_memmap.h"
#include "rom.h"
#include "gpio.h"
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

// Custom includes
#include "utils/network_utils.h"
#include "movement_detection.h"
#include "i2c_if.h"

//NEED TO UPDATE THIS FOR IT TO WORK!
#define DATE                20    /* Current Date */
#define MONTH               5     /* Month 1-12 */
#define YEAR                2026  /* Current year */
#define HOUR                9    /* Time - hours */
#define MINUTE              35    /* Time - minutes */
#define SECOND              00     /* Time - seconds */


#define APPLICATION_NAME      "SSL"
#define APPLICATION_VERSION   "SQ24"
#define SERVER_NAME           "a252vw5j8pdxk3-ats.iot.us-east-1.amazonaws.com" // CHANGE ME
#define GOOGLE_DST_PORT       8443


#define POSTHEADER "POST /things/Yao_CC3200_Board/shadow HTTP/1.1\r\n"             // CHANGE ME
#define HOSTHEADER "Host: a252vw5j8pdxk3-ats.iot.us-east-1.amazonaws.com\r\n"  // CHANGE ME
#define CHEADER "Connection: Keep-Alive\r\n"
#define CTHEADER "Content-Type: application/json; charset=utf-8\r\n"
#define CLHEADER1 "Content-Length: "
#define CLHEADER2 "\r\n\r\n"

#define MAX_MESSAGE_LEN      120
#define MAX_ESCAPED_LEN      240


#define SW2_GPIO_BASE  GPIOA1_BASE
#define SW2_GPIO_PIN   0x20

//*****************************************************************************
//                 GLOBAL VARIABLES -- Start
//*****************************************************************************

#if defined(ccs) || defined(gcc)
extern void (* const g_pfnVectors[])(void);
#endif
#if defined(ewarm)
extern uVectorEntry __vector_table;
#endif

//*****************************************************************************
//                 GLOBAL VARIABLES -- End: df
//*****************************************************************************


//****************************************************************************
//                      LOCAL FUNCTION PROTOTYPES
//****************************************************************************
static int set_time();
static void BoardInit(void);
static void json_escape(const char *src, char *dst, int maxLen);
static int http_post(int, const char *);

//*****************************************************************************
//
//! Board Initialization & Configuration
//!
//! \param  None
//!
//! \return None
//
//*****************************************************************************
static void BoardInit(void) {
/* In case of TI-RTOS vector table is initialize by OS itself */
#ifndef USE_TIRTOS
  //
  // Set vector table base
  //
#if defined(ccs)
    MAP_IntVTableBaseSet((unsigned long)&g_pfnVectors[0]);
#endif
#if defined(ewarm)
    MAP_IntVTableBaseSet((unsigned long)&__vector_table);
#endif
#endif
    //
    // Enable Processor
    //
    MAP_IntMasterEnable();
    MAP_IntEnable(FAULT_SYSTICK);

    PRCMCC3200MCUInit();
}

static unsigned char g_sw2IdleLevel = 0;

static int IsSW3Pressed(void)
{
    return GPIOPinRead(SW2_GPIO_BASE, SW2_GPIO_PIN) != g_sw2IdleLevel;
}
//*****************************************************************************
//
//! This function updates the date and time of CC3200.
//!
//! \param None
//!
//! \return
//!     0 for success, negative otherwise
//!
//*****************************************************************************

static int set_time() {
    long retVal;

    g_time.tm_day = DATE;
    g_time.tm_mon = MONTH;
    g_time.tm_year = YEAR;
    g_time.tm_sec = HOUR;
    g_time.tm_hour = MINUTE;
    g_time.tm_min = SECOND;

    retVal = sl_DevSet(SL_DEVICE_GENERAL_CONFIGURATION,
                          SL_DEVICE_GENERAL_CONFIGURATION_DATE_TIME,
                          sizeof(SlDateTime),(unsigned char *)(&g_time));

    ASSERT_ON_ERROR(retVal);
    return SUCCESS;
}

static void json_escape(const char *src, char *dst, int maxLen) {
    int i = 0;
    int j = 0;

    while(src[i] != '\0' && j < (maxLen - 1)) {
        if((src[i] == '\"' || src[i] == '\\') && j < (maxLen - 2)) {
            dst[j++] = '\\';
            dst[j++] = src[i];
        }
        else if(src[i] >= 32 && src[i] <= 126) {
            dst[j++] = src[i];
        }
        i++;
    }

    dst[j] = '\0';
}


//*****************************************************************************
//
//! Main 
//!
//! \param  none
//!
//! \return None
//!
//*****************************************************************************
void main() {
    long lRetVal = -1;

    int DEBUG_MODE = 1;
    int ARMED = 0;
    //
    // Initialize board configuration
    //
    BoardInit();

    PinMuxConfig();

    InitTerm();
    ClearTerm();


    // initialize global default app configuration
    g_app_config.host = SERVER_NAME;
    g_app_config.port = GOOGLE_DST_PORT;

    if (!DEBUG_MODE) {
        //Connect the CC3200 to the local access point
        lRetVal = connectToAccessPoint();
        //Set time so that encryption can be used
        lRetVal = set_time();
        if(lRetVal < 0) {
            UART_PRINT("Unable to set time in the device");
            LOOP_FOREVER();
        }
    }

    I2C_IF_Open(I2C_MASTER_MODE_FST);

    unsigned char ucDevAddr, ucRegOffset, ucRdLen;
    ucDevAddr = 0x18;
    ucRegOffset = 0x2;
    ucRdLen = 6;
    unsigned char aucRdDataBuf[256];
    int x;
    int y;
    int z;

    UART_PRINT("Ready to be armed...\n\r");

    while (true) {
        if (IsSW3Pressed()) { // button pressed
            UART_PRINT("Arming in 3...\n\r");
            MAP_UtilsDelay(10000000); // one second
            UART_PRINT("Arming in 2...\n\r");
            MAP_UtilsDelay(10000000);
            UART_PRINT("Arming in 1...\n\r");
            MAP_UtilsDelay(10000000);

            I2C_IF_Write(ucDevAddr, &ucRegOffset, 1, 0);
            I2C_IF_Read(ucDevAddr, &aucRdDataBuf[0], ucRdLen);
            z = (signed char)aucRdDataBuf[5];
            ARMED = true;
            int safe_value = z;
            UART_PRINT("ARMED\n\r");

            while (ARMED) {
                I2C_IF_Write(ucDevAddr, &ucRegOffset, 1, 0);
                I2C_IF_Read(ucDevAddr, &aucRdDataBuf[0], ucRdLen);
//                x = (signed char)aucRdDataBuf[1];
//                y = (signed char)aucRdDataBuf[3];
                z = (signed char)aucRdDataBuf[5];

                if (theft_detected(z, safe_value)) {
                    UART_PRINT("Detected Tampering\n\r");
                    UART_PRINT("Device no longer ARMED\n\r");
                    ARMED = 0;
                    //Connect to AWS with TLS encryption
                    if (!DEBUG_MODE) {
                        lRetVal = tls_connect();
                        if(lRetVal < 0) {
                            ERR_PRINT(lRetVal);
                        }
                        http_post(lRetVal, "Theft detected!!");

                        sl_Close(lRetVal);
                        sl_Stop(SL_STOP_TIMEOUT);
                    }
                }
                MAP_UtilsDelay(2000000);
            }
        }
        MAP_UtilsDelay(800000);
    }
}
//*****************************************************************************
//
// Close the Doxygen group.
//! @}
//
//*****************************************************************************

static int http_post(int iTLSSockID, const char *message){
    char acSendBuff[1024];
    char acRecvbuff[1460];
    char cCLLength[20];
    char dataBuff[384];
    char escapedMessage[MAX_ESCAPED_LEN];
    char* pcBufHeaders;
    int lRetVal = 0;
    int dataLength = 0;

    json_escape(message, escapedMessage, sizeof(escapedMessage));

    sprintf(dataBuff,
            "{"
            "\"state\": {"
            "\"desired\" : {"
            "\"default\" :\"%s\""
            "}"
            "}"
            "}\r\n\r\n",
            escapedMessage);

    dataLength = strlen(dataBuff);

    pcBufHeaders = acSendBuff;
    strcpy(pcBufHeaders, POSTHEADER);
    pcBufHeaders += strlen(POSTHEADER);
    strcpy(pcBufHeaders, HOSTHEADER);
    pcBufHeaders += strlen(HOSTHEADER);
    strcpy(pcBufHeaders, CHEADER);
    pcBufHeaders += strlen(CHEADER);
    strcpy(pcBufHeaders, "\r\n\r\n");


    strcpy(pcBufHeaders, CTHEADER);
    pcBufHeaders += strlen(CTHEADER);
    strcpy(pcBufHeaders, CLHEADER1);

    pcBufHeaders += strlen(CLHEADER1);
    sprintf(cCLLength, "%d", dataLength);

    strcpy(pcBufHeaders, cCLLength);
    pcBufHeaders += strlen(cCLLength);
    strcpy(pcBufHeaders, CLHEADER2);
    pcBufHeaders += strlen(CLHEADER2);

    strcpy(pcBufHeaders, dataBuff);
    pcBufHeaders += strlen(dataBuff);

    //
    // Send the packet to the server */
    //
    lRetVal = sl_Send(iTLSSockID, acSendBuff, strlen(acSendBuff), 0);
    if(lRetVal < 0) {
        UART_PRINT("POST failed. Error Number: %i\n\r",lRetVal);
        sl_Close(iTLSSockID);
        GPIO_IF_LedOn(MCU_RED_LED_GPIO);
        return lRetVal;
    }
    lRetVal = sl_Recv(iTLSSockID, &acRecvbuff[0], sizeof(acRecvbuff), 0);
    if(lRetVal < 0) {
        UART_PRINT("Received failed. Error Number: %i\n\r",lRetVal);
        //sl_Close(iSSLSockID);
        GPIO_IF_LedOn(MCU_RED_LED_GPIO);
           return lRetVal;
    }
    else {
        acRecvbuff[lRetVal+1] = '\0';
        UART_PRINT(acRecvbuff);
        UART_PRINT("\n\r\n\r");
    }

    return 0;
}
