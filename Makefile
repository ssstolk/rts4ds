#---------------------------------------------------------------------------------
.SUFFIXES:
#---------------------------------------------------------------------------------
ifndef GAME_TITLE
GAME_TITLE		:=	RTS4DS
GAME_SUBTITLE1	:=	by 
GAME_SUBTITLE2	:=	Sander Stolk
GAME_ICON		:=	$(CURDIR)/../logo.bmp
endif

ifeq ($(strip $(DEVKITARM)),)
$(error "Please set DEVKITARM in your environment. export DEVKITARM=<path to>devkitARM")
endif

include $(DEVKITARM)/ds_rules

#---------------------------------------------------------------------------------
# TARGET is the name of the output
# BUILD is the directory where object files & intermediate files will be placed
# SOURCES is a list of directories containing source code
# INCLUDES is a list of directories containing extra header files
#---------------------------------------------------------------------------------
TARGET		:=	$(shell basename $(CURDIR))
BUILD		:=	build
SOURCES		:=	gfx source data source/astar source/gif
INCLUDES	:=	include build
NITRODATA	:=	fs

#---------------------------------------------------------------------------------
# options for code generation
#---------------------------------------------------------------------------------
ARCH	:=	-mthumb -mthumb-interwork

CFLAGS	:=	-g -Wall -O2\
 			-march=armv5te -mtune=arm946e-s \
            -fomit-frame-pointer \
			-ffast-math \
			$(ARCH)
# note: add -fstack-protector-all to CFLAGS to enable stack overflow detection

CFLAGS	+=	$(INCLUDE) -DARM9 $(TARGET_CFLAGS)
CXXFLAGS	:= $(CFLAGS) -fno-rtti -fno-exceptions

ASFLAGS	:=	-g $(ARCH) -march=armv5te -mtune=arm946e-s $(INCLUDE)
LDFLAGS	=	-specs=ds_arm9.specs -g $(ARCH) -Wl,-Map,$(notdir $*.map)

#---------------------------------------------------------------------------------
# any extra libraries we wish to link with the project
#---------------------------------------------------------------------------------
LIBS	:= -lfilesystem -lfat -lmm9 -lnds9
 
#---------------------------------------------------------------------------------
# list of directories containing libraries, this must be the top level containing
# include and lib
#---------------------------------------------------------------------------------
LIBDIRS	:=	$(LIBNDS)
 
#---------------------------------------------------------------------------------
# no real need to edit anything past this point unless you need to add additional
# rules for different file extensions
#---------------------------------------------------------------------------------
ifneq ($(BUILD),$(notdir $(CURDIR)))
#---------------------------------------------------------------------------------
 
export OUTPUT	:=	$(CURDIR)/$(TARGET)
 
export VPATH	:=	$(foreach dir,$(SOURCES),$(CURDIR)/$(dir))
export DEPSDIR	:=	$(CURDIR)/$(BUILD)

ifneq ($(strip $(NITRODATA)),)
	ifdef NITRODATA_SUBFOLDER
		export NITRO_FILES	:=	$(CURDIR)/$(NITRODATA)/$(NITRODATA_SUBFOLDER)
	else
		export NITRO_FILES	:=	$(CURDIR)/$(NITRODATA)/rts4ds
	endif
endif

CFILES		:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.c)))
CPPFILES	:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.cpp)))
SFILES		:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.s)))
BINFILES	:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.bin)))
 
#---------------------------------------------------------------------------------
# use CXX for linking C++ projects, CC for standard C
#---------------------------------------------------------------------------------
ifeq ($(strip $(CPPFILES)),)
#---------------------------------------------------------------------------------
	export LD	:=	$(CC)
#---------------------------------------------------------------------------------
else
#---------------------------------------------------------------------------------
	export LD	:=	$(CXX)
#---------------------------------------------------------------------------------
endif
#---------------------------------------------------------------------------------

