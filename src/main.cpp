/*
 * MIT License
 * 
 * Copyright (c) 2019 AVBotz
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 * ========================================================================== */

#include <Arduino.h>
#include "ahrs/ahrs.h"
#include "dvl/dvl.h"
#include "streaming.h"
#include "config.h"
#include "kalman.hpp"
#include "motor.hpp"
#include "util.hpp"
#include "pid.hpp"
#include "io.hpp"
#include "rotation.h"
#include "voltage.hpp"


void run()
{
	if (!SIM) io();
	if (!SIM) ahrs_att_update();

	float INITIAL_YAW, INITIAL_PITCH, INITIAL_ROLL; 
	if (USE_INITIAL_HEADING)
		INITIAL_YAW = ahrs_att((enum att_axis) (YAW));
	else 
		INITIAL_YAW = FAR ? 225. : 340.;
	INITIAL_PITCH = ahrs_att((enum att_axis) (PITCH));
	INITIAL_ROLL = ahrs_att((enum att_axis) (ROLL));

	bool alive_state = alive();
	bool alive_state_prev = alive_state;
	bool pause = false;
	uint32_t pause_time;

	Motors motors;

	Kalman kalman;
	float state[N] = { 0.000, 0.000, 0.000, 0.000, 0.000, 0.000 };
	float covar[N*N] = {	
		1.000, 0.000, 0.000, 0.000, 0.000, 0.000,
		0.000, 1.000, 0.000, 0.000, 0.000, 0.000,
		0.000, 0.000, 1.000, 0.000, 0.000, 0.000,
		0.000, 0.000, 0.000, 1.000, 0.000, 0.000,
		0.000, 0.000, 0.000, 0.000, 1.000, 0.000,
		0.000, 0.000, 0.000, 0.000, 0.000, 1.000,
	};

	float current[DOF] = { 0., 0., 0., 0., 0., 0. };
	float desired[DOF] = { 0., 0., 0., 0., 0., 0. };
	float altitude;
	float desired_altitude = -1.;

	float dstate[DOF] = { 0., 0., 0., 0., 0., 0. };
	float daltitude;

	uint32_t ktime = micros();
	uint32_t mtime = micros();

	while (true)
	{
		if (!SIM) ahrs_att_update();

		if (DVL_ON && !SIM) dvl_data_update();

		if (Serial.available() > 0)
		{
			char c = Serial.read();

			if (c == 'a')
			{
				Serial << alive() << '\n';
			}
			else if (c == 'c')
			{
				for (int i = 0; i < DOF; i++)
					Serial << _FLOAT(current[i], 6) << ' '; Serial << '\n';
			}
			else if (c == 'd')
			{
				for (int i = 0; i < DOF; i++)
					Serial << _FLOAT(desired[i], 6) << ' '; Serial << '\n';
			}
			else if (c == 'p')
			{
				motors.p = Serial.parseFloat();
				if (motors.p < 0.05)
				{
					for (int i = 0; i < NUM_MOTORS; i++)
					{
						motors.thrust[i] = 0.;
						motors.buttons[i] = 0;
					}
				}
			}
			else if (c == 's')
			{
				for (int i = 0; i < DOF; i++)
					desired[i] = Serial.parseFloat();
			}
			else if (c == 'z')
			{
				desired_altitude = Serial.parseFloat();
			}
			else if (c == 'w')
			{
				Serial << altitude << '\n';
			}
			else if (c == 'r')
			{
				for (int i = 0; i < DOF; i++)
					desired[i] = current[i];
				float temp1[3];
				float temp2[3] = {current[Y], current[P], current[R]};
				for (int i = 0; i < BODY_DOF; i++)
					temp1[i] = Serial.parseFloat();
				float temp[3];
				body_to_inertial(temp1, temp2, temp);
				for (int i = 0; i < BODY_DOF; i++)
					desired[i] += temp[i];
				for (int i = BODY_DOF; i < GYRO_DOF; i++)
					desired[i] = angle_add(current[i], Serial.parseFloat());
			}
			else if (c == 'h' && !SIM)
			{
				Serial << ahrs_att((enum att_axis) (YAW)) << '\n';
			}
			else if (c == 'x' && !SIM)
			{
				state[0] = 0.;
				state[3] = 0.;
				desired[F] = 0.;
				desired[H] = 0.;
				desired[V] = 0.;
				desired[Y] = 0.;
				current[F] = 0.;
				current[H] = 0.;
				current[Y] = 0.;
				INITIAL_YAW = ahrs_att((enum att_axis) (YAW));
			}
			else if (c == 'f')
			{
				for (int i = 0; i < DOF; i++)
					Serial << _FLOAT(motors.forces[i], 6) << ' '; Serial << '\n';
			}
			else if (c == 'v') 
			{
				float v = voltage();
				Serial << v << '\n';
			}
			else if (c == 'w')
			{
				for (int i = 0; i < NUM_MOTORS; i++)
					motors.buttons[i] = Serial.parseInt();
				if (motors.p > 0.01) 
				{
					// Relative distance of 10 ensures max speed is used.
					float temp1[3] = {0, 0, 0};
					float temp2[3] = {current[Y], current[P], current[R]};
					if (motors.buttons[0] == 1)
						temp1[0] = 10.;
					if (motors.buttons[1] == 1)
						temp1[1] = -10.;
					if (motors.buttons[2] == 1)
						temp1[0] = -10.;
					if (motors.buttons[3] == 1)
						temp1[1] = 10.;
					if (motors.buttons[4] == 1)
						desired[Y] = angle_add(desired[Y], -10.);
					if (motors.buttons[5] == 1)
						desired[Y] = angle_add(desired[Y], 10.);
					float temp[3];
					body_to_inertial(temp1, temp2, temp);
					desired[F] = current[F] + temp[0];
					desired[H] = current[H] + temp[1];
				}
			}
			else if (c == 'g')
			{
				int idx = Serial.parseInt();
				int val = Serial.parseInt();
				drop(idx, val);
			}
			else if (c == 't')
			{
				for (int i = 0; i < 8; i++)
					Serial << _FLOAT(motors.thrust[i], 6) << " "; Serial << '\n';
			}
		}

		alive_state_prev = alive_state;
		alive_state = alive();

		// Enough time has elapsed for motors to start up. Don't forget to reset
		// time so the time difference for the first set of velocities from the
		// DVL are correct.
		if (pause && millis() - pause_time > PAUSE_TIME && !SIM)
		{
			pause = false;
			mtime = micros();
			ktime = micros();
		}

		// Kill switch has just been switched from alive to dead. Pause motor
		// communications. Until the sub has been set back to alive, none of
		// these "if" statements will run.
		if (alive_state_prev && !alive_state && !SIM)
		{
			motors.pause();
			// Serial << "Desired states being reset." << endl;
		}

		// Kill switch has just been switched from dead to alive. Reset all
		// values, including states and initial headings. Pause so motors have
		// time to start up. 
		if (!alive_state_prev && alive_state && !SIM)
		{ 
			state[0] = 0.;
			state[3] = 0.;
			desired[F] = 0.;
			desired[H] = 0.;
			desired[V] = 0.;
			desired[Y] = 0.;
			desired_altitude = -1.;
			current[F] = 0.;
			current[H] = 0.;
			current[Y] = 0.;
			if (USE_INITIAL_HEADING)
				INITIAL_YAW = ahrs_att((enum att_axis) (YAW));
			else 
				INITIAL_YAW = FAR ? 225. : 340.;
			INITIAL_PITCH = ahrs_att((enum att_axis) (PITCH));
			INITIAL_ROLL = ahrs_att((enum att_axis) (ROLL));
			pause = true;
			pause_time = millis();
			// Serial << "Current states being reset." << endl;
		}

		// Motors are done starting up and the sub is alive. Run the sub as
		// intended.
		if (SIM || (!pause && alive_state))
		{
			// Compute angles from AHRS and depth from pressure sensor.
			if (!SIM)
			{
				current[V] = (analogRead(DEPTH_PIN)-230.)/65.;
				current[Y] = ahrs_att((enum att_axis) (YAW)) - INITIAL_YAW;
				// current[P] = ahrs_att((enum att_axis) (PITCH)) - INITIAL_PITCH;
				// current[R] = ahrs_att((enum att_axis) (ROLL)) - INITIAL_ROLL;
				current[P] = ahrs_att((enum att_axis) (PITCH));
				current[R] = ahrs_att((enum att_axis) (ROLL));
				if (DVL_ON)
					altitude = dvl_get_range_to_bottom()/10000.;

				// Handle angle overflow/underflow.
				for (int i = BODY_DOF; i < GYRO_DOF; i++)
					current[i] += (current[i] > 180.) ? -360. : (current[i] < -180.) ? 360. : 0.;
			}
			float temp[3] = { current[Y], current[P], current[R] };

			// Kalman filter removes noise from measurements and estimates the new
			// state. Assume angle is 100% correct so no need for EKF or UKF.
			ktime = kalman.compute(state, covar, temp, ktime);

			// Use KF for N and E components of state. 
			current[F] = state[0];
			current[H] = state[3];

			// Change heading if desired state is far. Turned off for now
			// because we want to rely on DVL > AHRS.
			/*
			if (fabs(desired[F]-current[F]) > 3. || fabs(desired[H]-current[H] > 3.))
				desired[Y] = atan2(desired[H]-current[H], desired[F]-current[F]) * R2D;
			desired[Y] += (desired[Y] > 360.) ? -360. : (desired[Y] < 0.) ? 360. : 0.;
			*/

			// Compute the state difference. Change heading first if the error 
			// is high. Make depth changes regardless. 
			for (int i = 0; i < DOF; i++)
				dstate[i] = 0.;
			float d0 = desired[F] - current[F];
			float d1 = desired[H] - current[H];
			float i0 = d0*cos(D2R*current[Y]) + d1*sin(D2R*current[Y]);
			float i1 = d1*cos(D2R*current[Y]) - d0*sin(D2R*current[Y]);
			if (fabs(angle_difference(desired[Y], current[Y])) > 5.)
			{
				dstate[Y] = angle_difference(desired[Y], current[Y]);
			}
			else if (fabs(i1) > 2.)
			{
				dstate[Y] = angle_difference(desired[Y], current[Y]);
				dstate[F] = i0 < i1/3. ? i0 : i1/3.;
				dstate[H] = i1; 
			}
			else 
			{
				dstate[Y] = angle_difference(desired[Y], current[Y]);
				dstate[F] = i0;
				dstate[H] = i1;
			}
			dstate[V] = desired[V] - current[V];
			daltitude = desired_altitude > 0. ? desired_altitude-altitude : -9999.;

			// Compute PID within motors and set thrust.
			mtime = motors.run(dstate, daltitude, temp, mtime);
		}
	}
}

void setup()
{
	Serial.begin(9600);
	run();
}
void loop() {}
