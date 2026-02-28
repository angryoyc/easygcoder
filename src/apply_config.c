#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <stdbool.h>
#include <ctype.h>

#include "../include/define.h"
#include "../include/top.h"
#include "../include/conf.h"
#include "../include/smart_split.h"

typedef struct {
    char name[20];
    int type;
    char val[32];
    char* source;
} Param_t;

typedef struct {      // хранение интервало свёрел
	double minval;    // Начиная с этого диаметра
	double maxval;    // Заканчивая этим диаметром
	double nomval;    // Заменять на этот диаметр
	char* source; // Откуда взят этот интервал
} Drill_t;

char* end;

static Param_t params[100];
static Drill_t drills[100]; // интервалы замены свёрел
static int params_count=0;
static int drills_count=0;

double norm_dia(double d){
	for( int i=0; i<drills_count; i++){
		if( (drills[i].minval <= d) && (d < drills[i].maxval) ) return drills[i].nomval;
	}
	return d;
}

int env_i(char* name){
	for( int i=0; i<params_count; i++){
		if( strcmp(name, params[i].name )==0 ){
			int restored;
			memcpy(&restored, params[i].val, sizeof(int));
			return restored;
		}
	}
	return 0;
}

double env_d(char* name){
	for( int i=0; i<params_count; i++){
		if( strcmp(name, params[i].name)==0 ){
			double restored;
			memcpy(&restored, params[i].val, sizeof(double));
			return restored;
		}
	}
	return 0;
}

void set_int_param(struct pair ** config, char* name, int defval, char* conffile){
	char* strval = NULL;
	if( config ) strval = conf_get(config, name);
	int val;
	int i;
	int index = -1;
	int defined = 0;
	for( i=0; i<params_count; i++){
		if( strcmp(name, params[i].name)==0 ){
			index = i;
			break;
		}
	}
	if( strval && (strlen(strval))>0 ){
		val = strtol(strval , &end, 10 );
		if( val <= 0 ){
			fprintf(stderr, "Config error in parameter %s\n", name);
			exit(1);
		};
		defined = 1;
	}
	if( (index<0) || defined ){
		Param_t prm;
		memset(&prm, '\x00', sizeof(Param_t));
		strcpy((char *) &prm.name, name );
		prm.type = 1;
		if( !defined){
			val = defval;
			prm.source = "dafault";
		}else{
			prm.source = conffile;
		}
		memcpy(prm.val, &val, sizeof(int));
		if( index<0 ) index = params_count++;
		params[index] = prm;
	}
}

void set_dbl_param(struct pair ** config, char* name, double defval, char* conffile){
	char* strval = NULL;
	if( config ) strval = conf_get(config, name);
	double val;
	int i;
	int index = -1;
	int defined = 0;
	for( i=0; i<params_count; i++){
		if( strcmp(name, params[i].name)==0 ){
			index = i;
			break;
		}
	}
	if( strval && (strlen(strval))>0 ){
		val = strtod(strval, &end );
		if(  end == strval ){
			fprintf(stderr, "Config error in parameter %s\n", name);
			exit(1);
		};
		defined = 1;
	}
	if( (index<0) || defined ){
		Param_t prm;
		memset(&prm, '\x00', sizeof(Param_t));
		strcpy((char *) &prm.name, name );
		prm.type = 2;
		if( !defined){
			val = defval;
			prm.source = "default";
		}else{
			prm.source = conffile;
		}
		memcpy(prm.val, &val, sizeof(double));
		if( index<0 ) index = params_count++;
		params[index] = prm;
	}
}

void set_drills(struct pair ** config, char* conffile){
	if( config ){
		char** intervals = conf_aget(config, "drill");
		int intervals_len = 0;
		while( intervals[intervals_len]!=NULL ){
			SplitResult res = smart_split(intervals[intervals_len], NULL);
			if( res.count == 3 ){
				Drill_t d;
				d.source = conffile;
				d.minval = strtod(res.tokens[0], &end );
				d.maxval = strtod(res.tokens[1], &end );
				d.nomval = strtod(res.tokens[2], &end );
				if( d.minval >= d.maxval ) printf("WARNING! Invalid drill interval boundaries: %f >= %f\n", d.minval, d.maxval);
				int i;
				int index = -1;
				for( i=0; i<drills_count; i++){
					if( (drills[i].minval == d.minval) && (drills[i].maxval == d.maxval) ){
						index = i;
						break;
					}
				}
				if( index<0 ){ // новый интервал
					if( drills_count < 100 ){ //
						index = drills_count++;
					}else{
						intervals_len++;
						printf("WARNING! Too many intervals! Skip %s\n", intervals[intervals_len]);
						continue;
					}
				}
				drills[index] = d;
			}else{
				printf("WARNING! Unknown drill description format: %s. Skip.\n", intervals[intervals_len]);
			}
			split_result_free(&res);
			intervals_len++;
		}
	}
}

