struct Point{
	Reflist_t links;
	Obj_Type_t type;
	int (*onunlink)(Refitem_t* self, Refitem_t* ref);
	Cont_t* cont;
	int contcount;
	double x;
	double y;
};

Point_t* create_p(double x, double y);
uint8_t remove_p( Point_t** ptr );
uint8_t p_eq_p(Point_t* p1, Point_t* p2);
uint8_t p_ne_p(Point_t* p1, Point_t* p2);
