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

#ifndef  __ELF_ELF64IMAGE_H__
#define  __ELF_ELF64IMAGE_H__
/*
 * cElf64Image.h
 *
 * Read-only ELF64 little-endian parser (ET_EXEC, ET_DYN, ET_REL).
 * The image is validated once in the constructor: every table, string and
 * section/segment payload must lie inside the file, and every offset+size
 * pair is checked without overflow. A malformed image throws cException.
 * The parser reads fields byte by byte, so it does not depend on the host
 * endianness or on the alignment of the file buffer.
 * The ELF32 reader/writer (cElfFile) is not used or changed by this class.
 */

#include "xStl/types.h"
#include "xStl/data/array.h"
#include "xStl/data/sarray.h"
#include "xStl/data/string.h"
#include "xStl/except/exception.h"

#include "elf/opensource_headers/elf.h"

class cElf64Image
{
public:
	struct cProgramHeader
	{
		uint32 type;
		uint32 flags;
		uint64 offset;
		uint64 vaddr;
		uint64 filesz;
		uint64 memsz;
		uint64 align;
	};

	struct cSection
	{
		cString name;
		uint32 type;
		uint64 flags;
		uint64 addr;
		uint64 offset;
		uint64 size;
		uint32 link;
		uint32 info;
		uint64 entsize;
	};

	struct cSymbol
	{
		cString name;
		uint64 value;
		uint64 size;
		uint8 type;      // STT_*
		uint8 bind;      // STB_*
		uint16 sectionIndex;
	};

	struct cRelocation
	{
		uint32 targetSection;   // sh_info of the SHT_RELA section
		uint64 offset;
		uint32 type;            // R_* of the machine
		uint32 symbolIndex;     // index into the symbol table named by sh_link
		int64 addend;
	};

	// Parses and validates the image. Throws cException when it is malformed.
	explicit cElf64Image(const cBuffer& image);

	// D'Tor
	~cElf64Image();

	// ELF header fields. getClass() is always ELFCLASS64 for a parsed image.
	uint8 getClass() const;
	uint16 getType() const;
	uint16 getMachine() const;
	uint64 getEntry() const;
	uint32 getFlags() const;

	// Program header table location (zero when there is none).
	uint64 getPhOff() const;
	uint16 getPhNum() const;
	uint16 getPhEntSize() const;

	const cArray<cProgramHeader>& getProgramHeaders() const;
	const cArray<cSection>& getSections() const;

	// Symbols of the first SHT_SYMTAB section (empty when there is none).
	const cArray<cSymbol>& getSymbols() const;

	// Relocations of every SHT_RELA section, in section order.
	const cArray<cRelocation>& getRelocations() const;

	// Copies size bytes at file offset 'offset'. Throws when out of range.
	cBuffer getFileBytes(uint64 offset, uint64 size) const;

private:
	void parse();

	cBuffer m_image;
	uint8 m_class;
	uint16 m_type;
	uint16 m_machine;
	uint64 m_entry;
	uint32 m_flags;
	uint64 m_phOff;
	uint16 m_phNum;
	uint16 m_phEntSize;
	cArray<cProgramHeader> m_programHeaders;
	cArray<cSection> m_sections;
	cArray<cSymbol> m_symbols;
	cArray<cRelocation> m_relocations;
};

#endif // __ELF_ELF64IMAGE_H__
