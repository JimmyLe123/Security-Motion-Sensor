/*
* Name & E-mail: Jimmy Le jle071@ucr.edu
* Lab Section: 23
* Assignment: CS122A Project Milestone 1
*
* I acknowledge all content contained herein,excluding template or example code, is my own original work.
* Source for motion sensor code: https://www.gadgetronicx.com/pir-motion-sensor-interface-with-avr/
* Source for blue-tooth module: http://www.dsdtech-global.com/2017/08/hm-10.html
*/

#include <avr/io.h>
#include <avr/interrupt.h>
#include "usart_ATmega1284.h"
#include "nokia5110.h"
#include "nokia5110.c"
#include "keypad.h"
#include "bit.h"
#include "timer.h"
#include "PWM.h"
unsigned char temp;										//Hold value entered from phone using blue tooth
unsigned char KeypadInput;								//Hold value entered from keypad
unsigned char inputLock[4] = {'0','0','0','0'};			//Code to enter from user when 'locking' (activating motion sensor).
unsigned char inputUnlock[4] = {'0','0','0','0'};		//Code to enter from user when 'unlocking' (deactivating motion sensor).
unsigned char timecnt = 0;								//Keeping motion sensor on for 1 sec
unsigned char count = 0;								//A count to individually insert the inputs of the keypad into the array
unsigned char Index = 0;								//Current index of the inputLock array
unsigned char UnlockIndex = 0;							//Current index of the inputUnlock array
unsigned char Locked = 0;								//To set the motion sensor alarm on/off

enum KeyPadSM{Init, Press, PressHold, EnterInput, WaitRelease, Lock, LocktoUnlock, PressUnlock, UnlockInput, WaitRelUnlock, Unlock, UnlocktoLock}keyState;
enum MotionSM{Init1, WaitMotion}motionState;

