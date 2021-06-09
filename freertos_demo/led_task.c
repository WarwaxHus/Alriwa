//*****************************************************************************
// We used the example of the TIVA TM4C123GXL, FreeRTOS_demo
// So we left the copyright Message of the Program
//
//*****************************************************************************
//
// led_task.c - A simple flashing LED task.
//
// Copyright (c) 2012-2017 Texas Instruments Incorporated.  All rights reserved.
// Software License Agreement
// 
// Texas Instruments (TI) is supplying this software for use solely and
// exclusively on TI's microcontroller products. The software is owned by
// TI and/or its suppliers, and is protected under applicable copyright
// laws. You may not combine this software with "viral" open-source
// software in order to form a larger program.
// 
// THIS SOFTWARE IS PROVIDED "AS IS" AND WITH ALL FAULTS.
// NO WARRANTIES, WHETHER EXPRESS, IMPLIED OR STATUTORY, INCLUDING, BUT
// NOT LIMITED TO, IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE APPLY TO THIS SOFTWARE. TI SHALL NOT, UNDER ANY
// CIRCUMSTANCES, BE LIABLE FOR SPECIAL, INCIDENTAL, OR CONSEQUENTIAL
// DAMAGES, FOR ANY REASON WHATSOEVER.
// 
// This is part of revision 2.1.4.178 of the EK-TM4C123GXL Firmware Package.
//
//*****************************************************************************

// Libraries of the TM4C123GXL used in the project
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "inc/tm4c123gh6pm.h"
#include "inc/hw_i2c.h"
#include "inc/hw_gpio.h"
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "driverlib/gpio.h"
#include "driverlib/rom.h"
#include "driverlib/debug.h"
#include "driverlib/sysctl.h"
#include "driverlib/i2c.h"
#include "driverlib/pin_map.h"
#include "drivers/rgb.h"
#include "drivers/buttons.h"
#include "utils/uartstdio.h"
#include "led_task.h"
#include "priorities.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include "time.h"

//*******************************************************************//
//*******************************************************************//
//////////////      Associate the Pins with a name        /////////////
//*******************************************************************//
//////////////       So it will be easier to write        /////////////
//////////////          in the rest of the Code           /////////////
//////////////    It was practical, because sometimes     /////////////
//////////////     We tried the board of other groups     /////////////
////////////// that's how we knew our TMP101 didn't work  /////////////
//////////////  So, easy to change the pins in our code   /////////////


/////////////////////////LCD Pins
#define RS GPIO_PIN_2
#define EN GPIO_PIN_3
#define D4 GPIO_PIN_4
#define D5 GPIO_PIN_5
#define D6 GPIO_PIN_6
#define D7 GPIO_PIN_7
#define AllLCDPins  RS | EN | D7 | D6 | D5 | D4
#define LCDGPIOBASE GPIO_PORTA_BASE

/////////////////////////Keypad Pins
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

//////////////              END of Renaming               /////////////
//*******************************************************************//

//*******************************************************************//
//////////////            Conversion Function             /////////////
/* -- reverse ---------------------------------------------------------
 * Description  : Function to reverse a Char Array
 *
 * Parameters   : Char Array - S
 * Return       :
 * Notes        : Changes the Array without returning it
 *              : https://en.wikibooks.org/wiki/C_Programming/stdlib.h/itoa
 */
void reverse(char s[])
{
     int i, j;
     char c;
                                                        ////// Loop to increase i and decrease j until j gets lower than i
     for (i = 0, j = strlen(s)-1; i<j; i++, j--) {      ////// i takes the first value of the array and j the last and invert in mirror
         c = s[i];                                      ////// Put the first character in a temporary variable
         s[i] = s[j];                                   ////// put the last character in the first place
         s[j] = c;                                      ////// put the temporary varibale value in the last position
     }
 }

/* -- itoa ------------------------------------------------------------
* Description  : Transforms a given Integer to an Array of Char
* Parameters   : Integer n
*              : Char Array S
* Return       :
* Notes        : Changes the Array without returning it
*              : https://en.wikibooks.org/wiki/C_Programming/stdlib.h/itoa
*/
void itoa(int n, char s[])                             ////// N the integer we need to convert in char
{                                                      ////// S the Char Array we want to put the converted char
     int i, sign;

     if ((sign = n) < 0)                                ////// Put the value n in sign to keep the sign (-/+)
         n = -n;                                        ////// make n positive if it's negative
     i = 0;
     do {                                               ////// convert each digit in character
         s[i++] = n % 10 + '0';                         ////// take the unit value and sum it to the ASCII value of 0
                                                        //////      to have the ASCII of the unit
     } while ((n /= 10) > 0);                           ////// Divide n by 10 to delete the Unit and do it again until n=0
     if (sign < 0)
         s[i++] = '-';                                  ////// put a - if the number was negative
     s[i] = '\0';                                       ////// put \0 at the end of the array to precise the end of the array
     reverse(s);                                        ////// Reverse the Array to get the characters in the good order
     SysCtlDelay(10000);                                //////
}
//////////////        END of conversion function          /////////////
//*******************************************************************//





//*******************************************************************//
////////////////////         I2C Function         /////////////////////
/* -- I2CInit ---------------------------------------------------------
* Description  : Initialize I2C2 of the TM4C123GXL
* Parameters   :
* Return       :
* Notes        : https://www.digikey.com/eewiki/display/microcontroller/I2C+Communication+with+the+TI+Tiva+TM4C123GXL
*
*/


