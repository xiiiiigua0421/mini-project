#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>


// for lex
#define MAXLEN 256

// Token types
typedef enum {
    UNKNOWN, END, ENDFILE,
    INT, ID,
    UNARY,
    ADDSUB, MULDIV,
    AND,XOR,OR,
	ASSIGN, ADDSUB_ASSIGN,
    LPAREN, RPAREN
} TokenSet;

TokenSet getToken(void);
TokenSet curToken = UNKNOWN;
char lexeme[MAXLEN];

// Test if a token matches the current token
int match(TokenSet token);
// Get the next token
void advance(void);
// Get the lexeme of the current token
char *getLexeme(void);


// for parser
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

int sbcount = 0;
Symbol table[TBLSIZE];

// Initialize the symbol table with builtin variables
void initTable(void);
// Get the value of a variable
int getval(char *str);
// Set the value of a variable
int setval(char *str, int val);
// Make a new node according to token type and lexeme
BTNode *makeNode(TokenSet tok, const char *lexe);
// Free the syntax tree
void freeTree(BTNode *root);
BTNode *factor(void);
BTNode *term(void);
BTNode *term_tail(BTNode *left);
BTNode *expr(void);
BTNode *expr_tail(BTNode *left);
void statement(void);

BTNode* assign_expr(void);
BTNode* orexpr(void);

// Print error message and exit the program
void err(ErrorType errorNum);


// for codeGen
// Evaluate the syntax tree
int evaluateTree(BTNode *root);
// Print the syntax tree in prefix
void printPrefix(BTNode *root);


/*============================================================================================
lex implementation
============================================================================================*/

TokenSet getToken(void) {
    int i = 0;
    char c = '\0';
    while ((c = fgetc(stdin)) == ' ' || c == '\t');

    if (isdigit(c)) {
        lexeme[0] = c;
        c = fgetc(stdin);
        i = 1;
        while (isdigit(c) && i < MAXLEN) {
            lexeme[i] = c;
            ++i;
            c = fgetc(stdin);
        }
        ungetc(c, stdin);
        lexeme[i] = '\0';
        return INT;
    }
    else if (c == '+' || c == '-') {
        lexeme[0] = c;
		c = fgetc(stdin);
		if (c == '=') {
			lexeme[1] = c;
			lexeme[2] = '\0';
			return ADDSUB_ASSIGN;
		} else if (c == lexeme[0]) {
			lexeme[1] = c;
			lexeme[2] = '\0';
			return UNARY;
        } else {
            ungetc(c, stdin);
            lexeme[1] = '\0';
            return ADDSUB;
        }
    }
    else if (c == '*' || c == '/') {
        lexeme[0] = c;
        lexeme[1] = '\0';
        return MULDIV;
    } else if (c == '\n') {
        lexeme[0] = '\0';
        return END;
    } else if (c == '=') {
        strcpy(lexeme, "=");
        return ASSIGN;
    } else if (c == '(') {
        strcpy(lexeme, "(");
        return LPAREN;
    } else if (c == ')') {
        strcpy(lexeme, ")");
        return RPAREN;
    }
    else if (isalpha(c)) {
        lexeme[0] = c;
        c = fgetc(stdin);
        i = 1;
        while ((isalpha(c) || isdigit(c) || c=='_') && i < MAXLEN) {
            lexeme[i] = c;
            ++i;
            c = fgetc(stdin);
        }
        ungetc(c, stdin);
        lexeme[i] = '\0';
        return ID;
	}
    else if (c == '&') {
        lexeme[0] = c;
        lexeme[1] = '\0';
        return AND;
	} else if (c == '|') {
		lexeme[0] = c;
		lexeme[1] = '\0';
		return OR;
	} else if (c == '^') {
		lexeme[0] = c;
		lexeme[1] = '\0';
		return XOR;
    }
    else if (c == EOF) {
        return ENDFILE;
    } else {
        return UNKNOWN;
    }
}

