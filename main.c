// O'Dale Cotterell 10/29/2015
// HVAC Controller
// PORT C7 - PORT C4 output (LCD data bus); PORT C2 – PORT C0 output (LCD control signals)
// PORTB7 output (LCD Chip Select Bit – active Low)
// Includes LCD initialization routine

#include <hidef.h>      /* common defines and macros */
#include "derivative.h"      /* derivative-specific definitions */


//globals - 4-bit initialization sequence for LCD  -  data: PC7,6,5,4  E: PC2  R/W~: PC1 RS: PC0
const unsigned char cInit_commands[20] = {0x30,0x30,0x30,0x20,0x20,0x90,0x10,0x50,0x70,0x80,0x50,0xE0,
                                          0x60,0xA0,0x00,0xE0,0x00,0x10,0x00,0x60};
										  
const unsigned char cMessage[3][17] = 
  { {' ','W','e','l','c','o','m','e',' ','t','o',' ','t','h','e',' '}, 
     {' ',' ','H','V','A','C','-','S','y','s','t','e','m',' ',' ',' '},
     {' ',' ',' ','L','o','a','d','i','n','g','.','.','.',' ',' ',' '}};//data in Program Flash

const unsigned char cMessage2[3][17] = 
  { {'M','o','d','e',':',' ','O','F','F',' ',' ',' ',' ',' ',' ',' '}, 
     {'T','e','m','p',':',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' '},
     {'1','(','A','C',')',' ','-',' ','2','(','H','E','A','T',')',' '}};//data in Program Flash


//Keypad data
const unsigned char cRow[5] =  // One dimension array for rows
     { 
        0x00,                     // All rows logic Low
        0xE0,                     // Only Row 1 logic Low
        0xD0,                     // Only Row 2 logic Low
        0xB0,                     // Only Row 3 logic Low
        0x70                      // Only Row 4 logic Low
     };  
	 
unsigned char cCol[5];         // One dimension array for Columns
unsigned char cValue, cValues;          // Variable used in reading PORT A
unsigned char i, j, k, m, n, p;       // Counter variables 
const unsigned char cE  = 0x04;
const unsigned char cRS = 0x01;
unsigned char value[3];
unsigned int iValue;
int	ambientTemp;

//function prototypes
void init_ATD(void);
void initLCD(void);
void initPorts(void);
void LCDputs(char*);
void scanKeypad(void);
void cmdwrt(unsigned char);		 
void datawrt(char);	 
void position(int,int);	
void delay1u(void);		
void delay100u(void);	
void delay2m(void);		
void delays(int);
void Hex2Decimal(int);


/************main************/
void main()
{
	init_ATD();			//initialize ATD
	initPorts( );	  // port initializations
	initLCD( );	          // LCD inititialization   
	
	
	//display initial message 
	for (p=0;p<3;p++) 
	{
		position(p+1,0);
		LCDputs(cMessage[p]);    //passes message address
	}
	
	delays(10000);

  for (p=0;p<3;p++) 
	{
		position(p+1,0);
		LCDputs(cMessage2[p]);    //passes message address
	}
	
	EnableInterrupts;
	
	for(;;)
	{
		scanKeypad();			//check for user input
	}
}

/*****interrupts******/

// ADC Interrupt Service Routine

#pragma CODE_SEG NON_BANKED 

interrupt(((0x10000 - 0xFFD2) / 2 ) - 1 ) void isrADC(void)
{
		
	iValue = ATDDR0;		// read conversion result-saved to value
	
	//calculate Ambient temperature
	ambientTemp = ((iValue * 120) >> 8) - 50;
	Hex2Decimal(ambientTemp);
	
	
	position(2, 5);
	datawrt(value[2]);
	datawrt(value[1]);
	datawrt(value[0]);
	datawrt(0xDF);
	datawrt(0x43);
	
  ATDSTAT0_SCF = 1;	// clear convseq complete flag
  delay1u();
}

#pragma CODE_SEG DEFAULT

/*********functions*********/

