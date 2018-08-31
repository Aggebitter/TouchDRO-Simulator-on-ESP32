// Visual Micro is in vMicro>General>Tutorial Mode
// 
/*
    Name:       ESP32ArduinoTochDRO_test.ino
    Created:	2018-08-29 08:03:57
    Author:     Lenov_Agge\Agge
*/

/*
This program is free software : you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.If not, see <http://www.gnu.org/licenses/>.
*/

/*
###################################################################################################################
# 
#  This is just some tesing code for the protocol. No code for reading calipers !!
#
#
#  TouchDRO doeas at the moment only work with Bluetooth Classis and Serial Port Profile (SPP).
#  SPP does not use baudrate, the BT Device just sends as fast as the over air inteface can handle.
#  This is the same with the well known HC05 Devices, the Baudrate is just an higlevel software 
#  implementation when it acts as a server/client and uart bridge.
#
#  X,Y and Z -Axis: 
#  What I've managed to figure out on the Serial Protocol used by TouchDRO is that it uses plain ASCII (char).
#  The format is Axis + Absolute possition in Counts Per Inch(long) and ended with a terminating character.
#  Example:  X1234567;Y2345678;Z3456789;
# 
#  W - Axis:
#  Configured as an A-axis
#  A standard A-axis (This would be nice!!)
#  ToDo: Use W-axis as an A-axis. radians to degree conversion ?
#
#  Tachometer:
#  Same format, but uses "T" for tach and rpm in unsinged-long (Wy not a signed when reversing the spindel ?)
#  Example T123456789;
#  ToDo: test a signed long for negative read out then a CV and a CCV flag can be used
#
#  Probe:
#  Uses "P" and a unsigned int (using a bool for better looking code) (ToDo: check if inverted or not9
#  Example: P1; and P0;
#
#
#	Alot of info gain from Yuriy Krushelnytskiy and Ryszard Malinowski Arduino code linked below
#
#   ArduinoDRO + Tach V5.11
#
#   iGaging/AccuRemote Digital Scales Controller V3.3
#   Created 5 July 2014
#   Update 15 July 2014
#   Copyright (C) 2014 Yuriy Krushelnytskiy, http://www.yuriystoys.com
#
#
#   Updated 02 January 2016 by Ryszard Malinowski
#   http://www.rysium.com
#
###################################################################################################################
*/

// DRO configuration

#define UPDATE_FREQUENCY 100				//  Times per second TouhDRO is updated in Hz)

#define SCALE_X_ENABLED		1
#define X_CPI				2540
#define SCALE_X_TYPE		1			// 0 = Simulation, 1 = Chinese Caliper, 2 = iGaging ToDo!
#define SCALE_X_CLK_PIN		17
#define SCALE_X_DATA_PIN	16


#define SCALE_Y_ENABLED		1
#define Y_CPI				2540
#define SCALE_Y_TYPE		0			// 0 = Simulation, 1 = Chinese Caliper, 2 = iGaging ToDo! 
//#define SCALE_Y_CLK_PIN	**			// uncoment if not used
//#define SCALE_Y_DATA_PIN	**			// uncoment if not used

#define SCALE_Z_ENABLED		1
#define Z_CPI				2540
#define SCALE_Z_TYPE		0			// 0 = Simulation, 1 = Chinese Caliper, 2 = iGaging ToDo! 
//#define SCALE_Z_CLK_PIN	**			// uncoment if not used
//#define SCALE_Z_DATA_PIN	**			// uncoment if not used

#define SCALE_W_ENABLED		1
#define W_CPA				100			// xx:th of a degree
#define SCALE_W_TYPE		0			// 0 = Simulation, 1 = Chinese Caliper, 2 = iGaging ToDo! 
//#define SCALE_W_CLK_PIN	**			// uncoment if not used
//#define SCALE_W_DATA_PIN	**			// uncoment if not used

#define TACH_ENABLED		1			
#define TACH_TYPE			0			// 0 = Simulation, all other: uint8_t ticks per revolution
//#define TACH_PIN			**			// uncoment if not used
//#define TACH_SIG_INV		0			// true if inverted, uncoment if not used
#define TACH_MAX_SPEED		20000		// uncoment if not used

