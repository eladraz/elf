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
#include <string.h>
#include <time.h>

#include "elf/cElf64Image.h"
#include "elf/opensource_headers/elf.h"

#include "xStl/types.h"
#include "xStl/data/sarray.h"
#include "xStl/data/string.h"
#include "xStl/except/exception.h"

// R_RISCV_64 is not in the opensource elf.h; value from the RISC-V psABI.
static const uint32 R_RISCV_64_TYPE = 2;

static const uint32 EXPECTED_FUNCS = 2;
static const uint32 EXPECTED_ABS64 = 2;
static const uint32 MALFORMED_VARIANTS = 6;

static bool readFile(const char* path, cBuffer& out)
{
	FILE* file = fopen(path, "rb");
	if (NULL == file)
	{
		return false;
	}
	fseek(file, 0, SEEK_END);
	long size = ftell(file);
	fseek(file, 0, SEEK_SET);
	if (size <= 0)
	{
		fclose(file);
		return false;
	}
	out.changeSize((uint)size, true);
	size_t got = fread(out.getBuffer(), 1, (size_t)size, file);
	fclose(file);
	return got == (size_t)size;
}

static uint64 getLE(const uint8* p, uint32 size)
{
	uint64 value = 0;
	for (uint32 i = 0; i < size; i++)
	{
		value |= (uint64)p[i] << (8 * i);
	}
	return value;
}

static void putLE(uint8* p, uint64 value, uint32 size)
{
	for (uint32 i = 0; i < size; i++)
	{
		p[i] = (uint8)(value >> (8 * i));
	}
}

// Finds the first section header of the given type; returns its file offset
// of the header, or 0 when absent.
static uint64 findSectionHeader(const cBuffer& image, uint32 type)
{
	const uint8* b = image.getBuffer();
	const uint64 shOff = getLE(b + 40, 8);
	const uint64 shNum = getLE(b + 60, 2);
	for (uint64 i = 0; i < shNum; i++)
	{
		uint64 header = shOff + i * 64;
		if (getLE(b + header + 4, 4) == type)
		{
			return header;
		}
	}
	return 0;
}

// Returns true when the image is well formed and matches the expected fixture.
static bool checkImage(const char* path, const char* name, uint16 machine)
{
	cBuffer bytes;
	if (!readFile(path, bytes))
	{
		printf("elf64: cannot read %s\n", path);
		return false;
	}

	try
	{
		cElf64Image image(bytes);
		uint32 funcs = 0;
		uint32 abs64 = 0;
		const uint32 absType = (EM_AARCH64 == machine) ? R_AARCH64_ABS64 : R_RISCV_64_TYPE;

		if ((ELFCLASS64 != image.getClass()) || (ET_EXEC != image.getType()) || (machine != image.getMachine()) || (0 == image.getEntry()))
		{
			printf("elf64: %s header fields are wrong\n", name);
			return false;
		}
		for (uint i = 0; i < image.getSymbols().getSize(); i++)
		{
			const cElf64Image::cSymbol& sym = image.getSymbols()[i];
			if ((STT_FUNC == sym.type) && (0 != sym.value))
			{
				funcs++;
			}
		}
		for (uint i = 0; i < image.getRelocations().getSize(); i++)
		{
			if (absType == image.getRelocations()[i].type)
			{
				abs64++;
			}
		}

		const uint32 phdrs = (uint32)image.getProgramHeaders().getSize();
		// Sanity of the byte accessor: the first four bytes are the magic.
		cBuffer magic = image.getFileBytes(0, 4);
		if (0x7f != magic.getBuffer()[0] || 'E' != magic.getBuffer()[1])
		{
			printf("elf64: %s getFileBytes returned the wrong bytes\n", name);
			return false;
		}
		bool outOfRange = false;
		try
		{
			image.getFileBytes(bytes.getSize(), 1);
		}
		catch (cException&)
		{
			outOfRange = true;
		}
		if (!outOfRange)
		{
			printf("elf64: %s getFileBytes accepted a range past the end\n", name);
			return false;
		}

		if ((funcs != EXPECTED_FUNCS) || (abs64 != EXPECTED_ABS64) || (phdrs == 0))
		{
			printf("elf64: %s unexpected counts funcs=%u abs64=%u phdrs=%u\n", name, funcs, abs64, phdrs);
			return false;
		}
		printf("elf64: %s phdrs=%u funcs=%u abs64=%u OK\n", name, phdrs, funcs, abs64);
		return true;
	}
	catch (cException&)
	{
		printf("elf64: %s rejected a valid image\n", name);
		return false;
	}
}

