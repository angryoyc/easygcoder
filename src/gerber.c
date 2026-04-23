#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#include <ctype.h>
#include <string.h>
#include <math.h>
#include <locale.h>

#include "../include/define.h"
#include "../include/top.h"
#include "../include/conf.h"
#include "../include/reflist.h"
#include "../include/context.h"
#include "../include/point.h"
#include "../include/line.h"
#include "../include/cont.h"
#include "../include/geom.h"
#include "../include/milling.h"
#include "../include/mergcont.h"
#include "../include/excellon.h"
#include "../include/apply_config.h"

#include "../include/export.h"

#include "../include/main.h"

#define MAX_APERTURES 100
#define MAX_MACROS 100

// Структура для хранения координат
typedef struct {
	double x;
	double y;
} Coor;

// Структура для хранения апертуры
typedef struct {
    int number;
    char type;  // 'C' - круг, 'R' - прямоугольник, 'M' - макро
    double param1; // диаметр или ширина
    double param2; // высота (для прямоугольника)
    char macro[10];
	int points_len;
	Coor points[30];
} Aperture;


// Структура для хранения входных параметров макроса
typedef struct {
	int index;
	double value;
} Param;

// Структура для хранения macros
typedef struct {
	char macro[10];
//	int  len;           // убрать после переноса
//	Coor points[30];    // убрать после переноса
	int  params_len;
	Param params[30];
} Macro;

// Структура для хранения состояния парсера
typedef struct {
    double x, y;           // Текущая позиция
    int current_aperture;  // Текущая апертура
    int format_int;        // Цифр целой части
    int format_dec;        // Цифр дробной части
    int is_metric;         // 1 = мм, 0 = дюймы
    Aperture apertures[MAX_APERTURES];
    int aperture_count;
    Macro macros[MAX_MACROS];
    int macros_count;
} GerberState;

// Функции-заглушки для рисования
static int count = 0;


void grb_center_punching(double _x, double _y, int debug){
	int mirror_x = env_i("mirror_x");
	int mirror_y = env_i("mirror_y");
	double x = round_to_decimal(_x, 2) * (mirror_y?-1:1);
	double y = round_to_decimal(_y, 2) * (mirror_x?-1:1);
	double d = env_d("tool_d");
	Cont_t* cont = NULL;
	punch_milling( x, y, round_to_decimal(d, 2), -1, &cont );
}


void grb_line_milling(double _x1, double _y1, double _x2, double _y2, double diameter, int debug ){
	Cont_t* cont = NULL;
	int mirror_x = env_i("mirror_x");
	int mirror_y = env_i("mirror_y");

	double x1 = round_to_decimal(_x1, 2) * (mirror_y?-1:1);
	double y1 = round_to_decimal(_y1, 2) * (mirror_x?-1:1);
	double x2 = round_to_decimal(_x2, 2) * (mirror_y?-1:1);
	double y2 = round_to_decimal(_y2, 2) * (mirror_x?-1:1);


	/*
	if( debug ){
		line_milling( x1, y1, x2, y2, round_to_decimal(d, 2), -1, &cont );
		Refholder_t* list = split_all(0);
		walk_around_all_points();
		find_areas( list );
		clean_conts_by_list( &list );
		find_all_conts();
	}
	*/
	double R = diameter/2;
	R = round_to_decimal(R, 2);
	if( R <= 0 ) R = 0.01;
	line_milling( x1, y1, x2, y2, R , -1, &cont );
	Refholder_t* list = split_all(0);
	find_areas( list );
	clean_conts_by_list( &list );
	find_all_conts();
}

/*
void fix_app( Aperture* ap, double d ){
	Context_t* ctx_dst = create_context("dst_macro");
	Context_t* ctx = create_context("tmp_macro");
	Cont_t* cont = create_cont();
	Point_t* prev = NULL;
	Point_t* first = NULL;
	if( ap->points_len >0 ){
		for( int i = 0; i < ap->points_len; i++){
			Point_t* curr = create_p(ap->points[i].x, ap->points[i].y);
			if( !first ) first = curr;
			if( prev ){
				Line_t *l = create_line( prev, curr);
				add_item2cont((Refitem_t*) l, cont );
			}
			prev = curr;
		}
		Line_t *l = create_line( prev, first);
		add_item2cont( (Refitem_t*) l, cont );
		cont_reorder( cont, -1 );
	}
	outline_milling( cont, ctx_dst, d/2 );
	select_context( ctx_dst->name );

		printf("\n");
		walk_around_all_cont("svg", stdout);

	free_context_by_name( ctx_dst->name );
	free_context_by_name( ctx->name );
};
*/

