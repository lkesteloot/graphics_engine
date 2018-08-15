#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/stat.h>
#include <curses.h>
#include <time.h>
#undef reg
#include "sim.h"
#include "unasm.h"
#include "display.h"
#include "functions.h"
#include "util.h"

Word iMem[NUM_WORDS], dMem[NUM_WORDS];
Byte eMem[NUM_RAM];
Word reg[NUM_REGISTERS];
Word status, PC;
TreeLeaf tree[NUM_TREELEAVES];
Word xTree, yTree, fbAddr, zAddr, width, height;
char error[128];
int stopped;

static char *program;
static AddrInfo addrinfo[NUM_WORDS];
static int bp[NUM_WORDS];

static char files[MAXFILES][MAXSTR];
static int numfiles, displayfile, displayline;

static char regnames[MAXREGNAMES][MAXSTR];
static int numregnames;
static int curreg[NUM_REGISTERS];

static char curfile[MAXLINES][MAXSTR];
static int curlines, curmapping[MAXLINES];

static int update;

static int programSize, dataSize, programShow;

static struct stat statimem;

static Symbol *symbols, *dSymbol[NUM_WORDS];
static int opCounts[MAXFUNCTIONS];
static int opTotal = 0;

static int gBatch = 0;

WINDOW	*srcwin, *regwin, *outwin;
int srcsize, regsize, outsize;

Word mult32x32 (Word A, Word B)
{
    Word	A1, A2, B1, B2, t1, t2, t3, t4, res;
    int		sign;

    sign = (A >> 31) ^ (B >> 31);

    if ((long)A < 0) {
        A = -A;
    }
    if ((long)B < 0) {
        B = -B;
    }

    A1 = A >> 16;
    A2 = A & 0xFFFF;
    B1 = B >> 16;
    B2 = B & 0xFFFF;

    t1 = B2 * A2;
    t2 = B2 * A1;
    t3 = B1 * A2;
    t4 = B1 * A1;

    res = (t1 >> 16) + t2 + t3 + (t4 << 16);

    if (sign) {
        return -res;
    } else {
        return res;
    }
}

void DumpFrequency() {
    int i;
    double max = 0;
    FILE *f, *g;

    mvwprintw (outwin, outsize - 1, 0,
	"Dumping instruction frequencies to \"freq.dat\"...");
    wclrtoeol (outwin);
    wrefresh (outwin);
    f = fopen ("freq.dat", "w");
    if (f == NULL) {
	perror ("freq.dat");
	exit (1);
    }
    g = fopen ("freq.plot", "w");
    if (g == NULL) {
	perror ("freq.plot");
	exit (1);
    }

    for (i = 0; i < numOps; i++) {
	fprintf(f, "%d\n", opCounts[i]);
	if (opCounts[i] > max) {
	    max = opCounts[i];
	}
    }

    mvwprintw (outwin, outsize - 1, 0,
	"Dumping gnuplot instructions to \"freq.plot\"...");
    wclrtoeol (outwin);
    wrefresh (outwin);
    max /= -60;
    fprintf(g, "set boxwidth .6\n");
    for (i = 0; i < numOps; i++) {
	fprintf(g, "set label \"%s\" at %d, %g center\n", instlist[i].op,
		i, max); 
    }
    fprintf(g, "set noxtics\n");
    fprintf(g, "set xlabel \"Instructions\"\n");
    fprintf(g, "set ylabel \"# of uses\"\n");
    fprintf(g, "plot 'freq.dat' with boxes\n");
    fprintf(g, "pause -1 \"Press <return> to exit:\"");

    fclose(f);
    fclose(g);
}