// Each variant is a corrupted copy of the aarch64 fixture that the parser must reject.
static bool checkMalformed(const cBuffer& good)
{
	const char* names[MALFORMED_VARIANTS] = {
		"truncated header",
		"bad class",
		"phdr table past EOF",
		"section name offset past strtab",
		"symbol strtab out of range",
		"rela entsize not 24"
	};
	uint32 rejected = 0;

	for (uint v = 0; v < MALFORMED_VARIANTS; v++)
	{
		cBuffer bad;
		uint64 size = good.getSize();
		bad.changeSize((uint)size, true);
		memcpy(bad.getBuffer(), good.getBuffer(), (size_t)size);
		uint8* b = bad.getBuffer();

		switch (v)
		{
		case 0:
			bad.changeSize(40, true);
			break;
		case 1:
			b[EI_CLASS] = ELFCLASS32;
			break;
		case 2:
			putLE(b + 32, size, 8);                // e_phoff at the end of the file
			break;
		case 3:
			putLE(b + findSectionHeader(bad, SHT_PROGBITS), 0x7fffffff, 4);   // sh_name of the first PROGBITS section
			break;
		case 4:
		{
			uint64 symtab = findSectionHeader(bad, SHT_SYMTAB);
			uint64 symOff = getLE(b + symtab + 24, 8);
			putLE(b + symOff + 24, 0x7fffffff, 4); // st_name of symbol 1
			break;
		}
		case 5:
		{
			uint64 rela = findSectionHeader(bad, SHT_RELA);
			putLE(b + rela + 56, 16, 8);           // sh_entsize
			break;
		}
		}

		try
		{
			cElf64Image image(bad);
			printf("elf64: malformed %s accepted\n", names[v]);
		}
		catch (cException&)
		{
			printf("elf64: malformed %s rejected\n", names[v]);
			rejected++;
		}
	}

	printf("elf64: %u/%u malformed rejected\n", rejected, MALFORMED_VARIANTS);
	return rejected == MALFORMED_VARIANTS;
}

// Large synthetic input (clang-sized): LARGE_SYMBOLS symbols and LARGE_RELAS
// RELA entries in one ET_EXEC image. The parse must finish within LARGE_LIMIT_S.
static const uint32 LARGE_SYMBOLS = 200000;
static const uint32 LARGE_RELAS = 1000000;
static const double LARGE_LIMIT_S = 2.0;

