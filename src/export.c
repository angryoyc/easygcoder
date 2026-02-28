#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <math.h>

#include "../include/top.h"
#include "../include/conf.h"
#include "../include/reflist.h"
#include "../include/context.h"
#include "../include/cont.h"
#include "../include/point.h"
#include "../include/arc.h"
#include "../include/line.h"
#include "../include/mergcont.h"
#include "../include/gcode.h"
#include "../include/excellon.h"
#include "../include/apply_config.h"
#include "../include/export.h"


typedef struct {
	int W;
	int H;
	int dh;
	int dw;
	int M;
	int cx;
	int cy;
	int margin;
	int mirror_x;
	int mirror_y;
} Svg_context_t;

double svg_x(double v, Svg_context_t* env){
	return env->cx + env->M*v - env->dw;
}

double svg_y(double v, Svg_context_t* env){
	return env->cy - env->M*v + env->dh;
}

void walk_around_cont( Cont_t* cont, const char* mod, Svg_context_t* env, FILE* output_fd );


// Обход всех контуров в к контексте и вывод его состава в stdout
// Svg_context_t* env,
void walk_around_all_cont( const char* mod, FILE* output_fd ){
	Svg_context_t svg = {
		.W =env_i("svg_width"),
		.H =env_i("svg_height"),
		.dh=0,
		.dw=0,
		.M =env_i("svg_m"),
		.cx=0,
		.cy=0,
		.margin=env_i("svg_margin")
	};
	svg.cx = svg.W/2;
	svg.cy = svg.H/2;

	Context_t* ctx = get_context();
	context_get_bounds();

	if( (ctx->xmax*svg.M + 50 - svg.cx) > 0 ){
		svg.dw = (int) ((ctx->xmax * svg.M + 50 - svg.cx));
	}

	if( (ctx->ymax*svg.M + 50 - svg.cy) > 0 ){
		svg.dh = (int) ((ctx->ymax * svg.M + 50 - svg.cy));
	}

	if( (ctx->xmin*svg.M - 50 + svg.cx) < 0 ){
		svg.dw = (int) ((ctx->xmin * svg.M - 50 + svg.cx));
	}

	if( (ctx->ymin*svg.M - 50 + svg.cy) < 0 ){
		svg.dh = (int) ((ctx->ymin * svg.M - 50 + svg.cy));
	}

	if( strcmp( mod, "svg") == 0 ){
		fprintf(output_fd, "<svg width=\"%i\" height=\"%i\" xmlns=\"http://www.w3.org/2000/svg\">\n", svg.W, svg.H );
		fprintf(output_fd, "<path d=\"M %i %i L %i %i \" stroke=\"black\" stroke-width=\"1\" fill=\"none\" />\n", svg.cx-svg.dw, 0, svg.cx - svg.dw, svg.H );
		fprintf(output_fd, "<path d=\"M %i %i L %i %i \" stroke=\"black\" stroke-width=\"1\" fill=\"none\" />\n", 0, svg.cy + svg.dh, svg.W, svg.cy + svg.dh );
		fprintf(output_fd, "<path d=\"M %i %i L %i %i L %i %i L %i %i L %i %i\" stroke=\"black\" stroke-width=\"3\" fill=\"none\" />\n", 0, 0, 0, svg.H, svg.W, svg.H, svg.W, 0, 0, 0 );
	}

	if( strcmp( mod, "gcode") == 0 ){
		gcode_make_header( output_fd );
	}

	if( ctx->links.arr ){
		double start=0;
		double deep=-1;
		double step=-1;
		if( strcmp( mod, "gcode") == 0 ){
			start = env_d("milling_start");
			deep = env_d("milling_deep");
			step = env_d("milling_step");
		}
		//printf("\n\nmod = %s | start=%f; deep=%f; step=%f;\n\n", mod, start, deep, step);
		while((start+step)>=deep){
			if( strcmp( mod, "gcode") == 0 ) milling_start(start);
			for( int i=0; i < ctx->links.count; i++ ){
				Refitem_t* item1 = ctx->links.arr[i];
				if( item1->type == OBJ_TYPE_CONTUR ){
					walk_around_cont( (Cont_t*) item1, mod, &svg, output_fd );
				}
			}
			if( (start + step) == deep ) break;
			start = start + step;
			if( (start + step) < deep ) start = deep - step;
		}
	}

	if( strcmp( mod, "svg") == 0 ) fprintf(output_fd, "</svg>\n");
	if( strcmp( mod, "gcode") == 0 ){
		gcode_make_footer(output_fd);
	}
}

