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

#ifndef  __ELF_ELFMEMMAPPER_H__
#define  __ELF_ELFMEMMAPPER_H__
/*
 * cElfMemoryMapper.h
 *
 * ELF File Memory Mapper
 * This is special class that should help to map the memory
 * The class will do so by "walking" on the sections
 * and match the best mapping.
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
#include "elf/cElfSection.h"

class cElfMemoryMapper
{
public:
	// The C'Tor will recive the list of sections
	// and should build the program's header list.
	cElfMemoryMapper(cArray<cElfSectionPtr> fileSections,
					 uint32 dataOffset = sizeof(Elf32_Ehdr));

	~cElfMemoryMapper();

	// Setter/Getter for base Address.
	void setBaseAddress(uint32 newBase);
	uint32 getBaseAddress();

	// Setter/Getter for data start.
	void setDataStartOffset(uint32 newOffset);
	uint32 getDataStartOffset();

	/* Getters : */
	// The file content to be appeneded with Elf32_Ehdr
	cBuffer getFileContent();
	// The Section that holds section's namestrings
	uint32  getNamesSectionIndex();
	// the offset for sections table.
	uint32  getSectionsTableOffset();
	// how much entries inside sections table
	uint32  getSectionsTableCount();

private:
	// Basic Vars
	uint32 m_dataStartOffset;		// What is the offset we will be written to (Default is right after Elf32_Ehdr)

	// Calculated vars
	uint32 m_namesSectionIdx;		// Where is the Section of names
	uint32 m_sectionsStartOffset;	// Where inside fileContent sections Start.
	uint32 m_sectionsTableOffset;	// Where inside fileContent sectionsTable Is.

	// Data Holders
	cBuffer m_fileContnet;			// The total file content.
	cBuffer m_sectionsBlob;			// All sections data togather.
	uint32	m_totalDataSize;		// total count of section's blob.
	cArray<cElfSectionPtr>	  m_sections;
	cArray<Elf32_Shdr>	  m_sectionHeaders;

	//
	// basic initialize for my vars
	//
	void init();

	//
	// This method should build the mapping from sections list.
	// This is the main method of this class, it should contain the mapping algo.
	//
	void buildMapping();

	//
	// this method will do the first pass upon sections,
	// and build the list of sections and sections data.
	//
	void processSections();

	//
	// This method will calc the offsets everything should "sit"
	// inside cFileOffset.
	//
	void calcOffsets();

	//
	// This method should glue everything togather.
	//
	void buildFile();

	//
	// two methods to convert my arrays to cBuffer blobs.
	//
	cBuffer getSectionHeadersTableBlob();
	cBuffer getSectionsDataBlob();
};

#endif // __ELF_ELFMEMMAPPER_H__