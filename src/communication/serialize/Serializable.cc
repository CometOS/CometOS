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

#include "Serializable.h"


#ifndef NO_SERIALIZABLE_WRAPPER

LinkedOffsetList::LinkedOffsetList() :
		next(0), prev(0) {
}

LinkedOffsetList * LinkedOffsetList::getNext() {
	return ((LinkedOffsetList *) (((uint8_t*) this) + next));
}

LinkedOffsetList * LinkedOffsetList::getPrev() {
	return ((LinkedOffsetList *) (((uint8_t*) this) + prev));
}
bool LinkedOffsetList::checkOffset(intptr_t offset) {
	if (offset < INT8_MIN || offset > INT8_MAX) {
		return false;
	}
	return true;
}

void LinkedOffsetList::insertAfter(LinkedOffsetList* item) {
	LinkedOffsetList* p = getNext();
	link(this, item);
	link(item, p);
}

void LinkedOffsetList::link(LinkedOffsetList* first, LinkedOffsetList* second) {
	int tmp = ((uint8_t*) (second)) - ((uint8_t*) (first));
	ASSERT(checkOffset(tmp));
	ASSERT(checkOffset(-tmp));
	first->next = (int8_t) tmp;
	second->prev = (int8_t) -tmp;
}

FieldBase::FieldBase(FieldBase* prev_) {
	prev_->insertAfter(this);

}


Serializable::Serializable() :
		FieldBase(this) {
}

Serializable::Serializable(FieldBase* prev_) :
		FieldBase(prev_) {
}

void Serializable::serialize_(cometos::ByteVector& buffer) {
	FieldBase *start = this;
	FieldBase *it = this;

	while (it->getNext() != start) {
		it = (FieldBase*) it->getNext();
		it->serialize_(buffer);
	}

}

void Serializable::deserialize_(cometos::ByteVector& buffer) {
	FieldBase *start = this;
	FieldBase *it = this;

	while (it->getPrev() != start) {
		it = (FieldBase*) it->getPrev();
		it->deserialize_(buffer);
	}
}

void serialize(cometos::ByteVector& buffer, const Serializable& value) {
	const_cast<Serializable&>(value).serialize_(buffer);
}

void unserialize(cometos::ByteVector& buffer, Serializable& value) {
	value.deserialize_(buffer);
}

#endif
