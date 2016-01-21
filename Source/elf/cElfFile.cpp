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

#include "elf/cElfFile.h"

#include "xStl/types.h"
#include "xStl/data/char.h"
#include "xStl/data/string.h"
#include "xStl/except/trace.h"
#include "xStl/except/exception.h"
#include "xStl/except/assert.h"
#include "xStl/stream/traceStream.h"
#include "xStl/stream/fileStream.h"
#include "xStl/stream/ioStream.h"
#include "xStl/stream/endianFilterStream.h"
#include "xStl/os/virtualMemoryAccesser.h"
#include "xStl/os/threadUnsafeMemoryAccesser.h"
#include "xStl/stream/memoryAccesserStream.h"

cElfFile::cElfFile(basicIO& stream, bool shouldReadSections) {
	this->init();
	this->read(stream, shouldReadSections);
}

cElfFile::cElfFile() {
	this->init();
	this->setDefaults();
}

cElfFile::~cElfFile() {
	this->m_sections.changeSize(0, true);
	this->m_symbols.changeSize(0, true);
	this->m_relocations.changeSize(0, true);
}

void cElfFile::init() {
	// Initialize the strcut.
	memset(&this->m_elfHeader, 0, sizeof(this->m_elfHeader));
	this->m_sections.changeSize(0, true);
	this->m_symbols.changeSize(0, true);
	this->m_relocations.changeSize(0, true);
	this->m_symbolTableId = -1;
    this->m_hasDbgInfo = false;
}

void cElfFile::setDefaults()
{
    cBuffer emptyBuff;
    emptyBuff.changeSize(0, true);

	memcpy(this->m_elfHeader.e_ident, ELFMAG, SELFMAG);
    this->m_elfHeader.e_ident[EI_CLASS] = ELFCLASS32;
    this->m_elfHeader.e_ident[EI_DATA] = ELFDATA2LSB;
    this->m_elfHeader.e_ident[EI_VERSION] = 1;

	this->m_elfHeader.e_ehsize = sizeof(Elf32_Ehdr);
	this->m_elfHeader.e_type = ET_REL;
	this->m_elfHeader.e_machine = EM_386;
	this->m_elfHeader.e_version = EV_CURRENT;
	this->m_elfHeader.e_flags = EF_ARM_EABI_VER5;

	// Entry point
	this->m_elfHeader.e_entry = 0;

	// Section Headers
	this->m_elfHeader.e_shoff = 0;
	this->m_elfHeader.e_shentsize = 0;
	this->m_elfHeader.e_shnum = 0;
	this->m_elfHeader.e_shstrndx = 0;

	// Program Headers
	this->m_elfHeader.e_phoff = 0;
	this->m_elfHeader.e_phentsize = sizeof(Elf32_Phdr);
	this->m_elfHeader.e_phnum = 0;

    this->addSection(SHT_NULL, "", emptyBuff, 0);
    this->m_symbols.append(cElfSymbolPtr(new cElfSymbol("", 0, 0, false, cElfSymbol::SYMTYPE_NONE, 0)));
}

uint32 cElfFile::oldCountToNewCount(int32 oldCountId)
{
	for (uint32 i = 0; i < this->m_sections.getSize(); i ++ ) {
		if ( oldCountId == this->m_sections[i]->getOldCount() ) {
			return i;
		}
	}

	return oldCountId;
}

void cElfFile::updateSymbolTable(basicIO& stream, Elf32_Shdr SymbolHdr, Elf32_Shdr StringTableHdr, uint32 deletedSectionsCount)
{
	cElfStringTable stringTable(stream, StringTableHdr.sh_offset, StringTableHdr.sh_size);
	cElfSection curSymbolSection(stream, SymbolHdr);
	cBuffer symbolsData = curSymbolSection.getData();
	Elf32_Sym * curSymbol  = (Elf32_Sym *) symbolsData.getBuffer();
	cString symName = "";
	cElfSymbol::SYMBOL_TYPES symType = cElfSymbol::SYMTYPE_EXTERN;
	bool symVis = false;
	uint32 symSecIndex = 0;

	while (curSymbol < (Elf32_Sym *)(symbolsData.getBuffer() + symbolsData.getSize()))
    {
        // Fix endian
        curSymbol->st_name  = ((basicInput&)stream).readUint32((const uint8*)&curSymbol->st_name);
        curSymbol->st_value = ((basicInput&)stream).readUint32((const uint8*)&curSymbol->st_value);
        curSymbol->st_size  = ((basicInput&)stream).readUint32((const uint8*)&curSymbol->st_size);
        curSymbol->st_shndx  = ((basicInput&)stream).readUint16((const uint8*)&curSymbol->st_shndx);

        // Add symbol
		symName = (0 == curSymbol->st_name) ? "" : stringTable.getDataAt(curSymbol->st_name);
		symType = cElfSymbol::info2type(curSymbol->st_info);
		symVis  = cElfSymbol::info2visiable(curSymbol->st_info);
		symSecIndex = this->oldCountToNewCount(curSymbol->st_shndx);

		this->m_symbols.append(cElfSymbolPtr(new cElfSymbol(symName, curSymbol->st_value, curSymbol->st_size, symVis, symType, symSecIndex)));
		curSymbol++;
	}
}