void I2CInit(){
                                                                 //////
     SysCtlPeripheralEnable(SYSCTL_PERIPH_I2C2);                 ////// Enable the I2C2 peripheral
     SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOE);                ////// Enable the port where I2C2 is located Port E

     GPIOPinConfigure(GPIO_PE4_I2C2SCL);                         ////// Configure the pin for I2C2 functions on port E4 and E5.
     GPIOPinConfigure(GPIO_PE5_I2C2SDA);
     GPIOPinTypeI2CSCL(GPIO_PORTE_BASE, GPIO_PIN_4);
     GPIOPinTypeI2C(GPIO_PORTE_BASE, GPIO_PIN_5);

     I2CMasterInitExpClk(I2C2_BASE, SysCtlClockGet(), false);    ////// Initialize Master and Slave
}


///////////////////////////////////////////////////////////////////////
////////////////////          Send to I2C          ////////////////////
/////////////// Used for Lidar sensor, but doesn't work ///////////////
//// Same as in the site, lack of time to understand it and use it ////
/* -- I2CSend ---------------------------------------------------------
* Description  : Sends a register to a particular Slave address of I2C2
* Parameters   : uint8_t slave_addr
*              : uint8_t num_of_args
*              : uint8_t registers
* Return       :
* Notes        : Used for Lidar sensor, but doesn't work
*              : Same as in the site, lack of time to understand it and use it
*              : https://www.digikey.com/eewiki/display/microcontroller/I2C+Communication+with+the+TI+Tiva+TM4C123GXL
*/
/*
void I2CSend(uint8_t slave_addr, uint8_t num_of_args, ...)
{                                                                ////// Tell the master module what address it will place on the bus when
    I2CMasterSlaveAddrSet(I2C2_BASE, slave_addr, false);         ////// communicating with the slave.

    va_list vargs;                                               ////// stores list of variable number of arguments

    va_start(vargs, num_of_args);                                ////// specifies the va_list to "open" and the last fixed argument
                                                                 ////// so vargs knows where to start looking
                                                                 //////
    I2CMasterDataPut(I2C2_BASE, va_arg(vargs, uint32_t));        ////// put data to be sent into FIFO
                                                                 //////
                                                                 //////
    if(num_of_args == 1)                                         //////if there is only one argument, we only need to use the
    {                                                            ////// single send I2C function

        I2CMasterControl(I2C2_BASE, I2C_MASTER_CMD_SINGLE_SEND); //////Initiate send of data from the MCU

        while(I2CMasterBusy(I2C2_BASE));                         ////// Wait until MCU is done transferring.

        va_end(vargs);                                           ////// "close" variable argument list
    }                                                             ////// otherwise, we start transmission of multiple bytes on the I2C bus
    else                                                         //////
    {                                                            //////
        I2CMasterControl(I2C2_BASE, I2C_MASTER_CMD_BURST_SEND_START);// Initiate send of data from the MCU

                                                                 ////// Wait until MCU is done transferring.
        while(I2CMasterBusy(I2C2_BASE));                         //////
                                                                 ////// send num_of_args-2 pieces of data, using the
        uint8_t i;                                               ////// BURST_SEND_CONT command of the I2C module
        for( i = 1; i < (num_of_args - 1); i++)                  //////
        {                                                        //////
                                                                 ////// put next piece of data into I2C FIFO
            I2CMasterDataPut(I2C2_BASE, va_arg(vargs, uint32_t));//////
                                                                 ////// send next data that was just placed into FIFO
            I2CMasterControl(I2C2_BASE, I2C_MASTER_CMD_BURST_SEND_CONT);

            while(I2CMasterBusy(I2C2_BASE));                     ////// Wait until MCU is done transferring.
        }

        I2CMasterDataPut(I2C2_BASE, va_arg(vargs, uint32_t));   ////// put last piece of data into I2C FIFO
                                                                ////// send next data that was just placed into FIFO
        I2CMasterControl(I2C2_BASE, I2C_MASTER_CMD_BURST_SEND_FINISH);

        while(I2CMasterBusy(I2C2_BASE));                        ////// Wait until MCU is done transferring.
        va_end(vargs);                                          ////// "close" variable args list
    }
}
*/


/* -- I2CReceive ------------------------------------------------------
* Description  : Receive Data from an address of I2C2 via a particular register
* Parameters   : Integer - slave_addr
*              : Integer - reg
* Return       : Integer
* Notes        : https://www.digikey.com/eewiki/display/microcontroller/I2C+Communication+with+the+TI+Tiva+TM4C123GXL
*/
int I2CReceive(int slave_addr, int reg) {
    I2CMasterSlaveAddrSet(I2C2_BASE, slave_addr, true);          ////// Set the adress of the Slave and setting in read mode, to send the register

    I2CMasterDataPut(I2C2_BASE, reg);                            ////// Put the register in the Master

    I2CMasterControl(I2C2_BASE, I2C_MASTER_CMD_BURST_SEND_START);////// Send the Register to the Slave

    while(I2CMasterBusy(I2C2_BASE));                             ////// Wait until the end of the transaction

    I2CMasterSlaveAddrSet(I2C2_BASE, slave_addr, true);          ////// Set the address of the Slave for reading from it

    I2CMasterControl(I2C2_BASE, I2C_MASTER_CMD_SINGLE_RECEIVE);  ////// Activate receiving mode of Master

    while(I2CMasterBusy(I2C2_BASE));                             ////// Wait until the end of the transaction

    return I2CMasterDataGet(I2C2_BASE);                          ////// Return Received Data
    }
//////////////            END of I2C function             /////////////
//*******************************************************************//

