#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include "header.h"


int main( int argc, char *argv[] )
{
    FILE *source, *target;
    Program program;
    SymbolTable symtab;

    if( argc == 3){
        source = fopen(argv[1], "r");
        target = fopen(argv[2], "w");
        if( !source ){
            printf("can't open the source file\n");
            exit(2);
        }
        else if( !target ){
            printf("can't open the target file\n");
            exit(2);
        }
        else{
            program = parser(source);
            fclose(source);
            symtab = build(program);
            check(&program, &symtab);
            gencode(program, target);
        }
    }
    else{
        printf("Usage: %s source_file target_file\n", argv[0]);
    }


    return 0;
}


/********************************************* 
  Scanning 
 *********************************************/
Token getNumericToken( FILE *source, char c )
{
    Token token;
    int i = 0;

    while( isdigit(c) ) {
        token.tok[i++] = c;
        c = fgetc(source);
    }

    if( c != '.' ){
        ungetc(c, source);
        token.tok[i] = '\0';
        token.type = IntValue;
        return token;
    }

    token.tok[i++] = '.';

    c = fgetc(source);
    if( !isdigit(c) ){
        ungetc(c, source);
        printf("Expect a digit : %c\n", c);
        exit(1);
    }

    while( isdigit(c) ){
        token.tok[i++] = c;
        c = fgetc(source);
    }

    ungetc(c, source);
    token.tok[i] = '\0';
    token.type = FloatValue;
    return token;
}

Token scanner( FILE *source )
{
    char c;
    Token token;

    while( !feof(source) ){
        c = fgetc(source);

        while( isspace(c) ) c = fgetc(source);

        if( isdigit(c) )
            return getNumericToken(source, c);

        token.tok[0] = c;
        if( islower(c) ){
            c = fgetc(source);
            if( !islower(c) ){
                if( token.tok[0] == 'f' )
                    token.type = FloatDeclaration;
                else if( token.tok[0] == 'i' )
                    token.type = IntegerDeclaration;
                else if( token.tok[0] == 'p' )
                    token.type = PrintOp;
                else
                    token.type = Alphabet;
                token.tok[1] = '\0';
                ungetc(c, source);
                //printf("token %s\n",token.tok);
                return token;
            }
            token.type = Alphabet;
            int i=1;
            do{
                token.tok[i++] = c;
                c = fgetc(source);
            } while( islower(c) );
            ungetc(c, source);
            token.tok[i] = '\0';
            //printf("token %s\n",token.tok);
            return token;
        }

        switch(c){
            case '=':
                token.type = AssignmentOp;
                return token;
            case '+':
                token.type = PlusOp;
                return token;
            case '-':
                token.type = MinusOp;
                return token;
            case '*':
                token.type = MulOp;
                return token;
            case '/':
                token.type = DivOp;
                return token;
            case EOF:
                token.type = EOFsymbol;
                token.tok[0] = '\0';
                return token;
            default:
                printf("Invalid character : %c\n", c);
                exit(1);
        }
    }

    token.tok[0] = '\0';
    token.type = EOFsymbol;
    return token;
}


/********************************************************
  Parsing
 *********************************************************/
Declaration parseDeclaration( FILE *source, Token token )
{
    Token token2;
    switch(token.type){
        case FloatDeclaration:
        case IntegerDeclaration:
            token2 = scanner(source);
            if (strcmp(token2.tok, "f") == 0 ||
                    strcmp(token2.tok, "i") == 0 ||
                    strcmp(token2.tok, "p") == 0) {
                printf("Syntax Error: %s cannot be used as id\n", token2.tok);
                exit(1);
            }
            return makeDeclarationNode( token, token2 );
        default:
            printf("Syntax Error: Expect Declaration %s\n", token.tok);
            exit(1);
    }
}

Declarations *parseDeclarations( FILE *source )
{
    Token token = scanner(source);
    Declaration decl;
    Declarations *decls;
    switch(token.type){
        case FloatDeclaration:
        case IntegerDeclaration:
            decl = parseDeclaration(source, token);
            decls = parseDeclarations(source);
            return makeDeclarationTree( decl, decls );
        case PrintOp:
        case Alphabet:
            for(int i=strlen(token.tok)-1;i>=0;i--) ungetc(token.tok[i], source);
            return NULL;
        case EOFsymbol:
            return NULL;
        default:
            printf("Syntax Error: Expect declarations %s\n", token.tok);
            exit(1);
    }
}

