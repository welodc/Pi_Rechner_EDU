/*
 * EventBit.c
 *
 * Created: 20.03.2018 18:32:07
 * Author : chaos
 */ 

//#include <avr/io.h>
#include "stdio.h"
#include "stdlib.h"
#include "stdbool.h"
#include "math.h"

#include "avr_compiler.h"
#include "pmic_driver.h"
#include "TC_driver.h"
#include "clksys_driver.h"
#include "sleepConfig.h"
#include "port_driver.h"

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "event_groups.h"
#include "stack_macros.h"

#include "mem_check.h"

#include "init.h"
#include "utils.h"
#include "errorHandler.h"
#include "NHD0420Driver.h"

#include "ButtonHandler.h"
#include "avr_f64.h"


#define CALCBIT0	1 << 0
#define CALCBIT1	1 << 5
EventGroupHandle_t xPiEventGroup;

extern void vApplicationIdleHook( void );
void vCalcPi(void *pvParameters);
void vInterface(void *pvParameters);

char PiString[22] = "";

TaskHandle_t ledTask;

void vApplicationIdleHook( void )
{
    
}

int main(void)
{
    resetReason_t reason = getResetReason();

    vInitClock();
    vInitDisplay();
    
    xTaskCreate( vCalcPi, (const char *) "CalcPi", configMINIMAL_STACK_SIZE+500, NULL, 1, &ledTask);
    xTaskCreate(vInterface, (const char *) "interface", configMINIMAL_STACK_SIZE+300, NULL, 3, NULL);

    vDisplayClear();
    vDisplayWriteStringAtPos(0,0,"Pi Rechner");
    vDisplayWriteStringAtPos(1,0,"Zeit :");
    vDisplayWriteStringAtPos(2,0,"Pi:");
    vTaskStartScheduler();
    return 0;
}

void vInterface(void *pvParameters) {
    int timecounter = 0;
    xPiEventGroup = xEventGroupCreate();
    initButtons();
    for(;;) {
        
        
        vDisplayWriteStringAtPos(3,0,"%s", PiString);
        
        
        
        updateButtons();
        timecounter++;
        if(getButtonPress(BUTTON1) == SHORT_PRESSED) {
            xEventGroupSetBits(xPiEventGroup, CALCBIT0 );
            
        }
        if(getButtonPress(BUTTON2) == SHORT_PRESSED) {
            xEventGroupClearBits(xPiEventGroup, CALCBIT0);
            
        }
        if(getButtonPress(BUTTON3) == SHORT_PRESSED) {
            
        }
        if(getButtonPress(BUTTON4) == SHORT_PRESSED) {
            
        }
        if(getButtonPress(BUTTON1) == LONG_PRESSED) {
            
        }
        if(getButtonPress(BUTTON2) == LONG_PRESSED) {
            
        }
        if(getButtonPress(BUTTON3) == LONG_PRESSED) {
            
        }
        if(getButtonPress(BUTTON4) == LONG_PRESSED) {
            
        }

        if(timecounter >= 50) {
            //Set Display
            timecounter = 0;
        }
        vTaskDelay(10/portTICK_RATE_MS);
    }
}

void vCalcPi(void *pvParameters) {
    (void) pvParameters;
    uint32_t evBits = 0x00;
    while( xPiEventGroup == NULL ) {
        vTaskDelay(100/portTICK_RATE_MS);
    };
        PORTF.DIRSET = PIN0_bm; /*LED1*/
        PORTF.OUT = 0x01;
        
        float64_t OldPiVal = f_sd(0);                     // Variable f�r den den Wert vor 2 Berechnungen
        float64_t NewPiVal = f_sd(1);                     // Variable f�r den neuen Wert 
        float64_t PiBuffer = f_sd(0);                     // Buffer f�r den vorherigen Wert
        float64_t PiCalcVal = f_sd(0);                    // Variable Z�hler
        float64_t PiRes = f_sd(0);
           
        
        evBits = xEventGroupGetBits(xPiEventGroup);

        for (;;){
            evBits = xEventGroupGetBits(xPiEventGroup);
            
            if(evBits & CALCBIT0 == CALCBIT0) {   
             int32_t i = 0;
	         for(i=0;;i++){
                if(NewPiVal == f_sd(1))
                    {
                    OldPiVal = f_sd(0);
                    }
                   else
                    {
                    OldPiVal = PiBuffer;
                    }
                // Neuer Wert wird  dem Buffer �bergeben    
                PiBuffer = NewPiVal;
                
                //NewPiVal = NewPiVal + ((NewPiVal - OldPiVal) * i)/(i + i + 1);
                PiCalcVal = f_sub(NewPiVal,OldPiVal);
                PiCalcVal = f_mult(PiCalcVal,f_sd(i));
                PiCalcVal = f_div(PiCalcVal,f_sd(i+i+1));
                NewPiVal = f_add(NewPiVal,PiCalcVal);
                
                PiRes = f_add(NewPiVal,NewPiVal);
                
                char* tempResultString = f_to_string(PiRes, 16, 16);
                sprintf(PiString, "%s", tempResultString);
                //PORTF.OUTTGL = 0x01;
            }
            
        }        
        //vTaskDelay(100 / portTICK_RATE_MS);
        }            
}