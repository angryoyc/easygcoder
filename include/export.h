
uint8_t get_svg_arc_flags(Arc_t* arc);

void walk_around_cont_points( Point_t* p );

//void walk_around_all_cont(const char* mod, int M, FILE* output_fd );
//void walk_around_all_cont(const char* mod, Svg_env_t* env, FILE* output_fd );
void walk_around_all_cont( const char* mod, FILE* output_fd );

void walk_around_all_points();
void walk_around_all_prims();

void print_arc( Arc_t*  );
void print_line( Line_t* );
void print_p( Point_t* );
//void walk_around_cont( Cont_t* cont, const char* mod, Svg_env_t* env, FILE* output_fd );

//void svg_point(double x, double y, Svg_env_t* env, FILE* output_fd );
//void svg_line(double x1, double y1, double x2, double y2, int M);
//void svg_line(double x1, double y1, double x2, double y2, Svg_env_t* env, FILE* output_fd );
//void svg_arc(Arc_t* arc, Svg_env_t* env);
//void print_vector( Vector_t* v);
void print_item( Refitem_t* item );

void boring_gcode( FILE* output_fd, Excellon_t* exc );
