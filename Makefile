# Makefile

WARNINGS        = -Wno-padded -Wno-cast-align -Wno-unreachable-code -Wno-packed -Wno-missing-noreturn -Wno-float-equal -Wno-unused-macros -Werror=return-type -Wextra -Wno-unused-parameter -Wno-trigraphs

COMMON_CFLAGS   = -Wall -O0

CC              ?= clang
CXX             ?= clang++

CFLAGS          = $(COMMON_CFLAGS) -std=c99 -fPIC -O3
CXXFLAGS        = $(COMMON_CFLAGS) -Wno-old-style-cast -std=c++17 -fno-exceptions
OBJCFLAGS       = $(COMMON_CFLAGS)

CXXSRC          = $(shell find source -iname "*.cpp" -print)
CXXOBJ          = $(CXXSRC:.cpp=.cpp.o)
CXXDEPS         = $(CXXOBJ:.o=.d)


ifeq ("$(shell uname)","Darwin")
	OBJCSRC     = $(shell find source -iname "*.m" -print)
	OBJCOBJ     = $(OBJCSRC:.m=.m.o)

	SDL_LINK    = /usr/local/lib/libSDL2.a $(shell pkg-config --libs-only-other --static sdl2) -liconv
else
	OBJCSRC     =
	OBJCOBJ     =

	SDL_LINK    = $(shell pkg-config --libs sdl2) -ldl -lGL
endif

LIBS            = sdl2
FRAMEWORKS      =

IMGUI_SRCS      = $(shell find external/imgui -iname "*.cpp" -print)
IMGUI_OBJS      = $(IMGUI_SRCS:.cpp=.cpp.o)

GL3W_SRCS       = $(shell find external/GL -iname "*.c" -print)
GL3W_OBJS       = $(GL3W_SRCS:.c=.c.o)

UTF8PROC_SRCS   = external/utf8proc/utf8proc.c
UTF8PROC_OBJS   = $(UTF8PROC_SRCS:.c=.c.o)

PRECOMP_HDRS    := source/include/precompile.h
PRECOMP_GCH     := $(PRECOMP_HDRS:.h=.h.gch)

DEFINES         := -DIMGUI_IMPL_OPENGL_LOADER_GL3W=1
INCLUDES        := -Isource/include -Iexternal

OUTPUT_BIN      := build/palpha

.PHONY: all clean build output_headers
.PRECIOUS: $(PRECOMP_GCH)
.DEFAULT_GOAL = all

all: build

test: build
	@cd build && ./$(notdir $(OUTPUT_BIN))

build: $(OUTPUT_BIN)

$(OUTPUT_BIN): $(CXXOBJ) $(IMGUI_OBJS) $(GL3W_OBJS) $(UTF8PROC_OBJS) $(OBJCOBJ)
	@echo "  $(notdir $@)"
	@$(CXX) $(CXXFLAGS) $(WARNINGS) $(DEFINES) -Iexternal -o $@ $^ $(SDL_LINK)

%.cpp.o: %.cpp Makefile $(PRECOMP_GCH)
	@echo "  $(notdir $<)"
	@$(CXX) $(CXXFLAGS) $(WARNINGS) $(INCLUDES) $(DEFINES) -include source/include/precompile.h -MMD -MP -c -o $@ $< \
		$(shell pkg-config --cflags $(LIBS))

%.c.o: %.c Makefile
	@echo "  $(notdir $<)"
	@$(CC) $(CFLAGS) -MMD -MP -c -o $@ $<

%.m.o: %.m Makefile
	@echo "  $(notdir $<)"
	@$(CC) -fmodules -mmacosx-version-min=10.12 -c -o $@ $<

%.h.gch: %.h Makefile
	@printf "# precompiling header $<\n"
	@$(CXX) $(CXXFLAGS) $(WARNINGS) $(INCLUDES) -x c++-header -o $@ $<

clean:
	-@find source -iname "*.cpp.d" | xargs rm
	-@find source -iname "*.cpp.o" | xargs rm
	-@find build -iname "*.a" | xargs rm
	-@rm $(PRECOMP_GCH)

-include $(CXXDEPS)
-include $(CDEPS)












