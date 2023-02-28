####################################################################
# Makefile
#
# OS variable must either be 'posix' or 'win'. E.g. 'make OS=posix'.
# Error is thrown if OS variable is not equal with any of these.
#
####################################################################

.SUFFIXES:				# ignore builtin rules
.PHONY: all debug release clean

####################################################################
# Definitions                                                      #
####################################################################

# uniq is a function which removes duplicate elements from a list
uniq = $(strip $(if $1,$(firstword $1) \
       $(call uniq,$(filter-out $(firstword $1),$1))))

PROJECTNAME = locator-host

OBJ_DIR = build
EXE_DIR = exe
LST_DIR = lst


####################################################################
# Definitions of toolchain.                                        #
# You might need to do changes to match your system setup          #
####################################################################

RMDIRS     := rm -rf
RMFILES    := rm -rf
ALLFILES   := /*.*
NULLDEVICE := /dev/null
SHELLNAMES := $(ComSpec)$(COMSPEC)

# Try autodetecting the environment: Windows
ifneq ($(SHELLNAMES),)
  QUOTE :="
  ifeq (,$(filter $(OS),posix win))
    OS:=win
  endif
  ifneq ($(COMSPEC),)
    ifeq ($(findstring cygdrive,$(shell set)),)
      # We were not on a cygwin platform
      NULLDEVICE := NUL
    endif
  else
    # Assume we are making on a Windows platform
    # This is a convenient place to override TOOLDIR, DO NOT add trailing
    # whitespace chars, they do matter !
    SHELL      := $(SHELLNAMES)
    RMDIRS     := rd /s /q
    RMFILES    := del /s /q
    ALLFILES   := \*.*
    NULLDEVICE := NUL
  endif
# Other than Windows
else
  ifeq (,$(filter $(OS),posix win))
    OS:=posix
  endif
endif

ifeq ($(CROSSCOMPILE), raspbian)
  OS:=posix
endif

# Create directories and do a clean which is compatible with parallell make
$(shell mkdir $(OBJ_DIR)>$(NULLDEVICE) 2>&1)
$(shell mkdir $(EXE_DIR)>$(NULLDEVICE) 2>&1)
$(shell mkdir $(LST_DIR)>$(NULLDEVICE) 2>&1)
ifeq (clean,$(findstring clean, $(MAKECMDGOALS)))
  ifneq ($(filter $(MAKECMDGOALS),all debug release),)
    $(shell $(RMFILES) $(OBJ_DIR)$(ALLFILES)>$(NULLDEVICE) 2>&1)
    $(shell $(RMFILES) $(EXE_DIR)$(ALLFILES)>$(NULLDEVICE) 2>&1)
    $(shell $(RMFILES) $(LST_DIR)$(ALLFILES)>$(NULLDEVICE) 2>&1)
  endif
endif

CC = g++
LD = ld
AR = ar


####################################################################
# Flags                                                            #
####################################################################

INCLUDEPATHS += \
-I./src \
-I./inc \
-I./inc/rtl \
-I./inc/bgapi \
-I../libdoa

override CFLAGS += -fpermissive -std=c++17 -lutil -Wall

# -MMD : Don't generate dependencies on system header files.
# -MP  : Add phony targets, useful when a h-file is removed from a project.
# -MF  : Specify a file to write the dependencies to.
DEPFLAGS = \
-MMD \
-MP \
-MF $(@:.o=.d)

override ASMFLAGS += \
-std=c99

# Add -Wa,-ahld=$(LST_DIR)/$(@F:.o=.lst) to CFLAGS to produce assembly list files
# override CFLAGS += \
# -fno-short-enums \
# -Wall \
# -c \
# -fmessage-length=0 \
# -std=c99 \
# $(DEPFLAGS)

# Linux platform: if _DEFAULT_SOURCE is defined, the default is to have _POSIX_SOURCE set to one
# and _POSIX_C_SOURCE set to 200809L, as well as enabling miscellaneous functions from BSD and SVID.
# See usr/include/fetures.h for more information.
#
# _BSD_SOURCE (deprecated since glibc 2.20)
# Defining this macro with any value causes header files to expose BSD-derived definitions.
# In glibc versions up to and including 2.18, defining this macro also causes BSD definitions to be
# preferred in some situations where standards conflict, unless one or more of _SVID_SOURCE,
# _POSIX_SOURCE, _POSIX_C_SOURCE, _XOPEN_SOURCE, _XOPEN_SOURCE_EXTENDED, or _GNU_SOURCE is defined,
# in which case BSD definitions are disfavored. Since glibc 2.19, _BSD_SOURCE no longer causes BSD
# definitions to be preferred in case of conflicts. Since glibc 2.20, this macro is deprecated.
# It now has the same effect as defining _DEFAULT_SOURCE, but generates a compile-time warning
# (unless _DEFAULT_SOURCE is also defined). Use _DEFAULT_SOURCE instead.
# To allow code that requires _BSD_SOURCE in glibc 2.19 and earlier and _DEFAULT_SOURCE in glibc
# 2.20 and later to compile without warnings, define both _BSD_SOURCE and _DEFAULT_SOURCE.
#
# OSX platform: _DEFAULT_SOURCE is not used, instead _DARWIN_C_SOURCE is defined by default.
ifeq ($(OS),posix)
override CFLAGS += \
-D_DEFAULT_SOURCE \
-D_BSD_SOURCE \
-lpthread
else
ifeq ($(OS),win)
override CFLAGS += \
-DWINDOWS
endif
endif

# NOTE: The -Wl,--gc-sections flag may interfere with debugging using gdb.
ifeq ($(OS),posix)
  override LDFLAGS += -Llib
  override LDFLAGS += -lpthread

  ifeq ($(CROSSCOMPILE), raspbian)
    CC = arm-linux-gnueabihf-gcc
    LD = arm-linux-gnueabihf-ld
    AR = arm-linux-gnueabihf-ar
    override LDFLAGS += -laox_static_raspbian
    TARGET:=RASPBIAN
  else
    UNAME_S := $(shell uname -s)
    UNAME_M := $(shell uname -m)
    ifeq ($(UNAME_S), Linux)
      ifeq ($(UNAME_M), x86_64)
        override LDFLAGS += -laox_static_ubuntu
        TARGET:=UBUNTU
      else
        ifneq ($(filter arm%, $(UNAME_M)),)
          override LDFLAGS += -laox_static_raspbian
          TARGET:=RASPBIAN
        # else: can't figure out the OS, or OS not supported
        endif
      endif
    else
      ifeq ($(UNAME_S), Darwin)
        override LDFLAGS += -laox_static_macos
        TARGET:=MACOS
      # else: can't figure out the OS, or OS not supported
      endif
    endif
  endif

  override LDFLAGS += -lstdc++
  override LDFLAGS += -lm
else
  ifeq ($(ARCH),64)
    override LDFLAGS += -static lib/libaox_static_win64.a -lstdc++
    TARGET:=WIN64
  else
    override LDFLAGS += -static lib/libaox_static_win32.a -lstdc++
    TARGET:=WIN32
  endif
endif
####################################################################
# Files                                                            #
####################################################################

C_SRC +=  \
src/serial.cpp \
src/gecko_bglib.cpp \
src/main.cpp \
src/bg.cpp \
src/aox.cpp \
../libdoa/doa.cpp

# this file should be the last added
C_SRC += src/uart_posix.cpp

s_SRC +=

S_SRC +=

LIBS =


####################################################################
# Rules                                                            #
####################################################################

C_FILES = $(notdir $(C_SRC) )
S_FILES = $(notdir $(S_SRC) $(s_SRC) )
#make list of source paths, uniq removes duplicate paths
C_PATHS = $(call uniq, $(dir $(C_SRC) ) )
S_PATHS = $(call uniq, $(dir $(S_SRC) $(s_SRC) ) )

C_OBJS = $(addprefix $(OBJ_DIR)/, $(C_FILES:.cpp=.o))
S_OBJS = $(if $(S_SRC), $(addprefix $(OBJ_DIR)/, $(S_FILES:.S=.o)))
s_OBJS = $(if $(s_SRC), $(addprefix $(OBJ_DIR)/, $(S_FILES:.s=.o)))
C_DEPS = $(addprefix $(OBJ_DIR)/, $(C_FILES:.cpp=.d))
OBJS = $(C_OBJS) $(S_OBJS) $(s_OBJS)

vpath %.cpp $(C_PATHS)
vpath %.s $(S_PATHS)
vpath %.S $(S_PATHS)

# Default build is debug build
all:      debug

debug:    CFLAGS += -O0 -g3
debug:    $(EXE_DIR)/$(PROJECTNAME)

release:  $(EXE_DIR)/$(PROJECTNAME)


# Create objects from C SRC files
$(OBJ_DIR)/%.o: %.cpp
	@echo -e "\033[1;32m[Building $@]\033[0m"
	$(CC) $(CFLAGS) $(INCLUDEPATHS) -c -o $@ $<
	@echo -e  "\033[1;32m[Finished $@]\033[0m"

# Assemble .s/.S files
$(OBJ_DIR)/%.o: %.s
	@echo -e "\033[1;32m[Assembling $@]\033[0m"
	$(CC) $(ASMFLAGS) $(INCLUDEPATHS) -c-o $@ $<
	@echo -e  "\033[1;32m[Finished $@]\033[0m"

$(OBJ_DIR)/%.o: %.S
	@echo -e "\033[1;32m[Assembling $@]\033[0m"
	$(CC) $(ASMFLAGS) $(INCLUDEPATHS) -c -o $@ $<
	@echo -e  "\033[1;32m[Finished $@]\033[0m"

# Link
$(EXE_DIR)/$(PROJECTNAME): $(OBJS) $(LIBS)
	@echo -e "\033[1;32m[Linking $@]\033[0m"
	$(CC) $^ $(LDFLAGS) -o $@
	@echo -e  "\033[1;32m[Finished $@]\033[0m"

# Copy the libraries
lib_copy:
	@echo -e "\033[1;32m[Copy libs]\033[0m"
ifeq ($(OS),posix)
	cp libs/rtl_lib/libaox_shared.so exe/libaox_shared.so
else
ifeq ($(ARCH),64)
	cp .\lib\libaox_shared_win64.dll exe\libaox_shared.dll
else
	cp .\lib\libaox_shared_win32.dll exe\libaox_shared.dll
endif
endif

	@echo -e  "\033[1;32m[Finished $@]\033[0m"

PHONY := clean
clean:
ifeq ($(filter $(MAKECMDGOALS),all debug release),)
	$(RMDIRS) $(OBJ_DIR) $(LST_DIR) $(EXE_DIR)
endif

# include auto-generated dependency files (explicit rules)
ifneq (clean,$(findstring clean, $(MAKECMDGOALS)))
-include $(C_DEPS)
endif

#Black        0;30     Dark Gray     1;30
#Red          0;31     Light Red     1;31
#Green        0;32     Light Green   1;32
#Brown/Orange 0;33     Yellow        1;33
#Blue         0;34     Light Blue    1;34
#Purple       0;35     Light Purple  1;35
#Cyan         0;36     Light Cyan    1;36
#Light Gray   0;37     White         1;37
