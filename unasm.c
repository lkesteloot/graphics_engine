
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

#include "unasm.h"

Inst instlist[] = {
#include "inst.h"
};
int numinst = sizeof (instlist) / sizeof (instlist[0]);

static char *addr (unsigned long r, long imm) {
    static char		buf[256];

    if (r == 0) {
	sprintf (buf, "%ld", imm);
    } else if (imm == 0) {
	sprintf (buf, "R%lu", r);
    } else {
	sprintf (buf, "R%lu(%ld)", r, imm);
    }

    return buf;
}

char *unasm (unsigned long inst) {
    unsigned long	op, r1, r2, r3;
    long		imm;
    static char		buf[256], format[128];

    op = inst >> 26;
    r1 = (inst >> 21) & 31;
    r2 = (inst >> 16) & 31;
    r3 = (inst >> 11) & 31;
    imm = (signed long)(signed short)(inst & 0xFFFF);

    if (op >= numinst) {
	sprintf (buf, "Invalid opcode: %lu", op);
	return buf;
    }

    if (strcmp (instlist[op].format, "OP") == 0) {
	format[0] = '\0';
    } else if (strcmp (instlist[op].format, "R") == 0) {
	sprintf (format, "R%ld", r1);
    } else if (strcmp (instlist[op].format, "RR") == 0) {
	sprintf (format, "R%ld,R%ld", r1, r2);
    } else if (strcmp (instlist[op].format, "RRR") == 0) {
	sprintf (format, "R%ld,R%ld,R%ld", r1, r2, r3);
    } else if (strcmp (instlist[op].format, "RRI") == 0) {
	sprintf (format, "R%ld,R%ld,%ld", r1, r2, imm);
    } else if (strcmp (instlist[op].format, "RA") == 0) {
	sprintf (format, "R%ld,%s", r1, addr (r2, imm));
    } else if (strcmp (instlist[op].format, "AR") == 0) {
	sprintf (format, "%s,R%ld", addr (r2, imm), r1);
    } else if (strcmp (instlist[op].format, "A") == 0) {
	sprintf (format, "%s", addr (r2, imm));
    } else {
	sprintf (format, "(unknown format: %s)", instlist[op].format);
    }

    sprintf (buf, "(0x%08lx) %-8s %s", inst, instlist[op].op, format);

    return buf;
}