// Builds the image: header, 5 section headers (null, .shstrtab, .strtab,
// .symtab, .rela.text), then the tables.
static void buildLargeImage(cBuffer& out)
{
	const uint64 shNum = 5;
	// Symbol names are "s<index>" plus NUL; the string table starts with a NUL.
	uint64 strtabSize = 1;
	char name[32];
	for (uint32 i = 1; i <= LARGE_SYMBOLS; i++)
	{
		strtabSize += (uint64)sprintf(name, "s%u", (unsigned)i) + 1;
	}
	const char shstr[] = "\0.shstrtab\0.strtab\0.symtab\0.rela.text";
	const uint64 shstrBytes = sizeof(shstr);   // includes the final NUL
	const uint64 shOff = 64;
	const uint64 shstrOff = shOff + shNum * 64;
	const uint64 strOff = shstrOff + shstrBytes;
	const uint64 symOff = strOff + strtabSize;
	const uint64 symBytes = (uint64)(LARGE_SYMBOLS + 1) * 24;
	const uint64 relOff = symOff + symBytes;
	const uint64 relBytes = (uint64)LARGE_RELAS * 24;
	const uint64 total = relOff + relBytes;

	out.changeSize((uint)total, true);
	uint8* b = out.getBuffer();
	memset(b, 0, (size_t)total);
	b[0] = 0x7f; b[1] = 'E'; b[2] = 'L'; b[3] = 'F';
	b[EI_CLASS] = ELFCLASS64;
	b[EI_DATA] = ELFDATA2LSB;
	b[EI_VERSION] = EV_CURRENT;
	putLE(b + 16, ET_EXEC, 2);
	putLE(b + 18, EM_AARCH64, 2);
	putLE(b + 20, EV_CURRENT, 4);
	putLE(b + 24, 0x400000, 8);
	putLE(b + 40, shOff, 8);
	putLE(b + 52, 64, 2);
	putLE(b + 58, 64, 2);
	putLE(b + 60, shNum, 2);
	putLE(b + 62, 1, 2);

	// Section headers: type, offset, size, link, info, entsize.
	struct cShdr { uint32 name; uint32 type; uint64 offset; uint64 size; uint32 link; uint32 info; uint64 entsize; };
	const cShdr sh[shNum] = {
		{ 0, SHT_NULL, 0, 0, 0, 0, 0 },
		{ 1, SHT_STRTAB, shstrOff, shstrBytes, 0, 0, 0 },
		{ 11, SHT_STRTAB, strOff, strtabSize, 0, 0, 0 },
		{ 19, SHT_SYMTAB, symOff, symBytes, 2, 1, 24 },
		{ 27, SHT_RELA, relOff, relBytes, 3, 0, 24 }
	};
	for (uint64 i = 0; i < shNum; i++)
	{
		uint8* p = b + shOff + i * 64;
		putLE(p, sh[i].name, 4);
		putLE(p + 4, sh[i].type, 4);
		putLE(p + 24, sh[i].offset, 8);
		putLE(p + 32, sh[i].size, 8);
		putLE(p + 40, sh[i].link, 4);
		putLE(p + 44, sh[i].info, 4);
		putLE(p + 56, sh[i].entsize, 8);
	}

	memcpy(b + shstrOff, shstr, (size_t)shstrBytes);

	// String table and symbols (index 0 is the null symbol).
	uint64 nameOff = 1;
	for (uint32 i = 1; i <= LARGE_SYMBOLS; i++)
	{
		int n = sprintf((char*)(b + strOff + nameOff), "s%u", (unsigned)i);
		uint8* p = b + symOff + (uint64)i * 24;
		putLE(p, nameOff, 4);
		p[4] = (uint8)((STB_GLOBAL << 4) | STT_FUNC);
		putLE(p + 6, 1, 2);
		putLE(p + 8, 0x400000 + (uint64)i * 16, 8);
		putLE(p + 16, 16, 8);
		nameOff += (uint64)n + 1;
	}

	// RELA entries: symbol index cycles over the table, type R_AARCH64_ABS64.
	for (uint32 i = 0; i < LARGE_RELAS; i++)
	{
		uint8* p = b + relOff + (uint64)i * 24;
		const uint64 symbol = 1 + (uint64)(i % LARGE_SYMBOLS);
		putLE(p, 0x500000 + (uint64)i * 8, 8);
		putLE(p + 8, (symbol << 32) | R_AARCH64_ABS64, 8);
		putLE(p + 16, i, 8);
	}
}

// Parses the large synthetic image; returns false on a wrong count or when slower than the limit.
static bool checkLarge()
{
	cBuffer image;
	buildLargeImage(image);

	clock_t start = clock();
	cElf64Image parsed(image);
	double seconds = (double)(clock() - start) / CLOCKS_PER_SEC;

	const uint32 syms = (uint32)parsed.getSymbols().getSize();
	const uint32 relas = (uint32)parsed.getRelocations().getSize();
	printf("elf64: large parse symbols=%u relocations=%u time=%.3fs\n", syms, relas, seconds);

	if ((syms != LARGE_SYMBOLS + 1) || (relas != LARGE_RELAS))
	{
		printf("elf64: large image has wrong counts\n");
		return false;
	}
	const cElf64Image::cSymbol& last = parsed.getSymbols()[LARGE_SYMBOLS];
	const cElf64Image::cRelocation& lastRela = parsed.getRelocations()[LARGE_RELAS - 1];
	char expected[32];
	sprintf(expected, "s%u", (unsigned)LARGE_SYMBOLS);
	if (!(last.name == cString(expected)) || (lastRela.symbolIndex != 1 + (LARGE_RELAS - 1) % LARGE_SYMBOLS) ||
	    (lastRela.type != R_AARCH64_ABS64) || (lastRela.addend != (int64)(LARGE_RELAS - 1)))
	{
		printf("elf64: large image has wrong contents\n");
		return false;
	}
	if (seconds >= LARGE_LIMIT_S)
	{
		printf("elf64: large parse took %.3fs (limit %.1fs)\n", seconds, LARGE_LIMIT_S);
		return false;
	}
	return true;
}

int main(int argc, char** argv)
{
	if (argc != 3)
	{
		printf("usage: testElf64Image <aarch64-exec.elf> <riscv64-exec.elf>\n");
		return 2;
	}

	bool ok = checkImage(argv[1], "aarch64", EM_AARCH64);
	ok = checkImage(argv[2], "riscv64", EM_RISCV) && ok;

	cBuffer good;
	if (!readFile(argv[1], good))
	{
		return 1;
	}
	ok = checkMalformed(good) && ok;
	ok = checkLarge() && ok;

	return ok ? 0 : 1;
}
