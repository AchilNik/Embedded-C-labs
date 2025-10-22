#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/delay.h>
#define del 10

	int main(void)
	{
		PORTD.DIR |= 0b00000111; //PIN 0, 1, 2 as output
		PORTD.OUT |= 0b00000111; //Turn all floor LEDS off. Ground floor start
		PORTF.PIN5CTRL |= PORT_PULLUPEN_bm | PORT_ISC_BOTHEDGES_gc; // enable switch PIN5 with both edges sense
		PORTF.PIN6CTRL |= PORT_PULLUPEN_bm | PORT_ISC_BOTHEDGES_gc; // enable switch PIN6 with both edges sense
		
		sei(); //enable interrupts
		while (1)
		{
			
		}
	}

	ISR(PORTF_PORT_vect)
	{
		cli();
		if ((PORTF.INTFLAGS & PIN5_bm) && (PORTF.INTFLAGS & PIN6_bm)) //if the switches are pressed simultaneously (error)
		{
			PORTD.OUTCLR = PIN0_bm; //turn on the error LED
			_delay_ms(del); //wait 10ms
			PORTD.OUTSET = PIN0_bm; //turn off the error LED
		}
		else if (PORTF.INTFLAGS & PIN5_bm) //elevator going down, switch 5 was activated
		{
			if (PORTD.OUT == 0b00000001) PORTD.OUTSET = PIN2_bm; //if we are on the 2nd floor, go to 1st floor
			else if (PORTD.OUT == 0b00000101) PORTD.OUTSET = PIN1_bm; //if we are on the 1st floor, go to ground floor
		}
		else if (PORTF.INTFLAGS & PIN6_bm) //elevator going up, switch 6 was activated
		{
			if (PORTD.OUT == 0b00000111) PORTD.OUTCLR = PIN1_bm; //if we are on the ground floor, go to 1st floor
			else if (PORTD.OUT == 0b00000101) PORTD.OUTCLR = PIN2_bm; //if we are on 1st floor,  go to 2nd floor
		}
		int x = PORTF.INTFLAGS; //clear the interrupt flags
		PORTF.INTFLAGS = x;
sei();
	}
