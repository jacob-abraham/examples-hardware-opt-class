CC= clang
AS= nasm
LD= clang

override CFLAGS+= -Wall -Wextra -g
override CFLAGS+= -masm=intel

LDFLAGS+= 

ASFLAGS+= -felf64

SOURCES= $(wildcard *.c)
ASM_SOURCES= $(wildcard *.asm)
OBJECTS= $(patsubst %.c,%.o,$(SOURCES))
OBJECTS+= $(patsubst %.asm,%_a.o,$(ASM_SOURCES))
INCLUDE= -I../../timing
DEPENFLAGS+= -MMD -MP
DEPENDS= $(patsubst %.c,%.d,$(SOURCES))

TARGET= ll.out

.PHONY: all
all: $(TARGET)



.PHONY: clean
clean:
	$(RM) $(TARGET) $(OBJECTS) $(DEPENDS)

$(TARGET): $(OBJECTS)
	$(LD) $(LDFLAGS) $^ -o $@

%.s: %.c Makefile
	$(CC) $(CFLAGS) $(CONTROL_GUARDS) $(DEPENFLAGS) $(INCLUDE) -S $< -o $@

%c.o: %.c Makefile
	$(CC) $(CFLAGS) $(CONTROL_GUARDS) $(DEPENFLAGS) $(INCLUDE) -c $< -o $@

%_a.o: %.asm Makefile
	$(AS) $(ASFLAGS) $< -o $@

-include $(DEPENDS)
