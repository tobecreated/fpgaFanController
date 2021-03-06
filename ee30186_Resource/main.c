/*
 * main.c
 *
 *	main program for EE30186
 *
 *  Created on: 22 Sep 2015
 *      Author: Alex Beasley
 */

#include "EE30186.h"
#include "system.h"
#include "socal/socal.h"
#include <inttypes.h>
#include <stdio.h>


#define key0 0xE
#define key1 0xD
#define key2 0xB
#define key3 0x7

// Prototype Functions


int sevenSegmentDecoder (int);
int multiDigitDecoder  (int);
void setDisplays(int);
int rotaryEncoder(int, int);
int readPushButton(int);

volatile int * LEDs = (volatile int *) ALT_LWFPGA_LED_BASE;
volatile int * Switches = (volatile int *)(ALT_LWFPGA_SWITCH_BASE);
volatile int * Hex3to0  = (volatile int *)(ALT_LWFPGA_HEXA_BASE);
volatile int * Hex5to4  = (volatile int *)(ALT_LWFPGA_HEXB_BASE);
volatile int * GpioPort = (volatile int *)(ALT_LWFPGA_GPIO_1A_BASE);
volatile int * PushButton = (volatile int *)(ALT_LWFPGA_KEY_BASE);
volatile int * Counter = (volatile int *)(ALT_LWFPGA_COUNTER_BASE);

int main(int argc, char** argv)
{
	// Pointers for connecting to the switches and LED Hex digit displays

		// Temporary variables needed for Activity 1 and 2
		//int DigitDisplay;

	        // These are the temporary variables needed to make activity 3 more readable
//		int FirstVariable, SecondVariable, Sum, mddFirst, mddSecond, CombinedSegments;

		//function call to initialise the FPGA configuration
		EE30186_Start();
		int currentValue = 100;
		int gpioBit17, gpioBit19, rotaryOut;
//		volatile int *GpioDdr = GpioPort + 1;
//		volatile unsigned int *Counter = (volatile unsigned int *)(ALT_LWFPGA_COUNTER_BASE);

		while (1)
		{
//			FirstVariable  =  (*Switches)&0x01F;
//			SecondVariable = ((*Switches)&0x3E0) >> 5;
//
//			// Work out the sum as we need to display that too
//			Sum = FirstVariable + SecondVariable;
//
//			// Get the two digit decoder outputs for each input variable and
//			// combine them, to a single value that can be written to the output
//			// register
//			mddFirst = multiDigitDecoder(FirstVariable);
//			mddSecond = multiDigitDecoder(SecondVariable);
//
//			CombinedSegments = (multiDigitDecoder(FirstVariable)) & 0x0000FFFF;
//			CombinedSegments |= (multiDigitDecoder(SecondVariable)) << 16;
//
//			// Write the input values to the display
//			*Hex3to0 = CombinedSegments;
//
//			// convert the sum to the seven segment display equivalent and pass it
//			// to the second display register
////			*Hex5to4 = multiDigitDecoder(Sum);
			int pushButton = readPushButton(*PushButton);


			printf("%i", pushButton);
			gpioBit17 = ((*GpioPort >> 17) & 0b1);
			gpioBit19 = ((*GpioPort >> 19) & 0b1);
			rotaryOut = rotaryEncoder(gpioBit17, gpioBit19);
			currentValue += pushButton;
			currentValue += rotaryOut;
			setDisplays(currentValue);
		}

		//function call to clean up and close the FPGA configuration
	    EE30186_End();

	    return 0;
}

void setDisplays(int fullDisplayValue){
	int val3to0, val5to4, code3to0, code5to4;

	val3to0 = fullDisplayValue%10000;
	val5to4 = fullDisplayValue/10000;

	code3to0 = multiDigitDecoder(val3to0);
	code5to4 =  multiDigitDecoder(val5to4);

	*Hex3to0 = code3to0;

	// convert the sum to the seven segment display equivalent and pass it
	// to the second display register
	*Hex5to4 = code5to4;
}

