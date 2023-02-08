################################################################################
# BUILD OPTIONS
################################################################################

# Memory model of the target device ('medium' or 'large'). Devices with >32 KB
# of flash should typically use 'large'.
MODEL ?= medium

################################################################################

CC = sdcc
CFLAGS = -mstm8
ifeq ($(MODEL),large)
	CFLAGS += --model-large
	LIBSUFFIX = -large
endif

AR = sdar
AFLAGS = -c

ifeq ($(OS),Windows_NT)
	RM = cmd.exe /C del /Q
	MKDIR = mkdir
else
	RM = rm -fr
	MKDIR = mkdir -p
endif

LIBHEAD = lin_checksum.h
LIBSRC = lin_checksum.c

TESTHEAD = ucsim.h lin_checksum.h
TESTSRC = ucsim.c main.c

OBJDIR = obj
LIBOBJ = $(patsubst %.c,$(OBJDIR)/%.rel,$(LIBSRC))
TESTOBJ = $(patsubst %.c,$(OBJDIR)/%.rel,$(TESTSRC))

LIBDIR = lib
LIBRARY = $(LIBDIR)/stm8-lin-checksum$(LIBSUFFIX).lib

BINDIR = bin
BINARY = $(BINDIR)/test.ihx

.PHONY: library test all clean sim

all: test
test: $(BINARY)
library: $(LIBRARY)

$(LIBRARY): $(LIBOBJ) $(LIBDIR)
	$(AR) $(AFLAGS) -r $@ $(LIBOBJ)

$(BINARY): $(LIBRARY) $(TESTOBJ) $(BINDIR)
	$(CC) $(CFLAGS) --out-fmt-ihx -o $@ -l $(LIBRARY) $(TESTOBJ)

$(LIBOBJ): $(OBJDIR) $(LIBHEAD) $(LIBSRC)

$(TESTOBJ): $(OBJDIR) $(TESTHEAD) $(TESTSRC)

$(OBJDIR)/%.rel: %.c
	$(CC) $(CFLAGS) -o $@ -c $<

$(OBJDIR) $(LIBDIR) $(BINDIR):
	$(MKDIR) $@

clean:
	$(RM) $(OBJDIR)
	$(RM) $(LIBDIR)
	$(RM) $(BINDIR)

sim:
	ucsim_stm8 -G -t STM8S208 -X 16M -I if=rom[0x5800] $(BINARY)
