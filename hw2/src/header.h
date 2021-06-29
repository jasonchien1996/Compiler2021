struct symtab{
	char lexeme[256];
	struct symtab *front;
	struct symtab *back;
	struct symtab *next;
	int line;
	int counter;
};

typedef struct symtab symtab;
symtab * lookup(char *name);
void insertID(char *name);
