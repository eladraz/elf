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

#include "elf/cElf64Image.h"

#include <string.h>

#include "xStl/types.h"
#include "xStl/data/string.h"
#include "xStl/data/array.h"
#include "xStl/data/sarray.h"
#include "xStl/except/exception.h"

namespace
{

// Sizes of the ELF64 structures (the file layout, not the host struct layout).
const uint64 EHDR_SIZE = 64;
const uint64 PHDR_SIZE = 56;
const uint64 SHDR_SIZE = 64;
const uint64 SYM_SIZE  = 24;
const uint64 RELA_SIZE = 24;

// Reads an unsigned little-endian value of 'size' bytes (1 to 8).
uint64 readLE(const uint8* p, uint32 size)
{
	uint64 value = 0;
	for (uint32 i = 0; i < size; i++)
	{
		value |= (uint64)p[i] << (8 * i);
	}
	return value;
}

// True when [offset, offset + size) lies inside [0, total). Overflow safe.
bool rangeInside(uint64 offset, uint64 size, uint64 total)
{
	return (offset <= total) && (size <= total - offset);
}

void failImage(const character* message)
{
	XSTL_THROW(cException, message);
}

// Returns the NUL-terminated string at 'offset' of a string table of
// 'tableSize' bytes. Offset 0 is the empty name, as in every ELF string table.
cString nameAt(const uint8* table, uint64 tableSize, uint64 offset)
{
	if (0 == offset)
	{
		return cString("");
	}
	if (offset >= tableSize)
	{
		failImage(XSTL_STRING("ELF string offset is outside its string table."));
	}
	uint64 length = 0;
	while ((offset + length < tableSize) && (0 != table[offset + length]))
	{
		length++;
	}
	if (offset + length >= tableSize)
	{
		failImage(XSTL_STRING("ELF string is not terminated inside its string table."));
	}
	return cString((const char*)(table + offset));
}

} // namespace

cElf64Image::cElf64Image(const cBuffer& image)
	: m_image(image),
	  m_class(0),
	  m_type(0),
	  m_machine(0),
	  m_entry(0),
	  m_flags(0),
	  m_phOff(0),
	  m_phNum(0),
	  m_phEntSize(0)
{
	this->parse();
}

cElf64Image::~cElf64Image()
{
}

uint8 cElf64Image::getClass() const { return this->m_class; }
uint16 cElf64Image::getType() const { return this->m_type; }
uint16 cElf64Image::getMachine() const { return this->m_machine; }
uint64 cElf64Image::getEntry() const { return this->m_entry; }
uint32 cElf64Image::getFlags() const { return this->m_flags; }
uint64 cElf64Image::getPhOff() const { return this->m_phOff; }
uint16 cElf64Image::getPhNum() const { return this->m_phNum; }
uint16 cElf64Image::getPhEntSize() const { return this->m_phEntSize; }

const cArray<cElf64Image::cProgramHeader>& cElf64Image::getProgramHeaders() const { return this->m_programHeaders; }
const cArray<cElf64Image::cSection>& cElf64Image::getSections() const { return this->m_sections; }
const cArray<cElf64Image::cSymbol>& cElf64Image::getSymbols() const { return this->m_symbols; }
const cArray<cElf64Image::cRelocation>& cElf64Image::getRelocations() const { return this->m_relocations; }

cBuffer cElf64Image::getFileBytes(uint64 offset, uint64 size) const
{
	if (!rangeInside(offset, size, this->m_image.getSize()) || (size > 0xFFFFFFFFULL))
	{
		failImage(XSTL_STRING("Requested file bytes are outside the ELF image."));
	}

	cBuffer out;
	out.changeSize((uint)size, true);
	if (0 != size)
	{
		memcpy(out.getBuffer(), this->m_image.getBuffer() + offset, (size_t)size);
	}
	return out;
}