//Cont_t* poligon( double _x, double _y, Aperture* ap, int mirror_x, int mirror_y ){
Cont_t* poligon( double _x, double _y, Aperture* ap, double d, int mirror_x, int mirror_y ){
	Point_t* prev = NULL;
	Point_t* next = NULL;
	Point_t* start = NULL;

	Context_t* ctx_curr = get_context();
	Context_t* ctx_dst = create_context("dst_macro");
	Context_t* ctx = create_context("tmp_macro");

	select_context( ctx->name );

	Cont_t* cont = create_cont();
	for( int i = 0; i < ap->points_len; i++ ){
		if( next ) prev = next;
		double x = round_to_decimal(ap->points[i].x, 2) * (mirror_y?-1:1);
		double y = round_to_decimal(ap->points[i].y, 2) * (mirror_x?-1:1);
		next = create_p( _x + x, _y + y );
		if( !start ) start = next;
		if( prev && next ){
			Line_t* l = create_line( prev, next );
			add_item2cont( (Refitem_t*) l, cont );
		}
	}
	if( next && start ){
		Line_t* l = create_line( next, start );
		add_item2cont( (Refitem_t*) l, cont );
	}
	cont_reorder( cont, -1 );

	outline_milling( cont, ctx_dst, d, 1 );

	select_context( ctx_dst->name );

//printf("---\n");
//walk_around_all_cont("svg", stdout);


	for( int i=0; i < ctx_dst->links.count; i++){
		Refitem_t* item = (Refitem_t*) ctx_dst->links.arr[i];
		if( item->type == OBJ_TYPE_CONTUR ){
			Cont_t* next = (Cont_t*) item;
			copy_ctx2ctx( ctx_dst->name, ctx_curr->name, next );
			break;
		}
	}

    cont = last_cont(ctx_curr);

	free_context_by_name( ctx->name );
	free_context_by_name( ctx_dst->name );
	select_context( ctx_curr->name );

	cont_reorder( cont, -1 );

	return cont;
};


/*
Cont_t* poligon( double _x, double _y, Aperture* ap, double d, int mirror_x, int mirror_y ){
    printf("poligon d=%f\n", d);
	Point_t* prev = NULL;
	Point_t* start = NULL;
	Context_t* ctx_curr = get_context();
	Context_t* ctx_dst = create_context("dst_macro");
	Context_t* ctx = create_context("tmp_macro");
	Cont_t* cont = create_cont();
	if( ap->points_len > 0 ){
		for( int i = 0; i < ap->points_len; i++ ){
			double x = round_to_decimal(ap->points[i].x, 2) * (mirror_y?-1:1);
			double y = round_to_decimal(ap->points[i].y, 2) * (mirror_x?-1:1);
			Point_t* curr = create_p( _x + x, _y + y );
			if( !start ) start = curr;
			if( prev ){
				Line_t* l = create_line( prev, curr );
				add_item2cont( (Refitem_t*) l, cont );
			}
			prev = curr;
		}
		Line_t* l = create_line( prev, start );
		add_item2cont( (Refitem_t*) l, cont );
		cont_reorder( cont, -1 );
	}
	outline_milling( cont, ctx_dst, d, -1 );

	for( int i=0; i < ctx_dst->links.count; i++){
		Refitem_t* item = (Refitem_t*) ctx_dst->links.arr[i];
		if( item->type == OBJ_TYPE_CONTUR ){
			Cont_t* next = (Cont_t*) item;
			copy_ctx2ctx( ctx_dst->name, ctx_curr->name, next );
			break;
		}
	}
	select_context( ctx_dst->name );
	free_context_by_name( ctx->name );
	select_context( ctx_curr->name );
	free_context_by_name( ctx_dst->name );
	return cont;
};

//printf("---\n");
//walk_around_all_cont("svg", stdout);
*/

