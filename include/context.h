
typedef struct {
	Reflist_t links;
	Obj_Type_t type;
	int (*onunlink)( Refitem_t* self, Refitem_t* ref );
	char name[32];
	double xmax;           // максимальное значение x в данном контексте
	double xmin;           // Минимальное значение x в данном контексте
	double ymax;           // максимальное значение y в данном контексте
	double ymin;           // Минимальное значение y в данном контексте
} Context_t;

Context_t* create_context(const char *name);
Context_t* select_context(const char *name);
Context_t* get_context();
int free_context_by_name(const char *name);
Refholder_t* split_all(int debug);
void calc_contcount4all( int new_cont_id, int debug );

void context_get_bounds();
uint8_t find_all_conts();

void clean_all_cont();
int8_t copy_ctx2ctx( char* src, char* dst, Cont_t* src_cont );

uint8_t obhod( Refitem_t* _next, Cont_t* cont );
Cont_t* first_cont( Context_t* _ctx );
void clean_contcount( Context_t* _ctx );
void marking_of_imposed();

void fix_single_arc_cont( Cont_t* cont );
Arc_t* is_single_arc_cont( Cont_t* cont );
void clean_conts_by_list(Refholder_t** list);
void marking_of_imposed_by_list( Refholder_t* list );