void cElfFile::readSectionsHeader(basicIO& stream)
{
	int totalSize = 0;
	cArray<Elf32_Shdr> Sections;
	cString curName = "";
	cElfSectionPtr newSection;
	uint32 skippedSections = 0;
	int i = 0;
	int curInfo = 0;

	// Sanity checks
	CHECK(0 != this->m_elfHeader.e_shoff);
	CHECK(this->m_elfHeader.e_shentsize == sizeof(Elf32_Shdr));

	// Now i need to allocate the number of elements i have.
	Sections.changeSize(this->m_elfHeader.e_shnum, true);
	totalSize = this->m_elfHeader.e_shentsize * this->m_elfHeader.e_shnum;

	// Reads the file offset
	stream.seek(this->m_elfHeader.e_shoff, basicInput::IO_SEEK_SET);
    for (i = 0; i < this->m_elfHeader.e_shnum; ++i)
    {
        stream.streamReadUint32(Sections[i].sh_name);
        stream.streamReadUint32(Sections[i].sh_type);
        stream.streamReadUint32(Sections[i].sh_flags);
        stream.streamReadUint32(Sections[i].sh_addr);
        stream.streamReadUint32(Sections[i].sh_offset);
        stream.streamReadUint32(Sections[i].sh_size);
        stream.streamReadUint32(Sections[i].sh_link);
        stream.streamReadUint32(Sections[i].sh_info);
        stream.streamReadUint32(Sections[i].sh_addralign);
        stream.streamReadUint32(Sections[i].sh_entsize);
    }

	// First read names table.
	cElfStringTable cNamesTable(stream,
							    Sections[this->m_elfHeader.e_shstrndx].sh_offset,
								Sections[this->m_elfHeader.e_shstrndx].sh_size);

	// Foreach Section:
	skippedSections = 0;
	cArray<Elf32_Shdr>::iterator curSectionHdr = Sections.begin();
	for (i = 0; curSectionHdr != Sections.end(); ++curSectionHdr, ++i) {
		curName = cNamesTable.getDataAt((*curSectionHdr).sh_name);
		switch( (*curSectionHdr).sh_type ) {
		case SHT_SYMTAB:
			this->updateSymbolTable(stream, (*curSectionHdr), Sections[(*curSectionHdr).sh_link], skippedSections);
			// Skip SymTable
			skippedSections++;
			break;
		case SHT_STRTAB:
			skippedSections++;
			break;
		case SHT_REL:
			curInfo = (*curSectionHdr).sh_info;
			if ( curInfo > i ) {
				// if curInfo is after my current pos i need to calc it with skipped sections.
				curInfo -= skippedSections;
			}
			this->m_relocations.append( cElfRelocationSectionPtr(new cElfRelocationSection(curInfo, stream, (*curSectionHdr))) );
			skippedSections++;
			break;
		default:
			// creates the section
			newSection = cElfSectionPtr(new cElfSection(stream, (*curSectionHdr)));
			(*newSection).setName(curName);
			(*newSection).setOldCount(i);

			// add it to list
			this->m_sections.append(newSection);
			break;
		}
	}
}