void init_ATD(void)
{
	ATDCTL1_SRES = 1;					// 12-bit results
	ATDCTL2_ASCIE = 1;					// enable interrupt
	ATDCTL3_DJM = 1;					// right justified data
	ATDCTL3_S8C = 0;					// one conversion per sequence
	ATDCTL3_S4C = 0;					// one conversion per sequence
	ATDCTL3_S2C = 0;					// one conversion per sequence
	ATDCTL3_S1C = 1;					// one conversion per sequence
	ATDCTL4_PRS = 11;					// bus clk div 6.25MHz/2(1 + 2) = 1.04MHz ATD clock
	ATDCTL4_SMP = 4;					// 12 ATD clock cycle to sample
	ATDSTAT0_SCF = 1;					// clear flag
	ATDCTL5 = 0x21;						// select channel 1
}


void initPorts( )
{ 
  unsigned char cValue;
  
  DDRT = 0xF0;		//enable LED output PTT7-
	PTT = 0xF0;			//turn off LEDs
	
	DDRA = 0xF0; // Upper nibble Rows: output, Lower nibble Columns: input
  PUCR = 0x01; // Enables Port A pull up resistors
  
	DDRB   = 0x80;    //LCD CSB active low
	DDRC   = 0xFF;    // PC7-PC4 - 4-bit LCD data bus, PC2 - E, PC1 - R/W~, PC0 - RS: all outputs
	cValue = PORTB;
	PORTB  = cValue | 0x00;  // LCD CSB (PORT B7) enabled with a logic low
}
	 	  				
// sends initialization commands one-by-one
void initLCD( )
{
	unsigned char i;
	for (i=0;i<=19;i++)
	{  
		cmdwrt(cInit_commands[i]);
	}	

	DDRP_DDRP0 = 1;		//enable backlight
	PTP_PTP0 = 1;		//.....
}

void position(int iRow_value, int iCol_value) 
{
     int iPos_h, iPos_l, iValue;
     if(iRow_value == 1) iValue = (0x80+0x00);
     if(iRow_value == 2) iValue = (0x80+0x10);
     if(iRow_value == 3) iValue = (0x80+0x20);
     iPos_h=((iValue + iCol_value) & 0xF0);
     iPos_l=((iValue + iCol_value) & 0x0F) << 4;
     cmdwrt(iPos_h);
     cmdwrt(iPos_l);
}
// sends a control word to LCD bus
void cmdwrt(unsigned char cCtrlword)
{  
   asm("sei");
	 PORTC = cCtrlword;	  // output command onto LCD data pins
	 PORTC = cCtrlword + cE;	  // generate enable pulse to latch it (xxxx x100)
	 delay1u( );		  // hold it for 1us
	 PORTC = cCtrlword;		  // end enable pulse  (xxxx x000)
	 delay2m();		  // allow 2ms to latch command inside LCD
	 PORTC = 0x00;
	 delay2m();		  // allow 2ms to latch command inside LCD
}

//Sends a string of characters to the LCD;...  
void LCDputs(char *sptr)
{  
     while(*sptr)
     {    //...the string must end in a 0x00 (null character)
        datawrt(*sptr);   // sptr is a pointer to the characters in the string
	++sptr;
     }
}

