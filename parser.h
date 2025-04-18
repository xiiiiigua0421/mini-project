#ifndef __PARSER__
#define __PARSER__

#include "lex.h"
#define TBLSIZE 64

// Call this macro to print error message and exit the program
// This will also print where you called it in your program
#define error(errorNum) { \
    err(errorNum); \
}

// Error types
typedef enum {
    UNDEFINED, MISPAREN, NOTNUMID, NOTFOUND, RUNOUT, NOTLVAL, DIVZERO, 
	SYNTAXERR, UNDEFVAR, REDEFINITION, NOTASSIGN, NOTEXPR, NOTSTMT
} ErrorType;

// Structure of the symbol table
typedef struct {
    int val;
    char name[MAXLEN];
} Symbol;

// Structure of a tree node
typedef struct _Node {
    TokenSet data;
    int val;
    char lexeme[MAXLEN];
    struct _Node *left; 
    struct _Node *right;
} BTNode;

// The symbol table
extern Symbol table[TBLSIZE];

// Initialize the symbol table with builtin variables
extern void initTable(void);

// Get the value of a variable
extern int getval(char *str);

// Set the value of a variable
extern int setval(char *str, int val);

// Make a new node according to token type and lexeme
extern BTNode *makeNode(TokenSet tok, const char *lexe);

// Free the syntax tree
extern void freeTree(BTNode *root);

extern BTNode *factor(void);
extern BTNode *term(void);
extern BTNode *term_tail(BTNode *left);
extern BTNode *expr(void);
extern BTNode *expr_tail(BTNode *left);
extern void statement(void);

extern BTNode* assign_expr(void);
extern BTNode* orexpr(void);

// Print error message and exit the program
extern void err(ErrorType errorNum);

extern int getvariable(char* str);
extern int setvariable(char* str);

#endif // __PARSER__