Expression *parseValue( FILE *source )
{
    Token token = scanner(source);
    Expression *value = (Expression *)malloc( sizeof(Expression) );
    value->leftOperand = value->rightOperand = NULL;

    switch(token.type){
        case Alphabet:
            (value->v).type = Identifier;
            strcpy((value->v).val.id, token.tok);
            break;
        case IntValue:
            (value->v).type = IntConst;
            (value->v).val.ivalue = atoi(token.tok);
            break;
        case FloatValue:
            (value->v).type = FloatConst;
            (value->v).val.fvalue = atof(token.tok);
            break;
        default:
            printf("Syntax Error: Expect Identifier or a Number %s\n", token.tok);
            exit(1);
    }

    return value;
}

Expression *parseTerm( FILE *source, Expression *lvalue )
{    
    if( !lvalue ){
        Expression *val = parseValue(source);
        return parseTerm(source,val);
    }
    Token token = scanner(source);
    Expression *rvalue;
    Expression *term;

    switch(token.type){
        case MulOp:            
            rvalue = parseValue(source);
            term = (Expression *)malloc( sizeof(Expression) );
            if( (lvalue->v).type == IntConst ) {
                if( (rvalue->v).type == IntConst ) {
                    term->leftOperand = term->rightOperand = NULL;
                    (term->v).type = IntConst;
                    (term->v).val.ivalue = (lvalue->v).val.ivalue * (rvalue->v).val.ivalue;
                    return parseTerm(source, term);
                }
                else if( (rvalue->v).type == FloatConst) {
                    term->leftOperand = term->rightOperand = NULL;
                    (term->v).type = FloatConst;
                    (term->v).val.fvalue = (lvalue->v).val.ivalue * (rvalue->v).val.fvalue;
                    return parseTerm(source, term);
                }                
            }
            else if( (lvalue->v).type == FloatConst ) {
                if( (rvalue->v).type == IntConst ) {
                    term->leftOperand = term->rightOperand = NULL;
                    (term->v).type = FloatConst;
                    (term->v).val.fvalue = (lvalue->v).val.fvalue * (rvalue->v).val.ivalue;
                    return parseTerm(source, term);
                }
                else if( (rvalue->v).type == FloatConst) {
                    term->leftOperand = term->rightOperand = NULL;
                    (term->v).type = FloatConst;
                    (term->v).val.fvalue = (lvalue->v).val.fvalue * (rvalue->v).val.fvalue;
                    return parseTerm(source, term);
                }    
            }
            (term->v).type = MulNode;
            (term->v).val.op = Mul;
            term->leftOperand = lvalue;
            term->rightOperand = rvalue;
            return parseTerm(source, term);
        case DivOp:            
            rvalue = parseValue(source);
            term = (Expression *)malloc( sizeof(Expression) );
            if( (lvalue->v).type == IntConst ) {
                if( (rvalue->v).type == IntConst ) {
                    if((rvalue->v).val.ivalue == 0){
                        printf("Error: divided by zero");
                        exit(1);
                    }
                    term->leftOperand = term->rightOperand = NULL;
                    if( (((lvalue->v).val.ivalue)%((rvalue->v).val.ivalue))==0 ) {
                        (term->v).type = IntConst;
                        (term->v).val.ivalue = (lvalue->v).val.ivalue / (rvalue->v).val.ivalue;
                    }
                    else {
                        (term->v).type = FloatConst;
                        (term->v).val.fvalue = (float)(lvalue->v).val.ivalue / (rvalue->v).val.ivalue;
                    }
                    return parseTerm(source, term);
                }
                else if( (rvalue->v).type == FloatConst) {
                    if((rvalue->v).val.fvalue == 0){
                        printf("Error: divided by zero");
                        exit(1);
                    }
                    term->leftOperand = term->rightOperand = NULL;
                    (term->v).type = FloatConst;
                    (term->v).val.fvalue = (lvalue->v).val.ivalue / (rvalue->v).val.fvalue;
                    return parseTerm(source, term);
                }                
            }
            else if( (lvalue->v).type == FloatConst ) {
                if( (rvalue->v).type == IntConst ) {
                    if( (rvalue->v).val.ivalue == 0 ){
                        printf("Error: divided by zero");
                        exit(1);
                    }
                    term->leftOperand = term->rightOperand = NULL;
                    (term->v).type = FloatConst;
                    (term->v).val.fvalue = (lvalue->v).val.fvalue / (rvalue->v).val.ivalue;
                    return parseTerm(source, term);
                }
                else if( (rvalue->v).type == FloatConst) {
                    if( (rvalue->v).val.fvalue == 0 ){
                        printf("Error: divided by zero");
                        exit(1);
                    }
                    term->leftOperand = term->rightOperand = NULL;
                    (term->v).type = FloatConst;
                    (term->v).val.fvalue = (lvalue->v).val.fvalue / (rvalue->v).val.fvalue;
                    return parseTerm(source, term);
                }    
            }
            (term->v).type = DivNode;
            (term->v).val.op = Div;
            term->leftOperand = lvalue;
            term->rightOperand = rvalue;
            return parseTerm(source, term);
        case Alphabet:
            for(int i=strlen(token.tok)-1;i>=0;i--) ungetc(token.tok[i], source);
            return lvalue;            
        case PrintOp:
        case PlusOp:
        case MinusOp:
            ungetc(token.tok[0], source);
            return lvalue;
        case EOFsymbol:
            return lvalue;
        default:
            printf("Syntax Error: Expect a numeric value or an identifier %s\n", token.tok);
            exit(1);
    }
}

