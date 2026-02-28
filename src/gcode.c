#include <stdio.h>
#include <math.h>
#include <stdint.h>

#include "../include/top.h"
#include "../include/point.h"
#include "../include/arc.h"
#include "../include/line.h"
#include "../include/geom.h"
#include "../include/conf.h"
#include "../include/apply_config.h"

//#include "../include/excellon.h"
#include "../include/gcode.h"

//G0 X120.900 Y49.280
//G2 X120.900 Y47.580 R0.850 F1000.000
//G2 X120.900 Y49.280 R0.850 F1000.000

typedef struct{
    double current_x;
    double current_y;
    int tool_down;
    int position_valid;
    double tool_ch_height;
    double milling_safe;
    double milling_start;
    double milling_deep;
    double milling_step;
    int    milling_feed;
    double drill_safe;
    double drill_start;
    double drill_deep;
    double drill_step;
    int    drill_feed;
} GcodeContext_t;


static GcodeContext_t gcd = {
    .current_x      = 0.0,
    .current_y      = 0.0,
    .tool_down      = 0,
    .tool_ch_height = 50,
    .position_valid = 0,
    .milling_safe   = 5.0,
    .milling_start  = 0.0,
    .milling_deep   = -0.1,
    .milling_step   = -0.1,
    .milling_feed   = 100.0,
    .drill_safe     = 5.0,
    .drill_start    =  0.1,
    .drill_deep     = -2.3,
    .drill_step     = -0.4,
    .drill_feed     = 50
};

static double round3(double v){ return round(v*1000.0)/1000.0; }

void gcode_ini(){
    gcd.current_x      = 0.0;
    gcd.current_y      = 0.0;
    gcd.tool_down      = 0;
    gcd.position_valid = 0;

    gcd.tool_ch_height = env_d("tool_ch_height");

    gcd.milling_safe   = env_d("milling_safe");
    gcd.milling_start  = env_d("milling_start");
    gcd.milling_deep   = env_d("milling_deep");
    gcd.milling_step   = env_d("milling_step");
    gcd.milling_feed   = env_i("milling_feed");
    gcd.drill_safe     = env_d("drill_safe");
    gcd.drill_start    = env_d("drill_start");
    gcd.drill_deep     = env_d("drill_deep");
    gcd.drill_step     = env_d("drill_step");
    gcd.drill_feed     = env_i("drill_feed");
}


void milling_start(double val){
    gcd.milling_start  = val;
}

void gcode_drop_position(){
    gcd.position_valid = 0;
}

void gcode_internal_ensure_tool_up(FILE* output_fd){
    if(gcd.tool_down){
        fprintf(output_fd, "G0 Z%.3f\n", round3(gcd.milling_safe) ); // round3(gcd.milling_safe)
        gcd.tool_down = 0;
    }
}

void gcode_internal_ensure_tool_down(FILE* output_fd){
    if(!gcd.tool_down){
        fprintf(output_fd, "G0 Z%.3f\n", round3(gcd.milling_start) );
        //printf("gcd.milling_feed: %f\n", gcd.milling_feed);
        fprintf(output_fd, "G1 Z%.3f F%i\n", round3( gcd.milling_start + gcd.milling_step ), gcd.milling_feed );
        gcd.tool_down = 1;
    }
}

void gcode_make_header(FILE* output_fd){
    fprintf(output_fd, "%%\n");
    fprintf(output_fd, "G21\n");
    fprintf(output_fd, "G90\n");
    fprintf(output_fd, "G17\n");
    gcode_internal_ensure_tool_up(output_fd);
    fprintf(output_fd, "G0 Z%.3f\n", round3(gcd.milling_safe));
}

void gcode_make_footer(FILE* output_fd){
    gcode_internal_ensure_tool_up( output_fd );
    fprintf( output_fd, "G0 X0 Y0\n");
    fprintf( output_fd, "G0 Z%.3f\n", round3(gcd.tool_ch_height) );
    fprintf(output_fd, "M30\n");
    fprintf(output_fd, "%%\n");
}

void gcode_moveto_xy(FILE* output_fd, double x, double y){
    gcode_internal_ensure_tool_up(output_fd);
    fprintf(output_fd, "G0 X%.3f Y%.3f\n", round3(x), round3(y) );
    gcd.current_x = x;
    gcd.current_y = y;
}

void gcode_moveto(FILE* output_fd, Point_t* p){
    gcode_internal_ensure_tool_up(output_fd);
    if(fabs(gcd.current_x-p->x)<1e-9 && fabs(gcd.current_y-p->y)<1e-9) return;
    fprintf(output_fd, "G0 X%.3f Y%.3f\n", round3(p->x), round3(p->y));
    gcd.current_x=p->x;
    gcd.current_y=p->y;
}

static double prev_d = 0;

void gcode_drill(FILE* output_fd, double x, double y, double d){
	if( (prev_d!=d) && (prev_d!=0) ){
		gcode_internal_ensure_tool_up(output_fd);
		fprintf( output_fd, "G0 X0 Y0\n");
		fprintf( output_fd, "G0 Z%.3f\n", round3(gcd.tool_ch_height) );
		fprintf( output_fd, "(Change tool d=%0.3f)\n", d);
		fprintf( output_fd, "M0\n");
		prev_d = d;
	}
	fprintf( output_fd, "G0 Z%.3f\n", round3(gcd.drill_safe) );
	fprintf( output_fd, "G0 X%.3f Y%.3f\n", x, y );
	for( double i = gcd.drill_start+gcd.drill_step; i >= round3(gcd.drill_deep); i = i + round3(gcd.drill_step) ){
		fprintf( output_fd, "G0 Z%.3f\n", round3( i - gcd.drill_step ) );
		fprintf( output_fd, "G1 Z%.3f F%i\n", round3( i ), gcd.drill_feed );
		fprintf( output_fd, "G0 Z%.3f\n", round3( gcd.drill_safe ) );
	}
}