// Обход всех точек в к контексте и вывод связанных с ними объектами
void walk_around_all_points(){
	Context_t* ctx = get_context();
	if( ctx->links.arr ){
		for( int i=0; i < ctx->links.count; i++ ){
			Refitem_t* item1 = ctx->links.arr[i];
			if( item1->type == OBJ_TYPE_POINT ){
				walk_around_cont_points( (Point_t*) item1 );
			}
		}
	}
}

// Обход всех примитивов в к контексте и вывод связанных с ними точек
void walk_around_all_prims(){
	Context_t* ctx = get_context();
	if( ctx->links.arr ){
		for( int i=0; i < ctx->links.count; i++ ){
			Refitem_t* item1 = ctx->links.arr[i];
			if( item1->type == OBJ_TYPE_LINE ){
				print_line((Line_t*) item1);
				printf("\n");
			}else if( item1->type == OBJ_TYPE_ARC ){
				print_arc((Arc_t*) item1);
				printf("\n");
			}
		}
	}
}

void print_arc( Arc_t* arc ){
	if( arc->dir == 0 ){
		printf("Окружность с центом [%f, %f] ", arc->center.x, arc->center.y );
		printf("\n");
	}else{
		int a_linked = is_linked( (Refitem_t*)arc, (Refitem_t*)arc->a);
		int b_linked = is_linked( (Refitem_t*)arc, (Refitem_t*)arc->b);
		printf("Arc №%i с центом (%f, %f) и концами a:[%f, %f] %s --> b:[%f, %f] %s dir: %s R:%f ", arc->id, arc->center.x, arc->center.y, arc->a->x, arc->a->y, a_linked?"ok":"  ", arc->b->x, arc->b->y, a_linked?"ok":"  ", (arc->dir==-1)?"по часовой":"против часовой", arc->R );
	}
}

void print_line( Line_t* l ){
	int a_linked = is_linked((Refitem_t*)l, (Refitem_t*)l->a);
	int b_linked = is_linked((Refitem_t*)l, (Refitem_t*)l->b);
	printf("Line №%i [%f, %f] %s %i <--> [%f, %f] %s %i", l->id, l->a->x, l->a->y, a_linked?"ok":"  ", l->a->links.count, l->b->x, l->b->y, b_linked?"ok":"  ", l->b->links.count );
}

void print_p(Point_t* p){
	printf("P [%f, %f]", p->x, p->y );
}

void boring_gcode( FILE* output_fd, Excellon_t* exc ){
	gcode_make_header( output_fd );
	for( int i=0; i<exc->holes_count; i++ ){
		Hole_t h = exc->holes[i];
		gcode_drill(output_fd, h.x, h.y, h.d);
	}
	gcode_make_footer( output_fd );
};

