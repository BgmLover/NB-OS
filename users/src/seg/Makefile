# specifications
export PROJECT := seg
export TARGET := $(PROJECT).bin
export MAP := $(PROJECT).map
export ELF := $(PROJECT).elf
export DISASSEMBLY := $(PROJECT).txt
export OBJS := $(PROJECT).o
export CLEAN_LIST := $(TARGET) $(MAP) $(ELF) $(DISASSEMBLY) $(OBJS)

# code entrance
export ENTRANCE := seg_start

# config file path
CONFIG_PATH := ../../config

# include tools and flags
include $(CONFIG_PATH)/tools.conf

# compile flags
ASFLAG := -mips32 -EL
LDFLAG := -EL -T $(CONFIG_PATH)/kernel.ld -Ttext 0 -e $(ENTRANCE)
OCFLAG := -S -O binary

# default target
default: all

# targets
.PHONY: all
all: objcopy

.PHONY: objcopy
objcopy: $(OBJS)
	@echo -e "\t" LD  $(OBJS) -Map $(MAP) -o $(ELF)
	@$(LD) $(LDFLAG)  $(OBJS) -Map $(MAP) -o $(ELF)
	@echo -e "\t" OC $(ELF) $(TARGET)
	@$(OC) $(OCFLAG) $(ELF) $(TARGET)

.PHONY: disassembly
disassembly: objcopy
	$(OD) -S $(ELF) > $(DISASSEMBLY)

.PHONY: clean
clean:
	@echo CLEAN $(CLEAN_LIST)
	@rm -f $(CLEAN_LIST)

# dependencies
%.o: %.s
	@echo -e "\t" AS $< "\t" -o $@
	@$(AS) $(ASFLAG) $< -o $@