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

/*
 * @author Andreas Weigel
 */

#ifndef STATISTICS_H_
#define STATISTICS_H_

#include "stdint.h"

template<typename T, typename numT, typename sumT, typename sqrSumT>
class Sums {
public:
    Sums() :
        sum(0),
        sqrSum(0),
        len(0)
    {}

    bool reset(){
        sum = 0;
        sqrSum = 0;
        len = 0;
        return true;
    }

    bool add(T value) {
        sqrSumT tmp = ((sqrSumT)value) * value;
        if (sqrSum + tmp >= sqrSum) {
            sum += value;
            sqrSum += tmp;
            len++;
            return true;
        } else {
            return false;
        }
    }

    sumT getSum() const {
        return sum;
    }

    sqrSumT getSqrSum() const {
        return sqrSum;
    }

    numT n() const {
        return len;
    }

    numT maxN() const {
        return (1 << (8 * sizeof(numT))) - 1;
    }

    sumT sum;
    sqrSumT sqrSum;
    numT len;
};

template<typename T, typename numT, typename sumT, typename sqrSumT>
class SumsMinMax : public Sums<T, numT, sumT, sqrSumT> {
public:
    SumsMinMax() :
        Sums<T, numT, sumT, sqrSumT>(),
        min(0),
        max(0),
        numMinValues(0),
        init(false)
    {}

    bool reset() {
        if (Sums<T, numT,  sumT, sqrSumT>::reset()) {
            init = false;
            return true;
        } else {
            return false;
        }
    }

    bool add(T value) {
        if (Sums<T, numT, sumT, sqrSumT>::add(value)) {
            if (init) {
                if (value < min) {
                    min = value;
                    numMinValues = 0;
                }
                max = value > max ? value : max;
            } else {
                min = value;
                max = value;
                init = true;
            }
            if (value == min) {
                numMinValues++;
            }
            return true;
        } else {
            return false;
        }
    }

    T getMin() const {
        return min;
    }

    numT getNumMinValues() const {
        return numMinValues;
    }

    T getMax() const {
        return max;
    }

    bool isValid() {
        return init;
    }

    T min;
    T max;
    numT numMinValues;
    bool init;
};


#endif
