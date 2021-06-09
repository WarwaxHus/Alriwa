//*****************************************************************************
// We used the example of the TIVA TM4C123GXL, FreeRTOS_demo
// So we left the copyright Message of the Program
//*****************************************************************************
//
// switch_task.c - A simple switch task to process the buttons.
//
// Copyright (c) 2012-2017 Texas Instruments Incorporated.  All rights reserved.
// Software License Agreement
// 
//
//*****************************************************************************
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include "inc/tm4c123gh6pm.h"
#include "inc/hw_i2c.h"
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "inc/hw_gpio.h"
#include "driverlib/debug.h"
#include "driverlib/sysctl.h"
#include "driverlib/i2c.h"
#include "driverlib/pin_map.h"
#include "driverlib/sysctl.h"
#include "driverlib/gpio.h"
#include "driverlib/rom.h"
#include "drivers/buttons.h"
#include "utils/uartstdio.h"
#include "switch_task.h"
#include "led_task.h"
#include "priorities.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include "time.h"

#define RS GPIO_PIN_2
#define EN GPIO_PIN_3
#define D4 GPIO_PIN_4
#define D5 GPIO_PIN_5
#define D6 GPIO_PIN_6
#define D7 GPIO_PIN_7
#define AllLCDPins  RS | EN | D7 | D6 | D5 | D4
#define LCDGPIOBASE GPIO_PORTA_BASE

#define Col1 GPIO_PIN_0
#define Col2 GPIO_PIN_1
#define Col3 GPIO_PIN_2
#define Col4 GPIO_PIN_3
#define Row1 GPIO_PIN_0
#define Row2 GPIO_PIN_1
#define Row3 GPIO_PIN_2
#define Row4 GPIO_PIN_3
#define AllROWS  Row1 | Row2 | Row3 | Row4
#define AllCOL  Col1 | Col2 | Col3 | Col4
#define RowGPIOBASE GPIO_PORTB_BASE
#define ColGPIOBASE GPIO_PORTD_BASE

#define I2CSCLPin GPIO_PIN_4
#define I2CSDAPin GPIO_PIN_5
#define I2CGPIOBase GPIO_PORTE_BASE

//*****************************************************************************
// The stack size for the display task.
#define SWITCHTASKSTACKSIZE        128         // Stack size in words


extern xQueueHandle g_pLEDQueue;
extern xSemaphoreHandle g_pUARTSemaphore;



void itoa(int n, char s[]);


//*****************************************************************************
//
// This task reads the buttons' state and passes this information to LEDTask.
//
//*****************************************************************************
static void
SwitchTask(void *pvParameters)
{
    int Seconde=540;

    char tempor[10];
    uint32_t ui8Message;

        // Loop forever.
        //
        while(1)
        {

            SysCtlDelay(10000);
            Seconde++;

            ui8Message=Seconde;

                    if(xQueueSend(g_pLEDQueue, &ui8Message, portMAX_DELAY) !=pdPASS) ////// Pass the value of the Seconds to LEDTask.
                    {
                        while(1)                                                     ////// Infinite Loop of error
                        {
                        }
                    }

        }
    }

//*****************************************************************************
// Initializes the switch task.
uint32_t SwitchTaskInit(void)
{

    if(xTaskCreate(SwitchTask, (const portCHAR *)"Switch",                          ////// Create Switch Task
                   SWITCHTASKSTACKSIZE, NULL, tskIDLE_PRIORITY +
                   PRIORITY_SWITCH_TASK, NULL) != pdTRUE)
    {
        return(1);                                                                 ////// Return 1 if error
    }
    return(0);                                                                     ////// Return 0 if success
}
