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

#ifndef  __ELF_ELFSTRINGTABLE_H__
#define  __ELF_ELFSTRINGTABLE_H__
/*
 * cElfStringTable.h
 *
 * ELF String Table Class
 * Class to hold and manage string table sections.
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

class cElfStringTable {
public:
	// CTors
	cElfStringTable();
	cElfStringTable( cBuffer rawData );
	cElfStringTable( basicIO & stream, uint32 data_offset, uint32 data_size);

	// DTor
	~cElfStringTable();

	//
	// This method will add new string to the data buffer
	// and return his position to the caller.
	//
	uint32 addString( cString newString );

	//
	// this method will return the string on specified position.
	//
	cString getDataAt( uint32 pos );

	//
	// Returns the data as cBuffer ( To Add as section )
	//
	const cBuffer& getRawData();

private:
	cBuffer m_data;
};

typedef cSmartPtr<cElfStringTable> cElfStringTablePtr;
#endif // __ELF_ELFSTRINGTABLE_H__