void DisplayMemory (char type) {
    /* Type can be 'e' or 'd' */

    Word	i, j;

    if (type == 'd') {
	for (i = 0; i < (srcsize - 1) * 4; i++) {
	    if (dSymbol[i] != NULL) {
		mvwprintw (srcwin, i / 4, i % 4 * 20, "%10s: %08lX",
		    dSymbol[i]->name, dMem[i]);
	    } else {
		mvwprintw (srcwin, i / 4, i % 4 * 20, "  %8X: %08lX",
		    i, dMem[i]);
	    }
	}
	leftstatus (srcwin, srcsize - 1, " Data memory");
    } else {
	for (i = 0; i < srcsize - 1; i++) {
	    mvwprintw (srcwin, i, 0, "%08X:", i * 16);
	    wclrtoeol (srcwin);
	    for (j = 0; j < 16; j++) {
		mvwprintw (srcwin, i, 11 + j * 3 + j / 4, "%02X ",
		    eMem[i*16 + j]);
	    }
	    wclrtoeol (srcwin);
	}
	leftstatus (srcwin, srcsize - 1, " External memory");
    }
    wrefresh(srcwin);
}

void DumpMemory(char type) {
    /* Type can be 'E' or 'D' */

    int i, max, f;

    if (type == 'D') {
	for (i = NUM_WORDS - 1; i >= 0; i--) {
	    if (dMem[i] != 0) {
		break;
	    }
	}
	max = i + 1;
    } else {
	mvwprintw (outwin, outsize - 1, 0,
	    "Dumping external memory to \"emem.dump\"...");
	wclrtoeol (outwin);
	wrefresh (outwin);
	f = open ("emem.dump", O_WRONLY | O_CREAT | O_TRUNC, 0666);
	if (f == -1) {
	    perror ("emem.dump");
	    exit (1);
	}

	i = write (f, eMem, sizeof (eMem));
	if (i == -1) {
	    perror ("emem.dump");
	}

	close (f);

	wmove (outwin, outsize - 1, 0);
	mvwprintw (outwin, outsize - 1, 0,
	    "Dumped external memory to \"emem.dump\".");
	wclrtoeol (outwin);
	wrefresh (outwin);
    }
}

void UpdateSource (void) {
    int		i, line;
    int		addr;
    char	*s, *end;

    for (i = 0; i < srcsize - 1; i++) {
	line = programShow + i;
	wmove (srcwin, i, 0);
	if (line >= 0 && line < curlines) {
	    addr = curmapping[line];
	    if (addr != -1) {
		if (bp[addr] & USERBREAK) {
		    wattron (srcwin, A_BOLD);
		}
	    }
	    if (line == addrinfo[PC].line) {
		s = curfile[line];
		end = strchr (s, ';');
		if (end == NULL) {
		    end = s + strlen (s) - 1;
		} else {
		    end--;
		    while (end > s && (*end == ' ' || *end == '\t')) {
			end--;
		    }
		}
		while (*s == ' ' || *s == '\t') {
		    waddch (srcwin, *s++);
		}
		wattron (srcwin, A_UNDERLINE);
		while (s <= end) {
		    waddch (srcwin, *s++);
		}
		wattroff (srcwin, A_UNDERLINE);
		wprintw (srcwin, "%s", s);
	    } else {
		wprintw (srcwin, "%s", curfile[line]);
	    }
	    wattroff (srcwin, A_BOLD);
	}
	wclrtoeol (srcwin);
    }
    wrefresh (srcwin);
}

