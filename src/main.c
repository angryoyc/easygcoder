#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>
#include <math.h>
#include <ctype.h>
#include <stdarg.h>
#include <sys/stat.h>


#include "../include/define.h"
#include "../include/conf.h"
#include "../include/top.h"
#include "../include/reflist.h"
#include "../include/point.h"
#include "../include/cont.h"
#include "../include/geom.h"
#include "../include/context.h"
#include "../include/line.h"
#include "../include/arc.h"
#include "../include/gerber.h"
#include "../include/excellon.h"
#include "../include/gcode.h"
#include "../include/export.h"
#include "../include/apply_config.h"

#include "./copy_file.c"
#include "../include/main.h"

//char* end;
//struct pair ** config;             // Ссылка на загруженные данные /etc/gb2gc.conf


uint8_t isfile( char* path ){
	return ( access( path, F_OK ) == 0 )?1:0;
}

void message( int offset, char * format, ... ) {
	char str[500];
	memset(str, '\0', 500);
	va_list argv;
	va_start(argv, format);
	vsnprintf(str, 500, format, argv);
	va_end(argv);
	if( 1==1 ){ // варианты вывода - стандартный и отладочный
		for( int i = 0; i<(2-offset); i++) printf("\033[1A");
		printf("\r\033[K%s", str);
		//printf("                                                                                           \r");
		for( int i = 0; i<(2-offset); i++) printf("\n");
	}else{
		printf("%s\n", str);
	}
}

void remove_negative_cont(Context_t* ctx){
	if( ctx ){
		select_context( ctx->name );
		if( ctx->links.arr ){
			for( int i = ctx->links.count-1; i >= 0; i-- ){
				Refitem_t* item = ctx->links.arr[i];
				if( item && (item->type == OBJ_TYPE_CONTUR) ){
					Cont_t* cont =  (Cont_t*) item;
					if( cont->dir == 1 ) purge_cont(&cont);
				}
			}
		}
	}
}

void print_logo(char* binname){
	printf("EasyGcoder v%s (c) 2026 (EasyEDA gerber files to gcode simple decoder)\n", VERSION);
}

void print_help(char* binname){
	print_logo(binname);
	printf("Usage: \n");
	printf("%s [options] ", binname);
	printf("options (input data):\n");
	printf("  -c <filename>  Configuration file (optional)\n");
	printf("  -i <filename>  Input Gerber file (optional)\n");
	printf("  -d <filename>  Drill file1 (optional)\n");
	printf("  -d <filename>  Drill fileN (optional, up to 5)\n");
	printf("  -f <filename>  Board outline file (optional)\n");
	printf("  -m <axis>      Mirror along the axis x|y\n");
	printf("options (output data):\n");
	printf("  -s <filename>  Output file for SVG (optional)\n");
	printf("  -o <filename>  Output file for milling g-code (optional)\n");
	printf("  -b <filename>  Output file for boring g-code (optional)\n");
	printf("other options:\n");
	printf("  -p  Print parameters\n");
	printf("  -h, --help     Show this help message\n");
	printf("\nExample: %s -i ./Gerber_TopLayer.svg -d ./drill_PTH_Through.DRL -s ./output.svg -o ./output.gcode\n", binname);
}

