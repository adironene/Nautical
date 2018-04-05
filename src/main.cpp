#include <Arduino.h>

#include "streaming.h"
#include "state.hpp"
#include "io.hpp"
#include "m5/m5.h"

/*
 * IMPORTANT: The entire structure of this program hacks around the required
 * functions for .ino files. The reason for this is to make serial communication
 * much easier with Serial.begin() and to incorporate helpful Arduino functions.
 */

/*
 * Treat this as main().
 */
void run()
{
	Serial << "Beginning Nautical!" << '\n';
	
	/*
	 * Current state holds where the sub thinks it is at.
	 * Desired state holds where the sub wants to be.
	 */
	State current, desired;

	while (true)
	{
		if (Serial.available() > 0)
		{
			/*
			 * 's' = set state 
			 * 'p' = set power
			 * 'c' = req state 
			 * 'a' = req kills
			 */
			char c = Serial.read();

			switch (c)
			{
				case 's':
				{
					desired.read();
					desired.print();
					break;
				}
				case 'x':
				{
					float powers[NUM_THRUSTERS] = {0.f};
					//kill motors
					setpowers(powers);
					break;
				}
				case 'p':
				{
					float k = Serial.parseFloat();
					float powers[NUM_THRUSTERS] = {k};
					setpowers(powers);
					Serial << k << "\n";
					break;
				}
				case 't':
				{
					int x = Serial.parseInt();
					float k = Serial.parseFloat();
					Motor blah = {(enum thruster)x, k};
					setMotor(blah);
					Serial << x << " " << k << "\n";
					break;
				}
				case 'c': 
				{	
					current = getState();
					current.print();
					break; 
				}
				case 'a':
				{
					Serial << alive() ? 1:0 << "\n";
					break;
				}
			}
		}
	}
}

/*
 * Don't change this!
 */
void setup()
{
	Serial.begin(9600);
	init_io();
	run();
}
void loop() {}