//*******************************************************************//
//////////////               LCD Functions                /////////////
/* -- pulseLCD ------------------------------------------------------
* Description  : Allow the LCD to receive the bytes
*              : without it, the LCD doesn't react
*              : Turns Off then On then Off the EN pin of the LCD
* Parameters   :
* Return       :
* Notes        : https://mostlyanalog.blogspot.com/2015/07/lcd-display-library-for-tiva-and.html
*/
void pulseLCD()
{
    GPIOPinWrite(LCDGPIOBASE, EN, 0);                               ////// Put EN to 0
    GPIOPinWrite(LCDGPIOBASE, EN, EN);                              ////// Put EN to High
    GPIOPinWrite(LCDGPIOBASE, EN, 0);                               ////// Put EN to 0
}

/* -- initLCD -------------------------------------------------------
* Description  : Initialize the LCD's Pins and prepares it to print something
* Parameters   :
* Return       :
* Notes        : https://mostlyanalog.blogspot.com/2015/07/lcd-display-library-for-tiva-and.html
*/
void initLCD(void)
{
     SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);                   ////// Enable GPIO A where the LCD is connected
     GPIOPinTypeGPIOOutput(LCDGPIOBASE, AllLCDPins);                ////// Set all LCD Pins to Output
     GPIOPinWrite(LCDGPIOBASE, AllLCDPins, 0);                      ////// Put all Pins to Low

     SysCtlDelay(25000);                                            ////// Delay to let the LCD turn On

/////////////////// Function Set : sets the LCD in 4bit Mode and 1 Line
     GPIOPinWrite(LCDGPIOBASE, RS ,0);                              ////// Put RS to 0 because it's a command (already low but just in case)
     GPIOPinWrite(LCDGPIOBASE, D5, D5);                             ////// turn D5 to High
     pulseLCD();                                                    ////// Send the command to the LCD
     GPIOPinWrite(LCDGPIOBASE, D5, D5);                             ////// turn D5 to High (already high, but just in case)
     pulseLCD();                                                    ////// Send the command to the LCD

     SysCtlDelay(115000);                                           ////// Delay to let the LCD react

/////////////////// Turn On LCD (do the same to turn it off)
     GPIOPinWrite(LCDGPIOBASE,RS|D7|D6|D5|D4, 0x0|0x0|0x0|0x0|0x0); ////// Put all Pins to Low
     pulseLCD();                                                    ////// Send the command to the LCD
     GPIOPinWrite(LCDGPIOBASE, D5|D6|D7, D5|D6|D7);                 ////// Put D5 D6 and D7 to High
     pulseLCD();                                                    ////// Send the command to the LCD
     SysCtlDelay(10000);                                            ////// Delay to let the LCD react
}

/* -- MoveCursor ------------------------------------------------------
* Description  : Moves the cursor to the next case only if the actual case is used
* Parameters   :
* Return       :
* Notes        : https://mostlyanalog.blogspot.com/2015/07/lcd-display-library-for-tiva-and.html
*/
void MoveCursor(){
    GPIOPinWrite(LCDGPIOBASE, RS|D7|D6|D5|D4, 0x0|0x0|0x0|0x0|0x0); ////// Put all Pins to Low
    pulseLCD();                                                     ////// Send the command to the LCD
    GPIOPinWrite(LCDGPIOBASE, D6|D5, D6|D5 );                       ////// Put D6 and D5 to High
    pulseLCD();                                                     ////// Send the command to the LCD
    SysCtlDelay(10000);                                             ////// Delay to let the LCD react
}

/* -- PrintASCII ------------------------------------------------------
* Description  : Sub function to Print one character corresponding to the ASCII value put in parameter
* Parameters   : Integer - n
* Return       :
* Notes        : https://mostlyanalog.blogspot.com/2015/07/lcd-display-library-for-tiva-and.html
*/
void PrintASCII(int n)
{
    MoveCursor();                                                    ////// Move the cursor to not print on the previous character
    int binaryNum[8];                                                ////// Integer Array to store the binary value of the ASCII

    int i = 0;
    while (n > 0) {                                                  ////// Loop until N gets to 0
        binaryNum[i] = n % 2;                                        ////// Convert N into Binary and store it BinaryNum (it will be backwards, no need to reverse)
        n = n / 2;
        i++;
    }
    while (i<8){                                                     ////// Fills binaryNum with 0s to have 8 bits
        binaryNum[i] = 0;
        i++;
    }

///////////// 4 Highest Bit of the Character to print
    GPIOPinWrite(LCDGPIOBASE, RS|D7|D6|D5|D4, 0x0|0x0|0x0|0x0);      ////// Put all Pins to Low
    GPIOPinWrite(LCDGPIOBASE, RS, RS);                               ////// Turns RS to High because it's a Data (Print something on the LCD)

    if(binaryNum[7])                                                 ////// verify if 8th bit (Highest) is 0 or 1
            GPIOPinWrite(LCDGPIOBASE, D7, D7);                       ////// If it's 1 turns D7 to High, else leave it Low
    if(binaryNum[6])///D6                                            ////// verify if 7th bit is 0 or 1
            GPIOPinWrite(LCDGPIOBASE, D6, D6);                       ////// If it's 1 turns D6 to High, else leave it Low
    if(binaryNum[5])///D5                                            ////// verify if 6th bit is 0 or 1
            GPIOPinWrite(LCDGPIOBASE, D5, D5);                       ////// If it's 1 turns D5 to High, else leave it Low
    if(binaryNum[4])///D4                                            ////// verify if 5th bit is 0 or 1
            GPIOPinWrite(LCDGPIOBASE, D4, D4);                       ////// If it's 1 turns D4 to High, else leave it Low
                                                                     //////
    pulseLCD();                                                      ////// Send the Data to the LCD
                                                                     //////
///////////// 4 Lowest Bit of the Character to print
    GPIOPinWrite(LCDGPIOBASE, RS|D7|D6|D5|D4, 0x0|0x0|0x0|0x0);      ////// Put all Pins to Low
    GPIOPinWrite(LCDGPIOBASE, RS, RS);                               ////// Turns RS to High because it's a Data (Print something on the LCD)
                                                                     //////
    if(binaryNum[3])///D7                                            ////// verify if 4th bit is 0 or 1
            GPIOPinWrite(LCDGPIOBASE, D7, D7);                       ////// If it's 1 turns D7 to High, else leave it Low
    if(binaryNum[2])///D6                                            ////// verify if 3rd bit is 0 or 1
            GPIOPinWrite(LCDGPIOBASE, D6, D6);                       ////// If it's 1 turns D6 to High, else leave it Low
    if(binaryNum[1])///D5                                            ////// verify if 2nd bit is 0 or 1
            GPIOPinWrite(LCDGPIOBASE, D5, D5);                       ////// If it's 1 turns D5 to High, else leave it Low
    if(binaryNum[0])///D4                                            ////// verify if 1st bit (Lowest) is 0 or 1
            GPIOPinWrite(LCDGPIOBASE, D4, D4);                       ////// If it's 1 turns D4 to High, else leave it Low

    pulseLCD();                                                      ////// Send the Data to the LCD
}