Expression *parseExpressionTail( FILE *source, Expression *lvalue )
{
    Token token = scanner(source);
    Expression *expr;
    Expression *rvalue;

    switch(token.type){
        case PlusOp:
            rvalue = parseTerm(source, NULL);
            expr = (Expression *)malloc( sizeof(Expression) );
            if( (lvalue->v).type == IntConst ) {
                if( (rvalue->v).type == IntConst ) {
                    expr->leftOperand = expr->rightOperand = NULL;
                    (expr->v).type = IntConst;
                    (expr->v).val.ivalue = (lvalue->v).val.ivalue + (rvalue->v).val.ivalue;
                    return parseExpressionTail(source, expr);
                }
                else if( (rvalue->v).type == FloatConst) {
                    expr->leftOperand = expr->rightOperand = NULL;
                    (expr->v).type = FloatConst;
                    (expr->v).val.fvalue = (lvalue->v).val.ivalue + (rvalue->v).val.fvalue;
                    return parseExpressionTail(source, expr);
                }                
            }
            else if( (lvalue->v).type == FloatConst ) {
                if( (rvalue->v).type == IntConst ) {
                    expr->leftOperand = expr->rightOperand = NULL;
                    (expr->v).type = FloatConst;
                    (expr->v).val.fvalue = (lvalue->v).val.fvalue + (rvalue->v).val.ivalue;
                    return parseExpressionTail(source, expr);
                }
                else if( (rvalue->v).type == FloatConst) {
                    expr->leftOperand = expr->rightOperand = NULL;
                    (expr->v).type = FloatConst;
                    (expr->v).val.fvalue = (lvalue->v).val.fvalue + (rvalue->v).val.fvalue;
                    return parseExpressionTail(source, expr);
                }    
            }            
            (expr->v).type = PlusNode;
            (expr->v).val.op = Plus;
            expr->leftOperand = lvalue;
            expr->rightOperand = rvalue;
            return parseExpressionTail(source, expr);
        case MinusOp:
            rvalue = parseTerm(source, NULL);
            expr = (Expression *)malloc( sizeof(Expression) );
            if( (lvalue->v).type == IntConst ) {
                if( (rvalue->v).type == IntConst ) {
                    expr->leftOperand = expr->rightOperand = NULL;
                    (expr->v).type = IntConst;
                    (expr->v).val.ivalue = (lvalue->v).val.ivalue - (rvalue->v).val.ivalue;
                    return parseExpressionTail(source, expr);
                }
                else if( (rvalue->v).type == FloatConst) {
                    expr->leftOperand = expr->rightOperand = NULL;
                    (expr->v).type = FloatConst;
                    (expr->v).val.fvalue = (lvalue->v).val.ivalue - (rvalue->v).val.fvalue;
                    return parseExpressionTail(source, expr);
                }                
            }
            else if( (lvalue->v).type == FloatConst ) {
                if( (rvalue->v).type == IntConst ) {
                    expr->leftOperand = expr->rightOperand = NULL;
                    (expr->v).type = FloatConst;
                    (expr->v).val.fvalue = (lvalue->v).val.fvalue - (rvalue->v).val.ivalue;
                    return parseExpressionTail(source, expr);
                }
                else if( (rvalue->v).type == FloatConst) {
                    expr->leftOperand = expr->rightOperand = NULL;
                    (expr->v).type = FloatConst;
                    (expr->v).val.fvalue = (lvalue->v).val.fvalue - (rvalue->v).val.fvalue;
                    return parseExpressionTail(source, expr);
                }    
            }  
            (expr->v).type = MinusNode;
            (expr->v).val.op = Minus;
            expr->leftOperand = lvalue;
            expr->rightOperand = rvalue;
            return parseExpressionTail(source, expr);
        case Alphabet:
            for(int i=strlen(token.tok)-1;i>=0;i--) ungetc(token.tok[i], source);
            return lvalue;
        case PrintOp:
            ungetc(token.tok[0], source);
            return lvalue;
        case EOFsymbol:
            return lvalue;
        default:
            printf("Syntax Error: Expect a numeric value or an identifier %s\n", token.tok);
            exit(1);
    }
}