#define PROBE_ENABLED		1			// uncoment if not used
#define PROBE_INVERTED		0			// uncoment if not used

#define MAX7219_DRO			3			// number of 7-segment max7219 displays, 0 = disabled
#define LEDMATRIX_CS_PIN	5			// uncoment if no 7-segment is used

#if SCALE_X_ENABLED == 1
long X_Readout;
#endif 
#if SCALE_Y_ENABLED == 1
long Y_Readout;
#endif 
#if SCALE_Z_ENABLED == 1
long Z_Readout;
#endif 
#if SCALE_W_ENABLED == 1
long W_Readout;
#endif 
#if TACH_ENABLED == 1
long T_Readout;
#endif 

// Step size for the scales simulator 
#if SCALE_X_ENABLED == 1 && SCALE_X_TYPE == 0
	int X_StepSize = 100;
#endif 
#if SCALE_Y_ENABLED == 1 && SCALE_Y_TYPE == 0
	int Y_StepSize = 100;
#endif 
#if SCALE_Z_ENABLED == 1 && SCALE_Z_TYPE == 0
	int Z_StepSize = 100;
#endif 
#if SCALE_W_ENABLED == 1 && SCALE_W_TYPE == 0
	int W_StepSize = 10;
#endif 
#if TACH_ENABLED == 1 && TACH_TYPE == 0
	int T_StepSize = 100;
#endif 

 // ESP High level Arduino SPP API
#include "BluetoothSerial.h"

#if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
#error Bluetooth is not enabled! Please run `make menuconfig` to and enable it
#endif

BluetoothSerial SerialBT; 

#if SCALE_X_ENABLED == 1 && SCALE_X_TYPE == 1
int bit_array[25]; // For storing the data bit. bit_array[0] = data bit 1 (LSB), bit_array[23] = data bit 24 (MSB).
unsigned long time_now; // For storing the time when the clock signal is changed from HIGH to LOW (falling edge trigger of data output).
#endif

#if MAX7219_DRO	>=1 
	#include <LEDMatrixDriver.hpp>
	LEDMatrixDriver lmd(MAX7219_DRO, LEDMATRIX_CS_PIN);

	#if MAX7219_DRO	==3 
		#define X_Last_Digit 23
		#define Y_Last_Digit 15
		#define Z_Last_Digit 7
	#endif

	#if MAX7219_DRO	==2 
		#define X_Last_Digit 15
		#define Y_Last_Digit 7
	#endif

	#if MAX7219_DRO	==1 
		#define X_Last_Digit 7
	#endif
	
#endif



// The setup() function runs once each time the micro-controller starts
void setup()
{
#if MAX7219_DRO	>=1 
	// init the 7-segment display
	lmd.setEnabled(true);
	lmd.setIntensity(2);  // 0 = min, 15 = max
	lmd.setScanLimit(7);  // 0-7: Show 1-8 digits. Beware of currenct restrictions for 1-3 digits! See datasheet.
	StartUpDisplay(true);
	delay(200);
	StartUpDisplay(false);
#endif

	Serial.begin(115200);
	SerialBT.begin("ESP32testDRO"); //Bluetooth device name
	Serial.println("The device started, now you can pair it with bluetooth!");


#if SCALE_X_ENABLED == 1
	#if SCALE_X_TYPE == 0
		X_Readout = random(X_CPI * -10, X_CPI * 10);  // +- 10"
	#endif

	#if SCALE_X_TYPE == 1
		// start wiht trying to get the caliper to go in fast mode by setting DATA pin HIGH
		// see http://www.shumatech.com/support/chinese_scales.htm#Chinese%20Scale%20Protocol
		pinMode(SCALE_X_CLK_PIN, OUTPUT);
		digitalWrite(SCALE_X_CLK_PIN, HIGH);
		delay(10);
		digitalWrite(SCALE_X_CLK_PIN, LOW);
		delay(10);
		// then make the pin readable
		pinMode(SCALE_X_CLK_PIN, INPUT);
		pinMode(SCALE_X_DATA_PIN, INPUT);

	#endif
#endif
	// Get some random seed for the axises
	
	Y_Readout = random(Y_CPI * -10, Y_CPI * 10);
	Z_Readout = random(Z_CPI * -10, Z_CPI * 10);
	W_Readout = random(0, W_CPA * 360 );
	T_Readout = random(0, TACH_MAX_SPEED);
	//delay(10);

}

