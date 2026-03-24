#ifndef CLK_H
#define CLK_H


#if PHYS_REG_BASE == 0x3F000000u
    #define CLK_BASE PHYS_REG_BASE + 0x101000
#endif

#define CLK_SMI_CTL 0xb0
#define CLK_SMI_DIV 0xb4
#define CLK_PASSWD 0x5a000000

#endif