ENTRY(start32)

mb_magic = 0x1badb002;
mb_flags = 0x3;
mb_checksum = -(mb_magic + mb_flags);

SECTIONS
{
	. = 0x100000;
	kbegin = .;
	.text BLOCK(4K) : ALIGN(4K)
	{
		LONG(mb_magic)
		LONG(mb_flags)
		LONG(mb_checksum)
		*(.text.*)
	}

	.rodata BLOCK(4K) : ALIGN(4K)
	{
		*(.rodata.*)
	}

        .eh_frame_hdr ALIGN(4K) : {
                *(.eh_frame_hdr)
        }

        .eh_frame ALIGN(4) : ONLY_IF_RO {
                  __eh_frame_start = .;
                  *(.eh_frame)
                  QUAD(0);
        }

        .gcc_except_table BLOCK(4K) : ALIGN(4K) ONLY_IF_RO {
                          *(.gcc_except_table .gcc_except_table.*)
        }

        .note : ALIGN(16) {
                *(.note*)
        }

        .ctors BLOCK(4K) : ALIGN(4K) {
               start_ctors = .;
               /* KEEP(*(SORT_BY_INIT_PRIORITY(.ctors.*))) */
               KEEP(*(SORT(.ctors.*)))
               KEEP(*(.ctors))
               end_ctors = .;
        }

        .dtors BLOCK(4K) : ALIGN(4K) {
               start_dtors = .;
               KEEP(*(SORT(.dtors.*)))
               KEEP(*(.dtors))
               end_dtors = .;
        }

        .eh_frame ALIGN(4) : ONLY_IF_RW {
                  __eh_frame_start = .;
                  *(.eh_frame)
                  QUAD(0);
        }

	.data BLOCK(4K) : ALIGN(4K)
	{
		*(.data.*)
	}

        . = ALIGN(64);
        .tdata  :
        {
                tls_start = ABSOLUTE(.);
                *(.tdata)
                *(.tdata.*)
                *(.gnu.linkonce.td.*)
        }

        .tbss :
        {
                *(.tbss)
                *(.tbss.*)
                *(.gnu.linkonce.tb.*)
                . = ALIGN(64);
                tls_end = ABSOLUTE(.);
        }

	.bss BLOCK(4K) : ALIGN(4K)
	{
                tcb0 = ABSOLUTE(.);
                . += (tls_end - tls_start) + 8;
		*(COMMON)
                *(.bss)
		*(.bss.*)
                *(.gnu.linkonce.b*)
	}

	/DISCARD/ :
	{
		*(.note.*);
	}

	. = ALIGN(4K);
	kend = .;
}