void cElfFile::read(basicIO& _stream, bool shouldReadSections)
{
	// Reads the Header to myself
	_stream.seek(0, basicInput::IO_SEEK_SET);
	_stream.pipeRead(&this->m_elfHeader, sizeof(Elf32_Ehdr));
	// Makes sure magic is fine.
	CHECK(0 == memcmp(this->m_elfHeader.e_ident, ELFMAG, SELFMAG));
    CHECK(ELFCLASS32 == this->m_elfHeader.e_ident[4]);
    // Use little-endian vs big-endian stream
    cSmartPtr<cEndianFilterStream> endianStream;
    if (this->m_elfHeader.e_ident[5] == ELFDATA2LSB)
    {
        endianStream = cSmartPtr<cEndianFilterStream>(new cEndianFilterStream(cSmartPtr<basicIO>(&_stream, SMARTPTR_DESTRUCT_NONE), true));
    } else if (this->m_elfHeader.e_ident[5] == ELFDATA2MSB)
    {
        endianStream = cSmartPtr<cEndianFilterStream>(new cEndianFilterStream(cSmartPtr<basicIO>(&_stream, SMARTPTR_DESTRUCT_NONE), false));
    } else
    {
        CHECK_FAIL();
    }
    basicIO& stream(*endianStream);
	stream.seek(EI_NIDENT, basicInput::IO_SEEK_SET);

    stream.streamReadUint16(this->m_elfHeader.e_type);
    stream.streamReadUint16(this->m_elfHeader.e_machine);
    stream.streamReadUint32(this->m_elfHeader.e_version);
    stream.streamReadUint32(this->m_elfHeader.e_entry);
    stream.streamReadUint32(this->m_elfHeader.e_phoff);
    stream.streamReadUint32(this->m_elfHeader.e_shoff);
    stream.streamReadUint32(this->m_elfHeader.e_flags);
    stream.streamReadUint16(this->m_elfHeader.e_ehsize);
    stream.streamReadUint16(this->m_elfHeader.e_phentsize);
    stream.streamReadUint16(this->m_elfHeader.e_phnum);
    stream.streamReadUint16(this->m_elfHeader.e_shentsize);
    stream.streamReadUint16(this->m_elfHeader.e_shnum);
    stream.streamReadUint16(this->m_elfHeader.e_shstrndx);

	// Makes sure the header's size match (it should match).
	CHECK(this->m_elfHeader.e_ehsize == sizeof(Elf32_Ehdr));

	if (shouldReadSections)
    {
		if (0 != this->m_elfHeader.e_shoff)
        {
			this->readSectionsHeader(stream);
		}
	}
}

void cElfFile::createSymbolTableAndNamesSection()
{
	cBuffer symTableData;
	cElfStringTable stringTable;
	Elf32_Sym curSym;
	uint32 pos;
	uint32 localSymbolsCount = 0;

	symTableData.changeSize(0, true);
	cArray<cElfSymbolPtr>::iterator curSymbol = this->m_symbols.begin();
    for (;curSymbol != this->m_symbols.end(); ++curSymbol)
    {
		(*curSymbol)->getHeader(curSym);
		if ( (*curSymbol)->getType() == cElfSymbol::SYMTYPE_SECTION ) {
			// Sections doesn't need to have name.
			curSym.st_name = 0;
		} else {
			curSym.st_name = stringTable.addString((*curSymbol)->getName());
		}
		localSymbolsCount += (*curSymbol)->isVisiable() ? 0 : 1;

		// Extend the struct.
		pos = symTableData.getSize();
		symTableData.changeSize(pos + sizeof(Elf32_Sym));
		memcpy(symTableData.getBuffer() + pos,
			   &curSym,
			   sizeof(Elf32_Sym));
    }

	this->m_symbolTableId = this->m_sections.getSize();
	this->m_strTableId    = this->m_symbolTableId + 1;								// will always be after symbolTable.
	this->m_shstrTableId  = this->m_strTableId + 1 + this->m_relocations.getSize(); // will always be after relocations are placed.

	this->m_sections.append(cElfSectionPtr(new cElfSection(cElfConsts::SYMBOLTABLE_SECTION_NAME(),
														   symTableData,
                                                           SHT_SYMTAB,
                                                           0,						  /* No Flags */
														   0,                         /* No nametable index yet */
														   0x4,						  /* Aligned to 4 */
														   localSymbolsCount,		  /* Info is where globals start which is localSymbolsCount */
														   this->m_strTableId,		  /* Link set to my string table */
														   sizeof(Elf32_Sym))));
	this->m_sections.append(cElfSectionPtr(new cElfSection(cElfConsts::STRING_TABLE_DEFAULT_NAME(),
															stringTable.getRawData(),
															SHT_STRTAB,
															0,
															0,
															0x1)));
}

