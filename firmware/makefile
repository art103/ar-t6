# rudimentary (linux) makefile for ar-t6
# use:
# make clean && make
#
# produces ar-t6.bin

SRCS+=$(wildcard peripherals/src/*.c)
SRCS+=$(wildcard system/*.c)
SSRC+=$(wildcard system/*.S)
SRCS+=$(wildcard *.c)

OBJS+=$(SSRC:.S=.o)
OBJS+=$(SRCS:.c=.o)
DEPS=$(SRCS:.c=.d)
DEPS+=$(SSRC:.S=.d)

PROJ=ar-t6

INCLUDES=-I"peripherals/inc" -I"system"
WARNSUPR=-w -Wno-packed-bitfield-compat
CFLAGS=-O0 -std=c99 $(WARNSUPR) -DSTM32F10X_MD_VL=1 -DHSE_VALUE=12000000 -mcpu=cortex-m3 -mthumb -ffunction-sections -fdata-sections -fomit-frame-pointer -fconserve-stack -fstrict-overflow  -Wl,--gc-sections -g3 -Wall -c -fmessage-length=0 -Wstack-usage=128 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)"
LFLAGS=-static -mcpu=cortex-m3 -mthumb -Xlinker --gc-sections -Xlinker -Map=$(PROJ).map

RM := rm -rf

all: $(PROJ).bin

%.o: %.S
	@echo '$<'
	@arm-none-eabi-as -g3 -o "$@" "$<"
	
%.o: %.c
	@echo '$<'
	@arm-none-eabi-gcc $(CFLAGS) $(INCLUDES) -o "$@" "$<"

$(PROJ).elf: $(OBJS)
	@echo ' '
	@echo 'Linking $@'
	@arm-none-eabi-gcc $(LFLAGS) -T system/stm32_flash.ld -o $(PROJ).elf $(OBJS)
	-@arm-none-eabi-size $(PROJ).elf

$(PROJ).bin: $(PROJ).elf
	@echo ' '
	@echo 'Producing binary $@'
	@arm-none-eabi-objcopy -O binary $(PROJ).elf $(PROJ).bin
	@echo ' '

clean:
	-$(RM) $(OBJS) $(DEPS) $(PROJ).bin $(PROJ).elf $(PROJ).map
	-@echo "Cleaned up"

.PHONY: all clean
