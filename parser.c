#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "parser.h"
#include "codeGen.h"

int sbcount = 0;
Symbol table[TBLSIZE];

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
        for (int i = 0;i < 3;i++) {
            printf("MOV r%d [%d]\n", i, 4 * i);
        }
		printf("EXIT 0\n");
        exit(0);
    } else if (match(END)) {
        advance();
    } else {
        retp = assign_expr();
        freeRegister();

        if (match(END)) {
            printPrefix(retp);
            printf("\n");
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
