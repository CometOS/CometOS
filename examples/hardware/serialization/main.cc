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

/**This class shows different techniques to create serializable
 * object in CometOS.
 *
 * @author Stefan Unterschuetz
 */

#include "cometos.h"
#include "Serializable.h"
#include "AirString.h"
#include "Tuple.h"
#include "Airframe.h"

using namespace cometos;

/*PROTOTYPES-----------------------------------------------------------------*/

/**TECHNIQUE 1
 * The common way in CometOS to create a serializable object.
 */
class MyClass1 {
public:
	uint8_t a;
	uint16_t b;
	AirString c;
};

void serialize(ByteVector& buffer, const MyClass1& value) {
	serialize(buffer,value.c);
	serialize(buffer,value.b);
	serialize(buffer,value.a);
}

void unserialize(ByteVector& buffer, MyClass1& value) {
	unserialize(buffer,value.a);
	unserialize(buffer,value.b);
	unserialize(buffer,value.c);
}

/**TECHNIQUE 2
 * The Tuple class allows the use of variadic arguments.
 * Disadvantage of the Tuple class is that the members
 * are accessed via first,second, ...
 */
typedef Tuple<uint8_t, uint16_t, AirString> MyClass2;

/**TECHNIQUE 3
 * *Deriving from Serializable.This approach createsoverhead in RAM memory,
* but less overhead in ROM (in comparison to the other techniques).
*/
class MyClass3: public Serializable {
public:
	MyClass3() :
	a(this), b(this), c(this) {
	}

	Field<uint8_t> a;
	Field<uint16_t> b;
	Field<AirString> c;
};

int main() {
	MyClass1 before1,after1;
	MyClass2 before2,after2;
	MyClass3 before3,after3;

	cometos::initialize();

	cometos::getCout() << "Start \n";

	before1.a=1;
	before1.b=1000;
	before1.c.setStr("foo");

	before2.first=2;
	before2.second=2000;
	before2.third.setStr("bar");

	before3.a=3;
	before3.b=3000;
	before3.c->setStr("blub"); // must use -> because of Field wrapper

	/* TECHNIQUE 4
	 * If objects are only used in airframes, a
	 * manual (de)serialization can be done.
	 */
	Airframe * frame= new Airframe();
	(*frame)<<before1<<before2<<before3;
	(*frame)>>after3>>after2>>after1;
	delete frame;


	cometos::getCout() <<"MyClass1 "<< (int)after1.a<<" "<<after1.b << " "<<after1.c.getStr() << "\n";
	cometos::getCout() <<"MyClass2 "<< (int)after2.first<<" "<<after2.second << " "<<after2.third.getStr() << "\n";
	cometos::getCout() <<"MyClass3 "<< (int)after3.a<<" "<<after3.b << " "<<after3.c->getStr() << "\n";


	cometos::run();
	return 0;
}
