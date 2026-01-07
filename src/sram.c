#include <unistd.h>
#include <stdio.h>
#include <stdint.h>

#include "smi.h"


void sram_helloworld(MEM_MAP smi_regs)
{
    uint8_t data[] = {'h', 'e', 'l', 'l', 'o', 'w', 'o', 'r', 'l', 'd', '!', '\0'};

    printf("Writing\n");
    smi_8byte_write(smi_regs, 0, data, sizeof(data));

    sleep(1); /* Need meaningful sleep when switching between reading and writing */
    printf("Reading\n");
    
    for(int i = 0; i < 12; i++)
    {
        uint8_t val = smi_8b_read(smi_regs, i % 64);
        printf("Address: %x ; Value: %d ; ASCII: %c\n", (i % 64), val, val);
    }
}

void sram_block_byte_write(MEM_MAP smi_regs)
{
    uint8_t data[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

    printf("Writing\n");
    smi_8byte_write(smi_regs, 0, data, sizeof(data));

    sleep(1); /* Need meaningful sleep when switching between reading and writing */
    printf("Reading\n");
    
    for(int i = 0; i < sizeof(data); i++)
    {
        uint8_t val = smi_8b_read(smi_regs, i % 64);
        printf("Address: %x ; Value: %d ; ASCII: %c\n", (i % 64), val, val);
    }
}