void UpdateRegisters (void) {
    int		i;
    FILE	*f;
    struct stat	statsrc;

    if (gBatch) {
        return;
    }

    for (i = 0; i < NUM_REGISTERS; i++) {
	mvwprintw(regwin, i % 8, i / 8 * 20 + 10, ": %08lX ",
	    reg[i]);
    }
    leftstatus (regwin, regsize - 1, " Registers");
    rightstatus (regwin, regsize - 1, "Z: %ld  N: %ld  PC: 0x%04lx",
	CheckZ(), CheckN(), PC);
    if (addrinfo[PC].file != displayfile) {
	if (addrinfo[PC].file == -1) {
	    wclear (srcwin);
	    leftstatus (srcwin, srcsize - 1, " No source");
	    curlines = 0;
	} else {
	    wclear (srcwin);
	    leftstatus (srcwin, srcsize - 1, " File %s",
		files[addrinfo[PC].file]);
	    for (i = 0; i < MAXLINES; i++) {
		curmapping[i] = -1;
	    }
	    f = fopen (files[addrinfo[PC].file], "r");
	    if (f == NULL) {
		mvwprintw (srcwin, srcsize / 2, 5,
		    "Source file \"%s\" not found", files[addrinfo[PC].file]);
		curlines = 0;
	    } else {
		if (fstat (fileno (f), &statsrc) == 0) {
		    if (statsrc.st_mtime > statimem.st_mtime) {
			wstandout (outwin);
			mvwprintw (outwin, 7, 0, "WARNING: Source file \"%s\" "
			    "is more recent than imem file",
			    files[addrinfo[PC].file]);
			wstandend (outwin);
			wrefresh (outwin);
		    }
		}
		curlines = 0;
		while (fgets (curfile[curlines], MAXSTR, f) != NULL) {
		    curfile[curlines][strlen (curfile[curlines]) - 1] = '\0';
		    for (i = 0; i < NUM_WORDS; i++) {
			if (addrinfo[i].file == addrinfo[PC].file &&
			    addrinfo[i].line == curlines) {
			    curmapping[curlines] = i;
			    break;
			}
		    }
		    curlines++;
		    if (curlines == MAXLINES) {
			wholestatus (outwin, outsize - 1,
			    "Too many lines in source file");
			break;
		    }
		}
		fclose (f);
	    }
	}
	displayfile = addrinfo[PC].file;
	displayline = -1;
    }
    if (addrinfo[PC].line != displayline) {
	if (addrinfo[PC].line != -1) {
	    UpdateSource ();
	}
	rightstatus (srcwin, srcsize - 1, "Line %d   ",
	    addrinfo[PC].line + 1);
	for (i = 0; i < NUM_REGISTERS; i++) {
	    mvwprintw(regwin, i % 8, i / 8 * 20,
		"%10s", regnames[addrinfo[PC].regs[i]]);
	}
	wrefresh (srcwin);
	mvwprintw (outwin, 5, 0, "0x%08lx: %s\n", PC, unasm (iMem[PC]));
	wrefresh (outwin);
	displayline = addrinfo[PC].line;
    }

    wrefresh (regwin);
}

void Step (void) {
    Opcode op;
    Register r1, r2, r3;
    Immediate immediate;

    DecodeInstruction(iMem[PC], &op, &r1, &r2, &r3, &immediate);
    PC++;
    opCounts[op]++;
    opTotal++;
    opTable[op](r1, r2, r3, immediate);
    reg[0] = 0;
    programShow = addrinfo[PC].line - srcsize / 2;
}

void Continue (void) {
    if (!gBatch) {
        mvwprintw (outwin, outsize - 1, 0, "Continuing...");
        wclrtoeol (outwin);
        wrefresh (outwin);
    }

    stopped = GOING;
    while (PC < programSize && !stopped) {
	Step ();
	if (update) {
	    UpdateRegisters ();
	}
	if (bp[PC]) {
	    stopped = bp[PC];
	}
    }

    if (!gBatch) {
        switch (stopped) {
            case GOING:
                mvwprintw (outwin, outsize - 1, 0, "Program ran off end of memory");
                break;
            case USERBREAK:
                mvwprintw (outwin, outsize - 1, 0, "Hit breakpoint");
                break;
            case TMPBREAK:
                mvwprintw (outwin, outsize - 1, 0, "Hit next instruction");
                break;
            case CONTROLC:
                mvwprintw (outwin, outsize - 1, 0, "Program interrupted");
                break;
            case ERROR:
                wstandout (outwin);
                mvwprintw (outwin, outsize - 1, 0, "ERROR: %s", error);
                wstandend (outwin);
                break;
        }
        wclrtoeol (outwin);
        wrefresh (outwin);
    }
}

