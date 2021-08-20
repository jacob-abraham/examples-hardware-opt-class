CC= clang
AS= nasm
LD= clang

override CFLAGS+= -Wall -Wextra -g -O3
override CFLAGS+= -masm=intel
override CFLAGS+= 

override ASFLAGS+= -felf64
LDFLAGS+= 
LDLIBS+= 


SOURCES= $(wildcard *.c)
ASM_SOURCES= $(wildcard *.asm)
OBJECTS= $(patsubst %.c,%.o,$(SOURCES))
OBJECTS+= $(patsubst %.asm,%.o,$(ASM_SOURCES))
INCLUDE= -I../../timing
DEPENFLAGS+= -MMD -MP
DEPENDS= $(patsubst %.c,%.d,$(SOURCES))

TARGET= bitset.out

.PHONY: all
all: $(TARGET)

.PHONY: clean
clean:
	$(RM) $(TARGET) $(OBJECTS) $(DEPENDS) *.out

# -lm required to be after the file for gcc, which is annoying
$(TARGET): $(OBJECTS)
	$(LD) $(LDFLAGS) $(LDLIBS) $^ -o $@

%.o: %.c Makefile
	$(CC) $(CFLAGS) $(DEPENFLAGS) $(INCLUDE) -c $< -o $@

%.o: %.asm Makefile
	$(AS) $(ASFLAGS) $< -o $@

-include $(DEPENDS)