void scanKeypad()
{
	// Key press detector loop
	i = 0;
	cValue = PORTA & 0x0F;    // Read current value on lower nibble
	PORTA = cValue | cRow[i]; // Output logic Low to all rows
	
	while ( (PORTA & 0x0F) == 0x0F ); // Stay in loop until key pressed 

	// Key press identification loop
	for(i = 1; i < 5; i++) 
	{ 
		cValue = PORTA & 0x0F;      // Read current value on lower nibble
		PORTA = cValue | cRow[i];   // Output logic Low to row i – “|” = or operator
		delay1u();			         // Delay loop – allows time for the row output to reach keypad
		cCol[i] = PORTA & 0x0F;     // Read all column results for row i
	}
	// Key press assignment loop
	n  = 0;                           // Set n value to 0—used as the array pointer
	k  = 0;                           // Set k value to 0—used as row value
	for(i = 1; i < 5; i++) 
	{ 
		switch (cCol[i])
		{
		case 0x0F:         // No key pressed for row i
			   n += 0; 
			   break;
		case 0x0E:         // Row i Column 1 key pressed
			   n += 1 + k;   
			   break;
		case 0x0D:         // Row i Column 2 key pressed
			   n += 2 + k;   
			   break;
		case 0x0B:         // Row i Column 3 key pressed
			   n += 3 + k;   
			   break;
		case 0x07:         // Row i Column 4 key pressed
			   n += 4 + k;   
			   break;
		default:               // more than one key pressed
			   n += 17;   
			   break;
		}     // end of switch—case 
		k += 4;    // increment k value by 4 (4 keys on each row)
	}          // end for loop
	if (n >= 17) n = 0;  // More than one key pressed
	//output keypressed to LCD
  position(1, 6);
	switch(n)
	{
	  case 0:
		{
		  asm("cli");
			break;
		}
		case 1:
		{	
			datawrt('A');
			datawrt('C');
			datawrt(' ');
			datawrt(' ');
			PTT = 0x90;
			break;
		}
		case 2:
		{
			datawrt('H');
			datawrt('E');
			datawrt('A');
			datawrt('T');
			PTT = 0x20;
			break;
		}
		case 3:
		{
			datawrt('O');
			datawrt('F');
			datawrt('F');
			datawrt(' ');
			PTT = 0xF0;
			break;
		}
		default:
		{
			PTT = 0xF0;
			break;
		}
	}
	asm("cli");
}

// sends the character passed in by caller to LCD 
void datawrt(char cAscii)
{                   
	 char cAscii_high, cAscii_low;
	 cAscii_high = (cAscii & 0xF0);
	 cAscii_low  = (cAscii & 0x0F) << 4;  // Shift left by 4 bits 
	 PORTC = cAscii_high;              // output ASCII character upper nibble onto LCD data pins
	 PORTC = cAscii_high + cRS + cE;   // generate enable pulse to latch it  (0xxx x101)
	 delay1u( );  	// hold it for 1us
	 PORTC = cAscii_high + cRS;         // end enable pulse   (0xxx x001)
	 delay1u( );  	// hold it for 1us
	 PORTC = cAscii_low;               // output ASCII character lower nibble onto LCD data pins
	 PORTC = cAscii_low + cRS + cE;    // generate enable pulse to latch it  (0xxx x101)
	 delay1u( );  	// hold it for 1us
	 PORTC = cAscii_low + cRS;         // end enable pulse   (0xxx x001)
	 delay100u( );                     // allow 100us to latch data inside LCD
}
void delay1u( )
{
	 unsigned int i;
	 for(i=0;i<=0x0f;i++)
	 { /* adjust condition field for delay time */
	 	 asm("nop");
	 }
}
void delay100u( )
{
	 unsigned int i,j;
	 for(i=0;i<=0x02;i++)
	 {	 /* adjust condition field for delay time */
	  	for(j=0;j<=0xff;j++)
	  	{
	 	    asm("nop");
	  	}
	 }
}
void delay2m( )
{
   unsigned int i,j;	
   for (i=0;i<=0x20;i++)
   { /* adjust condition field for delay time */
 	   for (j=0;j<=0xff;j++)
 	   {
	     asm("nop");
	   }		   
   }
}    
void delays(int k )
{
   unsigned int i,j;	
   for (i=0;i<=k;i++)
   { /* adjust condition field for delay time */
 	   for (j=0;j<=0xff;j++)
 	   {
	     asm("nop");
	   }		   
   }
}

/* Converts an unsigned hex number availabe in the micro registers to decimal digits 
and stores the decimal digits in an array */
void Hex2Decimal(int num)
{
	unsigned char i = 0;

	while(num > 0)
	{
		value[i] = (num % 10) | 0x30;
		num /= 10;
		i = i + 1;
	}
}