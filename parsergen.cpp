/*
 *      Author: Hao Xu
 */
#include <stdlib.h>
#include "parserstructs.h"
#include "parsergen.h"
#include "linkedList.h"

/***** utility functions *****/
ParserContext::ParserContext(Region *r) {
    stackTopStackTop = 0;
    nodeStackTop = 0;
    error = 0;
    errloc.exprloc = -1;
    region = r;
    errmsg = new LinkedList<char *>();
    errnode = NULL;
    errmsgbuf[0] = '\0';
    tqp = tqtop = tqbot = 0;
}
ParserContext::~ParserContext() {
	delete errmsg;
}

void skipWhitespace(Pointer *expr) {
	int ch;
	ch = lookAhead(expr, 0);
	while (ch!=-1 && (ch==' ' || ch=='\t' || ch=='\r' || ch=='\n')) {
		ch = nextChar(expr);
	}
}
void skipComments(Pointer *e) {
    int ch = lookAhead(e, 0);
    while(ch!='\n' && ch!=-1) {
        ch = nextChar(e);
    }
}
char *findLineCont(char *expr) {
	char *e = expr + strlen(expr);
	while (e!=expr) {
		e--;
		if(*e==' ' || *e=='\t' || *e=='\r' || *e=='\n') continue;
		if(*e=='\\' || *e==',' || *e==';' || *e=='|' || *e=='#') return e;
		break;
	}
	return NULL;
}

/**
 * skip a token of type TK_TEXT, TK_OP, or TK_MISC_OP and text text, token will have type N_ERROR if the token does not match
 * return 0 failure non 0 success
 */
int skip(Pointer *e, char *text, Token **token, ParserContext *pc, int rulegen) {
	*token = nextTokenRuleGen(e, pc, rulegen);
	if(((*token)->type!=TK_TEXT && (*token)->type!=TK_OP && (*token)->type!=TK_MISC_OP) || strcmp((*token)->text,text)!=0) {
		(*token)->type = N_ERROR;
		return 0;
	}
	return 1;
}

/*void getRuleCode(char buf[MAX_RULE_LEN], Pointer *e, int rulegen, ParserContext *context) {

}*/