void grb_macro_touch(double _x1, double _y1, Aperture* ap, int debug){
	int mirror_x = env_i("mirror_x");
	int mirror_y = env_i("mirror_y");
	double x1 = round_to_decimal(_x1, 2) * (mirror_y?-1:1);
	double y1 = round_to_decimal(_y1, 2) * (mirror_x?-1:1);
	double d = env_d("tool_d");
	Cont_t* cont = poligon(x1, y1, ap, d, mirror_x, mirror_y);
	int r = cont_reorder( cont, -1 );
	if( r = -1 ){
		Refholder_t* list = split_all(0);
		find_areas( list );
		clean_conts_by_list( &list );
		find_all_conts();
		if( debug ) exit(0);
	}
}

void grb_ra_line(double _x1, double _y1, double _x2, double _y2, double width, double height, int debug){
	// Svg_env_t* env,
	int mirror_x = env_i("mirror_x");
	int mirror_y = env_i("mirror_y");
	double x1 = round_to_decimal(_x1, 2) * (mirror_y?-1:1);
	double y1 = round_to_decimal(_y1, 2) * (mirror_x?-1:1);
	double x2 = round_to_decimal(_x2, 2) * (mirror_y?-1:1);
	double y2 = round_to_decimal(_y2, 2) * (mirror_x?-1:1);
	Cont_t* cont = ra_line( x1, y1, x2, y2, round_to_decimal(width, 2), round_to_decimal(height, 2), -1 );

	//int debug = 0;
	/*
	Все оттестировано, но наш бронепоезд.... ну вы знаете.
	if( cont && cont->links.arr && (cont->links.count>1) ){
		for(int i = 0; i < cont->links.count; i++){
			Refitem_t* item = cont->links.arr[i];
			if( item->type == OBJ_TYPE_LINE ){
				Line_t* a1 = (Line_t*) item;
				//printf("Nespheratum finding %i...\n", a1->id);
				if( a1->id == 1882 ){
					printf("Nespheratum founded!!! cont %i\n", cont->num+1);
					debug = 1;
					break;
				}
			}
		}
	}
	*/
	int r = cont_reorder( cont, -1 );
	if( r = -1 ){
		Refholder_t* list = split_all(0);
		find_areas( list );
		clean_conts_by_list( &list );
		find_all_conts();
		if( debug ){
			exit(0);
		}
	}
}

// Преобразование строки координат в число с учетом формата
double parse_coordinate( const char* str, int int_digits, int dec_digits ){
    if( str == NULL || *str == '\0' ) return 0.0;
    int is_negative = 0;
    const char* ptr = str;
    if(*ptr == '-'){
        is_negative = 1;
        ptr++;
    }
    int len = strlen(ptr);
    char buffer[256] = {0};
    int buf_pos = 0;
    if(is_negative) buffer[buf_pos++] = '-';
    if(len <= dec_digits) {
        buffer[buf_pos++] = '0';
        buffer[buf_pos++] = '.';
        for(int i = 0; i < (dec_digits - len); i++){
            buffer[buf_pos++] = '0';
        }
        strcpy(buffer + buf_pos, ptr);
        buf_pos += len;
    }else{
        int int_len = len - dec_digits;
        strncpy(buffer + buf_pos, ptr, int_len);
        buf_pos += int_len;
        buffer[buf_pos++] = '.';
        strcpy(buffer + buf_pos, ptr + int_len);
        buf_pos += dec_digits;
    }
    return atof(buffer);
}

// Найти апертуру по номеру
Aperture* find_aperture(GerberState* state, int number){
    for (int i = 0; i < state->aperture_count; i++) {
        if (state->apertures[i].number == number) {
            return &state->apertures[i];
        }
    }
    return NULL;
}

