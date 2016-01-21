INCLUDES = -I $(top_srcdir)/Include -I $(XSTL_PATH)/Include

if DEBUG
DBGFLAGS = -ggdb -DDEBUG
else
DBGFLAGS = -g
endif

lib_LTLIBRARIES = libelf.la

libelf_la_SOURCES = cElfFile.cpp cElfRelocationSection.cpp cElfStringTable.cpp cElfMemoryMapper.cpp cElfSection.cpp cElfSymbol.cpp \
                    Dwarf/cDwarf2Manager.cpp  Dwarf/cDwarfAttribute.cpp  Dwarf/cDwarfDie.cpp


libelf_la_CFLAGS = $(CFLAGS_ELFLIB_COMMON) $(DBGFLAGS) $(AM_CFLAGS)
libelf_la_CPPFLAGS = $(CFLAGS_ELFLIB_COMMON) $(DBGFLAGS) $(AM_CFLAGS)

if UNICODE
libelf_la_CFLAGS+= -DXSTL_UNICODE -D_UNICODE
libelf_la_CPPFLAGS+= -DXSTL_UNICODE -D_UNICODE
endif

