%{
#define ASM_Y
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include "asm.h"
#include "util.h"

#define OP(op) (((op) - tLD) << 26)
#define R(op, r1) ((((op) - tLD) << 26) | ((r1) << 21))
#define RR(op, r1, r2) ((((op) - tLD) << 26) | ((r1) << 21) | ((r2) << 16))
#define RRR(op, r1, r2, r3) \
    ((((op) - tLD) << 26) | ((r1) << 21) | ((r2) << 16) | ((r3) << 11))
#define RRI(op, r1, r2, imm) \
    ((((op) - tLD) << 26) | ((r1) << 21) | ((r2) << 16) | ((imm) & 0xFFFF))
#define RA(op, r1, r2, imm) RRI(op, r1, r2, imm)
#define A(op, r2, imm) ( (((op) - tLD) << 26) | ((r2) << 16) | ((imm) & 0xFFFF))

#define YYDEBUG 1

#define NUM_LINESTACK	4

void yyerror (char *s, ...);
int yylex ();

int yylineno;
unsigned int op;
int imemf, dmemf;
FILE *outs;
Symbol *symbolTable;
FileType curfile;
Unresolved *unresolvedSymbols;
int currentIAddress, currentDAddress;

int linestack[NUM_LINESTACK], linehead, linetail;
%}

%start AsmLines

>>>include YACCTOKENS

%token  tSYM tUNSYM tRESYM tCONST tUNCONST tCODE tDATA tWORD tSPACE

%token <i> tNUMBER
%token <i> tREGISTER
%token <s> tNAME
%token <s> tLABEL
%token <symbol> tREGISTER_SYMBOL
%token <symbol> tCONST_SYMBOL
%token <symbol> tADDRESS_SYMBOL
%type <a> Address
%type <i> Immediate
%type <i> Register

%left '|'
%left '&'
%left '+' '-'
%left '*' '/' '%'
%left UMINUS UPLUS

%%

AsmLines	: /* Empty */
		| AsmLines AsmLine
		;

AsmLine		: tLABEL
                    {
			if (curfile == CODE) {
			    InsertSymbol($1, ADDRESS_SYMBOL, currentIAddress);
			} else {
			    InsertSymbol($1, ADDRESS_SYMBOL, currentDAddress);
			}
		    }
		| AsmInst
                | SymbolDefine
                | SymbolUndefine
                | SymbolRedefine
                | ConstDefine
                | ConstUndefine
                | Data
                | Code
                | Word
                | Space
		| tNAME
		    {
			yyerror ("Invalid opcode: %s", $1);
		    }
		;

>>>include YACCRULES

Address		: Register '(' Immediate ')'
		    {
			$$.reg = $1;
			$$.imm = $3;
		    }
		| Register
		    {
			$$.reg = $1;
			$$.imm = 0;
		    }
		| Immediate
		    {
			$$.reg = 0;
			$$.imm = $1;
		    }

Register	: tREGISTER_SYMBOL
                    {
			$$ = $1->imm;
		    }
                | tREGISTER
                    {
			$$ = $1;
		    }
		;


Immediate	: '(' Immediate ')'
		    {
			$$ = $2;
		    }
		| Immediate '+' Immediate
		    {
			$$ = $1 + $3;
		    }
		| Immediate '-' Immediate
		    {
			$$ = $1 - $3;
		    }
		| Immediate '*' Immediate
		    {
			$$ = $1 * $3;
		    }
		| Immediate '/' Immediate
		    {
			$$ = $1 / $3;
		    }
		| Immediate '%' Immediate
		    {
			$$ = $1 % $3;
		    }
		| Immediate '&' Immediate
		    {
			$$ = $1 & $3;
		    }
		| Immediate '|' Immediate
		    {
			$$ = $1 | $3;
		    }
		| '-' Immediate %prec UMINUS
		    {
			$$ = - $2;
		    }
		| '+' Immediate %prec UPLUS
		    {
			$$ = - $2;
		    }
		| tCONST_SYMBOL
                    {
			$$ = $1->imm;
		    }
                | tADDRESS_SYMBOL
                    {
			$$ = $1->imm;
		    }
                | tNAME
                    {
			AddUnresolved($1);
			$$ = 0;
		    }
                | tNUMBER
                ;

