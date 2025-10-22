#include <avr/io.h>
#include <avr/interrupt.h>
int pressed = 0;
int intflags;

int main()
{
	//PORTD setup
	PORTD.DIR = PIN0_bm | PIN1_bm;	//set output LED's
	PORTD.OUT = 0b00000011;	//set initial state, fan disabled both LED's off
	
	//PORTF setup
	PORTF.PIN6CTRL |= PORT_PULLUPEN_bm | PORT_ISC_BOTHEDGES_gc;	//enables interrupts by pushing button
	
	//TCA0 initial setup
	TCA0.SPLIT.CTRLD = TCA_SPLIT_SPLITM_bm;	//enables split mode
	TCA0.SPLIT.CTRLA = TCA_SPLIT_CLKSEL_DIV1024_gc;	//set prescaler to 1024
	TCA0.SPLIT.HPER = 39;	//base period set to 2ms, always the same
	TCA0.SPLIT.HCMP0 = 0.4*39;	//sets duty cycle for base movement, always the same
	TCA0.SPLIT.INTCTRL |= TCA_SPLIT_HUNF_bm | TCA_SPLIT_LUNF_bm;	//enables interrupts for HUNF and LUNF
	sei();	//enables interrupts
	while(1)
	{
		
	}
	cli();	//disables interrupts
}

ISR(TCA0_LUNF_vect)	//manages fan's blades rotation
{
	cli();	//disable interrupts
	TCA0.SPLIT.INTFLAGS = TCA_SPLIT_LUNF_bm;	//clear interrupt flags
	PORTD.OUT ^= PIN0_bm;	//toggles PORTD LED1, base LED
	sei();	//enables interrupts
}

ISR(TCA0_HUNF_vect)	//manages fan's base rotation
{
	cli();	//disables interrupts
	TCA0.SPLIT.INTFLAGS = TCA_SPLIT_HUNF_bm;	//clear interrupt flags
	PORTD.OUT ^= PIN1_bm;	//toggles PORTD LED0, fan LED
	sei();	//enables interrupts
}

ISR(PORTF_PORT_vect)	//manages button's pressing
{
	cli();	//disables interrupts
	intflags = PORTF.INTFLAGS;
	PORTF.INTFLAGS = intflags;	//clear interrupt flags
	mode_selection();
	sei();	//enables interrupts
}

void mode_selection()	//function used for mode selection 
{
	if (pressed == 0)	//first time button pressed
	{
		pressed = 1;	//button pressed one time
		TCA0.SPLIT.LPER = 39/2;	//set blade period to 1ms
		TCA0.SPLIT.LCMP0 = 0.6*39/2;	//set blade duty cycle to 60% of period
		TCA0.SPLIT.CTRLA |= TCA_SPLIT_ENABLE_bm;	//enables TCA0
	}
	
	else if (pressed == 1)	//second time button pressed
	{
		pressed = 2;	//button pressed two times
		TCA0.SPLIT.LCNT = 0;	//set low counter to 0 again for restart
		TCA0.SPLIT.HCNT = 0;	//set high counter to 0 again for restart
		TCA0.SPLIT.LPER = 39/4;	//set blade period cut to half, 0.5ms
		TCA0.SPLIT.LCMP0 = 0.5*39/4;	//set new duty cycle for blades, 50% of new period
	}
	
	else if (pressed == 2)	//third time button pressed
	{
		pressed = 0;	//button pressed 3 times
		TCA0.SPLIT.CTRLA &= ~TCA_SPLIT_ENABLE_bm;	//disables TCA0, fan turned off
		TCA0.SPLIT.LCNT = 0;	//set low counter to 0 to prepare for the next use of fan
		TCA0.SPLIT.HCNT = 0;	//set high counter to 0 to prepare for the next use of fan
	}
}