Token* nextTokenRuleGen(Pointer* e, ParserContext *context, int rulegen) {
	//printf("next token %d\n", context->nodeStackTop);
	if(context->tqp != context->tqtop) {
		Token *token = &(context->tokenQueue[context->tqp]);
		INC_MOD(context->tqp, 1024);
		return token;
	}

	Token* token = &(context->tokenQueue[context->tqp]);
	INC_MOD(context->tqtop, 1024);
	if(context->tqbot == context->tqtop)
		INC_MOD(context->tqbot, 1024);
	context->tqp = context->tqtop;

    while (1) {
        skipWhitespace(e);
        Label start;
        Label pos;
        getFPos(&start, e, context);
        token->exprloc = start.exprloc;
        int ch = lookAhead(e, 0);
        if (ch == -1) { /* reach the end of stream */
            token->type = TK_EOS;
            strcpy(token->text, "EOS");
            break;
        } else {
            if (ch == '=' || ch == '{' || ch == '}' || ch == '(' || ch == ')' || ch == ',' || ch == '@'|| ch == ';' || ch == '?' || ch == '^' || ch == '~' || ch == '|'  || ch == '&'  || ch == '.') {
                *(token->text) = ch;
                (token->text)[1] = '\0';
                token->type = TK_OP;
                nextChar(e);
                break;
            } else if (ch == '%') {
				skipComments(e);
				continue;
            } else if(ch == '-' || ch == '=') {
                if(lookAhead(e,1)=='>') {
                    token->text[0] = ch;
                    token->text[1] = '>';
                    token->text[2] = '\0';
                    token->type = TK_MISC_OP;
                    nextChar(e);
                    nextChar(e);
                    break;
                }
            } else if (ch == ':') {
                if (rulegen && lookAhead(e, 1) == ':'
                        && lookAhead(e, 2) == ':') {
                    token->text[0] = ch;
                    token->text[1] = lookAhead(e, 1);
                    token->text[2] = lookAhead(e, 2);
                    token->text[3] = '\0';
                    token->type = TK_MISC_OP;
                    nextChar(e);
                    nextChar(e);
                } else {
                    *(token->text) = ch;
                    (token->text)[1] = '\0';
                    token->type = TK_MISC_OP;
                }
                nextChar(e);
                break;
            }
            if (isdigit(ch)) {
                ch = nextChar(e);
                while (isdigit(ch) || ch == '.') {
                    ch = nextChar(e);
                }
                    FPOS;
                    dupString(e, &start, (int) (pos.exprloc - start.exprloc), token->text);
                if (strchr(token->text, '.')) {
                    token->type = TK_DOUBLE;
                } else {
                    token->type = TK_INT;
                }
            } else if (ch == '_' || isalpha(ch)) {
                ch = nextChar(e);
                while (ch == '_' || isalnum(ch)) {
                    ch = nextChar(e);
                }
                    FPOS;
                    dupString(e, &start, (int) (pos.exprloc - start.exprloc), token->text);
                token->type = TK_TEXT;
            } else if (ch == '\"') {
                nextString(e, token->text, token->vars);
                token->type = TK_STRING;
            } else if (ch == '\'') {
                nextString2(e, token->text, token->vars);
                token->type = TK_STRING;
            } else if (ch == '`') {
            	if(lookAhead(e, 1) == '`') {
            		if(nextStringBase(e, token->text, "``")==-1) {
            			token->type = N_ERROR;
            		} else {
            			token->type = TK_STRING;
            			token->vars[0] = -1;
            		}
            	} else {
					nextStringBase(e, token->text, "`", 1, '\\', token->vars);
					token->type = TK_BACKQUOTED;
            	}
            } else {
                token->type = N_ERROR;
            }
            break;
        }
    }
    return token;
}

void pushback(Pointer *e, Token *token, ParserContext *context) {
    if(token->type == TK_EOS) {
        return;
    }
	DEC_MOD(context->tqp, 1024);
}

void syncTokenQueue(Pointer *e, ParserContext *context) {
	if(context->tqp == context->tqtop) {
		return;
	}
	Token *nextToken = &context->tokenQueue[context->tqp];
	seekInFile(e, nextToken->exprloc);
	context->tqtop = context->tqp;
}

int eol(char ch) {
	return ch=='\n' || ch=='\r';
}


int nextStringBase(Pointer *e, char *value, const char* delim) {
		nextChar(e);
    	int ch = nextChar(e);
    	while(ch!=-1) {
			if(delim[0] == ch && delim[1] == lookAhead(e, 1)) {
				if(delim[0] == delim[1]) {
					while(lookAhead(e, 2) == delim[1]) {
						*(value++) = delim[0];
						nextChar(e);
					}
				}
				*value='\0';
				nextChar(e);
				nextChar(e);
				return 0;
			}
    		*(value++) = ch;
    		ch = nextChar(e);
    	}
    	return -1;

}
/*
 * return number of vars or -1 if no string found
 */