void LoadSymbol (char *fn) {
    FILE	*f;
    char	str[512], name[64], file[512];
    int		addr, line, i, reg, filetype;

    f = fopen (fn, "r");
    if (f == NULL) {
	perror (fn);
	exit (1);
    }

    while (fgets (str, 512, f)) {
	str[strlen (str) - 1] = '\0';  /* Remove \n */
	if (sscanf (str, "NAME: %s = R%d", name, &reg) == 2) {
	    for (i = 0; i < numregnames; i++) {
		if (strcmp (name, regnames[i]) == 0) {
		    break;
		}
	    }
	    if (i == numregnames) {
		if (numregnames == MAXREGNAMES) {
		    fprintf (stderr, "Too many register names (%d)\n",
			numregnames);
		    exit (1);
		}
		strcpy (regnames[i], name);
		numregnames++;
	    }
	    curreg[reg] = i;
	} else if (sscanf (str, "UNAME: R%d", &reg) == 1) {
	    curreg[reg] = reg;
	} else if (sscanf (str, "INST: %d:%d:%s", &addr, &line, file) == 3) {
	    for (i = 0; i < numfiles; i++) {
		if (strcmp (file, files[i]) == 0) {
		    break;
		}
	    }
	    if (i == numfiles) {
		if (numfiles == MAXFILES) {
		    fprintf (stderr, "Too many source files (%d)\n", numfiles);
		    exit (1);
		}
		strcpy (files[i], file);
		numfiles++;
	    }
	    addrinfo[addr].file = i;
	    addrinfo[addr].line = line - 1;
	    for (i = 0; i < NUM_REGISTERS; i++) {
		addrinfo[addr].regs[i] = curreg[i];
	    }
	} else if (sscanf (str, "SYM: %d:%d:%s", &addr, &filetype, name) == 3) {
	    Symbol	*p;

	    p = (Symbol *)malloc (sizeof (Symbol));
	    p->addr = addr;
	    p->filetype = filetype;
	    strcpy (p->name, name);
	    p->next = symbols;
	    symbols = p;
	    if (filetype == 1) {  /* DATA */
		dSymbol[addr] = p;
	    }
	} else {
	    fprintf (stderr, "Invalid line in symbol table \"%s\": \"%s\".\n",
		fn, str);
	    exit (1);
	}
    }

    fclose (f);
}

void AddBreakpoint (void) {
    Byte	addrstr[64];
    Word	addr;
    Symbol	*p;

    getinput ((char *)addrstr, outwin, outsize - 1, 0, "Breakpoint address: ");
    if (addrstr[0] == '\0') {
	addr = PC;
    } else if (isdigit(addrstr[0])) {
	addr = strtol ((char *)addrstr, NULL, 0);
    } else {
	for (p = symbols; p; p = p->next) {
	    /* filetype == 0 for instructions */
	    if (p->filetype == 0 && strcmp (p->name, (char *)addrstr) == 0) {
		addr = p->addr;
		break;
	    }
	}
	if (p == NULL) {
	    wstandout (outwin);
	    mvwprintw (outwin, outsize - 1, 0, "Invalid symbol: \"%s\"",
		addrstr);
	    wstandend (outwin);
	    goto end;
	}
    }

    if (addr >= NUM_WORDS) {
	wstandout (outwin);
	mvwprintw (outwin, outsize - 1, 0,
	    "Invalid breakpoint address: 0x%08x", addr);
	wstandend (outwin);
    } else {
	bp[addr] ^= USERBREAK;
	mvwprintw (outwin, outsize - 1, 0, "Breakpoint toggled at 0x%08x",
	    addr);
    }
end:
    wclrtoeol (outwin);
    wrefresh (outwin);
}

void loademem (char *fn) {
    int		f;

    f = open (fn, O_RDONLY);
    if (f == -1) {
	perror (fn);
	exit (1);
    }

    read (f, eMem, NUM_RAM);

    close (f);
}

