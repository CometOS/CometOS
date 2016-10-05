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

#ifndef REEDSOLOMONCODING_H_
#define REEDSOLOMONCODING_H_

#include <stdint.h>
#include "BitVector.h"
#include "gf_math.h"

template<uint8_t N, uint8_t M>
class ReedSolomonCoding {
public:

	ReedSolomonCoding() {
		gf_init();
		createVandermondeMatrix();
	}

	/**
	 * Generates Vandermonde matrix for en/decoding
	 */
	void createVandermondeMatrix() {

		// column iterator
		for (uint16_t j = 0; j < N; j++) {
			uint16_t value = 1;

			// row iterator
			for (uint16_t i = 0; i < M; i++) {

				encMatrix_[i * N + j] = (uint8_t) value;
				value = gf_mult(value, j + 1);
			}
		}
	}

	/**
	 * Reconstructs all data streams using the redundant streams
	 */
	bool streamDecoding(uint8_t * const data_[N],
			const cometos::BitVector<N> &dataValidity, uint8_t * const redundancy_[M],
			const cometos::BitVector<M> &redundancyValidity, uint8_t length) {

		// create iterators
		uint8_t* data[N];
		uint8_t* redundancy[M];
		for (uint8_t i = 0; i < N; i++) {
			data[i] = data_[i];
		}
		for (uint8_t i = 0; i < M; i++) {
			redundancy[i] = redundancy_[i];
		}

		// all data streams are already valid
		if (dataValidity.count(true) == N) {
			return true;
		}

		// check if decoding is possible
		if (dataValidity.count(true) + redundancyValidity.count(true) < N) {
			return false;
		}

		// first matrix for decoding is calculated
		uint8_t A[N * N];
		uint8_t A_inv[N * N];
		uint8_t *decodingStreams[N];

		// Computation of the identity as base for A and A_inverse (for gaussian)
		for (uint8_t i = 0; i < N; i++) {
			for (uint8_t j = 0; j < N; j++) {
				A[i * N + j] = 0;
				A_inv[i * N + j] = 0;
			}
			A[i * N + i] = 1;
			A_inv[i * N + i] = 1;
		}

		uint8_t red = 0;

		// Combine A_n from encMatrix_ and identity lines
		for (uint8_t i = 0; i < N; i++) {
			// stream lost, take row from encMatrix
			if (dataValidity.get(i) == false) {
				for (; red < M; red++) {
					if (redundancyValidity.get(red) == true) {
						decodingStreams[i] = redundancy[red];
						for (uint8_t j = 0; j < N; j++) {
							A[i * N + j] = encMatrix_[red * N + j];
						}
						red++;
						break;
					}
				}
			} else {
				decodingStreams[i] = data[i];
			}
		}

		gf_inverseMatrix(A, A_inv, N);

		for (uint8_t n = 0; n < length; n++) {
			for (uint8_t i = 0; i < N; i++) {

				// only reconstruct data which is lost
				if (dataValidity.get(i) == false) {
#ifdef GF_16
					uint8_t val1 = 0;
					uint8_t val2 = 0;
					for (uint8_t j = 0; j < N; j++) {
						val1 = gf_add(val1, gf_mult(A_inv[i * N + j],
								(*decodingStreams[j]) >> 4));
						val2 = gf_add(val2, gf_mult(A_inv[i * N + j],
								(*decodingStreams[j]) & 0xF));
					}

					*data[i] = val1 << 4 | val2;
#else
					uint8_t val = 0;
					for (uint8_t j = 0; j < N; j++) {
						val = gf_add(val, gf_mult(A_inv[i * N + j],
										*decodingStreams[j]));
					}

					*data[i] = val;

#endif
					data[i]++;
				}
			}

			for (uint8_t j = 0; j < N; j++) {
				decodingStreams[j]++;
			}
		}
		return true;

	}

	void streamEncoding(const uint8_t* const data_[N],
			uint8_t* const redundancy_[M], uint8_t length) {

		// create iterators
		const uint8_t* data[N];
		uint8_t* redundancy[M];
		for (uint8_t i = 0; i < N; i++) {
			data[i] = data_[i];
		}
		for (uint8_t i = 0; i < M; i++) {
			redundancy[i] = redundancy_[i];
		}

		for (uint8_t n = 0; n < length; n++) {
			for (uint8_t i = 0; i < M; i++) {

#ifdef GF_16
				uint8_t val1 = 0;
				uint8_t val2 = 0;
				for (uint8_t j = 0; j < N; j++) {

					val1 = gf_add(val1, gf_mult(encMatrix_[i * N + j],
							(*data[j]) >> 4));
					val2 = gf_add(val2, gf_mult(encMatrix_[i * N + j],
							(*data[j]) & 0xF));
				}
				*(redundancy[i]) = val1 << 4 | val2;
#else
				uint8_t val = 0;

				for (uint8_t j = 0; j < N; j++) {
					val = gf_add(val, gf_mult(encMatrix_[i * N + j], *data[j]));
				}
				*(redundancy[i]) = val;
#endif
				redundancy[i]++;
			}
			for (uint8_t j = 0; j < N; j++) {
				data[j]++;
			}
		}

	}

	inline uint8_t getN() {
		return N;
	}

	inline uint8_t getM() {
		return M;
	}

	uint8_t encMatrix_[N * M];
};

#endif /* REEDSOLOMONCODING_H_ */
