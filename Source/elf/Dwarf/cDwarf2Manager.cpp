/*
 * Copyright (c) 2008-2016, Integrity Project Ltd. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * Neither the name of the Integrity Project nor the names of its contributors
 * may be used to endorse or promote products derived from this software
 * without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE
 */

#include "elf/Dwarf/cDwarf2Manager.h"

// TODO: Remove them, debug only
#include "xStl/types.h"
#include "xStl/data/char.h"
#include "xStl/data/string.h"
#include "xStl/except/trace.h"
#include "xStl/except/exception.h"
#include "xStl/except/assert.h"
#include "xStl/stream/traceStream.h"
#include "xStl/stream/fileStream.h"
#include "xStl/stream/ioStream.h"
#include "xStl/os/virtualMemoryAccesser.h"
#include "xStl/os/threadUnsafeMemoryAccesser.h"
#include "xStl/stream/memoryAccesserStream.h"

#pragma pack(push, 1)
typedef struct DWARF_HEADER_TYPE_s
{
	uint32 Length;		 // Length of the CU's include this header (AFTER This field)
	uint16 Version;		 // Dwarf's Version
	uint32 AbbRevOffset; // Offset in abbrev table
	uint8  PtrSize;		 // How much bytes is pointer in this processor
	uint8  cuIndex;		 // Index of the CU
} DWARF_HEADER_TYPE_t;
#pragma pack(pop)

cDwarf2Manager::cDwarf2Manager( const cString & producer,
								const cString & fileName,
								const cString & filePath,
								uint8 pointerSize,
								uint32 codeSectionSize) :
	m_ptrSize(pointerSize),
	m_csSize(codeSectionSize)
{
	this->m_mainCU = cDwarfDiePtr(new cDwarfDie(DW_TAG_compile_unit));
	this->m_mainCU->addAttr(DW_AT_producer, producer);
	this->m_mainCU->addAttr(DW_AT_language, DW_LANG_C_plus_plus);
	this->m_mainCU->addAttr(DW_AT_name, fileName);
	this->m_mainCU->addAttr(DW_AT_comp_dir, filePath);
	this->m_mainCU->addAttr(DW_AT_low_pc, 0);
	this->m_mainCU->addAttr(DW_AT_high_pc, codeSectionSize);
}

cDwarf2Manager::~cDwarf2Manager()
{}

void cDwarf2Manager::getDebugInfoSec(cBuffer & outBuf)
{
	DWARF_HEADER_TYPE_t hdr = { 0 };
	cBuffer DataBuffer;
	cBuffer endingNull;
	endingNull.changeSize(1, true);
	memset(endingNull.getBuffer(), '\0', 1);

	// Get Values.
	this->m_mainCU->getValuesBuff(DataBuffer);
	DataBuffer += endingNull;

	hdr.Length		 = sizeof(DWARF_HEADER_TYPE_t) - 4; // SKIP Length
	hdr.Length		 += DataBuffer.getSize();			// Adds DataBuffer's Size to that.
	hdr.Version		 = this->m_DwarfVersion;			// Sets DwarfVersion.
	hdr.AbbRevOffset = 0;								// We do 1 file means 1 CU means we will be first all the times.
	hdr.PtrSize		 = m_ptrSize;						// Pointer size of the arch.
	hdr.cuIndex		 = 1;								// Always first cu.

	// Writes Header
	outBuf.changeSize(sizeof(hdr), true);
	memcpy(outBuf.getBuffer(), &hdr, sizeof(hdr));
	outBuf += DataBuffer;

}

void cDwarf2Manager::getDebugAbbSec(cBuffer & outBuf)
{
	cBuffer endingNull;
	endingNull.changeSize(1, true);
	memset(endingNull.getBuffer(), '\0', 1);

	outBuf.changeSize(0, true);
	this->m_mainCU->getAbbRev(outBuf);
	outBuf += endingNull;
}

void cDwarf2Manager::getDebugStrSec(cBuffer & outBuf)
{
	outBuf.changeSize(0, true);
	this->m_mainCU->getStrTable(outBuf);
}

void cDwarf2Manager::getDebugLineSec(cBuffer & outBuf)
{
	//Stub
	outBuf.changeSize(0, true);
}

void cDwarf2Manager::getDebugLocSec(cBuffer & outBuf)
{
	//Stub
	outBuf.changeSize(0, true);
}

void cDwarf2Manager::getDebugPubNamesSec(cBuffer & outBuf)
{
	//Stub
	outBuf.changeSize(0, true);
}

void cDwarf2Manager::getDebugPubTypesSec(cBuffer & outBuf)
{
	//Stub
	outBuf.changeSize(0, true);
}

void cDwarf2Manager::getDebugArrangesSec(cBuffer & outBuf)
{
	//Stub
	outBuf.changeSize(0, true);
}

void cDwarf2Manager::getDebugFrameSec(cBuffer & outBuf)
{
	//Stub
	outBuf.changeSize(0, true);
}