/* -- PrintLCD ---------------------------------------------------------
* Description  : Main function to print an Array of character
*              : Separate each character of the array and sends it to PrintASCII
* Parameters   : Char Array - Yo
* Return       :
* Notes        : https://mostlyanalog.blogspot.com/2015/07/lcd-display-library-for-tiva-and.html
*/
void PrintLCD (char Yo[50])
{
    int c=0;
    char x;
    int y=1;                                                                   ////// Y will take the integer of a character which will be the ASCII value
                                                                               ////// Initialized at 1 to enter in the the loop
    while(y!=0){                                                               ////// If Y == 0 it's the end of the Char Array
        x=Yo[c];                                                               ////// takes one by one the characters of the array
        y=x;                                                                   ////// converts the character to Int (ASCII Value)
        if (y==32){                                                            ////// if Y==32 it's a Space, manually put the Space
                                                                               ////// because it has some problem. ASCII of Space : 0010 0000
            MoveCursor();                                                      ////// Move the cursor
            GPIOPinWrite(LCDGPIOBASE, RS|D7|D6|D5|D4, 0x0|0x0|0x0|0x0|0x0);    ////// Puts all Pins to Low
            GPIOPinWrite(LCDGPIOBASE, RS|D5, RS|D5);                           ////// Turns on RS because it's a Data and R5
            pulseLCD();                                                        ////// Send the Data to the LCD
            GPIOPinWrite(LCDGPIOBASE, RS|D7|D6|D5|D4, 0x0|0x0|0x0|0x0|0x0);    ////// Puts all Pins to Low
            GPIOPinWrite(LCDGPIOBASE, RS, RS);                                 ////// Turns on RS because it's a Data
            pulseLCD();                                                        ////// Send the Data to the LCD
        }
        else if(y!=0)                                                          ////// If it's different from 0 and 32
            PrintASCII(y);                                                     ////// Launch the PrintASCII function above
        c++;
    }
}

/* -- ReturnHome ---------------------------------------------------------
* Description  : Puts the cursor in the first position
* Parameters   :
* Return       :
* Notes        : https://mostlyanalog.blogspot.com/2015/07/lcd-display-library-for-tiva-and.html
*/
void ReturnHome(){
    GPIOPinWrite(LCDGPIOBASE, RS|D7|D6|D5|D4, 0x0|0x0|0x0|0x0|0x0);            ////// Puts all Pins to Low
    pulseLCD();                                                                ////// Send the Command to the LCD
    GPIOPinWrite(LCDGPIOBASE, D5, D5);                                         ////// Turns D5 to High
    pulseLCD();                                                                ////// Send the Command to the LCD
}

/* -- ClearLCD ---------------------------------------------------------
* Description  : Clear the LCD
*
* Parameters   :
* Return       :
* Notes        : https://mostlyanalog.blogspot.com/2015/07/lcd-display-library-for-tiva-and.html
*/
void ClearLCD(){
    SysCtlDelay(16000);                                                        ////// Delay to let the System stabilize
    GPIOPinWrite(LCDGPIOBASE, RS|D7|D6|D5|D4, 0x0|0x0|0x0|0x0|0x0);            ////// Puts all pins to Low
    pulseLCD();                                                                ////// Send Command to LCD
    GPIOPinWrite(LCDGPIOBASE, D4, D4);                                         ////// Turns D4 to High
    pulseLCD();                                                                ////// Send Command to LCD
    SysCtlDelay(16000);                                                        ////// Delay to Let the LCD clean the LCD
}


/* -- ShiftLCursor ------------------------------------------------------
* Description  : Move Cursor to the Left
* Parameters   :
* Return       :
* Notes        : https://mostlyanalog.blogspot.com/2015/07/lcd-display-library-for-tiva-and.html
*              : Command : 0001 0000
*/
void ShiftLCursor(){
    SysCtlDelay(10000);
    GPIOPinWrite(LCDGPIOBASE, RS|D7|D6|D5|D4, 0x0|0x0|0x0|0x0|D4);
    pulseLCD();
    GPIOPinWrite(LCDGPIOBASE, D4, 0 );
    pulseLCD();
    SysCtlDelay(10000);
}

