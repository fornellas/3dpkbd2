MEMORY
{
 ram (rwx) : ORIGIN = 0x20000000, LENGTH = 128K
 rom (rx) : ORIGIN = 0x08004000, LENGTH = 496K
}

INCLUDE cortex-m-generic.ld