// Парсинг определения макроса
void parse_macros_definition( const char* line, GerberState* state ){
    //printf("parse_macros_definition\n");
	char macroName[32];
	int primCode, exposure, count, offset = 0;
	// 1. Парсим имя до '*' и три числа после
	// %[^*] — читать всё, пока не встретится '*'
	if( sscanf(line, "%%AM%[^*]*%d,%n", macroName, &primCode, &offset) < 2 ){
		printf("Ошибка структуры строки: %s\n", line);
		exit(1);
	}
	const char* ptr = line + offset;
	offset = 0;
	char* star = strchr(ptr, '*');
	if( star ) *star = '\0';
	Macro* mc = &state->macros[ state->macros_count ];
	state->macros_count++;
	mc->params[0].index = 0;
	mc->params[0].value = primCode;
	mc->params_len = 1;
	memcpy(&mc->macro, &macroName, 10);
	char par[32];
	while (sscanf(ptr, "%31[^,*]%n", par, &offset) == 1) {
		char *endptr;
		double val = strtod(par, &endptr);
		if(par == endptr) {
			// В строке вообще не было числа (например, там "$1")
			if (par[0] == '$') {
				int var_index = atoi(par + 1); // Обрабатываем как переменную
				mc->params[mc->params_len].index = var_index;
				mc->params[mc->params_len].value = 0;
				//printf("int=%i\n", var_index );
				mc->params_len++;
			}
		} else {
			// Число успешно прочитано
			mc->params[mc->params_len].index = 0;
			mc->params[mc->params_len].value = val;
			//printf(" %i --- float=%f\n", mc->params_len, val );
			mc->params_len++;
		}
		ptr += offset; // Сдвигаем указатель на длину прочитанного текста
		if (*ptr == ',') {
			ptr++; // Пропускаем запятую и идем на следующий круг
		} else if (*ptr == '*') {
			break; // Дошли до финальной звезды — выход
		} else {
			break; // Что-то пошло не так
		}
	}
/*
	for( int i = 0; i < mc->params_len; i++ ){
		printf("%i  =  ", i);
		if( mc->params[i].index == 0 ){       //абсолютное значение
			printf( " %f ", mc->params[i].value );
		}else{                                // параметрическое значение
			printf( " S%i ", mc->params[i].index );
		}
		printf("\n");
	}
	exit(1);
*/
}

Macro* macro_by_name(GerberState* state, char* macro){
    for( int i=0; i<state->macros_count; i++ ){
        if( strcmp(state->macros[i].macro, macro)==0 ){
            return &state->macros[i];
        };
    }
    return NULL;
}

double get_par( Macro* mc, double arg[], int i){
	double v = 0;
	if( mc->params[i].index ){
		v = arg[mc->params[i].index];
	}else{
		v = mc->params[i].value;
	};
	return v;
}



// Парсинг определения апертуры
void parse_aperture_definition( const char* line, GerberState* state ){
	//printf("parse_aperture_definition\n");
	char type;
	int num;
	char params[100];
	char macroName[32];
	int bytes = 0;
	if( sscanf(line, "%%ADD%d%c,%s", &num, &type, params) >= 3 ){
		// Убираем * в конце если есть
		char* star = strchr(params, '*');
		if( star ) *star = '\0';
		if( state->aperture_count >= MAX_APERTURES ){
			printf("Too many apertures!");
			return;
		}
		Aperture* ap = &state->apertures[state->aperture_count];
		ap->number = num;
		ap->type = type;
		if (type == 'C') {
			ap->param1 = atof(params);
			message(0, "Defined circular aperture D%d: dia=%.4f", num, ap->param1);
		}else if (type == 'R') {
			char* x = strchr(params, 'X');
			if(x){
				*x = '\0';
				ap->param1 = atof(params);
				ap->param2 = atof(x + 1);
				message(0, "Defined rectangular aperture D%d: %.4fx%.4f", num, ap->param1, ap->param2);
			}
		}
		state->aperture_count++;
	}else if( sscanf(line, "%%ADD%dM%[^*,]%n", &num, &macroName, &bytes) >= 2){
		Aperture* ap = &state->apertures[state->aperture_count];
		ap->number = num;
		ap->type = 'M';
		*ap->macro = 'M';
		strcpy( ap->macro+1, macroName );
		state->aperture_count++;
		//printf("parse_aperture_definition with macro %s\n", ap->macro);
		Macro* mc = macro_by_name(state, ap->macro );
		if( mc ){
			double d = env_d("tool_d");
			int code = (int) mc->params[0].value;
			if( code == 4 ){ //просто полигон по точкам
				int expl = (int) mc->params[1].value;
				int count = (int) mc->params[2].value;
				for( int i = 0; i < count; i++ ){
					ap->points[i].x = mc->params[3+i*2].value;
					ap->points[i].y = mc->params[3+i*2+1].value;
				}
				ap->points_len = count;
				//fix_app( ap, d );
			}else if(code==21){
				ap->points_len = 4;
				char* ptr = (char*) line + bytes;
				double arg[30];
				int n = sscanf(ptr, ",%lfX%lfX%lf*", &arg[1], &arg[2], &arg[3]);
				if( n >= 3 ){
					double exp = get_par( mc, arg, 1 );
					double w = get_par( mc, arg, 2 );
					double h = get_par( mc, arg, 3 );
					double cx = get_par( mc, arg, 4 );
					double cy = get_par( mc, arg, 5 );
					double ang = get_par( mc, arg, 6 );
					rotate_point(cx + (w+d)/2, cy + (h+d)/2, ang, &ap->points[0].x, &ap->points[0].y);
					rotate_point(cx + (w+d)/2, cy - (h+d)/2, ang, &ap->points[1].x, &ap->points[1].y);
					rotate_point(cx - (w+d)/2, cy - (h+d)/2, ang, &ap->points[2].x, &ap->points[2].y);
					rotate_point(cx - (w+d)/2, cy + (h+d)/2, ang, &ap->points[3].x, &ap->points[3].y);
				}
			}
		}else{
			printf("Macro %s not found\n", macroName);
		}
	}
	//printf("end of parse_aperture_definition\n");
}