SymbolDefine	: tSYM tNAME ',' Register
                    {
			InsertSymbol($2, REGISTER_SYMBOL, $4);
			fprintf(outs, "NAME: %s = R%d\n", $2, $4);
		    }
                ;

SymbolUndefine	: tUNSYM tREGISTER_SYMBOL
                    {
			RemoveSymbol($2->name, REGISTER_SYMBOL);
			fprintf(outs, "UNAME: R%d\n", $2->imm);
		    }
                ;

SymbolRedefine	: tRESYM tNAME ',' tREGISTER_SYMBOL
                    {
			int reg = RemoveSymbol($4->name, REGISTER_SYMBOL);
			fprintf(outs, "UNAME: R%d\n", $4->imm);
			InsertSymbol($2, REGISTER_SYMBOL, reg);
			fprintf(outs, "NAME: %s = R%d\n", $2, reg);
		    }
                ;

ConstDefine	: tCONST tNAME ',' Immediate
                    {
			InsertSymbol($2, CONST_SYMBOL, $4);
		    }
                ;

ConstUndefine	: tUNCONST tCONST_SYMBOL
                    {
			RemoveSymbol($2->name, CONST_SYMBOL);
		    }
                ;

Data		: tDATA
		    {
			curfile = DATA;
		    }
		;

Code		: tCODE
		    {
			curfile = CODE;
		    }
		;

Word		: tWORD Immediate
		    {
			if (curfile != DATA) {
			    yyerror ("Cannot use .word in code space.");
			}
			WriteData ($2);
		    }
		;

Space		: tSPACE tNUMBER
		    {
			int	i;

			if (curfile != DATA) {
			    yyerror ("Cannot use .space in code space.");
			}
			for (i = 0; i < $2; i++) {
			    WriteData (0);
			}
		    }
		;

%%

static char currentFile[256];

#include "lex.yy.c"

void die (void)
{
    fprintf (stderr, "Removing output file.\n");
    close (imemf);
    close (dmemf);
    fclose (outs);
    unlink ("imem");
    unlink ("dmem");
    unlink ("imem.sym");
    exit (1);
}

void yyerror (char *format, ...)
{
    va_list	argptr;

    fprintf(stderr,"%s: Error at line %d: ", currentFile, yylineno);
    va_start (argptr, format);
    vfprintf (stderr, format, argptr);
    va_end (argptr);
    fprintf(stderr, "\n");

    die ();
}

void PushLine (int line) {
    linestack[linehead] = line;
    linehead = (linehead + 1) % NUM_LINESTACK;
}

int PopLine (void) {
    int		line;

    line = linestack[linetail];
    linetail = (linetail + 1) % NUM_LINESTACK;

    return line;
}

void WriteInstruction (unsigned int op)  {
    SwapWords(&op, 1);
    write (imemf, &op, sizeof (int));
    fprintf(outs, "INST: %d:%d:%s\n", currentIAddress, PopLine (), currentFile);
    currentIAddress++;
}

void WriteData (unsigned int w) {
    SwapWords(&w, 1);
    write (dmemf, &w, sizeof (int));
    currentDAddress++;
}

Symbol *LookupSymbol (char *name) {
    Symbol *p = symbolTable;

    while (p) {
	if (strcmp(name, p->name) == 0)
	    break;
	p = p->next;
    }

    return p;
}

void InsertSymbol (char *name, SymbolType type, int imm) {
    Symbol *p = LookupSymbol(name);

    if (p != NULL) {
	yyerror ("%s multiply defined.", name);
    } else {
	p = (Symbol *) malloc(sizeof(Symbol));
	p->name = strdup(name);
	p->type = type;
	p->imm = imm;
	p->filetype = curfile;
	p->next = symbolTable;
	symbolTable = p;
    }
}
    
int RemoveSymbol(char *name, SymbolType type) {
    int		reg;

    Symbol *p = symbolTable;
    Symbol *q = NULL;
    
    while (p != NULL && strcmp(name, p->name) != 0) {
	q = p;
	p = p->next;
    }

    if (p != NULL) {
	if (p->type != type) {
	    yyerror("Symbol %s of the wrong type.", name);
	}
	reg = p->imm;
	if (q != NULL) {
	    q->next = p->next;
	} else {
	    symbolTable = p->next;
	}
	free(p->name);
	free(p);
    } else {
	yyerror("%s not defined.", name);
    }

    return reg;
}