Expression *parseExpression( FILE *source, Expression *lvalue )
{
    Token token = scanner(source);
    Expression *expr;
    Expression *rvalue;

    switch(token.type){
        case PlusOp:
            rvalue = parseTerm(source, NULL);
            expr = (Expression *)malloc( sizeof(Expression) );
            if( (lvalue->v).type == IntConst ) {
                if( (rvalue->v).type == IntConst ) {
                    expr->leftOperand = expr->rightOperand = NULL;
                    (expr->v).type = IntConst;
                    (expr->v).val.ivalue = (lvalue->v).val.ivalue + (rvalue->v).val.ivalue;
                    return parseExpressionTail(source, expr);
                }
                else if( (rvalue->v).type == FloatConst) {
                    expr->leftOperand = expr->rightOperand = NULL;
                    (expr->v).type = FloatConst;
                    (expr->v).val.fvalue = (lvalue->v).val.ivalue + (rvalue->v).val.fvalue;
                    return parseExpressionTail(source, expr);
                }                
            }
            else if( (lvalue->v).type == FloatConst ) {
                if( (rvalue->v).type == IntConst ) {
                    expr->leftOperand = expr->rightOperand = NULL;
                    (expr->v).type = FloatConst;
                    (expr->v).val.fvalue = (lvalue->v).val.fvalue + (rvalue->v).val.ivalue;
                    return parseExpressionTail(source, expr);
                }
                else if( (rvalue->v).type == FloatConst) {
                    expr->leftOperand = expr->rightOperand = NULL;
                    (expr->v).type = FloatConst;
                    (expr->v).val.fvalue = (lvalue->v).val.fvalue + (rvalue->v).val.fvalue;
                    return parseExpressionTail(source, expr);
                }    
            }            
            (expr->v).type = PlusNode;
            (expr->v).val.op = Plus;
            expr->leftOperand = lvalue;
            expr->rightOperand = rvalue;
            return parseExpressionTail(source, expr);
        case MinusOp:
            rvalue = parseTerm(source, NULL);
            expr = (Expression *)malloc( sizeof(Expression) );
            if( (lvalue->v).type == IntConst ) {
                if( (rvalue->v).type == IntConst ) {
                    expr->leftOperand = expr->rightOperand = NULL;
                    (expr->v).type = IntConst;
                    (expr->v).val.ivalue = (lvalue->v).val.ivalue - (rvalue->v).val.ivalue;
                    return parseExpressionTail(source, expr);
                }
                else if( (rvalue->v).type == FloatConst) {
                    expr->leftOperand = expr->rightOperand = NULL;
                    (expr->v).type = FloatConst;
                    (expr->v).val.fvalue = (lvalue->v).val.ivalue - (rvalue->v).val.fvalue;
                    return parseExpressionTail(source, expr);
                }                
            }
            else if( (lvalue->v).type == FloatConst ) {
                if( (rvalue->v).type == IntConst ) {
                    expr->leftOperand = expr->rightOperand = NULL;
                    (expr->v).type = FloatConst;
                    (expr->v).val.fvalue = (lvalue->v).val.fvalue - (rvalue->v).val.ivalue;
                    return parseExpressionTail(source, expr);
                }
                else if( (rvalue->v).type == FloatConst) {
                    expr->leftOperand = expr->rightOperand = NULL;
                    (expr->v).type = FloatConst;
                    (expr->v).val.fvalue = (lvalue->v).val.fvalue - (rvalue->v).val.fvalue;
                    return parseExpressionTail(source, expr);
                }    
            }  
            (expr->v).type = MinusNode;
            (expr->v).val.op = Minus;
            expr->leftOperand = lvalue;
            expr->rightOperand = rvalue;
            return parseExpressionTail(source, expr);
        case Alphabet:
            for(int i=strlen(token.tok)-1;i>=0;i--) ungetc(token.tok[i], source);
            return NULL;
        case PrintOp:
            ungetc(token.tok[0], source);
            return NULL;
        case EOFsymbol:
            return NULL;
        default:
            printf("Syntax Error: Expect a numeric value or an identifier %s\n", token.tok);
            exit(1);
    }
}