export OFILES	:=	$(BINFILES:.bin=.o) \
					$(CPPFILES:.cpp=.o) $(CFILES:.c=.o) $(SFILES:.s=.o)
 
export INCLUDE	:=	$(foreach dir,$(INCLUDES),-I$(CURDIR)/$(dir)) \
					$(foreach dir,$(LIBDIRS),-I$(dir)/include) \
					-I$(CURDIR)/$(BUILD)
 
export LIBPATHS	:=	$(foreach dir,$(LIBDIRS),-L$(dir)/lib)
 
.PHONY: $(BUILD) uw_demo1 uw_demo2 uw_demoX debug clean
 
#---------------------------------------------------------------------------------
$(BUILD):
	@[ -d $@ ] || mkdir -p $@
	@$(MAKE) --no-print-directory -C $(BUILD) -f $(CURDIR)/Makefile
 
#---------------------------------------------------------------------------------
uw_demo1:
	make uw_demoX DEMO_NUMBER=1

uw_demo2:
	make uw_demoX DEMO_NUMBER=2

uw_demoX:
	make TARGET_CFLAGS='-DDEFAULT_LOCATION_NDS=\"fat:/uw_demo$(DEMO_NUMBER).nds\" -DFS_ROOT_FAT=\"fat:/uw_demo$(DEMO_NUMBER)/\"' \
			GAME_TITLE="Ulterior Warzone (demo $(DEMO_NUMBER))" \
			GAME_SUBTITLE1="by Sander Stolk and" \
			GAME_SUBTITLE2="Violation Entertainment" \
			GAME_ICON=$(CURDIR)/logo_uw.bmp \
			NITRODATA_SUBFOLDER=uw_demo$(DEMO_NUMBER)
	mv $(TARGET).elf uw_demo$(DEMO_NUMBER).elf
	mv $(TARGET).nds uw_demo$(DEMO_NUMBER).nds
#	mv $(TARGET).nds.ezflash5 uw_demo$(DEMO_NUMBER).nds.ezflash5
#	@rm -fr $(TARGET).elf $(TARGET).nds $(TARGET).nds.ezflash5 $(TARGET).ds.gba

debug:
	make TARGET_CFLAGS=-DDEBUG_BUILD

clean:
	@echo clean ...
	@rm -fr $(BUILD) *.elf *.nds *.ezflash5 *.ds.gba 

 
#---------------------------------------------------------------------------------
else
 
DEPENDS	:=	$(OFILES:.o=.d)

#---------------------------------------------------------------------------------
# overriding default %.nds rule in order to also generate a homebrew binary compatible with ezflash5
#---------------------------------------------------------------------------------
#%.nds	: %.elf
#	@ndstool -c $@ -9 $< -b $(GAME_ICON) "$(GAME_TITLE);$(GAME_SUBTITLE1);$(GAME_SUBTITLE2)" $(_ADDFILES)
#	@echo built ... $(notdir $@)
#	@ndstool -c $@.ezflash5 -9 $< -b $(GAME_ICON) "$(GAME_TITLE);$(GAME_SUBTITLE1);$(GAME_SUBTITLE2)" -g "PASS" "00" "RTS4DS" $(_ADDFILES)
#	@echo built ... $(notdir $@).ezflash5
#	@dlditool $(CURDIR)/../dldi/EZ5V2.dldi $@.ezflash5

#---------------------------------------------------------------------------------
# main targets
#---------------------------------------------------------------------------------
$(OUTPUT).nds	: 	$(OUTPUT).elf
$(OUTPUT).elf	:	$(OFILES)

#---------------------------------------------------------------------------------
%.o	:	%.bin
#---------------------------------------------------------------------------------
	@echo $(notdir $<)
	$(bin2o)
 
 
-include $(DEPENDS)
 
#---------------------------------------------------------------------------------------
endif
#---------------------------------------------------------------------------------------
