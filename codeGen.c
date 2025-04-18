#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "codeGen.h"

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
