
typedef struct{
	int tool_num;
	double size_mm;
} Tool_t;


typedef struct{
	double x;
	double y;
	double d; // Диаметр инструмента
	Tool_t tool;
} Hole_t;


typedef struct{
	Tool_t* tools;
	int tools_count;
	int tools_size;
	Hole_t* holes;
	int holes_count;
	int holes_size;
} Excellon_t;


//void parse_drill_file(const char* filename, Context_t* ctx);
//void parse_drill_file(Svg_env_t* env, const char* filename, Context_t* ctx);
//void parse_drill_file(Svg_env_t* env, Excellon_t* exc, const char* filename, Context_t* ctx);
void parse_drill_file( Excellon_t* exc, const char* filename );
void print_holes( Excellon_t* exc );
void sort_holes( Excellon_t* exc );
void replace_drills( Excellon_t* exc );

