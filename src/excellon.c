#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#include <ctype.h>
#include <string.h>
#include <math.h>

#include "../include/top.h"
#include "../include/define.h"
#include "../include/conf.h"
#include "../include/reflist.h"
#include "../include/context.h"
#include "../include/gerber.h"
#include "../include/main.h"

#include "../include/apply_config.h"

#include "../include/excellon.h"

#define BATCHSIZE 100

typedef struct {
	double scale;
	int leading_zeros;
	int current_tool_num;
	Tool_t current_tool;
	double last_x; // Сохраняем последнее положение
	double last_y;
	int format_dec; // Количество знаков после запятой (обычно 3 для мм, 4 для дюймов)
} DrillState;

void add_hole( Excellon_t* exc, double x, double y, double d ){
	int mirror_y = env_i("mirror_y");
	int mirror_x = env_i("mirror_x");
	Hole_t hole = {
		.x=round_to_decimal(x, 2) * (mirror_y?-1:1),
		.y=round_to_decimal(y, 2) * (mirror_x?-1:1),
		.d=round_to_decimal(d, 2)
	};
	if( exc->holes_count >= exc->holes_size ){
		exc->holes_size = exc->holes_size + BATCHSIZE;
		Hole_t* newholes = malloc( exc->holes_size * sizeof(Hole_t) );
		if( newholes ){
			if( exc->holes ){
				memcpy( newholes, exc->holes, exc->holes_count * sizeof(Hole_t) );
				free( exc->holes );
			}
			exc->holes = newholes;
			exc->holes[exc->holes_count++] = hole;
			//printf("add hole %f %f\n", hole.x, hole.y);
		}
	}else{
		exc->holes[exc->holes_count++] = hole;
		//printf("add hole %f %f\n", hole.x, hole.y);
	}
}

int cmp_holes(Hole_t* h1, Hole_t* h2){
	if( h1->d > h2->d ) return 1;
	if( h1->d == h2->d ){
		if( h1->x > h2->x ) return 1;
		if( h1->x == h2->x ){
			if( h1->y > h2->y ) return 1;
		}
	}
	return 0;
}

void sort_holes( Excellon_t* exc ){
	int newholes_count = 0;
	//exc->holes_count = 20; //// !!!!! Толькао в целях тестировани! Не забыть убрать!!!
	Hole_t* newholes = malloc( exc->holes_count * sizeof(Hole_t) );
	for( int next = 0; next < exc->holes_count; next++ ){
		Hole_t h = exc->holes[ next ];
		int i = 0;
		for( i = 0; i < newholes_count; i++){
			if( cmp_holes( &newholes[i], &h ) ){
				for( int j = newholes_count; j>i; j--){
					newholes[j] = newholes[j-1];
				}
				break;
			}
		}
		//printf("insert hole %f to position i=%i\n", h.d, i);
		newholes[i] = h;
		newholes_count++;
	}
	free( exc->holes );
	exc->holes = newholes;
}

void replace_drills( Excellon_t* exc ){
	for( int next = 0; next < exc->holes_count; next++ ){
		exc->holes[ next ].d = norm_dia(exc->holes[ next ].d);
	}
}

void add_drill_tool(Excellon_t* exc, int tool_num, double size_mm){
	Tool_t tool = { .tool_num = tool_num, .size_mm = size_mm };
	if( exc->tools_count >= exc->tools_size ){
		exc->tools_size = exc->tools_size + BATCHSIZE;
		Tool_t* newtools = malloc( exc->tools_size * sizeof(Tool_t));
		if( newtools ){
			if( exc->tools ){
				memcpy( newtools, exc->tools, exc->tools_count * sizeof(Tool_t) );
				free( exc->tools );
			}
			exc->tools = newtools;
			exc->tools[exc->tools_count++] = tool;
		}
	}else{
		exc->tools[exc->tools_count++] = tool;
	}
}

void select_drill_tool( DrillState* state, Excellon_t* exc, int tool_num ){
	for( int i = 0; i < exc->tools_count; i++ ){
		if( exc->tools[i].tool_num == tool_num ){
			state->current_tool = exc->tools[i];
			state->current_tool_num = tool_num;
			return;
		}
	}
	state->current_tool_num = -1;
}

void flush_tools_num( Excellon_t* exc ){ // сбрасываем номера инструментов, готовимся к приёму следующего drill-файла
	for( int i=0; i<exc->tools_count; i++ ){
		exc->tools[i].tool_num=-1;
	}
}

void print_holes( Excellon_t* exc ){
	printf("==================== [ holes_size:%i, holes_count:%i ] == [ tools_size:%i, tools_count:%i ] ===\n", exc->holes_size, exc->holes_count, exc->tools_size, exc->tools_count);
	for( int i=0; i<exc->holes_count; i++ ){
		Hole_t h = exc->holes[i];
		printf(" HOLE  d%f (%f, %f)\n", h.d, h.x, h.y);
	}
}


