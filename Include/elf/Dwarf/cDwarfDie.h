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

#ifndef  __ELF_DWARFDIE_H__
#define  __ELF_DWARFDIE_H__
/*
 * cDwarfDie.h
 *
 *  Author: Hagai Cohen <hagai.co@gmail.com>
 */

#include "elf/cElfConsts.h"
#include "elf/opensource_headers/dwarf.h"
#include "elf/Dwarf/cDwarfAttribute.h"
#include "elf/cElfStringTable.h"

#include "xStl/types.h"
#include "xStl/os/streamMemoryAccesser.h"
#include "xStl/data/datastream.h"
#include "xStl/except/exception.h"
#include "xStl/except/trace.h"
#include "xStl/stream/memoryAccesserStream.h"

// Declare Ptr first so i can make a tree.
class cDwarfDie;
typedef cSmartPtr<cDwarfDie> cDwarfDiePtr;

class cDwarfDie
{
public:
	// Should be Initialize with tag (DW_TAG_*) From dwarf.h
	cDwarfDie(uint8 Tag);
	~cDwarfDie();

	// Adds Int Attribute
	void addAttr(uint8 Attr, uint32 Value);

	// Adds String Attribute
	void addAttr(uint8 Attr, const cString & Value);

	void getAbbRev(cBuffer & outBuff,
				   uint8 idx = 1);

	void getValuesBuff(cBuffer & outBuff);

	// Set Str Table all childs can have the same strtable.
	void setStrTable(cElfStringTablePtr & strTbl);

	// Outputs built strTable.
	void getStrTable(cBuffer & outBuff);
private:
	uint8 m_tag;
	uint8 m_idxCnt;
	cElfStringTablePtr m_intStrTable;

	cArray<cDwarfAttributePtr> m_attributes;
	cArray<cDwarfDiePtr> m_childs;
};
#endif // __ELF_DWARFDIE_H__