// Выбор апертуры
void select_aperture(GerberState* state, int number){
    state->current_aperture = number;
    Aperture* ap = find_aperture(state, number);
    if(ap){
        if(ap->type == 'C'){
            message(0, "Selected circular aperture D%d (dia=%.4f)", number, ap->param1);
        }else if(ap->type == 'R') {
            message(0, "Selected rectangular aperture D%d (%.4fx%.4f)", number, ap->param1, ap->param2);
        }else if(ap->type == 'M') {
            message(0, "Selected poligon aperture D%d", number);
        }
    }else{
        message(0, "Selected unknown aperture D%d", number);
    }
}

// Извлечение числа из строки после префикса
int extract_number(const char* line, const char prefix, double* value, GerberState* state){
    char* ptr = strchr(line, prefix);
    if (!ptr) return 0;
    ptr++;
    int is_negative = 0;
    if(*ptr == '-'){
        is_negative = 1;
        ptr++;
    }
    char num_str[256];
    int i = 0;
    while(*ptr && isdigit(*ptr)){
        num_str[i++] = *ptr++;
    }
    num_str[i] = '\0';
    if(i == 0) return 0;
    if(is_negative){
        char temp[256];
        temp[0] = '-';
        strcpy( temp + 1, num_str );
        *value = parse_coordinate( temp, state->format_int, state->format_dec );
    }else{
        *value = parse_coordinate( num_str, state->format_int, state->format_dec );
    }
    return 1;
}

static int grbr_lines_count = 0;
// Парсинг команды с координатами
void parse_coordinate_command(const char* line, GerberState* state) {
// Svg_env_t* env, 
//	grbr_lines_count++;
	int debug = 0;
//	message(1, "%s", line);
	if( line && strlen(line) > 0 ){
		message(1, "\r%i lines processed...", grbr_lines_count++ );
	}

//	printf("grbr_lines_count   =   %i\n", grbr_lines_count);
	//if( grbr_lines_count == 2 ) debug = 1;

    double new_x = state->x;
    double new_y = state->y;
    int d_code = -1;
    extract_number(line, 'X', &new_x, state);
    extract_number(line, 'Y', &new_y, state);
    char* d_ptr = strstr(line, "D0");
    if (d_ptr && strlen(d_ptr) >= 3) {
        d_code = atoi(d_ptr + 1);
    }
    // Получаем текущую апертуру
    Aperture* ap = find_aperture(state, state->current_aperture);
    if (!ap) {
        printf("Error: No aperture selected!\n");
        return;
    }
    // Выполняем команду
    if (d_code == 1) {  // D01 - фрезеровать по линии
        double tool_d = env_d("tool_d");
        int tinside  = env_i("tool_inside");
        double offset = ( tinside?(-tool_d):(tool_d) );
        //printf("OFFSET: %f\n", offset);
        if (ap->type == 'C') {
            //printf("ap->param1: %f; SIZE: %f\n",ap->param1,  ap->param1 + offset);
            printf(" grb_line_milling start with R = %f \n", ap->param1  );
            grb_line_milling( state->x, state->y, new_x, new_y, ap->param1 + offset, debug );
            printf(" grb_line_milling ok \n");
        }else if (ap->type == 'R') {
            grb_ra_line( state->x, state->y, new_x, new_y, ap->param1 + offset, ap->param2 + offset, debug );
        }else if (ap->type == 'M') {
            fprintf(stderr, "Error: Function not implemented: move polygon\n");
        }
        state->x = new_x;
        state->y = new_y;
    }else if (d_code == 2) {  // D02 - перемещение
        state->x = new_x;
        state->y = new_y;
        //printf("MOVE to: (%.4f,%.4f)\n", new_x, new_y);
    }else if (d_code == 3) {  // D03 - коснуться инструментом в одной точке
        double tool_d = env_d("tool_d");
        int tinside  = env_i("tool_inside");
        double offset = ( tinside?(-tool_d):(tool_d) );
        if (ap->type == 'C') {
            grb_line_milling( new_x, new_y, new_x, new_y, ap->param1+offset, debug );
        }else if (ap->type == 'R') {
            grb_ra_line( new_x, new_y, new_x, new_y, ap->param1+offset, ap->param2+offset, debug );
        }else if (ap->type == 'M') {
            Macro* m = macro_by_name(state, ap->macro);
            //printf("M: %s %i\n\n", m->macro, m->len);
            grb_macro_touch( new_x, new_y, ap, debug );
            //exit(1);
        }
        state->x = new_x;
        state->y = new_y;
    }
}