/////////             Move Cursor to the Right                /////////
/* -- ShiftRCursor ------------------------------------------------------
* Description  : Move Cursor to the Right even if there isn't data in the actual case
* Parameters   :
* Return       :
* Notes        : https://mostlyanalog.blogspot.com/2015/07/lcd-display-library-for-tiva-and.html
*              : Command : 0001 0111
*/
void ShiftRCursor(){
    SysCtlDelay(10000);
    GPIOPinWrite(LCDGPIOBASE, RS|D7|D6|D5|D4, 0x0|0x0|0x0|0x0|D4);
    pulseLCD();
    GPIOPinWrite(LCDGPIOBASE, D6|D5|D4, D6|D5|D4 );
    pulseLCD();
    SysCtlDelay(10000);
}
//////////////            END of LCD function             /////////////
//*******************************************************************//





//*******************************************************************//
//////////////            Temperature function            /////////////
/* -- showTTemp -------------------------------------------------------
* Description  : Search for the address of the temperature sensor
*              : Show the address and the temperature on the LCD
* Parameters   :
* Return       :
* Notes        :
*/
void showTTemp(){
    ClearLCD();                                                  ////// Clear the LCD
    PrintLCD("Ad : ");                                           ////// just Print "Ad :" for address
    float tempe=0;
    char str[12];
    char stror[12];
    int t=0;
    tempe=I2CReceive(t,0);                                       ////// tempe takes the value that the address 0 gives us from the I2C
    while (tempe==0 | tempe==255){                               ////// if Tempe = 0 it's an error, and 255 means nothing at this address
        t++;                                                     ////// so we search for every until it finds something
        tempe=I2CReceive(t,0);                                   ////// it can be infinite if the sensor is broken
    }
    itoa(t,str);                                                 ////// Convert T (correct address) to a Char Array
    PrintLCD(str);                                               ////// Print it
    PrintLCD(" : Val : ");                                       ////// Print "END"
    itoa(tempe,stror);                                           ////// Convert the Integer received from the I2C at the address t
    PrintLCD(stror);                                             ////// Print it
}


/* -- showTemp --------------------------------------------------------
* Description  : Show the temperature on the LCD
* Parameters   :
* Return       :
* Notes        :
*/
void showTemp(){
    ClearLCD();                                                   ////// Clear LCD
    float tempe=0;
    char str[12];
    tempe=I2CReceive(72,0);                                       ////// 72 is the address we found at the end
    if (tempe!=0){PrintLCD("temp : ");}                           ////// print "Temp :" if different from 0
    itoa(tempe,str);                                              ////// Convert Temp to char
    PrintLCD(str);                                                ////// Print it
    SysCtlDelay(1000000);                                         ////// Big Delay for LCD to print well while in loop
}
//////////////       End of Temperature function          /////////////
//*******************************************************************//

//*******************************************************************//
//////////////            Distance function               /////////////
/* -- showDistance ----------------------------------------------------
* Description  : Search for the correct Sending and Receiving address for the Distance sensor
* Parameters   :
* Return       :
* Notes        : Register 0 for sending and Register 1 for Receive. Didn't Work. (Datasheet : 0x52 Write, 0x53 Read)
*/
/*
void showDist(){

    ClearLCD();
    float dist=0;
    PrintLCD("S:");

    int r;
    int s;
    for (s=0;s<256;s++){
        for (r=0;r<256;r++){

                I2CSend(s,1,0x0);
                dist=I2CReceive(r,0x1);
                if(dist!=255 & dist!=0){
                    ClearLCD();
                    PrintLCD("S:");
                    char STR[12];
                    itoa(s,STR);
                    PrintLCD(STR);

                    PrintLCD(" /R:");
                    char RTR[12];
                    itoa(r,RTR);
                    PrintLCD(RTR);

                    PrintLCD(" /D:");
                    char disttr[12];
                    itoa(dist,disttr);
                    PrintLCD(disttr);
                    SysCtlDelay(5000000);
                }
        }
    }
    ClearLCD();
    PrintLCD("Search End");
}
*/
//////////////         End Distance function              /////////////
//*******************************************************************//


//*******************************************************************//
//////////////            Keypad Function                 /////////////
/* -- initKPD ---------------------------------------------------------
* Description  : Initialize Keypad Pins : Row as Output and Col as Inputs
*              : and Put all Rows at High
* Parameters   :
* Return       :
* Notes        : All pins in GPIO B had errors, seemed like Input pins (Cols) needed Analog Pins
*/
void initKPD(void)
{
     SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOB);                ////// Enable GPIO B for Rows
     SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOD);                ////// Enable GPIO D for Columns

     GPIOPinTypeGPIOOutput(RowGPIOBASE,  AllROWS);               ////// Set Row Pins to Output
     GPIOPinTypeGPIOInput(ColGPIOBASE,  AllCOL);                 ////// Set Col Pins to Input

     GPIOPinWrite(RowGPIOBASE, AllROWS, AllROWS );               ////// Set All Rows to High
}