// Add the main program code into the continuous loop() function
void loop()
{
	// Some counters to change the DRO vaules

	//SerialBT.print("X" + String(X_Readout) + ";" + "Y" + String(Y_Readout) + ";" + "Z" + String(Z_Readout) + ";");
	//Serial.println("X" + String(X_Readout) + ";" + "Y" + String(Y_Readout) + ";" + "Z" + String(Z_Readout) + ";");

#if SCALE_X_ENABLED == 1
	#if SCALE_X_TYPE == 0
		SerialBT.print("X" + String(X_Readout) + ";" );
		Serial.print("X" + String(X_Readout) + ";");
		X_Readout = X_Readout + X_StepSize;
		if (X_Readout >= 12 * X_CPI || X_Readout <= -12 * X_CPI) X_StepSize = X_StepSize * -1;             // switch direction at endings
	#endif
	#if SCALE_X_TYPE == 1
		while (digitalRead(SCALE_X_CLK_PIN) == LOW) {} // If clock is LOW wait until it turns to HIGH
		time_now = micros();
		while (digitalRead(SCALE_X_CLK_PIN) == HIGH) {} // Wait for the end of the HIGH pulse
		if ((micros() - time_now) > 500) { // If the HIGH pulse was longer than 500 micros we are at the start of a new bit sequence
		decode(); //decode the bit sequence
		}
	#endif
#endif

#if SCALE_Y_ENABLED == 1
	SerialBT.print("Y" + String(Y_Readout) + ";" );
	Y_Readout = Y_Readout + Y_StepSize;
	if (Y_Readout >= 12 * Y_CPI || Y_Readout <= -12 * Y_CPI) Y_StepSize = Y_StepSize * -1;             // switch direction at endings
#endif

#if SCALE_Z_ENABLED == 1
	SerialBT.print("Z" + String(Z_Readout) + ";" );
	Z_Readout = Z_Readout + Z_StepSize;
	if (Z_Readout >= 12 * Z_CPI || Z_Readout <= -12 * Z_CPI) Z_StepSize = Z_StepSize * -1;             // switch direction at endings
#endif

#if SCALE_W_ENABLED == 1
	SerialBT.print("W" + String(W_Readout) + ";" );
	W_Readout = W_Readout + W_StepSize;
	if (W_Readout >= 360 * W_CPA || W_Readout <= 0) W_StepSize = W_StepSize * -1;             // switch direction at endings
#endif

#if TACH_ENABLED == 1
	SerialBT.print("T" + String(T_Readout) + ";" );
	T_Readout = T_Readout + T_StepSize;
	if (T_Readout >= TACH_MAX_SPEED || T_Readout <= 0) T_StepSize = T_StepSize * -1;             // switch direction at endings
#endif



	//delay(1000 / UPDATE_FREQUENCY);

}
#if SCALE_X_ENABLED == 1 && SCALE_X_TYPE == 1
void decode() {
	int sign = 1;
	int i = 0;
	float value = 0.0;
	long testy = 0;
	float result = 0.0;
	bit_array[i] = digitalRead(SCALE_X_DATA_PIN); // Store the 1st bit (start bit) which is always 1.
	while (digitalRead(SCALE_X_CLK_PIN) == HIGH) {};
	for (i = 1; i <= 24; i++) {
		while (digitalRead(SCALE_X_CLK_PIN) == LOW) {} // Wait until clock returns to HIGH
		bit_array[i] = digitalRead(SCALE_X_DATA_PIN);
		while (digitalRead(SCALE_X_CLK_PIN) == HIGH) {} // Wait until clock returns to LOW
	}
	for (i = 0; i <= 24; i++) { // Show the content of the bit array. This is for verification only.
		//Serial.print(bit_array[i]);
		//Serial.print(" ");
	}
	//Serial.println();
	for (i = 1; i <= 20; i++) { // Turning the value in the bit array from binary to decimal.
		value = value + (pow(2, i - 1) * bit_array[i]);
		testy = testy + (pow(2, i - 1) * bit_array[i]);
	}
	if (bit_array[21] == 1) sign = -1; // Bit 21 is the sign bit. 0 -> +, 1 => -
	if (bit_array[24] == 1) { // Bit 24 tells the measureing unit (1 -> in, 0 -> mm)
		result = (value*sign) / 2000.00;
#if MAX7219_DRO	>=1 

		LedDisplay(value*sign/2, true, Y_Readout, true, Z_Readout, true);
		//Serial.println(value*sign);
#endif
		SerialBT.print("X" + String(result) + ";");
		//Serial.print(result, 3); // Print result with 3 decimals
		//Serial.println(" in");
	}
	else {
		result = (value*sign) / 100.00;
#if MAX7219_DRO	>=1 

		LedDisplay(value*sign, false, Y_Readout, false, Z_Readout, false);
		//Serial.println(value*sign);
#endif
		SerialBT.print("X" + String(result) + ";");
		////Serial.print(result, 2); // Print result with 2 decimals
		//Serial.println(" mm");

	}
	//delay(10);
}
#endif
#if MAX7219_DRO >= 1

