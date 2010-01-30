//Various utility functions for the Xiphos 1.0 Board
#include "globals.h"
#include <util/delay.h>
#include <avr/wdt.h>

/*
Key of pin operations:
DDR:           0=input, 1=output
input PORT:    0=disable pullup, 1=enable pullup
output PORT:   0=drive low, 1=drive high
write to PIN:  1=toggle value of PORT
read from PIN: value on the pin


Initializations:
DDRD4 = 0 - BTN1 input
PORT4 = 1 - BTN1 pullup

DDRG0 = X - NC
DDRG1 = X - NC
DDRG2 = 1 - LED (output)
DDRG3 = ? - Q2 Oscillator
DDRG4 = ? - Q2 Oscillator
*/

//perform all initialization
void initialize()
{
	//configure BTN1 as an input
	cbi(DDRD, 4);
	//enable pullup for BTN1
	sbi(PORTD, 4);

	//configure LED as an output
	sbi(DDRG, 2);

	//configure 74LS374 (D Flip-Flop) clock pin as an output
	sbi(DDRD, 5);

	//configure LCD/Servo bus on port C as an output
	DDRC = 0xFF;

	#if USE_LCD == 1
		//initialize LCD
		lcdInit();
	#endif

	#if USE_I2C == 1
		//configure I2C clock rate
		i2cInit();
	#endif

	#if USE_MOTOR0 == 1 || USE_MOTOR1 == 1
		//initialize enabled motors
		motorInit();
	#endif

	#if NUM_SERVOS > 0
		//initialize servos
		servoInit();
	#endif

	#if USE_ADC == 1
		//initialize ADC
		adcInit();
	#endif

	#if USE_UART0 == 1
		//initialize UART0
		uart0Init();
	#endif

	#if USE_UART1 == 1
		//initialize UART1
		uart1Init();
	#endif
	
	#if USE_RTC == 1
		//initialize RTC
		rtcInit();
	#endif
}

//Provides a busy wait loop for an approximate number of milliseconds
void delayMs(u16 num)
{
	for (; num > 0; num--)
	{
		_delay_loop_2(4000);
	}
}

//Provides a busy wait loop for an approximate number of microseconds
void delayUs(u16 num)
{
	for (; num > 0; num--)
	{
		_delay_loop_2(4);
	}
}

//Checks the status of the BTN1 button.
//Returns 0 when the button is not pressed and 1 when the button is pressed.
u08 getButton1()
{
	return gbi(PIND, 4) == 0;
}

//Waits for a complete button press and release, with button debouncing.
void buttonWait()
{
	//wait for button to be pushed down
	while (!getButton1());
	//delay 30 ms for button debouncing
	delayMs(30);
	//wait for button to be released, if it is still down
	while (getButton1());
	//delay 30 ms for button debouncing
	delayMs(30);
}

//Turns the led on
void ledOn()
{
	sbi(PORTG, 2);
}

//Turns the led off
void ledOff()
{
	cbi(PORTG, 2);
}

//Sets the direction and pullup resistor option for a digital pin.
//num argument can be 0 to 9.
//Use the INPUT, INPUT_PULLUP, or OUTPUT values of the Direction enumeration to specify the direction parameter.
void digitalDirection(u08 num, Direction direction)
{
	if (num > 1)
	{
		//adjust num offset here to simplify the code below
		num -= 2;
		switch (direction)
		{
		case INPUT:
			cbi(DDRA, num);  //input direction
			cbi(PORTA, num); //disable pullup
			break;
		case INPUT_PULLUP:
			cbi(DDRA, num);  //input direction
			sbi(PORTA, num); //enable pullup
			break;
		case OUTPUT:
			sbi(DDRA, num);  //output direction
			break;
		}
	}
	else if (num == 1)
	{
		switch (direction)
		{
		case INPUT:
			cbi(DDRB, 7);  //input direction
			cbi(PORTB, 7); //disable pullup
			break;
		case INPUT_PULLUP:
			cbi(DDRB, 7);  //input direction
			sbi(PORTB, 7); //enable pullup
			break;
		case OUTPUT:
			sbi(DDRB, 7);  //output direction
			break;
		}
	}
	else if (num == 0)
	{
		switch (direction)
		{
		case INPUT:
			cbi(DDRB, 4);  //input direction
			cbi(PORTB, 4); //disable pullup
			break;
		case INPUT_PULLUP:
			cbi(DDRB, 4);  //input direction
			sbi(PORTB, 4); //enable pullup
			break;
		case OUTPUT:
			sbi(DDRB, 4);  //output direction
			break;
		}
	}
}

