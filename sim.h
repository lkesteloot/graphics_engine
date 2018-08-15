#ifndef _SIM_H_
#define _SIM_H_

#include "types.h"

#define NUM_WORDS		1024
#define NUM_REGISTERS		32
#define NUM_TREELEAVES		16
#define NUM_RAM			(2*1024*1024)   /* Two megs for RAM	*/

#define MAXFUNCTIONS		64

#define MAXFILES		16	/* Max number of source files	*/
#define MAXLINES		1024	/* Max lines per source file	*/
#define MAXSTR			256	/* Good string length		*/
#define MAXREGNAMES		256	/* Maximum different reg names	*/

/* Values for the bp[] array and for "stopped": */
#define	GOING			0	/* Not stopped			*/
#define	USERBREAK		1	/* User-set breakpoint here	*/
#define	TMPBREAK		2	/* Temporary breakpoint here	*/
#define	CONTROLC		4	/* Hit Control-C		*/
#define ERROR			8	/* Error while executing	*/

typedef void (* OpFunction)(Register, Register, Register, Immediate);
typedef struct {
    Word	I:1, O:1;
    Halfword	Z, col;
} TreeLeaf;
typedef struct {
    int		file;
    int		line;
    int		regs[NUM_REGISTERS];
} AddrInfo;
typedef struct Symbol {
    int			addr;
    int			filetype;
    char		name[64];
    struct Symbol	*next;
} Symbol;

#define ClearStatus()	status = 0
#define SetStatus(a)    status = ((a) == 0) | (((a) >> 31) << 1)
#define CheckZ() ((status >> 0) & 1)
#define CheckN() ((status >> 1) & 1)
#define SetZ() (status |= 1)
#define SetN() (status |= 2)

#define CheckOpcode(o)						\
    if ((o) >= NUM_OPCODES) {					\
	sprintf(error, "Error: illegal opcode (%d)\n", (o));	\
	stopped = ERROR;					\
    }

#define CheckRegister(r)					\
    if ((r) > NUM_REGISTERS) {					\
	sprintf(error, "Error: illegal register (%d)\n",(int) (r));	\
	stopped = ERROR;					\
    }

#define CheckAddress(a)						\
    if ((a) > NUM_WORDS) {					\
	sprintf(error, "Error: illegal address (%d)\n", (int)(a));	\
	stopped = ERROR;					\
    }

extern Word iMem[NUM_WORDS], dMem[NUM_WORDS];
extern Byte eMem[NUM_RAM];
extern Word reg[NUM_REGISTERS];
extern Word status, PC;
extern TreeLeaf tree[NUM_TREELEAVES];
extern Word xTree, yTree, fbAddr, zAddr, width, height;

extern Word mult32x32 (Word A, Word B);

extern char error[];
extern int stopped;

#endif /* _SIM_H_ */
