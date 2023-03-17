#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

// for lex
#define MAXLEN 256

// Token types
typedef enum
{
    UNKNOWN,
    END,
    ENDFILE,
    INT,
    ID,
    ADDSUB,
    MULDIV,
    ASSIGN,
    LPAREN,
    RPAREN,
    INCDEC,
    XOR,
    OR,
    AND
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
// Set PRINTERR to 1 to print error message while calling error()
// Make sure you set PRINTERR to 0 before you submit your code
#define PRINTERR 1

// Call this macro to print error message and exit the program
// This will also print where you called it in your program
#define error(errorNum)                                                       \
    {                                                                         \
        if (PRINTERR)                                                         \
            fprintf(stderr, "error() called at %s:%d: ", __FILE__, __LINE__); \
        err(errorNum);                                                        \
    }

// Error types
typedef enum
{
    UNDEFINED,
    MISPAREN,
    NOTNUMID,
    NOTFOUND,
    RUNOUT,
    NOTLVAL,
    DIVZERO,
    SYNTAXERR,
    UNKNOWN_ERROR
} ErrorType;

// Structure of the symbol table
typedef struct
{
    int val;
    char name[MAXLEN];
    int UNKNOWN_ERROR;
} Symbol;

// Structure of a tree node
typedef struct _Node
{
    TokenSet data;
    int val;
    char lexeme[MAXLEN];
    struct _Node *left;
    struct _Node *right;
    int regis;
} BTNode;
int cnt = 0;
int cnt2 = 0;

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
void printAssembly(BTNode *root, int side);
void freeTree(BTNode *root);
BTNode *factor(void);
BTNode *term(void);
BTNode *term_tail(BTNode *left);
BTNode *assign_expr(void);
BTNode *or_expr(void);
BTNode *or_expr_tail(BTNode *left);
BTNode *xor_expr(void);
BTNode *xor_expr_tail(BTNode *left);
BTNode *and_expr(void);
BTNode *and_expr_tail(BTNode *left);
BTNode *addsub_expr(void);
BTNode *addsub_expr_tail(BTNode *left);
BTNode *muldiv_expr(void);
BTNode *muldiv_expr_tail(BTNode *left);
BTNode *unary_expr(void);
void statement(void);
// Print error message and exit the program
void err(ErrorType errorNum);
int ck(BTNode *root);

// for codeGen
// Evaluate the syntax tree
int evaluateTree(BTNode *root);
// Print the syntax tree in prefix
void printPrefix(BTNode *root);

/*============================================================================================
lex implementation
============================================================================================*/

TokenSet getToken(void)
{
    int i = 0;
    char c = '\0';

    while ((c = fgetc(stdin)) == ' ' || c == '\t')
        ;

    if (isdigit(c))
    {
        lexeme[0] = c;
        c = fgetc(stdin);
        i = 1;
        while (isdigit(c) && i < MAXLEN)
        {
            lexeme[i] = c;
            ++i;
            c = fgetc(stdin);
        }
        ungetc(c, stdin);
        lexeme[i] = '\0';
        return INT;
    }
    else if (c == '+' || c == '-')
    {
        lexeme[0] = c;
        lexeme[1] = '\0';
        c = fgetc(stdin);
        if (c == '+' && lexeme[0] == '+')
        {
            lexeme[1] = '+';
            lexeme[2] = '\0';
            return INCDEC;
        }
        else if (c == '-' && lexeme[0] == '-')
        {
            lexeme[1] = '-';
            lexeme[2] = '\0';
            return INCDEC;
        }
        ungetc(c, stdin);
        return ADDSUB;
    }
    else if (c == '*' || c == '/')
    {
        lexeme[0] = c;
        lexeme[1] = '\0';
        return MULDIV;
    }
    else if (c == '&')
    {
        lexeme[0] = c;
        lexeme[1] = '\0';
        return AND;
    }
    else if (c == '^')
    {
        lexeme[0] = c;
        lexeme[1] = '\0';
        return XOR;
    }
    else if (c == '|')
    {
        lexeme[0] = c;
        lexeme[1] = '\0';
        return OR;
    }
    else if (c == EOF)
    {
        return ENDFILE;
    }
    else if (c == '\n')
    {
        lexeme[0] = '\0';
        return END;
    }
    else if (c == '=')
    {
        strcpy(lexeme, "=");
        return ASSIGN;
    }
    else if (c == '(')
    {
        strcpy(lexeme, "(");
        return LPAREN;
    }
    else if (c == ')')
    {
        strcpy(lexeme, ")");
        return RPAREN;
    }
    else if (isalpha(c) || '_')
    {
        lexeme[0] = c;
        c = fgetc(stdin);
        i = 1;
        while ((isdigit(c) || isalpha(c) || c == '_') && i < MAXLEN)
        {
            lexeme[i] = c;
            ++i;
            c = fgetc(stdin);
        }
        ungetc(c, stdin);
        lexeme[i] = '\0';
        return ID;
    }
    else
    {
        return UNKNOWN;
    }
}

void advance(void)
{
    curToken = getToken();
}

int match(TokenSet token)
{
    if (curToken == UNKNOWN)
        advance();
    return token == curToken;
}

char *getLexeme(void)
{
    return lexeme;
}

/*============================================================================================
parser implementation
============================================================================================*/

void initTable(void)
{
    strcpy(table[0].name, "x");
    table[0].val = 0;
    table[0].UNKNOWN_ERROR = 87;
    strcpy(table[1].name, "y");
    table[1].val = 0;
    table[1].UNKNOWN_ERROR = 87;
    strcpy(table[2].name, "z");
    table[2].val = 0;
    table[2].UNKNOWN_ERROR = 87;
    sbcount = 3;
}

int getval(char *str)
{
    int i = 0;

    for (i = 0; i < sbcount; i++)
        if (strcmp(str, table[i].name) == 0)
        {
            table[i].UNKNOWN_ERROR += 1;
            return table[i].val;
        }

    if (sbcount >= TBLSIZE)
        error(RUNOUT);

    strcpy(table[sbcount].name, str);
    table[sbcount].val = 0;
    table[sbcount].UNKNOWN_ERROR = 1;
    sbcount++;
    return 0;
}
int setval(char *str, int val)
{
    int i = 0;

    for (i = 0; i < sbcount; i++)
    {
        if (strcmp(str, table[i].name) == 0)
        {
            table[i].val = val;
            return val;
        }
    }

    if (sbcount >= TBLSIZE)
        error(RUNOUT);

    strcpy(table[sbcount].name, str);
    table[sbcount].val = val;
    table[sbcount].UNKNOWN_ERROR += 1;
    sbcount++;
    return val;
}

BTNode *makeNode(TokenSet tok, const char *lexe)
{
    BTNode *node = (BTNode *)malloc(sizeof(BTNode));
    strcpy(node->lexeme, lexe);
    node->data = tok;
    node->val = 0;
    node->left = NULL;
    node->right = NULL;
    return node;
}

void freeTree(BTNode *root)
{
    if (root != NULL)
    {
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
BTNode *factor(void)
{
    BTNode *retp = NULL, *left = NULL;

    if (match(INT))
    {
        retp = makeNode(INT, getLexeme());
        advance();
    }
    else if (match(ID))
    {
        retp = makeNode(ID, getLexeme());
        advance();
        /*if (!match(ASSIGN)) {
            retp = left;
        } else {
            retp = makeNode(ASSIGN, getLexeme());
            advance();
            retp->left = left;
            retp->right = assign_expr();
        }*/
    }
    else if (match(INCDEC))
    {
        retp = makeNode(INCDEC, getLexeme());
        advance();
        if (match(ID))
        {
            retp->right = makeNode(ID, getLexeme());
            retp->left = NULL;
            advance();
        }
        else
            error(NOTNUMID);
    } /*
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
     }*/
    else if (match(LPAREN))
    {
        advance();
        retp = assign_expr();
        if (match(RPAREN))
            advance();
        else
            error(MISPAREN);
    }
    else
    {
        error(NOTNUMID);
    }
    return retp;
}

// term := factor term_tail
/*BTNode *term(void) {
    BTNode *node = factor();
    return term_tail(node);
}*/

// term_tail := MULDIV factor term_tail | NiL
/*BTNode *term_tail(BTNode *left) {
    BTNode *node = NULL;

    if (match(MULDIV)) {
        node = makeNode(MULDIV, getLexeme());
        advance();
        node->left = left;
        node->right = factor();
        return term_tail(node);
    } else {
        return left;
    }
}*/

// expr_tail := ADDSUB term expr_tail | NiL
BTNode *or_expr_tail(BTNode *left)
{
    BTNode *node = NULL;

    if (match(OR))
    {
        node = makeNode(OR, getLexeme());
        advance();
        node->left = left;
        node->right = xor_expr();
        return or_expr_tail(node);
    }
    else
    {
        return left;
    }
}
BTNode *xor_expr_tail(BTNode *left)
{
    BTNode *node = NULL;

    if (match(XOR))
    {
        node = makeNode(XOR, getLexeme());
        advance();
        node->left = left;
        node->right = and_expr();
        return xor_expr_tail(node);
    }
    else
    {
        return left;
    }
}
BTNode *and_expr_tail(BTNode *left)
{
    BTNode *node = NULL;

    if (match(AND))
    {
        node = makeNode(AND, getLexeme());
        advance();
        node->left = left;
        node->right = addsub_expr();
        return and_expr_tail(node);
    }
    else
    {
        return left;
    }
}
BTNode *addsub_expr_tail(BTNode *left)
{
    BTNode *node = NULL;

    if (match(ADDSUB))
    {
        node = makeNode(ADDSUB, getLexeme());
        advance();
        node->left = left;
        node->right = muldiv_expr();
        return addsub_expr_tail(node);
    }
    else
    {
        return left;
    }
}
BTNode *muldiv_expr_tail(BTNode *left)
{
    BTNode *node = NULL;

    if (match(MULDIV))
    {
        node = makeNode(MULDIV, getLexeme());
        advance();
        node->left = left;
        node->right = unary_expr();
        return muldiv_expr_tail(node);
    }
    else
    {
        return left;
    }
}

BTNode *or_expr()
{
    BTNode *node = xor_expr();
    return or_expr_tail(node);
}
BTNode *xor_expr()
{
    BTNode *node = and_expr();
    return xor_expr_tail(node);
}
BTNode *and_expr()
{
    BTNode *node = addsub_expr();
    return and_expr_tail(node);
}
BTNode *addsub_expr()
{
    BTNode *node = muldiv_expr();
    return addsub_expr_tail(node);
}
BTNode *muldiv_expr()
{
    BTNode *node = unary_expr();
    return muldiv_expr_tail(node);
}
BTNode *unary_expr()
{
    BTNode *node = NULL;
    if (match(ADDSUB))
    {
        node = makeNode(ADDSUB, getLexeme());
        advance();
        node->left = makeNode(INT, "0");
        node->right = unary_expr();
    }
    else
    {
        node = factor();
    }
    return node;
}

// expr := term expr_tail
BTNode *assign_expr(void)
{
    // BTNode *node = term();
    BTNode *node = or_expr();
    if (match(ASSIGN) && node->data == ID)
    {
        BTNode *new = makeNode(ASSIGN, getLexeme());
        advance();
        new->left = node;
        new->right = assign_expr();
        return new;
    }
    else
        return node;
}

// statement := ENDFILE | END | expr END
void statement(void)
{
    BTNode *retp = NULL;

    if (match(ENDFILE))
    {
        printf("MOV r0 [0]\n");
        printf("MOV r1 [4]\n");
        printf("MOV r2 [8]\n");
        printf("EXIT 0\n");
        exit(0);
    }
    else if (match(END))
    {
        // printf(">> ");
        advance();
    }
    else
    {
        retp = assign_expr();
        if (match(END))
        {
            evaluateTree(retp);
            // printf("%d\n", evaluateTree(retp));
            // printf("Prefix traversal: ");
            // printPrefix(retp);
            // printf("\n");
            printAssembly(retp, 0);
            freeTree(retp);
            advance();
        }
        else
        {
            error(SYNTAXERR);
        }
    }
}

int ck(BTNode *root)
{
    static int flag = 0;
    if (root->data == ID)
        flag = 1;
    if (root->left)
        ck(root->left);
    if (root->right)
        ck(root->right);
    return flag;
}

void err(ErrorType errorNum)
{
    if (PRINTERR)
    {
        fprintf(stderr, "error: ");
        switch (errorNum)
        {
        case MISPAREN:
            fprintf(stderr, "mismatched parenthesis\n");
            break;
        case NOTNUMID:
            fprintf(stderr, "number or identifier expected\n");
            break;
        case NOTFOUND:
            fprintf(stderr, "variable not defined\n");
            break;
        case RUNOUT:
            fprintf(stderr, "out of memory\n");
            break;
        case NOTLVAL:
            fprintf(stderr, "lvalue required as an operand\n");
            break;
        case DIVZERO:
            fprintf(stderr, "divide by constant zero\n");
            break;
        case SYNTAXERR:
            fprintf(stderr, "syntax error\n");
            break;
        case UNKNOWN_ERROR:
            fprintf(stderr, "UNKNOWN_ERROR\n");
            break;
        default:
            fprintf(stderr, "undefined error\n");
            break;
        }
    }
    printf("EXIT 1\n");
    exit(0);
}

/*============================================================================================
codeGen implementation
============================================================================================*/

int evaluateTree(BTNode *root)
{

    int retval = 0, lv = 0, rv = 0;

    if (root != NULL)
    {
        switch (root->data)
        {
        case ID:
            retval = getval(root->lexeme);
            break;
        case INT:
            retval = atoi(root->lexeme);
            break;
        case ASSIGN:
            rv = evaluateTree(root->right);
            retval = setval(root->left->lexeme, rv);
            break;
        case AND:
            lv = evaluateTree(root->left);
            rv = evaluateTree(root->right);
            retval = lv & rv;
            break;
        case OR:
            lv = evaluateTree(root->left);
            rv = evaluateTree(root->right);
            retval = lv | rv;
            break;
        case XOR:
            lv = evaluateTree(root->left);
            rv = evaluateTree(root->right);
            retval = lv ^ rv;
            break;
        case INCDEC:
            rv = evaluateTree(root->right);
            if (strcmp(root->lexeme, "++") == 0)
            {
                retval = setval(root->right->lexeme, rv + 1);
                break;
            }
            if (strcmp(root->lexeme, "--") == 0)
                retval = setval(root->right->lexeme, rv - 1);
            break;
        case ADDSUB:
        case MULDIV:
            lv = evaluateTree(root->left);
            rv = evaluateTree(root->right);
            if (strcmp(root->lexeme, "+") == 0)
            {
                retval = lv + rv;
            }
            else if (strcmp(root->lexeme, "-") == 0)
            {
                retval = lv - rv;
            }
            else if (strcmp(root->lexeme, "*") == 0)
            {
                retval = lv * rv;
            }
            else if (strcmp(root->lexeme, "/") == 0)
            {
                if (rv == 0 && ck(root) == 0)
                {
                    error(DIVZERO);
                }
                retval = 0;
            }
            break;
        default:
            retval = 0;
        }
    }
    return retval;
}

void printPrefix(BTNode *root)
{
    if (root != NULL)
    {
        printf("%s ", root->lexeme);
        printPrefix(root->left);
        printPrefix(root->right);
    }
}

void printAssembly(BTNode *root, int side)
{
    static int ok = 0;
    if (root->data == ASSIGN)
    {
        if (root->right)
            printAssembly(root->right, 0);
        // if(root->left) printAssembly(root->left);
        if (root->left->data == ID)
        {
            for (int i = 0; i < sbcount; i++)
            {
                if (strcmp(root->left->lexeme, table[i].name) == 0)
                {
                    root->regis = cnt;
                    printf("MOV [%d] r%d\n", i * 4, root->right->regis);
                    root->regis = root->right->regis;
                    cnt++;
                }
            }
        }
    }
    else
    {
        if (root->left)
            printAssembly(root->left, side);
        if (root->right)
            printAssembly(root->right, side);
        if (root->data == ID)
        {
            root->regis = cnt;
            for (int i = 0; i < sbcount; i++)
            {
                if (strcmp(root->lexeme, table[i].name) == 0)
                {
                    if (side == 0 && table[i].UNKNOWN_ERROR <= 1)
                        error(NOTFOUND);
                    root->regis = cnt;
                    printf("MOV r%d [%d]\n", cnt, i * 4);
                    cnt++;
                }
            }
        }
        else if (root->data == INT)
        {
            root->regis = cnt;
            printf("MOV r%d %d\n", cnt, atoi(root->lexeme));
            cnt++;
        }
        else if (root->data == ADDSUB)
        {
            if (strcmp(root->lexeme, "+") == 0)
            {
                printf("ADD r%d r%d\n", root->left->regis, root->right->regis);
                root->regis = root->left->regis;
                cnt--;
            }
            else if (strcmp(root->lexeme, "-") == 0)
            {
                printf("SUB r%d r%d\n", root->left->regis, root->right->regis);
                root->regis = root->left->regis;
                cnt--;
            }
        }
        else if (root->data == MULDIV)
        {
            if (strcmp(root->lexeme, "*") == 0)
            {
                printf("MUL r%d r%d\n", root->left->regis, root->right->regis);
                root->regis = root->left->regis;
                cnt--;
            }
            else if (strcmp(root->lexeme, "/") == 0)
            {
                printf("DIV r%d r%d\n", root->left->regis, root->right->regis);
                root->regis = root->left->regis;
                cnt--;
            }
        }
        else if (root->data == AND)
        {
            printf("AND r%d r%d\n", root->left->regis, root->right->regis);
            root->regis = root->left->regis;
            cnt--;
        }
        else if (root->data == OR)
        {
            printf("OR r%d r%d\n", root->left->regis, root->right->regis);
            root->regis = root->left->regis;
            cnt--;
        }
        else if (root->data == XOR)
        {
            printf("XOR r%d r%d\n", root->left->regis, root->right->regis);
            root->regis = root->left->regis;
            cnt--;
        }
        else if (root->data == INCDEC)
        {
            if (strcmp(root->lexeme, "++") == 0)
            {
                printf("MOV r%d 1\n", cnt);
                printf("ADD r%d r%d\n", root->right->regis, cnt);
                root->regis = cnt;
                cnt--;
                if (root->right->data == ID)
                    for (int i = 0; i < sbcount; i++)
                    {
                        if (strcmp(root->right->lexeme, table[i].name) == 0)
                        {
                            if (side == 0 && table[i].UNKNOWN_ERROR <= 1)
                                error(NOTFOUND);
                            root->regis = cnt;
                            printf("MOV [%d] r%d\n", i * 4, cnt);
                            cnt++;
                        }
                    }
            }
            else if (strcmp(root->lexeme, "--") == 0)
            {
                printf("MOV r%d 1\n", cnt);
                printf("SUB r%d r%d\n", root->right->regis, cnt);
                root->regis = cnt;
                cnt--;
                if (root->right->data == ID)
                    for (int i = 0; i < sbcount; i++)
                    {
                        if (strcmp(root->right->lexeme, table[i].name) == 0)
                        {
                            root->regis = cnt;
                            printf("MOV [%d] r%d\n", i * 4, cnt);
                            cnt++;
                        }
                    }
            }
        }
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

int main()
{
    initTable();
    // printf(">> ");
    while (1)
    {
        statement();
        cnt = 0;
    }
    return 0;
}