/* -- ScanKPD ---------------------------------------------------------
* Description  : Scan the Keypad
*              : Read which Key is pressed
* Parameters   :
* Return       : Integer
* Notes        : Comments just on the first column,
*              : Same for the others
*/
int ScanKPD(){
    uint32_t value1=GPIOPinRead(ColGPIOBASE, Col1 );             ////// Read the value of Col1
    uint32_t value2=GPIOPinRead(ColGPIOBASE, Col2 );             ////// Read the value of Col2
    uint32_t value3=GPIOPinRead(ColGPIOBASE, Col3 );             ////// Read the value of Col3
    uint32_t value4=GPIOPinRead(ColGPIOBASE, Col4 );             ////// Read the value of Col4

    if (value1!=0){
        SysCtlDelay(10000);                                      ////// Delay to Avoid repetition

        GPIOPinWrite(RowGPIOBASE, Row1, 0x0 );                   ////// Put Row1 to 0
        value1=GPIOPinRead(ColGPIOBASE, Col1 );                  ////// Verify if Col1 changed value if yes the pressed button is Col1 Row1
        if (value1==0){
            GPIOPinWrite(RowGPIOBASE, AllROWS, AllROWS );        ////// Turns every Rows on
            value1=GPIOPinRead(ColGPIOBASE, Col1 );
            while(value1!=0) value1=GPIOPinRead(ColGPIOBASE, Col1 );/// Verify if the button is still pressed
            return 1;                                            ////// Return 1 only if button released
            }
        else{
            GPIOPinWrite(RowGPIOBASE, Row2, 0x0 );               ////// Verify if Col1 changed value if yes the pressed button is Col1 Row2
            value1=GPIOPinRead(ColGPIOBASE, Col1 );
            if (value1==0){
                GPIOPinWrite(RowGPIOBASE, AllROWS, AllROWS );    ////// Turns every Rows on
                value1=GPIOPinRead(ColGPIOBASE, Col1 );
                while(value1!=0) value1=GPIOPinRead(ColGPIOBASE, Col1 );/// Verify if the button is still pressed
                return 4;                                       ////// Return 4 only if button released
            }
            else{
                GPIOPinWrite(RowGPIOBASE, Row3, 0x0 );          ////// Verify if Col1 changed value if yes the pressed button is Col1 Row3
                value1=GPIOPinRead(ColGPIOBASE, Col1 );
                if (value1==0){
                    GPIOPinWrite(RowGPIOBASE, AllROWS, AllROWS );///// Turns every Rows on
                    value1=GPIOPinRead(ColGPIOBASE, Col1 );
                    while(value1!=0) value1=GPIOPinRead(ColGPIOBASE, Col1 );/// Verify if the button is still pressed
                    return 7;                                   ////// Return 7 only if button released
                }
                else{
                    GPIOPinWrite(RowGPIOBASE, Row4, 0x0 );      ////// Verify if Col1 changed value if yes the pressed button is Col1 Row4
                    value1=GPIOPinRead(ColGPIOBASE, Col1 );
                    if (value1==0){
                        GPIOPinWrite(RowGPIOBASE, AllROWS, AllROWS );//// Turns every Rows on
                        value1=GPIOPinRead(ColGPIOBASE, Col1 );
                        while(value1!=0) value1=GPIOPinRead(ColGPIOBASE, Col1 );/// Verify if the button is still pressed
                        return 10;                            ////// Return 10 only if button released
                    }
                    else{
                        PrintLCD("Epic Fail");
                    }}}}}
//////////////////////////// It's the same for the 3 other Columns
    else if (value2!=0){
        SysCtlDelay(10000);

        GPIOPinWrite(RowGPIOBASE, Row1, 0x0 );
        value2=GPIOPinRead(ColGPIOBASE, Col2 );
        if (value2==0){
            GPIOPinWrite(RowGPIOBASE, AllROWS, AllROWS );
            value2=GPIOPinRead(ColGPIOBASE, Col2 );
            while(value2!=0) value2=GPIOPinRead(ColGPIOBASE, Col2 );
            return 2;
            }
        else{
            GPIOPinWrite(RowGPIOBASE, Row2, 0x0 );
            value2=GPIOPinRead(ColGPIOBASE, Col2 );
            if (value2==0){
                GPIOPinWrite(RowGPIOBASE, AllROWS, AllROWS );
                value2=GPIOPinRead(ColGPIOBASE, Col2 );
                while(value2!=0) value2=GPIOPinRead(ColGPIOBASE, Col2 );
                return 5;
            }
            else{
                GPIOPinWrite(RowGPIOBASE, Row3, 0x0 );
                value2=GPIOPinRead(ColGPIOBASE, Col2 );
                if (value2==0){
                    GPIOPinWrite(RowGPIOBASE, AllROWS, AllROWS );
                    value2=GPIOPinRead(ColGPIOBASE, Col2 );
                    while(value2!=0) value2=GPIOPinRead(ColGPIOBASE, Col2 );
                    return 8;
                }
                else{
                    GPIOPinWrite(RowGPIOBASE, Row4, 0x0 );
                    value2=GPIOPinRead(ColGPIOBASE, Col2 );
                    if (value2==0){
                        GPIOPinWrite(RowGPIOBASE, AllROWS, AllROWS );
                        value2=GPIOPinRead(ColGPIOBASE, Col2 );
                        while(value2!=0) value2=GPIOPinRead(ColGPIOBASE, Col2 );
                        return 0;
                    }
                    else{
                        PrintLCD("Epic Fail");
                    }}}}}

    else if (value3!=0){
        SysCtlDelay(10000);

        GPIOPinWrite(RowGPIOBASE, Row1, 0x0 );
        value3=GPIOPinRead(ColGPIOBASE, Col3 );
        if (value3==0){
            GPIOPinWrite(RowGPIOBASE, AllROWS, AllROWS );
            value3=GPIOPinRead(ColGPIOBASE, Col3 );
            while(value3!=0) value3=GPIOPinRead(ColGPIOBASE, Col3 );
            return 3;
            }
        else{
            GPIOPinWrite(RowGPIOBASE, Row2, 0x0 );
            value3=GPIOPinRead(ColGPIOBASE, Col3 );
            if (value3==0){
                GPIOPinWrite(RowGPIOBASE, AllROWS, AllROWS );
                value3=GPIOPinRead(ColGPIOBASE, Col3 );
                while(value3!=0) value3=GPIOPinRead(ColGPIOBASE, Col3 );
                return 6;
            }
            else{
                GPIOPinWrite(RowGPIOBASE, Row3, 0x0 );
                value3=GPIOPinRead(ColGPIOBASE, Col3 );
                if (value3==0){
                    GPIOPinWrite(RowGPIOBASE, AllROWS, AllROWS );
                    value3=GPIOPinRead(ColGPIOBASE, Col3 );
                    while(value3!=0) value3=GPIOPinRead(ColGPIOBASE, Col3 );
                    return 9;
                }
                else{
                    GPIOPinWrite(RowGPIOBASE, Row4, 0x0 );
                    value3=GPIOPinRead(ColGPIOBASE, Col3 );
                    if (value3==0){
                        GPIOPinWrite(RowGPIOBASE, AllROWS, AllROWS );
                        value3=GPIOPinRead(ColGPIOBASE, Col3 );
                        while(value3!=0) value3=GPIOPinRead(ColGPIOBASE, Col3 );
                        return 11;
                    }
                    else{
                        PrintLCD("Epic Fail");
                    }}}}}

    else if (value4!=0){
            SysCtlDelay(10000);
            GPIOPinWrite(RowGPIOBASE, Row1, 0x0 );
            value4=GPIOPinRead(ColGPIOBASE, Col4 );
            if (value4==0){
                GPIOPinWrite(RowGPIOBASE, AllROWS, AllROWS );
                value4=GPIOPinRead(ColGPIOBASE, Col4 );
                while(value4!=0) value4=GPIOPinRead(ColGPIOBASE, Col4 );
                return 15;
                }
            else{
                GPIOPinWrite(RowGPIOBASE, Row2, 0x0 );
                value4=GPIOPinRead(ColGPIOBASE, Col4 );
                if (value4==0){
                    GPIOPinWrite(RowGPIOBASE, AllROWS, AllROWS );
                    value4=GPIOPinRead(ColGPIOBASE, Col4 );
                    while(value4!=0) value4=GPIOPinRead(ColGPIOBASE, Col4 );
                    return 14;
                }
                else{
                    GPIOPinWrite(RowGPIOBASE, Row3, 0x0 );
                    value4=GPIOPinRead(ColGPIOBASE, Col4 );
                    if (value4==0){
                        GPIOPinWrite(RowGPIOBASE, AllROWS, AllROWS );
                        value4=GPIOPinRead(ColGPIOBASE, Col4 );
                        while(value4!=0) value4=GPIOPinRead(ColGPIOBASE, Col4 );
                        return 13;
                    }
                    else{
                        GPIOPinWrite(RowGPIOBASE, Row4, 0x0 );
                        value4=GPIOPinRead(ColGPIOBASE, Col4 );
                        if (value4==0){
                            GPIOPinWrite(RowGPIOBASE, AllROWS, AllROWS );
                            value4=GPIOPinRead(ColGPIOBASE, Col4 );
                            while(value4!=0) value4=GPIOPinRead(ColGPIOBASE, Col4 );
                            return 12;
                        }
                        else{
                            PrintLCD("Epic Fail");
                        }}}}}
    return 90;
}
//////////////          End Keypad Function               /////////////
//*******************************************************************//