void cElfFile::saveAs(basicIO& stream)
{
    cSmartPtr<cEndianFilterStream> endianStream;
    if (this->m_elfHeader.e_ident[5] == ELFDATA2LSB)
    {
        endianStream = cSmartPtr<cEndianFilterStream>(new cEndianFilterStream(cSmartPtr<basicIO>(&stream, SMARTPTR_DESTRUCT_NONE), true));
    } else if (this->m_elfHeader.e_ident[5] == ELFDATA2MSB)
    {
        endianStream = cSmartPtr<cEndianFilterStream>(new cEndianFilterStream(cSmartPtr<basicIO>(&stream, SMARTPTR_DESTRUCT_NONE), false));
    }
	this->write(*endianStream);
}

void cElfFile::addRelocations()
{
	cString curName = "";
	Elf32_Shdr curHdr;

	cArray<cElfRelocationSectionPtr>::iterator curRel = this->m_relocations.begin();
	for ( ; curRel != this->m_relocations.end() ; ++curRel ) {
		curName = cString(".rel") + this->m_sections[(*curRel)->getSectionId()]->getName();

		// Reads the header
		(*curRel)->getHeader(curHdr);

		this->m_sections.append(cElfSectionPtr(new cElfSection( curName,
																(*curRel)->getData(),
                                                                curHdr.sh_type,
                                                                curHdr.sh_flags,
																0,
																curHdr.sh_addralign,
																curHdr.sh_info,
																this->m_symbolTableId,
																curHdr.sh_entsize)));
	}
}

void cElfFile::write(basicIO& stream)
{
	cBuffer newFileBytes;
	uint32 oldSectionsCount = this->m_sections.getSize();
	uint32 oldRelocationsCount = this->m_relocations.getSize();

	if ( this->m_hasDbgInfo ) {
		this->addDbgSections();
	}

	// Prepare the names and symbols sections.
	this->createSymbolTableAndNamesSection();
	this->addRelocations();
	this->createSectionsNameSection();

	// Creating the file.
	cElfMemoryMapper memMapper(this->m_sections);

	// Object does not have entry.
	this->m_elfHeader.e_entry = 0;

	// Elf Object does not have program headers
	this->m_elfHeader.e_phoff = 0;
	this->m_elfHeader.e_phnum = 0;
	this->m_elfHeader.e_phentsize = 0;
	this->m_elfHeader.e_shoff = memMapper.getSectionsTableOffset();
	this->m_elfHeader.e_shentsize = sizeof(Elf32_Shdr);
	this->m_elfHeader.e_shnum = memMapper.getSectionsTableCount();
	this->m_elfHeader.e_shstrndx = memMapper.getNamesSectionIndex();

	// Write the ELF header
    stream.pipeWrite(this->m_elfHeader.e_ident, EI_NIDENT);
    stream.streamWriteUint16(this->m_elfHeader.e_type);
    stream.streamWriteUint16(this->m_elfHeader.e_machine);
    stream.streamWriteUint32(this->m_elfHeader.e_version);
    stream.streamWriteUint32(this->m_elfHeader.e_entry);
    stream.streamWriteUint32(this->m_elfHeader.e_phoff);
    stream.streamWriteUint32(this->m_elfHeader.e_shoff);
    stream.streamWriteUint32(this->m_elfHeader.e_flags);
    stream.streamWriteUint16(this->m_elfHeader.e_ehsize);
    stream.streamWriteUint16(this->m_elfHeader.e_phentsize);
    stream.streamWriteUint16(this->m_elfHeader.e_phnum);
    stream.streamWriteUint16(this->m_elfHeader.e_shentsize);
    stream.streamWriteUint16(this->m_elfHeader.e_shnum);
    stream.streamWriteUint16(this->m_elfHeader.e_shstrndx);

	// Appends the file content itself.
	newFileBytes = memMapper.getFileContent();
	stream.pipeWrite(newFileBytes, newFileBytes.getSize());

	// Remove the added sections.
	this->m_sections.changeSize(oldSectionsCount, true);
	this->m_relocations.changeSize(oldRelocationsCount, true);
}

