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


#define CALCBIT0	1 << 0
#define CALCBIT1	1 << 5
EventGroupHandle_t xPiEventGroup;

extern void vApplicationIdleHook( void );
void vCalcPi(void *pvParameters);
void vInterface(void *pvParameters);

double PiRes = 0;
char PiString[512] = "";

TaskHandle_t ledTask;

void vApplicationIdleHook( void )
{
    
}

int main(void)
{
    resetReason_t reason = getResetReason();

    vInitClock();
    vInitDisplay();
    
    xTaskCreate( vCalcPi, (const char *) "CalcPi", configMINIMAL_STACK_SIZE+300, NULL, 1, &ledTask);
    xTaskCreate(vInterface, (const char *) "interface", configMINIMAL_STACK_SIZE+100, NULL, 3, NULL);

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
        
        sprintf(PiString, "%f", PiRes);
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
        
        double j = 0;
        double termnew = 1;
        double termbuffer = 0;
        
        evBits = xEventGroupGetBits(xPiEventGroup);

        for (;;){
            evBits = xEventGroupGetBits(xPiEventGroup);
            
            if(evBits & CALCBIT0 == CALCBIT0) {   
             int32_t i = 0;
	         for(i=0;i<10;i++){
                if(termnew == 1){
                    j = 0;
                   }else {
                    j = termbuffer;
                }
                termbuffer = termnew;
                termnew = termnew + ((termnew - j) * i)/(i + i + 1);
                PiRes = termnew + termnew;
               // sprintf(PiString, "%f" , termres);
                //vDisplayWriteStringAtPos(3,0,"Pi: %s", PiString);
                PORTF.OUTTGL = 0x01;
            }
            
        }        
        vTaskDelay(100 / portTICK_RATE_MS);
        }            
}