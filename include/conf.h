
struct pair{
	char *key;
	char *value;
};

struct pair** conf_init();
struct pair** conf_push( struct pair**, char*, char* );
void conf_show( struct pair** );
void conf_free( struct pair** );
char* conf_get( struct pair**, char* );
char** conf_aget( struct pair**, char*);
struct pair** conf_load(char *);