void cElfFile::createSectionsNameSection()
{
	uint32 stringTableNamePos = 0;
	cElfStringTable stringTable;

	cArray<cElfSectionPtr>::iterator curSection = m_sections.begin();
    for (;curSection != m_sections.end(); ++curSection)
    {
		(*curSection)->setNameTableIndex(stringTable.addString((*curSection)->getName()));
    }

	// Not preset, need to add.
	stringTableNamePos = stringTable.addString(cElfConsts::SECTIONS_STRING_TABLE_DEFAULT_NAME());
	this->m_elfHeader.e_shstrndx = this->m_sections.getSize();
	this->m_sections.append(cElfSectionPtr(new cElfSection(cElfConsts::SECTIONS_STRING_TABLE_DEFAULT_NAME(),
														   stringTable.getRawData(),
                                                           SHT_STRTAB,
														   0,
														   stringTableNamePos)));
}

int32 cElfFile::findSectionByName(const cString & sectionName)
{
	int count = 0;

	cArray<cElfSectionPtr>::iterator curSection = m_sections.begin();
    for (count = 0; curSection != m_sections.end() ; ++curSection, count ++)
    {
		if ( (*curSection)->getName() == sectionName ) {
			return count;
		}
    }

	return -1;
}

int32 cElfFile::findSymbolByName(const cString & symbolName)
{
	int count = 0;
	cArray<cElfSymbolPtr>::iterator curSymbol = this->m_symbols.begin();
    for (count = 0;curSymbol != this->m_symbols.end(); ++curSymbol, count ++)
    {
		if ( (*curSymbol)->getName() == symbolName ) {
			return count;
		}
    }

	return -1;
}

void cElfFile::addSymbol(const cString & symbolName, cElfSymbol::SYMBOL_TYPES symbolType, const cString & sectionName, uint32 offsetFromSection, uint32 symbolSize, bool isVisible)
{
	int32 sectionId = this->findSectionByName(sectionName);

	CHECK ( sectionId != -1 );
	CHECK ( this->findSymbolByName(symbolName) == -1 );

	this->m_symbols.append(cElfSymbolPtr(new cElfSymbol(symbolName, offsetFromSection, symbolSize, isVisible, symbolType, sectionId)));
}

void cElfFile::removeSymbol(const cString & symbolName)
{
	int removedId = 0;
	int count = 0;
	int lastSym = this->m_symbols.getSize()-1;
	CHECK ( this->findSymbolByName(symbolName) != -1 );

	cArray<cElfSymbolPtr>::iterator curSymbol = this->m_symbols.begin();
	for (count = 0;curSymbol != this->m_symbols.end(); ++curSymbol, count ++)
	{
		if ( (*curSymbol)->getName() == symbolName ) {
			removedId = count;
			this->m_symbols[count] = this->m_symbols[lastSym];
			this->m_symbols.changeSize(lastSym);
			break;
		}
	}

	cArray<cElfRelocationSectionPtr>::iterator curRel = this->m_relocations.begin();
	for (;curRel != this->m_relocations.end(); ++curRel)
	{
		(*curRel)->removeRel(removedId);
		(*curRel)->changeRel(lastSym, removedId);
	}
}

void cElfFile::addSection(uint32 sectionType, const cString & sectionName, const cBuffer & data, uint32 flags)
{
    int32 sectionId = this->findSectionByName(sectionName);

    CHECK ( sectionId == -1 );

    this->m_sections.append(cElfSectionPtr(new cElfSection(sectionName, data, sectionType, flags, 0, 4)));
}

void cElfFile::useSymbol(const cString & symbolName, const cString & sectionName, uint32 relType, uint32 offsetFromSection)
{
	int32 symbolId = this->findSymbolByName(symbolName);
	int32 sectionId = this->findSectionByName(sectionName);
	int32 relocationId = -1;
	uint32 i = 0;

	CHECK( symbolId != -1 );
	CHECK( sectionId != -1 );

	// Looking for relocation.
	for ( i = 0 ; i < this->m_relocations.getSize() ; i ++ ) {
		if ( this->m_relocations[i]->getSectionId() == sectionId ) {
			relocationId = i;
			break;
		}
	}

	// Relocation doesn't exsist, needs to add it.
	if ( -1 == relocationId ) {
		relocationId = this->m_relocations.getSize();
		this->m_relocations.append( cElfRelocationSectionPtr(new cElfRelocationSection(sectionId)) );
	}

	this->m_relocations[relocationId]->addRel(symbolId, relType, offsetFromSection);

}

void cElfFile::setArch(Elf32_Half newMachine)
{
	this->m_elfHeader.e_machine = newMachine;
}

Elf32_Half cElfFile::getArch()
{
	return this->m_elfHeader.e_machine;
}