int main(int argc, char* argv[]) {
	char* input_file = NULL;

	char* drill_file[5] = { NULL, NULL, NULL, NULL, NULL };
	int drill_file_count = 0;

	char* config_file[5] = { CONFIGPATH, NULL, NULL, NULL, NULL };
	int config_file_count = 1;

	char* outline_file = NULL;
	char* output_gcode = NULL;
	char* output_svg = NULL;
	char* output_boring = NULL;
	int print_params = 0;


	Excellon_t exc = {
		.tools = NULL,
		.holes = NULL,
		.tools_count = 0,
		.tools_size = 0,
		.holes_count = 0,
		.holes_size = 0
	};

	// Парсинг аргументов командной строки
	if( argc > 1 ){
		for (int i = 1; i < argc; i++) {
			if( strcmp(argv[i], "init") == 0 || strcmp(argv[i], "ini") ==0 ) {
				if (mkdir("./gerber", 0755) == 0){
					printf("create folder: ./gerber\n");
				}
				if (mkdir("./gerber/1", 0755) == 0){
					printf("create folder: ./gerber/1\n");
				}
				if (mkdir("./gcode", 0755) == 0){
					printf("create folder: ./gcode\n");
				}
				if (mkdir("./gcode/1", 0755) == 0){
					printf("create folder: ./gcode/1\n");
				}
				char template_name[100];
				sprintf(template_name, "%s/convert.template", WORKDIR);
				printf("%s template_name=%s\n", WORKDIR, template_name );
				copy_file(template_name, "./convert.sh");
				change_file_permissions("./convert.sh", 0755);
				exit(0);
			}else if(strcmp(argv[i], "-i") == 0 && i + 1 < argc) {
				input_file = argv[++i];
			}else if (strcmp(argv[i], "-d") == 0 && i + 1 < argc) {
				if( drill_file_count < 5 ){
					drill_file[drill_file_count++] = argv[++i];
				}
			}else if (strcmp(argv[i], "-f") == 0 && i + 1 < argc) {
				outline_file = argv[++i];
			}else if (strcmp(argv[i], "-c") == 0 && i + 1 < argc) {
				if( config_file_count < 5 ){
					config_file[config_file_count++] = argv[++i];
				}
			}else if (strcmp(argv[i], "-m") == 0 && i + 1 < argc) {
				char* m = argv[++i];
				if( strcmp(m, "x") == 0 ) set_int_param(NULL, "mirror_x", 1);
				if( strcmp(m, "y") == 0 ) set_int_param(NULL, "mirror_y", 1);
			}else if (strcmp(argv[i], "-o") == 0 && i + 1 < argc) {
				output_gcode = argv[++i];
			}else if (strcmp(argv[i], "-s") == 0 && i + 1 < argc) {
				output_svg = argv[++i];
			}else if (strcmp(argv[i], "-b") == 0 && i + 1 < argc) {
				output_boring = argv[++i];
			}else if( strcmp(argv[i], "-p") == 0 ) {
				print_params = 1;
			}else if ( strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
				print_help(argv[0]);
			}else {
				// Если параметр не распознан, показываем ошибку
				if (argv[i][0] == '-') {
					fprintf(stderr, "Error: Unknown option '%s'\n", argv[i]);
					fprintf(stderr, "Use '%s --help' for usage information\n", argv[0]);
					return 1;
				} else {
					fprintf(stderr, "Error: Unexpected argument '%s'\n", argv[i]);
					fprintf(stderr, "Use '%s --help' for usage information\n", argv[0]);
					return 1;
				}
			}
		}
	}else{
		print_help(argv[0]);
		return 0;
	}

	//printf("\nInitiation of new processing:\n");
	print_logo(argv[0]);
	//


	// Если указаны файлы параметров, загружаем его
	apply_config(NULL);
	for( int i=0; i < config_file_count; i++ ){
		if( config_file[i] && isfile(config_file[i]) ){
			printf( "Loading config: %s\n", config_file[i] );
			apply_config(config_file[i]);
			//config = conf_load( config_file[i] );  // Открываем конфиг файл
			//if( config != NULL ){
			//	apply_config(config); // Анализируем конфиг и применяем его параметры
			//}
		}
	}

	// Парсинг основного файла Gerber
	Context_t* ctx_main;
	Context_t* ctx_outline;
	Context_t* ctx_drill;

	if(print_params){
		print_env();
		if( !test_env("all") ) exit(1);
	}

	if (output_gcode != NULL) if( !test_env("milling") ) exit(1); // для вывода указан gcode для фрезеровки, поэтому сразу проверяем параметры фрезеровки (начало, глубина, шаг и его знак)
	if (output_boring != NULL) if( !test_env("drill") ) exit(1);  // для вывода указан gcode для сверловки, поэтому сразу проверяем параметры сверловки (начало, глубина, шаг и его знак)

//	//-----< Gerber Layer Data
	ctx_main = create_context("main");

	// Проверка обязательного параметра
	if( (input_file != NULL) && isfile(input_file) ){
		printf("Using input file: %s\n", input_file);
		parse_gerber_file( input_file, ctx_main );
	}

	//-----< Gerber Board Outline
	if( (outline_file != NULL) && isfile(outline_file) ){ // Если указан файл обрезки платы, парсим его // && isfile(outline_file)
		printf( "Using board outline file: %s\n", outline_file );
		ctx_outline = create_context("outline");
		parse_gerber_file( outline_file, ctx_outline );
		remove_negative_cont( ctx_outline );
		copy_ctx2ctx( ctx_outline->name, ctx_main->name, NULL );
		//printf("Drill file processing would go here: %s\n", drill_file );
	}

	//-----< Gerber Drill
	if( drill_file_count > 0 ){ // Если указан файл сверловки, обрабатываем его
		for( int i=0; i < drill_file_count; i++ ){
			if( drill_file[i] && isfile(drill_file[i]) ){
				printf( "Using board drill file: %s\n", drill_file[i] );
				ctx_drill = create_context("drill");
				parse_drill_file( &exc, drill_file[i] );
				copy_ctx2ctx( ctx_drill->name, ctx_main->name, NULL );
				free_context_by_name( ctx_drill->name );
			}
		}
		//print_holes(&exc);
		replace_drills(&exc);
		sort_holes(&exc);
		//print_holes(&exc);
	}

	// ================= OUTPUT PHASE ===========
	select_context("main");
	FILE* output_stream;

	//-----> GCODE
	output_stream = stdout;
	if (output_gcode != NULL) {
		gcode_ini();
		if( strcmp(output_gcode, "-") != 0 ){
			output_stream = fopen(output_gcode, "w");
			if (output_stream == NULL) {
				fprintf(stderr, "Error: Cannot open output file '%s'\n", output_gcode );
				if(ctx_main) free_context_by_name(ctx_main->name);
				if(ctx_outline) free_context_by_name(ctx_outline->name);
				if(ctx_drill) free_context_by_name(ctx_drill->name);
				return 1;
			}
			printf("Writing output to file: %s\n", output_gcode );
		}
		walk_around_all_cont("gcode", output_stream );
		if (output_stream != stdout) fclose(output_stream);
	}

	//-----> SVG
	output_stream = stdout;
	if (output_svg != NULL) {
		if( strcmp(output_svg, "-") != 0 ){
			output_stream = fopen(output_svg, "w");
			if (output_stream == NULL) {
				fprintf(stderr, "Error: Cannot open output file '%s'\n", output_svg );
				if(ctx_main) free_context_by_name(ctx_main->name);
				if(ctx_outline) free_context_by_name(ctx_outline->name);
				if(ctx_drill) free_context_by_name(ctx_drill->name);
				return 1;
			}
			printf("Writing output to file: %s\n", output_svg );
		}
		walk_around_all_cont("svg", output_stream);
		if (output_stream != stdout) fclose(output_stream);
	}

	//-----> BORING
	output_stream = stdout;
	if (output_boring != NULL) {
		gcode_ini();
		if( strcmp(output_boring, "-") != 0 ){
			output_stream = fopen(output_boring, "w");
			if (output_stream == NULL) {
				fprintf(stderr, "Error: Cannot open output file '%s'\n", output_boring );
				if(ctx_main) free_context_by_name(ctx_main->name);
				if(ctx_outline) free_context_by_name(ctx_outline->name);
				if(ctx_drill) free_context_by_name(ctx_drill->name);
				return 1;
			}
			printf("Writing output to file: %s\n", output_boring );
		}
		boring_gcode( output_stream, &exc );
		if (output_stream != stdout) fclose(output_stream);
	}

	if(ctx_main) free_context_by_name(ctx_main->name);
	if(ctx_outline) free_context_by_name(ctx_outline->name);
	if(ctx_drill) free_context_by_name(ctx_drill->name);

	printf("\n");
	return 0;

}

