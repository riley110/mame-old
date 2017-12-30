// license:BSD-3-Clause
// copyright-holders:Juergen Buchmueller,Ernesto Corvi
/*****************************************************************************
 *
 *   8000dasm.c
 *   Portable Z8000(2) emulator
 *   Z8000 disassembler; requires the z8000_exec table to be initialized
 *
 *****************************************************************************/

#include "emu.h"
#include "8000dasm.h"

const z8000_disassembler::opcode z8000_disassembler::table[] = {
	{ 0x0000, 0xffff,  1, 1, ".word   %#w0",                    0 },

	{ 0x0000, 0x000f,  1, 2, "addb    %rb3,%#b3",               0 },
	{ 0x0010, 0x00ff,  1, 1, "addb    %rb3,@%rw2",              0 },
	{ 0x0100, 0x010f,  1, 2, "add     %rw3,%#w1",               0 },
	{ 0x0110, 0x01ff,  1, 1, "add     %rw3,@%rw2",              0 },
	{ 0x0200, 0x020f,  1, 2, "subb    %rb3,%#b3",               0 },
	{ 0x0210, 0x02ff,  1, 1, "subb    %rb3,@%rw2",              0 },
	{ 0x0300, 0x030f,  1, 2, "sub     %rw3,%#w1",               0 },
	{ 0x0310, 0x03ff,  1, 1, "sub     %rw3,@%rw2",              0 },
	{ 0x0400, 0x040f,  1, 2, "orb     %rb3,%#b3",               0 },
	{ 0x0410, 0x04ff,  1, 1, "orb     %rb3,@%rw2",              0 },
	{ 0x0500, 0x050f,  1, 2, "or      %rw3,%#w1",               0 },
	{ 0x0510, 0x05ff,  1, 1, "or      %rw3,@%rw2",              0 },
	{ 0x0600, 0x060f,  1, 2, "andb    %rb3,%#b3",               0 },
	{ 0x0610, 0x06ff,  1, 1, "andb    %rb3,@%rw2",              0 },
	{ 0x0700, 0x070f,  1, 2, "and     %rw3,%#w1",               0 },
	{ 0x0710, 0x07ff,  1, 1, "and     %rw3,@%rw2",              0 },
	{ 0x0800, 0x080f,  1, 2, "xorb    %rb3,%#b3",               0 },
	{ 0x0810, 0x08ff,  1, 1, "xorb    %rb3,@%rw2",              0 },
	{ 0x0900, 0x090f,  1, 2, "xor     %rw3,%#w1",               0 },
	{ 0x0910, 0x09ff,  1, 1, "xor     %rw3,@%rw2",              0 },
	{ 0x0a00, 0x0a0f,  1, 2, "cpb     %rb3,%#b3",               0 },
	{ 0x0a10, 0x0aff,  1, 1, "cpb     %rb3,@%rw2",              0 },
	{ 0x0b00, 0x0b0f,  1, 2, "cp      %rw3,%#w1",               0 },
	{ 0x0b10, 0x0bff,  1, 1, "cp      %rw3,@%rw2",              0 },
	{ 0x0c10, 0x0cf0, 16, 1, "comb    @%rw2",                   0 },
	{ 0x0c11, 0x0cf1, 16, 2, "cpb     @%rw2,%#b3",              0 },
	{ 0x0c12, 0x0cf2, 16, 1, "negb    @%rw2",                   0 },
	{ 0x0c14, 0x0cf4, 16, 1, "testb   @%rw2",                   0 },
	{ 0x0c15, 0x0cf5, 16, 2, "ldb     @%rw2,%#b3",              0 },
	{ 0x0c16, 0x0cf6, 16, 1, "tsetb   @%rw2",                   0 },
	{ 0x0c18, 0x0cf8, 16, 1, "clrb    @%rw2",                   0 },
	{ 0x0d10, 0x0df0, 16, 1, "com     @%rw2",                   0 },
	{ 0x0d11, 0x0df1, 16, 2, "cp      @%rw2,%#w1",              0 },
	{ 0x0d12, 0x0df2, 16, 1, "neg     @%rw2",                   0 },
	{ 0x0d14, 0x0df4, 16, 1, "test    @%rw2",                   0 },
	{ 0x0d15, 0x0df5, 16, 2, "ld      @%rw2,%#w1",              0 },
	{ 0x0d16, 0x0df6, 16, 1, "tset    @%rw2",                   0 },
	{ 0x0d18, 0x0df8, 16, 1, "clr     @%rw2",                   0 },
	{ 0x0d19, 0x0df9, 16, 2, "push    @%rw2,%#w1",              0 },
	{ 0x0e00, 0x0eff,  1, 1, "ext0e   %#b1",                    0 },
	{ 0x0f00, 0x0fff,  1, 1, "ext0f   %#b1",                    0 },
	{ 0x1000, 0x100f,  1, 3, "cpl     %rl3,%#l1",               0 },
	{ 0x1010, 0x10ff,  1, 1, "cpl     %rl3,@%rw2",              0 },
	{ 0x1111, 0x11ff,  1, 1, "pushl   @%rw2,@%rw3",             0 },
	{ 0x1200, 0x120f,  1, 3, "subl    %rl3,%#l1",               0 },
	{ 0x1210, 0x12ff,  1, 1, "subl    %rl3,@%rw2",              0 },
	{ 0x1311, 0x13ff,  1, 1, "push    @%rw2,@%rw3",             0 },
	{ 0x1400, 0x140f,  1, 3, "ldl     %rl3,%#l1",               0 },
	{ 0x1410, 0x14ff,  1, 1, "ldl     %rl3,@%rw2",              0 },
	{ 0x1511, 0x15ff,  1, 1, "popl    @%rw3,@%rw2",             0 },
	{ 0x1600, 0x160f,  1, 3, "addl    %rl3,%#l1",               0 },
	{ 0x1610, 0x16ff,  1, 1, "addl    %rl3,@%rw2",              0 },
	{ 0x1711, 0x17ff,  1, 1, "pop     @%rw3,@%rw2",             0 },
	{ 0x1800, 0x180f,  1, 1, "multl   %rq3,@%l#1",              0 },
	{ 0x1810, 0x18ff,  1, 1, "multl   %rq3,@%rw2",              0 },
	{ 0x1900, 0x190f,  1, 2, "mult    %rl3,%#w1",               0 },
	{ 0x1910, 0x19ff,  1, 1, "mult    %rl3,@%rw2",              0 },
	{ 0x1a00, 0x1a0f,  1, 3, "divl    %rq3,%#l1",               0 },
	{ 0x1a10, 0x1aff,  1, 1, "divl    %rq3,@%rw2",              0 },
	{ 0x1b00, 0x1b0f,  1, 2, "div     %rl3,%#w1",               0 },
	{ 0x1b10, 0x1bff,  1, 1, "div     %rl3,@%rw2",              0 },
	{ 0x1c11, 0x1cf1, 16, 2, "ldm     %rw5,@%rw2,#%n",          0 },
	{ 0x1c18, 0x1cf8, 16, 1, "testl   @%rw2",                   0 },
	{ 0x1c19, 0x1cf9, 16, 2, "ldm     @%rw2,%rw5,#%n",          0 },
	{ 0x1d10, 0x1dff,  1, 1, "ldl     @%rw2,%rl3",              0 },
	{ 0x1e10, 0x1eff,  1, 1, "jp      %c3,@%rl2",               0 },
	{ 0x1f10, 0x1ff0, 16, 1, "call    %rw2",                    STEP_OVER },
	{ 0x2000, 0x200f,  1, 2, "ldb     %rb3,%#b3",               0 },
	{ 0x2010, 0x20ff,  1, 1, "ldb     %rb3,@%rw2",              0 },
	{ 0x2100, 0x210f,  1, 2, "ld      %rw3,%#w1",               0 },
	{ 0x2110, 0x21ff,  1, 1, "ld      %rw3,@%rw2",              0 },
	{ 0x2200, 0x220f,  1, 2, "resb    %rb5,%rw3",               0 },
	{ 0x2210, 0x22ff,  1, 1, "resb    @%rw2,%3",                0 },
	{ 0x2300, 0x230f,  1, 2, "res     %rw5,%rw3",               0 },
	{ 0x2310, 0x23ff,  1, 1, "res     @%rw2,%3",                0 },
	{ 0x2400, 0x240f,  1, 2, "setb    %rb5,%rw3",               0 },
	{ 0x2410, 0x24ff,  1, 1, "setb    @%rw2,%3",                0 },
	{ 0x2500, 0x250f,  1, 2, "set     %rw5,%rw3",               0 },
	{ 0x2510, 0x25ff,  1, 1, "set     @%rw2,%3",                0 },
	{ 0x2600, 0x260f,  1, 2, "bitb    %rb5,%rw3",               0 },
	{ 0x2610, 0x26ff,  1, 1, "bitb    @%rw2,%3",                0 },
	{ 0x2700, 0x270f,  1, 2, "bit     %rw5,%rw3",               0 },
	{ 0x2710, 0x27ff,  1, 1, "bit     @%rw2,%3",                0 },
	{ 0x2810, 0x28ff,  1, 1, "incb    @%rw2,%+3",               0 },
	{ 0x2910, 0x29ff,  1, 1, "inc     @%rw2,%+3",               0 },
	{ 0x2a10, 0x2aff,  1, 1, "decb    @%rw2,%+3",               0 },
	{ 0x2b10, 0x2bff,  1, 1, "dec     @%rw2,%+3",               0 },
	{ 0x2c10, 0x2cff,  1, 1, "exb     %rb3,@%rw2",              0 },
	{ 0x2d10, 0x2dff,  1, 1, "ex      %rw3,@%rw2",              0 },
	{ 0x2e10, 0x2eff,  1, 1, "ldb     @%rw2,%rb3",              0 },
	{ 0x2f10, 0x2fff,  1, 1, "ld      @%rw2,%rw3",              0 },
	{ 0x3000, 0x300f,  1, 2, "ldrb    %rb3,%p1",                0 },
	{ 0x3010, 0x30ff,  1, 2, "ldb     %rb3,%rw2(%#w1)",         0 },
	{ 0x3100, 0x310f,  1, 2, "ldr     %rw3,%p1",                0 },
	{ 0x3110, 0x31ff,  1, 2, "ld      %rw3,%rw2(%#w1)",         0 },
	{ 0x3200, 0x320f,  1, 2, "ldrb    %p1,%rb3",                0 },
	{ 0x3210, 0x32ff,  1, 2, "ldb     %rw2(%#w1),%rb3",         0 },
	{ 0x3300, 0x330f,  1, 2, "ldr     %p1,%rw3",                0 },
	{ 0x3310, 0x33ff,  1, 2, "ld      %rw2(%#w1),%rw3",         0 },
	{ 0x3400, 0x340f,  1, 2, "ldar    p%rw3,%p1",               0 },
	{ 0x3410, 0x34ff,  1, 2, "lda     p%rw3,%rw2(%#w1)",        0 },
	{ 0x3500, 0x350f,  1, 2, "ldrl    %rl3,%p1",                0 },
	{ 0x3510, 0x35ff,  1, 2, "ldl     %rl3,%rw2(%#w1)",         0 },
	{ 0x3600, 0x3600,  1, 1, "bpt",                             0 },
	{ 0x3601, 0x36ff,  1, 1, "rsvd36",                          0 },
	{ 0x3700, 0x370f,  1, 2, "ldrl    %p1,%rl3",                0 },
	{ 0x3710, 0x37ff,  1, 2, "ldl     %rw2(%#w1),%rl3",         0 },
	{ 0x3800, 0x38ff,  1, 1, "rsvd38",                          0 },
	{ 0x3910, 0x39f0, 16, 1, "ldps    @%rw2",                   0 },
	{ 0x3a00, 0x3af0, 16, 2, "%R @%rw6,@%rw2,%rw5",             0 },
	{ 0x3a01, 0x3af1, 16, 2, "%R @%rw6,@%rw2,%rw5",             0 },
	{ 0x3a02, 0x3af2, 16, 2, "%R @%rw6,@%rw2,%rw5",             0 },
	{ 0x3a03, 0x3af3, 16, 2, "%R @%rw6,@%rw2,%rw5",             0 },
	{ 0x3a04, 0x3af4, 16, 2, "%R %rb2,%#w1",                    0 },
	{ 0x3a05, 0x3af5, 16, 2, "%R %rb2,%#w1",                    0 },
	{ 0x3a06, 0x3af6, 16, 2, "%R %#w1,%rb2",                    0 },
	{ 0x3a07, 0x3af7, 16, 2, "%R %#w1,%rb2",                    0 },
	{ 0x3a08, 0x3af8, 16, 2, "%R @%rw6,@%rw2,%rw5",             0 },
	{ 0x3a09, 0x3af9, 16, 2, "%R @%rw6,@%rw2,%rw5",             0 },
	{ 0x3a0a, 0x3afa, 16, 2, "%R @%rw6,@%rw2,%rw5",             0 },
	{ 0x3a0b, 0x3afb, 16, 2, "%R @%rw6,@%rw2,%rw5",             0 },
	{ 0x3b00, 0x3bf0, 16, 2, "%R @%rw6,@%rw2,%rw5",             0 },
	{ 0x3b01, 0x3bf1, 16, 2, "%R @%rw6,@%rw2,%rw5",             0 },
	{ 0x3b02, 0x3bf2, 16, 2, "%R @%rw6,@%rw2,%rw5",             0 },
	{ 0x3b03, 0x3bf3, 16, 2, "%R @%rw6,@%rw2,%rw5",             0 },
	{ 0x3b04, 0x3bf4, 16, 2, "%R %rw2,%#w1",                    0 },
	{ 0x3b05, 0x3bf5, 16, 2, "%R %rw2,%#w1",                    0 },
	{ 0x3b06, 0x3bf6, 16, 2, "%R %#w1,%rw2",                    0 },
	{ 0x3b07, 0x3bf7, 16, 2, "%R %#w1,%rw2",                    0 },
	{ 0x3b08, 0x3bf8, 16, 2, "%R @%rw6,@%rw2,%rw5",             0 },
	{ 0x3b09, 0x3bf9, 16, 2, "%R @%rw6,@%rw2,%rw5",             0 },
	{ 0x3b0a, 0x3bfa, 16, 2, "%R @%rw6,@%rw2,%rb5",             0 },
	{ 0x3b0b, 0x3bfb, 16, 2, "%R @%rw6,@%rw2,%rb5",             0 },
	{ 0x3c00, 0x3cff,  1, 1, "inb     %rb3,@%rw2",              0 },
	{ 0x3d00, 0x3dff,  1, 1, "in      %rw3,@%rw2",              0 },
	{ 0x3e00, 0x3eff,  1, 1, "outb    @%rw2,%rb3",              0 },
	{ 0x3f00, 0x3fff,  1, 1, "out     @%rw2,%rw3",              0 },
	{ 0x4000, 0x400f,  1, 2, "addb    %rb3,%a1",                0 },
	{ 0x4010, 0x40ff,  1, 2, "addb    %rb3,%a1(%rw2)",          0 },
	{ 0x4100, 0x410f,  1, 2, "add     %rw3,%a1",                0 },
	{ 0x4110, 0x41ff,  1, 2, "add     %rw3,%a1(%rw2)",          0 },
	{ 0x4200, 0x420f,  1, 2, "subb    %rb3,%a1",                0 },
	{ 0x4210, 0x42ff,  1, 2, "subb    %rb3,%a1(%rw2)",          0 },
	{ 0x4300, 0x430f,  1, 2, "sub     %rw3,%a1",                0 },
	{ 0x4310, 0x43ff,  1, 2, "sub     %rw3,%a1(%rw2)",          0 },
	{ 0x4400, 0x440f,  1, 2, "orb     %rb3,%a1",                0 },
	{ 0x4410, 0x44ff,  1, 2, "orb     %rb3,%a1(%rw2)",          0 },
	{ 0x4500, 0x450f,  1, 2, "or      %rw3,%a1",                0 },
	{ 0x4510, 0x45ff,  1, 2, "or      %rw3,%a1(%rw2)",          0 },
	{ 0x4600, 0x460f,  1, 2, "andb    %rb3,%a1",                0 },
	{ 0x4610, 0x46ff,  1, 2, "andb    %rb3,%a1(%rw2)",          0 },
	{ 0x4700, 0x470f,  1, 2, "and     %rw3,%a1",                0 },
	{ 0x4710, 0x47ff,  1, 2, "and     %rw3,%a1(%rw2)",          0 },
	{ 0x4800, 0x480f,  1, 2, "xorb    %rb3,%a1",                0 },
	{ 0x4810, 0x48ff,  1, 2, "xorb    %rb3,%a1(%rw2)",          0 },
	{ 0x4900, 0x490f,  1, 2, "xor     %rw3,%a1",                0 },
	{ 0x4910, 0x49ff,  1, 2, "xor     %rw3,%a1(%rw2)",          0 },
	{ 0x4a00, 0x4a0f,  1, 2, "cpb     %rb3,%a1",                0 },
	{ 0x4a10, 0x4aff,  1, 2, "cpb     %rb3,%a1(%rw2)",          0 },
	{ 0x4b00, 0x4b0f,  1, 2, "cp      %rw3,%a1",                0 },
	{ 0x4b10, 0x4bff,  1, 2, "cp      %rw3,%a1(%rw2)",          0 },
	{ 0x4c00, 0x4c00,  1, 2, "comb    %a1",                     0 },
	{ 0x4c01, 0x4c01,  1, 3, "cpb     %a1,%#b3",                0 },
	{ 0x4c02, 0x4c02,  1, 2, "negb    %a1",                     0 },
	{ 0x4c04, 0x4c04,  1, 2, "testb   %a1",                     0 },
	{ 0x4c05, 0x4c05,  1, 3, "ldb     %a1,%#b3",                0 },
	{ 0x4c06, 0x4c06,  1, 2, "tsetb   %a1",                     0 },
	{ 0x4c08, 0x4c08,  1, 2, "clrb    %a1",                     0 },
	{ 0x4c10, 0x4cf0, 16, 2, "comb    %a1(%rw2)",               0 },
	{ 0x4c11, 0x4cf1, 16, 3, "cpb     %a1(%rw2),%#b3",          0 },
	{ 0x4c12, 0x4cf2, 16, 2, "negb    %a1(%rw2)",               0 },
	{ 0x4c14, 0x4cf4, 16, 2, "testb   %a1(%rw2)",               0 },
	{ 0x4c15, 0x4cf5, 16, 3, "ldb     %a1(%rw2),%#b3",          0 },
	{ 0x4c16, 0x4cf6, 16, 2, "tsetb   %a1(%rw2)",               0 },
	{ 0x4c18, 0x4cf8, 16, 2, "clrb    %a1(%rw2)",               0 },
	{ 0x4d00, 0x4d00,  1, 2, "com     %a1",                     0 },
	{ 0x4d01, 0x4d01,  1, 3, "cp      %a1,%#w2",                0 },
	{ 0x4d02, 0x4d02,  1, 2, "neg     %a1",                     0 },
	{ 0x4d04, 0x4d04,  1, 2, "test    %a1",                     0 },
	{ 0x4d05, 0x4d05,  1, 3, "ld      %a1,%#w2",                0 },
	{ 0x4d06, 0x4d06,  1, 2, "tset    %a1",                     0 },
	{ 0x4d08, 0x4d08,  1, 2, "clr     %a1",                     0 },
	{ 0x4d10, 0x4df0, 16, 2, "com     %a1(%rw2)",               0 },
	{ 0x4d11, 0x4df1, 16, 3, "cp      %a1(%rw2),%#w2",          0 },
	{ 0x4d12, 0x4df2, 16, 2, "neg     %a1(%rw2)",               0 },
	{ 0x4d14, 0x4df4, 16, 2, "test    %a1(%rw2)",               0 },
	{ 0x4d15, 0x4df5, 16, 3, "ld      %a1(%rw2),%#w2",          0 },
	{ 0x4d16, 0x4df6, 16, 2, "tset    %a1(%rw2)",               0 },
	{ 0x4d18, 0x4df8, 16, 2, "clr     %a1(%rw2)",               0 },
	{ 0x4e11, 0x4ef0, 16, 2, "ldb     %a1(%rw2),%rb3",          0 },
	{ 0x5000, 0x500f,  1, 2, "cpl     %rl3,%a1",                0 },
	{ 0x5010, 0x50ff,  1, 2, "cpl     %rl3,%a1(%rw2)",          0 },
	{ 0x5110, 0x51f0, 16, 2, "pushl   @%rw2,%a1",               0 },
	{ 0x5111, 0x51f1, 16, 2, "pushl   @%rw2,%a1(%rw3)",         0 },
	{ 0x5112, 0x51f2, 16, 2, "pushl   @%rw2,%a1(%rw3)",         0 },
	{ 0x5113, 0x51f3, 16, 2, "pushl   @%rw2,%a1(%rw3)",         0 },
	{ 0x5114, 0x51f4, 16, 2, "pushl   @%rw2,%a1(%rw3)",         0 },
	{ 0x5115, 0x51f5, 16, 2, "pushl   @%rw2,%a1(%rw3)",         0 },
	{ 0x5116, 0x51f6, 16, 2, "pushl   @%rw2,%a1(%rw3)",         0 },
	{ 0x5117, 0x51f7, 16, 2, "pushl   @%rw2,%a1(%rw3)",         0 },
	{ 0x5118, 0x51f8, 16, 2, "pushl   @%rw2,%a1(%rw3)",         0 },
	{ 0x5119, 0x51f9, 16, 2, "pushl   @%rw2,%a1(%rw3)",         0 },
	{ 0x511a, 0x51fa, 16, 2, "pushl   @%rw2,%a1(%rw3)",         0 },
	{ 0x511b, 0x51fb, 16, 2, "pushl   @%rw2,%a1(%rw3)",         0 },
	{ 0x511c, 0x51fc, 16, 2, "pushl   @%rw2,%a1(%rw3)",         0 },
	{ 0x511d, 0x51fd, 16, 2, "pushl   @%rw2,%a1(%rw3)",         0 },
	{ 0x511e, 0x51fe, 16, 2, "pushl   @%rw2,%a1(%rw3)",         0 },
	{ 0x511f, 0x51ff, 16, 2, "pushl   @%rw2,%a1(%rw3)",         0 },
	{ 0x5200, 0x520f,  1, 2, "subl    %rl3,%a1",                0 },
	{ 0x5210, 0x52ff,  1, 2, "subl    %rl3,%a1(%rw2)",          0 },
	{ 0x5310, 0x53f0, 16, 2, "push    @%rw2,%a1",               0 },
	{ 0x5311, 0x53f1, 16, 2, "push    @%rw2,%a1(%rw3)",         0 },
	{ 0x5312, 0x53f2, 16, 2, "push    @%rw2,%a1(%rw3)",         0 },
	{ 0x5313, 0x53f3, 16, 2, "push    @%rw2,%a1(%rw3)",         0 },
	{ 0x5314, 0x53f4, 16, 2, "push    @%rw2,%a1(%rw3)",         0 },
	{ 0x5315, 0x53f5, 16, 2, "push    @%rw2,%a1(%rw3)",         0 },
	{ 0x5316, 0x53f6, 16, 2, "push    @%rw2,%a1(%rw3)",         0 },
	{ 0x5317, 0x53f7, 16, 2, "push    @%rw2,%a1(%rw3)",         0 },
	{ 0x5318, 0x53f8, 16, 2, "push    @%rw2,%a1(%rw3)",         0 },
	{ 0x5319, 0x53f9, 16, 2, "push    @%rw2,%a1(%rw3)",         0 },
	{ 0x531a, 0x53fa, 16, 2, "push    @%rw2,%a1(%rw3)",         0 },
	{ 0x531b, 0x53fb, 16, 2, "push    @%rw2,%a1(%rw3)",         0 },
	{ 0x531c, 0x53fc, 16, 2, "push    @%rw2,%a1(%rw3)",         0 },
	{ 0x531d, 0x53fd, 16, 2, "push    @%rw2,%a1(%rw3)",         0 },
	{ 0x531e, 0x53fe, 16, 2, "push    @%rw2,%a1(%rw3)",         0 },
	{ 0x531f, 0x53ff, 16, 2, "push    @%rw2,%a1(%rw3)",         0 },
	{ 0x5400, 0x540f,  1, 2, "ldl     %rl3,%a1",                0 },
	{ 0x5410, 0x54ff,  1, 2, "ldl     %rl3,%a1(%rw2)",          0 },
	{ 0x5510, 0x55f0, 16, 2, "popl    %a1,@%rw2",               0 },
	{ 0x5511, 0x55f1, 16, 2, "popl    %a1(%rw3),@%rw2",         0 },
	{ 0x5512, 0x55f2, 16, 2, "popl    %a1(%rw3),@%rw2",         0 },
	{ 0x5513, 0x55f3, 16, 2, "popl    %a1(%rw3),@%rw2",         0 },
	{ 0x5514, 0x55f4, 16, 2, "popl    %a1(%rw3),@%rw2",         0 },
	{ 0x5515, 0x55f5, 16, 2, "popl    %a1(%rw3),@%rw2",         0 },
	{ 0x5516, 0x55f6, 16, 2, "popl    %a1(%rw3),@%rw2",         0 },
	{ 0x5517, 0x55f7, 16, 2, "popl    %a1(%rw3),@%rw2",         0 },
	{ 0x5518, 0x55f8, 16, 2, "popl    %a1(%rw3),@%rw2",         0 },
	{ 0x5519, 0x55f9, 16, 2, "popl    %a1(%rw3),@%rw2",         0 },
	{ 0x551a, 0x55fa, 16, 2, "popl    %a1(%rw3),@%rw2",         0 },
	{ 0x551b, 0x55fb, 16, 2, "popl    %a1(%rw3),@%rw2",         0 },
	{ 0x551c, 0x55fc, 16, 2, "popl    %a1(%rw3),@%rw2",         0 },
	{ 0x551d, 0x55fd, 16, 2, "popl    %a1(%rw3),@%rw2",         0 },
	{ 0x551e, 0x55fe, 16, 2, "popl    %a1(%rw3),@%rw2",         0 },
	{ 0x551f, 0x55ff, 16, 2, "popl    %a1(%rw3),@%rw2",         0 },
	{ 0x5600, 0x560f,  1, 2, "addl    %rl3,%a1",                0 },
	{ 0x5610, 0x56ff,  1, 2, "addl    %rl3,%a1(%rw2)",          0 },
	{ 0x5710, 0x57f0, 16, 2, "pop     %a1,@%rw2",               0 },
	{ 0x5711, 0x57f1, 16, 2, "pop     %a1(%rw3),@%rw2",         0 },
	{ 0x5712, 0x57f2, 16, 2, "pop     %a1(%rw3),@%rw2",         0 },
	{ 0x5713, 0x57f3, 16, 2, "pop     %a1(%rw3),@%rw2",         0 },
	{ 0x5714, 0x57f4, 16, 2, "pop     %a1(%rw3),@%rw2",         0 },
	{ 0x5715, 0x57f5, 16, 2, "pop     %a1(%rw3),@%rw2",         0 },
	{ 0x5716, 0x57f6, 16, 2, "pop     %a1(%rw3),@%rw2",         0 },
	{ 0x5717, 0x57f7, 16, 2, "pop     %a1(%rw3),@%rw2",         0 },
	{ 0x5718, 0x57f8, 16, 2, "pop     %a1(%rw3),@%rw2",         0 },
	{ 0x5719, 0x57f9, 16, 2, "pop     %a1(%rw3),@%rw2",         0 },
	{ 0x571a, 0x57fa, 16, 2, "pop     %a1(%rw3),@%rw2",         0 },
	{ 0x571b, 0x57fb, 16, 2, "pop     %a1(%rw3),@%rw2",         0 },
	{ 0x571c, 0x57fc, 16, 2, "pop     %a1(%rw3),@%rw2",         0 },
	{ 0x571d, 0x57fd, 16, 2, "pop     %a1(%rw3),@%rw2",         0 },
	{ 0x571e, 0x57fe, 16, 2, "pop     %a1(%rw3),@%rw2",         0 },
	{ 0x571f, 0x57ff, 16, 2, "pop     %a1(%rw3),@%rw2",         0 },
	{ 0x5800, 0x580f,  1, 2, "multl   %rq3,%a1",                0 },
	{ 0x5810, 0x58ff,  1, 2, "multl   %rq3,%a1(%rw2)",          0 },
	{ 0x5900, 0x590f,  1, 2, "mult    %rl3,%a1",                0 },
	{ 0x5910, 0x59ff,  1, 2, "mult    %rl3,%a1(%rw2)",          0 },
	{ 0x5a00, 0x5a0f,  1, 2, "divl    %rq3,%a1",                0 },
	{ 0x5a10, 0x5aff,  1, 2, "divl    %rq3,%a1(%rw2)",          0 },
	{ 0x5b00, 0x5b0f,  1, 2, "div     %rl3,%a1",                0 },
	{ 0x5b10, 0x5bff,  1, 2, "div     %rl3,%a1(%rw2)",          0 },
	{ 0x5c01, 0x5c01,  1, 3, "ldm     %rw5,%a2,#%n",            0 },
	{ 0x5c08, 0x5c08,  1, 2, "testl   %a1",                     0 },
	{ 0x5c09, 0x5c09,  1, 3, "ldm     %a2,%rw5,#%n",            0 },
	{ 0x5c11, 0x5cf1, 16, 3, "ldm     %rw5,%a2(%rw2),#%n",      0 },
	{ 0x5c18, 0x5cf8, 16, 2, "testl   %a1(%rw2)",               0 },
	{ 0x5c19, 0x5cf9, 16, 3, "ldm     %a2(%rw2),%rw5,#%n",      0 },
	{ 0x5d00, 0x5d0f,  1, 2, "ldl     %a1,%rl3",                0 },
	{ 0x5d10, 0x5dff,  1, 2, "ldl     %a1(%rw2),%rl3",          0 },
	{ 0x5e00, 0x5e0f,  1, 2, "jp      %c3,%a1",                 0 },
	{ 0x5e10, 0x5eff,  1, 2, "jp      %c3,%a1(%rw2)",           0 },
	{ 0x5f00, 0x5f00,  1, 2, "call    %a1",                     STEP_OVER },
	{ 0x5f10, 0x5ff0, 16, 2, "call    %a1(%rw2)",               STEP_OVER },
	{ 0x6000, 0x600f,  1, 2, "ldb     %rb3,%a1",                0 },
	{ 0x6010, 0x60ff,  1, 2, "ldb     %rb3,%a1(%rw2)",          0 },
	{ 0x6100, 0x610f,  1, 2, "ld      %rw3,%a1",                0 },
	{ 0x6110, 0x61ff,  1, 2, "ld      %rw3,%a1(%rw2)",          0 },
	{ 0x6200, 0x620f,  1, 2, "resb    %a1,%3",                  0 },
	{ 0x6210, 0x62ff,  1, 2, "resb    %a1(%rw2),%3",            0 },
	{ 0x6300, 0x630f,  1, 2, "res     %a1,%3",                  0 },
	{ 0x6310, 0x63ff,  1, 2, "res     %a1(%rw2),%3",            0 },
	{ 0x6400, 0x640f,  1, 2, "setb    %a1,%3",                  0 },
	{ 0x6410, 0x64ff,  1, 2, "setb    %a1(%rw2),%3",            0 },
	{ 0x6500, 0x650f,  1, 2, "set     %a1,%3",                  0 },
	{ 0x6510, 0x65ff,  1, 2, "set     %a1(%rw2),%3",            0 },
	{ 0x6600, 0x660f,  1, 2, "bitb    %a1,%3",                  0 },
	{ 0x6610, 0x66ff,  1, 2, "bitb    %a1(%rw2),%3",            0 },
	{ 0x6700, 0x670f,  1, 2, "bit     %a1,%3",                  0 },
	{ 0x6710, 0x67ff,  1, 2, "bit     %a1(%rw2),%3",            0 },
	{ 0x6800, 0x680f,  1, 2, "incb    %a1,%+3",                 0 },
	{ 0x6810, 0x68ff,  1, 2, "incb    %a1(%rw2),%+3",           0 },
	{ 0x6900, 0x690f,  1, 2, "inc     %a1,%+3",                 0 },
	{ 0x6910, 0x69ff,  1, 2, "inc     %a1(%rw2),%+3",           0 },
	{ 0x6a00, 0x6a0f,  1, 2, "decb    %a1,%+3",                 0 },
	{ 0x6a10, 0x6aff,  1, 2, "decb    %a1(%rw2),%+3",           0 },
	{ 0x6b00, 0x6b0f,  1, 2, "dec     %a1,%+3",                 0 },
	{ 0x6b10, 0x6bff,  1, 2, "dec     %a1(%rw2),%+3",           0 },
	{ 0x6c00, 0x6c0f,  1, 2, "exb     %rb3,%a1",                0 },
	{ 0x6c10, 0x6cff,  1, 2, "exb     %rb3,%a1(%rw2)",          0 },
	{ 0x6d00, 0x6d0f,  1, 2, "ex      %rw3,%a1",                0 },
	{ 0x6d10, 0x6dff,  1, 2, "ex      %rw3,%a1(%rw2)",          0 },
	{ 0x6e00, 0x6e0f,  1, 2, "ldb     %a1,%rb3",                0 },
	{ 0x6e10, 0x6eff,  1, 2, "ldb     %a1(%rw2),%rb3",          0 },
	{ 0x6f00, 0x6f0f,  1, 2, "ld      %a1,%rw3",                0 },
	{ 0x6f10, 0x6fff,  1, 2, "ld      %a1(%rw2),%rw3",          0 },
	{ 0x7010, 0x70ff,  1, 2, "ldb     %rb3,%rw2(%rw5)",         0 },
	{ 0x7110, 0x71ff,  1, 2, "ld      %rw3,%rw2(%rw5)",         0 },
	{ 0x7210, 0x72ff,  1, 2, "ldb     %rw2(%rw5),%rb3",         0 },
	{ 0x7310, 0x73ff,  1, 2, "ld      %rw2(%rw5),%rw3",         0 },
	{ 0x7410, 0x74ff,  1, 2, "lda     p%rw3,%rw2(%rw5)",        0 },
	{ 0x7510, 0x75ff,  1, 2, "ldl     %rl3,%rw2(%rw5)",         0 },
	{ 0x7600, 0x760f,  1, 2, "lda     p%rw3,%a1",               0 },
	{ 0x7610, 0x76ff,  1, 2, "lda     p%rw3,%a1(%rw2)",         0 },
	{ 0x7710, 0x77ff,  1, 2, "ldl     %rw2(%rw5),%rl3",         0 },
	{ 0x7800, 0x78ff,  1, 1, "rsvd78",                          0 },
	{ 0x7900, 0x7900,  1, 2, "ldps    %a1",                     0 },
	{ 0x7910, 0x79f0, 16, 2, "ldps    %a1(%rw2)",               0 },
	{ 0x7a00, 0x7a00,  1, 1, "halt",                            STEP_OVER },
	{ 0x7b00, 0x7b00,  1, 1, "iret",                            STEP_OUT },
	{ 0x7b08, 0x7b08,  1, 1, "mset",                            0 },
	{ 0x7b09, 0x7b09,  1, 1, "mres",                            0 },
	{ 0x7b0a, 0x7b0a,  1, 1, "mbit",                            0 },
	{ 0x7b0d, 0x7bfd, 16, 1, "mreq    %rw2",                    0 },
	{ 0x7c00, 0x7c03,  1, 1, "di      %i3",                     0 },
	{ 0x7c04, 0x7c07,  1, 1, "ei      %i3",                     0 },
	{ 0x7d00, 0x7df0, 16, 1, "ldctl   %rw2,ctrl0",              0 },
	{ 0x7d01, 0x7df1, 16, 1, "ldctl   %rw2,ctrl1",              0 },
	{ 0x7d02, 0x7df2, 16, 1, "ldctl   %rw2,fcw",                0 },
	{ 0x7d03, 0x7df3, 16, 1, "ldctl   %rw2,refresh",            0 },
	{ 0x7d04, 0x7df4, 16, 1, "ldctl   %rw2,psapseg",            0 },
	{ 0x7d05, 0x7df5, 16, 1, "ldctl   %rw2,psapoff",            0 },
	{ 0x7d06, 0x7df6, 16, 1, "ldctl   %rw2,nspseg",             0 },
	{ 0x7d07, 0x7df7, 16, 1, "ldctl   %rw2,nspoff",             0 },
	{ 0x7d08, 0x7df8, 16, 1, "ldctl   ctrl0,%rw2",              0 },
	{ 0x7d09, 0x7df9, 16, 1, "ldctl   ctrl1,%rw2",              0 },
	{ 0x7d0a, 0x7dfa, 16, 1, "ldctl   fcw,%rw2",                0 },
	{ 0x7d0b, 0x7dfb, 16, 1, "ldctl   refresh,%rw2",            0 },
	{ 0x7d0c, 0x7dfc, 16, 1, "ldctl   psapseg,%rw2",            0 },
	{ 0x7d0d, 0x7dfd, 16, 1, "ldctl   psapoff,%rw2",            0 },
	{ 0x7d0e, 0x7dfe, 16, 1, "ldctl   nspseg,%rw2",             0 },
	{ 0x7d0f, 0x7dff, 16, 1, "ldctl   nspoff,%rw2",             0 },
	{ 0x7e00, 0x7eff,  1, 1, "rsvd7e  %#b1",                    0 },
	{ 0x7f00, 0x7fff,  1, 1, "sc      %#b1",                    STEP_OVER },
	{ 0x8000, 0x80ff,  1, 1, "addb    %rb3,%rb2",               0 },
	{ 0x8100, 0x81ff,  1, 1, "add     %rw3,%rw2",               0 },
	{ 0x8200, 0x82ff,  1, 1, "subb    %rb3,%rb2",               0 },
	{ 0x8300, 0x83ff,  1, 1, "sub     %rw3,%rw2",               0 },
	{ 0x8400, 0x84ff,  1, 1, "orb     %rb3,%rb2",               0 },
	{ 0x8500, 0x85ff,  1, 1, "or      %rw3,%rw2",               0 },
	{ 0x8600, 0x86ff,  1, 1, "andb    %rb3,%rb2",               0 },
	{ 0x8700, 0x87ff,  1, 1, "and     %rw3,%rw2",               0 },
	{ 0x8800, 0x88ff,  1, 1, "xorb    %rb3,%rb2",               0 },
	{ 0x8900, 0x89ff,  1, 1, "xor     %rw3,%rw2",               0 },
	{ 0x8a00, 0x8aff,  1, 1, "cpb     %rb3,%rb2",               0 },
	{ 0x8b00, 0x8bff,  1, 1, "cp      %rw3,%rw2",               0 },
	{ 0x8c00, 0x8cf0, 16, 1, "comb    %rb2",                    0 },
	{ 0x8c02, 0x8cf2, 16, 1, "negb    %rb2",                    0 },
	{ 0x8c04, 0x8cf4, 16, 1, "testb   %rb2",                    0 },
	{ 0x8c06, 0x8cf6, 16, 1, "tsetb   %rb2",                    0 },
	{ 0x8c01, 0x8cf1, 16, 1, "ldctlb  %rb2,flags",              0 },
	{ 0x8c08, 0x8cf8, 16, 1, "clrb    %rb2",                    0 },
	{ 0x8c09, 0x8cf9, 16, 1, "ldctlb  flags,%rb2",              0 },
	{ 0x8d00, 0x8df0, 16, 1, "com     %rw2",                    0 },
	{ 0x8d01, 0x8df1, 16, 1, "setflg  %f2",                     0 },
	{ 0x8d02, 0x8df2, 16, 1, "neg     %rw2",                    0 },
	{ 0x8d03, 0x8df3, 16, 1, "resflg  %f2",                     0 },
	{ 0x8d04, 0x8df4, 16, 1, "test    %rw2",                    0 },
	{ 0x8d05, 0x8df5, 16, 1, "comflg  %f2",                     0 },
	{ 0x8d06, 0x8df6, 16, 1, "tset    %rw2",                    0 },
	{ 0x8d07, 0x8d07,  1, 1, "nop",                             0 },
	{ 0x8d08, 0x8df8, 16, 1, "clr     %rw2",                    0 },
	{ 0x8e00, 0x8eff,  1, 1, "ext8e   %#b1",                    0 },
	{ 0x8f00, 0x8fff,  1, 1, "ext8f   %#b1",                    0 },
	{ 0x9000, 0x90ff,  1, 1, "cpl     %rl3,%rl2",               0 },
	{ 0x9110, 0x91ff,  1, 1, "pushl   @%rw2,%rl3",              0 },
	{ 0x9200, 0x92ff,  1, 1, "subl    %rl3,%rl2",               0 },
	{ 0x9310, 0x93ff,  1, 1, "push    @%rw2,%rw3",              0 },
	{ 0x9400, 0x94ff,  1, 1, "ldl     %rl3,%rl2",               0 },
	{ 0x9510, 0x95ff,  1, 1, "popl    %rl3,@%rw2",              0 },
	{ 0x9600, 0x96ff,  1, 1, "addl    %rl3,%rl2",               0 },
	{ 0x9710, 0x97ff,  1, 1, "pop     %rw3,@%rw2",              0 },
	{ 0x9800, 0x98ff,  1, 1, "multl   %rq3,%rl2",               0 },
	{ 0x9900, 0x99ff,  1, 1, "mult    %rl3,%rw2",               0 },
	{ 0x9a00, 0x9aff,  1, 1, "divl    %rq3,%rl2",               0 },
	{ 0x9b00, 0x9bff,  1, 1, "div     %rl3,%rw2",               0 },
	{ 0x9c00, 0x9cf8,  8, 1, "testl   %rl2",                    0 },
	{ 0x9d00, 0x9dff,  1, 1, "rsvd9d",                          0 },
	{ 0x9e00, 0x9e0f,  1, 1, "ret     %c3",                     STEP_OUT },
	{ 0x9f00, 0x9fff,  1, 1, "rsvd9f",                          0 },
	{ 0xa000, 0xa0ff,  1, 1, "ldb     %rb3,%rb2",               0 },
	{ 0xa100, 0xa1ff,  1, 1, "ld      %rw3,%rw2",               0 },
	{ 0xa200, 0xa2ff,  1, 1, "resb    %rb2,%3",                 0 },
	{ 0xa300, 0xa3ff,  1, 1, "res     %rw2,%3",                 0 },
	{ 0xa400, 0xa4ff,  1, 1, "setb    %rb2,%3",                 0 },
	{ 0xa500, 0xa5ff,  1, 1, "set     %rw2,%3",                 0 },
	{ 0xa600, 0xa6ff,  1, 1, "bitb    %rb2,%3",                 0 },
	{ 0xa700, 0xa7ff,  1, 1, "bit     %rw2,%3",                 0 },
	{ 0xa800, 0xa8ff,  1, 1, "incb    %rb2,%+3",                0 },
	{ 0xa900, 0xa9ff,  1, 1, "inc     %rw2,%+3",                0 },
	{ 0xaa00, 0xaaff,  1, 1, "decb    %rb2,%+3",                0 },
	{ 0xab00, 0xabff,  1, 1, "dec     %rw2,%+3",                0 },
	{ 0xac00, 0xacff,  1, 1, "exb     %rb3,%rb2",               0 },
	{ 0xad00, 0xadff,  1, 1, "ex      %rw3,%rw2",               0 },
	{ 0xae00, 0xaeff,  1, 1, "tccb    %c3,%rb2",                0 },
	{ 0xaf00, 0xafff,  1, 1, "tcc     %c3,%rw2",                0 },
	{ 0xb000, 0xb0f0, 16, 1, "dab     %rb2",                    0 },
	{ 0xb100, 0xb1f0, 16, 1, "extsb   %rw2",                    0 },
	{ 0xb107, 0xb1f7, 16, 1, "extsl   %rq2",                    0 },
	{ 0xb10a, 0xb1fa, 16, 1, "exts    %rl2",                    0 },
	{ 0xb200, 0xb2f0, 16, 1, "rlb     %rb2,%?3",                0 },
	{ 0xb201, 0xb2f1, 16, 2, "s%*lb    %rb2,%$3",               0 },
	{ 0xb202, 0xb2f2, 16, 1, "rlb     %rb2,%?3",                0 },
	{ 0xb203, 0xb2f3, 16, 2, "sdlb    %rb2,%rw5",               0 },
	{ 0xb204, 0xb2f4, 16, 1, "rrb     %rb2,%?3",                0 },
	{ 0xb206, 0xb2f6, 16, 1, "rrb     %rb2,%?3",                0 },
	{ 0xb208, 0xb2f8, 16, 1, "rlcb    %rb2,%?3",                0 },
	{ 0xb209, 0xb2f9, 16, 2, "s%*ab    %rb2,%$3",               0 },
	{ 0xb20a, 0xb2fa, 16, 1, "rlcb    %rb2,%?3",                0 },
	{ 0xb20b, 0xb2fb, 16, 2, "sdab    %rb2,%rw5",               0 },
	{ 0xb20c, 0xb2fc, 16, 1, "rrcb    %rb2,%?3",                0 },
	{ 0xb20e, 0xb2fe, 16, 1, "rrcb    %rb2,%?3",                0 },
	{ 0xb300, 0xb3f0, 16, 1, "rl      %rw2,%?3",                0 },
	{ 0xb301, 0xb3f1, 16, 2, "s%*l     %rw2,%$3",               0 },
	{ 0xb302, 0xb3f2, 16, 1, "rl      %rw2,%?3",                0 },
	{ 0xb303, 0xb3f3, 16, 2, "sdl     %rw2,%rw5",               0 },
	{ 0xb304, 0xb3f4, 16, 1, "rr      %rw2,%?3",                0 },
	{ 0xb305, 0xb3f5, 16, 2, "s%*ll    %rl2,%$3",               0 },
	{ 0xb306, 0xb3f6, 16, 1, "rr      %rw2,%?3",                0 },
	{ 0xb307, 0xb3f7, 16, 2, "sdll    %rl2,%rw5",               0 },
	{ 0xb308, 0xb3f8, 16, 1, "rlc     %rw2,%?3",                0 },
	{ 0xb309, 0xb3f9, 16, 2, "s%*a     %rw2,%$3",               0 },
	{ 0xb30a, 0xb3fa, 16, 1, "rlc     %rw2,%?3",                0 },
	{ 0xb30b, 0xb3fb, 16, 2, "sda     %rw2,%rw5",               0 },
	{ 0xb30c, 0xb3fc, 16, 1, "rrc     %rw2,%?3",                0 },
	{ 0xb30d, 0xb3fd, 16, 2, "s%*al    %rl2,%$3",               0 },
	{ 0xb30e, 0xb3fe, 16, 1, "rrc     %rw2,%?3",                0 },
	{ 0xb30f, 0xb3ff, 16, 2, "sdal    %rl2,%rw5",               0 },
	{ 0xb400, 0xb4ff,  1, 1, "adcb    %rb3,%rb2",               0 },
	{ 0xb500, 0xb5ff,  1, 1, "adc     %rw3,%rw2",               0 },
	{ 0xb600, 0xb6ff,  1, 1, "sbcb    %rb3,%rb2",               0 },
	{ 0xb700, 0xb7ff,  1, 1, "sbc     %rw3,%rw2",               0 },
	{ 0xb810, 0xb8f0, 16, 2, "trib    @%rw2,@%rw6,%rb5",        0 },
	{ 0xb812, 0xb8f2, 16, 2, "trtib   @%rw2,@%rw6,%rb5",        0 },
	{ 0xb814, 0xb8f4, 16, 2, "trirb   @%rw2,@%rw6,%rb5",        0 },
	{ 0xb816, 0xb8f6, 16, 2, "trtirb  @%rw2,@%rw6,%rb5",        0 },
	{ 0xb818, 0xb8f8, 16, 2, "trdb    @%rw2,@%rw6,%rb5",        0 },
	{ 0xb81a, 0xb8fa, 16, 2, "trtrb   @%rw2,@%rw6,%rb5",        0 },
	{ 0xb81c, 0xb8fc, 16, 2, "trdrb   @%rw2,@%rw6,%rb5",        0 },
	{ 0xb81e, 0xb8fe, 16, 2, "trtdrb  @%rw2,@%rw6,%rb5",        0 },
	{ 0xb900, 0xb9ff, 16, 1, "rsvdb9",                          0 },
	{ 0xba10, 0xbaf0, 16, 2, "cpib    %rb6,@%rw2,%rw5,%c7",     0 },
	{ 0xba11, 0xbaf1, 16, 2, "ldirb   @%rw6,@%rw2,%rw5",        STEP_OVER },
	{ 0xba12, 0xbaf2, 16, 2, "cpsib   @%rw6,@%rw2,%rw5,%c7",    0 },
	{ 0xba14, 0xbaf4, 16, 2, "cpirb   %rb6,@%rw2,%rw5,%c7",     STEP_OVER },
	{ 0xba16, 0xbaf6, 16, 2, "cpsirb  @%rw6,@%rw2,%rw5,%c7",    STEP_OVER },
	{ 0xba18, 0xbaf8, 16, 2, "cpdb    %rb6,@%rw2,%rw5,%c7",     0 },
	{ 0xba19, 0xbaf9, 16, 2, "lddrb   @%rw2,@%rw6,%rw5",        STEP_OVER },
	{ 0xba1a, 0xbafa, 16, 2, "cpsdb   @%rw6,@%rw2,%rw5,%c7",    0 },
	{ 0xba1c, 0xbafc, 16, 2, "cpdrb   %rb6,@%rw2,%rw5,%c7",     STEP_OVER },
	{ 0xba1e, 0xbafe, 16, 2, "cpsdrb  @%rw6,@%rw2,%rw5,%c7",    STEP_OVER },
	{ 0xbb10, 0xbbf0, 16, 2, "cpi     %rw6,@%rw2,%rw5,%c7",     0 },
	{ 0xbb11, 0xbbf1, 16, 2, "ldir    @%rw6,@%rw2,%rw5",        STEP_OVER },
	{ 0xbb12, 0xbbf2, 16, 2, "cpsi    @%rw6,@%rw2,%rw5,%c7",    0 },
	{ 0xbb14, 0xbbf4, 16, 2, "cpir    %rw6,@%rw2,%rw5,%c7",     STEP_OVER },
	{ 0xbb16, 0xbbf6, 16, 2, "cpsir   @%rw6,@%rw2,%rw5,%c7",    STEP_OVER },
	{ 0xbb18, 0xbbf8, 16, 2, "cpd     %rw6,@%rw2,%rw5,%c7",     0 },
	{ 0xbb19, 0xbbf9, 16, 2, "lddr    @%rw2,@%rw6,%rw5",        STEP_OVER },
	{ 0xbb1a, 0xbbfa, 16, 2, "cpsd    @%rw6,@%rw2,%rw5,%c7",    0 },
	{ 0xbb1c, 0xbbfc, 16, 2, "cpdr    %rw6,@%rw2,%rw5,%c7",     STEP_OVER },
	{ 0xbb1e, 0xbbfe, 16, 2, "cpsdr   @%rw6,@%rw2,%rw5,%c7",    STEP_OVER },
	{ 0xbc00, 0xbcff,  1, 1, "rrdb    %rb3,%rb2",               0 },
	{ 0xbd00, 0xbdff,  1, 1, "ldk     %rw2,%3",                 0 },
	{ 0xbe00, 0xbeff,  1, 1, "rldb    %rb3,%rb2",               0 },
	{ 0xbf00, 0xbfff,  1, 1, "rsvdbf",                          0 },
	{ 0xc000, 0xcfff,  1, 1, "ldb     %rb1,%#b1",               0 },
	{ 0xd000, 0xdfff,  1, 1, "calr    %d2",                     STEP_OVER },
	{ 0xe000, 0xefff,  1, 1, "jr      %c1,%d1",                 0 },
	{ 0xf000, 0xf07f,  1, 1, "dbjnz   %rb1,%d0",                STEP_OVER },
	{ 0xf100, 0xf17f,  1, 1, "dbjnz   %rb1,%d0",                STEP_OVER },
	{ 0xf200, 0xf27f,  1, 1, "dbjnz   %rb1,%d0",                STEP_OVER },
	{ 0xf300, 0xf37f,  1, 1, "dbjnz   %rb1,%d0",                STEP_OVER },
	{ 0xf400, 0xf47f,  1, 1, "dbjnz   %rb1,%d0",                STEP_OVER },
	{ 0xf500, 0xf57f,  1, 1, "dbjnz   %rb1,%d0",                STEP_OVER },
	{ 0xf600, 0xf67f,  1, 1, "dbjnz   %rb1,%d0",                STEP_OVER },
	{ 0xf700, 0xf77f,  1, 1, "dbjnz   %rb1,%d0",                STEP_OVER },
	{ 0xf800, 0xf87f,  1, 1, "dbjnz   %rb1,%d0",                STEP_OVER },
	{ 0xf900, 0xf97f,  1, 1, "dbjnz   %rb1,%d0",                STEP_OVER },
	{ 0xfa00, 0xfa7f,  1, 1, "dbjnz   %rb1,%d0",                STEP_OVER },
	{ 0xfb00, 0xfb7f,  1, 1, "dbjnz   %rb1,%d0",                STEP_OVER },
	{ 0xfc00, 0xfc7f,  1, 1, "dbjnz   %rb1,%d0",                STEP_OVER },
	{ 0xfd00, 0xfd7f,  1, 1, "dbjnz   %rb1,%d0",                STEP_OVER },
	{ 0xfe00, 0xfe7f,  1, 1, "dbjnz   %rb1,%d0",                STEP_OVER },
	{ 0xff00, 0xff7f,  1, 1, "dbjnz   %rb1,%d0",                STEP_OVER },
	{ 0xf080, 0xf0ff,  1, 1, "djnz    %rw1,%d0",                STEP_OVER },
	{ 0xf180, 0xf1ff,  1, 1, "djnz    %rw1,%d0",                STEP_OVER },
	{ 0xf280, 0xf2ff,  1, 1, "djnz    %rw1,%d0",                STEP_OVER },
	{ 0xf380, 0xf3ff,  1, 1, "djnz    %rw1,%d0",                STEP_OVER },
	{ 0xf480, 0xf4ff,  1, 1, "djnz    %rw1,%d0",                STEP_OVER },
	{ 0xf580, 0xf5ff,  1, 1, "djnz    %rw1,%d0",                STEP_OVER },
	{ 0xf680, 0xf6ff,  1, 1, "djnz    %rw1,%d0",                STEP_OVER },
	{ 0xf780, 0xf7ff,  1, 1, "djnz    %rw1,%d0",                STEP_OVER },
	{ 0xf880, 0xf8ff,  1, 1, "djnz    %rw1,%d0",                STEP_OVER },
	{ 0xf980, 0xf9ff,  1, 1, "djnz    %rw1,%d0",                STEP_OVER },
	{ 0xfa80, 0xfaff,  1, 1, "djnz    %rw1,%d0",                STEP_OVER },
	{ 0xfb80, 0xfbff,  1, 1, "djnz    %rw1,%d0",                STEP_OVER },
	{ 0xfc80, 0xfcff,  1, 1, "djnz    %rw1,%d0",                STEP_OVER },
	{ 0xfd80, 0xfdff,  1, 1, "djnz    %rw1,%d0",                STEP_OVER },
	{ 0xfe80, 0xfeff,  1, 1, "djnz    %rw1,%d0",                STEP_OVER },
	{ 0xff80, 0xffff,  1, 1, "djnz    %rw1,%d0",                STEP_OVER },
	{ 0,      0,       0, 0, nullptr,                            0}
};