int parse_axis(DrillState* state, const char* line, char axis_char, double* target){
	char* p;
	p = strchr(line, axis_char);
	if(p){
		char val_str[32];
		int i = 0;
		p++; // пропускаем 'X' или 'Y'
		while( isdigit(*p) || *p == '-' || *p == '.' ){
			val_str[i++] = *p++;
		}
		val_str[i] = '\0';
		if (strchr(val_str, '.')) {
			// Вариант 1: Координата уже с точкой (X12.345)
			*target = atof(val_str);
		} else {
			// Вариант 2: Целое число (X00123), требующее масштабирования
			long raw_val = atol(val_str);
			double divisor = (state->scale > 1.0) ? 10000.0 : 1000.0; // 2.4 для дюймов, 3.3 для мм
			*target = (double)raw_val / divisor;
			/*
			if (state->leading_zeros) {
				// Leading: "1" при 3 знаках после запятой -> 0.001
				*target = (double)raw_val / divisor;
			} else {
				// Trailing: "1" при 3 знаках -> 1.000 (самый частый вариант)
				// Для простоты считаем, что число уже в нужной доле,
				// так как в TZ формате число всегда дополняется до полной разрядности.
				*target = (double)raw_val / divisor;
			}
			*/
		}
		return 1;
	}
	return 0;
};

void parse_drill_coordinate( Excellon_t* exc, const char* line, DrillState* state ) {
	int updated = 0;
	// Вспомогательная функция для парсинга отдельной оси
	int has_x = parse_axis(state, line, 'X', &state->last_x);
	int has_y = parse_axis(state, line, 'Y', &state->last_y);
	if (has_x || has_y) {
		// Конвертируем в мм только для внутреннего контекста, если файл в дюймах
		// ВАЖНО: scale применяется к финальному значению, а не к сырым данным
		double final_x = state->last_x * state->scale;
		double final_y = state->last_y * state->scale;
		if (state->current_tool_num != -1) {
			message(0, "Drill Hole: X%.3f Y%.3f (T%02d)", final_x, final_y, state->current_tool_num);
			grb_line_milling(final_x, final_y, final_x, final_y, env_d("tool_d"), 0); // Рисуем кернинг
			Hole_t h = { .x=final_x, .y=final_y, .d=state->current_tool.size_mm };
			add_hole(exc, final_x, final_y, state->current_tool.size_mm);             // Добавляем отверстие на карту сверловки
		}
	}
}


void parse_drill_file( Excellon_t* exc, const char* filename ){
	FILE* file = fopen(filename, "r");
	if (!file) {
		fprintf(stderr, "Error opening file %s\n", filename);
		return;
	}
	DrillState state = {0};
	state.scale = 1.0;           // По умолчанию мм
	state.leading_zeros = 0;     // По умолчанию Trailing (обычно для Excellon)
	state.current_tool_num = -1;
	char line[1024];
	while (fgets(line, sizeof(line), file)) {
		// Очистка строки
		line[strcspn(line, "\n\r")] = '\0';
		if (strlen(line) == 0 || line[0] == ';') continue; // Пропуск пустых и комментариев
		// --- Заголовок и Единицы измерения ---
		if( strstr(line, "METRIC") ){
			state.scale = 1.0;
			if (strstr(line, "LZ") ) state.leading_zeros = 1;
			message(0, "Drill Units: Metric (mm)");
		}else if( strstr(line, "INCH")){
			state.scale = 25.4;
			if( strstr(line, "LZ") ) state.leading_zeros = 1;
			message(0, "Drill Units: Imperial (inch)");
		}
		// --- Определения и выбор инструментов (T-коды) ---
		// Формат: T01C0.500 или просто T01 (выбор)
		if( line[0] == 'T' && isdigit(line[1]) ){
			char* c_ptr = strchr(line, 'C');
			if (c_ptr) {
				// Это определение инструмента (Tool Definition)
				int tool_num;
				double diameter;
				if (sscanf(line, "T%dC%lf", &tool_num, &diameter) == 2) {
					double size_mm = diameter * state.scale;
					// Здесь вызываем вашу функцию сохранения апертуры/инструмента
					add_drill_tool( exc, tool_num, size_mm );
					message(0, "Tool T%02d definition: diam %.3f mm", tool_num, size_mm);
				}
			} else {
				// Это выбор инструмента (Tool Select)
				int tool_num = atoi(line + 1);
				//state.current_tool_num = tool_num;
				select_drill_tool(&state, exc, tool_num);
			}
		}
		// --- Координаты сверления ---
		// Формат: X12345Y67890 (без D-кода в конце, в отличие от Gerber)
		if( strchr(line, 'X') || strchr(line, 'Y') ){
			parse_drill_coordinate( exc, line, &state );
		}
		// --- Конец данных ---
		if( strstr(line, "M30") || strstr(line, "M00") ){
			message(0, "Drill processing complete");
			break;
		}
	}
	flush_tools_num(exc);
	fclose(file);
}
