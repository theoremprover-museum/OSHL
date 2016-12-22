/*
 *      Author: Hao Xu
 */


#ifndef PARSERGEN_H
#define PARSERGEN_H

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "parserstructs.h"
#include "region.h"
#include "linkedList.h"

#define POINTER_BUF_SIZE 1024
#define ERR_MSG_LEN 1024
#define NODE_STACK_SIZE 256 * 1024
#define INC_MOD(x,m) (x) = (((x)+1)%(m))
#define DEC_MOD(x,m) (x) = (((x)+(m)-1)%(m))

#define CONCAT2_EVAL(x,y) x##y
#define CONCAT2(x,y) CONCAT2_EVAL(x,y)

typedef struct op {
    char* string;
    int arity;
    int prec;
} Op;

#define num_ops 30
extern Op new_ops[];

struct Pointer {
	FILE *fp; /* file */
	char buf[POINTER_BUF_SIZE]; /* buffer */
	unsigned int len; /* len of string in buf */
	unsigned int p; /* pointer to next char in buf */
	unsigned long fpos; /* position of the beginning of the buffer in file */
    unsigned int strp; /* pointer to next char in strbuf */
    const char *strbuf; /* string buffer */
    unsigned int strbuflen;
    int isFile;
    char *base; /* f + filename without extension, or s + source */
    Pointer(const char *string);
    Pointer(FILE* fp, const char *source);
    ~Pointer();
private:
    void initPointer(const char *string);
    void initPointer(FILE *fp, const char *source);
} ;

#define pushRule(rs, r) ((rs)->rules[(rs)->len++] = (r))


struct ParserContext {
    Node *nodeStack[NODE_STACK_SIZE];
    int nodeStackTop;
    int stackTopStack[NODE_STACK_SIZE];
    int stackTopStackTop;
    int error;
    Node *errnode;
    Label errloc;
    char errmsgbuf[ERR_MSG_LEN];
    LinkedList<char *> *errmsg;
    Region *region;
    Token tokenQueue[1024];
    int tqp;
    int tqtop;
    int tqbot;
    ParserContext(Region *r);
    ~ParserContext();
};

#define PUSHNS(n) (context->nodeStack[(context->nodeStackTop)++] = n)
#define POPNS (context->nodeStack[--(context->nodeStackTop)])
#define NEXT_TOKEN \
{ \
    FPOS; \
    token = nextTokenRuleGen(e, context, 0); \
    if(token->type==N_ERROR) { \
        context->error=1; \
        if(pos.exprloc > context->errloc.exprloc) context->errloc = pos; \
        break;\
    } \
}
#define TOKEN_TYPE(t) (token->type == (t))
#define TOKEN_TEXT(str) (strcmp(token->text, (str))==0)
#define PUSHBACK pushback(e, token, context)
#define FPOS (getFPos(&pos, e, context))
#define UPDATE_ERR_LOC if(FPOS->exprloc > context->errloc.exprloc) {context->errloc = *FPOS;}
#define CASCADE(x) \
{\
        Node *_ncascade = (x); \
        if(_ncascade == NULL || _ncascade->nodeType == N_ERROR) { \
                UPDATE_ERR_LOC; \
                context->error = 1; \
                break;\
        } else { \
                PUSH(_ncascade); \
        } \
}
#define BUILD_NODE(type,cons,loc,deg, consume) \
        if(context->error==0){Node *var = newNode((type), (cons), (loc), context->region); \
            if(deg!=0) { \
                Node **subs = setDegree(var, (deg), context->region); \
                int counter; \
                for(counter = 1;counter <= (deg);counter ++) {\
                    subs[(deg)-counter] = context->nodeStack[context->nodeStackTop-counter];\
                } \
            } \
            context->nodeStackTop -= (consume); \
            CASCADE(var);}

#define BUILD_APP_NODE(cons,loc,deg) \
		BUILD_NODE(N_TUPLE, TUPLE, loc, deg, deg); \
		BUILD_NODE(TK_TEXT, cons, loc, 0, 0); \
		SWAP; \
		BUILD_NODE(N_APPLICATION, APPLICATION, loc, 2, 2); \

