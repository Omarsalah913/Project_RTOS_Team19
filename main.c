#include "FreeRTOS.h"
#include "FreeRTOSConfig.h"

#include "ports_init.h"

#include "task.h"
#include <stdint.h>
#include <stdbool.h>
#include "inc/hw_memmap.h"
#include "driverlib/debug.h"
#include "driverlib/gpio.h"
#include "driverlib/sysctl.h"
#include "driverlib/systick.h"
#include "inc/hw_ints.h"
#include "semphr.h"

#include "driverlib/interrupt.h"
#include "tm4c123gh6pm.h"


void vApplicationIdleHook(void);
void vApplicationIdleHook(void){
//DO NOTHING
}

	
void PortF_Init(void);
void PortE_Init(void);
void PortD_Init(void);
void Timer0A(unsigned short time);
void TIMER0A_Handler(void);
 
static unsigned long Timer_Ticks = 0;
volatile unsigned char manualUp=0;
volatile unsigned char manualDown=0;
volatile unsigned char motorUp=0;
volatile unsigned char LSUP=0;
volatile unsigned char LSDN=0;
volatile unsigned char control=0;

//Enum for Control Task
enum tasks{
	DRIVER_UP=1,DRIVER_DOWN,PASSENGER_UP_MANUAL,PASSENGER_DOWN_MANUAL,PASSENGER_UP_AUTO,PASSENGER_DOWN_AUTO,JAMMING
};

//Define Semaphore handle
xSemaphoreHandle xDriverUp;
xSemaphoreHandle xDriverDown;
xSemaphoreHandle xPassengerUp;
xSemaphoreHandle xPassengerDown;
xSemaphoreHandle xJamming;
xSemaphoreHandle xPassengerUpAuto;
xSemaphoreHandle xPassengerDownAuto;

xSemaphoreHandle xMutex;

//Define Task handle
TaskHandle_t drivUp;	
TaskHandle_t passengerUp;
TaskHandle_t passengerUpAuto;

//Timer0A init
void Timer0A(unsigned short time){ 

  SYSCTL_RCGCTIMER_R |= 0x01;   // 0) activate TIMER0
  TIMER0_CTL_R = 0x0;    // 1) disable TIMER0A during configuration
  TIMER0_CFG_R = 0x0;    // 2) configure for 32-bit mode
  TIMER0_TAMR_R = 0x02;   // 3) configure first bit for periodic mode, second bit for down-count settings (0x12 for count up)
  TIMER0_TAILR_R = (16000000*time)-1;    // 4) reload value
  TIMER0_ICR_R = 0x01;    // 6) clear TIMER0A timeout flag
  TIMER0_IMR_R = 0x01;    // 7) arm timeout interrupt
  NVIC_EN0_R = 1<<19;           // 9) ector number 35, enable interrupt number 19 in NVIC
  TIMER0_CTL_R = 0x01;    // 10) enable TIMER0A
  
}
//Timer0A Handler(ISR)
void TIMER0A_Handler(void)
{
  Timer_Ticks++;
  
  GPIO_PORTF_DATA_R ^= 0x08;

  TIMER0_ICR_R = TIMER_ICR_TATOCINT;// acknowledge timer0A timeout
}

/********************************Control Task*******************************/
//Sends semaphore to the correct task
void Controller (void *pvParameters){
	for(;;){
		switch (control){
			case DRIVER_UP: xSemaphoreGive(xDriverUp);
				break;
			case DRIVER_DOWN: xSemaphoreGive(xDriverDown);			
				break;
			case PASSENGER_UP_MANUAL: xSemaphoreGive(xPassengerUp);			
				break;		
			case PASSENGER_DOWN_MANUAL: xSemaphoreGive(xPassengerDown);			
				break;
			case PASSENGER_UP_AUTO: xSemaphoreGive(xPassengerUpAuto);			
				break;
			case PASSENGER_DOWN_AUTO: xSemaphoreGive(xPassengerDownAuto);			
				break;
			case JAMMING: xSemaphoreGive(xJamming);
				break;			
		  default:
			GPIOPinWrite(GPIO_PORTD_BASE, GPIO_PIN_1,0x0);
			GPIOPinWrite(GPIO_PORTD_BASE, GPIO_PIN_2,0x0);
			GPIOPinWrite(GPIO_PORTD_BASE, GPIO_PIN_3,0x0);
				break;			
			}			
	}
}

