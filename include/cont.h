struct Cont {               // Контур - группа примитивов
	Reflist_t links;
	Obj_Type_t type;
	int (*onunlink)( Refitem_t* self, Refitem_t* ref );
	int num;                // номер контура
	double xmax;            // максимальное значение x в данном контексте
	double xmin;            // Минимальное значение x в данном контексте
	double ymax;            // максимальное значение y в данном контексте
	double ymin;            // Минимальное значение y в данном контексте
	int mincc;              // минимальное значение contcount в контуре
	int dir;				// направление контура
};

Cont_t* create_cont();
uint8_t remove_cont( Cont_t** ptr );
uint8_t purge_cont( Cont_t** ptr );

Refitem_t* add_item2cont( Refitem_t* item, Cont_t* cont );
Refitem_t* add_item2cont_r( Refitem_t* item, Cont_t* cont );
Refitem_t* add_item2cont_l( Refitem_t* item, Cont_t* cont );

void rm_item2cont( Refitem_t* item );
void rm_item2cont_r( Refitem_t* item );
void rm_item2cont_l( Refitem_t* item );

int split_cont_by_cont(Cont_t* cont1, Cont_t* cont2, int debug);
void cont_get_bounds( Cont_t* cont );
uint8_t is_prim_in_cont( Refitem_t* item, Cont_t* cont, int flag );
void clean_cont( Cont_t* cont );
void cont_orienting(Cont_t* cont, int dir);

uint8_t is_xy_on_cont( double x, double y, Obj_Type_t type, Cont_t* cont );
int cont_dir(Cont_t* cont);
void cont_set_dir( Cont_t* cont, int dir );
int cont_reorder( Cont_t* cont, int _dir );

void put_after( Refitem_t* newitem, Cont_t* cont, Refitem_t* olditem );

void remove_conts_by_list( Refholder_t** list );
int is_seg( Refitem_t* item );
int is_cont( Refitem_t* item );



