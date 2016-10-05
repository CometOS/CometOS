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

#ifndef DUPLICATEFILTER_H_
#define DUPLICATEFILTER_H_

#include <stdint.h>
#include "Queue.h"

namespace cometos {

/**maximal supported length is 1
 *
 * */
template<class T, uint8_t SIZE>
class DuplicateFilter {
public:
	DuplicateFilter() :
		skipEntriesToDelete(0) {

	}

	bool checkAndAdd(const T &data) {

		// variant A: refreshes an entry as soon as a duplicate is detected
		bool res = history.remove(data);

		if (history.full()) {
			history.pop();
		}

		history.push(data);

		if (history.used() > skipEntriesToDelete) {
			skipEntriesToDelete++;
		}

		return res;

		// variant B: entry added once is always removed after the
		// second call to refresh
		/*if (NULL != history.get(data)) {
			return true;
		} else {
			if (history.full()) {
				history.pop();
			}
			history.push(data);
			skipEntriesToDelete++;
			return false;
		}*/
	}

	/**
	 * This methods allows an elimination of old entries.
	 * Each entry in the history is deleted after the second
	 * call of this method refresh since the packet was queued.
	 * Example: refresh is called each 5 seconds. This yields
	 * to an length of stay of 5 to 10 seconds. The former occurs
	 * if directly after queuing an entry refresh is executed.
	 * Latter (10 seconds time of stay) if the entry is queued
	 * directly after the call of refresh.
	 * An advantage in comparison to a solution with TTL (Time To Live)
	 * fields is the small memory overhead.
	 * Note that the owner of an instance of this class is responsible
	 * for deleting old entries.
	 */
	void refresh() {
		uint8_t entriesToDelete = history.used() - skipEntriesToDelete;

		while (!history.empty() && entriesToDelete > 0) {
			history.pop();
			entriesToDelete--;
		}

		skipEntriesToDelete = 0;
	}

private:
	Queue<T, SIZE> history;
	uint8_t skipEntriesToDelete;

};


}

#endif /* DUPLICATEFILTER_H_ */
