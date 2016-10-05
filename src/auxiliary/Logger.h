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
 * @author Stefan Unterschütz
 */

#ifndef LOGGER_H_
#define LOGGER_H_

#include "cometos.h"

#ifdef ENABLE_LOGGING
#include "palId.h"
#include <iostream>
#include <fstream>
#include <vector>
#include <algorithm>
#include <cctype>
#include <string>
#include <set>
#include <map>
#include <list>


typedef std::set<int> logNodeSet_t;

typedef struct LgDescriptor {
	LgDescriptor() :
		out(NULL),
		fb(NULL)
	{}

	~LgDescriptor() {
		if (fb!=NULL) {
			delete(fb);
		}
		if (out != NULL) {
			delete(out);
		}
	}

	std::string name;
	bool logAll;
	bool useStdout;
	std::ostream *out;
	std::filebuf *fb;
	std::string file;
	logNodeSet_t nodes;
	int level;


} logDescriptor_t;

typedef std::list<logDescriptor_t*> logDescriptorList_t;

typedef std::map<std::string, logDescriptorList_t> logDescriptorMap_t;

class Logger {

public:

	Logger();
	virtual ~Logger();

	void log(const std::string& name, int channel, int priority,
			const std::string& message);

private:
	std::filebuf * findActiveBuf(std::string s);
	logDescriptorMap_t loggers;
};


Logger& getLogger();

#endif /* ENABLE_LOGGING */

#endif /* LOGGER_H_ */
