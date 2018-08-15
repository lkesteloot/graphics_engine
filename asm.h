#ifndef ASM_H
#define ASM_H

struct Address {
    int reg, imm;
};

typedef enum  {REGISTER_SYMBOL, CONST_SYMBOL, ADDRESS_SYMBOL} SymbolType;
typedef enum { CODE, DATA } FileType;

typedef struct Symbol {
    SymbolType type;
    char *name;
    int imm;
    FileType filetype;
    struct Symbol *next;
} Symbol;

typedef struct Unresolved {
    char *name;
    char *file;
    int line;
    int address;
    FileType filetype;
    struct Unresolved *next;
} Unresolved;

typedef union {
    char *s;
    int i;
    struct Address a;
    Symbol *symbol;
} YYSTYPE;
#define YYSTYPE_IS_DECLARED

void WriteInstruction(unsigned int op);
void WriteData(unsigned int w);

void PushLine (int line);

Symbol *LookupSymbol(char *name);
void InsertSymbol(char *name, SymbolType type, int imm);
int RemoveSymbol(char *name, SymbolType type);

void AddUnresolved(char *name);
void ResolveSymbols(void);
#endif // ASM_H

