#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<ctype.h>
#include<math.h>
#include"header.h"

#define TABLE_SIZE	256

symtab * hash_table[TABLE_SIZE];
symtab * sorted_table[58];
extern int linenumber;

int HASH(char * str){
	int idx=0;
	while(*str){
		idx = idx << 1;
		idx+=*str;
		str++;
	}	
	return (idx & (TABLE_SIZE-1));
}

/*returns the symbol table entry if found else NULL*/

symtab * lookup(char *name){
	int hash_key;
	symtab* symptr;
	if(!name)
		return NULL;
	hash_key=HASH(name);
	symptr=hash_table[hash_key];

	while(symptr){
		if(!(strcmp(name,symptr->lexeme)))
			return symptr;
		symptr=symptr->front;
	}
	return NULL;
}


void insertID(char *name){
	symtab* symptr=(symtab*)malloc(sizeof(symtab));
	strcpy(symptr->lexeme,name);
	symptr->line=linenumber;
	symptr->counter=1;

	int hash_key=hash_key=HASH(name);
	symtab* ptr=hash_table[hash_key];
	
	if(ptr==NULL){
		/*first entry for this hash_key*/
		hash_table[hash_key]=symptr;
		symptr->front=NULL;
		symptr->back=symptr;
	}
	else{
		symptr->front=ptr;
		ptr->back=symptr;
		symptr->back=symptr;
		hash_table[hash_key]=symptr;	
	}

	int sort_key=(*name)-65;
	symtab* current=sorted_table[sort_key];
	symtab* pre=sorted_table[sort_key];
	if(current==NULL){
		sorted_table[sort_key]=symptr;
		symptr->next=NULL;
	}
	else{
		if(strcmp(current->lexeme,name)<0){
			pre=current;
			current=current->next;
		}
		else{
			symtab* temp = sorted_table[sort_key];
			sorted_table[sort_key]=symptr;
			symptr->next=temp;
			return;
		}
		while(current!=NULL){
			if(strcmp(current->lexeme,name)<0){
				pre=current;
				current=current->next;
				continue;
			}
			else{
				pre->next=symptr;
				symptr->next=current;	
				return;
			}
		}
		pre->next=symptr;
		symptr->next=NULL;
	}
}

void printSym(symtab* ptr) 
{
	printf(" Name = %s \n", ptr->lexeme);
	printf(" References = %d \n", ptr->counter);
}

void printSymTab()
{
    int i;
    printf("----- Symbol Table ---------\n");
    for (i=0; i<TABLE_SIZE; i++)
    {
        symtab* symptr;
		symptr = hash_table[i];
		while (symptr != NULL)
		{
			printf("====>  index = %d \n", i);
			printSym(symptr);
			symptr=symptr->front;
		}
    }
}

void printSortTab()
{
    int i;
    printf("----- Symbol Table ---------\n");
    for (i=0; i<58; i++)
    {
        symtab* symptr;
		symptr = sorted_table[i];
		while (symptr != NULL)
		{
			printf("====>  index = %d \n", i);
			printSym(symptr);
			symptr=symptr->next;
		}
    }
}
