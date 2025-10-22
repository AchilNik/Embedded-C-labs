#include <avr/io.h>
#include <avr/interrupt.h>
int intflags;
int degrees = 0;
int reverse_mode = 0;

int main(void)
{
	//LED settings
	PORTD.DIR |= PIN0_bm | PIN1_bm | PIN2_bm;	//output pins
	PORTD.OUT = 0b00000101;		//device starts, forward LED is on
	
	//reverse operation button settings
	PORTF.PIN5CTRL |= PORT_PULLUPEN_bm | PORT_ISC_BOTHEDGES_gc;		//enable interrupts from PORTF pin5
	
	//TCA0 settings
	TCA0.SPLIT.CTRLD = TCA_SPLIT_SPLITM_bm;	//enable split mode for TCA0
	TCA0.SPLIT.CTRLA = TCA_SPLIT_CLKSEL_DIV1024_gc;	//set prescaler to 1024
	TCA0.SPLIT.LCMP0 = 19;	//1ms delay to start going forward again, 1ms before interrupt happens from LCMP0
	TCA0.SPLIT.LCMP1 = 38;	//2ms going forward time, 2ms before interrupt happens from LCMP1
	
	//ADC settings
	ADC0.CTRLA |= ADC_RESSEL_10BIT_gc|ADC_ENABLE_bm;	//set 10 bit resolution, enable ADC
	ADC0.MUXPOS |= ADC_MUXPOS_AIN7_gc;	//set ADC position bit
	ADC0.WINLT |= 10;	//set window comparison low threshold to 10 mm for front space
	ADC0.WINHT|= 20;		//set window comparison high threshold to 20 mm side space
	ADC0.INTCTRL |= ADC_WCMP_bm;	//enable interrupts from WCM
	ADC0.DBGCTRL |= ADC_DBGRUN_bm;	//enable debug mode
	ADC0.CTRLE = ADC_WINCM1_bm;		//interrupt when RES > WINHT side check
	sei();	//enable interrupts
	ADC0.COMMAND |= ADC_STCONV_bm;	//ADC makes first conversion for side space
	
	TCA0.SPLIT.INTCTRL |= TCA_SPLIT_LCMP0_bm;	//enable first interrupt for forward control
	TCA0.SPLIT.LCNT = 0;	//set counter to 0 to time interrupt for forward control
	TCA0.SPLIT.CTRLA |= TCA_SPLIT_ENABLE_bm;	//start timer
	
	while (!(reverse_mode == 0 && degrees >= 360) && !(reverse_mode == 1 && degrees <0))	//device working conditions
	{
		
	}
	
	cli();
	//program terminated
}

ISR(PORTF_PORT_vect)	//reverse mode ISR
{
	cli();
	enable_reverse();
	intflags = PORTF.INTFLAGS;	//clear flags
	PORTF.INTFLAGS = intflags;
	sei();
}


ISR(TCA0_HUNF_vect)	//turn off LED's for reverse mode
{
	cli();
	intflags = TCA0.SPLIT.INTFLAGS;		//clear flags
	TCA0.SPLIT.INTFLAGS = intflags;
	reverse_indicator();
	sei();
}

ISR(TCA0_LCMP0_vect)	//going forward ISR 
{
	cli();	//disable interrupts
	forward_setup();
	intflags = TCA0_SPLIT_INTFLAGS;		//clear flags
	TCA0_SPLIT_INTFLAGS = intflags;
	sei();	//enable interrupts
}

ISR(TCA0_LCMP1_vect)		//going side ISR
{
	cli();	//disable interrupts
	side_setup();	
	intflags = TCA0.SPLIT.INTFLAGS;		//clear flags
	TCA0.SPLIT.INTFLAGS = intflags;
	sei();	//enable interrupts
}


ISR(ADC0_WCOMP_vect)	//changing direction ISR
{
	cli();	//disable interrupts
	movement();	
	intflags = ADC0.INTFLAGS;	//clear flags
	ADC0.INTFLAGS = intflags;
	sei();	//enable interrupts
}


