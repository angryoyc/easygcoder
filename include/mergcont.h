typedef struct {
	double dx, dy;  // вектор из точки P
	double k;       // параметр кривизны (для линии = 0, для дуги 1/R)
} Vector_t;

typedef struct {
	double dt;       // угол между векторами
	double dk;       // delta k
} Factor_t;


double angle_factor(Point_t* from, Point_t* at, Point_t* to);

//Vector_t line2vect(Line_t* line, Point_t* p);
//Vector_t line2vect( Line_t* line );

Vector_t line2vect( Line_t* line, Point_t* p, uint8_t inout );

Vector_t arc2vect(Arc_t* arc, Point_t* p, uint8_t inout);

double angle_between_vectors( Vector_t v1, Vector_t v2 );
//void find_areas( Refholder_t* souce_list, Refholder_t** list );
void find_areas( Refholder_t* souce_list );

//Vector_t item2vect( Refitem_t* item, Point_t* p );
//Vector_t item2vect( Refitem_t* item, char* dst );
Vector_t item2vect( Refitem_t* item, Point_t* p, uint8_t inout );

Factor_t factor_between_vectors( Vector_t v1, Vector_t v2 );

uint8_t factor_lt_factor( Factor_t left, Factor_t right );