void advance(void) {
    curToken = getToken();
}

int match(TokenSet token) {
    if (curToken == UNKNOWN)
        advance();
    return token == curToken;
}

char *getLexeme(void) {
    return lexeme;
}

/*============================================================================================
parser implementation
============================================================================================*/

void initTable(void) {
    strcpy(table[0].name, "x");
    table[0].val = 0;
    strcpy(table[1].name, "y");
    table[1].val = 0;
    strcpy(table[2].name, "z");
    table[2].val = 0;
    sbcount = 3;
}

int getvariable(char* str) {
    for (int i = 0; i < sbcount; i++) {
        if (strcmp(table[i].name, str) == 0) return i;
    }
    return -1;
}

int setvariable(char* str) {
    strcpy(table[sbcount].name, str);
    return sbcount++;
}

int existvariable(BTNode* node) {
    if (node) {
		if (node->data == ID) return 1;
		return existvariable(node->left) || existvariable(node->right);
    }
	return 0;
}

int evalValue(BTNode* root) {
    int lval, rval;
    switch (root->data) {
    case INT:
        return atoi(root->lexeme);

    case OR:
    case XOR:
    case AND:
    case ADDSUB:
    case MULDIV:
        lval = evalValue(root->left);
        rval = evalValue(root->right);

        if (strcmp(root->lexeme, "+") == 0) return lval + rval;
        else if (strcmp(root->lexeme, "-") == 0) return lval - rval;
        else if (strcmp(root->lexeme, "*") == 0) return lval * rval;
        else if (strcmp(root->lexeme, "/") == 0) {
            if (rval == 0) error(DIVZERO);
            return lval / rval;
        }
        else if (strcmp(root->lexeme, "|") == 0) return lval | rval;
        else if (strcmp(root->lexeme, "&") == 0) return lval & rval;
        else if (strcmp(root->lexeme, "^") == 0) return lval ^ rval;
        break;

    default:
		error(NOTNUMID);
        break;
    }
}

BTNode *makeNode(TokenSet tok, const char *lexe) {
    BTNode *node = (BTNode*)malloc(sizeof(BTNode));
    strcpy(node->lexeme, lexe);
    node->data = tok;
    node->val = 0;
    node->left = NULL;
    node->right = NULL;
    return node;
}

void freeTree(BTNode *root) {
    if (root != NULL) {
        freeTree(root->left);
        freeTree(root->right);
        free(root);
    }
}

// factor := INT | ADDSUB INT |
//		   	 ID  | ADDSUB ID  |
//		   	 ID ASSIGN expr |
//		   	 LPAREN expr RPAREN |
//		   	 ADDSUB LPAREN expr RPAREN
BTNode *factor(void) {
    BTNode *retp = NULL, *left = NULL;

    if (match(INT)) {
        retp = makeNode(INT, getLexeme());
        advance();
    } else if (match(ID)) {
        left = makeNode(ID, getLexeme());
        advance();
        retp = left;
    }
    else if (match(ADDSUB)) {
        retp = makeNode(ADDSUB, getLexeme());
        retp->left = makeNode(INT, "0");
        advance();
        if (match(INT)) {
            retp->right = makeNode(INT, getLexeme());
            advance();
        } else if (match(ID)) {
            retp->right = makeNode(ID, getLexeme());
            advance();
        } else if (match(LPAREN)) {
            advance();
			retp->right = assign_expr();
            if (match(RPAREN))
                advance();
            else
                error(MISPAREN);
        } else {
            error(NOTNUMID);
        }
    } else if (match(LPAREN)) {
        advance();
        retp = assign_expr();
        if (match(RPAREN))
            advance();
        else
            error(MISPAREN);
    } else {
        error(NOTNUMID);
    }
    return retp;
}

