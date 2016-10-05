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

#include "SurmAllocator.h"

#ifdef MEMORY_THREAD_SAFE
#include "atomic.h"
#endif

namespace cometos {

/****************************
 * Partitionsaufbau
 ****************************
 * Code | Size | NumElements
 *   0  | 128  |     1
 *   1  |  64  |     2
 *   2  |  32  |     4
 *   3  |  16  |     8
 *   4  |   8  |    16
 */

inline uint8_t SurmAllocator::GetPartitionElementSize(uint8_t PartitionCode)
{
	// 0 => 128, 1 => 64, usw.
	return 128 >> PartitionCode;
}

inline uint8_t SurmAllocator::GetNumPartitionElements(uint8_t PartitionCode)
{
	// 0 => 1, 1 => 2, 2 => 4, usw.
	return 1 << PartitionCode;
}

inline CList<CListElement>* SurmAllocator::GetPartitionList(uint8_t PartitionCode)
{
	return &SubLists[PartitionCode-1];
}


SurmAllocator::SurmAllocator()
{
	// TODO A BUG,  had to change >= to <=
	ASSERT(sizeof(CListElement) <= 8);
	ASSERT(sizeof(CDataListElement) <= 8);

	for (int i=0;i<MEMORY_HEAP_SIZE;i++) {
		Memory[i]=0;
	}

	MainList.Init(129);

	NumSubLists = 4;  // Bei 4-Byte-Zeiger auf 4 Listen setzen

	SubLists[0].Init(64);
	SubLists[1].Init(32);
	SubLists[2].Init(16);
	SubLists[3].Init(8);

	uint16_t NumElements = MEMORY_HEAP_SIZE / MainList.GetElementSize();

	for (int i=NumElements-1; i>=0; i--)
	{
		MainList.AddAsFirst((uint8_t*)Memory + i * MainList.GetElementSize());
	}
}


CDataListElement* SurmAllocator::GetMainElement(CListElement* Element)
{
	uint16_t Diff = (uint8_t*)Element - (uint8_t*)Memory;
	Diff = (Diff / MainList.GetElementSize()) * MainList.GetElementSize();

	return (CDataListElement*) ((uint8_t*)Memory + Diff);
}

void* SurmAllocator::allocate(size_t size)
{
    ASSERT(Size <=255);

	CListElement* Element = NULL;

	bool NumAllocatedPartsAlreadySet = false;

	for (int i=NumSubLists-1;i>=0;i--)
	{
		if (Size <= SubLists[i].GetElementSize())
		{
			Element = SubLists[i].RemoveFirstElement();
			if (!Element) 
			{
				// Element aus MainList holen und zerteilen
				CDataListElement* MainElement = MainList.RemoveFirstElement();
				if (!MainElement) return NULL;

				uint8_t PartitionCode = i+1;

				NumAllocatedPartsAlreadySet = true;
				MainElement->SetNumAllocatedParts(1);
				MainElement->SetPartitionCode(PartitionCode);

				uint8_t NumNewElements = GetNumPartitionElements(PartitionCode);// 1 << (i+1);
				for (int j=NumNewElements-1;j>=0;j--)
				{
					void* ElementPtr = (uint8_t*)MainElement->GetDataPtr() + j * SubLists[i].GetElementSize();
					SubLists[i].AddAsFirst(ElementPtr);
				}

				Element = SubLists[i].RemoveFirstElement();
			}
			break;
		}
	}

	if (Element) 
	{
		if (!NumAllocatedPartsAlreadySet)
		{
			// �bergeordnetes Element bestimmen und Unterteilungen erh�hen
			CDataListElement* MainElement = GetMainElement(Element);
			MainElement->SetNumAllocatedParts(MainElement->GetNumAllocatedParts() + 1);
		}

		return Element;
	}
	else
	{
		if (Size <= MainList.GetElementSize() - 1)
		{
			CDataListElement* MainElement = MainList.RemoveFirstElement();
			if (!MainElement) return NULL;

			MainElement->SetNumAllocatedParts(1);
			MainElement->SetPartitionCode(0);
			return MainElement->GetDataPtr();
		}
	}

	return NULL;
}

void SurmAllocator::deallocate(void* DataPtr)
{
	if (!DataPtr) return;

	CListElement* Element = (CListElement*)DataPtr;

	// �bergeordnetes Element bestimmen
	CDataListElement* MainElement = GetMainElement(Element);

	uint8_t NumAllocatedParts = MainElement->GetNumAllocatedParts();
	uint8_t PartitionCode	  = MainElement->GetPartitionCode();

	if (PartitionCode == 0)
	{
		// Dies ist ein Main-Element!
		// An Frei-Liste anf�gen und fertig.
		MainList.AddAsFirst(MainElement);
		return;
	}

	if (NumAllocatedParts == 1)
	{
		// Alle anderen Teile aus ihren Frei-Listen entfernen
		uint8_t	 ElementSize  = GetPartitionElementSize(PartitionCode);
		uint8_t	 ElementCount = GetNumPartitionElements(PartitionCode);
		uint8_t* First		  = (uint8_t*)MainElement->GetDataPtr();

		CList<CListElement>* List = GetPartitionList(PartitionCode);
		
		for (int i=0;i<ElementCount;i++)
		{
			CListElement* Cur = (CListElement*)( First + i*ElementSize );
			if (Cur == Element) continue;

			List->Remove(Cur);
		}
		
		// Main-Element an Frei-Liste anf�gen
		MainList.AddAsFirst(MainElement);
	}
	else
	{
		// Z�hler verringern
		MainElement->SetNumAllocatedParts(NumAllocatedParts - 1);

		// Element in Frei-Liste einf�gen
		CList<CListElement>* List = GetPartitionList(PartitionCode);
		List->AddAsFirst(Element);
	}
}

uint16_t SurmAllocator::GetMemAllocated()
{
	return 0;
}

uint16_t SurmAllocator::GetMemUsed()
{
	return 0;
}





// ensures that MemoryManager is initialized
static SurmAllocator &memory() {
	static SurmAllocator mem;
	return mem;
}

uint8_t heapGetUtilization() {
	return 0;
}

} // namespace

#include "memory_wrapper.h"