void cElf64Image::parse()
{
	const uint8* b = this->m_image.getBuffer();
	const uint64 fileSize = this->m_image.getSize();

	//
	// ELF header.
	//
	if (fileSize < EHDR_SIZE)
	{
		failImage(XSTL_STRING("ELF image is shorter than its header."));
	}
	if ((b[0] != 0x7f) || (b[1] != 'E') || (b[2] != 'L') || (b[3] != 'F'))
	{
		failImage(XSTL_STRING("Not an ELF image."));
	}
	if (b[EI_CLASS] != ELFCLASS64)
	{
		failImage(XSTL_STRING("ELF image is not ELFCLASS64."));
	}
	if (b[EI_DATA] != ELFDATA2LSB)
	{
		failImage(XSTL_STRING("ELF image is not little-endian."));
	}
	if (b[EI_VERSION] != EV_CURRENT)
	{
		failImage(XSTL_STRING("ELF identification version is not EV_CURRENT."));
	}
	if (readLE(b + 20, 4) != EV_CURRENT)
	{
		failImage(XSTL_STRING("ELF header version is not EV_CURRENT."));
	}
	if (readLE(b + 52, 2) != EHDR_SIZE)
	{
		failImage(XSTL_STRING("ELF header size is not 64."));
	}

	this->m_class = b[EI_CLASS];
	this->m_type = (uint16)readLE(b + 16, 2);
	this->m_machine = (uint16)readLE(b + 18, 2);
	this->m_entry = readLE(b + 24, 8);
	const uint64 phOff = readLE(b + 32, 8);
	const uint64 shOff = readLE(b + 40, 8);
	this->m_flags = (uint32)readLE(b + 48, 4);
	const uint64 phEnt = readLE(b + 54, 2);
	const uint64 phNum = readLE(b + 56, 2);
	const uint64 shEnt = readLE(b + 58, 2);
	const uint64 shNum = readLE(b + 60, 2);
	const uint64 shStrNdx = readLE(b + 62, 2);

	this->m_phOff = phOff;
	this->m_phNum = (uint16)phNum;
	this->m_phEntSize = (uint16)phEnt;

	//
	// Program headers.
	//
	if (0 != phNum)
	{
		if (phEnt != PHDR_SIZE)
		{
			failImage(XSTL_STRING("Program header entry size is not 56."));
		}
		if (!rangeInside(phOff, phNum * PHDR_SIZE, fileSize))
		{
			failImage(XSTL_STRING("Program header table is outside the ELF image."));
		}
	}
	for (uint64 i = 0; i < phNum; i++)
	{
		const uint8* p = b + phOff + i * PHDR_SIZE;
		cProgramHeader ph;
		ph.type = (uint32)readLE(p, 4);
		ph.flags = (uint32)readLE(p + 4, 4);
		ph.offset = readLE(p + 8, 8);
		ph.vaddr = readLE(p + 16, 8);
		ph.filesz = readLE(p + 32, 8);
		ph.memsz = readLE(p + 40, 8);
		ph.align = readLE(p + 48, 8);
		if (ph.memsz < ph.filesz)
		{
			failImage(XSTL_STRING("Program header memsz is smaller than filesz."));
		}
		if ((0 != ph.filesz) && !rangeInside(ph.offset, ph.filesz, fileSize))
		{
			failImage(XSTL_STRING("Program segment data is outside the ELF image."));
		}
		this->m_programHeaders.append(ph);
	}

	//
	// Section headers. Names are resolved in a second pass, once the
	// section name string table is known.
	//
	if (0 != shNum)
	{
		if (shEnt != SHDR_SIZE)
		{
			failImage(XSTL_STRING("Section header entry size is not 64."));
		}
		if (!rangeInside(shOff, shNum * SHDR_SIZE, fileSize))
		{
			failImage(XSTL_STRING("Section header table is outside the ELF image."));
		}
		if (shStrNdx >= shNum)
		{
			failImage(XSTL_STRING("Section name string table index is out of range."));
		}
	}
	cArray<uint64> nameOffsets;
	for (uint64 i = 0; i < shNum; i++)
	{
		const uint8* p = b + shOff + i * SHDR_SIZE;
		cSection s;
		s.type = (uint32)readLE(p + 4, 4);
		s.flags = readLE(p + 8, 8);
		s.addr = readLE(p + 16, 8);
		s.offset = readLE(p + 24, 8);
		s.size = readLE(p + 32, 8);
		s.link = (uint32)readLE(p + 40, 4);
		s.info = (uint32)readLE(p + 44, 4);
		s.entsize = readLE(p + 56, 8);
		if ((SHT_NULL != s.type) && (SHT_NOBITS != s.type) && (0 != s.size) &&
		    !rangeInside(s.offset, s.size, fileSize))
		{
			failImage(XSTL_STRING("Section data is outside the ELF image."));
		}
		nameOffsets.append(readLE(p, 4));
		this->m_sections.append(s);
	}

	const uint8* shStrBase = b;
	uint64 shStrSize = 0;
	if (0 != shNum)
	{
		const cSection shStr = this->m_sections[(uint)shStrNdx];
		if (SHT_STRTAB != shStr.type)
		{
			failImage(XSTL_STRING("Section name table is not a string table."));
		}
		shStrBase = b + shStr.offset;
		shStrSize = shStr.size;
	}
	for (uint64 i = 0; i < shNum; i++)
	{
		this->m_sections[(uint)i].name = nameAt(shStrBase, shStrSize, nameOffsets[(uint)i]);
	}

	//
	// Symbols of the first SHT_SYMTAB section.
	//
	for (uint i = 0; i < this->m_sections.getSize(); i++)
	{
		if (SHT_SYMTAB != this->m_sections[i].type)
		{
			continue;
		}
		const cSection sym = this->m_sections[i];
		if (0 != sym.size)
		{
			if (sym.entsize != SYM_SIZE)
			{
				failImage(XSTL_STRING("Symbol table entry size is not 24."));
			}
			if (0 != (sym.size % SYM_SIZE))
			{
				failImage(XSTL_STRING("Symbol table size is not a multiple of 24."));
			}
			if ((sym.link >= shNum) || (SHT_STRTAB != this->m_sections[sym.link].type))
			{
				failImage(XSTL_STRING("Symbol table does not link to a string table."));
			}
			const cSection strSec = this->m_sections[sym.link];
			const uint8* strBase = b + strSec.offset;
			const uint64 count = sym.size / SYM_SIZE;
			if (count > 0xFFFFFFFFULL)
			{
				failImage(XSTL_STRING("Symbol table has too many entries."));
			}
			// Size the array once, then fill it by index: appending would
			// reallocate and copy every name each time a page fills up.
			this->m_symbols.changeSize((uint)count, false);
			for (uint64 j = 0; j < count; j++)
			{
				const uint8* p = b + sym.offset + j * SYM_SIZE;
				cSymbol& s = this->m_symbols[(uint)j];
				s.name = nameAt(strBase, strSec.size, readLE(p, 4));
				s.type = (uint8)(p[4] & 0x0f);
				s.bind = (uint8)(p[4] >> 4);
				s.sectionIndex = (uint16)readLE(p + 6, 2);
				s.value = readLE(p + 8, 8);
				s.size = readLE(p + 16, 8);
			}
		}
		break;
	}

	//
	// RELA relocations of every SHT_RELA section. Pass 1 validates the
	// sections and counts the entries; pass 2 sizes the array once and fills
	// it by index.
	//
	uint64 relaTotal = 0;
	for (uint i = 0; i < this->m_sections.getSize(); i++)
	{
		if (SHT_RELA != this->m_sections[i].type)
		{
			continue;
		}
		const cSection rel = this->m_sections[i];
		if (0 != (rel.size % RELA_SIZE))
		{
			failImage(XSTL_STRING("Relocation table size is not a multiple of 24."));
		}
		if (0 == rel.size)
		{
			continue;
		}
		if (rel.entsize != RELA_SIZE)
		{
			failImage(XSTL_STRING("Relocation entry size is not 24."));
		}
		if (rel.info >= shNum)
		{
			failImage(XSTL_STRING("Relocation section target is out of range."));
		}
		if ((rel.link >= shNum) || (SHT_SYMTAB != this->m_sections[rel.link].type))
		{
			failImage(XSTL_STRING("Relocation section does not link to a symbol table."));
		}
		relaTotal += rel.size / RELA_SIZE;
		if (relaTotal > 0xFFFFFFFFULL)
		{
			failImage(XSTL_STRING("Too many relocation entries."));
		}
	}

	if (0 != relaTotal)
	{
		this->m_relocations.changeSize((uint)relaTotal, false);
	}
	uint64 relaIndex = 0;
	for (uint i = 0; i < this->m_sections.getSize(); i++)
	{
		if (SHT_RELA != this->m_sections[i].type)
		{
			continue;
		}
		const cSection rel = this->m_sections[i];
		if (0 == rel.size)
		{
			continue;
		}
		const uint64 symbolCount = this->m_sections[rel.link].size / SYM_SIZE;
		const uint64 count = rel.size / RELA_SIZE;
		for (uint64 j = 0; j < count; j++)
		{
			const uint8* p = b + rel.offset + j * RELA_SIZE;
			const uint64 info = readLE(p + 8, 8);
			const uint64 symbol = info >> 32;
			if (symbol >= symbolCount)
			{
				failImage(XSTL_STRING("Relocation refers to a symbol outside its symbol table."));
			}
			cRelocation& r = this->m_relocations[(uint)relaIndex];
			r.targetSection = rel.info;
			r.offset = readLE(p, 8);
			r.type = (uint32)(info & 0xFFFFFFFFULL);
			r.symbolIndex = (uint32)symbol;
			r.addend = (int64)readLE(p + 16, 8);
			relaIndex++;
		}
	}
}