void showhelp (void) {
    wclear (srcwin);
    wmove (srcwin, 0, 0);
    wattron (srcwin, A_UNDERLINE);
    wprintw (srcwin, "Running code\n\n");
    wattroff (srcwin, A_UNDERLINE);
    wprintw (srcwin, "s                See instruction memory\n");
    wprintw (srcwin, "Enter            Step one instruction\n");
    wprintw (srcwin, "r                Reset the PC to 0\n");
    wprintw (srcwin, "c                Continue the program\n");
    wprintw (srcwin, "C                Same as c, but the source and register "
	    "windows are updated\n");
    wattron (srcwin, A_UNDERLINE);
    wprintw (srcwin, "\nViewing code and data\n\n");
    wattroff (srcwin, A_UNDERLINE);
    wprintw (srcwin, "d                See data memory\n");
    wprintw (srcwin, "e                See external memory\n");
    wprintw (srcwin, "b [location]     Set a breakpoint (default to PC)\n");
    wprintw (srcwin, "Arrows/PgUp/PgDn Scroll the source window\n");
    wprintw (srcwin, "^P,^N,^V         Scroll the source window\n");
    wprintw (srcwin, "q                Quit\n");
    centerstatus (srcwin, srcsize - 1, "Press SPACE to continue");
    wrefresh (srcwin);
    getchar ();
    displayline = -1;
    displayfile = -1; /* Make sure this window gets redrawn */
}

void sigint () {
    stopped = CONTROLC;
    signal (SIGINT, sigint);
}

void usage (void) {
    fprintf (stderr, "usage: sim [-r] [-d] [-f] [-s imem.sym] [-e emem] "
		    "imem dmem\n");
    fprintf (stderr, "   -r   Run program (non-interactive mode)\n");
    fprintf (stderr, "   -d   Dump contents of external memory after run\n");
    fprintf (stderr, "   -f   Dump frequency count after run\n");
    fprintf (stderr, "   -s   Load symbol table\n");
    fprintf (stderr, "   -e   Load external memory\n");
    exit (1);
}