void gcode_cut_line(FILE* output_fd, Line_t* line){
	//if(fabs(gcd.current_x-line->a->x)>1e-9 || fabs(gcd.current_y-line->a->y)>1e-9){
	if( !gcd.position_valid ){
		gcd.position_valid = 1;
		if(fabs(gcd.current_x-line->a->x)>1e-9 || fabs(gcd.current_y-line->a->y)>1e-9){
			//gcode_internal_ensure_tool_up(output_fd);
			gcode_moveto(output_fd, line->a);
		}
	}
	gcode_internal_ensure_tool_down(output_fd);
	fprintf(output_fd,"G1 X%.3f Y%.3f F%i\n", round3(line->b->x), round3(line->b->y), gcd.milling_feed);
	gcd.current_x=line->b->x;
	gcd.current_y=line->b->y;
}

void cut_arc_in_ij(FILE* output_fd, Arc_t* arc, int dir, double start_x, double start_y, double stop_x, double stop_y){
	fprintf(output_fd, "G%s X%.3f Y%.3f I%.3f J%.3f F%i\n", (dir==-1)?"2":"3", round3(stop_x), round3(stop_y), round3(arc->center.x - start_x), round3(arc->center.y - start_y), gcd.milling_feed);
}

void gcode_cut_arc(FILE* output_fd, Arc_t* arc ){
    double start_x;
    double start_y;
    double stop_x;
    double stop_y;
    if( arc->dir == 0 ){
        if( !gcd.position_valid ){ //необходимо определение стартовой позиции
            start_x = arc->center.x;
            start_y = arc->center.y + arc->R;
            stop_x = arc->center.x;
            stop_y = arc->center.y - arc->R;
            //gcode_internal_ensure_tool_up(output_fd);
            gcode_moveto_xy(output_fd, start_x, start_y);
            gcd.position_valid = 1; // теперь позиция известна
        }else{
            start_x = gcd.current_x;
            start_y = gcd.current_y;
            double dx = arc->center.x-start_x;
            double dy = arc->center.y-start_y;
            stop_x = arc->center.x + dx;
            stop_y = arc->center.y + dy;
        }

        gcode_internal_ensure_tool_down(output_fd);

        cut_arc_in_ij(output_fd, arc, -1, start_x, start_y, stop_x, stop_y );
        //fprintf( output_fd, "G2 X%.3f Y%.3f R%.3f F%i\n", round3(stop_x), round3(stop_y), round3(arc->R), gcd.milling_feed );
        gcd.current_x = stop_x;
        gcd.current_y = stop_y;

        cut_arc_in_ij(output_fd, arc, -1, stop_x, stop_y, start_x, start_y );
        //fprintf( output_fd, "G2 X%.3f Y%.3f R%.3f F%i\n", round3(start_x), round3(start_y), round3(arc->R), gcd.milling_feed );
        gcd.current_x = start_x;
        gcd.current_y = start_y;
        return;
    }

    if( arc->dir != 0 ){
        start_x = arc->a->x;
        start_y = arc->a->y;
        if( !gcd.position_valid ){
            gcd.position_valid = 1; // теперь позиция известна
            gcode_moveto(output_fd, arc->a);
        }
        gcode_internal_ensure_tool_down(output_fd);
        if( p_eq_p(arc->a, arc->b) ){ // снова полная окружность?
            double dx = arc->center.x-start_x;
            double dy = arc->center.y-start_y;
            stop_x = arc->center.x + dx;
            stop_y = arc->center.y + dy;
            cut_arc_in_ij(output_fd, arc, -1, start_x, start_y, stop_x, stop_y );
            //fprintf( output_fd, "G2 X%.3f Y%.3f R%.3f F%i\n", round3(stop_x), round3(stop_y), round3(arc->R), gcd.milling_feed );
            gcd.current_x = stop_x;
            gcd.current_y = stop_y;
            cut_arc_in_ij(output_fd, arc, -1, stop_x, stop_y, start_x, start_y );
            //fprintf( output_fd, "G2 X%.3f Y%.3f R%.3f F%i\n", round3(start_x), round3(start_y), round3(arc->R), gcd.milling_feed );
            gcd.current_x = start_x;
            gcd.current_y = start_y;
            return;
        }else{ // здесь полноценная дуга от начала в конец.
            //fprintf( output_fd, ";// simple arc\n");
            double mid_x = 0;
            double mid_y = 0;
            get_xy_of_arc_mid( arc, &mid_x, &mid_y );
            cut_arc_in_ij(output_fd, arc, arc->dir, arc->a->x, arc->a->y, mid_x, mid_y );
            cut_arc_in_ij(output_fd, arc, arc->dir, mid_x, mid_y, arc->b->x, arc->b->y );
            //fprintf( output_fd, "G%d X%.3f Y%.3f R%.3f F%i\n", (arc->dir==-1)?2:3, round3(arc->b->x), round3(arc->b->y), round3(arc->R), gcd.milling_feed );
            gcd.current_x = arc->b->x;
            gcd.current_y = arc->b->y;
        }
    }
}
