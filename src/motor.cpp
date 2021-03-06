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

#include "streaming.h"
#include "pid.hpp"
#include "motor.hpp"
#include "m5/io_m5.h"
#include "util.hpp"
#include "rotation.h"


Motors::Motors()
{
	for (int i = 0; i < DOF; i++)
		this->controllers[i].init(GAINS[i][0], GAINS[i][1], GAINS[i][2]);
	this->controllers[D].init(GAINS[D][0], GAINS[D][1], GAINS[D][2]);
	for (int i = 0; i < NUM_MOTORS; i++)
		this->thrust[i] = 0.;
	for (int i = 0; i < DOF; i++)
		this->forces[i] = 0.;
	for (int i = 0; i < DOF; i++)
		this->pid[i] = 0.;
	this->p = 0.;
}

void Motors::power()
{
	m5_power(VERT_FL, thrust[0]);
	m5_power(VERT_FR, thrust[1]);
	m5_power(VERT_BL, thrust[2]);
	m5_power(VERT_BR, thrust[3]);
	m5_power(SURGE_FL, thrust[4]);
	m5_power(SURGE_FR, thrust[5]);
	m5_power(SURGE_BL, thrust[6]);
	m5_power(SURGE_BR, thrust[7]);
	m5_power_offer_resume();
}

void Motors::pause()
{
	io_m5_trans_stop();
}

uint32_t Motors::run(float *dstate, float daltitude, float *angles, uint32_t t)
{
	// Calculate time difference since last iteration.
	uint32_t temp = micros();
	float dt = (temp - t)/(float)(1000000);

	// Calculate PID values. Third argument is minimum PID value, which allows
	// changes for small values, though it doesn't seem to affect the code for
	// now.
	pid[F] = controllers[F].calculate(dstate[F], dt, 0.20);
	pid[H] = controllers[H].calculate(dstate[H], dt, 0.20);
	for (int i = BODY_DOF; i < GYRO_DOF; i++)
		pid[i] = controllers[i].calculate(dstate[i], dt, 0.0);

	// Choose between depth from bottom or depth sensor. 
	if (daltitude < -999.)
	{
		pid[V] = controllers[V].calculate(dstate[V], dt, 0.00);
		// Serial << dstate[V] << " " << pid[V] << '\n';
	}
	else 
	{
		pid[V] = -1.*controllers[D].calculate(daltitude, dt, 0.00);
		// Serial << daltitude << " " << pid[V] << '\n';
	}

	// Default motor thrusts are 0. Add 0.15 to vertical thrusts so the sub
	// remains at the same depth.
	for (int i = 0; i < NUM_MOTORS; i++)
		thrust[i] = 0.;
	if (p > 0.01)
	{
		thrust[0] += 0.15;
		thrust[1] -= 0.15;
		thrust[2] -= 0.15;
		thrust[3] += 0.15;
	}

	// Compute final thrust given to each motor based on orientation matrix and
	// PID values.
	for (int i = 0; i < NUM_MOTORS; i++)
		for (int j = 0; j < DOF; j++) 
			thrust[i] += p*pid[j]*ORIENTATION[i][j];
	if (!SIM) power();

	// Compute forces from motors. This isn't used at the moment, though it
	// could be useful if someone wanted to integrate a simulator with Nautical
	// at some point.
	float bforces[BODY_DOF];
	float MOUNT_ANGLE = sin(45.);
	for (int i = 0; i < BODY_DOF; i++)
		bforces[i] = 0.;
	bforces[F] = MOUNT_ANGLE*(thrust[4]-thrust[5]-thrust[6]+thrust[7]);
	bforces[H] = MOUNT_ANGLE*(thrust[4]+thrust[5]+thrust[6]+thrust[7]);
	bforces[V] = thrust[0]-thrust[1]-thrust[2]+thrust[3];
	float iforces[BODY_DOF];
	body_to_inertial(bforces, angles, iforces);
	forces[F] = iforces[F];
	forces[H] = iforces[H];
	forces[V] = iforces[V]; 

	return temp;
}