Statement parseStatement( FILE *source, Token token )
{
    Token next_token;
    Expression *term, *expr;

    switch(token.type){
        case Alphabet:
            next_token = scanner(source);
            if(next_token.type == AssignmentOp){
                term = parseTerm(source, NULL);
                expr = parseExpression(source, term);
                return makeAssignmentNode(token.tok, term, expr);
            }
            else{
                printf("Syntax Error: Expect an assignment op %s\n", next_token.tok);
                exit(1);
            }
        case PrintOp:
            next_token = scanner(source);
            if(next_token.type == Alphabet)
                return makePrintNode(next_token.tok);
            else{
                printf("Syntax Error: Expect an identifier %s\n", next_token.tok);
                exit(1);
            }
            break;
        default:
            printf("Syntax Error: Expect a statement %s\n", token.tok);
            exit(1);
    }
}

Statements *parseStatements( FILE * source )
{

    Token token = scanner(source);
    Statement stmt;
    Statements *stmts;

    switch(token.type){
        case Alphabet:
        case PrintOp:
            stmt = parseStatement(source, token);
            stmts = parseStatements(source);
            return makeStatementTree(stmt , stmts);
        case EOFsymbol:
            return NULL;
        default:
            printf("Syntax Error: Expect statements %s\n", token.tok);
            exit(1);
    }
}


/*********************************************************************
  Build AST
 **********************************************************************/
Declaration makeDeclarationNode( Token declare_type, Token identifier )
{
    Declaration tree_node;

    switch(declare_type.type){
        case FloatDeclaration:
            tree_node.type = Float;
            break;
        case IntegerDeclaration:
            tree_node.type = Int;
            break;
        default:
            break;
    }
    strcpy(tree_node.name, identifier.tok);

    return tree_node;
}

Declarations *makeDeclarationTree( Declaration decl, Declarations *decls )
{
    Declarations *new_tree = (Declarations *)malloc( sizeof(Declarations) );
    new_tree->first = decl;
    new_tree->rest = decls;

    return new_tree;
}


Statement makeAssignmentNode( char *id, Expression *v, Expression *expr_tail )
{
    Statement stmt;
    AssignmentStatement assign;

    stmt.type = Assignment;
    strcpy(assign.id, id);
    if(expr_tail == NULL)
        assign.expr = v;
    else
        assign.expr = expr_tail;
    stmt.stmt.assign = assign;

    return stmt;
}

Statement makePrintNode( char *id )
{
    Statement stmt;
    stmt.type = Print;
    strcpy(stmt.stmt.variable, id);

    return stmt;
}

Statements *makeStatementTree( Statement stmt, Statements *stmts )
{
    Statements *new_tree = (Statements *)malloc( sizeof(Statements) );
    new_tree->first = stmt;
    new_tree->rest = stmts;

    return new_tree;
}

/* parser */
Program parser( FILE *source )
{
    Program program;

    program.declarations = parseDeclarations(source);
    program.statements = parseStatements(source);

    return program;
}


/********************************************************
  Build symbol table
 *********************************************************/
void InitializeTable( SymbolTable *table )
{
    int i;

    for(i = 0 ; i < 26; i++)
        table->table[i] = NULL;
}

void add_table( SymbolTable *table, char *c, DataType t )
{
    int index = (int)(c[0] - 'a');

    if( !table->table[index] ){
        table->table[index] = (Item *) malloc( sizeof(Item) );
        strcpy(table->table[index]->key,c);
        table->table[index]->type = t;
        table->table[index]->next = NULL;
        return;
    }

    Item *current = table->table[index];

    while(current->next != NULL){
        if( !strcmp(current->key,c) ){
            printf("Error : id %s has been declared\n", c);//error
            current->type=t;
            return;
        }
        current = current->next;        
    }
    if( !strcmp(current->key,c) ){
        printf("Error : id %s has been declared\n", c);//error
        current->type=t;
        return;
    }
    current->next = (Item *) malloc( sizeof(Item) );
    strcpy(current->next->key,c);
    current->next->type = t;
    current->next->next = NULL;
}