z8000_disassembler::z8000_disassembler(config *conf) : m_config(conf)
{
	for (const opcode *opc = table; opc->size; opc++)
		for (u32 val = opc->beg; val <= opc->end; val += opc->step)
			oplist[val] = opc - table;
}


void z8000_disassembler::get_op(const data_buffer &opcodes, int i, offs_t &new_pc, u16 *w, u8 *b, u8 *n)
{
	uint16_t opcode = opcodes.r16(new_pc);
	w[i] = opcode;
	b[i*2+0] = opcode >> 8;
	b[i*2+1] = opcode & 0xff;
	n[i*4+0] = (opcode >> 12) & 0x0f;
	n[i*4+1] = (opcode >> 8) & 0x0f;
	n[i*4+2] = (opcode >> 4) & 0x0f;
	n[i*4+3] = opcode & 0x0f;
	new_pc += 2;
}

const char *const z8000_disassembler::cc[16] = {
	"n",   "lt",  "le",  "ule",  "pe/ov",   "mi",  "eq/z",   "c/ult",
	"a",   "ge",  "gt",  "ugt",  "po/nov",  "pl",  "ne/nz",  "nc/uge"
};

const char *const z8000_disassembler::flg[16] = {
	"",    "p/v",  "s",   "p/v,s",   "z",   "p/v,z",  "s,z",  "p/v,s,z",
	"c",   "p/v,c","s,c", "p/v,s,c", "z,c", "p/v,z,c","s,z,c","p/v,s,z,c"
};

