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

#include "elf/cElfSymbol.h"

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

cElfSymbol::cElfSymbol(cString symbolName,
					uint32 offsetFromSectionStart,
					uint32 symbolSize,
					bool isVisiable,
					SYMBOL_TYPES symbolType,
					uint32 sectionIndex) :
m_name(symbolName)
{
	uint32 binding = (isVisiable ? STB_GLOBAL : STB_LOCAL);
	uint32 symType = 0;
	uint32 OtherVal = ELF32_ST_VISIBILITY(STV_DEFAULT);
	switch ( symbolType ) {
        case cElfSymbol::SYMTYPE_NONE:
            symType = STT_NOTYPE;
        break;
		case cElfSymbol::SYMTYPE_EXTERN:
			symType = STT_NOTYPE;
			sectionIndex = SHN_UNDEF;
			binding = STB_GLOBAL;
			symbolSize = 0;
			offsetFromSectionStart = 0;
			break;
		case cElfSymbol::SYMTYPE_FUNC:
			symType = STT_FUNC;
			break;
		case cElfSymbol::SYMTYPE_OBJECT:
			symType = STT_OBJECT;
			break;
		case cElfSymbol::SYMTYPE_SECTION:
			symType = STT_SECTION;
			symbolSize = 0;
			offsetFromSectionStart = 0;
			binding = STB_LOCAL;
			break;
		case cElfSymbol::SYMTYPE_FILE:
			symType = STT_FILE;
			symbolSize = 0;
			offsetFromSectionStart = 0;
			sectionIndex = SHN_ABS;
			binding = STB_LOCAL;
			break;
	}

	m_header.st_name = 0;
	m_header.st_info = ELF32_ST_INFO(binding, symType);
	m_header.st_shndx = sectionIndex;
	m_header.st_size  = symbolSize;
	m_header.st_value = offsetFromSectionStart;
	m_header.st_other = OtherVal;
}

cElfSymbol::~cElfSymbol()
{

}

cString cElfSymbol::getName()
{
	return this->m_name;
}

void cElfSymbol::setName(cString newName)
{
	this->m_name = newName;
}

void cElfSymbol::getHeader(Elf32_Sym & output)
{
	memcpy(&output, &this->m_header, sizeof(Elf32_Sym));
}

cElfSymbol::SYMBOL_TYPES cElfSymbol::info2type(uint32 infoType)
{
	cElfSymbol::SYMBOL_TYPES ret = cElfSymbol::SYMTYPE_EXTERN;
	switch ( ELF32_ST_TYPE(infoType) ) {
		case STT_NOTYPE:
			ret = cElfSymbol::SYMTYPE_EXTERN;
			break;
		case STT_FUNC:
			ret = cElfSymbol::SYMTYPE_FUNC;
			break;
		case STT_OBJECT:
			ret = cElfSymbol::SYMTYPE_OBJECT;
			break;
		case STT_SECTION:
			ret = cElfSymbol::SYMTYPE_SECTION;
			break;
	}

	return ret;
}

cElfSymbol::SYMBOL_TYPES cElfSymbol::getType()
{
	return this->info2type(this->m_header.st_info);
}

bool cElfSymbol::info2visiable(uint32 infoType)
{
	return (ELF32_ST_BIND(infoType) == STB_GLOBAL);
}

bool cElfSymbol::isVisiable() {
	return info2visiable(this->m_header.st_info);
}