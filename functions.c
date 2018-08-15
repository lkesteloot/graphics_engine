
#include <stdio.h>
#include <string.h>
#include "sim.h"
#include "functions.h"

void LDfunc(Register r1, Register r2, Register r3, Immediate imm) {
    CheckRegister(r1);
    CheckRegister(r2);
    imm += reg[r2];
    CheckAddress(imm);
    
    reg[r1] = dMem[imm];
    SetStatus(reg[r1]);
}

void STfunc(Register r1, Register r2, Register r3, Immediate imm) {
    CheckRegister(r1);
    CheckRegister(r2);
    imm += reg[r2];
    CheckAddress(imm);

    dMem[imm] = reg[r1];
    SetStatus(reg[r1]);
}

void LIfunc(Register r1, Register r2, Register r3, Immediate imm) {
    CheckRegister(r1);
    CheckRegister(r2);
    imm += reg[r2];

    reg[r1] = imm;
    SetStatus(reg[r1]);
}

void ADDfunc(Register r1, Register r2, Register r3, Immediate imm) {
    CheckRegister(r1);
    CheckRegister(r2);
    CheckRegister(r3);

    reg[r1] = reg[r2] + reg[r3];
    SetStatus(reg[r1]);
}

void SUBfunc(Register r1, Register r2, Register r3, Immediate imm) {
    CheckRegister(r1);
    CheckRegister(r2);
    CheckRegister(r3);

    reg[r1] = reg[r2] - reg[r3];
    SetStatus(reg[r1]);
}

void MULfunc(Register r1, Register r2, Register r3, Immediate imm) {
    CheckRegister(r1);
    CheckRegister(r2);
    CheckRegister(r3);

    reg[r1] = reg[r2] * reg[r3];
    SetStatus(reg[r1]);
}

void MULFfunc(Register r1, Register r2, Register r3, Immediate imm) {
    CheckRegister(r1);
    CheckRegister(r2);
    CheckRegister(r3);

    reg[r1] = mult32x32(reg[r2], reg[r3]);
    SetStatus(reg[r1]);
}

void RECPfunc(Register r1, Register r2, Register r3, Immediate imm) {
    CheckRegister(r1);
    CheckRegister(r2);

    if (reg[r2] == 0) {
	strcpy (error, "Divide by zero");
	stopped = ERROR;
    } else {
	reg[r1] = ((signed long)0x7fffffff /
	    (signed long)reg[r2] + 1) << 1;
    }
    SetStatus(reg[r1]);
}

void TSTfunc(Register r1, Register r2, Register r3, Immediate imm) {
    CheckRegister(r1);

    SetStatus(reg[r1]);
}

void ANDfunc(Register r1, Register r2, Register r3, Immediate imm) {
    CheckRegister(r1);
    CheckRegister(r2);
    CheckRegister(r3);

    reg[r1] = reg[r2] & reg[r3];
    SetStatus(reg[r1]);
}

void ORfunc(Register r1, Register r2, Register r3, Immediate imm) {
    CheckRegister(r1);
    CheckRegister(r2);
    CheckRegister(r3);

    reg[r1] = reg[r2] | reg[r3];
    SetStatus(reg[r1]);
}

void XORfunc(Register r1, Register r2, Register r3, Immediate imm) {
    CheckRegister(r1);
    CheckRegister(r2);
    CheckRegister(r3);

    reg[r1] = reg[r2] ^ reg[r3];
    SetStatus(reg[r1]);
}

void NOTfunc(Register r1, Register r2, Register r3, Immediate imm) {
    CheckRegister(r1);
    CheckRegister(r2);
    
    reg[r1] = ~reg[r2];
    SetStatus(reg[r1]);
}

void LSLfunc(Register r1, Register r2, Register r3, Immediate imm) {
    CheckRegister(r1);
    CheckRegister(r2);
    CheckRegister(r3);

    reg[r1] = reg[r2] << reg[r3];
    SetStatus(reg[r1]);
}

void LSRfunc(Register r1, Register r2, Register r3, Immediate imm) {
    CheckRegister(r1);
    CheckRegister(r2);
    CheckRegister(r3);

    reg[r1] = reg[r2] >> reg[r3];
    SetStatus(reg[r1]);
}

void ASRfunc(Register r1, Register r2, Register r3, Immediate imm) {
    CheckRegister(r1);
    CheckRegister(r2);
    CheckRegister(r3);

    reg[r1] = (signed) reg[r2] >> reg[r3];
    SetStatus(reg[r1]);
}

void LSLIfunc(Register r1, Register r2, Register r3, Immediate imm) {
    CheckRegister(r1);
    CheckRegister(r2);

    reg[r1] = reg[r2] << imm;
    SetStatus(reg[r1]);
}