bool cElfFile::hasSymbol(const cString & symName)
{
    return ( -1 != this->findSymbolByName(symName) );
}

void cElfFile::addDbgSections()
{
	cBuffer tmpSection;

	if ( false == this->m_hasDbgInfo ) {
		traceHigh("ELFFile: Can't add dbg sections without debug initiate" << endl);
		CHECK_FAIL();
	}


	this->m_dbgManager->getDebugAbbSec(tmpSection);
	if ( tmpSection.getSize() > 0 ) {
		this->m_sections.append(cElfSectionPtr(new cElfSection(cString(".debug_abbrev"),	//TODO: Add to strings
			tmpSection,
			SHT_PROGBITS,
			0,
			0,
			0)));
	}

	this->m_dbgManager->getDebugInfoSec(tmpSection);
	if ( tmpSection.getSize() > 0 ) {
		this->m_sections.append(cElfSectionPtr(new cElfSection(cString(".debug_info"),	//TODO: Add to strings
																tmpSection,
																SHT_PROGBITS,
																0,
																0,
																0)));
	}

	this->m_dbgManager->getDebugStrSec(tmpSection);
	if ( tmpSection.getSize() > 0 ) {
		this->m_sections.append(cElfSectionPtr(new cElfSection(cString(".debug_str"),	//TODO: Add to strings
			tmpSection,
			SHT_PROGBITS,
			0,
			0,
			0)));
	}

	this->m_dbgManager->getDebugLineSec(tmpSection);
	if ( tmpSection.getSize() > 0 ) {
		this->m_sections.append(cElfSectionPtr(new cElfSection(cString(".debug_line"),	//TODO: Add to strings
			tmpSection,
			SHT_PROGBITS,
			0,
			0,
			0)));
	}

	this->m_dbgManager->getDebugLocSec(tmpSection);
	if ( tmpSection.getSize() > 0 ) {
		this->m_sections.append(cElfSectionPtr(new cElfSection(cString(".debug_loc"),	//TODO: Add to strings
			tmpSection,
			SHT_PROGBITS,
			0,
			0,
			0)));
	}

	this->m_dbgManager->getDebugPubNamesSec(tmpSection);
	if ( tmpSection.getSize() > 0 ) {
		this->m_sections.append(cElfSectionPtr(new cElfSection(cString(".debug_pubnames"),	//TODO: Add to strings
			tmpSection,
			SHT_PROGBITS,
			0,
			0,
			0)));
	}

	this->m_dbgManager->getDebugPubTypesSec(tmpSection);
	if ( tmpSection.getSize() > 0 ) {
		this->m_sections.append(cElfSectionPtr(new cElfSection(cString(".debug_pubtypes"),	//TODO: Add to strings
			tmpSection,
			SHT_PROGBITS,
			0,
			0,
			0)));
	}

	this->m_dbgManager->getDebugArrangesSec(tmpSection);
	if ( tmpSection.getSize() > 0 ) {
		this->m_sections.append(cElfSectionPtr(new cElfSection(cString(".debug_aranges"),	//TODO: Add to strings
			tmpSection,
			SHT_PROGBITS,
			0,
			0,
			0)));
	}

	this->m_dbgManager->getDebugFrameSec(tmpSection);
	if ( tmpSection.getSize() > 0 ) {
		this->m_sections.append(cElfSectionPtr(new cElfSection(cString(".debug_frame"),	//TODO: Add to strings
			tmpSection,
			SHT_PROGBITS,
			0,
			0,
			0)));
	}
}

void cElfFile::debugInit(const cString & codeSectionName, const cString &  fileName, const cString &  filePath, int dwarfVersion)
{
	int textSectionIndex = findSectionByName(codeSectionName);

	// TODO: Create Dwarf Factory and support more versions
	// of Dwarf.
	if ( dwarfVersion != 2 ) {
		traceHigh("ELFFile: Only DWARF Version 2 is supported at the moment." << endl);
		CHECK_FAIL();
	}

	this->m_hasDbgInfo = true;
	this->m_dbgManager = cDwarf2ManagerPtr(new cDwarf2Manager(cElfConsts::DWARF_DEFAULT_PRODUCER(),
																fileName,
																filePath,
																4, //TODO: Get Pointer size per arch, Arch is at this->m_elfHeader.e_machine.
																this->m_sections[textSectionIndex]->getData().getSize()));
}