//*******************************************************************//
//////////////           Set Date and Time                /////////////
/* -- SetTime ---------------------------------------------------------
* Description  : Set the time via the Keypad
* Parameters   : char Array - Time
* Return       :
* Notes        : Modifies the value of Time without returning it
*/
void SetTime(char Time[10]){
    ClearLCD();                                                  ////// Clear LCD
    PrintLCD(Time);                                              ////// Print the actual Time (default 00:00 00)
    SysCtlDelay(10000);
    ReturnHome();                                                ////// Put the Cursor at the beginning of the LCD
    int block=1;                                                 ////// variable to stay in the loop until it's over
    int position=0;                                              ////// get the position we are changing
    int Recu;

    char Sale[10]="0123456789";                                  ////// Char Array with Numbers 0-9
    while (block){
        Recu=ScanKPD();                                          ////// gets the value pressed from keyboard
        if (position==2 | position==5 ){                         ////// position 2 and 5 are the : so should jump when at this place
            ShiftRCursor();                                      ////// Shift Cursor to the right
            position++;                                          ////// Position increased
        }
        if (position==8){                                        ////// Position 8 is the end
            Recu=15;                                             ////// force the press of F (15)
        }
        switch (Recu){                                           ////// Modify the values of the Array Time with the key pressed
            case 0 : PrintLCD("0");                              ////// and increase the position
                            Time[position]=Sale[0];
                            position++;
                               break;
            case 1 : PrintLCD("1");
                            Time[position]=Sale[1];
                            position++;
                               break;
            case 2 : PrintLCD("2");
                            Time[position]=Sale[2];
                            position++;
                               break;
            case 3 : PrintLCD("3");
                            Time[position]=Sale[3];
                            position++;
                               break;
            case 4 : PrintLCD("4");
                            Time[position]=Sale[4];
                            position++;
                               break;
            case 5 : PrintLCD("5");
                            Time[position]=Sale[5];
                            position++;
                               break;
            case 6 : PrintLCD("6");
                            Time[position]=Sale[6];
                            position++;
                               break;
            case 7 : PrintLCD("7");
                            Time[position]=Sale[7];
                            position++;
                               break;
            case 8 : PrintLCD("8");
                            Time[position]=Sale[8];
                            position++;
                               break;
            case 9 : PrintLCD("9");
                            Time[position]=Sale[9];
                            position++;
                               break;
            case 13: ShiftRCursor();                             ////// Press D to modify Next value (Direio)
                            position++;                          ////// Increase Position
                               break;
            case 14: ShiftLCursor();                             ////// Press E to modify Previous value (Esquerda)
                            position=position-1;                 ////// Decrease Position
                               break;
            case 15: block=0;                                    ////// Press F to validate the time
                          ClearLCD();
                          PrintLCD("Time : ");                   ////// Say the time is the one you entered
                          PrintLCD(Time);
                        break;
        }
    }

}

