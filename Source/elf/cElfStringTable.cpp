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

#include "elf/cElfStringTable.h"

// TODO: Remove them, debug only
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

cElfStringTable::cElfStringTable( cBuffer rawData ) : m_data(rawData) {}
cElfStringTable::cElfStringTable( basicIO & stream,
								  uint32 data_offset,
								  uint32 data_size)
{
	this->m_data.changeSize(data_size, true);
	stream.seek(data_offset, basicInput::IO_SEEK_SET);
	stream.pipeRead(this->m_data.getBuffer(), data_size);
}
cElfStringTable::cElfStringTable()
{
	this->m_data.changeSize(0, true);
}

cElfStringTable::~cElfStringTable()
{
	this->m_data.changeSize(0, true);
}

uint32 cElfStringTable::addString( cString newString )
{
	uint32 pos = this->m_data.getSize();
	cSArray<char> arrString;

	arrString = newString.getASCIIstring();
	this->m_data.changeSize(pos + arrString.getSize(), true);
	memcpy(this->m_data.getBuffer() + pos,
		   arrString.getBuffer(),
		   arrString.getSize());

	return pos;
}

cString cElfStringTable::getDataAt( uint32 pos )
{
	cString ret = "";
	for ( int i = pos ; this->m_data[i] != '\0' ; i ++ ) {
		ret += this->m_data[i];
	}

	return ret;
}

const cBuffer& cElfStringTable::getRawData()
{
	return this->m_data;
}
