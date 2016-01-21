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

#ifndef  __ELF_ELFRELOCATIONSECTION_H__
#define  __ELF_ELFRELOCATIONSECTION_H__
/*
 * cElfRelocationSection.h
 *
 * ELF Relocation Section Class
 * Relocations needs to be written in a diffret section for
 * for each section, let say we need to make relocations for .text section, so
 * we are going to be .rel.text section.
 *
 *  Author: Hagai Cohen <hagai.co@gmail.com>
 */


#include "xStl/types.h"
#include "xStl/os/streamMemoryAccesser.h"
#include "xStl/data/datastream.h"
#include "xStl/except/exception.h"
#include "xStl/except/trace.h"
#include "xStl/stream/memoryAccesserStream.h"

#include "elf/cElfConsts.h"
#include "elf/cElfSymbol.h"


class cElfRelocationSection
{
public:
	// CTor
	// Initialize new section
	cElfRelocationSection(uint32 sectionId);
	// Initialize from exsiting section
	cElfRelocationSection(uint32 sectionId, basicIO& stream, Elf32_Shdr & sectionHdr);

	// DTor
	~cElfRelocationSection();

	//
	// Gets data for the section
	//
	cBuffer getData();

	//
	// Gets the header for the section
	//
	void getHeader(Elf32_Shdr& output);

	//
	// Adds new Rel Struct.
	//
	void addRel(uint32 symId, uint32 symType, uint32 offset);

	// getter/setter for section id
	uint32 getSectionId();
	void setSectionId(uint32 newSecId);

	// This function will remove target symbol by id.
	void removeRel(uint32 SymToRemove);

	// this method will change target symbol referance by id.
	void changeRel(uint32 SrcSym, uint32 DstSym);

private:
	uint32 m_sectionId;
	cArray<Elf32_Rel> m_rels;
};

typedef cSmartPtr<cElfRelocationSection> cElfRelocationSectionPtr;


#endif //  __ELF_ELFRELOCATIONSECTION_H__
