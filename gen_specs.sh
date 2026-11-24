#!/bin/sh
# Copyright (C) 2026 Ivan Gaydardzhiev
# Licensed under the GPL-3.0-only
#
# gen_specs.sh LIBDIR INCDIR SPECSFILE [COMPILER_INCLUDE_DIR]
# Use: gcc -specs=LIBDIR/a64libc-gcc.specs -static -no-pie prog.c -o prog
# -static and -no-pie must be given: a spec cannot switch off a distro's default PIE.
# gcc crashes on an empty spec, so %{ZZZ:} (a condition that is never true) stands for "nothing".
LIBDIR="$1"
INCDIR="$2"
SPECSFILE="$3"
GCCINC="$4"

cat > "$SPECSFILE" << SPECSEOF
*cc1:
+ -nostdinc -I $INCDIR ${GCCINC:+-isystem $GCCINC} -ffreestanding -fno-builtin -fno-stack-protector -fno-asynchronous-unwind-tables -fno-pie %{!O*:-Os}

*startfile:
$LIBDIR/crt0.o

*endfile:
%{ZZZ:}

*lib:
$LIBDIR/a64libc.a

*libgcc:
%{ZZZ:}

*link:
+ -T $LIBDIR/a64libc.ld -z max-page-size=0x10000
SPECSEOF