/*
* Обход контура по порядку связанности
*/
void walk_around_cont( Cont_t* cont, const char* mod, Svg_context_t* svg, FILE* output_fd ){
	if( cont ){
		if( strcmp( mod, "gcode") == 0 ){
			gcode_drop_position();
			fprintf(output_fd, ";== start new cont\n");
			//gcode_internal_ensure_tool_up(output_fd);
			if( cont->links.arr ){
				for( int i=0; i < cont->links.count; i++ ){
					if( is_seg( cont->links.arr[i] ) ){
						Refitem_t* item = cont->links.arr[i];
						if( is_line(item) ){
							Line_t* line = (Line_t*) item;
							gcode_cut_line(output_fd, line);
						}else if( is_arc(item) ){
							Arc_t* arc = (Arc_t*) item;
							gcode_cut_arc(output_fd, arc);
						}
					}
				}
			}
			gcode_internal_ensure_tool_up(output_fd);
		}else if( strcmp( mod, "log") == 0 ){
			fprintf(output_fd, "Обход контура %i (Min contcount=%i) dir:%s\n", cont->num, cont->mincc, (cont->dir==1)?"ПротивЧасовой":"ПоЧасовой");
			if( cont->links.arr ){
				for( int i=0; i < cont->links.count; i++ ){
					Refitem_t* item = cont->links.arr[i];
					if( item->type == OBJ_TYPE_LINE ){
						Line_t* l = (Line_t*) item;
						fprintf(output_fd,  "Line A:(%f, %f)-B:(%f, %f) cc: %i\n", l->a->x, l->a->y, l->b->x, l->b->y, l->contcount );
					}else if(item->type == OBJ_TYPE_ARC){
						Arc_t* arc = (Arc_t*) item;
						if( (arc->dir == 0) || (arc->a == arc->b) ){
							fprintf(output_fd,  "Circle (%f, %f) R:%f, dir=%i cc:%i\n", arc->center.x, arc->center.y, arc->R, arc->dir, arc->contcount );
						}else{
							fprintf(output_fd,  "Arc (%f, %f) R:%f, dir:%s  A:(%f, %f)-B:(%f, %f) cc:%i\n", arc->center.x, arc->center.y, arc->R, (arc->dir==-1)?"ПЧ":"ПрЧ", arc->a->x, arc->a->y, arc->b->x, arc->b->y, arc->contcount );
						}
					}
				}
			}
		}else if( strcmp( mod, "svg") == 0 ){
			fprintf(output_fd,  "<g id=\"contour%i\" title=\"%i\">", cont->num, cont->dir );
			if( cont->links.arr  && (cont->links.count>0)){
				for( int i=cont->links.count-1; i >=0; i-- ){
					Refitem_t* item = cont->links.arr[i];
					if( item ){
						if(item->type == OBJ_TYPE_LINE){
							Line_t* l = (Line_t*) item;
							//fprintf(output_fd, "<path id=\"line%i\" d=\"M %f %f L %f %f\" stroke=\"red\" stroke-width=\"1\" fill=\"none\" title=\"%i\"/>\n", l->id, M*l->a->x + cx, cy - M*l->a->y, M*l->b->x + cx, cy - M*l->b->y, l->contcount );
							fprintf(output_fd, "<path id=\"line%i\" d=\"M %f %f L %f %f\" stroke=\"%s\" stroke-width=\"1\" fill=\"none\" title=\"%i\" inshadow=\"%i\"/>\n", l->id, svg_x(l->a->x, svg) , svg_y(l->a->y, svg), svg_x(l->b->x, svg), svg_y(l->b->y, svg), (cont->dir==1)?"green":"red", l->contcount, l->inshadow );
						}else if(item->type == OBJ_TYPE_ARC){
							Arc_t* arc = (Arc_t*) item;
							if( (arc->dir == 0) || (arc->a == arc->b) || (p_eq_p(arc->a, arc->b)) ){
								//fprintf(output_fd,  "<circle id=\"circle%i\" cx=\"%f\" cy=\"%f\" r=\"%f\" stroke=\"red\" stroke-width=\"1\" fill=\"none\"  title=\"%i\" />\n", arc->id, M*arc->center.x + cx, cy - M*arc->center.y, M*arc->R, arc->contcount );
								fprintf(output_fd, "<circle id=\"circle%i\" cx=\"%f\" cy=\"%f\" r=\"%f\" stroke=\"%s\" stroke-width=\"1\" fill=\"none\"  title=\"%i\" />\n", arc->id, svg_x(arc->center.x, svg), svg_y(arc->center.y, svg), svg->M*arc->R, (cont->dir==1)?"green":"red", arc->contcount );
							}else{
								//fprintf(output_fd,  "<path id=\"arc%i\" d=\"M %f %f A %f %f 0 %i %i %f %f\" stroke=\"red\" stroke-width=\"1\" fill=\"none\" title=\"%i\"/>\n", arc->id, M*arc->a->x + cx, cy - M*arc->a->y, M*arc->R, M*arc->R, ((get_svg_arc_flags(arc)&2)==2)?1:0, (arc->dir==-1)?1:0, M*arc->b->x + cx, cy - M*arc->b->y, arc->contcount);
								fprintf(output_fd, "<path id=\"arc%i\" d=\"M %f %f A %f %f 0 %i %i %f %f\" stroke=\"%s\" stroke-width=\"1\" fill=\"none\" title=\"%i\" inshadow=\"%i\"/>\n", arc->id, svg_x(arc->a->x, svg), svg_y(arc->a->y, svg), svg->M*arc->R, svg->M*arc->R, ((get_svg_arc_flags(arc)&2)==2)?1:0, (arc->dir==-1)?1:0, svg_x(arc->b->x, svg), svg_y(arc->b->y, svg), (cont->dir==1)?"green":"red",  arc->contcount, arc->inshadow);
								//svg_point(arc->a->x, arc->a->x, M, output_fd);
								//svg_point(arc->b->x, arc->b->x, M, output_fd);
							}
						}
					}
				}
			}
			fprintf(output_fd,  "</g>" );
		}
	}
	return;
}

