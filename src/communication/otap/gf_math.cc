/*
 * CometOS --- a component-based, extensible, tiny operating system
 *             for wireless networks
 *
 * Copyright (c) 2015, Institute of Telematics, Hamburg University of Technology
 * All rights reserved.

 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the Institute nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include "gf_math.h"

#ifdef GF_16
#define PRIM_POLY	(023)
#define ELEMENTS	(16)
#else
#define PRIM_POLY	(0435)
#define ELEMENTS	(256)
#endif

static uint8_t gflog[ELEMENTS];
static uint8_t gfilog[ELEMENTS];
static bool initialized = false;

void gf_init() {
	if (initialized == true) {
		return;
	}
	initialized = true;

	uint16_t prim_poly = PRIM_POLY;

	uint16_t b = 1;
	for (uint8_t log = 0; log < ELEMENTS - 1; log++) {
		gflog[b] = log;
		gfilog[log] = b;
		b = b << 1;
		if (b & ELEMENTS)
			b = b ^ prim_poly;
	}

}

uint8_t gf_mult(uint8_t a, uint8_t b) {
	uint16_t sum;
	if (a == 0 || b == 0)
		return 0;
	sum = gflog[a] + gflog[b];
	if (sum >= ELEMENTS - 1)
		sum -= (ELEMENTS - 1);
	return gfilog[sum];
}

uint8_t gf_div(uint8_t a, uint8_t b) {
	int16_t diff;
	if (a == 0)
		return 0;
	if (b == 0)
		return 0; // invalid operation
	diff = gflog[a] - gflog[b];
	if (diff < 0)
		diff += (ELEMENTS - 1);
	return gfilog[diff];
}

uint8_t gf_add(uint8_t a, uint8_t b) {
	return a ^ b;
}

uint8_t gf_sub(uint8_t a, uint8_t b) {
	return a ^ b;
}

void gf_inverseMatrix(uint8_t *A, uint8_t *A_inv, uint8_t n) {
	uint8_t diag_ele;
	uint8_t k;
	uint8_t gf_multiplic;

	// we use Gaussian Elimination
	// for every row
	for (uint8_t i = 0; i < n; i++) {
		// diagonal-element has to be one
		diag_ele = A[i * n + i];

		for (uint8_t j = 0; j < n; j++) {
			// gf_divide every element of the line by diagonal-element - also the inverse has to be changed
			A[i * n + j] = gf_div(A[i * n + j], diag_ele);
			A_inv[i * n + j] = gf_div(A_inv[i * n + j], diag_ele);
		}

		for (uint8_t j = 0; j < n; j++) {
			if (i != j) {
				// check for zero
				if (A[j * n + i] == 0) {
					// zero at the right place
					;
				} else {

					// subtract line i*gf_mult(factor for bringing the element j,i to zero) from j so that line j has the a zero at row i
					// because diagonal elements = 1 the line j row i is the gf_multiplier gf_mult
					// subtract in inverse too

					gf_multiplic = A[j * n + i];
					for (k = 0; k < n; k++) {
						A[j * n + k] = gf_sub(A[j * n + k], gf_mult(
								A[i * n + k], gf_multiplic));
						A_inv[j * n + k] = gf_sub(A_inv[j * n + k], gf_mult(
								A_inv[i * n + k], gf_multiplic));
					}
				}
			}
		}
	}
}

