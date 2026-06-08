# Role: MASTER or SLAVE (default: MASTER)
ROLE ?= MASTER
BUILD_DIR = build/$(ROLE)
TARGET    = $(BUILD_DIR)/hoverboard-$(ROLE)

CC      = arm-none-eabi-gcc
OBJCOPY = arm-none-eabi-objcopy
SIZE    = arm-none-eabi-size

# CMSIS external path — set via environment in CI or on command line locally
CMSIS_PATH ?= $(error CMSIS_PATH is not set. Example: make CMSIS_PATH=~/cmsis ROLE=MASTER)

# Compiler flags
CFLAGS  = -mcpu=cortex-m3 -mthumb -mfloat-abi=soft
CFLAGS += -Os -std=c99 -Wall -Wextra -Wno-unused-parameter
CFLAGS += -ffunction-sections -fdata-sections
CFLAGS += -D$(ROLE) -DARM_MATH_CM3 -DUSE_STDPERIPH_DRIVER

# Include paths
INCLUDES  = -IHoverBoardGigaDevice/Inc
INCLUDES += -IHoverBoardGigaDevice/RTE/Device/GD32F130C8
INCLUDES += -IHoverBoardGigaDevice/RTE/_Target_1
INCLUDES += -IHoverBoardGigaDevice/Drivers/Device/Include
INCLUDES += -IHoverBoardGigaDevice/Drivers/Peripherals/inc
INCLUDES += -I$(CMSIS_PATH)/CMSIS/Core/Include
INCLUDES += -I$(CMSIS_PATH)/CMSIS/DSP/Include

# Source files
C_SRCS   = $(wildcard HoverBoardGigaDevice/Src/*.c)
C_SRCS  += $(wildcard HoverBoardGigaDevice/RTE/Device/GD32F130C8/*.c)
ASM_SRCS = HoverBoardGigaDevice/RTE/Device/GD32F130C8/startup_gd32f1x0.s

C_OBJS   = $(patsubst %.c,$(BUILD_DIR)/%.o,$(C_SRCS))
ASM_OBJS = $(patsubst %.s,$(BUILD_DIR)/%.o,$(ASM_SRCS))
OBJS     = $(C_OBJS) $(ASM_OBJS)

# Linker flags
LDFLAGS  = -mcpu=cortex-m3 -mthumb -mfloat-abi=soft
LDFLAGS += -THoverBoardGigaDevice/Linker/gd32f130c8.ld
LDFLAGS += -Wl,--gc-sections
LDFLAGS += -Wl,-Map=$(TARGET).map
LDFLAGS += -specs=nosys.specs -lnosys

.PHONY: all clean

all: $(TARGET).elf $(TARGET).bin
	@$(SIZE) $(TARGET).elf

$(TARGET).elf: $(OBJS)
	$(CC) $(LDFLAGS) $^ -o $@

$(TARGET).bin: $(TARGET).elf
	$(OBJCOPY) -O binary $< $@

$(BUILD_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(BUILD_DIR)/%.o: %.s
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

clean:
	rm -rf build/
