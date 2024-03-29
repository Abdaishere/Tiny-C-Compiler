#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

using namespace std;

/*
{ Sample program
  in TINY language
  compute factorial
}

read x; {input an integer}
if 0<x then {compute only if x>=1}
  fact:=1;
  repeat
    fact := fact * x;
    x:=x-1
  until x=0;
  write fact {output factorial}
end
*/

// sequence of statements separated by ;
// no procedures - no declarations
// all variables are integers
// variables are declared simply by assigning values to them :=
// if-statement: if (boolean) then [else] end
// repeat-statement: repeat until (boolean)
// boolean only in if and repeat conditions < = and two mathematical expressions
// math expressions integers only, + - * / ^
// I/O read write
// Comments {}

////////////////////////////////////////////////////////////////////////////////////
// Strings /////////////////////////////////////////////////////////////////////////

bool Equals(const char *a, const char *b) {
    return strcmp(a, b) == 0;
}

bool StartsWith(const char *a, const char *b) {
    int nb = strlen(b);
    return strncmp(a, b, nb) == 0;
}

void Copy(char *a, const char *b, int n = 0) {
    if (n > 0) {
        strncpy(a, b, n);
        a[n] = 0;
    } else strcpy(a, b);
}

void AllocateAndCopy(char **a, const char *b) {
    if (b == 0) {
        *a = 0;
        return;
    }
    int n = strlen(b);
    *a = new char[n + 1];
    strcpy(*a, b);
}

////////////////////////////////////////////////////////////////////////////////////
// Input and Output ////////////////////////////////////////////////////////////////

#define MAX_LINE_LENGTH 10000

struct InFile {
    FILE *file;
    int cur_line_num;

    char line_buf[MAX_LINE_LENGTH];
    int cur_ind, cur_line_size;

    InFile(const char *str) {
        file = 0;
        if (str) file = fopen(str, "r");
        cur_line_size = 0;
        cur_ind = 0;
        cur_line_num = 0;
    }

    ~InFile() { if (file) fclose(file); }

    void SkipSpaces() {
        while (cur_ind < cur_line_size) {
            char ch = line_buf[cur_ind];
            if (ch != ' ' && ch != '\t' && ch != '\r' && ch != '\n') break;
            cur_ind++;
        }
    }

    bool SkipUpto(const char *str) {
        while (true) {
            SkipSpaces();
            while (cur_ind >= cur_line_size) {
                if (!GetNewLine()) return false;
                SkipSpaces();
            }

            if (StartsWith(&line_buf[cur_ind], str)) {
                cur_ind += strlen(str);
                return true;
            }
            cur_ind++;
        }
        return false;
    }

    bool GetNewLine() {
        cur_ind = 0;
        line_buf[0] = 0;
        if (!fgets(line_buf, MAX_LINE_LENGTH, file)) return false;
        cur_line_size = strlen(line_buf);
        if (cur_line_size == 0) return false; // End of file
        cur_line_num++;
        return true;
    }

    char *GetNextTokenStr() {
        SkipSpaces();
        while (cur_ind >= cur_line_size) {
            if (!GetNewLine()) return 0;
            SkipSpaces();
        }
        return &line_buf[cur_ind];
    }

    void Advance(int num) {
        cur_ind += num;
    }
};

struct OutFile {
    FILE *file;

    OutFile(const char *str) {
        file = 0;
        if (str) file = fopen(str, "w");
    }

    ~OutFile() { if (file) fclose(file); }

    void Out(basic_string<char> s) {
        fprintf(file, "%s\n", s.c_str());
        fflush(file);
    }
};

////////////////////////////////////////////////////////////////////////////////////
// Compiler Parameters /////////////////////////////////////////////////////////////

struct CompilerInfo {
    InFile in_file;
    OutFile out_file;
    OutFile debug_file;

    CompilerInfo(const char *in_str, const char *out_str, const char *debug_str)
            : in_file(in_str), out_file(out_str), debug_file(debug_str) {
    }
} compiler("input.txt", "output.txt", "debug.txt");

////////////////////////////////////////////////////////////////////////////////////
// Scanner /////////////////////////////////////////////////////////////////////////