BTNode* unary() {
    if (match(UNARY)) {
        BTNode* node = makeNode(UNARY, getLexeme());
        advance();
        if (match(ID)) {
            node->left = unary();
            node->right = makeNode(INT, "1");
            return node;
        }
        else error(NOTNUMID);
    } else return factor();
}

// term := factor term_tail
BTNode *term(void) {
    BTNode *node = unary();
    return term_tail(node);
}

// term_tail := MULDIV factor term_tail | NiL
BTNode* term_tail(BTNode* left) {
    BTNode* node = NULL;

    if (match(MULDIV)) {
        node = makeNode(MULDIV, getLexeme());
        advance();
        node->left = left;
        node->right = unary();
        if (!existvariable(node->right) && evalValue(node->right) == 0)
			error(DIVZERO);
        return term_tail(node);
    }
    else return left;
}

// expr_tail := ADDSUB term expr_tail | NiL
BTNode* expr_tail(BTNode* left) {
    BTNode* node = NULL;

    if (match(ADDSUB)) {
        node = makeNode(ADDSUB, getLexeme());
        advance();
        node->left = left;
        node->right = term();
        return expr_tail(node);
    }
    else {
        return left;
    }
}

// expr := term expr_tail
BTNode* expr(void) {
    BTNode* node = term();
    return expr_tail(node);
}

BTNode* andexpr_tail(BTNode* left) {
    BTNode* node = NULL;

    if (match(AND)) {
        node = makeNode(AND, getLexeme());
        advance();
        node->left = left;
        node->right = expr();
        return andexpr_tail(node);
    }
    else {
        return left;
    }
}

BTNode* andexpr(void) {
    BTNode* node = expr();
    return andexpr_tail(node);
}

BTNode* xorexpr_tail(BTNode* left) {
    BTNode* node = NULL;

    if (match(XOR)) {
        node = makeNode(XOR, getLexeme());
        advance();
        node->left = left;
		node->right = andexpr();
        return xorexpr_tail(node);
    }
    else {
        return left;
    }
}

BTNode* xorexpr(void) {
	BTNode* node = andexpr();
    return xorexpr_tail(node);
}

BTNode* orexpr_tail(BTNode* left) {
    BTNode* node = NULL;

    if (match(OR)) {
        node = makeNode(OR, getLexeme());
        advance();
        node->left = left;
		node->right = xorexpr();
        return orexpr_tail(node);
    }
    else {
        return left;
    }
}

BTNode* orexpr(void) {
	BTNode* node = xorexpr();
    return orexpr_tail(node);
}

BTNode* assign_expr() {
	BTNode* retp = NULL, *left=orexpr();//assume not =
    if (left->data == ID && (match(ASSIGN) || match(ADDSUB_ASSIGN))) {
        retp = makeNode(match(ASSIGN) ? ASSIGN : ADDSUB_ASSIGN, getLexeme());
        advance();
        retp->left = left;
        retp->right = assign_expr();
	} else retp = left;
	return retp;
}

// statement := ENDFILE | END | expr END
void statement(void) {
    BTNode *retp = NULL, *variable=NULL;

    if (match(ENDFILE)) {
        for (int i = 0;i < 3;i++) printf("MOV r%d [%d]\n", i, 4 * i);
		printf("EXIT 0\n");
        exit(0);
    } else if (match(END)) {
        advance();
    } else {
        retp = assign_expr();
        freeRegister();

        if (match(END)) {
            /*printPrefix(retp);
            printf("\n");*/
            evaluateTree(retp);
            freeTree(retp);
            advance();
        }
        else error(SYNTAXERR);
    }
}

void err(ErrorType errorNum) {
	printf("EXIT 1\n");
    exit(0);
}

/*============================================================================================
codeGen implementation
============================================================================================*/

int nowregister = 0;

void resetRegister() { nowregister = 0; }
int allocateRegister() { return nowregister++; }
void freeRegister() { if (nowregister > 0) nowregister--; }

