%option noyywrap 
%{
#include <stdio.h>
#include "header.h"
int linenumber;
symtab * lookup();
symtab * ptr;
void insertID();
void printSymTab();
void printSortTab();
/* You need to define for all tokens in C--, here are some examples */
#define INT		   10
#define FLOAT 		11 
#define OP_ASSIGN 12 
#define OP_OR 		13 
#define MK_LPAREN 14 
#define MK_RPAREN 15 
#define ERROR 		100 

%}

/*reserved word*/
return   "return"
typedef  "typedef"
if       "if"
else     "else"
int      "int"
float    "float"
for      "for"
void     "void"
while    "while"

letter       [A-Za-z]
digit        [0-9]
ID	          {letter}({letter}|{digit}|"_")*
WS	          [ \t]+
Int_constant {digit}+

/* You need to define the following RE's*/
Float_constant  ({digit}+(\.{digit}+)?("e"("+"|"-")?{digit}+)?)|({digit}+)?(\.{digit}+)("e"("+"|"-")?{digit}+)?|({digit}+)("e"("+"|"-")?{digit}+)
String_constant \".*\"
comment         [/][*][^*]*[*]+([^*/][^*]*[*]+)*[/]


/* Other operators appear here */
OP_assign  "="
OP_or      "||"
OP_add     "+"
OP_sub     "-"
OP_div     "/"
OP_mul     "*"
OP_smaller "<"
OP_greater ">"
OP_geq     ">="
OP_leq     "<="
OP_neq     "!="
OP_equal   "=="
OP_and     "&&"
OP_not     "!"

newline  "\n"

/* Other separators appear here */
DL_lparen "("
DL_rparen ")"
DL_lbrace "{"
DL_rbrace "}"
DL_lbracket "["
DL_rbracket "]"
DL_comma  ","
DL_semicol ";"
DL_dot  "."

error    .

%%
{return}            {/*printf("%s \n",yytext);*/}
{typedef}           {/*printf("%s \n",yytext);*/;}
{if}                {/*printf("%s \n",yytext);*/}
{else}              {/*printf("%s \n",yytext);*/}
{int}               {/*printf("%s \n",yytext);*/}
{float}             {/*printf("%s \n",yytext);*/}
{for}               {/*printf("%s \n",yytext);*/}
{void}              {/*printf("%s \n",yytext);*/}
{while}             {/*printf("%s \n",yytext);*/}
{ID}  { 
			/*printf("%s \n",yytext);*/
         ptr = lookup(yytext);
			if (ptr == NULL)
			     insertID(yytext);	
			else 
			     ptr->counter++;
		}
{WS}                {/*printf("%s \n",yytext);*/}
{Int_constant}      {/*printf("%s \n",yytext); return INT; */ }
{Float_constant}    {/*printf("%s \n",yytext); return FLOAT; */}
{String_constant}   {/*printf("%s \n",yytext);*/}
{comment}           {printf("%s \n",yytext);}
{OP_assign}         {/*printf("%s \n",yytext); return OP_ASSIGN; */}
{OP_or}             {/*printf("%s \n",yytext); return OP_OR; */}
{OP_add}            {/*printf("%s \n",yytext); return OP_add; */}
{OP_sub}            {/*printf("%s \n",yytext); return OP_sub; */}
{OP_div}            {/*printf("%s \n",yytext); return OP_div; */}
{OP_mul}            {/*printf("%s \n",yytext); return OP_mul; */}
{OP_smaller}        {/*printf("%s \n",yytext); return OP_smaller; */}
{OP_greater}        {/*printf("%s \n",yytext); return OP_greater; */}
{OP_geq}            {/*printf("%s \n",yytext); return OP_geq; */}
{OP_leq}            {/*printf("%s \n",yytext); return OP_leq; */}
{OP_neq}            {/*printf("%s \n",yytext); return OP_neq; */}
{OP_equal}          {/*printf("%s \n",yytext); return OP_equal; */}
{OP_and}            {/*printf("%s \n",yytext); return OP_and; */}
{OP_not}            {/*printf("%s \n",yytext); return OP_not; */}
{newline}           {/*printf("%s \n",yytext);*/ linenumber += 1;}
{DL_lparen}         {/*printf("%s \n",yytext); return MK_LPAREN; */}
{DL_rparen}         {/*printf("%s \n",yytext); return MK_RPAREN; */}
{DL_lbrace}         {/*printf("%s \n",yytext); return MK_LBRACE; */}
{DL_rbrace}         {/*printf("%s \n",yytext); return MK_RBRACE; */}
{DL_lbracket}       {/*printf("%s \n",yytext); return MK_LBRACKET; */}
{DL_rbracket}       {/*printf("%s \n",yytext); return MK_RBRACKET; */} 
{DL_comma}          {/*printf("%s \n",yytext);*/}
{DL_semicol}        {/*printf("%s \n",yytext);*/}
{DL_dot}            {/*printf("%s \n",yytext);*/}

{error}         printf("ERR (%s) \n",yytext);/* return ERROR; */


%%

int main(int argc, char **argv)
{
   argc--; ++argv;
   if (argc > 0)
      yyin = fopen(argv[0], "r");
   else
      yyin = stdin;
   yylex();
   //printSymTab();
   printSortTab();
   return 0;
}