#define PARSER_FUNC_PROTO(l) \
void CONCAT2(parse, l)(Pointer* e, ParserContext *context)
#define PARSER_FUNC_PROTO1(l, p) \
void CONCAT2(parse, l)(Pointer* e, ParserContext *context, p)
#define PARSER_FUNC_PROTO2(l, p, q) \
void CONCAT2(parse, l)(Pointer* e, ParserContext *context, p, q)
#define PARSER_BEGIN(l) \
    Label start; \
    Label pos; \
    Token *token; (void)token; \
    skipWhitespace(e); \
    getFPos(&start, (e), context); \
    do {

#define PARSER_FUNC_BEGIN(l) \
PARSER_FUNC_PROTO(l) { \
    PARSER_BEGIN(l)
#define PARSER_FUNC_BEGIN1(l, p) \
PARSER_FUNC_PROTO1(l, p) { \
    PARSER_BEGIN(l)
#define PARSER_FUNC_BEGIN2(l, p, q) \
PARSER_FUNC_PROTO2(l, p, q) { \
    PARSER_BEGIN(l)

#define PARSER_FUNC_END(l) \
    PARSER_END(l) \
}
#define PARSER_END(l) \
	} while(0);

#define SWAP \
{\
    Node *node = POPNS;\
    Node *node2 = POPNS;\
    PUSHNS(node);\
    PUSHNS(node2);\
}
#define UNZIP(n) \
{ \
    int i; \
    Node *node[1024]; \
    for(i=0;i<n;i++) { \
        node[i] = context->nodeStack[context->nodeStackTop - 2*(n-i) + 1]; \
        context->nodeStack[context->nodeStackTop-2*n+i] = context->nodeStack[context->nodeStackTop-2*n+2*i]; \
    } \
    for(i=0;i<n;i++) { \
        context->nodeStack[context->nodeStackTop-n+i] = node[i]; \
    } \
}

#define TTEXT(x) \
    NEXT_TOKEN; \
    if(!((TOKEN_TYPE(TK_TEXT)||TOKEN_TYPE(TK_OP)||TOKEN_TYPE(TK_MISC_OP)) && TOKEN_TEXT(x))) { \
        context->error = 1; \
        /*printf("expected token %s, but encountered %s, at %s:%d\n", x, token->text, __FILE__, __LINE__); */\
        if(pos.exprloc > context->errloc.exprloc) context->errloc = pos; \
        break; \
    }
#define TTEXT2(x,y) \
    NEXT_TOKEN; \
    if(!((TOKEN_TYPE(TK_TEXT)||TOKEN_TYPE(TK_OP)||TOKEN_TYPE(TK_MISC_OP)) && (TOKEN_TEXT(x)||TOKEN_TEXT(y)))) { \
        context->error = 1; \
        /*printf("expected token %s or %s, but encountered %s, at %s:%d\n", x, y, token->text, __FILE__, __LINE__); */\
        if(pos.exprloc > context->errloc.exprloc) context->errloc = pos; \
        break; \
    }
#define TTEXT3(x,y,z) \
    NEXT_TOKEN; \
    if(!((TOKEN_TYPE(TK_TEXT)||TOKEN_TYPE(TK_OP)||TOKEN_TYPE(TK_MISC_OP)) && (TOKEN_TEXT(x)||TOKEN_TEXT(y)||TOKEN_TEXT(z)))) { \
        context->error = 1; \
        if(pos.exprloc > context->errloc.exprloc) context->errloc = pos; \
        break; \
    }
#define TTEXT_LOOKAHEAD(x) \
    TTEXT(x); \
    PUSHBACK;
#define TTYPE(x) \
    NEXT_TOKEN; \
    if(!TOKEN_TYPE(x)) { \
        context->error = 1; \
        if(pos.exprloc > context->errloc.exprloc) context->errloc = pos; \
        break; \
    }
#define TTYPE_LOOKAHEAD(x) \
    TTYPE(x); \
    PUSHBACK;

#define NO_SYNTAX_ERROR (context->error == 0)
#define CHECK_ERROR \
if(context->error!=0) { \
    break; \
}

#define NT(x) \
CONCAT2(parse, x)(e, context); \
CHECK_ERROR;

#define NT1(x, p) \
CONCAT2(parse, x)(e, context, p); \
CHECK_ERROR;

#define NT2(x, p, q) \
CONCAT2(parse, x)(e, context, p, q); \
CHECK_ERROR;

#define CHOICE_BEGIN(l) \
if(context->error==0) { \
    Label CONCAT2(l,Start); \
    int CONCAT2(l,Finish) = 0; \
    int CONCAT2(l,TokenQueueP) = context->tqp; \
    getFPos(&CONCAT2(l,Start), e, context); \
    context->stackTopStack[context->stackTopStackTop++] = context->nodeStackTop;

#define CHOICE_END(l) \
    (context->stackTopStackTop)--; \
    if(!CONCAT2(l,Finish)) { \
        UPDATE_ERR_LOC; \
        context->error = 1; \
        break;\
    } \
}

#define BRANCH_BEGIN(l) \
if(!CONCAT2(l,Finish)) { \
    do { \
    	context->tqp = CONCAT2(l,TokenQueueP); \
        context->nodeStackTop = context->stackTopStack[context->stackTopStackTop-1]; \
        context->error = 0;

#define BRANCH_END(l) \
        if(context->error == 0) \
            CONCAT2(l,Finish) = 1; \
    } while(0);\
}

#define TRY(l) \
CHOICE_BEGIN(l) \
BRANCH_BEGIN(l)

#define OR(l) \
BRANCH_END(l) \
BRANCH_BEGIN(l)

#define FINALLY(l) \
BRANCH_END(l) \
{ \
    do {

#define END_TRY(l) \
BRANCH_END(l) \
CHOICE_END(l)

#define ABORT(x) \
if(x) { \
        context->error = 1; \
        break;\
}

#define OPTIONAL_BEGIN(l) \
CHOICE_BEGIN(l); \
BRANCH_BEGIN(l);

#define OPTIONAL_END(l) \
BRANCH_END(l); \
BRANCH_BEGIN(l); \
BRANCH_END(l); \
CHOICE_END(l);

#define LOOP_BEGIN(l) \
int CONCAT2(done, l) = 0; \
while(!CONCAT2(done, l) && NO_SYNTAX_ERROR) {

#if defined(solaris_platform)
#define LOOP_END(l) \
} \
if(!CONCAT2(done, l)) { \
    break; \
}

#define DONE(l) \
CONCAT2(done, l) = 1;

#else
#define LOOP_END(l) \
} \
CONCAT2(exit, l): \
if(!CONCAT2(done, l)) { \
    break; \
}

#define DONE(l) \
CONCAT2(done, l) = 1; \
goto CONCAT2(exit, l);
#endif

/** utility functions */

Token *nextTokenRuleGen(Pointer* expr, ParserContext* pc, int rulegen);
int nextString(Pointer *e, char *value, int vars[]);
int nextString2(Pointer *e, char *value, int vars[]);
int eol(char ch);
int isOp(char *token);
int isUnaryOp(Token* token);
int getUnaryPrecedence(Token* token);
int isBinaryOp(Token *token);
int getBinaryPrecedence(Token* token);
void getCoor(Pointer *p, Label * errloc, int coor[2]);
int getLineRange(Pointer *p, int line, long range[2]);

/**
 * skip a token of type TK_TEXT, TK_OP, or TK_MISC_OP and text text, token will has type N_ERROR if the token does not match
 */
int skip(Pointer *expr, char *text, Token **token, ParserContext *pc, int rulegen);
void skipWhitespace(Pointer *expr);
char *findLineCont(char *expr);

void pushback(Pointer *e, Token *token, ParserContext *pc);
void initPointer(Pointer *p, FILE* fp, char* ruleBaseName);
void initPointer2(Pointer *p, char* buf);
Pointer *newPointer(FILE* buf, char *ruleBaseName);
Pointer *newPointer2(char* buf);
void deletePointer(Pointer* buf);

void skipComments(Pointer *e);
int nextChar(Pointer *p);
int lookAhead(Pointer *p, unsigned int n);

char* trim(char* str);
void trimquotes(char *string);

Label *getFPos(Label *label, Pointer *p, ParserContext *context);
void clearBuffer(Pointer *p);
void seekInFile(Pointer *p, unsigned long x);
void nextChars(Pointer *p, int len);

void syncTokenQueue(Pointer *e, ParserContext *context);

int dupLine(Pointer *p, Label * start, int n, char *buf);
int dupString(Pointer *p, Label * start, int n, char *buf);

int nextStringBase(Pointer *e, char *value, const char* delim, int consumeDelim, char escape, int vars[]);
int nextStringBase(Pointer *e, char *value, const char* delim);

void printTree(Node *n, int indent);
void printIndent(int indent);

void generateErrMsgFromFile(char *msg, long errloc, char *ruleBaseName, char* ruleBasePath, char errbuf[ERR_MSG_LEN]);
void generateErrMsgFromSource(char *msg, long errloc, char *src, char errbuf[ERR_MSG_LEN]);
void generateErrMsgFromPointer(char *msg, Label *l, Pointer *e, char errbuf[ERR_MSG_LEN]);
char *generateErrMsg(char *msg, long errloc, char* ruleBaseName, char errbuf[ERR_MSG_LEN]);
void generateAndAddErrMsg(char *msg, Node *node, int errcode, LinkedList<char *> *errmsg);

#endif