void AddUnresolved(char *name) {
    Unresolved *u = (Unresolved *) malloc(sizeof(Unresolved));

    u->name = name;
    if (curfile == CODE) {
	u->address = currentIAddress;
    } else {
	u->address = currentDAddress;
    }
    u->file = strdup(currentFile);
    u->line = yylineno;
    u->filetype = curfile;
    u->next = unresolvedSymbols;
    unresolvedSymbols = u;
}

void ResolveSymbols(void) {
    Unresolved *u;

    for (u = unresolvedSymbols; u; u = u->next) {
	Symbol *s = LookupSymbol(u->name);
	if (s == NULL) {
	    strcpy(currentFile, u->file);
	    yylineno = u->line;
	    yyerror("Unresolved symbol: %s", u->name);
	} else if (s->type != ADDRESS_SYMBOL) {
	    strcpy(currentFile, u->file);
	    yylineno = u->line;
	    yyerror("Expecting address symbol");
	}
	if (u->filetype == CODE) {
	    short imm;
	    imm = s->imm;
	    lseek(imemf, u->address * 4 + 2, SEEK_SET);
            SwapHalfword((Halfword *)&imm);
	    write(imemf, &imm, 2);
	} else {
	    int imm;
	    imm = s->imm;
	    lseek(dmemf, u->address * 4, SEEK_SET);
            SwapWords((Word *)&imm, 1);
	    write(dmemf, &imm, 4);
	}
    }
}

void DumpSymbolTable (void) {
    Symbol *p;

    for (p = symbolTable; p; p = p->next) {
	if (p->type == ADDRESS_SYMBOL) {
	    fprintf (outs, "SYM: %d:%d:%s\n", p->imm, p->filetype, p->name);
	}
    }
}

static char cpp_filter[256] = "/usr/bin/cpp";

int parse_options(int argc, char *argv[]) {
    int arglen;
    
    while(--argc > 0) {
	arglen = strlen(*++argv);

	/* grab cpp options */
	if (strncmp(*argv, "-I", 2) == 0 ||
	    strncmp(*argv, "-D", 2) == 0 ||
	    strncmp(*argv, "-U", 2) == 0 ||
	    strncmp(*argv, "-C", 2) == 0 ||
	    strncmp(*argv, "-P", 2) == 0) {
	    strcat(strcat(cpp_filter, " "), *argv);
	}

	/* output file name -- reroute stdout */
/*	else if (strncmp(*argv, "-output", arglen) == 0) {
	    if (--argc <= 0)
		return 1;
	    freopen(*++argv, "w", stdout);
	}*/

	/* want usage?  just return with error */
	else if (strncmp(*argv, "-help", arglen) == 0)
	    return 1;

	/* input file (must be last, so break out of the loop) */
	else {
	    strcat(strcat(cpp_filter, " "), *argv);
	    strcpy(currentFile, *argv);
	    argc--;
	    break;
	}
    }

    /* Did we handle all of the args? */
    if (argc > 0)
	return 1;
    else
	return 0;
}

static void Usage(char *argv[]) {
    fprintf(stderr, "Usage: %s [options] [input file]\n", argv[0]);
    fprintf(stderr, "  -help\n");
    fprintf(stderr, "  cpp -I, -D, -U, -C, or -P options\n");
}

int main (int argc, char *argv[])
{
    int i;
    yylineno = 1;
    symbolTable = NULL;
    unresolvedSymbols = NULL;
    currentIAddress = 0;
    currentDAddress = 0;
    linehead = 0;
    linetail = 0;

    strcpy (currentFile, "stdin");

    if (parse_options(argc, argv)) {
	Usage(argv);
	exit(1);
    }

    /* pipe input through cpp */
    yyin = popen(cpp_filter, "r");

    imemf = open ("imem", O_CREAT | O_WRONLY | O_TRUNC, 0666);
    if (imemf == -1) {
	perror ("imem");
	exit (1);
    }

    dmemf = open ("dmem", O_CREAT | O_WRONLY | O_TRUNC, 0666);
    if (dmemf == -1) {
	perror ("dmem");
	exit (1);
    }

    curfile = CODE;

    outs = fopen("imem.sym", "w");
    if (outs == NULL) {
	perror ("imem.sym");
	exit (1);
    }

    i = yyparse( );

    ResolveSymbols ();
    DumpSymbolTable ();

    close (imemf);
    close (dmemf);
    fclose (outs);
    
    return i;
}

