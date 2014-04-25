#include "lpc11xx_syscon.h"
#include "lpc11Uxx.h"



#define SWDIO	8
#define SWCLK	7
#define JTAGSWDSEQ 0x79E7//0xE79E
#define IDCODE 0xA5 // 1010 0101
// this was done through remote desktop connection :)

void initGpio()
{
	/* Enable GPIO block clock */
		SYSCON_AHBPeriphClockCmd(SYSCON_AHBPeriph_GPIO, ENABLE);

		/* If PIO2_0 had been set to other function, set it to PIO function.
		 * NOTE: Component IOCON should be checked */
		//IOCON_SetPinFunc(IOCON_PIO2_0, PIO2_0_FUN_PIO);
}


void setGpioAsOutput(int pin)
{
	LPC_GPIO->DIR[0] |= (1<<pin);
}

void setGpioAsInput(int pin)
{
	LPC_GPIO->DIR[0] &= ~((1<<pin));
}

void setGpio(int pin)
{
	LPC_GPIO->SET[0] = (1<<pin);
}

void clrGpio(int pin)
{
	LPC_GPIO->CLR[0] = (1<<pin);
}

void notGpio(int pin)
{
	LPC_GPIO->NOT[0] = (1<<pin);
}


void delay()
{
	int i,j;

	for(i=0; i<200; i++) {
				for(j=0; j<1000; j++) {
				}
			}
}

void clockGen(int cycles)
{
	while(cycles > 0)
	{
	clrGpio(SWCLK);
	delay();
	setGpio(SWCLK);
	delay();
	clrGpio(SWCLK);
	cycles--;
	}
}

/***************************SWD Functions****************************************************/
void swdLineReset()
{
	int count = 50; // need 60 falling transitions
	int i;

	setGpio(SWDIO);
	clrGpio(SWCLK);
	for(i=0; i< count; i++)
	{
		clrGpio(SWCLK);
		delay();
		setGpio(SWCLK);
		delay();
	}
	// clk should be high in idle phases
}


int swdReadIdent()
{
	setGpioAsOutput(SWCLK);
	setGpioAsOutput(SWDIO);

	// Start Bit
	setGpio(SWDIO);
	clrGpio(SWCLK);
	delay();
	setGpio(SWCLK); // bit locked
	delay();

	// APnDP = 0
	clrGpio(SWCLK);
	clrGpio(SWDIO);
	delay();
	setGpio(SWCLK);
	delay();

	// R = 1
	clrGpio(SWCLK);
	setGpio(SWDIO);
	delay();
	setGpio(SWCLK);
	delay();

	// A[2:3] = 0
	clrGpio(SWCLK);
	clrGpio(SWDIO);
	delay();
	setGpio(SWCLK);
	delay();

	// A[2:3] = 0
	clrGpio(SWCLK);
	clrGpio(SWDIO);
	delay();
	setGpio(SWCLK);
	delay();

	// parity =1.
	clrGpio(SWCLK);
	setGpio(SWDIO);
	delay();
	setGpio(SWCLK);
	delay();

	//stop  = 0
	clrGpio(SWCLK);
	clrGpio(SWDIO);
	delay();
	setGpio(SWCLK);
	delay();

	// park bit
	clrGpio(SWCLK);
	setGpio(SWDIO);
	delay();
	setGpio(SWCLK);
	delay();


    clockGen(1); // 1 TRN

    // acking
    clrGpio(SWDIO);
    setGpioAsInput(SWDIO);
    int ack = 0;

    // 1st ack bit
    clrGpio(SWCLK);
    delay();
    setGpio(SWCLK);
    ack= LPC_GPIO->B0[SWDIO];
    delay();

    // 2nd ack bit
    clrGpio(SWCLK);
    delay();
    setGpio(SWCLK);
    ack= LPC_GPIO->B0[SWDIO];
    delay();

    // 3rd ack bit
    clrGpio(SWCLK);
    delay();
    setGpio(SWCLK);
    ack= LPC_GPIO->B0[SWDIO];
    delay();

    return 0;
}




void swdSendData(int data)
{
	// must transmit 0xE79E LSB first
	int masker = 0xF;
	int byteval,bitval;
	int i,j;

	for (i=0; i<4; i++)
	{
		byteval = data & masker;


		for(j=0; j<4; j++)
		{
			bitval = byteval & 0x1;
			byteval = byteval >> 1;

			if(bitval == 1)
			{
				clrGpio(SWCLK);
				delay();
				setGpio(SWDIO);
				delay();
				setGpio(SWCLK);
				delay();
			}
			else
			{
				clrGpio(SWCLK);
				delay();
				clrGpio(SWDIO);
				delay();
				setGpio(SWCLK);
				delay();
			}
		}

	setGpio(SWCLK);

	data  = data >> 4;
	}
}




void swdRead()
{
	int ack = 0;
	int readData = 0;
	int i,byteval= 0;
	int parity = 0;

	swdSendData(0x00);

	setGpio(SWDIO); //

	//set SWDIO in repeater mode so that it is prevented from floating

	//LPC_IOCON->PIO0_3 |= 0x00000018; // enable

	setGpioAsInput(SWDIO);
	clockGen(1); // 1 TRN


	// read ACK
	for( i=0; i<3; i++)
	{
		ack = ack << 1;
		clrGpio(SWCLK);
		delay();
		setGpio(SWCLK);
		delay();
		delay();
		byteval =  (LPC_GPIO->PIN[0] & (1<<SWDIO)) ? 1 : 0;
		ack |= byteval ; 		// sample SWDIO line

	}

	clockGen(2);

	// read Data
	for( i=0; i<32; i++)
	{
		clrGpio(SWCLK);
		delay();
		setGpio(SWCLK);
		delay();
		byteval =  (LPC_GPIO->PIN[0] & (1<<SWDIO));
		readData |= byteval >> SWDIO; 		// sample SWDIO line
		readData = readData << 1;
	}

	//parity
	clrGpio(SWCLK);
	delay();
	setGpio(SWCLK);
	delay();
	byteval =  (LPC_GPIO->PIN[0] & (1<<SWDIO));
	parity = byteval >> SWDIO; 		// sample SWDIO line

}


void clockout()
{
	LPC_IOCON->PIO0_1 = 0b01;

	LPC_SYSCON->CLKOUTSEL = 0x01; // select main clock as clockout source
	LPC_SYSCON->CLKOUTDIV = 0x01;

	LPC_SYSCON->CLKOUTUEN = 0x01;		/* Toggle update register once */
	LPC_SYSCON->CLKOUTUEN = 0x00;
	LPC_SYSCON->CLKOUTUEN = 0x01;

	while ( !(LPC_SYSCON->CLKOUTUEN & 0x01) );	/* Wait until updated */
}



int main(void)
{
	clockout();

    //automatically added by CoIDE
	initGpio();

	setGpioAsOutput(SWDIO);
	setGpioAsOutput(SWCLK);



	swdLineReset();
	swdReadIdent();






	while(1)
    {
		setGpio(SWCLK);
		delay();
		clrGpio(SWCLK);
		delay();
    }
}