const char *const z8000_disassembler::ints[4] = {
	"",    "vi",  "nvi",   "vi,nvi"
};

u32 z8000_disassembler::opcode_alignment() const
{
	return 2;
}

offs_t z8000_disassembler::disassemble(std::ostream &stream, offs_t pc, const data_buffer &opcodes, const data_buffer &params)
{
	u8 n[16];   /* opcode nibbles */
	u8 b[8];    /* opcode bytes */
	u16 w[4];    /* opcode words */

	offs_t new_pc = pc, i, j, tmp;
	const char *src;
	uint32_t flags = 0;
	uint32_t old_w;

	get_op(opcodes, 0, new_pc, w, b, n);
	switch (pc)
	{
		case 0x0000:
			util::stream_format(stream, ".word   #%%%04x ;RST", w[0]);
			break;
		case 0x0002:
			util::stream_format(stream, ".word   #%%%04x ;RST FCW", w[0]);
			break;
		case 0x0004:
			util::stream_format(stream, ".word   #%%%04x ;RST PC", w[0]);
			break;
		default:
		{
			const opcode &o = table[oplist[w[0]]];
			if (o.size > 1)
				get_op(opcodes, 1, new_pc, w, b, n);
			if (o.size > 2)
				get_op(opcodes, 2, new_pc, w, b, n);
			src = o.dasm;
			flags = o.dasmflags;

			while (*src)
			{
				if (*src == '%')
				{
					src++;
					switch (*src) {
					case '0': case '1': case '2': case '3':
					case '4': case '5': case '6': case '7':
						/* nibble number */
						i = *src++ - '0';
						util::stream_format(stream, "%d", n[i]);
						break;
					case '#':
						/* immediate */
						src++;
						switch (*src++) {
							case 'b': /* imm8 (byte) */
								i = *src++ - '0';
								util::stream_format(stream, "#%%%02x", b[i]);
								break;
							case 'w': /* imm16 (word) */
								i = *src++ - '0';
								util::stream_format(stream, "#%%%04x", w[i]);
								break;
							case 'l': /* imm32 (long) */
								i = *src++ - '0';
								util::stream_format(stream, "#%%%04x%04x", w[i], w[i+1]);
								break;
						}
						break;
					case '$':
						/* absolute immediate 8bit (rl/rr) */
						src++;
						i = *src++ - '0';
						util::stream_format(stream, "#%d", ((int8_t)b[i]<0) ? -(int8_t)b[i] : b[i]);
						break;
					case '+':
						/* imm4m1 (inc/dec value) */
						src++;
						i = *src++ - '0';
						util::stream_format(stream, "%i", n[i] + 1);
						break;
					case '*':
						/* left/right (rotate/shift) */
						src++;
						util::stream_format(stream, "%c", b[2] ? 'r' : 'l');
						break;
					case '?':
						/* imm1or2 (shift/rotate once or twice) */
						src++;
						i = *src++ - '0';
						util::stream_format(stream, "%c", (n[i] & 2) ? '2' : '1');
						break;
					case 'R':
						src++;
						//tmp = ((n[1] & 0x01) << 16) + (n[3] << 8) + (n[7] & 0x08);
						tmp = ((n[1] & 0x01) << 8) + (n[3] << 4) + (n[7] & 0x08);
						switch (tmp)
						{
							case 0x000: util::stream_format(stream, "inirb "); flags = STEP_OVER; break;
							case 0x008: util::stream_format(stream, "inib  "); break;
							case 0x010: util::stream_format(stream, "sinirb"); flags = STEP_OVER; break;
							case 0x018: util::stream_format(stream, "sinib "); break;
							case 0x020: util::stream_format(stream, "otirb "); flags = STEP_OVER; break;
							case 0x028: util::stream_format(stream, "outib "); break;
							case 0x030: util::stream_format(stream, "soutib"); break;
							case 0x038: util::stream_format(stream, "sotirb"); flags = STEP_OVER; break;
							case 0x040: util::stream_format(stream, "inb   "); break;
							case 0x048: util::stream_format(stream, "inb   "); break;
							case 0x050: util::stream_format(stream, "sinb  "); break;
							case 0x058: util::stream_format(stream, "sinb  "); break;
							case 0x060: util::stream_format(stream, "outb  "); break;
							case 0x068: util::stream_format(stream, "outb  "); break;
							case 0x070: util::stream_format(stream, "soutb "); break;
							case 0x078: util::stream_format(stream, "soutb "); break;
							case 0x080: util::stream_format(stream, "indrb "); flags = STEP_OVER; break;
							case 0x088: util::stream_format(stream, "indb  "); break;
							case 0x090: util::stream_format(stream, "sindrb"); flags = STEP_OVER; break;
							case 0x098: util::stream_format(stream, "sindb "); break;
							case 0x0a0: util::stream_format(stream, "otdrb "); flags = STEP_OVER; break;
							case 0x0a8: util::stream_format(stream, "outdb "); break;
							case 0x0b0: util::stream_format(stream, "soutdb"); break;
							case 0x0b8: util::stream_format(stream, "sotdrb"); flags = STEP_OVER; break;
							case 0x100: util::stream_format(stream, "inir  "); flags = STEP_OVER; break;
							case 0x108: util::stream_format(stream, "ini   "); break;
							case 0x110: util::stream_format(stream, "sinir "); flags = STEP_OVER; break;
							case 0x118: util::stream_format(stream, "sini  "); break;
							case 0x120: util::stream_format(stream, "otir  "); flags = STEP_OVER; break;
							case 0x128: util::stream_format(stream, "outi  "); break;
							case 0x130: util::stream_format(stream, "souti "); break;
							case 0x138: util::stream_format(stream, "sotir "); flags = STEP_OVER; break;
							case 0x140: util::stream_format(stream, "in    "); break;
							case 0x148: util::stream_format(stream, "in    "); break;
							case 0x150: util::stream_format(stream, "sin   "); break;
							case 0x158: util::stream_format(stream, "sin   "); break;
							case 0x160: util::stream_format(stream, "out   "); break;
							case 0x168: util::stream_format(stream, "out   "); break;
							case 0x170: util::stream_format(stream, "sout  "); break;
							case 0x178: util::stream_format(stream, "sout  "); break;
							case 0x180: util::stream_format(stream, "indr  "); flags = STEP_OVER; break;
							case 0x188: util::stream_format(stream, "ind   "); break;
							case 0x190: util::stream_format(stream, "sindr "); flags = STEP_OVER; break;
							case 0x198: util::stream_format(stream, "sind  "); break;
							case 0x1a0: util::stream_format(stream, "otdr  "); flags = STEP_OVER; break;
							case 0x1a8: util::stream_format(stream, "outd  "); break;
							case 0x1b0: util::stream_format(stream, "soutd "); break;
							case 0x1b8: util::stream_format(stream, "sotdr "); flags = STEP_OVER; break;
							default:
								util::stream_format(stream, "unk(0x%x)", tmp);
						}
						break;
					case 'a':
						/* address */
						src++;
						i = *src++ - '0';
						if (m_config->get_segmented_mode()) {
							uint32_t address;
							if (w[i] & 0x8000) {
								old_w = w[i];
								for (j = i; j < o.size; j++)
									w[j] = w[j + 1];
								get_op(opcodes, o.size - 1, new_pc, w, b, n);
								address = ((old_w & 0x7f00) << 16) | (w[i] & 0xffff);
							}
							else {
								address = ((w[i] & 0x7f00) << 16) | (w[i] & 0xff);
							}
							util::stream_format(stream, "<%%%02X>%%%04X", (address >> 24) & 0xff, address & 0xffff);
						}
						else util::stream_format(stream, "%%%04x", w[i]);
						break;
					case 'c':
						/* condition code */
						src++;
						i = *src++ - '0';
						if (n[i] == 8) {    /* always? */
							/* skip following comma */
							if (*src == ',')
								src++;
						}
						else util::stream_format(stream, "%s", cc[n[i]]);
						break;
					case 'd':
						/* displacement */
						src++;
						i = *src++ - '0';
						switch (i) {
							case 0: /* disp7 */
								tmp = new_pc - 2 * (w[0] & 0x7f);
								break;
							case 1: /* disp8 */
								tmp = new_pc + 2 * (int8_t)(w[0] & 0xff);
								break;
							case 2: /* disp12 */
								tmp = w[0] & 0x7ff;
								if (w[0] & 0x800)
									tmp = new_pc + 0x1000 -2 * tmp;
								else
									tmp = new_pc + -2 * tmp;
								break;
							default:
								tmp = 0;
								abort();
						}
						if (m_config->get_segmented_mode())
							util::stream_format(stream, "<%%%02X>%%%04X", (tmp >> 16) & 0xff, tmp & 0xffff);
						else
							util::stream_format(stream, "%%%04x", tmp);
						break;
					case 'f':
						/* flag (setflg/resflg/comflg) */
						src++;
						i = *src++ - '0';
						util::stream_format(stream, "%s", flg[n[i]]);
						break;
					case 'i':
						/* interrupts */
						src++;
						i = *src++ - '0';
						util::stream_format(stream, "%s", ints[n[i] & 3]);
						break;
					case 'n':
						/* register count for ldm */
						src++;
						util::stream_format(stream, "%d", n[7] + 1);
						break;
					case 'p':
						/* disp16 (pc relative) */
						src++;
						i = *src++ - '0';
						util::stream_format(stream, "%%%04x", new_pc + w[i]);
						break;
					case 'r':
						/* register */
						src++;
						switch (*src++) {
							case 'b':
								/* byte */
								i = *src++ - '0';
								if (n[i] & 8)
									util::stream_format(stream, "rl%d", n[i] & 7);
								else
									util::stream_format(stream, "rh%d", n[i]);
								break;
							case 'w':
								/* word */
								i = *src++ - '0';
								util::stream_format(stream, "r%d", n[i]);
								break;
							case 'l':
								/* long */
								i = *src++ - '0';
								util::stream_format(stream, "rr%d", n[i]);
								break;
							case 'q':
								/* quad word (long long) */
								i = *src++ - '0';
								util::stream_format(stream, "rq%d", n[i]);
								break;
						}
						break;
					default:
						stream << '%' << *src++;
						break;
					}
				} else stream << *src++;
			}
		}
		break;
	}
	return (new_pc - pc) | flags | SUPPORTED;
}
