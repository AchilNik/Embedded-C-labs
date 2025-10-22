#include <avr/io.h>
#include <avr/interrupt.h>
int intflags;
int LEDS;
int rising = 0;


int main(void)
{
	//PORTD setup
	PORTD.DIR = 0b00000111;	//set PORTD pins direction
	PORTD.OUT = 0b00000111;	//all LED's initially turned off
	
	//PORTF setup
	PORTF.PIN5CTRL = PORT_PULLUPEN_bm | PORT_ISC_BOTHEDGES_gc;	//enables interrupts from pin5 PORTF
	PORTF.PIN6CTRL = PORT_PULLUPEN_bm | PORT_ISC_BOTHEDGES_gc;	//enables interrupts from pin6 PORTF
	
	//ADC setup
	ADC0.CTRLA |= ADC_RESSEL_10BIT_gc;	//set conversion resolution to 10 bits
	ADC0.CTRLA |= ADC_FREERUN_bm;	//enable free running mode
	ADC0.CTRLA |= ADC_ENABLE_bm;	//enable ADC0
	ADC0.MUXPOS |= ADC_MUXPOS_AIN7_gc;	//set position bit
	ADC0.DBGCTRL |= ADC_DBGRUN_bm;	//enable debug mode
	ADC0.WINLT |= 19;	//set low threshold to 19
	ADC0.WINHT	|= 24;	//set high threshold to 24
	ADC0.INTCTRL |= ADC_WCMP_bm;	//enable interrupts from comparator mode
	ADC0.CTRLE |= ADC_WINCM_OUTSIDE_gc;	//enables interrupts when RES < WINLT or RES > WINHT
	ADC0.COMMAND |= ADC_STCONV_bm;	//ADC0 starts converting
	sei();
    while (1) 
    {
    }
}

ISR(ADC0_WCOMP_vect)	//handles interrupts from ADC0 comparator mode
{
	cli();
	if (ADC0.RES < ADC0.WINLT)	//if humidity is below WINLT
	{
		PORTD.OUT = ~PIN0_bm;	//LED for water is turned on
	}
	else if (ADC0.RES > ADC0.WINHT)	//if humidity is above WINHT
	{
		PORTD.OUT = ~PIN1_bm;	//LED for fan usage is turned on
	}
	ADC0.INTCTRL &= ~ADC_WCMP_bm;	//disable ADC0 until the emergency is taken care off
	intflags = ADC0.INTFLAGS;	//reset flags
	ADC0.INTFLAGS = intflags;
	sei();
}

ISR(PORTF_PORT_vect)	//handles interrupts from PORTF when buttons are pushed
{
	cli();
	if (!(PORTD.OUT & PIN0_bm) && (PORTF.INTFLAGS & PIN5_bm) && !(PORTF.INTFLAGS & PIN6_bm))	//if LED for low humidity is on and ONLY the water button was pushed
	{
		TCA0.SINGLE.CNT = 0;	//set counter to 0
		TCA0.SINGLE.CMP0 = ADC0.WINLT - ADC0.RES;	//set time for watering
		TCA0.SINGLE.CTRLB = 0;	//set normal mode on TCA0
		TCA0.SINGLE.CTRLA = TCA_SINGLE_CLKSEL_DIV1024_gc;	//set prescaler to 1024
		TCA0.SINGLE.INTCTRL = TCA_SINGLE_CMP0_bm;	//enables interrupts for CMP0
		TCA0.SINGLE.CTRLA |= TCA_SINGLE_ENABLE_bm;	//TCA0 starts counting
	}
	else if (!(PORTD.OUT & PIN1_bm) && (PORTF.INTFLAGS & PIN6_bm) && !(PORTF.INTFLAGS & PIN5_bm))	//if LED for high humidity is on and ONLY the fan button was pushed
	{
		TCA0.SINGLE.CTRLA = TCA_SINGLE_CLKSEL_DIV1024_gc;	//set prescaler to 1024
		TCA0.SINGLE.PER = 19;	//set PWM period to 1ms
		TCA0.SINGLE.CNT = 0;	//set counter to 0
		TCA0.SINGLE.CMP0 = 0.5*19;	//set PWM duty cycle to 50%
		TCA0.SINGLE.CTRLB = TCA_SINGLE_WGMODE_SINGLESLOPE_gc;	//select single slope PWM
		TCA0.SINGLE.INTCTRL |= TCA_SINGLE_OVF_bm;	//enable interrupt for overflow to indicate rising edge
		TCA0.SINGLE.CTRLA |= TCA_SINGLE_ENABLE_bm;	//TCA0 starts counting
	}
	else  //in any other case the wrong button was pushed
	{
		LEDS = PORTD.OUT;	//store initial LED indication
		PORTD.OUT = 0b00000000;	//turn on warning indication
		PORTD.OUT = LEDS;	//turn on again the initial indication
	}
	intflags = PORTF.INTFLAGS;	//reset flags
	PORTF.INTFLAGS = intflags;
	sei();
}

ISR(TCA0_CMP0_vect)	//handles interrupts for water system
{
	cli();
	TCA0.SINGLE.CTRLA &= ~TCA_SINGLE_ENABLE_bm;	//disables TCA0
	PORTD.OUTSET = PIN0_bm;	//LED for water is turned off
	ADC0.INTCTRL |= ADC_WCMP_bm;	//enable interrupts from comparator mode again,ready for next conversion
	TCA0.SINGLE.INTCTRL &= ~TCA_SINGLE_CMP0_bm;	//disables interrupt for CMP0
	intflags = TCA0.SINGLE.INTFLAGS;	//reset flags
	TCA0.SINGLE.INTFLAGS = intflags;
	sei();
}

ISR(TCA0_OVF_vect)	//handles interrupts for fan system
{
	cli();
	rising++ ;	//rising edge counter
	PORTD.OUT ^= PIN2_bm;	//toggles fan LED
	if (rising == 4)	//if this is the 4th rising edge
	{
		PORTD.OUTSET = PIN1_bm | PIN2_bm;	//LED for fan indication and fan system are turned off
		TCA0.SINGLE.INTCTRL &= ~TCA_SINGLE_OVF_bm;	//disables interrupt for overflow to indicate rising edge
		TCA0.SINGLE.CTRLA &= ~TCA_SINGLE_ENABLE_bm;	//disables TCA0
		ADC0.INTCTRL |= ADC_WCMP_bm;	//enable interrupts from comparator mode again,ready for next conversion
		rising = 0;	//resets counter for rising edges
	}
	intflags = TCA0.SINGLE.INTFLAGS;	//reset flags
	TCA0.SINGLE.INTFLAGS = intflags;
	sei();
}