const char* getType(enum DataType type) {
   switch (type) 
   {
      case Int: return "Int";
      case Float: return "Float";
      case Notype: return "Notype";
      /* etc... */
   }
}

SymbolTable build( Program program )
{
    SymbolTable table;
    Declarations *decls = program.declarations;
    Declaration current;

    InitializeTable(&table);

    while(decls !=NULL){
        current = decls->first;
        add_table(&table, current.name, current.type);
        decls = decls->rest;
    }
    /*
    for(int i=0; i<26; i++){
        Item *current=table.table[i];
        while(current){
            printf("%s %s ", current->key, getType(current->type));
            current=current->next;
        }
        printf("None\n");
    }
    */
    return table;
}


/********************************************************************
  Type checking
 *********************************************************************/

void convertType( Expression * old, DataType type )
{
    if(old->type == Float && type == Int){
        printf("error : can't convert float to integer\n");
        return;
    }
    if(old->type == Int && type == Float){
        Expression *tmp = (Expression *)malloc( sizeof(Expression) );
        if(old->v.type == Identifier)
            printf("convert to float %s \n",old->v.val.id);
        else {
            switch(old->v.type){
                case FloatConst:
                    printf("convert to float FloatConst \n");
                    break;
                case PlusNode:
                    printf("convert to float PlusNode \n");
                    break;
                case MinusNode:
                    printf("convert to float MinusNode \n");
                    break;
                case MulNode:
                    printf("convert to float MulNode \n");
                    break;
                case DivNode:
                    printf("convert to float DivNode \n");
                    break;
            }
        }
        tmp->v = old->v;
        tmp->leftOperand = old->leftOperand;
        tmp->rightOperand = old->rightOperand;
        tmp->type = old->type;

        Value v;
        v.type = IntToFloatConvertNode;
        v.val.op = IntToFloatConvert;
        old->v = v;
        old->type = Int;
        old->leftOperand = tmp;
        old->rightOperand = NULL;
    }
}

DataType generalize( Expression *left, Expression *right )
{
    if(left->type == Float || right->type == Float){
        printf("generalize : float\n");
        return Float;
    }
    printf("generalize : int\n");
    return Int;
}

DataType lookup_table( SymbolTable *table, char *c )
{
    int id = c[0]-'a';
    Item *current = table->table[id];
    //printf("search %s\n",c);
    while( current ){
        if( !strcmp(current->key,c) ){
            //printf("found %s\n", current->key);
            return current->type;
        }
        current = current->next;
    }
    printf("Error : identifier %s is not declared\n", c);//error
    return Notype;
}

void checkexpression( Expression * expr, SymbolTable * table )
{
    char c[64];
    if(expr->leftOperand == NULL && expr->rightOperand == NULL){
        switch(expr->v.type){
            case Identifier:
                strcpy(c,expr->v.val.id);
                printf("identifier : %s\n",c);
                expr->type = lookup_table(table, c);
                break;
            case IntConst:
                printf("constant : int\n");
                expr->type = Int;
                break;
            case FloatConst:
                printf("constant : float\n");
                expr->type = Float;
                break;
                //case PlusNode: case MinusNode: case MulNode: case DivNode:
            default:
                break;
        }
    }
    else{
        Expression *left = expr->leftOperand;
        Expression *right = expr->rightOperand;

        checkexpression(left, table);
        checkexpression(right, table);

        DataType type = generalize(left, right);
        convertType(left, type);//left->type = type;//converto
        convertType(right, type);//right->type = type;//converto
        expr->type = type;
    }
}


void checkstmt( Statement *stmt, SymbolTable * table )
{
    if(stmt->type == Assignment){
        AssignmentStatement assign = stmt->stmt.assign;
        printf("assignment : %s \n",assign.id);
        checkexpression(assign.expr, table);//set expressrion type
        stmt->stmt.assign.type = lookup_table(table, assign.id);
        //printf("1\n");
        
        if (assign.expr->type == Float && stmt->stmt.assign.type == Int) {
            printf("error : can't convert float to integer\n");
        } else {
            convertType(assign.expr, stmt->stmt.assign.type);
        }
    }
    else if (stmt->type == Print){
        printf("print : %s \n",stmt->stmt.variable);
        lookup_table(table, stmt->stmt.variable);
    }
    else printf("error : statement error\n");//error
}

