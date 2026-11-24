ENTRY(_start)

PHDRS {
	text PT_LOAD FILEHDR PHDRS FLAGS(5);	/* R X */
	data PT_LOAD FLAGS(6);			/* R W */
	stack PT_GNU_STACK FLAGS(6);		/* non executable stack */
}

SECTIONS {
	. = 0x400000 + SIZEOF_HEADERS;
	.text : { *(.text .text.*) } :text
	.rodata ALIGN(8) : { *(.rodata .rodata.*) } :text
	. = ALIGN(CONSTANT(MAXPAGESIZE)) + (. & (CONSTANT(MAXPAGESIZE) - 1));
	.data ALIGN(8) : { *(.data .data.*) } :data
	. = ALIGN(8);
	_bss_start = .;
	.bss : { *(.bss .bss.*) *(COMMON) } :data
	. = ALIGN(16);
	_end = .;
	/DISCARD/ : { *(.comment) *(.note .note.*) *(.eh_frame .eh_frame_hdr) }
}