int nextStringBase(Pointer *e, char *value, const char* delim, int consumeDelim, char escape, int vars[]) {
	int mode=1; /* 1 string 3 escape */
        int nov = 0;
	char* value0=value;
	*value = lookAhead(e, 0);
	value++;
	int ch = nextChar(e);
	while(ch!=-1) {
		*value = ch;
		switch(mode) {
                    case 1:
                        if(ch ==escape) {
                            value--;
                            mode = 3;
                        } else if(strchr(delim, ch)!=NULL) {
                            if(consumeDelim) {
                                value[1]='\0';
                                trimquotes(value0);
                                nextChar(e);
                            } else {
                                value[0]='\0';
                            }
                            vars[nov] = -1;
                            return nov;
                        } else if((ch =='*' || ch=='$') &&
                                isalpha(lookAhead(e, 1))) {
                            vars[nov++] = value - value0 - 1;
                        }


                        break;
                    case 3:
                        if(ch=='n') {
                            *value = '\n';
                        } else if(ch == 't') {
                            *value = '\t';
                        } else if(ch =='r') {
                            *value = '\r';
                        } else if(ch =='0') {
                            *value = '\0';
                        } else {
                            *value = ch;
                        }
                        mode -= 2;
		}
		ch = nextChar(e);
		value ++;
	}
	return -1;
}
int nextStringParsed(Pointer *e, char *value, const char* deliml, const char *delimr, const char *delim, int consumeDelim, int vars[]) {
	int mode=0; /* level */
	int nov = 0;
	char* value0=value;
	int ch = lookAhead(e, 0);
	while(ch!=-1) {
		*value = ch;
        if(strchr(deliml, ch)!=NULL) {
        	mode ++;
        } else if(mode > 0 && strchr(delimr, ch)!=NULL) {
			mode --;
		} else if(mode == 0 && strchr(delim, ch)) {
            if(consumeDelim) {
                value[1]='\0';
                trimquotes(value0);
                nextChar(e);
            } else {
                value[0]='\0';
            }
            vars[nov] = -1;
            return nov;
        } else if((ch =='*' || ch=='$') &&
                isalpha(lookAhead(e, 1))) {
            vars[nov++] = value - value0;
        }
		ch = nextChar(e);
		value ++;
	}
	return -1;
}
int nextString(Pointer *e, char *value, int vars[]) {
	return nextStringBase(e, value, "\"", 1, '\\', vars);
}
int nextString2(Pointer *e, char *value, int vars[]) {
	return nextStringBase(e, value, "\'", 1, '\\', vars);
}

char* trim(char* str) {
	char* trimmed = str;
	while(*trimmed =='\t' || *trimmed==' ') {
		trimmed ++;
	}
	int l = strlen(trimmed)-1;
	while(l>=0 && (trimmed[l] =='\t' || trimmed[l]==' ')) {
		l--;
	}
	trimmed[l+1] = '\0';
	return trimmed;

}
void trimquotes(char *string) {
	int len = strlen(string)-2;
	memmove(string, string+1, len*sizeof(char));
	string[len]='\0';
}

void printTree(Node *n, int indent) {
	printIndent(indent);
	// char buf[128], buf2[128];
	printf("%s\n",n->text);
	int i;
	for(i=0;i<n->degree;i++) {
		printTree(n->subtrees[i],indent+1);
	}

}

void printIndent(int n) {
	int i;
	for(i=0;i<n;i++) {
		printf("\t");
	}
}


/************** file/buffer pointer utilities ***************/
void nextChars(Pointer *p, int len) {
	int i;
        for(i=0;i<len;i++) {
		nextChar(p);
	}
}

/**
 * returns -1 if reached eos
 */
int nextChar(Pointer *p) {
    if(p->isFile) {
	int ch = lookAhead(p, 1);
	/*if(ch != -1) { */
		p->p++;
	/*} */
	return ch;
    } else {
        if(p->strbuf[p->strp] == '\0') {
            return -1;
        }
        int ch = p->strbuf[++p->strp];
        if(ch == '\0') {
            ch = -1; /* return -1 for eos */
	}
	return ch;
    }
}

Pointer::Pointer(FILE *fp, const char *ruleBaseName) {
        initPointer(fp, ruleBaseName);
}
Pointer::Pointer(const char* buf) {
        initPointer(buf);
}
Pointer::~Pointer() {
    if(isFile) {
        fclose(fp);
    }
    free(base);

}

void Pointer::initPointer(FILE* fp0, const char* source) {
    fseek(fp0, 0, SEEK_SET);
    fp = fp0;
    fpos = 0;
    len = 0;
    p = 0;
    isFile = 1;
    base = (char *)malloc(strlen(source)+2);
    base[0] = 'f';
    strcpy(base + 1, source);
}

