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
#define UPDATE_FREQUENCY 10				//  Times per second TouhDRO is updated in Hz)

#define SCALE_X_ENABLED		1
#define X_CPI				254000
#define SCALE_X_TYPE		1			/* 0 = Simulation, 1 = Chinese Caliper, 2 = iGaging */
#define SCALE_X_CLK_PIN		17
#define SCALE_X_DATA_PIN	16


#define SCALE_Y_ENABLED		1
#define Y_CPI				254000

#define SCALE_Z_ENABLED		1
#define Z_CPI				254000

#define SCALE_W_ENABLED		1
#define W_CPA				100			//xxth of a degree

#define TACH_ENABLED		1
#define TACH_MAX_SPEED		20000      

#define PROBE_ENABLED		1
#define PROBE_INVERTED		0


long X_Readout;
long Y_Readout;
long Z_Readout;
long W_Readout;
long T_Readout;

int X_StepSize = 1000;			// Step size for the scales simulator 
int Y_StepSize = 1000;
int Z_StepSize = 1000;
int W_StepSize = 10;
int T_StepSize = 100;

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


// Define Functions below here or use other .ino or cpp files
//

// The setup() function runs once each time the micro-controller starts
void setup()
{
	Serial.begin(115200);

	SerialBT.begin("ESP32testDRO"); //Bluetooth device name
	Serial.println("The device started, now you can pair it with bluetooth!");


#if SCALE_X_ENABLED == 1
	#if SCALE_X_TYPE == 0
		X_Readout = random(X_CPI * -10, X_CPI * 10);  // +- 10"
	#endif

	#if SCALE_X_TYPE == 1
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
	Serial.print("Y" + String(Y_Readout) + ";");
	Y_Readout = Y_Readout + Y_StepSize;
	if (Y_Readout >= 12 * Y_CPI || Y_Readout <= -12 * Y_CPI) Y_StepSize = Y_StepSize * -1;             // switch direction at endings
#endif

#if SCALE_Z_ENABLED == 1
	SerialBT.print("Z" + String(Z_Readout) + ";" );
	Serial.print("Z" + String(Z_Readout) + ";");
	Z_Readout = Z_Readout + Z_StepSize;
	if (Z_Readout >= 12 * Z_CPI || Z_Readout <= -12 * Z_CPI) Z_StepSize = Z_StepSize * -1;             // switch direction at endings
#endif

#if SCALE_W_ENABLED == 1
	SerialBT.print("W" + String(W_Readout) + ";" );
	Serial.print("W" + String(W_Readout) + ";");
	W_Readout = W_Readout + W_StepSize;
	if (W_Readout >= 360 * W_CPA || W_Readout <= 0) W_StepSize = W_StepSize * -1;             // switch direction at endings
#endif

#if TACH_ENABLED == 1
	SerialBT.print("T" + String(T_Readout) + ";" );
	Serial.print("T" + String(T_Readout) + ";");
	T_Readout = T_Readout + T_StepSize;
	if (T_Readout >= TACH_MAX_SPEED || T_Readout <= 0) T_StepSize = T_StepSize * -1;             // switch direction at endings
#endif

	Serial.println("");					// Newline

	delay(1000 / UPDATE_FREQUENCY);

}

void decode() {
	int sign = 1;
	int i = 0;
	float value = 0.0;
	float result = 0.0;
	bit_array[i] = digitalRead(SCALE_X_DATA_PIN); // Store the 1st bit (start bit) which is always 1.
	while (digitalRead(SCALE_X_CLK_PIN) == HIGH) {};
	for (i = 1; i <= 24; i++) {
		while (digitalRead(SCALE_X_CLK_PIN) == LOW) {} // Wait until clock returns to HIGH
		bit_array[i] = digitalRead(SCALE_X_DATA_PIN);
		while (digitalRead(SCALE_X_CLK_PIN) == HIGH) {} // Wait until clock returns to LOW
	}
	for (i = 0; i <= 24; i++) { // Show the content of the bit array. This is for verification only.
		Serial.print(bit_array[i]);
		Serial.print(" ");
	}
	Serial.println();
	for (i = 1; i <= 20; i++) { // Turning the value in the bit array from binary to decimal.
		value = value + (pow(2, i - 1) * bit_array[i]);
	}
	if (bit_array[21] == 1) sign = -1; // Bit 21 is the sign bit. 0 -> +, 1 => -
	if (bit_array[24] == 1) { // Bit 24 tells the measureing unit (1 -> in, 0 -> mm)
		result = (value*sign) / 2000.00;
		SerialBT.print("X" + String(result) + ";");
		Serial.print(result, 3); // Print result with 3 decimals
		Serial.println(" in");
	}
	else {
		result = (value*sign) / 100.00;
		SerialBT.print("X" + String(result) + ";");
		Serial.print(result, 2); // Print result with 2 decimals
		Serial.println(" mm");
	}
	//delay(10);
}
