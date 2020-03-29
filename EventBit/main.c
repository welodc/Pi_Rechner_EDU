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
#define CALCBIT1	1 << 3
#define CALCBIT2	1 << 5
#define CALCBIT3	1 << 7

EventGroupHandle_t xPiEventGroup;

QueueHandle_t PiQueue;

//Char Array für Ausgabe Resultat
char PiString[20] = "";

extern void vApplicationIdleHook( void );
void vCalcPi(void *pvParameters);
void vInterface(void *pvParameters);

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
    xTaskCreate(vInterface, (const char *) "interface", configMINIMAL_STACK_SIZE+300, NULL, 5, NULL);

    PiQueue = xQueueCreate(1 , sizeof(uint32_t));
    vDisplayClear();
    //vDisplayWriteStringAtPos(0,0,"Pi Rechner");
    //vDisplayWriteStringAtPos(1,0,"Zeit :");
    //vDisplayWriteStringAtPos(2,0,"Pi:");
    vTaskStartScheduler();
    return 0;
}

void vInterface(void *pvParameters) {
    int timecounter = 0;
    xPiEventGroup = xEventGroupCreate();
    initButtons();
    uint32_t evBits = 0x00;
    // Anzahl berchnungen
    uint32_t LoopCount = 0x00000000;
    //Char Array für Ausgabe Resultat
    char LoopString[10] = "";
    for(;;) {
        
        evBits = xEventGroupGetBits(xPiEventGroup);
        
        vDisplayClear();
        vDisplayWriteStringAtPos(0,0,"Pi Rechner");
        vDisplayWriteStringAtPos(1,0,"Zeit :");
        
        if(PiQueue != 0){
            xQueueReceive(PiQueue, &LoopCount, ( TickType_t ) 5 );
             sprintf(LoopString, "%lu", LoopCount);   
                if((evBits & CALCBIT1) == CALCBIT1){
                    if((evBits & CALCBIT0) == CALCBIT0){
                        vDisplayWriteStringAtPos(2,0,"Anz. %s Pause", LoopString);
                    }
                    else{
                        vDisplayWriteStringAtPos(2,0,"Anz. %s Reset", LoopString);
                    }                        
                }
                
                else if((evBits & CALCBIT2) == CALCBIT2){
                    vDisplayWriteStringAtPos(2,0,"Anz. %s MAX", LoopString);
                }
                
                else{
                    vDisplayWriteStringAtPos(2,0,"Anz. %s", LoopString);
                }
            
            vDisplayWriteStringAtPos(3,0,"%s", PiString);
        }        
        
        
        updateButtons();
        timecounter++;
        if(getButtonPress(BUTTON1) == SHORT_PRESSED) {
            xEventGroupSetBits(xPiEventGroup, CALCBIT0);
            xEventGroupClearBits(xPiEventGroup, CALCBIT1);
        }
        if(getButtonPress(BUTTON2) == SHORT_PRESSED) {
            xEventGroupSetBits(xPiEventGroup, CALCBIT1);
            xEventGroupSetBits(xPiEventGroup, CALCBIT0);
            
        }
        if(getButtonPress(BUTTON3) == SHORT_PRESSED) {
           xEventGroupClearBits(xPiEventGroup, CALCBIT0);
           xEventGroupSetBits(xPiEventGroup, CALCBIT1);
           xEventGroupClearBits(xPiEventGroup, CALCBIT2); 
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
        
        uint32_t i = 0x00000000;
        
        float64_t OldPiVal = f_sd(0);                     // Variable für den den Wert vor 2 Berechnungen
        float64_t NewPiVal = f_sd(1);                     // Variable für den neuen Wert 
        float64_t PiBuffer = f_sd(0);                     // Buffer für den vorherigen Wert
        float64_t PiCalcVal = f_sd(0);                    // Variable Zähler
        float64_t PiRes = f_sd(0);                        // Variable fürs Resultat
           
        

        for (;;){
            evBits = xEventGroupGetBits(xPiEventGroup);
            
            if((evBits & CALCBIT0) != CALCBIT0) { 
                // Reset der Variablen
                 i = 0x00000000;
                 
                 xQueueSend(PiQueue, &i, ( TickType_t ) 0 );
                 
                 OldPiVal = f_sd(0);                     
                 NewPiVal = f_sd(1);                     
                 PiBuffer = f_sd(0);                     
                 PiCalcVal = f_sd(0);                    
                 PiRes = f_sd(0);
                 char* tempResultString = f_to_string(PiRes, 18, 18);
                 sprintf(PiString, "Pi:%s", tempResultString);
            // if Reset}
            }
            else{
                                   
                if ((i<0xFFFFFFFE))
                {
	                 for(;(i<0xFFFFFFFE)&((evBits & CALCBIT1) != CALCBIT1);i++){
                        evBits = xEventGroupGetBits(xPiEventGroup);
                        if(NewPiVal == f_sd(1))
                            {
                            OldPiVal = f_sd(0);
                            }
                           else
                            {
                            OldPiVal = PiBuffer;
                            }
                        // Neuer Wert wird  dem Buffer übergeben    
                        PiBuffer = NewPiVal;
                
                        //NewPiVal = NewPiVal + ((NewPiVal - OldPiVal) * i)/(i + i + 1);
                        PiCalcVal = f_sub(NewPiVal,OldPiVal);
                        PiCalcVal = f_mult(PiCalcVal,f_sd(i));
                        PiCalcVal = f_div(PiCalcVal,f_sd(i+i+1));
                        NewPiVal = f_add(NewPiVal,PiCalcVal);
                
                        PiRes = f_add(NewPiVal,NewPiVal);
                        
                        xQueueSend(PiQueue, &i, ( TickType_t ) 0 );
                
                        char* tempResultString = f_to_string(PiRes, 18, 18);
                        sprintf(PiString, "Pi:%s", tempResultString);
                    // For Rechnung              
                    }
            //if Max ereicht
            }
            else{ 
              xEventGroupSetBits(xPiEventGroup, CALCBIT2 );
            }                                                           
        }        
        vTaskDelay(100 / portTICK_RATE_MS);
       } 
   }              
            