#define MAIN_MEMORY_BASE 0x08000000
#define MAIN_MEMORY_SECTOR_0_SIZE 0x4000
#define MAIN_MEMORY_SECTOR_1_SIZE 0x4000
// Keep in sync with keyboard/keyboard.ld
// The embedded DFU bootloader erases the whole Sector 0 when we program
// any address of it. This forces us to offset the main program address to
// Sector 1, so we don't erase the bootloader when we flash the main program.
#define MAIN_PROGRAM_BASE (MAIN_MEMORY_BASE + MAIN_MEMORY_SECTOR_0_SIZE + MAIN_MEMORY_SECTOR_1_SIZE)

#define MAIN_MEMORY_MAX 0x0807FFFF