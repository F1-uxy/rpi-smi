#ifndef SRAM_H
#define SRAM_H

void sram_helloworld(SMI_CXT* cxt);
void sram_block_byte_write(MEM_MAP smi_regs);

int testbench_write(SMI_CXT* cxt, size_t len);
long testbench_read(SMI_CXT* cxt, size_t len, int block_len);
void megbyte_load_thread_test(SMI_CXT* cxt);
void megbyte_load_block_test(SMI_CXT* cxt);

#endif