/********************************Driver Up Manual/Automatic*******************************/
void DriverUp(void *pvParameters){
	
		xSemaphoreTake(xDriverUp,0);
	for(;;)
	{
		xSemaphoreTake(xDriverUp,portMAX_DELAY);	
		
		motorUp=1;
		
		for(int i =0;i<=16000*75;i++){}
			// 							SW2								 Limit Switch		  	Jamming
		while((!(GPIO_PORTF_DATA_R & 0x01))&& (LSUP==0)&&((GPIO_PORTE_DATA_R & 0x04)))
		{
			manualUp=1;
			GPIOPinWrite(GPIO_PORTD_BASE, GPIO_PIN_1,0x0);
			GPIOPinWrite(GPIO_PORTD_BASE, GPIO_PIN_2,GPIO_PIN_2);
			GPIOPinWrite(GPIO_PORTD_BASE, GPIO_PIN_3,GPIO_PIN_3);
		}
		//Automatic
		//						SW2
		if( ((GPIO_PORTF_DATA_R & 0x01)) && manualUp ==0 ){ 
			// 	Limit Switch				Jamming
			while ((LSUP==0)&&((GPIO_PORTE_DATA_R & 0x04))){ 
				//						SW1															SW2
				if((!(GPIO_PORTF_DATA_R & 0x10)) || (!(GPIO_PORTF_DATA_R & 0x01)))//Corner case
				{
					break;
				}
				GPIOPinWrite(GPIO_PORTD_BASE, GPIO_PIN_1,0x0);
		    GPIOPinWrite(GPIO_PORTD_BASE, GPIO_PIN_2,GPIO_PIN_2);
				GPIOPinWrite(GPIO_PORTD_BASE, GPIO_PIN_3,GPIO_PIN_3);
				manualUp =1;
				}				
	    }
			LSUP=0;
			control=0;

	}
}

/************Driver Down Manual/Automatic*****************/

void DriverDown(void *pvParameters){
	
		xSemaphoreTake(xDriverDown,0);

	for(;;)
	{
		xSemaphoreTake(xDriverDown,portMAX_DELAY);
		motorUp=0;
		
		for(int i =0;i<=16000*75;i++){}
		//						SW1
		while((!(GPIO_PORTF_DATA_R & 0x10)))
		{
			if(LSDN==1){
				break;
			}
			else{
			manualDown=1;
			GPIOPinWrite(GPIO_PORTD_BASE, GPIO_PIN_1,GPIO_PIN_1);
			GPIOPinWrite(GPIO_PORTD_BASE, GPIO_PIN_2,0x0);
			GPIOPinWrite(GPIO_PORTD_BASE, GPIO_PIN_3,GPIO_PIN_3);	
			}
		}
		//Automatic
		//							SW1
		if( ((GPIO_PORTF_DATA_R & 0x10)) && (manualDown ==0) ){ //automatic down

			while (LSDN==0){
				//						SW1															SW2
				if((!(GPIO_PORTF_DATA_R & 0x10)) || (!(GPIO_PORTF_DATA_R & 0x01))) //Corner case
				{
					break;
				}
				GPIOPinWrite(GPIO_PORTD_BASE, GPIO_PIN_1,GPIO_PIN_1);
				GPIOPinWrite(GPIO_PORTD_BASE, GPIO_PIN_2,0x0);
				GPIOPinWrite(GPIO_PORTD_BASE, GPIO_PIN_3,GPIO_PIN_3); 
				manualDown =1;
				}				
	    }
		LSDN=0;
			control=0;	

	}
}


/***************Passenger Up Manual*****************/

void PassengerUp(void *pvParameters){ 
	
		xSemaphoreTake(xPassengerUp,0);
	
	for(;;)
	{
		xSemaphoreTake(xPassengerUp,portMAX_DELAY);
		motorUp=1;	
		for(int i =0;i<=16000*75;i++){}
			//	  	Passenger up manual					        	Lock									    	Passnger down manual					Jamming
	  while((!(GPIO_PORTD_DATA_R & 0x01)) && (!(GPIO_PORTE_DATA_R & 0x02))&&((GPIO_PORTD_DATA_R & 0x40))&&((GPIO_PORTE_DATA_R & 0x04))) 
		{
			//    LS                SW2																		SW1
			if((LSUP==1)|| (!(GPIO_PORTF_DATA_R & 0x01))|| (!(GPIO_PORTF_DATA_R & 0x10))){ 
				break;
			}
      GPIOPinWrite(GPIO_PORTD_BASE, GPIO_PIN_1,0x0);
			GPIOPinWrite(GPIO_PORTD_BASE, GPIO_PIN_2,GPIO_PIN_2);
			GPIOPinWrite(GPIO_PORTD_BASE, GPIO_PIN_3,GPIO_PIN_3);
	  }
		  
		LSUP=0;
			control=0;		

	}
}

/***************Passenger Down Manual*****************/

