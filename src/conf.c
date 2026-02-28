#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "../include/define.h"

struct pair{
	char *key;
	char *value;
};

struct pair** conf;
struct stack * stack_head = NULL;

struct stack {
	char ** result;
	struct stack *next;
};

struct stack *new_stack_el(struct stack* next, char ** result){
	struct stack *element;
	element = (struct stack *)malloc(sizeof(struct stack));
	element->result = result;
	element->next = next;
	return element;
}

struct pair* create(char* key, char* value){
	struct pair* p;
	p = (struct pair *) malloc(sizeof(struct pair));
	p->key = malloc(strlen(key) + 1);
	strcpy(p->key, key);
	p->value = malloc(strlen(value) + 1);
	strcpy(p->value, value);
	return p;
}


void conf_show( struct pair** rows ){
	int i = 0;
	while( rows[i]!=NULL ){
		printf( "[%s] = \"%s\"\n", rows[i]->key, rows[i]->value );
		i++;
	};
}

int alen(struct pair** rows){
	int i = 0;
	while( rows[i]!=NULL ){
		i++;
	}
	return i;
}

struct pair** conf_push_row( struct pair** rows, struct pair* row){
//	int i=0;
	int len = alen(rows);
	rows = (struct pair**)realloc(rows, sizeof(struct pair*) * (len+2));
	rows[len] = row;
	rows[len+1] = NULL;
	return rows;
}

struct pair** conf_push( struct pair** rows, char* key, char* value){
	struct pair* row = create(key, value);
	rows = conf_push_row( rows, row);
	return rows;
}

struct pair** conf_init(){
	struct pair** rows;
	rows = (struct pair**)malloc(sizeof(struct pair*));
	rows[0] = NULL;
	return rows;
}

void conf_free( struct pair** rows ){
	int i=0;
	while( rows[i]!=NULL ){
		free(rows[i]);
		i++;
	}
	free(rows);
	while(stack_head!=NULL){
		free(stack_head->result);
		stack_head=stack_head->next;
	}
}

char * conf_get( struct pair** rows, char * key ){ // Возвращает первый value с указанным key
	int i=0;
	while( rows[i]!=NULL ){
		if( strcmp(rows[i]->key, key)==0 ){
			return rows[i]->value;
		};
		i++;
	}
	return "";
}

char ** conf_aget( struct pair** rows, char * key ){ // Возвращает массив value с одинаковым key
	int i=0;
	int c=0;
	char ** result = NULL;
	result = (char **)realloc(result, sizeof(char*));
	result[0] = NULL;
	while( rows[i]!=NULL ){
		if( strcmp(rows[i]->key, key)==0 ){
			c++;
			result = (char **)realloc(result, sizeof(char*) * c+1);
			result[c-1] = malloc(strlen(rows[i]->value) + 1);
			strcpy( result[c-1], rows[i]->value );
			result[c] = NULL;
		};
		i++;
	}
	stack_head = new_stack_el(stack_head, result);
	return result;
}

char * trim(char *s){
	char *p = s + strlen(s) - 1;
	while (isascii(*s) && isspace(*s))
		s++;
	while (isascii(*p) && isspace(*p))
		*p-- = '\0';
	return s;
}

struct pair** rdconf(struct pair** conf, FILE *fd){
	char buf[1810], *s, *p;
	char *key, *value;
//	int i;
	while (fgets(buf, sizeof(buf), fd) != NULL) {
		s = trim(buf);
		if (*s == '#' || *s == '\0') continue; /* got comment or empty line? go ahead */
		p = strchr(s, '=');                    /* not a key=value pair? go ahead */
		if (!p) continue;
		*p++ = '\0';                           /* separate key and value */
		key = trim(s);
		value = trim(p);
		conf = conf_push( conf, key, value);
	}
	return conf;
}

struct pair** conf_load(char * configpath) {
	char* fpath;
	FILE* fptr;

	if( !configpath ){
		fpath = CONFIGPATH;
	}else{
		fpath = configpath;
	}
	if ( (fptr = fopen( fpath, "r") ) == NULL){
		printf("ERR: Configuration file opening error!\n");
		exit(1);
	}
	struct pair** conf = conf_init();
	conf = rdconf( conf, fptr );
	return conf;
}

/*
int main(){
	struct pair** conf = conf_load("./conf.conf");
//	struct pair** conf = conf_load(NULL);
//	conf_show( conf ); // show all config
//	printf("folder - > %s\n", conf_get(conf, "folder") ); // get and show single value for key=folder

	char ** res = conf_aget(conf, "broadcast");           // get and show string array on key=host values
	int i = 0;
	while(res[i]!=NULL){
		printf("broadcast-> %s\n", res[i] );
		i++;
	}
	conf_free( conf );                                      // free memory
	return 0;
}
*/