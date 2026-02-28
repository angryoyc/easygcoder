#ifndef EPSILON_H
#define EPSILON_H
extern const double epsilon;
#endif

struct Arc{
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
	Point_t center;
	double R;
	int dir;
};

Arc_t* create_arc( double x, double y, double R );
uint8_t remove_arc( Arc_t** ptr );
uint8_t arc_dir( Arc_t* arc, Point_t* a, Point_t* b, Point_t* c );
uint8_t is_p_on_arc( Arc_t* arc, Point_t* p);
uint8_t is_p_on_arcs( Arc_t* arc1, Arc_t* arc2, Point_t* p);
uint8_t is_xy_on_arc( Arc_t* arc, double x, double y);

uint8_t break_the_circle(Arc_t* arc, Point_t* p, int dir);
Arc_t* split_arc_by_p(Arc_t* arc, Point_t* p);
void arc_get_bounds(const Arc_t* arc, double* xmin, double* xmax, double* ymin, double* ymax);

//   void create_p_of_arc_x_arc(Arc_t* c1, Arc_t* c2, Point_t** p1, Point_t** p2 );

uint8_t xy_of_arc_x_arc( Arc_t* c1, Arc_t* c2, double* x1, double* y1, double* x2, double* y2 );
uint8_t create_p_of_arc_x_arc(Arc_t* arc1, Arc_t* arc2, Point_t** p1, Point_t** p2 );

void create_p_of_line_x_arc(Line_t* l, Arc_t* crcl, Point_t** p1, Point_t** p2);
uint8_t xy_of_line_x_arc(double ax, double ay, double bx, double by, Arc_t* crcl, double* p1x, double* p1y, double* p2x, double* p2y );

void replace_arc_a( Arc_t* arc, Point_t* p);
void replace_arc_b( Arc_t* arc, Point_t* p);
uint8_t is_arc(Refitem_t* item);
