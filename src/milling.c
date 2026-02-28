#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>
#include <math.h>

#include "../include/top.h"
#include "../include/reflist.h"
#include "../include/context.h"
#include "../include/point.h"
#include "../include/geom.h"
#include "../include/cont.h"
#include "../include/line.h"
#include "../include/arc.h"

#define MAX_VERTICES 6

void line_milling( double x1, double y1, double x2, double y2, double R, int dir, Cont_t** cont1 ){
	double cx, cy, dx, dy, ex, ey, fx, fy;
	double dist = distance( x1, y1, x2, y2 );
	*cont1 = NULL;
	if( dist < epsilon ){
		//printf("// здесь просто рисуем окружность\n");
		*cont1 = create_cont();
		Arc_t* arc1 = create_arc(x1, y1, R);
		add_item2cont( (Refitem_t*) arc1, *cont1);
		cont_reorder(*cont1, dir); // Направление контура определяет с какой стороны конутра заполнение. Заполнение всегда СПРАВА. Если обходим контур по часовой стрелки, значит заполняем контур, если обходим против, то заполняем всё, кроме контура
		return;
	}else{
		if( create_px4_of_rect_by_2p( x1, y1, x2, y2, R, &cx, &cy, &dx, &dy, &ex, &ey, &fx, &fy ) ){
			*cont1 = create_cont();
			Point_t* c = create_p(cx, cy);
			Point_t* d = create_p(dx, dy);
			Point_t* e = create_p(ex, ey);
			Point_t* f = create_p(fx, fy);
			// Дуга на старте
			Arc_t* arc1 = create_arc(x1, y1, R);
			add_item2cont( (Refitem_t*) arc1, *cont1);
			break_the_circle(arc1, d, dir);
			Arc_t* new_arc1 = split_arc_by_p(arc1, c);
			remove_arc( &new_arc1 );
			// ПРЯМАЯ1 слева, по движению
			add_item2cont( (Refitem_t*) create_line( c, e ), *cont1);
			// Дуга на финише
			Arc_t* arc2 = create_arc(x2, y2, R);
			add_item2cont( (Refitem_t*) arc2, *cont1);
			break_the_circle(arc2, e, dir);
			Arc_t* new_arc2 = split_arc_by_p(arc2, f);
			remove_arc( &new_arc2 );
			// ПРЯМАЯ2 справа, по движению
			add_item2cont( (Refitem_t*) create_line( f, d ), *cont1);
			cont_reorder(*cont1, dir); // Направление контура определяет с какой стороны конутра заполнение. Заполнение всегда СПРАВА. Если обходим контур по часовой стрелки, значит заполняем контур, если обходим против, то заполняем всё, кроме контура
		}
	}
	return;
};

typedef struct {
    double x;
    double y;
} P;

