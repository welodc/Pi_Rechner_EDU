/*
 * EventBit.c
 *
 * Created: 20.03.2018 18:32:07
 * Author : Hüseyin Dogrucam
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

// Event 
EventGroupHandle_t xPiEventGroup;

//Start und Reset Bit
#define CALCBIT0	1 << 0
//Pause Bit        
#define CALCBIT1	1 << 1
//Calc Max. Bit
#define CALCBIT2	1 << 2
//Timer Startbit
#define CALCBIT3	1 << 3
//Timer Stoppbit
#define CALCBIT4	1 << 4

//Queue
QueueHandle_t PiQueue;

//String für Ausgabe Resultat
char PiString[20] = "";

//Zeitzähler
float TimeCounter = 0.0000000000;

//Tasks
extern void vApplicationIdleHook( void );
void vCalcPi(void *pvParameters);
void vInterface(void *pvParameters);
void vTimeCountTask(void *pvParameters);

//Idle
void vApplicationIdleHook( void )
{
    
}

int main(void)
{
    vInitClock();
    vInitDisplay();
    
    //Creat Task
    xTaskCreate( vCalcPi, (const char *) "CalcPi", configMINIMAL_STACK_SIZE+500, NULL, 1, NULL);
    xTaskCreate(vInterface, (const char *) "interface", configMINIMAL_STACK_SIZE+300, NULL, 2, NULL);
    xTaskCreate(vTimeCountTask, (const char *) "TCTask", configMINIMAL_STACK_SIZE+20, NULL, 2, NULL);
    
    //Creat Event Group
    xPiEventGroup = xEventGroupCreate();
    
    
    //Creat Queue
    PiQueue = xQueueCreate(1 , sizeof(uint32_t));
    
    vDisplayClear();
    
    vTaskStartScheduler();
    
   
    
    return 0;
}

void vTimeCountTask(void *pvParameters) {
     (void) pvParameters;
     
     //Timer initialisieren
     TC0_ConfigClockSource( &TCD0, TC_CLKSEL_OFF_gc ); // Wird ohne Clock initialisiert
     TC0_ConfigWGM(&TCD0, TC_WGMODE_NORMAL_gc);
     TC_SetPeriod( &TCD0, 0x7CFF ); // 31999
     TC0_SetOverflowIntLevel(&TCD0, TC_OVFINTLVL_MED_gc); // Auto restart bei Overflow
    
    //Eventbit Buffer
    uint32_t evBits = 0x00;

    for (;;)
    {
        //EventBits empfangen
        evBits = xEventGroupGetBits(xPiEventGroup);
        
        //If Timer Start Bit
        if((evBits & CALCBIT3) == CALCBIT3)                        
        {
                //If Pause
                if((evBits & CALCBIT1) == CALCBIT1)
            {
                //Timer Pause
                TC0_ConfigClockSource( &TCD0, TC_CLKSEL_OFF_gc );
            }
            else
            {
                //If Pi auf 5 Stellen nach dem Komma Berechnet
                if ((evBits & CALCBIT4) != CALCBIT4)
                {
                    TC0_ConfigClockSource( &TCD0, TC_CLKSEL_DIV1_gc );
                    xEventGroupSetBits(xPiEventGroup, CALCBIT0); 
                }
                else
                {
                    //Timer Pause
                    TC0_ConfigClockSource( &TCD0, TC_CLKSEL_OFF_gc ); 
                    xEventGroupSetBits(xPiEventGroup, CALCBIT0);
                    xEventGroupClearBits(xPiEventGroup, CALCBIT3);
                }
            }           
        }
        else
        {
            //Timer Aus
            TC0_ConfigClockSource( &TCD0, TC_CLKSEL_OFF_gc ); 
        }
        
        //If Timer in gebrauch        
        if ((evBits & CALCBIT4) == CALCBIT4)
        {
            vTaskDelay(100/portTICK_RATE_MS);
        }
        else
        {
            vTaskDelay(5/portTICK_RATE_MS);
        }
        
    }
  
}  
    

void vInterface(void *pvParameters) {
    (void) pvParameters;
    
    initButtons();
   
   //Eventbit Buffer 
    uint32_t evBits = 0x00;
    
    // Anzahl berchnungen
    uint32_t LoopCount = 0x00000000;
    //Char Array für Ausgabe Resultat
    char LoopString[10] = "";
    //Char Array für die Zeitausgabe
    char TimeString[7] = "";
    
    
    for(;;) {
        
        //EventBits empfangen
        evBits = xEventGroupGetBits(xPiEventGroup);
        
        vDisplayClear();
        //
        vDisplayWriteStringAtPos(0,0,"Pi Rechner");
        //
        sprintf(TimeString, "%.3f", TimeCounter);
        vDisplayWriteStringAtPos(1,0,"Zeit :%ssec", TimeString);
        //If Daten im Queue
        if(PiQueue != 0){
            
            //Daten aus dem Queue nehmen
            xQueueReceive(PiQueue, &LoopCount, ( TickType_t ) 5 );
            sprintf(LoopString, "%lu", LoopCount);   
            
            //If Pausen 
            if((evBits & CALCBIT1) == CALCBIT1){
                    //If Rechenbit Ein sonst
                    if((evBits & CALCBIT0) == CALCBIT0)
                    {
                        vDisplayWriteStringAtPos(2,0,"Anz. %s Pause", LoopString);
                    }
                    else
                    {
                        vDisplayWriteStringAtPos(2,0,"Anz. %s Reset", LoopString);
                    }                        
                }
                //If  Max ereicht
                else if((evBits & CALCBIT2) == CALCBIT2)
                {
                    vDisplayWriteStringAtPos(2,0,"Anz. %s MAX", LoopString);
                }
                //Normalzustand
                else
                {
                    vDisplayWriteStringAtPos(2,0,"Anz. %s", LoopString);
                }
            //
            vDisplayWriteStringAtPos(3,0,"%s", PiString);
        }        
        
        
        updateButtons();
        
        //Start Button
        if(getButtonPress(BUTTON1) == SHORT_PRESSED) {
            xEventGroupSetBits(xPiEventGroup, CALCBIT3);
            xEventGroupClearBits(xPiEventGroup, CALCBIT1);
        }
        //Pause Button
        if(getButtonPress(BUTTON2) == SHORT_PRESSED) {
            xEventGroupSetBits(xPiEventGroup, CALCBIT1);
            xEventGroupSetBits(xPiEventGroup, CALCBIT3);
            
        }
        //ResetButton
        if(getButtonPress(BUTTON3) == SHORT_PRESSED) {
           xEventGroupClearBits(xPiEventGroup, CALCBIT0);
           xEventGroupSetBits(xPiEventGroup, CALCBIT1);
           xEventGroupClearBits(xPiEventGroup, CALCBIT2);
           xEventGroupClearBits(xPiEventGroup, CALCBIT3);
             
        }
        /* Reserves
        if(getButtonPress(BUTTON4) == SHORT_PRESSED) {
            
        }
        if(getButtonPress(BUTTON1) == LONG_PRESSED) {
            
        }
        if(getButtonPress(BUTTON2) == LONG_PRESSED) {
            
        }
        if(getButtonPress(BUTTON3) == LONG_PRESSED) {
            
        }
        if(getButtonPress(BUTTON4) == LONG_PRESSED) {
            
        }*/

        vTaskDelay(10/portTICK_RATE_MS);
    }
}

