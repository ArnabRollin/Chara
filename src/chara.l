%{
#include "parser.tab.h"
%}

%%
[ \t\r\n]+  { /* skip all whitespace */ }

":"             { return TOK_COLON; }
"="             { return TOK_EQUALS; }
";"             { return TOK_SEMICOLON; }
"do"            { return TOK_DO; }
"end"           { return TOK_END; }
"("             { return TOK_LPAREN; }
")"             { return TOK_RPAREN; }

\"[^\"]*\"          { 
    yytext[strlen(yytext) - 1] = '\0'; 
    yylval.str = strdup(yytext + 1); 
    return TOK_STRING; 
}

    /* Matches letters/numbers (main, out, etc) */
[a-zA-Z_][a-zA-Z0-9_]* { 
    yylval.str = strdup(yytext); 
    return TOK_IDENT; 
}

[0-9]+      { yylval.num = atoi(yytext); return TOK_NUMBER; }
.           { printf("Unknown char: %s\n", yytext); }

%%

int yywrap() { return 1; }
