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

#include "matrix.h"


void print(double *A, int m, int n)
{
	for (int r = 0; r < m; r++)
	{
		for (int c = 0; c < n; c++)
			Serial.print(A[n*r+c]);
		Serial.print("\n");
	}
}

void identity(double *A, int n)
{
	for (int r = 0; r < n; r++)
		for (int c = 0; c < n; c++)
			A[r*n+c] = (r == c) ? 1. : 0.;
}

void copy(float *A, int n, int m, float *B)
{
	for (int r = 0; r < n; r++)
		for (int c = 0; c < n; c++)
			B[r*n+c] = A[r*n+c];
}

void multiply(float *A, float *B, int m, int p, int n, float *C)
{
	for (int r = 0; r < m; r++)
	{
		for (int c = 0; c < n; c++)
		{
			C[n*r+c] = 0.;
			for (int k = 0; k < p; k++)
				C[n*r+c] += A[p*r+k] * B[n*k+c];
		}
	}
}

void add(float *A, float *B, int m, int n, float *C)
{
	for (int r = 0; r < m; r++)
		for (int c = 0; c < n; c++)
			C[n*r+c] = A[n*r+c] + B[n*r+c];
}

void subtract(float *A, float *B, int m, int n, float *C)
{
	for (int r = 0; r < m; r++)
		for (int c = 0; c < n; c++)
			C[n*r+c] = A[n*r+c] - B[n*r+c];
}

void transpose(float *A, int m, int n, float *B)
{
	for (int r = 0; r < m; r++)
		for (int c = 0; c < n; c++)
			B[m*c+r] = A[n*r+c];
}

void scale(float *A, int m, int n, float k)
{
	for (int r = 0; r < m; r++)
		for (int c = 0; c < n; c++)
			A[n*r+c] = k*A[n*r+c];
}

int invert(float *A, int n, float *B)
{
	identity(B, n);

	for (int i = 0; i < n; i++)
	{
		double k = A[i*n+i];
		for (int c = 0; c < n; c++)
		{
			A[i*n+c] /= k;
			B[i*n+c] /= k;
		}

		for (int r = 0; r < n; r++)
		{
			if (r == i) continue;

			k = A[r*n+i];
			for (int c = 0; c < n; c++)
			{
				A[r*n+c] -= k*A[i*n+c];
				B[r*n+c] -= k*B[i*n+c];
			}
		}
	}
}
