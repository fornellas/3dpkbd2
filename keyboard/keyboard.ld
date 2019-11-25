MEMORY
{
 ram (rwx) : ORIGIN = 0x20000000, LENGTH = 128K
 rom (rx) : ORIGIN = 0x08008000, LENGTH = 480K
}

INCLUDE cortex-m-generic.ld