/*
* фрезеровка по дуге (создаём контур, оставляемый круглым инструментом при движении по дуге или окружности)
*/
void arc_milling(Arc_t* arc, double r, int dir, Cont_t** cont1, Cont_t** cont2){
//	int dir = (arc->dir!=0)?arc->dir:(arc->cont->dir);
	double R = arc->R;
	*cont1 = NULL;
	*cont2 = NULL;
	Point_t* c = &(arc->center);
	// 1. Обработка полной окружности
	*cont1 = create_cont();
	if(arc->dir == 0 || arc->a == NULL || arc->b == NULL || distance(arc->a->x, arc->a->y, arc->b->x, arc->b->y) < epsilon) {
		// Внешнее кольцо
		Arc_t* outer_ring = create_arc(c->x, c->y, R + r);
		add_item2cont( (Refitem_t*) outer_ring, *cont1 );
		cont_reorder( *cont1, dir );
		// Внутреннее кольцо (только если R > r)
		if( fabs(R - r) > epsilon ){
			Arc_t* inner_ring = create_arc(c->x, c->y, R - r);
			*cont2 = create_cont();
			add_item2cont( (Refitem_t*) inner_ring, *cont2 );
			cont_reorder( *cont2, dir * (-1) ); // Внутреннее кольцо обходим против часовой, потому что заполнение для него справа,  а слева пустота
		}
		return;
	}
	if( fabs(R - r) < epsilon ){ // случай, когда радиус дуги траектории равен радиусу инструмента
		double k_out = (R + r) / R;
		// Смещение для точки A
		double dx_a = arc->a->x - c->x;
		double dy_a = arc->a->y - c->y;
		double dx_b = arc->b->x - c->x;
		double dy_b = arc->b->y - c->y;
		Point_t *p_out_a = create_p(c->x + dx_a * k_out, c->y + dy_a * k_out);
		Point_t *p_out_b = create_p(c->x + dx_b * k_out, c->y + dy_b * k_out);
		Arc_t* arc_top = create_arc(c->x, c->y, R + r);
		add_item2cont( (Refitem_t*) arc_top, *cont1 );
		break_the_circle( arc_top, p_out_a, dir );
		Arc_t* new_arc_top = split_arc_by_p( arc_top, p_out_b );
		remove_arc(&new_arc_top);
		double x1 = 0;
		double y1 = 0;
		double x2 = 0;
		double y2 = 0;
		Arc_t* arc_a = create_arc( arc->a->x, arc->a->y, r );
		Arc_t* arc_b = create_arc( arc->b->x, arc->b->y, r );
		add_item2cont( (Refitem_t*) arc_a, *cont1 );
		add_item2cont( (Refitem_t*) arc_b, *cont1 );
		Point_t* p_c = NULL;
		double dist = p_to_line_distance( arc->center.x, arc->center.y, arc->a->x, arc->a->y, arc->b->x, arc->b->y );
		if( (dist * arc->dir) >= 0 ){
			p_c = create_p( arc->center.x, arc->center.y);
		}else{
//			uint8_t n = xy_of_arc_x_arc( arc_a,  arc_b, &x1,  &y1,  &x2,  &y2 );
			double dist1  = p_to_line_distance( x1, y1, arc->a->x, arc->a->y, arc->b->x, arc->b->y );
			double dist2  = p_to_line_distance( x2, y2, arc->a->x, arc->a->y, arc->b->x, arc->b->y );
			if( (dist1 * arc->dir) >= 0 ){
				p_c = create_p( x1, y1 );
			}else if( (dist2 * arc->dir) >=0 ){
				p_c = create_p( x2, y2 );
			}
		}
		if( p_c ){
			break_the_circle( arc_b, p_out_b, arc->dir );
			Arc_t* new_arc_b = split_arc_by_p( arc_b, p_c );
			remove_arc(&new_arc_b);
			break_the_circle( arc_a, p_c, arc->dir );
			Arc_t* new_arc_a = split_arc_by_p( arc_a, p_out_a );
			remove_arc(&new_arc_a);
		}
		cont_reorder( *cont1, dir );
		return;
	}
	if( R < r ){ // случай, когда радиус инструмента больше радиуса дуги
		double k_out = (R + r) / R;
		// Смещение для точки A
		double dx_a = arc->a->x - c->x;
		double dy_a = arc->a->y - c->y;
		double dx_b = arc->b->x - c->x;
		double dy_b = arc->b->y - c->y;
		Point_t *p_out_a = create_p(c->x + dx_a * k_out, c->y + dy_a * k_out);
		Point_t *p_out_b = create_p(c->x + dx_b * k_out, c->y + dy_b * k_out);
		Arc_t* arc_top = create_arc(c->x, c->y, R + r);
		add_item2cont( (Refitem_t*) arc_top, *cont1 );
		break_the_circle( arc_top, p_out_a, arc->dir );
		Arc_t* new_arc_top = split_arc_by_p( arc_top, p_out_b );
		remove_arc(&new_arc_top);
		double x1 = 0;
		double y1 = 0;
		double x2 = 0;
		double y2 = 0;
		Arc_t* arc_a = create_arc( arc->a->x, arc->a->y, r );
		Arc_t* arc_b = create_arc( arc->b->x, arc->b->y, r );
		add_item2cont( (Refitem_t*) arc_a, *cont1 );
		add_item2cont( (Refitem_t*) arc_b, *cont1 );
		Point_t* p_c = NULL;
//		double dist = p_to_line_distance( arc->center.x, arc->center.y, arc->a->x, arc->a->y, arc->b->x, arc->b->y );
//		uint8_t n = xy_of_arc_x_arc( arc_a,  arc_b, &x1,  &y1,  &x2,  &y2 );
		double dist1  = p_to_line_distance( x1, y1, arc->a->x, arc->a->y, arc->b->x, arc->b->y );
		double dist2  = p_to_line_distance( x2, y2, arc->a->x, arc->a->y, arc->b->x, arc->b->y );
		if( (dist1 * arc->dir) >= 0 ){
			p_c = create_p( x1, y1 );
		}else if( (dist2 * arc->dir) >=0 ){
			p_c = create_p( x2, y2 );
		}
		if( p_c ){
			break_the_circle( arc_b, p_out_b, arc->dir );
			Arc_t* new_arc_b = split_arc_by_p( arc_b, p_c );
			remove_arc(&new_arc_b);
			break_the_circle( arc_a, p_c, arc->dir );
			Arc_t* new_arc_a = split_arc_by_p( arc_a, p_out_a );
			remove_arc(&new_arc_a);
		}
		cont_reorder( *cont1, dir );
		return;
	}
	// 2. Подготовка базовых точек (нормали в концах дуги)
	// Находим векторы от центра к точкам a и b, чтобы отложить r
	Point_t *p_out_a = NULL;
	Point_t *p_out_b = NULL;
	Point_t *p_in_a = NULL;
	Point_t *p_in_b = NULL;
	double k_out = (R + r) / R;
	double k_in  = (R - r) / R;

	// Смещение для точки A
	double dx_a = arc->a->x - c->x;
	double dy_a = arc->a->y - c->y;
	p_out_a = create_p(c->x + dx_a * k_out, c->y + dy_a * k_out);
	p_in_a = create_p(c->x + dx_a * k_in, c->y + dy_a * k_in);
	// Смещение для точки B
	double dx_b = arc->b->x - c->x;
	double dy_b = arc->b->y - c->y;
	p_out_b = create_p(c->x + dx_b * k_out, c->y + dy_b * k_out);
	p_in_b = create_p(c->x + dx_b * k_in, c->y + dy_b * k_in);

	Arc_t* arc_top = create_arc(c->x, c->y, R + r);							//arc_top
	break_the_circle( arc_top, p_out_a, arc->dir );
	Arc_t* new_arc_top = split_arc_by_p( arc_top, p_out_b );
	remove_arc(&new_arc_top);

	Arc_t* arc_bot = create_arc(c->x, c->y, R - r);							////arc_bot
	break_the_circle( arc_bot, p_in_a, arc->dir );
	Arc_t* new_arc_bot = split_arc_by_p( arc_bot, p_in_b );
	remove_arc(&new_arc_bot);

	// 3. Анализ пересечения торцевых окружностей (Случай 3.3)
	double p1x, p1y, p2x, p2y;
	Arc_t* arc_a = create_arc( arc->a->x, arc->a->y, r );					// arc_a
	Arc_t* arc_b = create_arc( arc->b->x, arc->b->y, r );					// arc_b

	uint8_t n = xy_of_arc_x_arc( arc_a, arc_b, &p1x, &p1y, &p2x, &p2y );

	break_the_circle( arc_a, p_in_a, arc->dir );
	Arc_t* new_arc_a = split_arc_by_p( arc_a, p_out_a );
	remove_arc(&new_arc_a);

	break_the_circle( arc_b, p_out_b, arc->dir );
	Arc_t* new_arc_b = split_arc_by_p( arc_b, p_in_b );
	remove_arc(&new_arc_b);

	if(n >= 2){
		if( is_xy_on_arc( arc_a, p1x, p1y) && is_xy_on_arc( arc_a, p2x, p2y) && is_xy_on_arc( arc_b, p1x, p1y) && is_xy_on_arc( arc_b, p2x, p2y) ){
			Point_t *p1 = NULL;
			Point_t *p2 = NULL;
			if( distance(c->x, c->y, p1x, p1y) <  distance(c->x, c->y, p2x, p2y) ){
				p1 = create_p(p1x, p1y);
				p2 = create_p(p2x, p2y);
			}else{
				p1 = create_p(p2x, p2y);
				p2 = create_p(p1x, p1y);
			}
			*cont2 = create_cont();
			add_item2cont( (Refitem_t*) arc_top, *cont1 );
			add_item2cont( (Refitem_t*) arc_bot, *cont2 );

			Arc_t* new_arc_a = split_arc_by_p( arc_a, p1 );
			Arc_t* new_new_arc_a = split_arc_by_p( new_arc_a, p2 );			// new_new_arc_a
			remove_arc(&new_arc_a);

			add_item2cont( (Refitem_t*) arc_a, *cont2 );
			add_item2cont( (Refitem_t*) new_new_arc_a, *cont1 );
			cont_reorder( *cont1, dir );

			Arc_t* new_arc_b = split_arc_by_p( arc_b, p2 );
			Arc_t* new_new_arc_b = split_arc_by_p( new_arc_b, p1 );			// new_new_arc_b
			remove_arc(&new_arc_b);

			add_item2cont( (Refitem_t*) arc_b, *cont1 );
			add_item2cont( (Refitem_t*) new_new_arc_b, *cont2 );
			cont_reorder( *cont2, dir*(-1) );
		}
	}else{
		add_item2cont( (Refitem_t*) arc_top, *cont1 );
		add_item2cont( (Refitem_t*) arc_bot, *cont1 );
		add_item2cont( (Refitem_t*) arc_a, *cont1 );
		add_item2cont( (Refitem_t*) arc_b, *cont1 );
		int d = cont_reorder( *cont1, dir );
		//printf( "полный формат %i %i\n", (*cont1)->dir, d );
	}
	return;
}



