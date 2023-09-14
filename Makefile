
CC := cc
LD := ld
AS := as

N := pprc
C := $(wildcard *.c)
S := $(wildcard *.s)
O := $(C:.c=.o) $(S:.s=.o)

CFLAGS := -std=c89 -ansi -pedantic
CFLAGS += -Wall -Wextra
CFLAGS += -Oz -fno-stack-protector
CFLAGS += -fomit-frame-pointer
CFLAGS += -ffunction-sections -fdata-sections
CFLAGS += -fno-unwind-tables -fno-asynchronous-unwind-tables
CFLAGS += -fmerge-all-constants
CFLAGS += -fno-ident

LDFLAGS := -m elf_x86_64
LDFLAGS += --gc-sections -z norelro --build-id=none

DEPS := c ncurses

RSC := .eh_frame .hash .gnu.version
# `strip -R.gnu.hash` isn't compatible with ncurses
#RSC := .gnu.hash

all: clean $(N)

$(N): $(O)
	$(LD) $(LDFLAGS) $(addprefix -l,$(DEPS)) $^ -o $@
	strip -S $(addprefix -R,$(RSC))  $@

%.o: %.c
	$(CC) $(CFLAGS) -c $^ -o $@

%.o: %.s
	$(AS) $^ -o $@

clean:
	rm -rf $(O) $(N)