#define MAX_TOKEN_LEN 40

enum TokenType {
    IF, THEN, ELSE, END, REPEAT, UNTIL, READ, WRITE,
    ASSIGN, EQUAL, LESS_THAN,
    PLUS, MINUS, TIMES, DIVIDE, POWER,
    SEMI_COLON,
    LEFT_PAREN, RIGHT_PAREN,
    LEFT_BRACE, RIGHT_BRACE,
    ID, NUM,
    ENDFILE, ERROR
};

// Used for debugging only /////////////////////////////////////////////////////////
const char *TokenTypeStr[] =
        {
                "If", "Then", "Else", "End", "Repeat", "Until", "Read", "Write",
                "Assign", "Equal", "LessThan",
                "Plus", "Minus", "Times", "Divide", "Power",
                "SemiColon",
                "LeftParen", "RightParen",
                "LeftBrace", "RightBrace",
                "ID", "Num",
                "EndFile", "Error"
        };

struct Token {
    TokenType type;
    char str[MAX_TOKEN_LEN + 1];

    Token() {
        str[0] = 0;
        type = ERROR;
    }

    Token(TokenType _type, const char *_str) {
        type = _type;
        Copy(str, _str);
    }
};

const Token reserved_words[] =
        {
                Token(IF, "if"),
                Token(THEN, "then"),
                Token(ELSE, "else"),
                Token(END, "end"),
                Token(REPEAT, "repeat"),
                Token(UNTIL, "until"),
                Token(READ, "read"),
                Token(WRITE, "write")
        };
const int num_reserved_words = sizeof(reserved_words) / sizeof(reserved_words[0]);

// if there is tokens like < <=, sort them such that sub-tokens come last: <= <
// the closing comment should come immediately after opening comment
const Token symbolic_tokens[] =
        {
                Token(ASSIGN, ":="),
                Token(EQUAL, "="),
                Token(LESS_THAN, "<"),
                Token(PLUS, "+"),
                Token(MINUS, "-"),
                Token(TIMES, "*"),
                Token(DIVIDE, "/"),
                Token(POWER, "^"),
                Token(SEMI_COLON, ";"),
                Token(LEFT_PAREN, "("),
                Token(RIGHT_PAREN, ")"),
                Token(LEFT_BRACE, "{"),
                Token(RIGHT_BRACE, "}")
        };
const int num_symbolic_tokens = sizeof(symbolic_tokens) / sizeof(symbolic_tokens[0]);

inline bool IsDigit(char ch) { return (ch >= '0' && ch <= '9'); }

