#include <avr/io.h>
#include <avr/interrupt.h>
char priority = 'c';	//cars have priority, global variable to access it from ISRs
int intflags = 0;	//variable for reseting intflags

int main(void)
{
    PORTD.DIR |= PIN0_bm | PIN1_bm | PIN2_bm;	//set output pins
	TCA0.SPLIT.CTRLD = TCA_SPLIT_SPLITM_bm;		//split mode enabled
	TCA0.SPLIT.CTRLA = TCA_SPLIT_CLKSEL_DIV1024_gc;	//set prescaler 1024
	PORTF.PIN5CTRL |= PORT_PULLUPEN_bm | PORT_ISC_BOTHEDGES_gc;		//enable interrupts by pushing pedestrian button
	TCA0.SPLIT.INTCTRL |= TCA_SPLIT_HUNF_bm;	//enables interrupts from HUNF, tram passing
	TCA0.SPLIT.LCMP0 = 19;	//pedestrian green light time --> 1ms
	TCA0.SPLIT.LCMP1 = 19;	//pedestrian button enable time --> 1ms after priority goes back to cars again
	TCA0.SPLIT.HCNT = 41;	//tram passing time, HCNT counting every tick until 255 --> 11ms between every tram pass
	sei();	//enable interrupts
	TCA0.SPLIT.CTRLA |= TCA_SPLIT_ENABLE_bm;  //start timer

    while (1) 
    {
		if (priority == 'c')	//cars have priority
		{
			PORTD.OUT = 0b00000110; //green car light, red pedestrian and tram
		}
		
		else if (priority == 'p')	//pedestrians have priority
		{
			TCA0.SPLIT.INTCTRL &= ~TCA_SPLIT_HUNF_bm;	//disables interrupts from tram while pedestrians have priority
			PORTF.PIN5CTRL = 0;		//disable interrupts from pedestrian button
			PORTD.OUT = 0b00000011;		//red car and tram light, green pedestrian light
		}
		
		else if (priority == 't')
		{
			PORTD.OUT = 0b00000001;		//tram green light, red for cars and pedestrians
			TCA0.SPLIT.HCNT = 41;	//tram passing time set again
		}
    }
}

ISR(PORTF_PORT_vect)	//pedestrian button was pushed
{
	cli();	//disable interrupts
	priority = 'p';		//priority to pedestrians
	intflags = PORTF.INTFLAGS;	//clear vector flags
	PORTF.INTFLAGS = intflags;
	TCA0.SPLIT.LCNT = 0;	//resets counter to time T2
	TCA0.SPLIT.INTCTRL |= TCA_SPLIT_LCMP0_bm;	//enables interrupts from LCMP0
	TCA0.SPLIT.INTCTRL &= ~TCA_SPLIT_HUNF_bm;	//disables interrupts from HUNF,no tram passing
	sei();	//enable interrupts
}

ISR(TCA0_LCMP0_vect)		//priority goes back to cars
{
	cli();	//disable interrupts
	priority = 'c';		//priority to cars
	TCA0.SPLIT.INTFLAGS = TCA_SPLIT_LCMP0_bm;	//clear interrupt flag for LCMP0
	TCA0.SPLIT.LCNT = 0;	//LCNT zeroed again to time button enable
	TCA0.SPLIT.INTCTRL &= ~TCA_SPLIT_LCMP0_bm;	//disable LCMP0 interrupts
	TCA0.SPLIT.INTCTRL |= TCA_SPLIT_LCMP1_bm;	//enables LCMP1 interrupts
	TCA0.SPLIT.INTCTRL |= TCA_SPLIT_HUNF_bm;	//enables HUNF interrupts before priority goes back to cars
	sei();	//enable interrupts
}

ISR(TCA0_LCMP1_vect)		//pedestrian button can be pressed again
{
	cli();	//disable interrupts
	TCA0.SPLIT.INTFLAGS = TCA_SPLIT_LCMP1_bm;	//clear interrupt flag for LCMP1
	PORTF.PIN5CTRL |= PORT_PULLUPEN_bm | PORT_ISC_BOTHEDGES_gc;		//enable interrupts by pushing button on PIN5, pedestrian
	TCA0.SPLIT.INTCTRL &= ~TCA_SPLIT_LCMP1_bm;	//disables interrupts from LCMP0 and LCMP1	
	sei();	//enable interrupts
}

ISR(TCA0_HUNF_vect)		//priority goes to tram
{
	cli();
	priority = 't';		//priority to tram
	TCA0.SPLIT.INTFLAGS = TCA_SPLIT_HUNF_bm;	//clear interrupt flag for HUNF only
	TCA0.SPLIT.INTCTRL &= ~TCA_SPLIT_HUNF_bm;	//disables interrupt from tram to be enabled again when priority goes to cars
	TCA0.SPLIT.INTCTRL |= TCA_SPLIT_LCMP0_bm;	//enables interrupts from LCNT to sen priority back to cars after T2
	TCA0.SPLIT.LCNT = 0;	//reset timer to make interrupt after T2
	sei();	//enable interrupts
}