//Sets the data direction for all 10 digital pins, using the lower 10 bits of the argument.
//Bit 0 (LSB) matches digital0 ... bit 9 matches digital9.
//A high (1) sets the pin as an output. A low (0) sets the pin as an input.
void digitalDirections(u16 directions)
{
	u08 lower = (u08)directions;
	//set digital0 as output
	if (gbi(lower, 0))
		sbi(DDRB, 4);
	//else set digital0 as input
	else
		cbi(DDRB, 4);

	//set digital1 as output
	if (gbi(lower, 1))
		sbi(DDRB, 7);
	//else set digital1 as input
	else
		cbi(DDRB, 7);

	//set directions for digital2-digital9
	DDRA = (u08)(directions >> 2);
}

//Sets the pullup resistor options for all 10 digital pins, using the lower 10 bits of the argument.
//Bit 0 (LSB) matches digital0 ... bit 9 matches digital9.
//A high bit will enable the pullup, low bit will disable the pullup.
//Pullup option will only be set for a pin if the pin is currently set as an input.
void digitalPullups(u16 pullups)
{
	//digital0
	if (gbi(DDRB, 4) == 0)
	{
		if (gbi(pullups, 0))
			sbi(PORTB, 4); //enable pullup
		else
			cbi(PORTB, 4); //disable pullup
	}
	//digital1
	if (gbi(DDRB, 7) == 0)
	{
		if (gbi(pullups, 1))
			sbi(PORTB, 7); //enable pullup
		else
			cbi(PORTB, 7); //disable pullup
	}
	//digital2-digital9
	u08 i;
	u08 pullups2 = (u08)(pullups >> 2);
	for (i = 0; i < 8; i++)
	{
		if (gbi(DDRA, i) == 0)
		{
			if (gbi(pullups2, i))
				sbi(PORTA, i); //enable pullup
			else
				cbi(PORTA, i); //disable pullup
		}
	}
}

//Returns the value of a digital input.
//num argument can be 0 to 9.
u08 digitalInput(u08 num)
{
	if (num > 1)
		return gbis(PINA, num - 2);
	else if (num == 1)
		return gbis(PINB, 7);
	else if (num == 0)
		return gbis(PINB, 4);
}

//Sets the value of a digital output.
//num argument can be 0 to 9.
void digitalOutput(u08 num, u08 value)
{
	//if the output should be set
	if (value > 0)
	{
		if (num > 1)
			sbi(PORTA, num - 2);
		else if (num == 1)
			sbi(PORTB, 7);
		else if (num == 0)
			sbi(PORTB, 4);
	}
	//else the output should be cleared
	else
	{
		if (num > 1)
			cbi(PORTA, num - 2);
		else if (num == 1)
			cbi(PORTB, 7);
		else if (num == 0)
			cbi(PORTB, 4);
	}
}

//Pass a u16 number(generally easier if expressed in hex by placing 0x before the 4 digits) 
//into digitalOutputs and the lower ten bits will be outputted to the digital port.
void digitalOutputs(u16 outputs)
{
	PORTA = (u08)(outputs>>2);
	PORTB = (PORTB & 0x6F) | ((gbis((u08)outputs, 1) << PB7) | (gbi((u08)outputs, 0) << PB4));
}

//The u16 number returned from digitalInputs is the the result of the 10 inputs being treated as a binary number.
//This number will make more sense if displayed in hex by the printHex_u16 function.
u16 digitalInputs()
{
	u16 result = 0;
	result = (u16)((PINA<<2) | (gbis(PINB, 7)<<1) | gbis(PINB, 4));
	return result;
}


//Toggles the output of a digital pin from 0 to 1 or vice versa.
//Only makes sense to use this with pins set as outputs.
void digitalOutputToggle(u08 num)
{
	//write a 1 to PIN to toggle value of PORT
	if (num > 1)
		PINA = _BV(num - 2);
	else if (num == 1)
		PINB = _BV(7);
	else if (num == 0)
		PINB = _BV(4);
}

//Performs a software reset by setting the smallest watchdog timeout possible and infinite looping
inline void softReset()
{
	wdt_enable(WDTO_15MS);
	while (1)
	{
	}
}

//Since the watchdog timer remains enabled after a resetting,
//this function is added to a section of code that runs very early during startup to disable the watchdog.
void wdt_init() __attribute__((naked)) __attribute__((section(".init3")));
void wdt_init()
{
	MCUSR = 0;
	wdt_disable();
	return;
}