inline bool IsLetter(char ch) { return ((ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z')); }

inline bool IsLetterOrUnderscore(char ch) { return (IsLetter(ch) || ch == '_'); }

void GetNextToken(CompilerInfo *pci, Token *ptoken) {
    ptoken->type = ERROR;
    ptoken->str[0] = 0;

    int i;
    char *s = pci->in_file.GetNextTokenStr();
    if (!s) {
        ptoken->type = ENDFILE;
        ptoken->str[0] = 0;
        return;
    }

    for (i = 0; i < num_symbolic_tokens; i++) {
        if (StartsWith(s, symbolic_tokens[i].str))
            break;
    }

    if (i < num_symbolic_tokens) {
        if (symbolic_tokens[i].type == LEFT_BRACE) {
            pci->in_file.Advance(strlen(symbolic_tokens[i].str));
            if (!pci->in_file.SkipUpto(symbolic_tokens[i + 1].str)) return;
            return GetNextToken(pci, ptoken);
        }
        ptoken->type = symbolic_tokens[i].type;
        Copy(ptoken->str, symbolic_tokens[i].str);
    } else if (IsDigit(s[0])) {
        int j = 1;
        while (IsDigit(s[j])) j++;

        ptoken->type = NUM;
        Copy(ptoken->str, s, j);
    } else if (IsLetterOrUnderscore(s[0])) {
        int j = 1;
        while (IsLetterOrUnderscore(s[j])) j++;

        ptoken->type = ID;
        Copy(ptoken->str, s, j);

        for (i = 0; i < num_reserved_words; i++) {
            if (Equals(ptoken->str, reserved_words[i].str)) {
                ptoken->type = reserved_words[i].type;
                break;
            }
        }
    }

    int len = strlen(ptoken->str);
    if (len > 0) pci->in_file.Advance(len);
}

////////////////////////////////////////////////////////////////////////////////////
// Parser //////////////////////////////////////////////////////////////////////////

enum NodeKind {
    IF_NODE, REPEAT_NODE, ASSIGN_NODE, READ_NODE, WRITE_NODE,
    OPER_NODE, NUM_NODE, ID_NODE
};

// Used for debugging only /////////////////////////////////////////////////////////
const char *NodeKindStr[] =
        {
                "If", "Repeat", "Assign", "Read", "Write",
                "Oper", "Num", "ID"
        };

enum ExprDataType {
    VOID, INTEGER, BOOLEAN
};

// Used for debugging only /////////////////////////////////////////////////////////
const char *ExprDataTypeStr[] =
        {
                "Void", "Integer", "Boolean"
        };

#define MAX_CHILDREN 7

struct TreeNode {
    TreeNode *child[MAX_CHILDREN];
    TreeNode *sibling; // used for sibling statements only

    NodeKind node_kind;

    union {
        TokenType oper;
        int num;
        char *id;
    }; // defined for expression/int/identifier only
    ExprDataType expr_data_type; // defined for expression/int/identifier only

    int line_num;

    TreeNode() {
        int i;
        for (i = 0; i < MAX_CHILDREN; i++) child[i] = 0;
        sibling = 0;
        expr_data_type = VOID;
    }
};

struct ParseInfo {
    Token next_token;

    inline TokenType getType() {
        return next_token.type;
    }
} token;

void PrintTree(TreeNode *node, int sh = 0) {
    int i, NSH = 3;
    for (i = 0; i < sh; i++) printf(" ");

    printf("[%s]", NodeKindStr[node->node_kind]);

    if (node->node_kind == OPER_NODE) printf("[%s]", TokenTypeStr[node->oper]);
    else if (node->node_kind == NUM_NODE) printf("[%d]", node->num);
    else if (node->node_kind == ID_NODE || node->node_kind == READ_NODE || node->node_kind == ASSIGN_NODE)
        printf("[%s]", node->id);

    if (node->expr_data_type != VOID) printf("[%s]", ExprDataTypeStr[node->expr_data_type]);

    printf("\n");

    for (i = 0; i < MAX_CHILDREN; i++) if (node->child[i]) PrintTree(node->child[i], sh + NSH);
    if (node->sibling) PrintTree(node->sibling, sh);
}


//Extended BNF:


// program -> stmtseq
// stmtseq -> stmt { ; stmt }
// stmt -> ifstmt | repeatStmt | assignStmt | readStmt | writeStmt
// ifstmt -> if expr then stmtseq [ else stmtseq ] end
// repeatStmt -> repeat stmtseq until expr
// assignStmt -> identifier := expr
// readStmt -> read identifier
// writeStmt -> write expr
// expr -> mathexpr [ (<|=) mathexpr ]
// mathexpr -> term { (+|-) term }    left associative
// term -> factor { (*|/) factor }    left associative
// factor -> newexpr { ^ newexpr }    right associative
// newexpr -> ( mathexpr ) | number | identifier


TreeNode *stmt();

TreeNode *stmtSeq();

TreeNode *ifStmt();

TreeNode *repeatStmt();

TreeNode *assignStmt();

TreeNode *readStmt();

TreeNode *writeStmt();

TreeNode *expr();

TreeNode *mathExpr();

TreeNode *term();

TreeNode *factor();

TreeNode *newExpr();

void displayError(string message, int status = 1)
{
    compiler.out_file.Out(message);
    throw status;
}

// check if expected and get the next token
void check(TokenType expected) {
    if (token.getType() == expected)
        GetNextToken(&compiler, &token.next_token);
    else
    {
        displayError("Error in line " + to_string(compiler.in_file.cur_line_num) + " ==> Expected : " +
                              TokenTypeStr[expected] + " ==> Found : " + TokenTypeStr[token.getType()]);
    }
}

TreeNode *stmtSeq() {
    TreeNode *Stmt = stmt();
    TreeNode *seq = Stmt;
    while ((token.getType() != ENDFILE) && (token.getType() != END)
           && (token.getType() != ELSE) && (token.getType() != UNTIL)) {
        TreeNode *temp;
        //  { ; stmt }
        check(SEMI_COLON);
        temp = stmt();

        Stmt->sibling = temp;
        Stmt = Stmt->sibling;

    }
    return seq;
}

TreeNode *stmt() {
    TreeNode *stmt = NULL;
    switch (token.getType()) {
        case ID:
            stmt = assignStmt();
            break;
        case IF:
            stmt = ifStmt();
            break;
        case REPEAT:
            stmt = repeatStmt();
            break;
        case WRITE:
            stmt = writeStmt();
            break;
        case READ:
            stmt = readStmt();
            break;
        default:
            check(token.getType());
            displayError("Wrong statement in line " + to_string(compiler.in_file.cur_line_num));
            break;
    }
    return stmt;
}

TreeNode *ifStmt() {

    TreeNode *stmt = new TreeNode();
    stmt->node_kind = IF_NODE;

    stmt->line_num = compiler.in_file.cur_line_num;
    stmt->id = "IF";
    check(IF);
    stmt->child[0] = expr();

    check(THEN);
    stmt->child[1] = stmtSeq();

    if (token.getType() == ELSE) {
        check(ELSE);
        stmt->child[2] = stmtSeq();
    }

    check(END);
    return stmt;
}

TreeNode *repeatStmt() {
    TreeNode *stmt = new TreeNode();
    stmt->node_kind = REPEAT_NODE;

    check(REPEAT);
    stmt->child[0] = stmtSeq();

    check(UNTIL);
    stmt->child[1] = expr();

    return stmt;
}

TreeNode *assignStmt() {
    TreeNode *iden = new TreeNode();
    iden->node_kind = ID_NODE;
    iden->id = (char *) malloc(MAX_LINE_LENGTH);
    strcpy(iden->id, token.next_token.str);

    check(ID);
    TreeNode *assignNode = new TreeNode();

    // (:=)
    check(ASSIGN);
    assignNode->oper = ASSIGN;
    assignNode->node_kind = ASSIGN_NODE;

    assignNode->id = (char *) malloc(MAX_LINE_LENGTH);
    strcpy(assignNode->id, iden->id);

    assignNode->child[0] = expr();
    return assignNode;
}

TreeNode *readStmt() {
    TreeNode *readNode = new TreeNode();
    readNode->node_kind = READ_NODE;

    check(READ);
    if (token.getType() == ID)
    {
        readNode->id = (char *) malloc(MAX_LINE_LENGTH);
        strcpy(readNode->id,token.next_token.str);
    }
    check(ID);

    return readNode;
}

TreeNode *writeStmt() {
    TreeNode *writeNode = new TreeNode();
    writeNode->node_kind = WRITE_NODE;

    check(WRITE);
    writeNode->child[0] = expr();

    return writeNode;
}

TreeNode *expr()  { // (<|=)    //left
    TreeNode *mExprNode = mathExpr();
    TreeNode *root = new TreeNode();

    int cld = 0;
    root->child[cld++] = mExprNode;

    while ((token.getType() != ENDFILE) && (token.getType() != END)
           && (token.getType() != ELSE) && (token.getType() != UNTIL) &&
           (token.getType() == EQUAL || token.getType() == LESS_THAN))
    {
        TreeNode *operNode = new TreeNode();
        operNode->oper = token.getType();
        operNode->node_kind = OPER_NODE;
        root->child[cld++] = operNode;
        check(token.getType());

        mExprNode = factor();
        root->child[cld++] = mExprNode;
    }

    if(cld>1)
    {
        TreeNode *prev = root->child[0];

        for (int i=1; i<cld; i+=2)
        {
            TreeNode *oper = root->child[i];
            TreeNode *curr = root->child[i+1];

            oper->child[0] = prev;
            oper->child[1] = curr;

            prev = oper;
        }
        return prev;
    }
    return mExprNode;
}

TreeNode *mathExpr() { //  { (+|-) term }   //Left
    TreeNode *termNode = term();
    TreeNode *root = new TreeNode();

    int cld = 0;
    root->child[cld++] = termNode;

    while ((token.getType() != ENDFILE) && (token.getType() != END)
           && (token.getType() != ELSE) && (token.getType() != UNTIL) &&
           (token.getType() == MINUS || token.getType() == PLUS))
    {
        TreeNode *operNode = new TreeNode();
        operNode->oper = token.getType();
        operNode->node_kind = OPER_NODE;
        root->child[cld++] = operNode;
        check(token.getType());

        termNode = factor();
        root->child[cld++] = termNode;
    }

    if(cld>1)
    {
        TreeNode *prev = root->child[0];

        for (int i=1; i<cld; i+=2)
        {
            TreeNode *oper = root->child[i];
            TreeNode *curr = root->child[i+1];

            oper->child[0] = prev;
            oper->child[1] = curr;

            prev = oper;
        }
        return prev;
    }
    return termNode;
}

TreeNode *term() { //  { (*|/) term }   //left
    TreeNode *root = new TreeNode();
    TreeNode *factorNode = factor();

    int cld = 0;
    root->child[cld++] = factorNode;

    while ((token.getType() != ENDFILE) && (token.getType() != END)
           && (token.getType() != ELSE) && (token.getType() != UNTIL) &&
           (token.getType() == TIMES || token.getType() == DIVIDE))
    {
        TreeNode *operNode = new TreeNode();
        operNode->oper = token.getType();
        operNode->node_kind = OPER_NODE;
        root->child[cld++] = operNode;
        check(token.getType());

        factorNode = factor();
        root->child[cld++] = factorNode;
    }

    if(cld>1)
    {
        TreeNode *prev = root->child[0];

        for (int i=1; i<cld; i+=2)
        {
            TreeNode *oper = root->child[i];
            TreeNode *curr = root->child[i+1];

            oper->child[0] = prev;
            oper->child[1] = curr;

            prev = oper;
        }
        return prev;
    }
    return factorNode;
}


TreeNode *factor() //  { ^ newExpr }  //Right
{
    TreeNode *newExpNode = newExpr();
    TreeNode *seq = newExpNode;


    if((token.getType() != ENDFILE) && (token.getType() != END)
       && (token.getType() != ELSE) && (token.getType() != UNTIL) &&
       (token.getType() == POWER))
    {
        TreeNode *operNode = new TreeNode();
        operNode->oper = POWER;
        operNode->node_kind = OPER_NODE;
        check(POWER);

        operNode->child[0] = newExpNode;
        operNode->child[1] = factor();
        return operNode;
    }
    return seq;
}

TreeNode *newExpr() {
    TreeNode *expr = NULL;
    switch (token.getType()) {
        case LEFT_PAREN:
            check(LEFT_PAREN);
            expr = mathExpr();
            check(RIGHT_PAREN);
            break;
        case NUM:
            expr = new TreeNode();
            expr->node_kind = NUM_NODE;
            expr->expr_data_type = INTEGER;
            expr->num = atoi(token.next_token.str);
            check(NUM);
            break;
        case ID:
            expr = new TreeNode();
            expr->node_kind = ID_NODE;
            expr->id = (char *) malloc(MAX_LINE_LENGTH);
            strcpy(expr->id, token.next_token.str);
            check(ID);
            break;
        default:
            displayError("Wrong Identifier " + to_string(compiler.in_file.cur_line_num));
    }
    return expr;
}

//Driver function
TreeNode *parse() {
    TreeNode *root;
    GetNextToken(&compiler, &token.next_token);
    root = stmtSeq();
    PrintTree(root);

    if (token.getType() != ENDFILE)
        displayError("Incomplete syntax" + to_string(compiler.in_file.cur_line_num));

    return root;
}

int main()
{
    try
    {
        parse();
    }
    catch (int status)
    {
        return status;
    }

    return 0;
}