void PassengerDown(void *pvParameters){
		xSemaphoreTake(xPassengerDown,0);
		
	for(;;)
	{
		xSemaphoreTake(xPassengerDown,portMAX_DELAY);
		motorUp=0;	
		for(int i =0;i<=16000*75;i++){}	
			//					Passnger down manual										Lock									Passenger up manual
		  while((!(GPIO_PORTD_DATA_R & 0x40)) && (!(GPIO_PORTE_DATA_R & 0x02))&&((GPIO_PORTD_DATA_R & 0x01)))
			{
				//	LS				    			SW2																SW1
				if(LSDN==1 || (!(GPIO_PORTF_DATA_R & 0x01))|| (!(GPIO_PORTF_DATA_R & 0x10))){//limit switch down pressed
					break;
				}
			GPIOPinWrite(GPIO_PORTD_BASE, GPIO_PIN_1,GPIO_PIN_1);
			GPIOPinWrite(GPIO_PORTD_BASE, GPIO_PIN_2,0x0);
			GPIOPinWrite(GPIO_PORTD_BASE, GPIO_PIN_3,GPIO_PIN_3);
	    }
		  
			LSDN=0;
			control=0;	

	}
}

/***************Passenger Up Automatic*****************/

void PassengerUpAuto(void *pvParameters){
	
		xSemaphoreTake(xPassengerUpAuto,0);

	for(;;)
	{
		xSemaphoreTake(xPassengerUpAuto,portMAX_DELAY);
		motorUp=1;
		//				LS							    Lock									Passenger Down Automatic					
		while ((LSUP==0)&& (!(GPIO_PORTE_DATA_R & 0x02))&&((GPIO_PORTE_DATA_R & 0x10))&&((GPIO_PORTE_DATA_R & 0x04))){
			//				Passenger up manual					Passneger down manual										SW2															SW1
			if((!(GPIO_PORTD_DATA_R & 0x01))||(!(GPIO_PORTD_DATA_R & 0x40))|| (!(GPIO_PORTF_DATA_R & 0x01))|| (!(GPIO_PORTF_DATA_R & 0x10))){
				break;
			}
			GPIOPinWrite(GPIO_PORTD_BASE, GPIO_PIN_1,0x0);
	    GPIOPinWrite(GPIO_PORTD_BASE, GPIO_PIN_2,GPIO_PIN_2);
      GPIOPinWrite(GPIO_PORTD_BASE, GPIO_PIN_3,GPIO_PIN_3);
				}	
			LSUP=0;		
			control=0;		

	}
}

/***************Passenger Down Automatic*****************/

void PassengerDownAuto(void *pvParameters){
	
  xSemaphoreTake(xPassengerDownAuto,0);
	
	for(;;)
	{
		xSemaphoreTake(xPassengerDownAuto,portMAX_DELAY);
		motorUp=0;
		//				LS							Lock												Passenger Up Automatic
	  while ((LSDN==0)&& (!(GPIO_PORTE_DATA_R & 0x02))&&((GPIO_PORTE_DATA_R & 0x08))){
			//				Passenger up manual							Passneger down manual						  		SW2															SW1
			if((!(GPIO_PORTD_DATA_R & 0x01))||(!(GPIO_PORTD_DATA_R & 0x40))|| (!(GPIO_PORTF_DATA_R & 0x01))|| (!(GPIO_PORTF_DATA_R & 0x10))){
				break;
			}
 		  GPIOPinWrite(GPIO_PORTD_BASE, GPIO_PIN_1,GPIO_PIN_1);
	 	  GPIOPinWrite(GPIO_PORTD_BASE, GPIO_PIN_2,0x0);
		  GPIOPinWrite(GPIO_PORTD_BASE, GPIO_PIN_3,GPIO_PIN_3); 
				}		
			LSDN=0;
			control=0;

	}
}

void Jamming(void *pvParameters){
	xSemaphoreTake(xJamming,0);
	
	for(;;)
	{
		xSemaphoreTake(xJamming,portMAX_DELAY);
		
		if(motorUp==1) //To lock up push button until a down push button is pressed
		{
		//Stop
		for(int i =0;i<=16000*20;i++)
		{
			GPIOPinWrite(GPIO_PORTD_BASE, GPIO_PIN_1,0x0);
	  	GPIOPinWrite(GPIO_PORTD_BASE, GPIO_PIN_2,0x0);
		  GPIOPinWrite(GPIO_PORTD_BASE, GPIO_PIN_3,0x0);
		}
		//Reverse Direction
		for(int i =0;i<=16000*30;i++)
		{
			GPIOPinWrite(GPIO_PORTD_BASE, GPIO_PIN_1,GPIO_PIN_1);
	  	GPIOPinWrite(GPIO_PORTD_BASE, GPIO_PIN_2,0x0);
		  GPIOPinWrite(GPIO_PORTD_BASE, GPIO_PIN_3,GPIO_PIN_3);
		}
		}
		GPIOPinWrite(GPIO_PORTD_BASE, GPIO_PIN_1,0x0);
	  GPIOPinWrite(GPIO_PORTD_BASE, GPIO_PIN_2,0x0);
	  GPIOPinWrite(GPIO_PORTD_BASE, GPIO_PIN_3,0x0);
		control=0;
		while(((GPIO_PORTD_DATA_R & 0x40))&&((GPIO_PORTE_DATA_R & 0x10))&&(GPIO_PORTF_DATA_R & 0x10)){}

	
  }
}