// Флаг выборы дуги (для svg)
uint8_t get_svg_arc_flags(Arc_t* arc){
	uint8_t flags = 0;
	if (arc->dir == -1) {
		flags |= (1 << 0); // Устанавливаем Bit 0 в 1
	}
	double Vax = arc->a->x - arc->center.x;
	double Vay = arc->a->y - arc->center.y;
	double Vbx = arc->b->x - arc->center.x;
	double Vby = arc->b->y - arc->center.y;
	double dot_product = Vax * Vbx + Vay * Vby;
	double mag_A = sqrt(Vax * Vax + Vay * Vay);
	double mag_B = sqrt(Vbx * Vbx + Vby * Vby);
	if (mag_A < epsilon || mag_B < epsilon) {
		return flags; // large_arc_flag = 0
	}
	double denominator = mag_A * mag_B;
	double cos_alpha = dot_product / denominator;
	if (cos_alpha > 1.0) cos_alpha = 1.0;
	if (cos_alpha < -1.0) cos_alpha = -1.0;
	double alpha_rad = acos(cos_alpha);
	if (alpha_rad >= M_PI - epsilon) {
		flags |= (1 << 1); // Устанавливаем Bit 1 в 1
		return flags;
	}
	double cross_product = Vax * Vby - Vay * Vbx;
	int is_shortest_path_ccw = (cross_product >= 0);
	int is_path_shortest;
	if (arc->dir == -1) { // Заданный путь CW
		is_path_shortest = !is_shortest_path_ccw;
	} else { // Заданный путь CCW
		is_path_shortest = is_shortest_path_ccw;
	}
	if (!is_path_shortest) {
		flags |= (1 << 1); // Устанавливаем Bit 1 в 1
	}
	return flags;
}


void svg_arc(Arc_t* arc, Svg_context_t* svg ){
	if( (arc->dir == 0) || (arc->a == arc->b) ){
		printf( "<circle cx=\"%f\" cy=\"%f\" r=\"%f\" stroke=\"black\" stroke-width=\"1\" fill=\"none\" />\n", svg_x(arc->center.x, svg), svg_y(arc->center.y, svg), svg->M*arc->R );
	}else{
		printf( "<path d=\"M %f %f A %f %f 0 %i %i %f %f\" stroke=\"black\" stroke-width=\"1\" fill=\"none\" title=\"id_%i: %i\"/>\n", svg_x(arc->a->x, svg), svg_y(arc->a->y, svg), svg->M*arc->R, svg->M*arc->R, ((get_svg_arc_flags(arc)&2)==2)?1:0, (arc->dir==-1)?1:0, svg_x(arc->b->x, svg), svg_y(arc->b->y, svg), arc->id, arc->contcount);
	}
}

void svg_line(double x1, double y1, double x2, double y2, Svg_context_t* svg, FILE* output_fd ){
	fprintf(output_fd, "<path d=\"M %f %f L %f %f\" stroke=\"black\" stroke-width=\"1\" fill=\"none\" title=\"%f %f - %f %f\" />\n", svg_x(x1, svg), svg_y(y1, svg), svg_x(x2, svg), svg_y(y2, svg), x1, y1, x1, y2 );
}

void svg_point(double x, double y, Svg_context_t* svg, FILE* output_fd){
	fprintf(output_fd, "\n<circle cx=\"%f\" cy=\"%f\" r=\"%i\" stroke=\"black\" stroke-width=\"1\" fill=\"none\" />\n", svg_x(x, svg), svg_y(y, svg), 1 );
}

void walk_around_cont_points( Point_t* p ){
	if( p ){
		if( p->links.arr ){
			printf( "\n\nТочка (%f,%f) %p :\n", p->x, p->y, p );
			for( int idx=0; idx < p->links.count; idx++){
				if(((Refitem_t*) p->links.arr[idx])->type == OBJ_TYPE_LINE){
					Line_t* line = (Line_t*) p->links.arr[idx];
					printf("Line #%i ",  line->id );
					print_p(line->a);
					printf("|");
					print_p(line->b);
					printf("|");
					if( line->a == p ) printf("A->");
					if( line->b == p ) printf("B->");
					printf("\n");
				}
				if(((Refitem_t*) p->links.arr[idx])->type == OBJ_TYPE_ARC){
					Arc_t* arc = (Arc_t*) p->links.arr[idx];
					printf("Arc #%i ",  arc->id);
					print_p(arc->a);
					printf("|");
					print_p(arc->b);
					printf("|");
					if( arc->a == p ) printf("A->");
					if( arc->b == p ) printf("B->");
					printf("\n");
				}
				if(((Refitem_t*) p->links.arr[idx])->type == OBJ_TYPE_CONTEXT) printf("CONTEXT for (%f,%f)\n", p->x, p->y);
				if(((Refitem_t*) p->links.arr[idx])->type == OBJ_TYPE_CONTUR)  printf("CONTUR for (%f,%f)\n",  p->x, p->y);
			}
		}
	}
}

/*
void print_vector( Vector_t* v){
	printf("dx, dy - (%f, %f) (k=%f) ", v->dx, v->dy, v->k );
}
*/

void print_item( Refitem_t* item ){
	if( is_line(item) ){
		print_line((Line_t*) item);
	}else if( is_arc(item) ){
		print_arc((Arc_t*) item);
	}
};
