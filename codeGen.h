#ifndef __CODEGEN__
#define __CODEGEN__

#include "parser.h"

extern void freeRegister();

// Evaluate the syntax tree
extern int evaluateTree(BTNode* root);

// Print the syntax tree in prefix
extern void printPrefix(BTNode *root);

#endif // __CODEGEN__