void check( Program *program, SymbolTable * table )
{
    Statements *stmts = program->statements;
    while(stmts != NULL){
        checkstmt(&stmts->first,table);
        stmts = stmts->rest;
    }
}


/***********************************************************************
  Code generation
 ************************************************************************/
void fprint_op( FILE *target, ValueType op )
{
    switch(op){
        case MinusNode:
            fprintf(target,"-\n");
            break;
        case PlusNode:
            fprintf(target,"+\n");
            break;
        case MulNode:
            fprintf(target,"*\n");
            break;
        case DivNode:
            fprintf(target,"/\n");
            break;
        default:
            fprintf(target,"Error in fprintf_op ValueType = %d\n",op);
            break;
    }
}

void fprint_expr( FILE *target, Expression *expr)
{

    if(expr->leftOperand == NULL){
        switch( (expr->v).type ){
            case Identifier:
                fprintf(target,"l%s\n",(expr->v).val.id);
                break;
            case IntConst:
                fprintf(target,"%d\n",(expr->v).val.ivalue);
                break;
            case FloatConst:
                fprintf(target,"%f\n", (expr->v).val.fvalue);
                break;
            default:
                fprintf(target,"Error In fprint_left_expr. (expr->v).type=%d\n",(expr->v).type);
                break;
        }
    }
    else{
        fprint_expr(target, expr->leftOperand);
        if(expr->rightOperand == NULL){
            fprintf(target,"5 k\n");
        }
        else{
            //	fprint_right_expr(expr->rightOperand);
            fprint_expr(target, expr->rightOperand);
            fprint_op(target, (expr->v).type);
        }
    }
}

void gencode(Program prog, FILE * target)
{
    Statements *stmts = prog.statements;
    Statement stmt;

    while(stmts != NULL){
        stmt = stmts->first;
        switch(stmt.type){
            case Print:
                fprintf(target,"l%s\n",stmt.stmt.variable);
                fprintf(target,"p\n");
                break;
            case Assignment:
                fprint_expr(target, stmt.stmt.assign.expr);
                /*
                if(stmt.stmt.assign.type == Int){
                   fprintf(target,"0 k\n");
                }
                else if(stmt.stmt.assign.type == Float){
                   fprintf(target,"5 k\n");
                }
                */                
                fprintf(target,"s%s\n",stmt.stmt.assign.id);
                fprintf(target,"0 k\n");
                break;
        }
        stmts=stmts->rest;
    }
}


/***************************************
  For our debug,
  you can omit them.
 ****************************************/
void print_expr(Expression *expr)
{
    if(expr == NULL)
        return;
    else{
        print_expr(expr->leftOperand);
        switch((expr->v).type){
            case Identifier:
                printf("%s ", (expr->v).val.id);
                break;
            case IntConst:
                printf("%d ", (expr->v).val.ivalue);
                break;
            case FloatConst:
                printf("%f ", (expr->v).val.fvalue);
                break;
            case PlusNode:
                printf("+ ");
                break;
            case MinusNode:
                printf("- ");
                break;
            case MulNode:
                printf("* ");
                break;
            case DivNode:
                printf("/ ");
                break;
            case IntToFloatConvertNode:
                printf("(float) ");
                break;
            default:
                printf("error ");
                break;
        }
        print_expr(expr->rightOperand);
    }
}

void test_parser( FILE *source )
{
    Declarations *decls;
    Statements *stmts;
    Declaration decl;
    Statement stmt;
    Program program = parser(source);

    decls = program.declarations;

    while(decls != NULL){
        decl = decls->first;
        if(decl.type == Int)
            printf("i ");
        if(decl.type == Float)
            printf("f ");
        printf("%s ",decl.name);
        decls = decls->rest;
    }

    stmts = program.statements;

    while(stmts != NULL){
        stmt = stmts->first;
        if(stmt.type == Print){
            printf("p %s ", stmt.stmt.variable);
        }

        if(stmt.type == Assignment){
            printf("%s = ", stmt.stmt.assign.id);
            print_expr(stmt.stmt.assign.expr);
        }
        stmts = stmts->rest;
    }

}
