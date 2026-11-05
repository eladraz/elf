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

#include "elf/cElf64ObjectWriter.h"

#include "xStl/types.h"
#include "xStl/data/string.h"
#include "xStl/data/sarray.h"
#include "xStl/except/exception.h"
#include "xStl/stream/basicIO.h"

namespace
{

// Sizes of the ELF64 structures written by this file.
const uint64 ELF64_EHDR_SIZE  = sizeof(Elf64_Ehdr);
const uint64 ELF64_SHDR_SIZE  = sizeof(Elf64_Shdr);
const uint64 ELF64_SYM_SIZE   = sizeof(Elf64_Sym);
const uint64 ELF64_RELA_SIZE  = sizeof(Elf64_Rela);

// Appends raw bytes to the buffer.
void appendBytes(cBuffer& buffer, const void* data, uint size)
{
	uint pos = buffer.getSize();
	buffer.changeSize(pos + size, true);
	if (size != 0)
	{
		memcpy(buffer.getBuffer() + pos, data, size);
	}
}

// Appends 'size' zero bytes to the buffer.
void appendZeros(cBuffer& buffer, uint64 size)
{
	uint pos = buffer.getSize();
	buffer.changeSize(pos + (uint)size, true);
	if (size != 0)
	{
		memset(buffer.getBuffer() + pos, 0, (size_t)size);
	}
}

// Appends an unsigned value of 'size' bytes in little-endian order.
void appendLittleEndian(cBuffer& buffer, uint64 value, uint size)
{
	uint pos = buffer.getSize();
	buffer.changeSize(pos + size, true);
	for (uint i = 0; i < size; i++)
	{
		buffer.getBuffer()[pos + i] = (uint8)(value >> (8 * i));
	}
}

// Appends a NUL terminated name to a string table and returns its offset.
// The empty name is always offset 0, which is the leading NUL of every table.
uint32 addStringToTable(cBuffer& table, const cString& name)
{
	if (name.length() == 0)
	{
		return 0;
	}

	uint32 pos = table.getSize();
	cSArray<char> ascii = name.getASCIIstring();
	appendBytes(table, ascii.getBuffer(), ascii.getSize());
	return pos;
}

uint64 alignUp(uint64 value, uint64 alignment)
{
	return ((value + alignment - 1) / alignment) * alignment;
}

// Pads the body so the next section starts at an offset aligned to 'alignment'
// in the file (the ELF header precedes the body). Returns that file offset.
uint64 padBody(cBuffer& body, uint64 alignment)
{
	uint64 pos = ELF64_EHDR_SIZE + body.getSize();
	uint64 aligned = alignUp(pos, alignment);
	appendZeros(body, aligned - pos);
	return aligned;
}

void appendSectionHeader(cBuffer& headers,
                         uint32 nameOffset,
                         uint32 type,
                         uint64 flags,
                         uint64 fileOffset,
                         uint64 size,
                         uint32 link,
                         uint32 info,
                         uint64 alignment,
                         uint64 entrySize)
{
	appendLittleEndian(headers, nameOffset, 4);
	appendLittleEndian(headers, type, 4);
	appendLittleEndian(headers, flags, 8);
	appendLittleEndian(headers, 0, 8);          // sh_addr: relocatable object
	appendLittleEndian(headers, fileOffset, 8);
	appendLittleEndian(headers, size, 8);
	appendLittleEndian(headers, link, 4);
	appendLittleEndian(headers, info, 4);
	appendLittleEndian(headers, alignment, 8);
	appendLittleEndian(headers, entrySize, 8);
}

} // namespace

cElf64ObjectWriter::cElf64ObjectWriter(uint16 machine) : m_machine(machine)
{
}

cElf64ObjectWriter::~cElf64ObjectWriter()
{
}

uint cElf64ObjectWriter::addSection(const cString & name, uint32 type, uint64 flags, uint64 alignment, const cBuffer & data)
{
	uint64 effectiveAlignment = (0 == alignment) ? 1 : alignment;
	if (0 != (effectiveAlignment & (effectiveAlignment - 1)))
	{
		XSTL_THROW(cException, XSTL_STRING("ELF section alignment must be a power of two."));
	}

	cSection section;
	section.name = name;
	section.type = type;
	section.flags = flags;
	section.alignment = effectiveAlignment;
	section.data = data;
	this->m_sections.append(section);

	// Section index 0 is the null section.
	return this->m_sections.getSize();
}

uint cElf64ObjectWriter::addSymbol(const cString & name, uint16 sectionIndex, uint64 value, uint64 size, bool isFunction, bool isGlobal)
{
	// Reserved indices (SHN_ABS, SHN_COMMON, ...) are passed through as is.
	if ((sectionIndex > this->m_sections.getSize()) && (sectionIndex < SHN_LORESERVE))
	{
		XSTL_THROW(cException, XSTL_STRING("ELF symbol refers to a section that was not added."));
	}

	cSymbol symbol;
	symbol.name = name;
	symbol.sectionIndex = sectionIndex;
	symbol.value = value;
	symbol.size = size;
	symbol.isFunction = isFunction;
	symbol.isGlobal = isGlobal;
	this->m_symbols.append(symbol);

	// Symbol index 0 is the null symbol.
	return this->m_symbols.getSize();
}