int multiDigitDecoder(int Value){
	// Start with a blank display value to return
	int ReturnValue = 0xFFFFFFFF;

	// We need to be able to keep track of which digit in the number we are dealing
	// with
	int CurrentDigit = 0;

	// As we extract the digits we need a temporary variable to put the values into
	int encodedDigit, bottomDigit;

	// loop up through the digits in the number. By using a do-while loop we
	// ensure that the decoder is called at least once. This means that a zero is
	// displayed as a single zero on the display rather than all blank.
	do
	{
		// Extract bottom digit and remove it from value
		bottomDigit = Value%10;
		Value /= 10;
		// Get encoded digit
		encodedDigit = sevenSegmentDecoder(bottomDigit);

		// Clear the space that we are going to put the decoder result into
		// Sets all the bits to 0 for this space
		// (everything else stays as is because NOT makes all lower bits 1s, all change bits 0s)
		ReturnValue = ReturnValue & ~(0xFF << (CurrentDigit * 8));

		// Shift the single decoded digit to the right place in the int and insert
		// it
		ReturnValue = ReturnValue |  (encodedDigit << (CurrentDigit * 8));

		// Update the digit position so that if the value is non-zero we put the
		// next digit 8 bits further to the left.
		CurrentDigit++;

	} while (Value > 0);

	// Pass back the multi-digit decoded result.
	return ReturnValue;
}

int sevenSegmentDecoder(int digit){
	int segmentsArray[10] = {0x40, 0xF9, 0x24, 0x30, 0x19, 0x12, 0x02, 0xF8, 0x00, 0x10};
	int defaultSegment = 0xFF;
	if (digit >= 0 && digit <= 9){
		return segmentsArray[digit];
	} else {
		return defaultSegment;
	}
}

int indexOf(int *array, int array_size, int value){
    int i;
    for (i=0; i < array_size; i++) {
        if (array[i] == value)
            return i;
    }
    return -1;
}

int rotaryEncoder(int gpioBit17, int gpioBit19){
	int delta = 0;
	static int prevBit17, prevBit19;
	int clockwise1 = (prevBit17==1) && (prevBit19==1) && (gpioBit17==1) && (gpioBit19==0);
	int clockwise2 = (prevBit17==0) && (prevBit19==0) && (gpioBit17==0) && (gpioBit19==1);
	int anticlockwise1 = (prevBit17==1) && (prevBit19==1) && (gpioBit17==0) && (gpioBit19==1);
	int anticlockwise2 = (prevBit17==0) && (prevBit19==0) && (gpioBit17==1) && (gpioBit19==0);
	if (clockwise1 || clockwise2){
		delta = 1;
	} else if (anticlockwise1 || anticlockwise2){
		delta = -1;
	}
	prevBit17 = gpioBit17;
	prevBit19 = gpioBit19;

	return delta;
}

int readPushButton(int PushButton){
	int key = 0;
	switch (PushButton){
	case key0:
		key = 0;
		break;
	case key1:
		key = 1;
		break;
	case key2:
		key = 2;
		break;
	case key3:
		key = 3;
		break;
	}
	return key;
}

int tachoRPM(void){
	static int rpmSum;
	static unsigned int i;
	static unsigned int previousTacho;
	static unsigned int previousTachoPosEdge;
	int c = 30;
	int currentTacho = *GpioPort & 0x2, tachoPeriod, RPM;
	static int RPMarray[averageSize];

}

int pwmSigGen(int RPM){

}

int tachoRPM(int gpioBit1, int gpioBit3){
	int arraySize = 1000;
	static int rpmArray [arraySize];
	static int averageRPM;
	static int prevReading;
	static int prevCount;
	static int index;
	int currentReading = gpioBit1;
	int currentCount = *Counter;
	if (currentReading != prevReading){
		rpmArray[index] = currentCount;
		index += 1;
		if (index >= arraySize){
			index = 0;
		}
	}
	int maxCount = rpmArray[index-1]; // The entry we just added is the most recent
	// Find index of count averageRPM ago...
	int averageRPS = averageRPM/60;
	int minIndex = index - averageRPS - 1; // Index 1 second ago according to previous aveRPM


}


