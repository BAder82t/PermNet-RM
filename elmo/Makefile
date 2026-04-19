# ============================================================================
# Makefile -- build PermNet-RM and BIT0MASK ARM Thumb binaries for ELMO,
#             then run ELMO power simulation on both.
# ============================================================================

PREFIX     ?= arm-none-eabi
CC         := $(PREFIX)-gcc
OBJCOPY    := $(PREFIX)-objcopy
OBJDUMP    := $(PREFIX)-objdump

ARCH_FLAGS  = -mthumb -mcpu=cortex-m0 -msoft-float
GCC_INCDIR  = $(shell $(CC) -print-file-name=include)
CFLAGS      = -Os -g -ffreestanding -nostdinc -isystem $(GCC_INCDIR) \
              -fno-common -ffunction-sections -fdata-sections \
              -Wall -Wextra -Wmissing-prototypes -Wstrict-prototypes
LDFLAGS     = -nostdlib -Telmo.ld -Wl,--gc-sections
LDLIBS      = -lgcc

# Absolute paths so they survive the `cd output_{permnet,bitmask,masked}`
# inside the run-* rules. `$(abspath ...)` resolves without requiring the
# path to exist yet (unlike $(realpath ...)).
ELMO        = $(abspath ../elmo_tool/elmo)
# Paper Table 5 was produced with coeffs_M3.txt (mixed-sign values in the
# +/-800 range). The upstream elmo_tool also ships coeffs.txt (small
# positive floats 0.04-0.09); swapping to that file changes the numerical
# output of ELMO and is NOT what the paper used. Leaving coeffs_M3.txt
# here so reruns match the paper's Table 5.
ELMO_COEFFS = $(abspath ../elmo_tool/coeffs_M3.txt)
ELMO_DATA   = $(abspath ../elmo_tool/Examples/randdata100000.txt)

# Common objects
VECTOR_OBJ  = vector.o
ASMFUNC_OBJ = elmoasmfunctions.o

TARGETS = permnet bitmask masked_d1

# ============================================================================
all: $(addsuffix .bin,$(TARGETS))

# --- Assembly objects -------------------------------------------------------
$(VECTOR_OBJ): vector.s
	$(CC) $(ARCH_FLAGS) -c -o $@ $<

$(ASMFUNC_OBJ): elmoasmfunctions.s
	$(CC) $(ARCH_FLAGS) -c -o $@ $<

# --- PermNet-RM -------------------------------------------------------------
elmo_permnet.o: elmo_permnet.c elmoasmfunctionsdef.h
	$(CC) $(CFLAGS) $(ARCH_FLAGS) -c -o $@ $<

permnet.elf: elmo_permnet.o $(VECTOR_OBJ) $(ASMFUNC_OBJ) elmo.ld
	$(CC) $(LDFLAGS) $(ARCH_FLAGS) $(VECTOR_OBJ) elmo_permnet.o $(ASMFUNC_OBJ) $(LDLIBS) -o $@

# --- BIT0MASK ---------------------------------------------------------------
elmo_bitmask.o: elmo_bitmask.c elmoasmfunctionsdef.h
	$(CC) $(CFLAGS) $(ARCH_FLAGS) -c -o $@ $<

bitmask.elf: elmo_bitmask.o $(VECTOR_OBJ) $(ASMFUNC_OBJ) elmo.ld
	$(CC) $(LDFLAGS) $(ARCH_FLAGS) $(VECTOR_OBJ) elmo_bitmask.o $(ASMFUNC_OBJ) $(LDLIBS) -o $@

# --- Masked PermNet-RM, d = 1 -----------------------------------------------
elmo_masked_d1.o: elmo_masked_d1.c elmoasmfunctionsdef.h
	$(CC) $(CFLAGS) $(ARCH_FLAGS) -c -o $@ $<

masked_d1.elf: elmo_masked_d1.o $(VECTOR_OBJ) $(ASMFUNC_OBJ) elmo.ld
	$(CC) $(LDFLAGS) $(ARCH_FLAGS) $(VECTOR_OBJ) elmo_masked_d1.o $(ASMFUNC_OBJ) $(LDLIBS) -o $@

# --- Generic rules ----------------------------------------------------------
%.bin: %.elf
	$(OBJCOPY) -Obinary $< $@

%.list: %.elf
	$(OBJDUMP) -S $< > $@

# --- ELMO simulation --------------------------------------------------------
.PHONY: run run-permnet run-bitmask run-masked disasm clean

run: run-permnet run-bitmask run-masked

run-permnet: permnet.bin
	@echo "=== Running ELMO on PermNet-RM encoder ==="
	@mkdir -p output_permnet/traces
	cd output_permnet && cp $(ELMO_COEFFS) coeffs_M3.txt && \
	    mkdir -p output output/traces output/nonprofiledindexes output/asmoutput Examples && \
	    ln -sf $(ELMO_DATA) Examples/randdata100000.txt && \
	    $(ELMO) ../permnet.bin --Ntrace 256 2>&1 | tee elmo_log.txt
	@echo "=== PermNet-RM traces in output_permnet/ ==="

run-bitmask: bitmask.bin
	@echo "=== Running ELMO on BIT0MASK encoder ==="
	@mkdir -p output_bitmask/traces
	cd output_bitmask && cp $(ELMO_COEFFS) coeffs_M3.txt && \
	    mkdir -p output output/traces output/nonprofiledindexes output/asmoutput Examples && \
	    ln -sf $(ELMO_DATA) Examples/randdata100000.txt && \
	    $(ELMO) ../bitmask.bin --Ntrace 256 2>&1 | tee elmo_log.txt
	@echo "=== BIT0MASK traces in output_bitmask/ ==="

run-masked: masked_d1.bin
	@echo "=== Running ELMO on Boolean-masked (d=1) PermNet-RM encoder ==="
	@mkdir -p output_masked/traces
	cd output_masked && cp $(ELMO_COEFFS) coeffs_M3.txt && \
	    mkdir -p output output/traces output/nonprofiledindexes output/asmoutput Examples && \
	    ln -sf $(ELMO_DATA) Examples/randdata100000.txt && \
	    $(ELMO) ../masked_d1.bin --Ntrace 256 2>&1 | tee elmo_log.txt
	@echo "=== masked-d1 traces in output_masked/ ==="

disasm: permnet.list bitmask.list masked_d1.list
	@echo "=== Disassembly generated ==="

clean:
	rm -f *.o *.elf *.bin *.list *.map *.d
	rm -rf output_permnet output_bitmask output_masked
