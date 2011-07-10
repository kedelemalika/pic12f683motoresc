/*  ************** Program I/O map *************
	GP0 = clockwise PWM signal on MC33886
	GP1 = used only for ICSP
	GP2 = enable/disable
	GP3 = used only for ICSP
	GP4 = speed input as pulse width of 1ms wide to 2 ms wide occuring in 20 ms intervals
	GP5 = counterclockwise PWM signal on MC33886
	************** End Program I/O map **********

  	*************** Program States **************
	1)  Zero:  default state
		a)  enable high
		a)  output zero speed (no PWM signal) until a command is given
	2)  Clockwise
		a)  enable high
		b)  all speed commands are outputted on the clockwise pin for clockwise motor speed control
	3)  Counter-clockwise
		a)  enable high
		b)  all speed commands are outputted on the counter-clockwise pin for counter-clockwise motor speed control
    ************** End Program States ***********

	*************** Program pseudo code steps ***
	1)  Turn on enable bit		
	2)	Read in pulse input signal width of 1ms to 2ms.
		a)  If pulse is < 1.49ms then set state to counterclockwise
		b)	If pulse is > 1.51ms then set state to clockwise
		c)	If pulse is > 1.48ms and < 1.52ms then set state to zero.  Motor should not move  

	3)  Scale the pulse input signal for use as a speed command
	4)	Output speed command based upon select statement
    ********** End Program pseudo code steps *****
*/

#include <pic.h>

__CONFIG(INTIO & WDTDIS & PWRTDIS & MCLRDIS & UNPROTECT & BORDIS & IESODIS & FCMDIS);

static void interrupt isr(void); //interrupt code


//defines
#define PWM_CW		GPIO0
#define ENABLE  	GPIO2
#define SPEED_CMD	GPIO4
#define PWM_CCW		GPIO5

//255 = 2ms   191 = 1.5ms  127 = 1ms
#define ZERO_MID	191 
#define ZERO_LOW	ZERO_MID - 2
#define ZERO_HI		ZERO_MID + 2

typedef enum DIR_STATE {DIR_ZERO = 0, DIR_CW = 1, DIR_CCW = 2} DIR_STATE;

DIR_STATE d = DIR_ZERO;

static unsigned char bitTime = ZERO_MID;

void init_interrupts(void){

//	Timer 0 interrupt enable 
	//  clock frequency/4/prescaler/max count = interrupt frequency in hertz
	//  example:  8,000,000/4/16/256 = 488.28hz
	//  duration between interrupts:  1/hz or in this case 1/488.28hz = 0.002048 or 2.048msec
	//  see page 232 of "Designing Embedded Systems with PIC Microcontrollers" by Tim Wilmshurst
	T0CS = 0; 	// timer0 clock source set to internal clock
	PSA = 0; 	// prescaler is assigned to timer0
	PS2 = 0;	//prescaler set to 1:16
	PS1 = 1;
	PS0 = 1;
	
	IOC = 0x10;	 //interrupt on change on the pulse input
	T0IE = 1;	//enable timer0 interrupt	
	TMR1IE = 1;	//enable timer1 interrupt
	TMR2IE = 1;	//enable timer2 interrupt
	PEIE = 1;	// enable peripheral interrupts
	GPIE = 1;   // enable detecting on-change interrupts
	GIE =  1; 	// enable global interrupts
	
}

void config_io(void){

	//all pins are digital.  Must disable analog input as that is the default
	ANSEL &= 0xF0;  //sets all four low bits low while not affecting the highest four bits
	
	//comparators must be disabled for comparator capable pins to be digital
	CMCON0 |= 0x07;  //sets three low bits high while not affecting the highest five bits 

	TRISIO0 = 0; //clockwise PWM signal	
	TRISIO2 = 0; //MC33886 enable command received from PIC
	TRISIO4 = 1; //pulse input indicating speed desired
	TRISIO5 = 0; //counter-clockwise PWM signal		

	WPU = 0x10;  //put a weak pullup on the pulsin signal	
}

static void interrupt
isr(void)			// Here be interrupt function - the name is unimportant.
{

	// ********** setup for SPEED_CMD changed
	if((GPIE) && (GPIF)) 
	{	
		if(SPEED_CMD)
		{
			TMR0=0;  // high pulse now we measure the width but first reset the timer
		}
		else
		{
			bitTime = TMR0; //pulse went high to low so get the time that it was high
			if (bitTime < ZERO_LOW)
			{
				 d = DIR_CCW;
			}
			else if (bitTime > ZERO_HI)
			{
				d = DIR_CW;
			}
			else
			{
				d = DIR_ZERO;
			}
		}
		GPIE = 0;		// clear the interrupt flag
	}

	// ********** setup for timer 0
	if((T0IE) && (T0IF)) 
	{	
	
		T0IF = 0;			// clear the interrupt flag
	}
}


void main(void){

	//sets the clock for 8 mhz from the default 4 mhz
	OSCCON |= 0x70;
	
	config_io();
	init_interrupts();

	ENABLE = 1;
	
	for(;;)
	{	
		//apparently the interrupt can properly read the speed command now
		//However, the PWM for the forward and reverse pin will need to be done
		//in theory.  The PWM signal is the same and thus it only needs one direction, but possibly
		//changing the PWM from pin to pin - not generating it once for each pin.	
		switch(d)
		{
			case DIR_ZERO:
				break;
			case DIR_CW:
				break;
			case DIR_CCW:
				break;
			default:
				break;
		} // end switch	
	} //end for
}  //end main



