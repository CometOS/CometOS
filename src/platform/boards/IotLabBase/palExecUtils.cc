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

#include "palExecUtils.h"
#include "logging.h"
#include "cmsis_device.h"

#ifdef CFS
#include "cfs.h"
#endif


#include <string.h>


uint8_t palExec_getResetReason(){
    return(0);
}

uint32_t palExec_getPcAtReset(){

#ifdef CFS
	int32_t fd = cfs_open("._pc", CFS_READ);

	if (fd < 0)
	{
		LOG_ERROR("Could not open file to read program counter");
		return 0;
	}

	uint32_t pc;

	cfs_read(fd, &pc, 4);
	cfs_close(fd);

	return pc;
#else
	return 0;
#endif
}

bool palExec_writePc(uint8_t * pc){
#ifdef CFS
	int32_t fd = cfs_open("._pc", CFS_READ | CFS_WRITE);

	if (fd < 0)
	{
		LOG_ERROR("Could not open file to write program counter");
		return false;
	}
	cfs_write(fd, pc, 4);
	cfs_close(fd);

	return true;
#else
	return false;
#endif
}

void palExec_clearResetStatus(){

}

bool palExec_readAssertInfoShort(uint16_t & line, uint16_t & fileId){
#ifdef CFS
	int32_t fd = cfs_open("._assertShort", CFS_READ);

	if (fd < 0)
	{
		line = 0;
		fileId = 0;
		return false;
	}

	uint8_t la[2];
	uint8_t fa[2];

	cfs_read(fd, la, 2);
	cfs_read(fd, fa, 2);

	line = la[0] + (la[1] << 8);
	fileId = fa[0] + (fa[1] << 8);
	cfs_close(fd);

	return true;
#else
	return false;
#endif
}

bool palExec_readAssertInfoLong(uint16_t & line, char * filename, uint8_t fileLen){
#ifdef CFS
	int32_t fd = cfs_open("._assertLong", CFS_READ);

	if (fd < 0)
	{
		line = 0;
		filename[0] = 0;
		return false;
	}
	cfs_read(fd, &line, 2);
	cfs_read(fd, filename, fileLen);

	cfs_close(fd);

	return true;
#else
	return false;
#endif
}

bool palExec_writeAssertInfoShort(uint16_t line, uint16_t fileId){
#ifdef CFS
	int32_t fd = cfs_open("._assertShort", CFS_READ | CFS_WRITE);

	if (fd < 0)
	{
		LOG_ERROR("Could not open file to write assert short");
		return false;
	}
	uint8_t lineArray[2];
	lineArray[0] = line & 0xFF;
	lineArray[1] = line >> 8;
	uint8_t fileArray[2];
	fileArray[0] = fileId & 0xFF;
	fileArray[1] = fileId >> 8;

	cfs_write(fd, lineArray, 2);
	cfs_write(fd, fileArray, 2);

	// add another byte to prevent a 0 byte at last position, which kills coffee
	uint8_t dummyEnd = 0xAB;
	cfs_write(fd, &dummyEnd, 1);

	cfs_close(fd);

	return true;
#else
	return false;
#endif
}

bool palExec_writeAssertInfoLong(uint16_t line, const char * filename){

#ifdef CFS
	int32_t fd = cfs_open("._assertLong", CFS_READ | CFS_WRITE);

	if (fd < 0)
	{
		LOG_ERROR("Could not open file to write assert long");
		return false;
	}
	cfs_write(fd, &line, 2);
	cfs_write(fd, filename, strlen(filename) + 1);

	cfs_close(fd);
	return true;
#else
	return false;
#endif
}

bool palExec_clearAssertInfo(){
#ifdef CFS
	cfs_remove("._asserLong");
	cfs_remove("._assertShort");

	return true;
#else
	return false;
#endif
}

void palExec_badisr(){

}

/**
 * In case of no error, this function always resets the node.
 */
cometos_error_t palExec_reset(){
	NVIC_SystemReset();
    return(COMETOS_SUCCESS);
}

extern long __stack;
extern long __Main_Stack_Limit;

/* never tested
void enableStackProtection() {
	// Disable MPU
	MPU->CTRL &= ~MPU_CTRL_ENABLE_Msk;

	MPU->RNR  = 0;
	MPU->RBAR = ((size_t)&__Main_Stack_Limit);// Address
	MPU->RASR =  (4 << MPU_RASR_SIZE_Pos)     // 2*(SIZE+1) -> 32 Byte
				 | (0 << MPU_RASR_AP_Pos)     // No access allowed
				 | MPU_RASR_XN_Msk            // XN bit
				 | MPU_RASR_ENA_Msk;          // Enable

	// Enable memory fault exception
	SCB->SHCSR |= SCB_SHCSR_MEMFAULTENA_Msk;

	// Enable MPU
	MPU->CTRL |= MPU_CTRL_PRIVDEFENA_Msk | MPU_CTRL_ENABLE_Msk;
}
*/

uint32_t palExec_getStackSize() {
    auto msp = __get_MSP();
    auto stackSize = ((size_t)&__stack) - msp;
    if(((size_t)&__Main_Stack_Limit) >= msp) {
        cometos::getCout() << "0x" << cometos::hex << msp << " 0x" << ((size_t)&__Main_Stack_Limit) << " " << cometos::dec << stackSize << cometos::endl;
        ASSERT(false); // Stack Overflow!
    }
    return stackSize;
}
