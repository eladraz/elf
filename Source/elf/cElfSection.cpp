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

#include "elf/cElfSection.h"

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

cElfSection::cElfSection(const cString & name,
						 const cBuffer & data,
                         uint32 section_type,
                         uint32 flags,
						 uint32 nametable_index,
						 uint32 addr_align,
						 uint32 info,
						 uint32 link,
						 uint32 entry_size) :
	m_data(data),
	m_name(name)
{
	memset(&this->m_header, 0, sizeof(Elf32_Shdr));
	this->m_header.sh_type = section_type;
	this->m_header.sh_name = nametable_index;
	this->m_header.sh_flags = flags;
	this->m_header.sh_addralign = addr_align;
	this->m_header.sh_info = info;
	this->m_header.sh_link = link;
	this->m_header.sh_entsize = entry_size;
	this->m_oldCountId = -1;
}

cElfSection::cElfSection(basicIO& stream, const Elf32_Shdr & sourceHdr) :
	m_name("")
{
	memcpy(&this->m_header, &sourceHdr, sizeof(Elf32_Shdr));
	this->m_header.sh_addr = 0;
	this->m_header.sh_offset  = 0;
	this->m_oldCountId = -1;

	if ( sourceHdr.sh_type != SHT_NULL ) {
		// Creates a buffer of the section's data.
		this->m_data.changeSize(sourceHdr.sh_size, true);

		// Reads the section's data.
		stream.seek(sourceHdr.sh_offset, basicInput::IO_SEEK_SET);
		stream.pipeRead(this->m_data.getBuffer(), sourceHdr.sh_size);
	}
}

cElfSection::~cElfSection()
{
	this->m_data.changeSize(0, true);
}

void cElfSection::toStruct(Elf32_Shdr & output)
{
	// Clears the struct.
	memcpy(&output, &this->m_header, sizeof(Elf32_Shdr));
}

void cElfSection::setData(const cBuffer & newData)
{
	this->m_data = newData;
}

cBuffer & cElfSection::getData()
{
	return this->m_data;
}

void cElfSection::setType(uint32 newType)
{
	this->m_header.sh_type = newType;
}

uint32 cElfSection::getType()
{
	return this->m_header.sh_type;
}

void cElfSection::setFlags(uint32 newFlags)
{
	this->m_header.sh_flags = newFlags;
}

uint32 cElfSection::getFlags()
{
	return this->m_header.sh_flags;
}

void cElfSection::setName(const cString & name)
{
	this->m_name = name;
}

cString & cElfSection::getName()
{
	return this->m_name;
}


void cElfSection::setNameTableIndex(uint32 newIndex)
{
	this->m_header.sh_name = newIndex;
}

uint32 cElfSection::getNameTableIndex()
{
	return this->m_header.sh_name;
}

void cElfSection::setLink(uint32 newLink)
{
	this->m_header.sh_link = newLink;
}

uint32 cElfSection::getLink()
{
	return this->m_header.sh_link;
}

void cElfSection::setOldCount(int32 oldCount)
{
	this->m_oldCountId = oldCount;
}

int32 cElfSection::getOldCount()
{
	return this->m_oldCountId;
}