void forward_setup()		//changes settings for forward movement
{
	TCA0.SPLIT.INTCTRL &= ~TCA_SPLIT_LCMP0_bm;	//disables interrupts from LCMP0
	ADC0.CTRLE = ADC_WINCM0_bm;		//interrupt when RES < WINLT front check
	ADC0.CTRLA |= ADC_FREERUN_bm;	//enables free run mode
	ADC0.COMMAND = ADC_STCONV_bm;	//start free run conversion
	TCA0.SPLIT.INTCTRL |= TCA_SPLIT_LCMP1_bm;	//enables interrupts from LCMP1
	TCA0.SPLIT.LCNT = 0;	//set counter to 0 to time interrupt for side control	
}


void side_setup()	//changes settings for side movement
{
	ADC0.CTRLA &= ~ADC_FREERUN_bm;	//disables free run mode
	TCA0.SPLIT.INTCTRL &= ~TCA_SPLIT_LCMP1_bm;	//disables interrupts from LCMP1
	ADC0.CTRLE = ADC_WINCM1_bm;		//interrupt when RES > WINHT side check
	ADC0.COMMAND = ADC_STCONV_bm;	//start single conversion
	TCA0.SPLIT.INTCTRL |= TCA_SPLIT_LCMP0_bm;	//enables interrupts from LCMP0
	TCA0.SPLIT.LCNT = 0;	//set counter to 0 to time interrupt for forward control	
}


void movement()	//sets device movement and degrees made
{
	if (reverse_mode == 0)	//reverse mode disabled
	{
		if (ADC0.RES < ADC0.WINLT)	//if forward space <  low threshold
		{
			degrees += 90;	//left corner
			PORTD.OUT = 0b00000011;		//left LED on, device turns left
			PORTD.OUT = 0b00000101;		//forward LED on again, device going forward
		}
		else if (ADC0.RES > ADC0.WINHT)		//if side space > high threshold
		{
			degrees -= 90;	//right corner
			PORTD.OUT =0b00000110;		//right LED on, device turns right
			PORTD.OUT =0b00000101;		//forward LED on again, device going forward 
		}
	}
	else if (reverse_mode == 1)	//reverse mode enabled
	{
		if (ADC0.RES < ADC0.WINLT)	//if forward space <  low threshold
		{
			degrees -= 90;		//right corner
			PORTD.OUT = 0b00000110;	//right LED on, device turns right
			PORTD.OUT = 0b00000101;	//forward LED on again, device going forward
		}
		else if (ADC0.RES > ADC0.WINHT)		//if side space > high threshold
		{
			degrees += 90;		//left corner
			PORTD.OUT =0b00000011;		//left LED on, device turns left
			PORTD.OUT =0b00000101;		//forward LED on again, device going forward
		}
	}
}


void enable_reverse()		//enables reverse mode
{
	reverse_mode = 1;
	PORTD.OUT = 0b00000000;	//all LED's turned on for the reverse mode indication
	TCA0.SPLIT.INTCTRL |= TCA_SPLIT_HUNF_bm;	//enables interrupt from HUNF to turn off reverse mode LED's
	TCA0.SPLIT.HCNT = 236;	//set counter to delay 1ms the reverse mode LED's
	ADC0.INTCTRL &= ~ADC_WCMP_bm;	//disable interrupts from window compare
	TCA0.SPLIT.INTCTRL &= ~(TCA_SPLIT_LCMP0_bm | TCA_SPLIT_LCMP1_bm);	//disables interrupts from LCMP0 and LCMP1, device stops and turn
}


void reverse_indicator()	//turns reverse mode indicator off
{
{
	TCA0.SPLIT.INTCTRL &= ~TCA_SPLIT_HUNF_bm;	//disables interrupt from HUNF
	ADC0.INTCTRL |= ADC_WCMP_bm;	//enable interrupts from window compare, device moves again
	TCA0.SPLIT.INTCTRL |= TCA_SPLIT_LCMP0_bm;	//enables interrupts from LCMP0, device moving again
	TCA0.SPLIT.LCNT = 0;	//set counter to 0 to time interrupt for forward control
	PORTD.OUT = 0b00000101;		//LED's set again for forward movement
}
