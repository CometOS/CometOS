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

#ifndef SURM_ALLOCATOR_H
#define SURM_ALLOCATOR_H

#include <stdint.h>
#include <stdlib.h>

#include "logging.h"
#include "memory.h"


namespace cometos {


class CListElement
{
public:
	CListElement*	Next;
	CListElement*	Prev;
};


class CDataListElement : public CListElement
{
	uint8_t* GetControlData()
	{
		return (uint8_t*)this;
	}

public:
	void*	GetDataPtr()
	{
		return (uint8_t*)this + 1;
	}

	uint8_t	GetNumAllocatedParts()
	{
		return (*GetControlData() & 0xF) + 1;
	}

	void SetNumAllocatedParts(uint8_t NumParts)
	{
		ASSERT(NumParts > 0);

		*GetControlData() &= ~0xF;
		*GetControlData() |= NumParts - 1;
	}

	uint8_t	GetPartitionCode()
	{
		return (*GetControlData() & 0xF0) >> 4;
	}

	void SetPartitionCode(uint8_t PartitionCode)
	{
		ASSERT(PartitionCode >= 0);
		ASSERT(PartitionCode <= 4);

		*GetControlData() &= ~0xF0;
		*GetControlData() |= PartitionCode << 4;
	}
};


template <typename ElementType>
class CList
{
	ElementType* FirstElement;
	uint8_t		 ElementSize;

public:
	void Init(uint8_t ElementSize)
	{
		this->ElementSize = ElementSize;
		FirstElement = NULL;
	}

	void AddAsFirst(void* BlockPos)
	{
		ElementType* CurElement = (ElementType*)BlockPos;

		CurElement->Prev = NULL;

		if (FirstElement)
		{
			CurElement  ->Next = FirstElement;
			FirstElement->Prev = CurElement;
		}
		else
		{
			CurElement->Next = NULL;
		}

		FirstElement = CurElement;
	}

	ElementType* GetFirstElement()
	{
		return FirstElement;
	}

	ElementType* RemoveFirstElement()
	{
		if (!FirstElement) return NULL;

		ElementType* First = FirstElement;

		FirstElement = (ElementType*)First->Next;

		if (FirstElement)
			FirstElement->Prev = NULL;

		return First;
	}

	uint8_t GetElementSize()
	{
		return ElementSize;
	}

	void Remove(ElementType* Element)
	{
		ASSERT(FirstElement);

		ElementType* Prev = Element->Prev;
		ElementType* Next = Element->Next;

		if (Prev) 
		{
			Prev->Next = Next;
		}
		else
		{
			ASSERT(Element == FirstElement);
			FirstElement = Next;
		}

		if (Next) Next->Prev = Prev;
	}
};


class SurmAllocator
{
	inline CList<CListElement>* GetPartitionList		(uint8_t PartitionCode);
	inline uint8_t				GetNumPartitionElements	(uint8_t PartitionCode);
	inline uint8_t				GetPartitionElementSize	(uint8_t PartitionCode);

	CDataListElement* GetMainElement(CListElement* Element);

public:
	uint8_t	 Memory[MEMORY_HEAP_SIZE];
	uint8_t	 NumSubLists;

	CList<CDataListElement> MainList;
	CList<CListElement>		SubLists[5];

	SurmAllocator();

	void*	 allocate(size_t size);
	void	 deallocate(void* DataPtr);
	uint16_t GetMemAllocated();
	uint16_t GetMemUsed();
};

} // namespace

#endif // SURM_ALLOCATOR_H