void vCalcPi(void *pvParameters) {
    (void) pvParameters;
    //Eventbit Buffer
    uint32_t evBits = 0x00;
        
        //Zähler For-Schleife 
        uint32_t i = 0x00000000;
        
        // Variable für den den Wert vor 2 Berechnungen
        float64_t OldPiVal = f_sd(0);                  
        // Variable für den neuen Wert    
        float64_t NewPiVal = f_sd(1);
        // Buffer für den vorherigen Wert                     
        float64_t PiBuffer = f_sd(0); 
        // Variable Zähler                    
        float64_t PiCalcVal = f_sd(0);   
        // Variable fürs Resultat                 
        float64_t PiRes = f_sd(0);                        
        
        //Pi als Float zum Vergleichen 5 Stellen nach dem Komma
        float64_t Pi5Float = f_sd(3.14159);
        

        for (;;){
            //EventBits empfangen
            evBits = xEventGroupGetBits(xPiEventGroup);
            
            //If gestoppt oder noch nicht gestartet
            if((evBits & CALCBIT0) != CALCBIT0) { 
                
                // Reset der Variablen
                 i = 0x00000000;
                 
                 //Senden in die Queue nach dem Reset
                 xQueueSend(PiQueue, &i, ( TickType_t ) 0 );
                 
                 TimeCounter = 0.00000;
                 
                 
                 OldPiVal = f_sd(0);                     
                 NewPiVal = f_sd(1);                     
                 PiBuffer = f_sd(0);                     
                 PiCalcVal = f_sd(0);                    
                 PiRes = f_sd(0);
                 char* tempResultString = f_to_string(PiRes, 18, 18);
                 sprintf(PiString, "Pi:%s", tempResultString);
                 
                 //Reset des Timer aus Bits
                 xEventGroupClearBits(xPiEventGroup, CALCBIT4);
      
            }
            else{
               //If Zähler Maximum nicht erreicht                    
                if ((i<0xFFFFFFFE))
                {
                    //Solange Pausebit nicht 1
	                 for(;(evBits & CALCBIT1) != CALCBIT1;i++){
                        
                        //EventBits empfangen
                        evBits = xEventGroupGetBits(xPiEventGroup);
                        
                        //If für    0! = 1     1!= 1     2! = 3    3! = 6 
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
                
                        //NewPiVal = NewPiVal + ((NewPiVal - OldPiVal) * i)/(i + i + 1); -> Pi halbe
                        PiCalcVal = f_sub(NewPiVal,OldPiVal);
                        PiCalcVal = f_mult(PiCalcVal,f_sd(i));
                        PiCalcVal = f_div(PiCalcVal,f_sd(i+i+1));
                        NewPiVal = f_add(NewPiVal,PiCalcVal);
                
                        PiRes = f_add(NewPiVal,NewPiVal);
                        
                        //Senden in die Queue
                        xQueueSend(PiQueue, &i, ( TickType_t ) 0 );
                        
                        //Float64 zu String
                        char* tempResultString = f_to_string(PiRes, 18, 18);
                        sprintf(PiString, "Pi:%s", tempResultString);
                        
                        //If Timer noch ein
                        if ((evBits & CALCBIT4) != CALCBIT4)
                        {
                            //If 5 Stellen nach dem Komma ereicht 
                            if((Pi5Float & PiRes) == Pi5Float)
                            {
                                //Timer Aus
                                xEventGroupSetBits(xPiEventGroup, CALCBIT4);
                            }
                        
                        }                                                      
                    }
            }
            else
            { 
              //Max ereicht
              xEventGroupSetBits(xPiEventGroup, CALCBIT2 );
            }                                                           
        }        
        vTaskDelay(100 / portTICK_RATE_MS);
       } 
}  

// Time Counter 
ISR(TCD0_OVF_vect)
{   
   TimeCounter = TimeCounter +0.001;   
}
             
            