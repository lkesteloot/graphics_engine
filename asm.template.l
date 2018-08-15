%option noyywrap

%{
#define ASM_L
#include "asm.h"
%}

Wspace		[ \t]+
Alpha		[A-Za-z_]
Digit		[0-9]
OctDigit	[0-7]
HexDigit	[0-9a-fA-F]
Name		{Alpha}({Alpha}|{Digit})*
HexNumber       0[xX]{HexDigit}+
OctNumber       0{OctDigit}*
DecNumber	[1-9]{Digit}*
Number		{DecNumber}|{HexNumber}|{OctNumber}
Register	[Rr]{Digit}{1,2}

%{
extern YYSTYPE yylval;
extern int yylineno;
%}

%%

^#{Wspace}{Number}{Wspace}["][^\"]*["]{Wspace}*\n { /* handle cpp lines */
    int fnstart, fnlen=0;
    sscanf(yytext, "# %d \"%n", &yylineno, &fnstart);
    sscanf(yytext+fnstart, "%*[^\"]%n", &fnlen);

    (void) memcpy(currentFile, yytext+fnstart, fnlen);
    currentFile[fnlen] = '\0';
} 

^#{Wspace}{Number}\n { /* handle cpp line number changes */
    sscanf(yytext, "# %d", &yylineno);
}

{Wspace}	{}

"\n"		yylineno++;

";".*$		{} /* remove comments */

>>>include LEXRULES

[-+,()*/%|&]	return *yytext;

"\.sym"		return tSYM;
"\.unsym"	return tUNSYM;
"\.resym"	return tRESYM;
"\.const"	return tCONST;
"\.unconst"	return tUNCONST;
"\.code"	return tCODE;
"\.data"	return tDATA;
"\.word"	return tWORD;
"\.space"	return tSPACE;

{Number}	{
                    yylval.i = strtol(yytext, (char **) NULL, 0);
		    return tNUMBER;
                };

{Register}	{
		    yylval.i = strtol (yytext + 1, NULL, 0);
		    return tREGISTER;
		};

{Name}:         {
	  	    yylval.s = strdup(yytext);
		    yylval.s[strlen(yylval.s)-1] = 0;
		    return tLABEL;
                };
{Name}		{
		    Symbol *p = LookupSymbol(yytext);
		    if (p != NULL) {
			yylval.symbol = p;
			switch (p->type) {
			case REGISTER_SYMBOL:
			    return tREGISTER_SYMBOL;
			case CONST_SYMBOL:
			    return tCONST_SYMBOL;
			case ADDRESS_SYMBOL:
			    return tADDRESS_SYMBOL;
			}
		    } else {
			yylval.s = strdup(yytext);
			return tNAME;
		    }
		};

%%
