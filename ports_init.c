#include "ports_init.h"
#include "tm4c123gh6pm.h"


#include <stdint.h>
#include <stdbool.h>
#include "inc/hw_memmap.h"
#include "driverlib/debug.h"
#include "driverlib/gpio.h"
#include "driverlib/sysctl.h"
#include "driverlib/systick.h"
#include "driverlib/interrupt.h"
#include "inc/hw_ints.h"

//Initialize the hardware of Port-D
void PortD_Init(void){ 
	
	  SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOD); //Enabling clock
	
	  GPIO_PORTD_LOCK_R = 0x4C4F434B;   //Unlock commit register 
    GPIO_PORTD_CR_R  = 0xDF;           //Make PORTD configurable
    GPIO_PORTD_LOCK_R = 0;            //Lock commit register

		//Configuring Pins to be input
	  GPIOPinTypeGPIOInput(GPIO_PORTD_BASE, GPIO_PIN_0); //Passenger up manual
	  GPIOPinTypeGPIOInput(GPIO_PORTD_BASE, GPIO_PIN_6); // Passneger down manual
    GPIOPinTypeGPIOInput(GPIO_PORTD_BASE, GPIO_PIN_7); //Limit Switch Up
	
		//Configuring Pins to be output
    GPIOPinTypeGPIOOutput(GPIO_PORTD_BASE, GPIO_PIN_1);
    GPIOPinTypeGPIOOutput(GPIO_PORTD_BASE, GPIO_PIN_2);
	  GPIOPinTypeGPIOOutput(GPIO_PORTD_BASE, GPIO_PIN_3);
	
	  //Pull down
		GPIO_PORTD_PUR_R |= (1<<0) | (1<<6) | (1<<7);
		

	 GPIOPinWrite(GPIO_PORTD_BASE, GPIO_PIN_1,0x0);  //IN1
	 GPIOPinWrite(GPIO_PORTD_BASE, GPIO_PIN_2,0x0);  //IN2
	 GPIOPinWrite(GPIO_PORTD_BASE, GPIO_PIN_3,0x0);  //EN
	  
	 //Setting Interrupt type for PD0,PD6,PD7 (Falling)
	 GPIOIntTypeSet(GPIO_PORTD_BASE, GPIO_PIN_0,GPIO_FALLING_EDGE);
	 GPIOIntTypeSet(GPIO_PORTD_BASE, GPIO_PIN_6,GPIO_FALLING_EDGE);
	 GPIOIntTypeSet(GPIO_PORTD_BASE, GPIO_PIN_7,GPIO_FALLING_EDGE);
	 
	 //Clearing interrupt on PortD 
	 GPIOIntClear(GPIO_PORTD_BASE, INT_GPIOD_TM4C123);
	
	// Setup the interrupt on PortD
	GPIO_PORTD_IM_R  |= (1<<0) | (1<<6)| (1<<7); //Mask
	IntDisable(INT_GPIOD_TM4C123);
  IntPrioritySet(INT_GPIOD_TM4C123,2); 
  IntEnable(INT_GPIOD_TM4C123);

}

//Initialize the hardware of Port-E
void PortE_Init(void){ 
	
	  SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOE); //Enabling clock
	  GPIO_PORTE_LOCK_R = 0x4C4F434B;   // unlock commit register
		GPIO_PORTE_CR_R  = 0x1F;           // make PORTE configurable
		GPIO_PORTE_LOCK_R = 0;             	// lock commit register 
	
	  //Configuring Pins to be input
		GPIOPinTypeGPIOInput(GPIO_PORTE_BASE, GPIO_PIN_0); //Limit Switch Down
		GPIOPinTypeGPIOInput(GPIO_PORTE_BASE, GPIO_PIN_1); //Lock
    GPIOPinTypeGPIOInput(GPIO_PORTE_BASE, GPIO_PIN_2); //Jamming
		GPIOPinTypeGPIOInput(GPIO_PORTE_BASE, GPIO_PIN_3); //Passenger Up Automatic
    GPIOPinTypeGPIOInput(GPIO_PORTE_BASE, GPIO_PIN_4); //Passenger Down Automatic

	  //Pull down
		GPIO_PORTE_PUR_R  |= (1<<2) | (1<<3) | (1<<4)| (1<<0);
		GPIO_PORTE_PDR_R  |= (1<<1);

		//Setting interrupt types
		GPIOIntTypeSet(GPIO_PORTE_BASE, GPIO_PIN_0,GPIO_FALLING_EDGE);
	  GPIOIntTypeSet(GPIO_PORTE_BASE, GPIO_PIN_2,GPIO_FALLING_EDGE);
		GPIOIntTypeSet(GPIO_PORTE_BASE, GPIO_PIN_3,GPIO_FALLING_EDGE);
		GPIOIntTypeSet(GPIO_PORTE_BASE, GPIO_PIN_4,GPIO_FALLING_EDGE);
	
		//Clearing interrupt on PortE		
	  GPIOIntClear(GPIO_PORTE_BASE, INT_GPIOE_TM4C123);
	 

		// Setup the interrupt on PortE
		GPIO_PORTE_IM_R  |= (1<<2) | (1<<3) | (1<<4)|(1<<0);
		IntDisable(INT_GPIOE_TM4C123);
		IntPrioritySet(INT_GPIOE_TM4C123,4);
		IntEnable(INT_GPIOE_TM4C123);
	
}

//Initialize the hardware of Port-F
void PortF_Init(void){ 
	
	  SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);
	  GPIO_PORTF_LOCK_R = 0x4C4F434B;   //Unlock commit register
		GPIO_PORTF_CR_R  = 0x09;           //Make PORTF configurable
		GPIO_PORTF_LOCK_R = 0;             	//Lock commit register 
	
	  //Configuring Pins to be input
		GPIOPinTypeGPIOInput(GPIO_PORTF_BASE, GPIO_PIN_0); // SW2
		GPIOPinTypeGPIOInput(GPIO_PORTF_BASE, GPIO_PIN_4); // SW1
	
		//Configuring Pins to be input
	  GPIOPinTypeGPIOOutput(GPIO_PORTF_BASE, GPIO_PIN_3);

	  //Pull Up
	  GPIO_PORTF_PUR_R  |= (1<<0)|(1<<4);
 
	 GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_3,0x0); //Iniitializing Green led to be off
	 
	 //Setting interrupt types
	 GPIOIntTypeSet(GPIO_PORTF_BASE, GPIO_PIN_0,GPIO_FALLING_EDGE); 
	 GPIOIntTypeSet(GPIO_PORTF_BASE, GPIO_PIN_4,GPIO_FALLING_EDGE);
	 
	 //Clearing interrupt on PortF 
	 GPIOIntClear(GPIO_PORTF_BASE, INT_GPIOF_TM4C123);
	 
		// Setup the interrupt on PortF
		GPIO_PORTF_IM_R  |= (1<<4)|(1<<0);
		IntDisable(INT_GPIOF_TM4C123);
		IntPrioritySet(INT_GPIOF_TM4C123,3);
		IntEnable(INT_GPIOF_TM4C123);

}