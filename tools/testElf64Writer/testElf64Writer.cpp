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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef LINUX
#include <sys/wait.h>
#endif

#include "elf/cElf64ObjectWriter.h"

#include "xStl/types.h"
#include "xStl/data/string.h"
#include "xStl/except/trace.h"
#include "xStl/except/exception.h"
#include "xStl/stream/fileStream.h"
#include "xStl/stream/ioStream.h"

// The function answer(): mov eax, 42 ; ret
static const uint8 ANSWER_CODE[] = { 0xb8, 0x2a, 0x00, 0x00, 0x00, 0xc3 };
static const int ANSWER_EXPECTED_EXIT = 42;

// Writes an ELF64 object that defines the global function 'answer'.
static void writeAnswerObject(const char* outputPath)
{
	cBuffer code;
	code.changeSize(sizeof(ANSWER_CODE), true);
	memcpy(code.getBuffer(), ANSWER_CODE, sizeof(ANSWER_CODE));

	cElf64ObjectWriter writer(EM_X86_64);
	uint text = writer.addSection(cString(".text"), SHT_PROGBITS, SHF_ALLOC | SHF_EXECINSTR, 16, code);
	writer.addSymbol(cString("answer"), text, 0, sizeof(ANSWER_CODE), true, true);

	cFileStream output(cString(outputPath), cFile::WRITE | cFile::CREATE | cFile::SHARE_READ | cFile::SHARE_WRITE);
	writer.save(output);
}

#ifdef LINUX
// Links the object with a C main() using the system compiler, runs it and
// checks that the exit status is the value returned by answer().
static bool linkAndRun(const char* objectPath, const char* mainPath)
{
	char executable[4096];
	char command[8192];
	int status = 0;

	snprintf(executable, sizeof(executable), "%s.run", objectPath);
	snprintf(command, sizeof(command), "cc -o \"%s\" \"%s\" \"%s\"", executable, mainPath, objectPath);
	if (0 != system(command))
	{
		cout << "Linking the ELF64 object failed.\n";
		return false;
	}

	snprintf(command, sizeof(command), "\"%s\"", executable);
	status = system(command);
	if ((-1 == status) || !WIFEXITED(status))
	{
		cout << "The linked executable did not exit normally.\n";
		return false;
	}
	if (ANSWER_EXPECTED_EXIT != WEXITSTATUS(status))
	{
		cout << "Unexpected exit status from the linked executable.\n";
		return false;
	}

	return true;
}
#endif

int main(int argc, char * argv[])
{
	XSTL_TRY
	{
		if (argc != 3)
		{
			cout << "Usage: testElf64Writer <output.o> <main.c>\n";
			return RC_ERROR;
		}

		writeAnswerObject(argv[1]);

#ifdef LINUX
		if (!linkAndRun(argv[1], argv[2]))
		{
			return RC_ERROR;
		}
#endif

		return 0;
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