int main(int argc, char *argv[]) {
    int fd, c, dump, freq, i, j, done;
    char *filename, escbuf[16];
    extern int optind;

    program = argv[0];
    gBatch = 0;
    dump = 0;
    freq = 0;
    for (i = 0; i < NUM_WORDS; i++) {
	addrinfo[i].file = -1;
	addrinfo[i].line = -1;
	for (j = 0; j < NUM_REGISTERS; j++) {
	    addrinfo[i].regs[j] = j;
	}
    }
    numfiles = 0;
    for (i = 0; i < NUM_REGISTERS; i++) {
	sprintf (regnames[i], "R%02d", i);
    }
    numregnames = NUM_REGISTERS;
    for (i = 0; i < NUM_REGISTERS; i++) {
	curreg[i] = i;
    }
    displayfile = -2;
    displayline = -2;
    update = 0;
    for (i = 0; i < NUM_WORDS; i++) {
	bp[i] = 0;
	dSymbol[i] = NULL;
    }
    symbols = NULL;
    reg[0] = 0;

    while ((c = getopt (argc, argv, "rdfs:e:")) != EOF) {
	switch (c) {
	    case 'r':
		gBatch = 1;
		break;
	    case 'd':
		dump = 1;
		break;
	    case 'f':
		freq = 1;
		break;
	    case 's':
		LoadSymbol (optarg);
		break;
	    case 'e':
		loademem (optarg);
		break;
	    case '?':
		usage ();
		break;
	}
    }

    if (optind + 2 != argc) {
	usage ();
    }

    filename = argv[optind];
    if ( (fd = open (filename, O_RDONLY)) == -1) {
	perror (argv[optind]);
	exit (1);
    }
    fstat (fd, &statimem);
    programSize = read(fd, iMem, NUM_WORDS * sizeof(Word)) / sizeof(Word);
    SwapWords(iMem, programSize);
    close (fd);

    optind++;

    filename = argv[optind];
    if ( (fd = open (filename, O_RDONLY)) == -1) {
	perror (argv[optind]);
	exit (1);
    }
    dataSize = read(fd, dMem, NUM_WORDS * sizeof(Word)) / sizeof(Word);
    SwapWords(dMem, dataSize);
    close (fd);

    PC = 0;

    if (gBatch) {
        clock_t beforeTime = clock();

	do {
            Continue ();
        } while (stopped != GOING);

        clock_t afterTime = clock();
        clock_t totalTime = afterTime - beforeTime;

	if (dump) {
	    DumpMemory ('E');
	}
	if (freq) {
	    DumpFrequency();
	}

        float seconds = (double)totalTime / CLOCKS_PER_SEC;
        printf("Executed %d instructions in %.2f seconds.\n", opTotal, seconds);
        printf("Effective clock speed is %d MHz.\n", (int)(opTotal / seconds / 1024 / 1024 + 0.5));

	return 0;
    }

    initscr ();

    nonl ();
    cbreak ();
    noecho ();

    regsize = 9;
    outsize = 10;
    srcsize = LINES - regsize - outsize;
    if (srcsize < 3) {
        fprintf (stderr, "Not enough lines on the display.\n");
        exit (1);
    }

    srcwin = newwin (srcsize, COLS, 0, 0);
    regwin = newwin (regsize, COLS, srcsize, 0);
    outwin = newwin (outsize, COLS, srcsize + regsize, 0);

    wprintw (outwin, "Program contains: %d instructions\n", programSize);
    wprintw (outwin, "Data contains: %d words\n", dataSize);

    mvwprintw (outwin, 3, 0, "Press (h) for help");

    wrefresh (outwin);

    signal (SIGINT, sigint);

    programShow = addrinfo[PC].line - srcsize / 2;

    UpdateRegisters ();
    done = 0;
    while (!done) {
	wmove (outwin, outsize - 1, 0);
	wrefresh (outwin);
	c = getchar ();
	wmove (outwin, outsize - 1, 0);
	wclrtoeol (outwin);
	wrefresh (outwin);
	switch (c) {
	    case 'h':
	    case 'H':
	    case '?':
		showhelp ();
		UpdateRegisters ();
		break;
	    case '\n':
	    case '\r':
		Step ();
		UpdateRegisters ();
		break;
	    case 'r':
		PC = 0;
		UpdateRegisters ();
		break;
	    case 'c':
	    case 'C':
		update = (c == 'C');
		Continue ();
		UpdateRegisters ();
		break;
	    case 'b':
		AddBreakpoint ();
		UpdateSource ();
		break;
	    case 'q':
		done = 1;
		break;
	    case 'd':
	    case 'e':
		DisplayMemory (c);
		break;
	    case 'D':
	    case 'E':
		DumpMemory (c);
		break;
	    case 's':
		UpdateSource ();
		break;
	    case '':
		programShow--;
		UpdateSource ();
		break;
	    case '':
		programShow++;
		UpdateSource ();
		break;
	    case '':
		programShow += srcsize;
		UpdateSource ();
		break;
	    case '\e':
		i = 0;
		do {
		    c = getchar ();
		    escbuf[i++] = c;
		} while (!isalpha (c) && c != '~');
		escbuf[i] = '\0';
		if (strcmp (escbuf, "[5~") == 0) {
		    programShow -= srcsize;
		    UpdateSource ();
		} else if (strcmp (escbuf, "[6~") == 0) {
		    programShow += srcsize;
		    UpdateSource ();
		} else if (strcmp (escbuf, "[A") == 0) {
		    programShow--;
		    UpdateSource ();
		} else if (strcmp (escbuf, "[B") == 0) {
		    programShow++;
		    UpdateSource ();
		} else {
		    mvwprintw (outwin, outsize - 2, 0,
			"Unknown escape sequence: ", escbuf);
		}
		break;
	    default:
		{ static int i = 0;
		mvwprintw (outwin, outsize - 2, i*4, "%d", c);
		i++;
		}
		break;
	}
    }

    resetterm ();

    return 0;
}