void Pointer::initPointer(const char *buf) {
    strbuf = buf;
    strbuflen = strlen(buf);
    strp = 0;
    isFile = 0;
    base = (char *)malloc(strlen(buf)+2);
    base[0] = 's';
    strcpy(base + 1, buf);
}

void readToBuffer(Pointer *p) {
    if(p->isFile) {
	unsigned int move = (p->len+1)/2;
	move = move > p->p? p->p : move; /* prevent next char from being deleted */
	int startpos = p->len - move;
	int load = POINTER_BUF_SIZE - startpos;
	/* move remaining to the top of the buffer */
	memmove(p->buf,p->buf+move,startpos*sizeof(char));
	/* load from file */
	int count = fread(p->buf+startpos, sizeof(char), load, p->fp);
	p->len = startpos + count;
	p->p -= move;
	p->fpos += move * sizeof(char);
   } else {
   }
}

void seekInFile(Pointer *p, unsigned long x) {
   if(p -> isFile) {
       if(p->fpos > x * sizeof(char) || p->fpos + p->len <= x * sizeof(char)) {
           fseek(p->fp, x * sizeof(char), SEEK_SET);
           clearBuffer(p);
           p->fpos = x * sizeof(char);
           readToBuffer(p);
       } else {
           p->p = x * sizeof(char) - p->fpos;
       }
   } else {
        p->strp = x;
   }
}

void clearBuffer(Pointer *p) {
   if(p->isFile) {
	p->fpos += p->len * sizeof(char);
	p->len = p->p = 0;
   } else {
   }
}

/* assume that n is less then POINTER_BUF_SIZE */
int dupString(Pointer *p, Label * start, int n, char *buf) {
   if(p->isFile) {
	Label curr;
        getFPos(&curr, p, NULL);
	seekInFile(p, start->exprloc);
        int len = 0;
        int ch;
        while(len < n && (ch=lookAhead(p, 0)) != -1) {
            buf[len++] = (char) ch;
            nextChar(p);
        }
	buf[len] = '\0';
	seekInFile(p, curr.exprloc);
	return len;
   } else {
	int len = strlen(p->strbuf + start->exprloc);
        len = len > n ? n : len;
        memcpy(buf, p->strbuf + start->exprloc, len * sizeof(char));
        buf[len] = '\0';
        return len;
   }
}

int dupLine(Pointer *p, Label * start, int n, char *buf) {
    Label pos;
    getFPos(&pos, p, NULL);
    seekInFile(p, 0);
    int len = 0;
    int i = 0;
    int ch = lookAhead(p, 0);
    while(ch != -1) {
        if(ch=='\n') {
            if(i<start->exprloc)
                len = 0;
            else {
                break;
            }
        } else {
            buf[len] = ch;
            len++;
            if(len == n - 1) {
                break;
            }
        }
        i++;
        ch = nextChar(p);
    }
    buf[len] = '\0';
    seekInFile(p, pos.exprloc);
    return len;
}

void getCoor(Pointer *p, Label * errloc, int coor[2]) {
    Label pos;
    getFPos(&pos, p, NULL);
    seekInFile(p, 0);
    coor[0] = coor[1] = 0;
    int i;
    char ch = lookAhead(p, 0);
    for(i =0;i<errloc->exprloc;i++) {
        if(ch=='\n') {
            coor[0]++;
            coor[1] = 0;
        } else {
            coor[1]++;
        }
        ch = nextChar(p);
/*        if(ch == '\r') { skip \r
            ch = nextChar(p);
        } */
    }
    seekInFile(p, pos.exprloc);
}

int getLineRange(Pointer *p, int line, long range[2]) {
    Label pos;
    getFPos(&pos, p, NULL);
    seekInFile(p, 0);
	Label l;
    range[0] = range[1] = 0;
    int i = 0;
    int ch = lookAhead(p, 0);
    while (i < line && ch != -1) {
        if(ch=='\n') {
            i++;
        }
        ch = nextChar(p);
/*        if(ch == '\r') { skip \r
            ch = nextChar(p);
        } */
    }
    if(ch == -1) {
    	return -1;
    }
    range[0] = getFPos(&l, p, NULL)->exprloc;
    while (i == line && ch != -1) {
        if(ch=='\n') {
            i++;
        }
        ch = nextChar(p);
/*        if(ch == '\r') { skip \r
            ch = nextChar(p);
        } */
    }
    range[1] = getFPos(&l, p, NULL)->exprloc;
    seekInFile(p, pos.exprloc);
    return 0;
}

