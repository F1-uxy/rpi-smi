#include <unistd.h>
#include <stdio.h>
#include <stdint.h>

#include "smi.h"
#include "errors.h"


void sram_helloworld(SMI_CXT* cxt)
{
    cxt->rw_config->rconfig->rwidth = SMI_16_BITS;
    cxt->rw_config->wconfig->wformat = SMI_XRGB;
    cxt->rw_config->wconfig->wswap = 0;


    uint32_t clearData[] = {'\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0'};
    uint32_t data32[] = {'h', 'e', 'l', 'l', 'o', 'w', 'o', 'r', 'l', 'd', '!', '\0'};
    //uint32_t data32[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, '\0'};

    //smi_direct_write_arr(cxt, clearData, 0, 12, SMI_ADDR_INC);
    //sleep(1);
    printf("Writing\n");
    smi_direct_write_arr(cxt, data32, 0, 12, SMI_ADDR_INC);
    sleep(1); /* Need meaningful sleep when switching between reading and writing */
    printf("Reading\n");

    uint16_t ret[12];
    for(int i = 0; i < 12; i++)
    {
        ret[i] = 0;
    }
    //int len_read = smi_direct_read_arr(cxt, ret, 0, 12, SMI_ADDR_INC);
    int len_read = smi_programmed_read_arr(cxt, ret, 5, 5);
    if(len_read < 0)
    {
        ERROR("Did not read");
        return;
    }

    for(int i = 0; i < len_read; i++)
    {
        printf("i=%d ; Address: %x ; Value: %u ; ASCII: %c\n", i, (i % 64), ret[i], ret[i]);
    }
}

int testbench_write(SMI_CXT* cxt, size_t len)
{
    int count = 0;
    int val = 0;
    for(size_t i = 0; i < len; i++)
    {
        count += smi_direct_write(cxt, 0xA, 0);
        //smi_direct_read(cxt, &val, 0);
        //if(val == 0xA) count++;
        //val = 0;
    }

    return count;
}

int testbench_read(SMI_CXT* cxt, size_t len)
{
    int count = 0;
    int val = 0;
    uint32_t ret[1024];
    uint32_t ret_single;

    for(size_t i = 0; i < len; i++)
    {
        //count += smi_direct_read_arr(cxt, ret, 1, 1024, 0);
        //count += smi_programmed_read_arr(cxt, ret, 1, 1024);
        smi_direct_read(cxt, &ret[0], 0);
        if(ret[0] == 0xA) count++;
        //printf("Val: %d\n", ret[0]);
        ret[0] = 0;
    }

    return count;
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