int tachometerRPM (int Pin1, int Pin3) {

	/* The tachometer function reads the actual speed sent by the tachometer of the fan, and returns an average value. It also displays this value on the HexA display

	 * It samples the square wave on the tachometer, in order to detect the positive/negative edge of a wave. The time period is then established by comparing the count at two edges.

	 * NOTE: 1 revolution of the fan is equal to TWO square waves.

	 * This function must be called constantly (by main) in order to sample the tachometer bit1 from the GPIO. */

	static int RPMsum; // variable to accommodate for summed RPM values, to allow for averaging
	static unsigned int i; // incrementing i
	static unsigned int PreviousTacho; // the previous output from the tachometer is equal to the first signal from the gpio
	static unsigned int PreviousTachoPosedge; // variable to store count at positive edge of square wave
	int c = 30; // constant to allow for display to be changed less regularly, to make display readable. also sets speed leds change at.
	int CurrentTacho = *GPIO & 0x2, tachoPeriod, RPM; // current tacho reading from gpio is assigned to 'currentTacho'. 'tachoPeriod' is the time in frequency of 0.5 of a period
	static int RPMarray [averageSize]; // array to assemble the RPM of the tacho
	static int averageRPM; // value to be displayed and returned

	unsigned int CurrentTachoPosedge; //current count at any edge (positive or negative)

	if (CurrentTacho != PreviousTacho) { // if the current reading from the tachometer is not equal to the previous reading, an edge of the square wave has occured
		CurrentTachoPosedge = *Counter; // read the count at that edge
		PreviousTacho = CurrentTacho; // update 'previousTacho' with new value of Bit1
		tachoPeriod = CurrentTachoPosedge - PreviousTachoPosedge; // half of period
		RPM = 750000000/tachoPeriod; // 2 periods = 4*tachoperiod per revolution of the fan. RPM calculates the speed
		PreviousTachoPosedge = CurrentTachoPosedge; // update the count at the previous edge of the square wave with the new edge

		if ((RPM < 3300) && (RPM > 50)) { // RPM will not exceed 3300 and go lower than 50 so ignore all reading from the tach that are not in this range
			RPMsum = RPMsum - RPMarray [i % c]; // do moving average of RPM - SAME METHOD AS IN CONTROLLER. LOOK THERE FOR MORE DETAILED COMMENTS

			RPMarray [i % c] = RPM;

			RPMsum = RPMsum + RPMarray[i % c];

			i++;

			averageRPM = (RPMsum / c);
		}

		if ((i % c) == 0) { // delay to make display readable
			int desiredSpeed = rotaryEncoder (*GPIO, *PushButton);
			int Error2Disp = abs(averageRPM - desiredSpeed);

			if ((Error2Disp) >= 300) *LEDs = 0x3FF;//
			else if (((Error2Disp) >= 150) && ((Error2Disp) < 200)) *LEDs = 0xFF;//
			else if (((Error2Disp) >= 100) && ((Error2Disp) < 150)) *LEDs = 0x3F;//LEDs all light up when error is large (over 300) and gradually switch off as error
			else if (((Error2Disp) >= 50) && ((Error2Disp) < 100)) *LEDs = 0xFF;//is reduced
			else if ((Error2Disp) < 50) *LEDs = 0x3;//

			if ((*Switches == 0x201) || (*Switches == 0x301)) *HexA = multipleDigitNum (desiredSpeed); // use switch 0 to toggle between desired speed and actual speed
			else if ((*Switches == 0x202) || (*Switches == 0x302)) *HexA = multipleDigitNum (Error2Disp); // use switch 1 to toggle between error value and actual speed
			else *HexA = multipleDigitNum (averageRPM);

		}
	}

	return averageRPM;

}