Label *getFPos(Label *l, Pointer *p, ParserContext *context) {
	if(context == NULL || context->tqtop == context->tqp) {
		if(p->isFile) {
			l->exprloc = p->fpos/sizeof(char) + p->p;
		} else {
			l->exprloc = p->strp;
		}
	} else {
		l->exprloc = context->tokenQueue[context->tqp].exprloc;
	}
    l->base = p->base;
    return l;
}
/* assume that n is less then POINTER_BUF_SIZE/2 and greater than or equal to 0 */
int lookAhead(Pointer *p, unsigned int n) {
	if(p->isFile) {
		if(p->p+n >= p->len) {
			readToBuffer(p);
			if(p->p+n >= p->len) {
				return -1;
			}
		}
		return (int)(p->buf[p->p+n]);
	} else {
		if(n+p->strp >= p->strbuflen) {
			return -1;
		}
		return (int)p->strbuf[p->strp+n];
	}

}

/* backward compatibility with the previous version */
char *functionParameters(char *e, char *value) {
	int mode=0; /* 0 params 1 string 2 string2 3 escape 4 escape2 */
	int l0 = 0;
	while(*e!=0) {
		*value = *e;
		switch(mode) {
			case 0:

				switch(*e) {
					case '(':
						l0++;
						break;
					case '\"':
						mode = 1;
						break;
					case '\'':
						mode = 2;
						break;
					case ')':
						l0--;
						if(l0==0) {
							value[1]='\0';
							return e+1;
						}
				}
				break;
			case 1:
				switch(*e) {
					case '\\':
						mode = 3;
						break;
					case '\"':
						mode = 0;
				}
				break;
			case 2:
				switch(*e) {
					case '\\':
						mode = 4;
						break;
					case '\'':
						mode = 0;
				}
				break;
			case 3:
			case 4:
				mode -= 2;
		}
		e++; value ++;
	}
	*value=0;
	return e;
}
char *nextStringString(char *e, char *value) {
	int mode=1; /* 1 string 3 escape */
	char* e0=e;
	char* value0=value;
			*value = *e;
	value++;
	e++;
	while(*e!=0) {
		*value = *e;
		switch(mode) {
			case 1:
				switch(*e) {
					case '\\':
					    value--;
						mode = 3;
						break;
					case '\"':
						value[1]='\0';
						trimquotes(value0);
						return e+1;
				}
				break;
			case 3:
				mode -= 2;
		}
		e++; value ++;
	}
	return e0;
}
char *nextString2String(char *e, char *value) {
	int mode=1; /* 1 string 3 escape */
	char* e0=e;
	char* value0 = value;
	*value = *e;
	value ++;
	e++;
	while(*e!=0) {
		*value = *e;
		switch(mode) {
			case 1:
				switch(*e) {
					case '\\':
						value--;
						mode = 3;
						break;
					case '\'':
						value[1]='\0';
						trimquotes(value0);
						return e+1;
				}
				break;
			case 3:
				mode -= 2;
		}
		e++; value ++;
	}
	return e0;
}


char * nextRuleSection(char* buf, char* value) {
	char* e=buf;
	int mode=0; /* 0 section 1 string 2 string2 3 escape 4 escape2 */
	while(*e!=0) {
		*value = *e;
		switch(mode) {
			case 0:

				switch(*e) {
					case '\"':
						mode = 1;
						break;
					case '\'':
						mode = 2;
						break;
					case '|':
						*value='\0';
						return e+1;
				}
				break;
			case 1:
				switch(*e) {
					case '\\':
						mode = 3;
						break;
					case '\"':
						mode = 0;
				}
				break;
			case 2:
				switch(*e) {
					case '\\':
						mode = 4;
						break;
					case '\'':
						mode = 0;
				}
				break;
			case 3:
			case 4:
				mode -= 2;
		}
		e++; value ++;
	}
	*value=0;
	return e;


}

