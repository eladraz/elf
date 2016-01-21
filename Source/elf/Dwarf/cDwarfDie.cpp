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

#include "elf/Dwarf/cDwarfDie.h"

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

#define ABBREV_CLOSE_TOKEN ("\x00") // I Need Double NULL Which is "\x00" and compiler add "\x00" to it.
#pragma pack(push, 1)
typedef struct DWARF_DIE_ABBREV_HEADER_TYPE_s
{
	uint8 Index;		 // Index of the
	uint8 Tag;			 // Dwarf's Version
	uint8 HasChilds;	 // Offset in abbrev table
} DWARF_DIE_ABBREV_HEADER_TYPE_t;
#pragma pack(pop)

cDwarfDie::cDwarfDie(uint8 Tag) :
	m_tag(Tag){};

cDwarfDie::~cDwarfDie()
{
	this->m_attributes.changeSize(0, true);
	this->m_childs.changeSize(0, true);
}

void cDwarfDie::setStrTable(cElfStringTablePtr & strTbl)
{
	this->m_intStrTable = strTbl;
}

void cDwarfDie::getStrTable(cBuffer & outBuff)
{
	outBuff = this->m_intStrTable->getRawData();
}

void cDwarfDie::addAttr(uint8 Attr, uint32 Value)
{
	this->m_attributes.append(cDwarfAttributePtr(new cDwarfAttribute(Attr, Value)));
}

void cDwarfDie::addAttr(uint8 Attr, const cString & Value)
{
	this->m_attributes.append(cDwarfAttributePtr(new cDwarfAttribute(Attr, Value)));
}

void cDwarfDie::getAbbRev(cBuffer & outBuff, uint8 idx)
{
	DWARF_DIE_ABBREV_HEADER_TYPE_t thisHdr = { 0 };
	cBuffer newBuff;
	cBuffer curAttrBuff;

	// Creates Header
	newBuff.changeSize(sizeof(DWARF_DIE_ABBREV_HEADER_TYPE_t));
	thisHdr.Index	  = idx;
	thisHdr.Tag		  = this->m_tag;
	thisHdr.HasChilds = (this->m_childs.getSize() > 0);
	memcpy(newBuff.getBuffer(), &thisHdr, sizeof(DWARF_DIE_ABBREV_HEADER_TYPE_t));

	// Builds Attributes List
	cArray<cDwarfAttributePtr>::iterator curAttr = this->m_attributes.begin();
	for (;curAttr != this->m_attributes.end(); ++curAttr)
	{
		(*curAttr)->getAbbRevVal(curAttrBuff);
		newBuff += curAttrBuff;
	}
	newBuff.changeSize(newBuff.getSize() + sizeof(ABBREV_CLOSE_TOKEN), true);
	memcpy(newBuff.getBuffer() + newBuff.getSize() - sizeof(ABBREV_CLOSE_TOKEN),
		   ABBREV_CLOSE_TOKEN,
		   sizeof(ABBREV_CLOSE_TOKEN));

	outBuff += newBuff;

	// TODO: Iter Childs (if have).
}

void cDwarfDie::getValuesBuff(cBuffer & outBuff)
{
	uint32 val = 0;
	uint32 valSize = 0;

	cBuffer thisBuff;

	// TODO: Continue;
	outBuff.changeSize(0, true);

	cArray<cDwarfAttributePtr>::iterator curAttr = this->m_attributes.begin();
	for (;curAttr != this->m_attributes.end(); ++curAttr)
	{
		if ( (*curAttr)->isString() ) {
			if ( NULL == this->m_intStrTable ) {
				this->m_intStrTable = cElfStringTablePtr(new cElfStringTable());
			}
			val = this->m_intStrTable->addString((*curAttr)->getStrValue());
			valSize = sizeof(uint32);
		} else {
			val = (*curAttr)->getIntValue();
			valSize = (*curAttr)->getIntSize();
		}

		thisBuff.changeSize(valSize, true);
		memcpy(thisBuff.getBuffer(), &val, valSize);
		outBuff += thisBuff;
	}
}