void tick_InputCode()
{
	switch(keyState)
	{
		case Init:
		keyState = Press;
		break;
		
		case Press:
		KeypadInput = GetKeypadKey();
		if(KeypadInput != '\0')							//If entered a character from keypad
		{
			keyState = PressHold;
		}
		else if (USART_HasReceived(0))					//If entered a character using blue-tooth(phone app)
		{
			temp = USART_Receive(0);
			keyState = PressHold;
		}
		else if (KeypadInput == '\0')					//Keep waiting for input from keypad or phone
		{
			keyState = Press;
		}
		break;
		
		case PressHold:
		if(KeypadInput != '\0' || USART_Receive(0))
		{
			switch (temp)								//Verify that both keypad and blue tooth are working.
			{
				case '1':
				PORTA = 0x01;
				break;
				case '2':
				PORTA = 0x02;
				break;
				case '3':
				PORTA = 0x03;
				break;
				case '4':
				PORTA = 0x04;
				break;
				case '5':
				PORTA = 0x05;
				break;
				case '6':
				PORTA = 0x06;
				break;
				case '7':
				PORTA = 0x07;
				break;
				case '8':
				PORTA = 0x08;
				break;
				case '9':
				PORTA = 0x09;
				break;
				case '0':
				PORTA = 0x00;
				break;
				case 'A':
				PORTA = 0x0A;
				break;
				case 'B':
				PORTA = 0x0B;
				break;
				case 'C':
				PORTA = 0x0C;
				break;
				case 'D':
				PORTA = 0x0D;
				break;
				case '*':
				PORTA = 0x0E;
				break;
				case '#':
				PORTA = 0x0F;
				break;
			}
		}
		if(Locked == 0)									//While motion sensor not active. Enter code for activating motion sensor
		{
			keyState = EnterInput;
		}
		else if(Locked == 1)							//While motion sensor active. Enter code for deactivating motion sensor
		{
			keyState = UnlockInput;
		}
		break;

		case EnterInput:
		if(KeypadInput != '\0')							//If the input is being held, then input it into the lock array.
		{
			inputLock[Index] = KeypadInput;
			Index++;
		}
		else if(USART_Receive(0))						//If Microcontroller has received data through USART from blue-tooth device.
		{
			inputLock[Index] = temp;
			Index++;
		}
		keyState = WaitRelease;
		if(Index > 3)
		{
			keyState = Lock;
		}
		break;
		
		case WaitRelease:
		KeypadInput = GetKeypadKey();
		if (KeypadInput != '\0')						//If input is still being held, wait until release.
		{
			keyState = WaitRelease;
		}
		else if(KeypadInput == '\0')					//If input is released go back to the state where you input something again.
		{
			keyState = Press;
		}
		else if(USART_Receive(0))
		{
			keyState = Press;
			USART_Flush(0);
		}
		break;
		
		case Lock:
		Index = 0;
		keyState = LocktoUnlock;
		break;
		
		case LocktoUnlock:
		KeypadInput = GetKeypadKey();
		if(KeypadInput != '\0')
		{
			keyState = LocktoUnlock;
		}
		else if(KeypadInput == '\0')
		{
			Locked = 1;
			keyState = PressUnlock;
		}
		break;
		
		case PressUnlock:
		KeypadInput = GetKeypadKey();
		if(KeypadInput != '\0')
		{
			keyState = PressHold;
		}
		else if (USART_HasReceived(0))
		{
			temp = USART_Receive(0);
			keyState = PressHold;
		}
		else if(KeypadInput == '\0')
		{
			keyState = PressUnlock;
		}
		break;
		
		case UnlockInput:
		if(KeypadInput != '\0')
		{
			inputUnlock[UnlockIndex] = KeypadInput;
			UnlockIndex++;
		}
		else if (USART_Receive(0))
		{
			inputUnlock[UnlockIndex] = temp;
			UnlockIndex++;
		}
		keyState = WaitRelUnlock;
		if(UnlockIndex > 3)
		{
			keyState = Unlock;
			UnlockIndex = 0;
		}
		break;
		
		case WaitRelUnlock:
		KeypadInput = GetKeypadKey();
		if(KeypadInput == '\0')
		{
			keyState = PressUnlock;
		}
		else if(KeypadInput != '\0')
		{
			keyState = WaitRelUnlock;
		}
		else if(USART_Receive(0))
		{
			keyState = PressUnlock;
			USART_Flush(0);
		}
		break;
		
		case Unlock:
		for(int j = 0; j < 4; j++)
		{
			if(inputLock[j] == inputUnlock[j])
			{
				count++;
			}
		}
		if(count == 4)									//When lock array code does equal the unlock array code
		{
			count = 0;
			keyState = UnlocktoLock;
		}
		else if (count != 4)							//When unlock array code doesn't equal the lock array code
		{
			PORTA = 0xA2;
			UnlockIndex = 0;
			count = 0;
			for(int i = 0; i < 4; i++)
			{
				inputUnlock[i] = '0';					//Reset the array and try again.
			}
			keyState = WaitRelUnlock;
		}
		break;
		
		case UnlocktoLock:
		KeypadInput = GetKeypadKey();
		if(KeypadInput != '\0')
		{
			keyState = UnlocktoLock;
		}
		else if (KeypadInput == '\0')
		{
			Locked = 0;
			for(int k = 0; k < 4; k++)
			{
				inputLock[k] = '0';				//Reset both arrays for another pass code.
				inputUnlock[k] = '0';
			}
			keyState = Press;
		}
		break;
	}
	switch(keyState)
	{
		case Init:
		break;
		
		case Press:
		PORTA = 0x1F;
		break;
		
		case PressHold:
		break;
		
		case EnterInput:
		break;
		
		case WaitRelease:
		break;
		
		case Lock:
		break;
		
		case LocktoUnlock:
		break;
		
		case PressUnlock:
		PORTA = 0xFF;
		break;
		
		case UnlockInput:
		break;
		
		case WaitRelUnlock:
		break;
		
		case Unlock:
		break;
		
		case UnlocktoLock:
		break;
	}
}

void tick_Motion()
{
	switch (motionState)
	{
		case Init1:
		motionState = WaitMotion;
		break;
		
		case WaitMotion:
		if(Locked == 0)				  //From previous state machine, when code entered, set motion sensor active.
		{
			PORTB = 0x04;
			if((PINB&(1<<0)))         // check for sensor pin PB0 using bit
			{
				PWM_on();
				if(timecnt < 10)
				{
					PORTD = 0x20;           //Test LED on
					set_PWM(1000);		    // buzzer on
					_delay_ms(100);
					PORTD = 0x00;
					timecnt++;
				}
				else if (timecnt == 10)
				{
					PWM_off();
					//set_PWM(0);
					timecnt = 0;
				}
			}
			else
			{
				PORTD=0x00;			 //Test LED off
				PWM_off();
				//set_PWM(0);			 // buzzer off
			}
		}
		else if(Locked == 0)
		{
			PWM_off();
			PORTB = 0x00;
		}
		motionState = WaitMotion;
		break;
	}
	
	switch(motionState)
	{
		case Init1:
		break;
		
		case WaitMotion:
		break;
	}
}

int main(void)
{
	DDRA = 0xFF; PORTA = 0x00;	//Set port A to be output
	DDRB = 0xFE; PORTB = 0x01;  //Only set B0 to be input
	DDRC = 0xF0; PORTC = 0x0F;  //Set lower half of port C to be input and the upper half to be output
	DDRD = 0xFF; PORTD = 0x00;  //Set port D to be output
	initUSART(0);
	keyState = Init;
	motionState = Init1;
	TimerSet(100);
	TimerOn();
	PWM_on();					//Turn on the pulse width modulator
	while (1)
	{
		tick_InputCode();
		tick_Motion();
	}
	return (0);
}