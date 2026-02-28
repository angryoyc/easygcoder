
//Cont_t* line_milling( double x1, double y1, double x2, double y2, double R );
//void line_milling( double x1, double y1, double x2, double y2, double R, Cont_t** cont1);
void line_milling( double x1, double y1, double x2, double y2, double R, int dir, Cont_t** cont1);

//Cont_t* arc_milling(Arc_t* arc, double r);
//void arc_milling(Arc_t* arc, double r, Cont_t** cont1, Cont_t** cont2);
void arc_milling(Arc_t* arc, double r, int dir, Cont_t** cont1, Cont_t** cont2);

Cont_t* ra_line( double x1, double y1, double x2, double y2, double W, double H, int _dir );
//void outline_milling(Cont_t* cont, double r);
//Context_t* outline_milling(Cont_t* cont, double r);
void outline_milling(Cont_t* cont, Context_t* ctx_dst, double r);



