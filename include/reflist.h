#define MAXLEN 20

typedef struct Refholder Refholder_t;

struct Refholder{
	Refitem_t* refitem;
	Refholder_t* next;
};

struct Refitem{
	Reflist_t links;
	Obj_Type_t type;
	int (*onunlink)(Refitem_t* self, Refitem_t* ref);
	Cont_t* cont;
	Cont_t* cont_r;
	Cont_t* cont_l;
	int inshadow;      // признак сегмента, подлежащего удалению по результатам разбора совпадающих сегментов
	int contcount;
	int contcount1;
	int contcount2;
};

void* linkobj( Reflist_t* reflist, void* _ref);
void unlinkobj( Reflist_t* reflist, void* ref );
void* linkobj2obj( void* ref1, void* ref2);
void purge_obj(Refitem_t** _ref);
Refholder_t* unlinkyouself(Refitem_t* ref);
int unlinkobj2obj( void* _ref1, void* _ref2);
void purge_by_list( Refholder_t* list, Refitem_t** _ref );
uint8_t is_linked( Refitem_t* obj1, Refitem_t* obj2);
void cp_links( Refitem_t* src, Refitem_t* dst);
int obj_count(Refitem_t* item, Obj_Type_t type);
char* obj_type(Refitem_t* item);
Refitem_t* first_obj_by_type( Refitem_t* item, Obj_Type_t type );
void push2list( Refitem_t* item, Refholder_t** list );
int list_len( Refholder_t* list );


