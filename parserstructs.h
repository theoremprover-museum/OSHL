/*
 *      Author: Hao Xu
 */

#ifndef PARSERSTRUCTS_H
#define PARSERSTRUCTS_H
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "region.h"
#include "hashtable.h"

#define MAX_TOKEN_TEXT_LEN 1024

enum NodeType {
    TK_EOS,
    N_ERROR,
    TK_INT,
    TK_DOUBLE,
    TK_TEXT,
    TK_STRING,
    TK_BOOL,
    TK_BACKQUOTED,
    TK_OP,
    TK_MISC_OP,
    N_TUPLE,
    N_APPLICATION,
    N_UNPARSED
};

struct Node {
    NodeType nodeType; /* node type */
    int degree;
    int ival;
    char *text;
    long expr;
    Node **subtrees;
    char *base;
    double dval;
    long lval;
};

struct Label {
    long exprloc;
    char *base;
};

struct Token {
    NodeType type;
    char text[MAX_TOKEN_TEXT_LEN+1];
    int vars[100];
    long exprloc;
};


Node *newNode(NodeType type, char* text, Label * exprloc, Region *r);

#endif
