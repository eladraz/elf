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

#ifndef  __ELF_ELFSYMBOL_H__
#define  __ELF_ELFSYMBOL_H__
/*
 * cElfSymbol.h
 *
 * ELF Symbol Struct class
 * Class to hold and manage Symbols
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

class cElfSymbol
{
public:
	enum SYMBOL_TYPES
	{
        SYMTYPE_NONE,
		SYMTYPE_EXTERN,
		SYMTYPE_FUNC,
		SYMTYPE_OBJECT,
		SYMTYPE_SECTION,
		SYMTYPE_FILE,
	};

	// CTor
	cElfSymbol(cString symbolName,
				uint32 offsetFromSectionStart,
				uint32 symbolSize,
				bool isVisiable,
				SYMBOL_TYPES symbolType,
				uint32 sectionIndex);
	// DTor
	~cElfSymbol();

	//
	// This method will copy the header to the caller.
	//
	void getHeader(Elf32_Sym & output);

	//
	// will return symbol type.
	//
	SYMBOL_TYPES getType();

	//
	// Setter/getter for name.
	//
	cString getName();
	void setName(cString newName);

	//
	// is symbol visiable.
	//
	bool isVisiable();

	static SYMBOL_TYPES info2type(uint32 infoType);
	static bool info2visiable(uint32 infoType);

private:
	Elf32_Sym m_header;
	cString m_name;
};

// Ptr for Sections
typedef cSmartPtr<cElfSymbol> cElfSymbolPtr;

#endif // __ELF_ELFSYMBOL_H__