void nextActionArgumentStringBackwardCompatible(Pointer *e, Token *token) {
    skipWhitespace(e);
    Label start;
    token->exprloc = getFPos(&start, e, NULL)->exprloc;
    int ch = lookAhead(e, 0);
    if (ch==-1) { /* reach the end of stream */
        token->type = TK_EOS;
        strcpy(token->text,"EOS");
    } else {
        ch = lookAhead(e, 0);
        if(ch == '\"') {
            nextStringBase(e, token->text, "\"", 1, '\\', token->vars);
            skipWhitespace(e);
        } else if( ch == '\'') {
            nextStringBase(e, token->text, "\'", 1, '\\', token->vars);
            skipWhitespace(e);
        } else {
            nextStringParsed(e, token->text, "(", ")", ",|)", 0, token->vars);
            /* remove trailing ws */
            int l0;
            l0 = strlen(token->text);
            while(isspace(token->text[l0-1])) {
                l0--;
            }
            token->text[l0++]='\0';
        }
        token->type = TK_STRING;
    }
}

void generateErrMsgFromFile(char *msg, long errloc, char* ruleBasePath, char errbuf[ERR_MSG_LEN]) {
    FILE *fp = fopen(ruleBasePath, "r");
    Pointer e(fp, ruleBasePath);
    Label l;
    l.base = NULL;
    l.exprloc = errloc;
    generateErrMsgFromPointer(msg, &l, &e, errbuf);
}

void generateErrMsgFromSource(char *msg, long errloc, char *src, char errbuf[ERR_MSG_LEN]) {
    Pointer e(src);
    Label l;
    l.base = NULL;
    l.exprloc = errloc;
    generateErrMsgFromPointer(msg, &l, &e, errbuf);
}
void generateErrMsgFromPointer(char *msg, Label *l, Pointer *e, char errbuf[ERR_MSG_LEN]) {
    char buf[ERR_MSG_LEN];
    dupLine(e, l, ERR_MSG_LEN, buf);
    int len = strlen(buf);
    int coor[2];
    getCoor(e, l, coor);
    int i;
    if(len < ERR_MSG_LEN - 1) {
        buf[len++] = '\n';
    }
    for(i=0;i<coor[1];i++) {
        if(len >= ERR_MSG_LEN - 1) {
            break;
        }
        buf[len++] = buf[i] == '\t'?'\t':' ';
    }
    if(len < ERR_MSG_LEN - 2) {
        buf[len++] = '^';
    }
    buf[len++] = '\0';
    if(e->isFile)
    snprintf(errbuf, ERR_MSG_LEN,
            "%s\nline %d, col %d, file %s\n%s\n", msg, coor[0], coor[1], e->base+1, buf);
    else
    snprintf(errbuf, ERR_MSG_LEN,
            "%s\nline %d, col %d\n%s\n", msg, coor[0], coor[1], buf);

}
void generateAndAddErrMsg(char *msg, Node *node, LinkedList<char *> *errmsg) {
	char errmsgBuf[ERR_MSG_LEN];
	generateErrMsg(msg, node->expr, node->base, errmsgBuf);
	errmsg->append(strdup(errmsgBuf));
}
char *generateErrMsg(char *msg, long errloc, char *ruleBaseName, char errmsg[ERR_MSG_LEN]) {
    switch(ruleBaseName[0]) {
        case 's': // source
            generateErrMsgFromSource(msg, errloc, ruleBaseName + 1, errmsg);
            return errmsg;
        case 'f': // file
            generateErrMsgFromFile(msg, errloc, ruleBaseName + 1, errmsg);
            return errmsg;
        default:
            snprintf(errmsg, ERR_MSG_LEN, "<unknown source type>");
            return errmsg;
    }
}
