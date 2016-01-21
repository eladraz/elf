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

#ifndef  __ELF_ELFFILE_H__
#define  __ELF_ELFFILE_H__
/*
 * cElfFile.h
 *
 * ELF File format parser.
 * You can create or edit any file for Unix Systems with it.
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
#include "elf/cElfMemoryMapper.h"
#include "elf/cElfStringTable.h"
#include "elf/cElfSymbol.h"
#include "elf/cElfRelocationSection.h"
#include "elf/Dwarf/cDwarf2Manager.h"

class cElfFile
{
public:
	// Basic CTor to parse files.
	cElfFile(basicIO& stream,
             bool shouldReadSections = true);

	// Create New File
	cElfFile();

	// D'Tor
	~cElfFile();

	// this method will save the elf file to target stream,
	void saveAs(basicIO& stream);

	// Arch Setter/Getter - can be found in opensource_headers/elf.h, prefix is EM_
	void setArch(Elf32_Half newMachine);
	Elf32_Half getArch();

	//
	// use symbol inside a section
	// symbolName        - the name of the symbol to be used, make sure the symbol is registered first!
	// sectionName       - where you want to use this symbol, please make sure the section is registered first!
	// relType           - Relative Type ( For more information read Elf Spec, those are accepted values ) :
	//	R_386_NONE		===> none
	//	R_386_32		===> S + A
	//	R_386_PC32		===> S + A - P
	//	R_386_GOT32		===> G + A - P
	//	R_386_PLT32		===> L + A - P
	//	R_386_COPY		===> none none
	//	R_386_GLOB_DAT	===> S
	//	R_386_JMP_SLOT	===> S
	//	R_386_RELATIVE	===> B + A
	//	R_386_GOTOFF	===> S + A - GOT
	//	R_386_GOTPC		===> GOT + A - P
	// offsetFromSection - where inside the section's data it is used.
	//
	void useSymbol(const cString & symbolName, const cString & sectionName, uint32 relType, uint32 offsetFromSection);

	//
	// add symbol to the object in order to use it later:
	// symbolName - name of the symbol
	// symbolType - what type is this symbol :
	// 		SYMTYPE_EXTERN  ==> external function usage
	//		SYMTYPE_FUNC    ==> function symbol
	//		SYMTYPE_OBJECT  ==> object/global symbol
	//		SYMTYPE_SECTION ==> section symbol
	// sectionName - what section the symbol is stored?
	// offsetFromSection - where in the section the symbol is stored ?
	// symbolSize - how long this symbol is?
	// isVisiable - Global or local symbol?
	//
	void addSymbol(const cString & symbolName, cElfSymbol::SYMBOL_TYPES symbolType, const cString & sectionName, uint32 offsetFromSection, uint32 symbolSize, bool isVisible = true);

	//
	// removes target symbol by name
	// and all of the symbol's usage.
	//
	void removeSymbol(const cString & symbolName);

    bool hasSymbol(const cString & symName);

    //
    // adds new section to the elf file.
    //
    void addSection(uint32 sectionType, const cString & sectionName, const cBuffer & data, uint32 flags);

	//
	// Debug functions export
	//
	void debugInit(const cString & codeSectionName, const cString &  fileName, const cString &  filePath, int dwarfVersion = 2);

private:
	Elf32_Ehdr				m_elfHeader;
	cArray<cElfSectionPtr>	m_sections;
	cArray<cElfSymbolPtr>	m_symbols;
	cArray<cElfRelocationSectionPtr> m_relocations;
	int32					m_symbolTableId;
	int32					m_strTableId;
	int32					m_shstrTableId;
	bool					m_hasDbgInfo;
	cDwarf2ManagerPtr		m_dbgManager;

	// Initalized my struct.
	void init();

	// Will load the struct with default values
	void setDefaults();

	// This method will read and parse the data
	void read(basicIO& _stream, bool shouldReadSections);

	// This method will read the program's sections from a elf file stream.
	void readSectionsHeader(basicIO& stream);

	//
	// This method will add the section names section again,
	// while doing so, it will update all section's pointers as well.
	//
	void createSectionsNameSection();

	//
	// this method will create the symbol table again,
	// while doing so, it will create the names section and add both
	//
	void createSymbolTableAndNamesSection();

	//
	// this method will readd the relocation striped
	//
	void addRelocations();

	//
	// Helper for symbols table.
	// it will help identify old section index
	// because i'm stripping sections out.
	//
	uint32 oldCountToNewCount(int32 oldCountId);

	//
	// This method will update the symbol table about a new symbol table section
	//
	void updateSymbolTable(basicIO& stream, Elf32_Shdr SymbolHdr, Elf32_Shdr StringTableHdr,  uint32 deletedSectionsCount);

	//
	// This method will rewrite the file to stream.
	//
	void write(basicIO& stream);

	//
	// Finds section by name and return it's id to the caller.
	// or -1 incase section didn't found.
	//
	int32 findSectionByName(const cString & sectionName);

	//
	// Finds symbol by name and return it's id to the caller.
	// or -1 incase symbol didn't found.
	//
	int32 findSymbolByName(const cString & symbolName);

	//
	// Rebuilts Debug sections
	//
	void addDbgSections();
};

#endif  //__ELF_ELFFILE_H__
