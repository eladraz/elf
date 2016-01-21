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

#ifndef  __ELF_DWARFATTR_H__
#define  __ELF_DWARFATTR_H__
/*
 * cDwarfAttribute.h
 *
 *  Author: Hagai Cohen <hagai.co@gmail.com>
 */

#include "elf/cElfConsts.h"
#include "elf/opensource_headers/dwarf.h"

#include "xStl/types.h"
#include "xStl/os/streamMemoryAccesser.h"
#include "xStl/data/datastream.h"
#include "xStl/except/exception.h"
#include "xStl/except/trace.h"
#include "xStl/stream/memoryAccesserStream.h"

class cDwarfAttribute
{
public:
	// Should be Initialize with AttrType (DW_AT_*) From dwarf.h
	cDwarfAttribute(uint32 AttrType, const cString & Value);
	cDwarfAttribute(uint32 AttrType, uint32 Value);
	~cDwarfAttribute();

	bool isString();
	cString getStrValue();
	uint32 getIntValue();
	uint32 getIntSize();
	void getAbbRevVal(cBuffer & outBuff);
private:
	uint32 m_type;
	uint32 m_uintVal;
	cString m_strVal;

	uint8 decideType();
};

typedef cSmartPtr<cDwarfAttribute> cDwarfAttributePtr;
#endif // __ELF_DWARFATTR_H__
