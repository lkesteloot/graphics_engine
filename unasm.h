char *unasm (unsigned long inst);

typedef struct Inst {
    char *op;
    char *format;
} Inst;

extern Inst instlist[];
