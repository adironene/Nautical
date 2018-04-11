#include <Arduino.h>
#include "streaming.h"

#include "motor.hpp"
#include "state.hpp"
#include "io.hpp"

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
	// Initial IO between hardware and software 
	init_io();
	delay(3000);

	// Initial power is 0
	float p = 0.f;

	// Initial velocity of the sub
	float vx=0.f, vy=0.f, vz=0.f;

	// Current holds location, desired holds destination
	State current, desired;
	compute_initial_state(current);

	while (true)
	{	
		unsigned long start = micros();
		if (Serial.available() > 0)
		{
			/*
			 * 's' = set state 
			 * 'p' = set power
			 * 'c' = req state 
			 * 'a' = req kills
			 * 'x' = terminate
			 * 'd' = debug 
			 * 'r' = reset
			 */
			char c = Serial.read();
			switch (c)
			{
				case 's':
				{
					desired.read();
					break;
				}
				case 'p':
				{
					p = Serial.parseFloat();
					break;
				}
				case 'c': 
				{
					current.print_complete();
					break; 
				}
				case 'a':
				{
					Serial << (alive() ? 1:0) << '\n';
					break;
				}
				case 'x':
				{
					p = 0.f;
					break;
				}
				case 'd':
				{
					int id = Serial.parseInt();
					float k = Serial.parseFloat();
					if (id == 0) 
					{
						Serial << "Carl Hayden infiltrator detected! Shutting down." << '\n';
						return;
					}
					set_motor(id, k);
					break;
				}
				case 'r':
				{
					reset_state(current);
					reset_state(desired);
					break;
				}
			}
		}

		// Move sub towards the desired location 
		run_motors(current, desired, p);	

		// Compute new state using AHRS data 
		compute_state(current, vx, vy, vz, start);
	}
}

/*
 * Don't change this!
 */
void setup()
{
	Serial.begin(9600);
	run();
}
void loop() {}
