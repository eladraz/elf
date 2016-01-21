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

#include "elf/cElfMemoryMapper.h"

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

cElfMemoryMapper::cElfMemoryMapper(cArray<cElfSectionPtr> fileSections, uint32 dataOffset) :
m_sections(fileSections),
m_dataStartOffset(dataOffset)
{
	this->buildMapping();
}

cElfMemoryMapper::~cElfMemoryMapper()
{
	this->m_sections.changeSize(0, true);
	this->m_sectionsBlob.changeSize(0, true);
	this->m_sectionHeaders.changeSize(0, true);
	this->m_fileContnet.changeSize(0, true);
}

void cElfMemoryMapper::init()
{
	this->m_namesSectionIdx     = -1;
	this->m_totalDataSize		= 0;
	this->m_sectionsStartOffset = 0;
	this->m_sectionsTableOffset = 0;

	this->m_sectionsBlob.changeSize(0, true);
	this->m_sectionHeaders.changeSize(0, true);
	this->m_fileContnet.changeSize(0, true);
}

void cElfMemoryMapper::processSections()
{
	Elf32_Shdr curSectionHdr;
	cBuffer	   curSectionData;
	int		sections_count = 0;
	int		namesIndex = 0;
	uint32 totalDataCount = 0;
	cString sectionName;

	// First of all i need to write a temp ELF Header which i replace later,
	// for now i'll use 0.
	cArray<cElfSectionPtr>::iterator i = this->m_sections.begin();
	for (; i != this->m_sections.end(); ++i)
	{
		sections_count ++;
		// Convert to struct.

		(*i)->toStruct(curSectionHdr);
		curSectionData = (*i)->getData();
		if ( curSectionHdr.sh_type == SHT_NULL ) {
			this->m_sectionHeaders += curSectionHdr;
			continue;
		}

		curSectionHdr.sh_offset = totalDataCount;
		curSectionHdr.sh_size	= curSectionData.getSize();

		// Copying the header to the headers buffer.
		this->m_sectionHeaders += curSectionHdr;
		if ( curSectionHdr.sh_type != SHT_NOBITS ) { // NOBITS Sections shouls be without data.
			totalDataCount		   += curSectionHdr.sh_size;
			this->m_sectionsBlob   += curSectionData;
		}

		sectionName = (*i)->getName();
		if ( sectionName == cElfConsts::SECTIONS_STRING_TABLE_DEFAULT_NAME() ) {
			this->m_namesSectionIdx = sections_count-1;
		}
	}

	// update total count.
	this->m_totalDataSize = totalDataCount;
}

void cElfMemoryMapper::calcOffsets()
{
	uint32 sectionHeaderSize = this->m_sectionHeaders.getSize() * sizeof(Elf32_Shdr);

	// sections table after programs
	this->m_sectionsTableOffset = this->m_dataStartOffset;

	// sections data after that till the end of the file.
	this->m_sectionsStartOffset = (this->m_sectionsTableOffset + sectionHeaderSize);

	// Fix Addresses as well now that i know offsets
	cArray<Elf32_Shdr>::iterator i = this->m_sectionHeaders.begin();
	for (; i != this->m_sectionHeaders.end(); ++i)
	{
		if ( (*i).sh_type == SHT_NULL ) {
			continue;
		}

		// Fix offset for data.
		(*i).sh_offset += this->m_sectionsStartOffset;
		(*i).sh_addr  = 0;
	}
}

cBuffer cElfMemoryMapper::getSectionHeadersTableBlob()
{
	cBuffer output_blob;
	uint32 count = 0;

	output_blob.changeSize(this->m_sectionHeaders.getSize() * sizeof(Elf32_Shdr));
	cArray<Elf32_Shdr>::iterator i = this->m_sectionHeaders.begin();
	for (; i != this->m_sectionHeaders.end(); ++i) {
		memcpy(output_blob.getBuffer() + (sizeof(Elf32_Shdr) * count), &(*i), sizeof(Elf32_Shdr));
		count++;
	}

	return output_blob;
}

cBuffer cElfMemoryMapper::getSectionsDataBlob()
{
	return this->m_sectionsBlob;
}

void cElfMemoryMapper::buildFile()
{
	this->m_fileContnet.changeSize(0, true);
	this->m_fileContnet += this->getSectionHeadersTableBlob();
	this->m_fileContnet += this->getSectionsDataBlob();
}

void cElfMemoryMapper::buildMapping()
{
	// First of all, Init.
	this->init();

	// First proccess upon sections
	this->processSections();

	// Now that i know everything i can calc offsets.
	this->calcOffsets();

	// will build the file itself.
	this->buildFile();
}

void cElfMemoryMapper::setDataStartOffset(uint32 newOffset)
{
	this->m_dataStartOffset = newOffset;
	this->buildMapping();
}

uint32 cElfMemoryMapper::getDataStartOffset()
{
	return this->m_dataStartOffset;
}

uint32 cElfMemoryMapper::getNamesSectionIndex()
{
	return this->m_namesSectionIdx;
}

uint32 cElfMemoryMapper::getSectionsTableOffset()
{
	return ( this->getSectionsTableCount() == 0 ) ? 0 : this->m_sectionsTableOffset;
}

uint32 cElfMemoryMapper::getSectionsTableCount()
{
	return this->m_sectionHeaders.getSize();
}

cBuffer cElfMemoryMapper::getFileContent()
{
	return this->m_fileContnet;
}