int calc_points( double x1, double y1, double x2, double y2, double W, double H, Point_t* contour_points[MAX_VERTICES] ){
	int ret = 0;
    double w = W / 2.0;
    double h = H / 2.0;
    double dX = x2 - x1;
    double dY = y2 - y1;
    // --- 1. Определение 8 угловых точек ---
    // Для минимализма и простоты, используем локальные переменные
    // P1 Corners: A1(LL), B1(RL), C1(RU), D1(LU)
    P A1 = {x1 - w, y1 - h};
    P B1 = {x1 + w, y1 - h};
    P C1 = {x1 + w, y1 + h};
    P D1 = {x1 - w, y1 + h};
    // P2 Corners: A2(LL), B2(RL), C2(RU), D2(LU)
    P A2 = {x2 - w, y2 - h};
    P B2 = {x2 + w, y2 - h};
    P C2 = {x2 + w, y2 + h};
    P D2 = {x2 - w, y2 + h};
    // --- 2. Обработка случая "Нет движения" ---
    if (fabs(dX) < epsilon && fabs(dY) < epsilon) { // Контур - это сам прямоугольник P1 (4 точки, CCW)
        contour_points[0] = create_p(A1.x, A1.y);
        contour_points[1] = create_p(D1.x, D1.y);
        contour_points[2] = create_p(C1.x, C1.y);
        contour_points[3] = create_p(B1.x, B1.y);
        ret = 4;
    }else if (fabs(dY) < epsilon){ // Строго горизонтальное движение (Left/Right)     // --- 3. Обработка 4-точечных случаев (строго Horizontal/Vertical) ---
        if (dX > 0) { // Движение ВПРАВО (P1_left -> P2_right)
            contour_points[0] = create_p(A1.x, A1.y);
            contour_points[1] = create_p(B2.x, B2.y);
            contour_points[2] = create_p(C2.x, C2.y);
            contour_points[3] = create_p(D1.x, D1.y);
        }else{ // Движение ВЛЕВО (P1_right -> P2_left)
            contour_points[0] = create_p(A2.x, A2.y);
            contour_points[1] = create_p(B1.x, B1.y);
            contour_points[2] = create_p(C1.x, C1.y);
            contour_points[3] = create_p(D2.x, D2.y);
        }
        ret = 4;
    }else if (fabs(dX) < epsilon){ // Строго вертикальное движение (Up/Down)
        if (dY > 0) { // Движение ВВЕРХ (P1_bottom -> P2_top)
            contour_points[0] = create_p(A1.x, A1.y);
            contour_points[1] = create_p(B1.x, B1.y);
            contour_points[2] = create_p(C2.x, C2.y);
            contour_points[3] = create_p(D2.x, D2.y);

        } else { // Движение ВНИЗ (P1_top -> P2_bottom)
            contour_points[0] = create_p(C1.x, C1.y);
            contour_points[1] = create_p(D1.x, D1.y);
            contour_points[2] = create_p(A2.x, A2.y);
            contour_points[3] = create_p(B2.x, B2.y);
        }
        ret = 4;
    }else{
        // --- 4. Обработка 6-точечных случаев (Диагональное движение) ---
        // Порядок основан на CCW обходе внешней оболочки (Convex Hull)
        if (dX > 0 && dY > 0){ // I квадрант: Вправо-Вверх
            // Исключаются: B1 (правый нижний P1) и D2 (левый верхний P2)
            contour_points[0] = create_p(A1.x, A1.y); // LL P1
            contour_points[1] = create_p(B1.x, B1.y); // LU P1
            contour_points[2] = create_p(B2.x, B2.y); // RU P1 (промежуточная)
            contour_points[3] = create_p(C2.x, C2.y); // RU P2
            contour_points[4] = create_p(D2.x, D2.y); // RL P2
            contour_points[5] = create_p(D1.x, D1.y); // LL P2 (промежуточная)
        }else if(dX < 0 && dY > 0){ // II квадрант: Влево-Вверх
            // Исключаются: C1 (правый верхний P1) и A2 (левый нижний P2)
            contour_points[0] = create_p(A1.x, A1.y); // RL P1
            contour_points[1] = create_p(B1.x, B1.y); // LL P1
            contour_points[2] = create_p(C1.x, C1.y); // LU P1
            contour_points[3] = create_p(C2.x, C2.y); // LU P2
            contour_points[4] = create_p(D2.x, D2.y); // RU P2 (промежуточная)
            contour_points[5] = create_p(A2.x, A2.y); // RL P2 (промежуточная)
        }else if(dX < 0 && dY < 0){ // III квадрант: Влево-Вниз
            // Исключаются: D1 (левый верхний P1) и B2 (правый нижний P2)
            contour_points[0] = create_p(A2.x, A2.y); // RU P1
            contour_points[1] = create_p(B2.x, B2.y); // RL P1
            contour_points[2] = create_p(B1.x, B1.y); // LL P1
            contour_points[3] = create_p(C1.x, C1.y); // LL P2
            contour_points[4] = create_p(D1.x, D1.y); // LU P2 (промежуточная)
            contour_points[5] = create_p(D2.x, D2.y); // RU P2 (промежуточная)
        }else{ // dX > 0 && dY < 0 (IV квадрант: Вправо-Вниз)
            // Исключаются: A1 (левый нижний P1) и C2 (правый верхний P2)
            contour_points[0] = create_p(A1.x, A1.y); // LU P1
            contour_points[1] = create_p(A2.x, A2.y); // RU P1
            contour_points[2] = create_p(B2.x, B2.y); // RL P1
            contour_points[3] = create_p(C2.x, C2.y); // RL P2
            contour_points[4] = create_p(C1.x, C1.y); // LL P2
            contour_points[5] = create_p(D1.x, D1.y); // LU P2
        }
        ret = 6;
    }
    return ret;
}

