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
 * @author Stefan Unterschuetz
 */

#include "Logger.h"

#ifdef ENABLE_LOGGING
#define NUM_LOG_LEVELS 	5
const char *LOG_LEVEL[NUM_LOG_LEVELS] = {"fatal", "error", "warn", "info",
    "debug"};

#include "stringparser.h"

using namespace std;

Logger::~Logger() {
    // delete all
    for (logDescriptorMap_t::iterator it = loggers.begin(); it != loggers.end(); it++) {
        for (logDescriptorList_t::iterator it2 = it->second.begin(); it2
                != it->second.end(); it2++) {
            delete *it2;
        }
    }
}

Logger::Logger() {
    const char log_conf[] = "log.conf";
    ifstream ifs(log_conf);
    if (ifs.is_open() == false) {
        cout << "could not find logging configuration file " << log_conf << "; creating it"
        << endl;
        ofstream tmpfile(log_conf);
        return;
    }
    cout << "Reading logging config file: " << log_conf << endl;
    while (!ifs.eof()) {
        string line;
        // get line
        getline(ifs, line);

        // trim string
        trim(line, " \t");

        // check for comments or empty lines
        if (line.length() == 0 || line.substr(0, 2).compare("//") == 0 || line.substr(0, 1).compare("#") == 0) {
            continue;
        }

        // tokenize
        list<string> tokens_;
        tokenize(line, tokens_, ",", " \t\r\n");

        vector<string> tokens;
        for (list<string>::iterator it=tokens_.begin();it!=tokens_.end();it++) {
            tokens.push_back(*it);
        }

        // check stuff
        if (tokens.size() < 4) {
            cout << "<error> missing arguments: " << line << endl;
        }

        logDescriptor_t *descr = new logDescriptor_t;

        // read logger name
        if (tokens[0].compare("*") == 0) {
            descr->name = "ALL";
        } else {
            descr->name = tokens[0];
        }

        // read logger debug level
        std::transform(tokens[1].begin(), tokens[1].end(), tokens[1].begin(),
                ::tolower);
        int logLevel = NUM_LOG_LEVELS;

        for (logLevel = 0; logLevel < NUM_LOG_LEVELS; logLevel++) {
            if (tokens[1].compare(LOG_LEVEL[logLevel]) == 0) {
                break;
            }
        }
        descr->level = logLevel;

        if (logLevel == NUM_LOG_LEVELS) {
            cout << "<error> unknown log level for argument 2: " << line
            << endl;
            delete descr;
            continue;
        }
        descr->fb = NULL;
        descr->file = tokens[2];
        if (tokens[2].compare("stdout") == 0) {
            descr->useStdout = true;
        } else {
            descr->useStdout = false;
//			cout << "trying to find existing outputBuf for module " << tokens[0] << endl;
            filebuf * buf = findActiveBuf(tokens[2]);
            if (buf != NULL) {
//				cout << "using existing filebuf " << tokens[2] << "; &fb: " << (long) buf << endl;

                descr->out = new ostream(buf);
            } else {
                descr->fb = new filebuf();
                //filebuf * result =
                descr->fb->open(tokens[2].c_str(), ios::out);
//				cout << "opened logfile " << tokens[2] << "; &fb: " << (long) result << endl;
                descr->out = new ostream(descr->fb);
//				descr->out.open(tokens[2].c_str());
            }
        }

        descr->logAll = false;
//		cout << tokens[3] << endl;
        if (tokens[3].compare("*") == 0) {
            descr->logAll = true;
        } else {
            for (unsigned int i = 3; i < tokens.size(); i++) {
                if (tokens[i].compare(0, 2, "0x") == 0) {
                    int value;
                    sscanf(tokens[i].c_str(), "0x%x", &value);
                    descr->nodes.insert(value);
                } else {
                    descr->nodes.insert(atoi(tokens[i].c_str()));
                }
            }
        }

        loggers[descr->name.c_str()].push_back(descr);

    }
    cout << "Logger configured successfully "<< endl << endl << endl;
}

filebuf * Logger::findActiveBuf(string s) {
    for(logDescriptorMap_t::iterator itMap = loggers.begin();
            itMap != loggers.end(); itMap++) {
        for(logDescriptorList_t::iterator it = loggers[itMap->first].begin(); it
                != loggers[itMap->first].end(); it++) {
            if (!((*it)->useStdout)) {
//				cout << "comparing " << s << " with existing file " << (*it)->file << " for module " << (*it)->name << endl;
                if (s.compare((*it)->file) == 0) {
                    if ((*it)->fb != NULL) {
                        return (*it)->fb;
                    }
                }
            }
        }
    }
    return NULL;
}

void Logger::log(const string& name, int channel, int priority,
        const string& message) {
    {

        // abort recursion if this was called with ALL, which is always done
        // once to check if some ALL target was defined
        if (name.compare("ALL") != 0) {
            log("ALL", channel, priority, message);
        }

        for (logDescriptorList_t::iterator it = loggers[name].begin(); it
                != loggers[name].end(); it++) {
            if (priority <= (*it)->level) {
                if ((*it)->logAll || (*it)->nodes.count(channel) > 0) {
                    if ((*it)->useStdout) {
                        cout << message;
                        cout.flush();
                    } else {
                        *((*it)->out) << message;
                        ((*it)->out)->flush();
                    }
                }

            }
        }

    }
}

Logger& getLogger() {
    static Logger logger;
    return logger;

}

#endif
