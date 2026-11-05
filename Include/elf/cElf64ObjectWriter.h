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

#ifndef  __ELF_ELF64OBJECTWRITER_H__
#define  __ELF_ELF64OBJECTWRITER_H__
/*
 * cElf64ObjectWriter.h
 *
 * ELF64 relocatable object (ET_REL) writer.
 * Builds a little-endian ELF64 object file in memory from sections,
 * symbols and RELA relocations and writes it to a basicIO stream.
 * The ELF32 reader/writer (cElfFile) is not used or changed by this class.
 */

#include "xStl/types.h"
#include "xStl/data/array.h"
#include "xStl/data/string.h"
#include "xStl/data/datastream.h"
#include "xStl/except/exception.h"
#include "xStl/stream/basicIO.h"

#include "elf/opensource_headers/elf.h"

class cElf64ObjectWriter
{
public:
	// C'Tor. machine is one of the EM_ constants (default EM_X86_64).
	cElf64ObjectWriter(uint16 machine = EM_X86_64);

	// D'Tor
	~cElf64ObjectWriter();

	//
	// Adds a section and returns its ELF section index (always >= 1).
	// type      - SHT_ value (SHT_PROGBITS, SHT_NOBITS, ...)
	// flags     - SHF_ value (SHF_ALLOC, SHF_EXECINSTR, SHF_WRITE, ...)
	// alignment - sh_addralign; 0 and 1 mean no alignment, otherwise a power of 2.
	// data      - section contents (ignored for SHT_NOBITS, size is kept).
	//
	uint addSection(const cString & name, uint32 type, uint64 flags, uint64 alignment, const cBuffer & data);

	//
	// Adds a symbol and returns its index (always >= 1, index 0 is the null symbol).
	// sectionIndex - ELF section index returned by addSection, or SHN_UNDEF (0)
	//                for an undefined (external) symbol.
	// value        - offset inside the section (or 0 for undefined symbols).
	// isFunction   - STT_FUNC when true; otherwise STT_OBJECT for defined symbols
	//                and STT_NOTYPE for undefined ones.
	// isGlobal     - STB_GLOBAL when true, otherwise STB_LOCAL.
	//
	uint addSymbol(const cString & name, uint16 sectionIndex, uint64 value, uint64 size, bool isFunction, bool isGlobal);

	//
	// Adds a RELA relocation to the section sectionIndex (as returned by addSection).
	// offset      - where the relocation is applied inside the section.
	// type        - R_X86_64_* relocation type.
	// symbolIndex - symbol index returned by addSymbol.
	// addend      - RELA addend.
	//
	void addRelocation(uint sectionIndex, uint64 offset, uint32 type, uint symbolIndex, int64 addend);

	//
	// Writes the complete object file to the stream. Can be called more than once.
	// Emits the null section, the user sections, one .rela<name> per section with
	// relocations, .note.GNU-stack (non-executable stack), .symtab, .strtab and .shstrtab.
	//
	void save(basicIO & stream);

private:
	struct cSection
	{
		cString name;
		uint32 type;
		uint64 flags;
		uint64 alignment;
		cBuffer data;
	};

	struct cSymbol
	{
		cString name;
		uint16 sectionIndex;
		uint64 value;
		uint64 size;
		bool isFunction;
		bool isGlobal;
	};

	struct cRelocation
	{
		uint sectionIndex;
		uint64 offset;
		uint32 type;
		uint symbolIndex;
		int64 addend;
	};

	uint16 m_machine;
	cArray<cSection> m_sections;
	cArray<cSymbol> m_symbols;
	cArray<cRelocation> m_relocations;
};

#endif // __ELF_ELF64OBJECTWRITER_H__
