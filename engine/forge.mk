# FORGE – gemeinsames 3DS-Makefile. Im Spiel-Makefile: TARGET, APP_TITLE, APP_DESCRIPTION, APP_AUTHOR, ICON setzen,
# optional GAME_SOURCES (Standard: source), GRAPHICS (gfx), ROMFS, EXTRA_LIBS, FORGE_MODULES (physics2d); danach: include <pfad>/engine/forge.mk
.SUFFIXES:
ifeq ($(strip $(DEVKITARM)),)
$(error "DEVKITARM fehlt (devkitPro). Am einfachsten ../../build.sh nutzen")
endif
FORGE := $(abspath $(dir $(lastword $(MAKEFILE_LIST))))
TOPDIR ?= $(CURDIR)
include $(DEVKITARM)/3ds_rules

BUILD        := build
GAME_SOURCES ?= source
GRAPHICS     ?= gfx
ENGINE_SRC   := $(FORGE)/src $(FORGE)/platform/3ds $(foreach m,$(FORGE_MODULES),$(FORGE)/modules/$(m))

ARCH     := -march=armv6k -mtune=mpcore -mfloat-abi=hard -mtp=soft
CFLAGS   := -g -Wall -O2 -mword-relocations -ffunction-sections $(ARCH)
CFLAGS   += $(INCLUDE) -D__3DS__
CXXFLAGS := $(CFLAGS) -fno-rtti -fno-exceptions -std=gnu++17
ASFLAGS  := -g $(ARCH)
LDFLAGS   = -specs=3dsx.specs -g $(ARCH) -Wl,-Map,$(notdir $*.map)
LIBS     := $(EXTRA_LIBS) $(if $(filter physics2d,$(FORGE_MODULES)),-lbox2d) -lcitro2d -lcitro3d -lctru -lm
LIBDIRS  := $(PORTLIBS) $(CTRULIB)

ifneq ($(BUILD),$(notdir $(CURDIR)))
export OUTPUT   := $(CURDIR)/$(TARGET)
export TOPDIR   := $(CURDIR)
export VPATH    := $(foreach d,$(GAME_SOURCES),$(CURDIR)/$(d)) $(ENGINE_SRC) $(foreach d,$(GRAPHICS),$(CURDIR)/$(d))
export DEPSDIR  := $(CURDIR)/$(BUILD)
CFILES   := $(foreach d,$(GAME_SOURCES),$(notdir $(wildcard $(d)/*.c))) $(foreach d,$(ENGINE_SRC),$(notdir $(wildcard $(d)/*.c)))
CPPFILES := $(foreach d,$(GAME_SOURCES),$(notdir $(wildcard $(d)/*.cpp))) $(foreach d,$(ENGINE_SRC),$(notdir $(wildcard $(d)/*.cpp)))
GFXFILES := $(foreach d,$(GRAPHICS),$(notdir $(wildcard $(d)/*.t3s)))
export LD             := $(CXX)
export T3XFILES       := $(GFXFILES:.t3s=.t3x)
export OFILES_SOURCES := $(CPPFILES:.cpp=.o) $(CFILES:.c=.o)
export OFILES         := $(addsuffix .o,$(T3XFILES)) $(OFILES_SOURCES)
export HFILES         := $(addsuffix .h,$(subst .,_,$(T3XFILES)))
export INCLUDE        := $(foreach d,$(GAME_SOURCES),-I$(CURDIR)/$(d)) -I$(FORGE)/include $(foreach d,$(LIBDIRS),-I$(d)/include) -I$(CURDIR)/$(BUILD)
export LIBPATHS       := $(foreach d,$(LIBDIRS),-L$(d)/lib)
export APP_ICON       := $(TOPDIR)/$(ICON)
export _3DSXFLAGS     += --smdh=$(CURDIR)/$(TARGET).smdh
ifneq ($(strip $(ROMFS)),)
export _3DSXFLAGS     += --romfs=$(CURDIR)/$(ROMFS)
endif

.PHONY: $(BUILD) clean all
all: $(BUILD)
$(BUILD):
	@[ -d $@ ] || mkdir -p $@
	@$(MAKE) --no-print-directory -C $(BUILD) -f $(CURDIR)/Makefile
clean:
	@rm -fr $(BUILD) $(TARGET).3dsx $(TARGET).smdh $(TARGET).elf
else
DEPENDS := $(OFILES:.o=.d)
$(OUTPUT).3dsx: $(OUTPUT).elf $(OUTPUT).smdh
$(OUTPUT).elf: $(OFILES)
$(OFILES_SOURCES): $(HFILES)
.PRECIOUS: %.t3x
%.t3x.o %_t3x.h: %.t3x
	@echo $(notdir $<)
	@$(bin2o)
%.t3x: %.t3s
	@echo $(notdir $<)
	@tex3ds -i $< -o $@
-include $(DEPENDS)
endif
