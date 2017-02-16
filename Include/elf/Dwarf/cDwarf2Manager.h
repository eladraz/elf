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

#ifndef  __ELF_DWARF2MANAGER_H__
#define  __ELF_DWARF2MANAGER_H__
/*
 * cDwarf2Manager.h
 *
 * ELF's Debug info manager
 * used to create debug information about the elf.
 *
 *  Author: Hagai Cohen <hagai.co@gmail.com>
 */

#include "elf/cElfConsts.h"
#include "elf/opensource_headers/dwarf.h"
#include "elf/Dwarf/cDwarfDie.h"

#include "xStl/types.h"
#include "xStl/os/streamMemoryAccesser.h"
#include "xStl/data/datastream.h"
#include "xStl/except/exception.h"
#include "xStl/except/trace.h"
#include "xStl/stream/memoryAccesserStream.h"

class cDwarf2Manager
{
public:
	cDwarf2Manager(const cString & producer,
				   const cString & fileName,
				   const cString & filePath,
				   uint8 pointerSize,
				   uint32 codeSectionSize);

	~cDwarf2Manager();

	//
	// The core DWARF data
	// containing DIEs
	//
	void getDebugInfoSec(cBuffer & outBuf);

	//
	// Abbreviationsused in the
	// .debug_info section
	//
	void getDebugAbbSec(cBuffer & outBuf);

	//
	// String table used by
	// .debug_info
	//
	void getDebugStrSec(cBuffer & outBuf);

	//
	// Line Number Program
	//
	void getDebugLineSec(cBuffer & outBuf);

	//
	// Macro descriptions
	//
	void getDebugLocSec(cBuffer & outBuf);

	//
	// A lookup table for global
	// objects and functions
	//
	void getDebugPubNamesSec(cBuffer & outBuf);

	//
	// A lookup table for global
	// types
	//
	void getDebugPubTypesSec(cBuffer & outBuf);

	//
	// A mapping between
	// memory address and
	// compilation
	//
	void getDebugArrangesSec(cBuffer & outBuf);

	//
	// Call Frame Information
	//
	void getDebugFrameSec(cBuffer & outBuf);

private:
	const static uint16 m_DwarfVersion = 2; // Implmention of Dwarf ver 2.
	cDwarfDiePtr m_mainCU;

	cString m_fileName;
	cString m_filePath;
	cString m_producer;
	uint8	m_ptrSize;
};

typedef cSmartPtr<cDwarf2Manager> cDwarf2ManagerPtr;
#endif // __ELF_DWARF2MANAGER_H__