// Основная функция парсинга
void parse_gerber_file( const char* filename, Context_t* ctx ){
	setlocale(LC_NUMERIC, "C");
	if( !ctx ){
		fprintf(stderr, "Empty context. Skip input file %s\n", filename);
		return;
	}
	select_context(ctx->name);
	FILE* file = fopen(filename, "r");
	if (!file) {
		fprintf(stderr, "Error opening file %s\n", filename);
		return;
	}
	printf("\n\n");
	GerberState state = {0};
	state.format_int = 4;
	state.format_dec = 5;
	state.is_metric = 1;
	state.current_aperture = -1;
	state.aperture_count = 0;
	char line[1024];
	while( fgets(line, sizeof(line), file) ){
		line[strcspn(line, "\n\r")] = '\0';
		if( strlen(line) == 0 ) continue;
		// Комментарии
		if( strncmp(line, "G04", 3) == 0 ) continue;
		// Формат
		if( strstr(line, "%FSLAX") ){
			char format[20];
			if (sscanf(line, "%%FSLAX%s", format) == 1) {
				char* y_part = strchr(format, 'Y');
				if (y_part) *y_part = '\0';
				char* star = strchr(format, '*');
				if (star) *star = '\0';
				if (strlen(format) >= 2) {
					state.format_int = format[0] - '0';
					state.format_dec = format[1] - '0';
					message(0, "Format: %d integer, %d decimal digits", state.format_int, state.format_dec);
				}
			}
		}
		// Единицы измерения
		if( strstr(line, "%MO") ){
			if(strstr(line, "MM")) {
				state.is_metric = 1;
				message(0, "Units: millimeters");
			}else if( strstr(line, "IN") ){
				state.is_metric = 0;
				message(0, "Units: inches");
			}
		}
		// Определения апертур
		if( strstr(line, "%ADD") ) parse_aperture_definition(line, &state);
		if( strstr(line, "%AM")  ) parse_macros_definition(line, &state);
		// Выбор апертуры
		if( line[0] == 'D' && isdigit(line[1]) && strchr(line, '*') && !strstr(line, "%ADD") && !strstr(line, "D0") ){
			char num_str[10];
			strncpy(num_str, line + 1, sizeof(num_str) - 1);
			char* star = strchr(num_str, '*');
			if (star) *star = '\0';
			int d_num = atoi(num_str);
			select_aperture(&state, d_num);
		}
		// Координатные команды
		if( (strchr(line, 'X') || strchr(line, 'Y')) && (strstr(line, "D01") || strstr(line, "D02") || strstr(line, "D03")) ){
			parse_coordinate_command( line, &state);
		}
		// Начало рисования
		if( strstr(line, "%LPD") ){
			message(0, "Start drawing (dark polarity)");
		}
		// Линейная интерполяция
		if( strstr(line, "G01") ){
			//printf("Linear interpolation mode\n");
		}
		// Конец файла
		if( strstr(line, "M02") || strstr(line, "M00") ){
			message(0, "Milling processing complete");
			break;
		}
	}
	fclose(file);
}