void print_env(){
//	printf("Version: %s\n", VERSION);
	printf("Default config: %s\n", CONFIGPATH);
	printf("Loaded parameters (%i):\n", params_count);
	for(int i=0; i<params_count; i++){
		if( params[i].type == 1 ){
			printf("%s = %i\t\t(%s)\n", params[i].name, env_i(params[i].name), params[i].source );
		}else if( params[i].type == 2 ){
			printf("%s = %.3f\t\t(%s)\n", params[i].name, env_d(params[i].name), params[i].source );
		}
	}
	printf("Loaded intervals (%i):\n", drills_count);
	for(int i=0; i<drills_count; i++){
		printf("drill interval: %0.3f - %0.3f => %0.3f (%s)\n", drills[i].minval, drills[i].maxval, drills[i].nomval, drills[i].source);
	}
}

int test_env(char* block){
	if( (strcmp(block, "milling")==0) || (strcmp(block, "all")==0) ){
		double milling_start = env_d("milling_start");
		double milling_deep = env_d("milling_deep");
		double milling_step = env_d("milling_step");
		if( fabs(milling_step) < 0.001f ){
			fprintf(stderr, "WARNING! Potentially problematic milling parameters:\nToo small step\n");
			return 0;
		}
		if( (milling_start > milling_deep) && (milling_step>0)){
			fprintf(stderr, "WARNING! Potentially problematic milling parameters:\nThe step has the wrong sign. For these values, \"milling_start\" and \"milling_deep\" must be negative.\n");
			return 0;
		}
		if( (milling_start < milling_deep) && (milling_step<0)){
			fprintf(stderr, "WARNING! Potentially problematic milling parameters:\nThe step has the wrong sign. For these values, \"milling_start\" and \"milling_deep\" must be positive.\n");
			return 0;
		}
	}

	if( (strcmp(block, "drill")==0) || (strcmp(block, "all")==0) ){
		double drill_start = env_d("drill_start");
		double drill_deep = env_d("drill_deep");
		double drill_step = env_d("drill_step");
		if( fabs(drill_step) < 0.001f ){
			fprintf(stderr, "WARNING! Potentially problematic drill parameters:\nToo small step\n");
			return 0;
		}
		if( (drill_start > drill_deep) && (drill_step>0) ){
			fprintf(stderr, "WARNING! Potentially problematic drill parameters:\nThe step has the wrong sign. For these values, \"drill_start\" and \"drill_deep\" must be negative.\n");
			return 0;
		}
		if( (drill_start < drill_deep) && (drill_step<0) ){
			fprintf(stderr, "WARNING! Potentially problematic drill parameters:\nThe step has the wrong sign. For these values, \"drill_start\" and \"drill_deep\" must be positive.\n");
			return 0;
		}
	}

	return 1;
}

// , Svg_env_t* env
void apply_config(char* conffile){
	struct pair ** config = NULL;
	if( conffile ) config = conf_load( conffile );

	set_dbl_param(config, "tool_d", TOOL_D, conffile);
	set_dbl_param(config, "tool_ch_height", TOOL_CH_HEIGHT, conffile);

	set_int_param(config, "svg_m", SVG_M, conffile);
	set_int_param(config, "svg_width", SVG_WIDTH, conffile);
	set_int_param(config, "svg_height", SVG_HEIGHT, conffile);
	set_int_param(config, "svg_margin", SVG_MARGIN, conffile);

	set_dbl_param(config, "drill_safe", DRILL_SAFE, conffile);
	set_dbl_param(config, "drill_start", DRILL_START, conffile);
	set_dbl_param(config, "drill_deep", DRILL_DEEP, conffile);
	set_dbl_param(config, "drill_step", DRILL_STEP, conffile);
	set_int_param(config, "drill_feed", DRILL_FEED, conffile);

	set_dbl_param(config, "milling_safe", MILLING_SAFE, conffile);
	set_dbl_param(config, "milling_start", MILLING_START, conffile);
	set_dbl_param(config, "milling_deep", MILLING_DEEP, conffile);
	set_dbl_param(config, "milling_step", MILLING_STEP, conffile);
	set_int_param(config, "milling_feed", MILLING_FEED, conffile);

	set_drills(config, conffile);
}

#include "./smart_split.c"

