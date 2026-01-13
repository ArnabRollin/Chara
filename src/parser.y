%{
#include <stdio.h>
#include "ast.h"
extern int yylex();
void yyerror(const char* s);
ASTNode* root;
%}

%union {
    char* str;
    int num;           /* Add this for numbers */
    struct ASTNode* node;
}

%token <str> TOK_IDENT TOK_STRING
%token <num> TOK_NUMBER  /* We need a number token */
%token TOK_COLON TOK_EQUALS TOK_SEMICOLON TOK_DO TOK_END TOK_LPAREN TOK_RPAREN

%type <node> program functions function statements statement call args arg

%%

/* The top-level rule now accepts a list of functions */
program:
    functions { root = $1; }
    ;

/* This allows 1 or more functions */
functions:
    function { $$ = $1; }
    | functions function { 
        ASTNode* current = $1;
        /* Use a dedicated 'next' pointer so you don't break the return value! */
        while (current->next) current = current->next;
        current->next = $2;
        $$ = $1;
    }
    ;

function:
    TOK_COLON TOK_IDENT TOK_EQUALS TOK_DO statements TOK_END TOK_NUMBER 
    { 
        ASTNode* ret_node = create_node(NODE_NUM, NULL, NULL, NULL);
        ret_node->int_value = $7; 
        
        /* node->left: body statements */
        /* node->right: return value node */
        /* node->next: next function in list (ADD THIS TO YOUR STRUCT) */
        $$ = create_node(NODE_FUNC, $2, $5, ret_node); 
    }
    ;

statements:
    statement { $$ = $1; }
    | statements statement { 
        ASTNode* curr = $1;
        while(curr->right) curr = curr->right;
        curr->right = $2;
        $$ = $1;
    }
    ;

statement:
    call TOK_SEMICOLON
    { $$ = $1; }

    | TOK_IDENT TOK_EQUALS arg TOK_SEMICOLON {
        $$ = create_node(NODE_VAR, $1, $3, NULL);
    }
    ;

call:
    TOK_IDENT args
    {
        /* NODE_CALL:
           - name = function name
           - left = argument list (linked)
           - right = NULL
        */
        $$ = create_node(NODE_CALL, $1, $2, NULL);
    }
    ;

args:
    /* empty */
    { $$ = NULL; }

    | args arg
    {
        if (!$1) {
            $$ = $2;
        } else {
            ASTNode* curr = $1;
            while (curr->next) curr = curr->next;
            curr->next = $2;
            $$ = $1;
        }
    }
    ;

arg:
    TOK_STRING
    {
        $$ = create_node(NODE_STR, $1, NULL, NULL);
    }
    | TOK_NUMBER
    {
        $$ = create_node(NODE_NUM, NULL, NULL, NULL);
        $$->int_value = $1;
    }
    | TOK_LPAREN call TOK_RPAREN { $$ = $2; }
    | TOK_IDENT
    {
        $$ = create_node(NODE_IDENT, $1, NULL, NULL);
    }
    ;

%%

void yyerror(const char* s) { fprintf(stderr, "Error: %s\n", s); }
