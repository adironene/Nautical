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

/** @file kalman.hpp
 *  @brief Kalman filter struct and constant definitions.
 *
 *  The state of the Kalman filter can be described by the following vector: 
 *  [X VX AX Y VY AY]
 *
 *  The main measurement instrument used for this Kalman filter is a DVL, which
 *  returns VX and VY. The exact variances of the DVL are on the spreadsheet.
 *  
 *  @author David Zhang
 */
#ifndef KALMAN_HPP
#define KALMAN_HPP

/** N represents the number of elements in the state, while M represents the
 *  number of sensors. 
 */
static const int N = 6;
static const int M = 2;

/** Qk describes how accurate the model is. The variance of each element of the
 *  should be along the main diagonal of the matrix.
 */
static float Qk[N*N] = {
	0.005, 0.000, 0.000, 0.000, 0.000, 0.000,
	0.000, 0.005, 0.000, 0.000, 0.000, 0.000,
	0.000, 0.000, 0.005, 0.000, 0.000, 0.000,
	0.000, 0.000, 0.000, 0.005, 0.000, 0.000,
	0.000, 0.000, 0.000, 0.000, 0.005, 0.000,
	0.000, 0.000, 0.000, 0.000, 0.000, 0.005
};

/** Hk maps the predicted state to measurements. 
 */
static float Hk[M*N] = {
 	0.000, 1.000, 0.000, 0.000, 0.000, 0.000,
	0.000, 0.000, 0.000, 0.000, 1.000, 0.000
};

/** Rk describes the precision of each measurement.
 */
static float Rk[M*M] = {
	5.000, 0.000,
	0.000, 5.000
};

/** @brief Struct to make using the Kalman filter easier.
 */
struct Kalman
{
	//@{
	/** Supposed to be used for accelerometer bias removal, unused for now. */
	int skip, iter;
	float m_orig[M];
	float m_bias[M];
	//@}
	
	Kalman();

	/** @brief Removes bias from accelerometer data.
	 */
	void bias();

	/** @brief Computes one iteration of the Kalman filter.
	 *
	 *  @param state The current state of the sub using the Kalman state model.
	 *  @param covar The current error of the state.
	 *  @param angles The current euler angles of the sub.
	 *  @param t The current time of system, used to compute time differences.
	 *  @return The end time of the system.
	 */
	uint32_t compute(float *state, float *covar, float *angles, uint32_t t);
};

#endif 