void LSRIfunc(Register r1, Register r2, Register r3, Immediate imm) {
    CheckRegister(r1);
    CheckRegister(r2);

    reg[r1] = reg[r2] >> imm;
    SetStatus(reg[r1]);
}

void ASRIfunc(Register r1, Register r2, Register r3, Immediate imm) {
    CheckRegister(r1);
    CheckRegister(r2);

    reg[r1] = (signed) reg[r2] >> imm;
    SetStatus(reg[r1]);
}

void JMPfunc(Register r1, Register r2, Register r3, Immediate imm) {
    CheckRegister(r2);
    imm += reg[r2];
    CheckAddress(imm);
    PC = imm;
}

void CALLfunc(Register r1, Register r2, Register r3, Immediate imm) {
    CheckRegister(r2);
    imm += reg[r2];
    CheckAddress(imm);
    reg[31] = PC;
    PC = imm;
}

void JEQfunc(Register r1, Register r2, Register r3, Immediate imm) {
    CheckRegister(r2);
    imm += reg[r2];
    CheckAddress(imm);
    if (CheckZ()) {
	PC = imm;
    }
}

void JEQLfunc(Register r1, Register r2, Register r3, Immediate imm) {
    JEQfunc (r1, r2, r3, imm);
}

void JNEfunc(Register r1, Register r2, Register r3, Immediate imm) {
    CheckRegister(r2);
    imm += reg[r2];
    CheckAddress(imm);
    if (!CheckZ()) {
	PC = imm;
    }
}

void JNELfunc(Register r1, Register r2, Register r3, Immediate imm) {
    JNEfunc (r1, r2, r3, imm);
}

void JLTfunc(Register r1, Register r2, Register r3, Immediate imm) {
    CheckRegister(r2);
    imm += reg[r2];
    CheckAddress(imm);
    if (CheckN()) {
	PC = imm;
    }
}

void JLTLfunc(Register r1, Register r2, Register r3, Immediate imm) {
    JLTfunc (r1, r2, r3, imm);
}

void JLEfunc(Register r1, Register r2, Register r3, Immediate imm) {
    CheckRegister(r2);
    imm += reg[r2];
    CheckAddress(imm);
    if (CheckN() || CheckZ()) {
	PC = imm;
    }
}

void JLELfunc(Register r1, Register r2, Register r3, Immediate imm) {
    JLEfunc (r1, r2, r3, imm);
}

void JGTfunc(Register r1, Register r2, Register r3, Immediate imm) {
    CheckRegister(r2);
    imm += reg[r2];
    CheckAddress(imm);
    if (!CheckN() && !CheckZ()) {
	PC = imm;
    }
}

void JGTLfunc(Register r1, Register r2, Register r3, Immediate imm) {
    JGTfunc (r1, r2, r3, imm);
}

void JGEfunc(Register r1, Register r2, Register r3, Immediate imm) {
    CheckRegister(r2);
    imm += reg[r2];
    CheckAddress(imm);
    if (!CheckN()) {
	PC = imm;
    }
}

void JGELfunc(Register r1, Register r2, Register r3, Immediate imm) {
    JGEfunc (r1, r2, r3, imm);
}

void DOTfunc(Register r1, Register r2, Register r3, Immediate imm) {
    int		i;
    Word	dot;
    
    CheckRegister(r1);
    CheckRegister(r2);
    CheckRegister(r3);
    CheckAddress(reg[r2]);
    CheckAddress(reg[r3]);

    dot = 0;
    for (i = 0; i < 4; i++) {
	dot += mult32x32 (dMem[reg[r2] + i],
	    dMem[reg[r3] + i]);
    }
    reg[r1] = dot;
    SetStatus(reg[r1]);
}

void READfunc(Register r1, Register r2, Register r3, Immediate imm) {
    int i;

    CheckRegister(r1);
    CheckRegister(r2);

    for (i = 0; i < 16; i++) {
        // External memory is big endian.
	dMem[reg[r1] + i] =
	    eMem[(reg[r2] + i) * 4 + 0] << 24 |
	    eMem[(reg[r2] + i) * 4 + 1] << 16 |
	    eMem[(reg[r2] + i) * 4 + 2] << 8 |
	    eMem[(reg[r2] + i) * 4 + 3] << 0;
    }
}

void WRITEfunc(Register r1, Register r2, Register r3, Immediate imm) {
    int i;

    CheckRegister(r1);
    CheckRegister(r2);

    for (i = 0; i < 16; i++) {
        // External memory is big endian.
	eMem[(reg[r1] + i)*4 + 0] = dMem[reg[r2] + i] >> 24;
	eMem[(reg[r1] + i)*4 + 1] = dMem[reg[r2] + i] >> 16;
	eMem[(reg[r1] + i)*4 + 2] = dMem[reg[r2] + i] >> 8;
	eMem[(reg[r1] + i)*4 + 3] = dMem[reg[r2] + i] >> 0;
    }
}