void StartUpDisplay(bool on) {

	lmd.setDecode(0x00);  // no decoding

	for (uint8_t j = 0; j <= 23; j++) {
		for (uint8_t i = 0; i <= 7; i++) {
			lmd.setPixel(j, i, on);
			lmd.display();
			delay(7);
		}
	}
}

void LedDisplay(long X_value, bool imperialX, long Y_value, bool imperialY, long Z_value, bool imperialZ) {

	// set MAX7219 register as it gets scrambled some times due lack of caps on spi bus
	lmd.setEnabled(true);
	lmd.setIntensity(2);  // 0 = min, 15 = max
	lmd.setScanLimit(7);  // 0-7: Show 1-8 digits. Beware of currenct restrictions for 1-3 digits! See datasheet.
	lmd.setDecode(0xFF);

	//lmd.setPixel(16, 2, 1); // X 2 från vänster

	
	char charbufX[9];		// Buffers for int to char conversion, it's needed for blanking zeros and create "-" dash on 7-segment


	char charbufY[9];


	char charbufZ[9];

	

	// Filling the char array's with long int value
	// sanity check for input. Must be in range of -9999999 to 9999999 and no buffer overrun I hope
	if (X_value < -9999999 || X_value > 9999999) {
		for (int i = 0; i < 8; i++) {
			charbufX[i] = '-';
		}
	}
	else dtostrf(X_value, 8, 0, charbufX);

	if (Y_value < -9999999 || Y_value > 9999999) {
		for (int i = 0; i < 8; i++) {
			charbufY[i] = '-';
		}
	}
	else dtostrf(Y_value, 8, 0, charbufY);

	if (Z_value < -9999999 || Z_value > 9999999) {
		for (int i = 0; i < 8; i++) {
			charbufZ[i] = '-';
		}
	}
	else dtostrf(Z_value, 8, 0, charbufZ);

	// Each char from array is after blanking and signing converted back to uint8_t
	uint8_t digitX;
	uint8_t digitY;
	uint8_t digitZ;

	for (int i = 0; i < 8; i++) {
		// the counter goes thru every char in the respective char array.
		// The Lib-Driver or my 7-segment are reversed in segment order so a negative counter (-i) to set the rigth place for digit

		// X-Axis
		if (imperialX) {
			if (charbufX[i] == ' ' && i < 4) lmd.setDigit(X_Last_Digit - i, LEDMatrixDriver::BCD_BLANK);
			else if (charbufX[i] == '-' && i >= 3 ) lmd.setDigit(X_Last_Digit - 3, LEDMatrixDriver::BCD_DASH);
			else if (charbufX[i] == '-' && i < 3 ) lmd.setDigit(X_Last_Digit - i, LEDMatrixDriver::BCD_DASH);
			else {
				digitX = byte(charbufX[i]); // fixed place for decimal point later in function
				lmd.setDigit(X_Last_Digit - i, digitX, false);
			}
		}
		if (!imperialX) {
			if (charbufX[i] == ' ' && i < 5 ) lmd.setDigit(X_Last_Digit - i, LEDMatrixDriver::BCD_BLANK);
			else if (charbufX[i] == '-' && i >= 4 ) lmd.setDigit(X_Last_Digit - 4, LEDMatrixDriver::BCD_DASH);
			else if (charbufX[i] == '-' && i < 4 ) lmd.setDigit(X_Last_Digit - i, LEDMatrixDriver::BCD_DASH);
			else {
				digitX = byte(charbufX[i]); // fixed place for decimal point later in function
				lmd.setDigit(X_Last_Digit - i, digitX, false);
			}
		}
		

		// Y-Axis
		if (imperialY) {
			if (charbufY[i] == ' ' && i < 4) lmd.setDigit(Y_Last_Digit - i, LEDMatrixDriver::BCD_BLANK);
			else if (charbufY[i] == '-'  && i >= 3) lmd.setDigit(Y_Last_Digit - 3, LEDMatrixDriver::BCD_DASH);
			else if (charbufY[i] == '-'  && i < 3) lmd.setDigit(Y_Last_Digit - i, LEDMatrixDriver::BCD_DASH);
			else {
				digitY = byte(charbufY[i]);
				lmd.setDigit(Y_Last_Digit - i, digitY, false);
			}
		}
		if (!imperialY) {
			if (charbufY[i] == ' ' && i < 5 ) lmd.setDigit(Y_Last_Digit - i, LEDMatrixDriver::BCD_BLANK);
			else if (charbufY[i] == '-'  && i >= 4 ) lmd.setDigit(Y_Last_Digit - 4, LEDMatrixDriver::BCD_DASH);
			else if (charbufY[i] == '-'  && i < 4 ) lmd.setDigit(Y_Last_Digit - i, LEDMatrixDriver::BCD_DASH);
			else {
				digitY = byte(charbufY[i]);
				lmd.setDigit(Y_Last_Digit - i, digitY, false);
			}
		}

		// Z-Axis
		if (imperialZ) {
			if (charbufZ[i] == ' ' && i < 4) lmd.setDigit(Z_Last_Digit - i, LEDMatrixDriver::BCD_BLANK);
			else if (charbufZ[i] == '-'  && i >= 3) lmd.setDigit(Z_Last_Digit - 3, LEDMatrixDriver::BCD_DASH);
			else if (charbufZ[i] == '-'  && i < 3) lmd.setDigit(Z_Last_Digit - i, LEDMatrixDriver::BCD_DASH);
			else {
				digitZ = byte(charbufZ[i]);
				lmd.setDigit(Z_Last_Digit - i, digitZ, false);
			}
		}
		if (!imperialZ) {
			if (charbufZ[i] == ' ' && i < 5) lmd.setDigit(Z_Last_Digit - i, LEDMatrixDriver::BCD_BLANK);
			else if (charbufZ[i] == '-'  && i >= 4 ) lmd.setDigit(Z_Last_Digit - 4, LEDMatrixDriver::BCD_DASH);
			else if (charbufZ[i] == '-'  && i < 4 ) lmd.setDigit(Z_Last_Digit - i, LEDMatrixDriver::BCD_DASH);
			else {
				digitZ = byte(charbufZ[i]);
				lmd.setDigit(Z_Last_Digit - i, digitZ, false);
			}
		}

	}
	// fixed place for decimal point
	uint8_t decimalsX = 2; 			// Default 2 decimals for metric
	if (imperialX) decimalsX = 3;	// Imperial uses 3 decimals

	uint8_t decimalsY = 2; 			// Default 2 decimals for metric
	if (imperialY) decimalsY = 3;	// Imperial uses 3 decimals

	uint8_t decimalsZ = 2; 			// Default 2 decimals for metric
	if (imperialZ) decimalsZ = 3;	// Imperial uses 3 decimals
	
	lmd.setPixel(16, 2 + imperialX, 1);
	lmd.setPixel( 8, 2 + imperialY, 1);
	lmd.setPixel( 0, 2 + imperialZ, 1);

	lmd.display();

}
#endif