/* -- SetDate ----------------------------------------------------------
* Description  : Set the date via the Keypad
* Parameters   : char Array - Date
* Return       :
* Notes        : Modifies the value of Date without returning it
*              : Works the same way of SetTime
*/
void SetDate(char Date[12]){
    ClearLCD();                                                  ////// Clear LCD
    PrintLCD(Date);                                              ////// Print the actual Date (default "00/00/0000")
    SysCtlDelay(10000);
    ReturnHome();
    int block=1;
    int position=0;
    int Recu;

    char Sale[10]="0123456789";
    while (block){
        Recu=ScanKPD();
        if (position==2 | position==5 ){
            ShiftRCursor();
            position++;
        }
        if (position==10){
            Recu=15;
        }
        switch (Recu){
            case 0 : PrintLCD("0");
                     Date[position]=Sale[0];
                     position++;
                     break;
            case 1 : PrintLCD("1");
                     Date[position]=Sale[1];
                     position++;
                     break;
            case 2 : PrintLCD("2");
                     Date[position]=Sale[2];
                     position++;
                     break;
            case 3 : PrintLCD("3");
                     Date[position]=Sale[3];
                     position++;
                     break;
            case 4 : PrintLCD("4");
                     Date[position]=Sale[4];
                     position++;
                     break;
            case 5 : PrintLCD("5");
                     Date[position]=Sale[5];
                     position++;
                     break;
            case 6 : PrintLCD("6");
                     Date[position]=Sale[6];
                     position++;
                     break;
            case 7 : PrintLCD("7");
                     Date[position]=Sale[7];
                     position++;
                     break;
            case 8 : PrintLCD("8");
                     Date[position]=Sale[8];
                     position++;
                     break;
            case 9 : PrintLCD("9");
                     Date[position]=Sale[9];
                     position++;
                     break;
            case 13: ShiftLCursor();
                     position++;
                     break;
            case 14: ShiftRCursor();
                     position=position-1;
                     break;
            case 15: block=0;
                     ClearLCD();
                     PrintLCD("Date : ");
                     PrintLCD(Date);
                     break;
        }
    }

}
//////////////           End Date and Time                /////////////
//*******************************************************************//


//*******************************************************************//
//////////////             FreeRTOS Value                 /////////////
///////////////////////////////////////////////////////////////////////
//////////////  The stack size for the LED toggle task    /////////////
#define LEDTASKSTACKSIZE        128         // Stack size in words

/////// The item size and queue size for the LED message queue ////////
#define LED_ITEM_SIZE           sizeof(uint8_t)
#define LED_QUEUE_SIZE          5

////////  The queue that holds messages sent to the LED task  /////////
xQueueHandle g_pLEDQueue;


int i8Message;

/* -- LEDTask -----------------------------------------------------------------
* Description  : Task function used for our program to run
* Parameters   : void - *pvParameters
* Return       :
* Notes        : About the same of Freertos Example of the TM4C123GXL
*/
static void LEDTask(void *pvParameters)
{

    portTickType ui16LastTime;
    uint32_t ui32SwitchDelay = 25;
    //uint32_t ui8Message;

    ui16LastTime = xTaskGetTickCount();

    char Date[12]="00/00/0000";
    char Time[10]="00:00 00";

    int Recu;
    int ytr=0

    while(1){

        if (ytr==1){
            showTemp();
        }

        Recu=ScanKPD();
        switch (Recu){
            case 0 : PrintLCD("0");ytr=0;break;
            case 1 : PrintLCD("1");ytr=0;break;
            case 2 : PrintLCD("2");ytr=0;break;
            case 3 : PrintLCD("3");ytr=0;break;
            case 4 : PrintLCD("4");ytr=0;break;
            case 5 : PrintLCD("5");ytr=0;break;
            case 6 : PrintLCD("6");ytr=0;break;
            case 7 : PrintLCD("7");ytr=0;break;
            case 8 : PrintLCD("8");ytr=0;break;
            case 9 : PrintLCD("9");ytr=0;break;
            case 10: SetTime(Time);ytr=0;break;
            case 11: SetDate(Date);ytr=0;break;
            case 12: showTemp();ytr=1;break;
            case 13: ShiftRCursor();ytr=0;break;
            case 14: ShiftLCursor();ytr=0;break;
            case 15: ClearLCD();ytr=0;break;

                    }

        //if(xQueueReceive(g_pLEDQueue, &ui8Message, 0) == pdPASS)
        //{
        //    ScanKPD(ui8Message);
        //}


        //
        // Wait for the required amount of time to check back.
        //
        vTaskDelayUntil(&ui16LastTime, ui32SwitchDelay / portTICK_RATE_MS);

        }
}

/* -- LEDTaskInit -------------------------------------------------------------
* Description  : Initializes the LED task, task used for our program to run
*              : launched in freertos_demo
* Parameters   :
* Return       :
* Notes        : About the same of Freertos Example of the TM4C123GXL
*/

uint32_t LEDTaskInit(void)
{
    initLCD();

    ClearLCD();
    initKPD();
    I2CInit();

    g_pLEDQueue = xQueueCreate(LED_QUEUE_SIZE, LED_ITEM_SIZE);                ////// Create a queue for sending messages to the LED task.

    if(xTaskCreate(LEDTask, (const portCHAR *)"LED", LEDTASKSTACKSIZE, NULL,  ////// Create the LED task.
                   tskIDLE_PRIORITY + PRIORITY_LED_TASK, NULL) != pdTRUE)
    {
        return(1);                                                            ////// If there's an error sends 1
    }

    return(0);                                                               ////// No Problem
}