void CLIPfunc(Register r1, Register r2, Register r3, Immediate imm) {
    CheckRegister(r1);
    CheckRegister(r2);

    ClearStatus();
    if (reg[r1] > reg[r2])
	SetZ();

    if (reg[r1] < -reg[r2])
	SetN();
}

void SCRSETfunc(Register r1, Register r2, Register r3, Immediate imm) {
    width = reg[r1];
    height = reg[r2];
    fbAddr = (dMem[imm] >> 16) << 10; /* Specified in K */
    zAddr = (dMem[imm] & 0xFFFF) << 10;
}

void CLSfunc(Register r1, Register r2, Register r3, Immediate imm) {
    Word	color, z, i;

    CheckRegister(r1);
    CheckRegister(r2);

    color = reg[r1];
    z = reg[r2];

    for (i = 0; i < width * height; i++) {
	eMem[fbAddr + i*2    ] = (color >> 8) & 0xFF;
	eMem[fbAddr + i*2 + 1] = (color >> 0) & 0xFF;
	eMem[zAddr  + i*2    ] = (z >> 8) & 0xFF;
	eMem[zAddr  + i*2 + 1] = (z >> 0) & 0xFF;
    }
}

void TREESETfunc(Register r1, Register r2, Register r3, Immediate imm) {
    TreeLeaf	*t;
    int		i;

    CheckRegister(r1);
    CheckRegister(r2);
    CheckRegister(r3);

    xTree = reg[r1];
    yTree = reg[r2];

    t = tree;
    for (i = 0; i < NUM_TREELEAVES; i++) {
	t->I = ~(r3 >> (NUM_TREELEAVES - 1 - i)) & 1;
	t->O = (r3 >> (NUM_TREELEAVES - 1 - i)) & 1;
	t++;
    }
}

void TREEINfunc(Register r1, Register r2, Register r3, Immediate imm) {
    int		i;
    Word	tmp;
    TreeLeaf	*t;

    tmp = reg[r1];
    t = tree;
    for (i = 0; i < NUM_TREELEAVES; i++) {
	t->I &= (tmp >> 31) | (tmp == 0);
	t->O |= (tmp >> 31);
	tmp += reg[r2];
	t++;
    }

    reg[r1] = tmp;
}

void TREEVALfunc(Register r1, Register r2, Register r3, Immediate imm) {
    int		i;
    Word	tmp, val;
    TreeLeaf	*t;

    tmp = reg[r1];
    t = tree;
    for (i = 0; i < NUM_TREELEAVES; i++) {
	val = tmp + 0x8000;  /* Round to nearest integer */
	switch (imm) {
	    case 0:
		t->Z = val >> 16;
		/* Maybe compare Z here */
		break;
	    case 1:
		t->col = (t->col & 0x07FF) | ((val>> 5) & 0xF800);
		break;
	    case 2:
		t->col = (t->col & 0xF81F) | ((val >> 11) & 0x07E0);
		break;
	    case 3:
		t->col = (t->col & 0xFFE0) | ((val >> 16) & 0x001F);
		break;
	}
	tmp += reg[r2];
	t++;
    }

    reg[r1] = tmp;
}

void TREEPUTfunc(Register r1, Register r2, Register r3, Immediate imm) {
    int		i, zval;
    TreeLeaf	*t;

    t = tree;
    for (i = 0; i < NUM_TREELEAVES; i++) {
	zval = eMem[zAddr + (yTree*width + xTree + i)*2] << 8 |
	    eMem[zAddr + (yTree*width + xTree + i)*2 + 1];
	if (t->Z > zval) {
	    t->I = 0;
	    t->O = 1;
	}
	if (t->I || !t->O) {
	    eMem[zAddr + (yTree*width + xTree + i)*2] = t->Z >> 8;
	    eMem[zAddr + (yTree*width + xTree + i)*2 + 1] = t->Z & 0xFF;
	    eMem[fbAddr + (yTree*width + xTree + i)*2] = t->col >> 8;
	    eMem[fbAddr + (yTree*width + xTree + i)*2 + 1] = t->col & 0xFF;
	}
	t++;
    }
}

OpFunction opTable[] = {
#include "optable.h"
};

#define NUM_OPCODES	(sizeof (opTable) / sizeof (opTable[0]))

void DecodeInstruction(Word inst, Opcode *op, Register *r1,
		       Register *r2, Register *r3, Immediate *imm) {
    *op = inst >> 26;
    CheckOpcode(*op);
    
    *r1 = (inst >> 21) & 31;
    *r2 = (inst >> 16) & 31;
    *r3 = (inst >> 11) & 31;
    *imm = (Immediate)(signed long)(signed short)(inst & 0xFFFF);
}

int numOps = NUM_OPCODES;
