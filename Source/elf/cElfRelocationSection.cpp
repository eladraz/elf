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

#include "elf/cElfRelocationSection.h"

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

cElfRelocationSection::cElfRelocationSection(uint32 sectionId) :
m_sectionId(sectionId)
{
	this->m_rels.changeSize(0, true);
}

cElfRelocationSection::cElfRelocationSection( uint32 sectionId, basicIO& stream, Elf32_Shdr & sectionHdr) :
m_sectionId(sectionId)
{
	uint32 entries = 0;

	// Reads the section's data.
	entries = sectionHdr.sh_size / sizeof(Elf32_Rel);
	this->m_rels.changeSize(entries, true);
	stream.seek(sectionHdr.sh_offset, basicInput::IO_SEEK_SET);
	stream.pipeRead(this->m_rels.getBuffer(), sectionHdr.sh_size);
}

cElfRelocationSection::~cElfRelocationSection()
{
	this->m_rels.changeSize(0, true);
}

cBuffer cElfRelocationSection::getData()
{
	cBuffer rawData;
	uint curPos = 0;

	rawData.changeSize(sizeof(Elf32_Rel) * this->m_rels.getSize());
	cArray<Elf32_Rel>::iterator curRel = this->m_rels.begin();
	for(curPos = 0; curRel != this->m_rels.end(); ++curRel, curPos += sizeof(Elf32_Rel)) {
		memcpy(rawData.getBuffer() + curPos, &(*curRel), sizeof(Elf32_Rel));
	}

	return rawData;
}

void cElfRelocationSection::getHeader(Elf32_Shdr& output)
{
	output.sh_type		= SHT_REL;
	output.sh_addralign = 0x4;
	output.sh_entsize	= sizeof(Elf32_Rel);
	output.sh_size		= this->m_rels.getSize() * output.sh_entsize;
	output.sh_info		= this->m_sectionId;

	// Not mine to fill ;)
	output.sh_offset	= 0;
	output.sh_name		= 0;
	output.sh_addr		= 0;
	output.sh_flags		= 0;
	output.sh_link		= 0;
}

void cElfRelocationSection::addRel(uint32 symId, uint32 symType, uint32 offset)
{
	Elf32_Rel newRel;

	newRel.r_info   = ELF32_R_INFO(symId, symType);
	newRel.r_offset = offset;

	this->m_rels += newRel;
}

uint32 cElfRelocationSection::getSectionId()
{
	return this->m_sectionId;
}

void cElfRelocationSection::setSectionId(uint32 newSecId)
{
	this->m_sectionId = newSecId;
}

void cElfRelocationSection::removeRel(uint32 SymToRemove)
{
	int count = 0;
	int newId = 0;
	cArray<Elf32_Rel>::iterator curRel = this->m_rels.begin();

	for(count = 0; curRel != this->m_rels.end(); ++count, ++curRel) {
		if ( ELF32_R_SYM((*curRel).r_info) == SymToRemove ) {
			newId = this->m_rels.getSize()-1;
			this->m_rels[count] = this->m_rels[newId];
			this->m_rels.changeSize(newId);
		}
	}
}

void cElfRelocationSection::changeRel(uint32 SrcSym, uint32 DstSym)
{
	cArray<Elf32_Rel>::iterator curRel = this->m_rels.begin();
	for(; curRel != this->m_rels.end(); ++curRel) {
		if ( ELF32_R_SYM((*curRel).r_info) == SrcSym ) {
			(*curRel).r_info = ELF32_R_INFO(DstSym, ELF32_R_TYPE((*curRel).r_info));
		}
	}
}
