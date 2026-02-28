#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#include <ctype.h>
#include <string.h>
#include <math.h>

#include "../include/define.h"
#include "../include/top.h"
#include "../include/conf.h"
#include "../include/reflist.h"
#include "../include/context.h"
#include "../include/cont.h"
#include "../include/milling.h"
#include "../include/mergcont.h"
#include "../include/excellon.h"
#include "../include/apply_config.h"

#include "../include/export.h"

#include "../include/main.h"

#define MAX_APERTURES 100

// Структура для хранения апертуры
typedef struct {
    int number;
    char type;  // 'C' - круг, 'R' - прямоугольник
    double param1; // диаметр или ширина
    double param2; // высота (для прямоугольника)
} Aperture;

// Структура для хранения состояния парсера
typedef struct {
    double x, y;           // Текущая позиция
    int current_aperture;  // Текущая апертура
    int format_int;        // Цифр целой части
    int format_dec;        // Цифр дробной части
    int is_metric;         // 1 = мм, 0 = дюймы
    Aperture apertures[MAX_APERTURES];
    int aperture_count;
} GerberState;

// Функции-заглушки для рисования
static int count = 0;

void grb_line_milling(double _x1, double _y1, double _x2, double _y2, double diameter, int debug ){
	// Svg_env_t* env, 
	//select_context("main");
	//int debug = 0;
	double d = env_d("tool_d");
	d = diameter/2 + d/2;
	Cont_t* cont = NULL;
	int mirror_x = env_i("mirror_x");
	int mirror_y = env_i("mirror_y");

	/*
	if( cont && cont->links.arr && (cont->links.count>1) ){
		count = count + 1;
		//if( cont->links.count > 0 ){
			for(int i = 0; i < cont->links.count; i++){
				Refitem_t* item = cont->links.arr[i];
				if( item->type == OBJ_TYPE_LINE ){
					Line_t* a1 = (Line_t*) item;
					//printf("Nespheratum finding %i...\n", a1->id);
					if( a1->id == 1886 ){
						printf("Nespheratum founded!!!\n");
						debug = 1;
					}
				}
			}
		//}
	}
	*/

	double x1 = round_to_decimal(_x1, 2) * (mirror_y?-1:1);
	double y1 = round_to_decimal(_y1, 2) * (mirror_x?-1:1);
	double x2 = round_to_decimal(_x2, 2) * (mirror_y?-1:1);
	double y2 = round_to_decimal(_y2, 2) * (mirror_x?-1:1);

	if( count == 2 ) debug = 1;

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

	line_milling( x1, y1, x2, y2, round_to_decimal(d, 2), -1, &cont );
	Refholder_t* list = split_all(0);
	find_areas( list );
	clean_conts_by_list( &list );
	find_all_conts();
}

void grb_ra_line(double _x1, double _y1, double _x2, double _y2, double width, double height, int debug){
	// Svg_env_t* env, 
	double d = env_d("tool_d");
	d = d/2;
	int mirror_x = env_i("mirror_x");
	int mirror_y = env_i("mirror_y");

	double x1 = round_to_decimal(_x1, 2) * (mirror_y?-1:1);
	double y1 = round_to_decimal(_y1, 2) * (mirror_x?-1:1);
	double x2 = round_to_decimal(_x2, 2) * (mirror_y?-1:1);
	double y2 = round_to_decimal(_y2, 2) * (mirror_x?-1:1);
	Cont_t* cont = ra_line( x1, y1, x2, y2, round_to_decimal(width + d, 2), round_to_decimal(height + d, 2), -1 );
	
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

//		split_all(0);
//		calc_contcount4all( 0, debug );
//		marking_of_imposed(); // Маркируем совпадающие грани контуров
//		clean_all_cont();
//		find_all_conts();

		if( debug ){
			//walk_around_all_cont("svg", stdout);
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

// Парсинг определения апертуры
void parse_aperture_definition( const char* line, GerberState* state ){
    char type;
    int num;
    char params[100];
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
    }
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

//	if( strcmp("X-1016000Y-4686300D01*", line) == 0){
//		printf("DEBUG MODE ENABLED!!!\n");
//		debug = 1;
//	}

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
        if (ap->type == 'C') {
            grb_line_milling( state->x, state->y, new_x, new_y, ap->param1, debug );
        }else if (ap->type == 'R') {
            grb_ra_line( state->x, state->y, new_x, new_y, ap->param1, ap->param2, debug );
        }
        state->x = new_x;
        state->y = new_y;
    }else if (d_code == 2) {  // D02 - перемещение
        state->x = new_x;
        state->y = new_y;
        //printf("MOVE to: (%.4f,%.4f)\n", new_x, new_y);
    }else if (d_code == 3) {  // D03 - коснуться инструментом в одной точке
        if (ap->type == 'C') {
            grb_line_milling( new_x, new_y, new_x, new_y, ap->param1, debug );
        }else if (ap->type == 'R') {
            grb_ra_line( new_x, new_y, new_x, new_y, ap->param1, ap->param2, debug );
        }
        state->x = new_x;
        state->y = new_y;
    }
}

// Основная функция парсинга
void parse_gerber_file( const char* filename, Context_t* ctx ){
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

