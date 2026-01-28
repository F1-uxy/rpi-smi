#include <unistd.h>
#include <stdio.h>
#include <stdint.h>

#include "smi.h"


void sram_helloworld(SMI_CXT* cxt)
{
    uint32_t data32[] = {'h', 'e', 'l', 'l', 'o', 'w', 'o', 'r', 'l', 'd', '!', '\0'};

    printf("Writing\n");
    smi_direct_write_arr(cxt, data32, 0, 12, SMI_ADDR_INC);
    sleep(1); /* Need meaningful sleep when switching between reading and writing */
    printf("Reading\n");

    uint32_t ret[12];
    int len_read = smi_direct_read_arr(cxt, ret, 0, 12, SMI_ADDR_INC);

    for(int i = 0; i < len_read; i++)
    {
        printf("Address: %x ; Value: %d ; ASCII: %c\n", (i % 64), ret[i], ret[i]);
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