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

#ifndef  __ELF_ELFSECTION_H__
#define  __ELF_ELFSECTION_H__
/*
 * cElfSection.h
 *
 * ELF Section Class
 * This class describes a section of a Elf File Format.
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

class cElfSection {
public:
	// C'Tor
	cElfSection(const cString & name,
				const cBuffer & data,
                uint32 section_type,
                uint32 flags = 0,
				uint32 nametable_index = 0,
				uint32 addr_align = 1,
				uint32 info = 0,
				uint32 link = 0,
				uint32 entry_size = 0);

	cElfSection(basicIO& stream, const Elf32_Shdr & sourceHdr);

	// D'Tor
	~cElfSection();

	// This method will copy the section's headers to struct.
	void toStruct(Elf32_Shdr & output);

	// setter/getter for data
	void setData(const cBuffer & newData);
	cBuffer & getData();

	// setter/getter for type
	void setType(uint32 newType);
	uint32 getType();

	// setter/getter for flags
	void setFlags(uint32 newFlags);
	uint32 getFlags();

	// setter/getter for name
	void setName(const cString & name);
	cString & getName();

	// Setter/Getter for link attribute
	void setLink(uint32 newLink);
	uint32 getLink();

	// setter/getter for nametable Index.
	void setNameTableIndex(uint32 newIndex);
	uint32 getNameTableIndex();

	// setter/getter for old count.
	void setOldCount(int32 oldCount);
	int32 getOldCount();

private:
	Elf32_Shdr m_header;
	cBuffer m_data;
	cString m_name;
	int32 m_oldCountId;
};

// Ptr for Sections
typedef cSmartPtr<cElfSection> cElfSectionPtr;

#endif