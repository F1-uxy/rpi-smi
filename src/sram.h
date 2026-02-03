#ifndef SRAM_H
#define SRAM_H

void sram_helloworld(SMI_CXT* cxt);
void sram_block_byte_write(MEM_MAP smi_regs);

int testbench_write(SMI_CXT* cxt, size_t len);
int testbench_read(SMI_CXT* cxt, size_t len);


#endif