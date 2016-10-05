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

/**
 * Galois field arithmetic for uint8_t
 */

#ifndef GF_MATH_H_
#define GF_MATH_H_

#include <stdint.h>
#include <stdbool.h>

/**
 * uncomment if you want to use a polynomial length of 4 bit (16 elements) instead of 8 for
 * GF operations
 * Memory demand is divided by 16, execution time is doubled !
 * If using GF_16 all passed values have to be between 0 - 15 !
 * For ReedSolomon the number of parallel devices is also limited by 16
 */
#define GF_16


#ifdef __cplusplus
extern "C" {
#endif

/**
 * Initializes needed lookup tables
 */
void gf_init();

/**
 * Multiplication
 */
uint8_t gf_mult(uint8_t a, uint8_t b);

/**
 * Division
 */
uint8_t gf_div(uint8_t a, uint8_t b);

/**
 * Addition
 */
uint8_t gf_add(uint8_t a, uint8_t b);

/**
 * Subtraction
 */
uint8_t gf_sub(uint8_t a, uint8_t b);

/**
 * Calculates inverse of square matrix.
 *
 * @param A			matrix for which to calculate inverse. Note that after calling
 * 					this function A becomes an identity matrix!
 * @param A_inv		has to be identity matrix !
 * @param n			number of rows/ columns
 */
void gf_inverseMatrix(uint8_t *A, uint8_t *A_inv, uint8_t n);

#ifdef __cplusplus
}
#endif

#endif /* GF_MATH_H_ */
