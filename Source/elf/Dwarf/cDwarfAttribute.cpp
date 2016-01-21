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

#include "elf/Dwarf/cDwarfAttribute.h"

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
typedef struct DWARF_ATTR_ABBREV_HEADER_TYPE_s
{
	uint8 AttrName;		 // Index of the
	uint8 AttrType;		 // Dwarf's Version
} DWARF_ATTR_ABBREV_HEADER_TYPE_s;
#pragma pack(pop)

cDwarfAttribute::cDwarfAttribute(uint32 AttrType, const cString & Value) :
	m_type(AttrType),
	m_uintVal(0),
	m_strVal(Value){};
cDwarfAttribute::cDwarfAttribute(uint32 AttrType, uint32 Value) :
	m_type(AttrType),
	m_uintVal(Value),
	m_strVal(""){};

bool cDwarfAttribute::isString()
{
	if (this->m_strVal.length() > 0) {
		return true;
	}

	return false;
}

cString cDwarfAttribute::getStrValue()
{
	return this->m_strVal;
}

uint32 cDwarfAttribute::getIntValue()
{
	return this->m_uintVal;
}

void cDwarfAttribute::getAbbRevVal(cBuffer & outBuff)
{
	DWARF_ATTR_ABBREV_HEADER_TYPE_s abrevHdr = { 0 };

	outBuff.changeSize(sizeof(DWARF_ATTR_ABBREV_HEADER_TYPE_s), true);
	abrevHdr.AttrName = this->m_type;
	abrevHdr.AttrType = this->decideType();
	memcpy(outBuff.getBuffer(), &abrevHdr, sizeof(DWARF_ATTR_ABBREV_HEADER_TYPE_s));
}

uint32 cDwarfAttribute::getIntSize()
{
	if ( this->m_uintVal < 0xFF ) {
		return sizeof(uint8);
	}

	if  ( this->m_uintVal < 0xFFFF ) {
		return sizeof(uint16);
	}

	return sizeof(uint32);
}

uint8 cDwarfAttribute::decideType()
{
	if ( this->isString() ) {
		return DW_FORM_strp;
	}

	switch (this->getIntSize())
	{
		case sizeof(uint8):
			return DW_FORM_data1;
		case sizeof(uint16):
			return DW_FORM_data2;
		case sizeof(uint32):
			return DW_FORM_data4;
	}

	return 0;
}

cDwarfAttribute::~cDwarfAttribute(){}