int main( void )
{
	//Initializing Ports
    PortF_Init();
	  PortD_Init();
	  PortE_Init();

	//Initializing Semaphores	
		vSemaphoreCreateBinary(xDriverUp);
		vSemaphoreCreateBinary(xDriverDown);
		vSemaphoreCreateBinary(xPassengerUp);
		vSemaphoreCreateBinary(xPassengerDown);	
		vSemaphoreCreateBinary(xJamming);	
		vSemaphoreCreateBinary(xPassengerUpAuto);
		vSemaphoreCreateBinary(xPassengerDownAuto);	
		xMutex = xSemaphoreCreateMutex();
	
		Timer0A(2);
		
	if(( xDriverUp != NULL )&& (xDriverDown !=NULL ) && (xPassengerUp !=NULL) && (xPassengerDown !=NULL) && (xJamming !=NULL) && (xPassengerUpAuto !=NULL) && (xPassengerDownAuto !=NULL))
		{
			//Initializing Tasks
			xTaskCreate( DriverUp, "Driver upwards", 250, NULL, 4, &drivUp );
			xTaskCreate( DriverDown, "Driver downwards", 250, NULL, 4, NULL );
			xTaskCreate( PassengerUp, "Passenger Up", 200, NULL, 3, &passengerUp );
			xTaskCreate( PassengerDown, "Passenger Down", 200, NULL, 3, NULL );
			xTaskCreate( PassengerUpAuto, "Passenger Up Auto", 200, NULL, 3, &passengerUpAuto );
			xTaskCreate( PassengerDownAuto, "Passenger Down Auto", 200, NULL, 3, NULL );
		  xTaskCreate( Jamming, "Jamming", 200, NULL, 3, NULL );
			xTaskCreate( Controller,"Controller",160,NULL,2,NULL);			
			vTaskStartScheduler();
			
		}

    /* If all is well we will never reach here as the scheduler will now be
    running the tasks.  If we do reach here then it is likely that there was
    insufficient heap memory available for a resource to be created. */
    for( ;; );
}



//Port-F handler
void GPIOF_Handler(void){
		
			if ( GPIO_PORTF_MIS_R & 0x01) // check if interrupt causes by PF0/SW2
			{
				manualUp= 0;
				control=DRIVER_UP;
				GPIO_PORTF_ICR_R |= 0x01; 
			}else if ( GPIO_PORTF_MIS_R & 0x10) // check if interrupt causes by PF4/SW1
			{
				manualDown= 0;		
				control=DRIVER_DOWN;				
				GPIO_PORTF_ICR_R |= 0x10; 				
			}
}


void GPIOD_Handler(void){
	
	if ( GPIO_PORTD_MIS_R & 0x01) // check if interrupt causes by PD0
  { 
			control=PASSENGER_UP_MANUAL;
			GPIO_PORTD_ICR_R |= 0x01; 
	}else if ( GPIO_PORTD_MIS_R & 0x40) // check if interrupt causes by PD6
  { 
			control=PASSENGER_DOWN_MANUAL;
			GPIO_PORTD_ICR_R |= 0x40;

	}else if (GPIO_PORTD_MIS_R & 0x80) // check if interrupt causes by PD7
	{
			LSUP =1;
			GPIO_PORTD_ICR_R |= 0x80;		
	}

}


void GPIOE_Handler(void){
	
	if (( GPIO_PORTE_MIS_R & 0x04)){ // check if interrupt causes by PE2
		
		control=JAMMING;
		GPIO_PORTE_ICR_R |= 0x04; 
	}
	else	if ( GPIO_PORTE_MIS_R & 0x08){ // check if interrupt causes by PE3
		control=PASSENGER_UP_AUTO;
		GPIO_PORTE_ICR_R |= 0x08; 
	}
	else	if ( GPIO_PORTE_MIS_R & 0x10){ 	// check if interrupt causes by PE4
		control=PASSENGER_DOWN_AUTO;
		GPIO_PORTE_ICR_R |= 0x10; 
		
	}else if(GPIO_PORTE_MIS_R & 0x01) // check if interrupt causes by PE0
	{
		LSDN=1;
		GPIO_PORTE_ICR_R |= 0x01;
	}

}