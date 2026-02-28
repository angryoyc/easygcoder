struct Line{
	Reflist_t links;
	Obj_Type_t type;
	int (*onunlink)( Refitem_t* self, Refitem_t* ref );
	Cont_t* cont;
	Cont_t* cont_r;
	Cont_t* cont_l;
	int inshadow;      // признак сегмента, подлежащего удалению по результатам разбора совпадающих сегментов
	int contcount;
	int contcount1;
	int contcount2;
	Point_t* a;
	Point_t* b;
	int id;
	double A;
	double B;
	double C;
	double len;
};

Line_t* create_line( Point_t* a, Point_t* b );
uint8_t remove_line( Line_t** ptr );
uint8_t is_p_on_line(Point_t* a, Point_t* b, Point_t* p);
Line_t* split_line_by_p(Line_t* l, Point_t* p);
Point_t* create_p_of_line_mid(Point_t* a, Point_t* b);

uint8_t create_p_of_line_x_line(Point_t* a, Point_t* b, Point_t* c, Point_t* d, Point_t** p1, Point_t** p2);

void line_get_bounds(const Line_t* l, double* xmin, double* xmax, double* ymin, double* ymax);

void replace_a( Line_t* l, Point_t* p);
void replace_b( Line_t* l, Point_t* p);
void replace_same_p2lines( Line_t* l1, Line_t* l2 );

uint8_t is_line(Refitem_t* item);