void cElf64ObjectWriter::addRelocation(uint sectionIndex, uint64 offset, uint32 type, uint symbolIndex, int64 addend)
{
	if ((0 == sectionIndex) || (sectionIndex > this->m_sections.getSize()))
	{
		XSTL_THROW(cException, XSTL_STRING("ELF relocation refers to a section that was not added."));
	}
	if (symbolIndex > this->m_symbols.getSize())
	{
		XSTL_THROW(cException, XSTL_STRING("ELF relocation refers to a symbol that was not added."));
	}

	cRelocation relocation;
	relocation.sectionIndex = sectionIndex;
	relocation.offset = offset;
	relocation.type = type;
	relocation.symbolIndex = symbolIndex;
	relocation.addend = addend;
	this->m_relocations.append(relocation);
}

void cElf64ObjectWriter::save(basicIO & stream)
{
	const uint userCount = this->m_sections.getSize();
	const uint symbolCount = this->m_symbols.getSize();
	uint i = 0;
	uint s = 0;
	uint pass = 0;

	//
	// Section indices: null, user sections, one .rela per section with
	// relocations, .note.GNU-stack, .symtab, .strtab, .shstrtab.
	//
	cArray<uint> relaIndex;
	relaIndex.changeSize(userCount, true);
	uint next = userCount + 1;
	for (i = 0; i < userCount; i++)
	{
		relaIndex[i] = 0;
		for (uint r = 0; r < this->m_relocations.getSize(); r++)
		{
			if (this->m_relocations[r].sectionIndex == i + 1)
			{
				relaIndex[i] = next++;
				break;
			}
		}
	}
	next++;                                       // .note.GNU-stack
	const uint symtabIndex = next++;
	const uint strtabIndex = next++;
	const uint shstrtabIndex = next++;
	const uint sectionCount = next;

	//
	// Section name table. Names are added before any layout is done.
	//
	cBuffer shstrtab;
	appendBytes(shstrtab, "", 1);
	cArray<uint32> userNameOffset;
	cArray<uint32> relaNameOffset;
	userNameOffset.changeSize(userCount, true);
	relaNameOffset.changeSize(userCount, true);
	for (i = 0; i < userCount; i++)
	{
		userNameOffset[i] = addStringToTable(shstrtab, this->m_sections[i].name);
	}
	for (i = 0; i < userCount; i++)
	{
		relaNameOffset[i] = 0;
		if (0 != relaIndex[i])
		{
			relaNameOffset[i] = addStringToTable(shstrtab, cString(".rela") + this->m_sections[i].name);
		}
	}
	const uint32 noteNameOffset = addStringToTable(shstrtab, cString(".note.GNU-stack"));
	const uint32 symtabNameOffset = addStringToTable(shstrtab, cString(".symtab"));
	const uint32 strtabNameOffset = addStringToTable(shstrtab, cString(".strtab"));
	const uint32 shstrtabNameOffset = addStringToTable(shstrtab, cString(".shstrtab"));

	//
	// Symbol order: null symbol, then all local symbols, then all global symbols.
	// symbolIndex maps a user symbol index (1-based) to the final ELF index.
	//
	cArray<uint> symbolIndex;
	symbolIndex.changeSize(symbolCount + 1, true);
	symbolIndex[0] = 0;
	uint firstGlobal = 1;
	uint finalIndex = 1;
	for (pass = 0; pass < 2; pass++)
	{
		for (s = 0; s < symbolCount; s++)
		{
			if (this->m_symbols[s].isGlobal == (1 == pass))
			{
				symbolIndex[s + 1] = finalIndex++;
			}
		}
		if (0 == pass)
		{
			firstGlobal = finalIndex;
		}
	}

	//
	// .strtab and .symtab contents.
	//
	cBuffer strtab;
	appendBytes(strtab, "", 1);
	cArray<uint32> symbolNameOffset;
	symbolNameOffset.changeSize(symbolCount, true);
	for (s = 0; s < symbolCount; s++)
	{
		symbolNameOffset[s] = addStringToTable(strtab, this->m_symbols[s].name);
	}

	cBuffer symtab;
	appendZeros(symtab, ELF64_SYM_SIZE);
	for (pass = 0; pass < 2; pass++)
	{
		for (s = 0; s < symbolCount; s++)
		{
			const cSymbol& symbol = this->m_symbols[s];
			if (symbol.isGlobal != (1 == pass))
			{
				continue;
			}

			uint8 bind = symbol.isGlobal ? STB_GLOBAL : STB_LOCAL;
			uint8 type = STT_NOTYPE;
			if (symbol.isFunction)
			{
				type = STT_FUNC;
			}
			else if (SHN_UNDEF != symbol.sectionIndex)
			{
				type = STT_OBJECT;
			}

			appendLittleEndian(symtab, symbolNameOffset[s], 4);
			appendLittleEndian(symtab, ELF64_ST_INFO(bind, type), 1);
			appendLittleEndian(symtab, 0, 1);                       // st_other
			appendLittleEndian(symtab, symbol.sectionIndex, 2);
			appendLittleEndian(symtab, symbol.value, 8);
			appendLittleEndian(symtab, symbol.size, 8);
		}
	}

	//
	// Lay out the file: ELF header, section contents, then the section header table.
	//
	cBuffer body;
	cBuffer sectionHeaders;
	appendZeros(sectionHeaders, ELF64_SHDR_SIZE);

	for (i = 0; i < userCount; i++)
	{
		const cSection& section = this->m_sections[i];
		uint64 alignment = section.alignment;
		uint64 offset = padBody(body, alignment);
		uint64 size = section.data.getSize();
		if (SHT_NOBITS != section.type)
		{
			appendBytes(body, section.data.getBuffer(), (uint)size);
		}
		appendSectionHeader(sectionHeaders, userNameOffset[i], section.type, section.flags,
		                    offset, size, 0, 0, alignment, 0);
	}

	for (i = 0; i < userCount; i++)
	{
		if (0 == relaIndex[i])
		{
			continue;
		}

		cBuffer relocations;
		for (uint r = 0; r < this->m_relocations.getSize(); r++)
		{
			const cRelocation& relocation = this->m_relocations[r];
			if (relocation.sectionIndex != i + 1)
			{
				continue;
			}

			uint64 info = ELF64_R_INFO(symbolIndex[relocation.symbolIndex], relocation.type);
			appendLittleEndian(relocations, relocation.offset, 8);
			appendLittleEndian(relocations, info, 8);
			appendLittleEndian(relocations, (uint64)relocation.addend, 8);
		}

		uint64 offset = padBody(body, 8);
		appendBytes(body, relocations.getBuffer(), relocations.getSize());
		appendSectionHeader(sectionHeaders, relaNameOffset[i], SHT_RELA, SHF_INFO_LINK,
		                    offset, relocations.getSize(), symtabIndex, i + 1, 8, ELF64_RELA_SIZE);
	}

	// .note.GNU-stack marks the stack as non-executable.
	appendSectionHeader(sectionHeaders, noteNameOffset, SHT_PROGBITS, 0,
	                    padBody(body, 1), 0, 0, 0, 1, 0);

	{
		uint64 offset = padBody(body, 8);
		appendBytes(body, symtab.getBuffer(), symtab.getSize());
		appendSectionHeader(sectionHeaders, symtabNameOffset, SHT_SYMTAB, 0,
		                    offset, symtab.getSize(), strtabIndex, firstGlobal, 8, ELF64_SYM_SIZE);
	}
	{
		uint64 offset = padBody(body, 1);
		appendBytes(body, strtab.getBuffer(), strtab.getSize());
		appendSectionHeader(sectionHeaders, strtabNameOffset, SHT_STRTAB, 0,
		                    offset, strtab.getSize(), 0, 0, 1, 0);
	}
	{
		uint64 offset = padBody(body, 1);
		appendBytes(body, shstrtab.getBuffer(), shstrtab.getSize());
		appendSectionHeader(sectionHeaders, shstrtabNameOffset, SHT_STRTAB, 0,
		                    offset, shstrtab.getSize(), 0, 0, 1, 0);
	}

	// The section header table is 8-byte aligned.
	uint64 sectionHeaderOffset = padBody(body, 8);

	//
	// ELF header (little-endian, relocatable, no program headers).
	//
	cBuffer file;
	uint8 ident[EI_NIDENT];
	memset(ident, 0, sizeof(ident));
	memcpy(ident, ELFMAG, SELFMAG);
	ident[EI_CLASS] = ELFCLASS64;
	ident[EI_DATA] = ELFDATA2LSB;
	ident[EI_VERSION] = EV_CURRENT;
	ident[EI_OSABI] = ELFOSABI_NONE;
	appendBytes(file, ident, EI_NIDENT);
	appendLittleEndian(file, ET_REL, 2);
	appendLittleEndian(file, this->m_machine, 2);
	appendLittleEndian(file, EV_CURRENT, 4);
	appendLittleEndian(file, 0, 8);                     // e_entry
	appendLittleEndian(file, 0, 8);                     // e_phoff
	appendLittleEndian(file, sectionHeaderOffset, 8);   // e_shoff
	appendLittleEndian(file, 0, 4);                     // e_flags
	appendLittleEndian(file, ELF64_EHDR_SIZE, 2);       // e_ehsize
	appendLittleEndian(file, 0, 2);                     // e_phentsize
	appendLittleEndian(file, 0, 2);                     // e_phnum
	appendLittleEndian(file, ELF64_SHDR_SIZE, 2);       // e_shentsize
	appendLittleEndian(file, sectionCount, 2);          // e_shnum
	appendLittleEndian(file, shstrtabIndex, 2);         // e_shstrndx

	appendBytes(file, body.getBuffer(), body.getSize());
	appendBytes(file, sectionHeaders.getBuffer(), sectionHeaders.getSize());

	stream.pipeWrite(file, file.getSize());
}
