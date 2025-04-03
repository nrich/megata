MKDIR = mkdir -p
CXX = g++
CC = gcc
RM = rm -f
RMDIR = rm -rf
INC = -I src -I thirdparty -I thirdparty/raylib-cpp-5.5.0 -I thirdparty/miniz-3.0.2 -I thirdparty/emu2149-1.16 -I thirdparty/imgui-1.91.9 -I thirdparty/rlImGui -I thirdparty/nativefiledialog-extended-1.2.1 $(shell pkg-config --cflags gtk+-3.0)
LDFLAGS = -lraylib $(shell pkg-config --libs gtk+-3.0)
CPPFLAGS = -g -O3 -std=c++20 $(INC) -Wall
CFLAGS = -g -O3 $(INC) -Wall -pthread
STRIP = strip

UNAME := $(shell uname -s)

ifeq ($(findstring MINGW, $(UNAME)), MINGW)
    CONFIG_W64=1
endif
 
ifdef CONFIG_W64
    CXX = x86_64-w64-mingw32-g++
    CC = x86_64-w64-mingw32-gcc
    LDFLAGS = -mwindows -static-libgcc -static-libstdc++ -L /opt/local/raylib-5.5_win64_mingw-w64/lib -lraylib -lgdi32 -lopengl32 -lwinmm -lole32 -luuid -lshell32
    INC = -I src -I thirdparty -I thirdparty/raylib-cpp-5.5.0 -I /opt/local/raylib-5.5_win64_mingw-w64/include -I thirdparty/miniz-3.0.2 -I thirdparty/emu2149-1.16 -I thirdparty/imgui-1.91.9 -I thirdparty/rlImGui -I thirdparty/nativefiledialog-extended-1.2.1
    CPPFLAGS = -O3 -std=c++20 -pthread $(INC) -D_WIN64 -Wall
    ifeq ($(UNAME), Linux)
        WINDRES = x86_64-w64-mingw32-windres
        STRIP = x86_64-w64-mingw32-strip
    else
        WINDRES = windres
        STRIP = strip
    endif
    WINDRESARGS = 
endif

VERSION = $(shell cat VERSION.txt)
CPPFLAGS := $(CPPFLAGS) -DVERSION="\"$(VERSION)\""
LDFLAGS := $(LDFLAGS)

# Temporary build directories
ifdef CONFIG_W64
    BUILD := .win64
else
    BUILD := .nix
endif
 
# Define V=1 to show command line.
ifdef V
    Q :=
    E := @true
else
    Q := @
    E := @echo
endif
 
ifdef CONFIG_W64
    TARG := megata.exe
else
    TARG := megata
endif
 
all: $(TARG)
 
default: all
 
.PHONY: all default clean strip
 
COMMON_OBJS := \
        thirdparty/emu2149-1.16/emu2149.o \
        thirdparty/imgui-1.91.9/imgui.o \
        thirdparty/imgui-1.91.9/imgui_demo.o \
        thirdparty/imgui-1.91.9/imgui_draw.o \
        thirdparty/imgui-1.91.9/imgui_tables.o \
        thirdparty/imgui-1.91.9/imgui_widgets.o \
        thirdparty/miniz-3.0.2/miniz.o \
        thirdparty/rlImGui/rlImGui.o \
	src/CPU.o \
	src/Emulation.o \
	src/LCD.o \
	src/UI.o \
	src/main.o

ifdef CONFIG_W64
OBJS := \
	$(COMMON_OBJS) \
	thirdparty/nativefiledialog-extended-1.2.1/nfd_win.o \
	assets/megata.res
else
OBJS := \
	$(COMMON_OBJS) \
        thirdparty/nativefiledialog-extended-1.2.1/nfd_gtk.o
endif
 
# Rewrite paths to build directories
OBJS := $(patsubst %,$(BUILD)/%,$(OBJS))

$(TARG): $(OBJS)
	$(E) [LD] $@    
	$(Q)$(MKDIR) $(@D)
	$(Q)$(CXX) -o $@ $(OBJS) $(LDFLAGS)

clean:
	$(E) [CLEAN]
	$(Q)$(RM) $(TARG)
	$(Q)$(RMDIR) $(BUILD)

strip: $(TARG)
	$(E) [STRIP]
	$(Q)$(STRIP) $(TARG)

$(BUILD)/%.o: %.cpp
	$(E) [CXX] $@
	$(Q)$(MKDIR) $(@D)
	$(Q)$(CXX) -c $(CPPFLAGS) -o $@ $<

$(BUILD)/%.o: %.c
	$(E) [CC] $@
	$(Q)$(MKDIR) $(@D)
	$(Q)$(CC) -c $(CFLAGS) -o $@ $<

$(BUILD)/%.res: %.rc
	$(E) [RES] $@
	$(Q)$(MKDIR) $(@D)
	$(Q)$(WINDRES) $< -O coff -o $@ $(WINDRESARGS)
