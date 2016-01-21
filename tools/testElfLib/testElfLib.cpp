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
#include "xStl/os/virtualMemoryAccesser.h"
#include "xStl/os/threadUnsafeMemoryAccesser.h"
#include "xStl/stream/memoryAccesserStream.h"

#define OUTPUT_FILENAME ("output.elf")

void copyfile(char * in, char * outname) {
	// Load a file
	cFileStream elfInputFileStream(in);
	cFileStream elfOutputFileStream(outname, cFile::WRITE | cFile::CREATE | cFile::SHARE_READ |  cFile::SHARE_WRITE);

	cElfFile cCurFile(elfInputFileStream, true);
	cCurFile.addSymbol("retFunc", cElfSymbol::SYMTYPE_FUNC, ".text", 0x9, 1, true);
	//cCurFile.useSymbol("gCounterCopy", ".text", R_386_32, 4);
	cCurFile.saveAs(elfOutputFileStream);
}

int main(int argc, char * argv[])
{
	XSTL_TRY
	{
		if (argc != 2)
		{
			cout << "Usage: dumpPE <filename>";
			return RC_ERROR;
		}

		copyfile(argv[1], OUTPUT_FILENAME);
		cFileStream elfTestRead(OUTPUT_FILENAME);
 		cElfFile outputReadTest(elfTestRead, true);

		return 1;
	}
	XSTL_CATCH(cException& e)
	{
		// Print the exception
		e.print();
		return RC_ERROR;
	}
	XSTL_CATCH_ALL
	{
		TRACE(TRACE_VERY_HIGH,
			XSTL_STRING("Unknwon exceptions caught at main()..."));
		return RC_ERROR;
	}

	return 0;
}