/*
* Рисование контура оставляемого прямоугольным инструментом (высоты H, ширины W) при движении из x1, y1 в x2, y2. Контуру задаётся направление _dir
*/
Cont_t* ra_line( double x1, double y1, double x2, double y2, double W, double H, int _dir ){
	int dir = (_dir==0)?-1:_dir;
	Point_t* result[MAX_VERTICES];
	int count = calc_points(x1, y1, x2, y2, H, W, result);
	Cont_t* cont = create_cont();
	int i;
	for( i = 0; i<(count - 1); i++){
		Line_t* l = create_line( result[i], result[i+1] );
		add_item2cont( (Refitem_t*) l, cont);
	}
	add_item2cont( (Refitem_t*) create_line( result[i], result[0] ), cont);
//	printf( "DIR = %i %s\n", dir, (dir==1)?"CCW":"CW" );
	cont_reorder( cont, dir );
	return cont;
}

/*
* Обрисовка контура круглым инструментом
*/
void outline_milling(Cont_t* cont, Context_t* ctx_dst, double r){
	Context_t* ctx = get_context();
	Context_t* ctx_tmp = NULL;
	if( cont ){
		ctx_tmp = create_context("milling_tmp");
		select_context("milling_tmp");
//		copy_ctx2ctx( ctx->name, ctx_tmp->name, cont );
		if( cont->links.arr ){
			for( int i=0; i < cont->links.count; i++ ){
				Refitem_t* item = cont->links.arr[i];
				if( item ){
					Cont_t* new_cont1 = NULL;
					Cont_t* new_cont2 = NULL;
					Arc_t* arc = NULL;
					Line_t* line = NULL;
					if(item->type == OBJ_TYPE_LINE){
						line = (Line_t *) item;
						//printf("Отрезок!\n");
						line_milling( line->a->x, line->a->y, line->b->x, line->b->y, r, -1, &new_cont1 );
					}else if(item->type == OBJ_TYPE_ARC){
						arc = (Arc_t *) item;
						//printf("Дуга! Dir = %i\n", arc->dir);
						arc_milling( arc, r, -1, &new_cont1, &new_cont2 );
					}
					if( new_cont2 ){
						fix_single_arc_cont( new_cont2 );
					}
					if( new_cont1 ){
						fix_single_arc_cont( new_cont1 );
						split_all(0);
						//walk_around_all_points();
						calc_contcount4all(new_cont1->num, 0);
						marking_of_imposed(); // Маркируем совпадающие грани контуров
						clean_all_cont();
						find_all_conts();
						//walk_around_all_cont("svg", 10);
					}
				}
			}
		}
		//printf("\n");
		//walk_around_all_cont("svg", 10);
		// Копируем контуры, совпадающие с исходным по напрвлению в целевой контекст
		for( int i=0; i < ctx_tmp->links.count; i++){
			Refitem_t* item = (Refitem_t*) ctx_tmp->links.arr[i];
			if( item->type == OBJ_TYPE_CONTUR ){
				Cont_t* next = (Cont_t*) item;
				if( cont->dir == next->dir ){
					//printf("копирую\n");
					copy_ctx2ctx( ctx_tmp->name, ctx_dst->name, next );
				}
			}
		}
		select_context( ctx->name );			// переключаемся на исходный контекст
		free_context_by_name(ctx_tmp->name);	// удаляем временный контекст
	}
	return;
}