int evaluateTree(BTNode* root) {
    int reg = -1, lreg, rreg, varidx;
    switch (root->data) {
    case ID:
		varidx = getvariable(root->lexeme);
        if (varidx != -1) {
            reg = allocateRegister();
            printf("MOV r%d [%d]\n", reg, 4*varidx);  // load variable
        } else error(UNDEFVAR);
        break;

    case INT:
        reg = allocateRegister();
        printf("MOV r%d %s\n", reg, root->lexeme);    // load constant
        break;

    case ASSIGN:
		varidx = getvariable(root->left->lexeme);
        if (varidx == -1) {
            varidx = setvariable(root->left->lexeme);
        }
        rreg = evaluateTree(root->right);
        printf("MOV [%d] r%d\n", 4*varidx, rreg); // store value
        reg = rreg; // result kept in rreg
        break;

	case ADDSUB_ASSIGN:
    case UNARY:
        varidx = getvariable(root->left->lexeme);
        if (varidx != -1) {
            rreg = evaluateTree(root->right);
            lreg = allocateRegister();
            printf("MOV r%d [%d]\n", lreg, 4 * varidx);  // load variable
            printf("%s r%d r%d\n", root->lexeme[0] == '+' ? "ADD" : "SUB", lreg, rreg);
            printf("MOV [%d] r%d\n", 4 * varidx, lreg); // store value
            printf("MOV r%d r%d\n", rreg, lreg); // copy value
            reg = rreg; // result kept in rreg
            freeRegister();
        } else error(UNDEFVAR);
        break;

    case OR:
    case XOR:
    case AND:
    case ADDSUB:
    case MULDIV:
        lreg = evaluateTree(root->left);
        rreg = evaluateTree(root->right);

        if (strcmp(root->lexeme, "+") == 0)
            printf("ADD r%d r%d\n", lreg, rreg);
        else if (strcmp(root->lexeme, "-") == 0)
            printf("SUB r%d r%d\n", lreg, rreg);
        else if (strcmp(root->lexeme, "*") == 0)
            printf("MUL r%d r%d\n", lreg, rreg);
        else if (strcmp(root->lexeme, "/") == 0)
            printf("DIV r%d r%d\n", lreg, rreg);
        else if (strcmp(root->lexeme, "|") == 0)
            printf("OR r%d r%d\n", lreg, rreg);
        else if (strcmp(root->lexeme, "&") == 0)
            printf("AND r%d r%d\n", lreg, rreg);
        else if (strcmp(root->lexeme, "^") == 0)
            printf("XOR r%d r%d\n", lreg, rreg);

        freeRegister();   // free rreg
        reg = lreg;       // result stays in lreg
        break;

    default: // handle error or noop
        break;
    }
    return reg;
}

void printPrefix(BTNode *root) {
    if (root != NULL) {
        printf("%s ", root->lexeme);
        printPrefix(root->left);
        printPrefix(root->right);
    }
}

/*============================================================================================
main
============================================================================================*/

// This package is a calculator
// It works like a Python interpretor
// Example:
// >> y = 2
// >> z = 2
// >> x = 3 * y + 4 / (2 * z)
// It will print the answer of every line
// You should turn it into an expression compiler
// And print the assembly code according to the input

// This is the grammar used in this package
// You can modify it according to the spec and the slide
// statement  :=  ENDFILE | END | expr END
// expr    	  :=  term expr_tail
// expr_tail  :=  ADDSUB term expr_tail | NiL
// term 	  :=  factor term_tail
// term_tail  :=  MULDIV factor term_tail| NiL
// factor	  :=  INT | ADDSUB INT |
//		   	      ID  | ADDSUB ID  |
//		   	      ID ASSIGN expr |
//		   	      LPAREN expr RPAREN |
//		   	      ADDSUB LPAREN expr RPAREN

int main() {
    initTable();
    while (1) {
        statement();
    }
    return 0;
}
