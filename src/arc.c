#include <math.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "../include/top.h"
#include "../include/reflist.h"
#include "../include/context.h"
#include "../include/point.h"
#include "../include/cont.h"
#include "../include/line.h"
#include "../include/geom.h"
#include "../include/arc.h"


uint8_t is_arc(Refitem_t* item){
	return (item->type == OBJ_TYPE_ARC);
}


// вызывается перед разрывом связи с другими объектами. Если эта функция вернёт 0, то связь не будет разорвана
int arc_onunlink( Refitem_t* self, Refitem_t* ref ){
	Arc_t* this = (Arc_t*) self;
	if( this->a && (Refitem_t*) this->a == ref ) return 0;
	if( this->b && (Refitem_t*) this->b == ref ) return 0;
	return 1; // можно удалять точки
}

/*
* Создание дуги
*/
Arc_t* create_arc( double x, double y, double R ){
	int n = sizeof( Arc_t );
	Arc_t* arc = malloc( n );
	memset( arc, 0x00, n );
	arc->type = OBJ_TYPE_ARC;
	arc->onunlink = arc_onunlink;
	arc->R = R;
	arc->a = NULL;
	arc->b = NULL;
	arc->id = prim_counter++;
	Context_t* ctx = get_context();
	linkobj2obj(ctx, arc);
	Point_t* p = create_p(x, y);
	unlinkobj2obj(ctx, p);
	arc->center = *p;
	purge_obj( (Refitem_t**) &p);
	return arc;
}

/*
* удаление дуги (отвязка примитива от всех точек)
*/
uint8_t remove_arc( Arc_t** ptr ){
	Arc_t* arc = *ptr;
	if( arc ){
		arc->a = NULL;
		arc->b = NULL;
		Refholder_t* list = unlinkyouself( (Refitem_t*) arc );
		purge_by_list( list, (Refitem_t**) &arc );
		if( arc->links.count != 0 ) return 0;
		if( arc->links.arr ){
			free( arc->links.arr );
			arc->links.arr = NULL;
		}
		if(arc->center.cont) arc->center.cont = NULL;
		free(arc);
		arc = NULL;
	}
	*ptr = NULL;
	return 1;
}

/*
* arc - параметры окружности
* a, b, c  -  точки лежащие на окружности.
* дуга будет строиться из точки а  в точку b в обход точки c.
* то есть, для построения дуги будет вычислено, как нужно двигаться по окружности из A в B (по часовой или против) чтобы НЕ пройти через C
*/
uint8_t arc_dir( Arc_t* arc, Point_t* a, Point_t* b, Point_t* c ){
	double angleA = atan2(a->y - arc->center.y, a->x - arc->center.x);
	double angleB = atan2(b->y - arc->center.y, b->x - arc->center.x);
	double angleC = atan2(c->y - arc->center.y, c->x - arc->center.x);
	angleA = normalize_angle(angleA);
	angleB = normalize_angle(angleB);
	angleC = normalize_angle(angleC);
	int c_on_ccw_arc = 0;
	if (fabs(angleA - angleB) < epsilon || fabs(angleA - angleC) < epsilon || fabs(angleB - angleC) < epsilon) {
		//printf("Точки совпадают, невозможно определить направление.\n");
		//return 0;
	}else{
		if (angleA < angleB) {
			//c_on_ccw_arc = (angleC > angleA && angleC < angleB)?1:2;
			c_on_ccw_arc = (angleC > angleA && angleC < angleB)?-1:1;
		} else {
			//c_on_ccw_arc = (angleC > angleA || angleC < angleB)?1:2;
			c_on_ccw_arc = (angleC > angleA || angleC < angleB)?-1:1;
		}
	}
	return  c_on_ccw_arc;
}

void replace_arc_a( Arc_t* arc, Point_t* p){
	if( arc->a != p ){
		Point_t* old = arc->a;
		cp_links( (Refitem_t*) old, (Refitem_t*) p);
		arc->a = NULL;
		remove_p( &old );
		arc->a = p;
		linkobj2obj( arc, arc->a );
	}
}

void replace_arc_b( Arc_t* arc, Point_t* p){
	if( arc->b != p ){
		Point_t* old = arc->b;
		cp_links( (Refitem_t*) old, (Refitem_t*) p);
		arc->b = NULL;
		remove_p( &old );
		arc->b = p;
		linkobj2obj( arc, arc->b );
	}
}



/*
* Проверка принадлежности точки P дуге с концами AB, центром C, радиуса R и направлением обхода dir от A к B
*/
uint8_t is_xy_on_arc( Arc_t* arc, double x, double y){
	if( arc==NULL ){ return 0; }
	if( !is_point_on_circle( arc->center.x, arc->center.y, arc->R, x, y ) ) return 0;
	if( (arc->dir == 0) || (arc->a == arc->b) || ( p_eq_p(arc->a, arc->b) && p_eq_p(arc->a, arc->b)) ) return 1;
	return is_p_on_arc_geom( arc->center.x,  arc->center.y,  arc->R,  arc->a->x, arc->a->y, arc->b->x, arc->b->y, arc->dir, x, y);
}

/*
* Проверка принадлежности точки P дуге с концами AB, центром C, радиуса R и направлением обхода dir от A к B
*/
uint8_t is_p_on_arc( Arc_t* arc, Point_t* p){
	if( p ){
		return is_xy_on_arc( arc, p->x, p->y);
	}else{
		return 0;
	}
}

/*
* Проверка принадлежности точки P двум дугам arc1 b arc2
*/
uint8_t is_xy_on_arcs( Arc_t* arc1, Arc_t* arc2, double x, double y){
	return is_xy_on_arc( arc1, x, y) && is_xy_on_arc( arc2, x, y);
}

/*
* Разбить разорвать окружность точкой (превратить в дугу)
*/
uint8_t break_the_circle(Arc_t* arc, Point_t* p, int dir){
	if( (arc->dir) == 0 && is_point_on_circle( arc->center.x, arc->center.y, arc->R, p->x, p->y)){
		arc->dir = dir;
		linkobj2obj(arc, p);
		arc->a = p;
		arc->b = p;
		return 1;
	}else{
		return 0;
	}
}

/*
* Разбить дугу, на две части точкой.
*/
Arc_t* split_arc_by_p(Arc_t* arc, Point_t* p){
	if( arc->dir == 0 ) return NULL; // Нельзя разбивать окружность, вначале надо использовать break_the_circle и определить направление dir
	if( (is_p_on_arc(arc, p)) && (arc->a) && (arc->b) ){ // разбиение дуги на две части по точке p
		Arc_t* new_arc = create_arc( arc->center.x, arc->center.y, arc->R );
		new_arc->dir = arc->dir;
		Point_t* last_point = arc->b;
		arc->b = p;
		linkobj2obj( arc, p );
		if( arc->b != arc->b ) unlinkobj2obj( arc, last_point );
		linkobj2obj( new_arc, p );
		new_arc->a = p;
		linkobj2obj( new_arc, last_point );
		new_arc->b = last_point;
		if( arc->cont ){
			add_item2cont( (Refitem_t*) new_arc, arc->cont );
			put_after( (Refitem_t*) new_arc, arc->cont, (Refitem_t*) arc );
		}
		if( arc->links.count > 0 ){
			for( int i = arc->links.count-1; i>=0 ; i--){                          // перенос точек
				if( ((Refitem_t*) arc->links.arr[i])->type == OBJ_TYPE_POINT ){
					Point_t* pnt = arc->links.arr[i];
					if( (arc->a != pnt) && (arc->b != pnt) ){
						if( !is_p_on_arc(arc, pnt) ){ // если координаты точки больше не принадлежит дуге то переносим точку на новую дугу
							unlinkobj2obj(arc, pnt);
							linkobj2obj(new_arc, pnt);
						}
					}
				}
			}
		}
		return new_arc;
	}else{
		return NULL;
	}
}

/*
* вычисление экстремальных координат всех точек дуги.
*/
void arc_get_bounds(const Arc_t* arc, double* xmin, double* xmax, double* ymin, double* ymax){
	*xmin = *ymin = -1000000;
	*xmax = *ymax =  1000000;
	if(arc){
		*xmin = arc->center.x - arc->R;
		*xmax = arc->center.x + arc->R;
		*ymin = arc->center.y - arc->R;
		*ymax = arc->center.y + arc->R;
		if( (arc->dir == 0) || (arc->a == arc->b) || (( arc->a->x == arc->b->x) && ( arc->a->y == arc->b->y)) ){
			return;
		}else{
			if( !is_p_on_arc_geom( arc->center.x, arc->center.y, arc->R, arc->a->x, arc->a->y, arc->b->x, arc->b->y, arc->dir, arc->center.x - arc->R, arc->center.y ) ) *xmin = fminf( arc->a->x, arc->b->x );
			if( !is_p_on_arc_geom( arc->center.x, arc->center.y, arc->R, arc->a->x, arc->a->y, arc->b->x, arc->b->y, arc->dir, arc->center.x + arc->R, arc->center.y ) ) *xmax = fmaxf( arc->a->x, arc->b->x );
			if( !is_p_on_arc_geom( arc->center.x, arc->center.y, arc->R, arc->a->x, arc->a->y, arc->b->x, arc->b->y, arc->dir, arc->center.x, arc->center.y - arc->R ) ) *ymin = fminf( arc->a->y, arc->b->y );
			if( !is_p_on_arc_geom( arc->center.x, arc->center.y, arc->R, arc->a->x, arc->a->y, arc->b->x, arc->b->y, arc->dir, arc->center.x, arc->center.y + arc->R ) ) *ymax = fmaxf( arc->a->y, arc->b->y );
		}
	}else{
		return;
	}
}


// Возвращает количество пересечений окружностей и заполняет x1,y1 и x2,y2  координатами этих пересечений
// Если количество пересечений >2 - дуги лежат на одной окружности, надо проверять концы
uint8_t xy_of_arc_x_arc( Arc_t* c1, Arc_t* c2, double* x1, double* y1, double* x2, double* y2 ){
	*x1 = *x2 = *y1 = *y2 = 0;
	double d = points_distance( &c1->center, &c2->center );
	double dx = c2->center.x - c1->center.x;
	double dy = c2->center.y - c1->center.y;
	if (d > c1->R + c2->R) return 0;						  // пересечение окружностей = 0
	if (d < fabs(c1->R - c2->R)) return 0;					  // пересечение окружностей = 0
	double a = (c1->R * c1->R - c2->R * c2->R + d * d) / (2 * d);
	double h = sqrt(c1->R * c1->R - a * a);
	double p2x = c1->center.x + (a * dx) / d;
	double p2y = c1->center.y + (a * dy) / d;
	if( d < epsilon ){
		if( c1->R == c2->R ){
			return 3; // окружности совпадают
		}else{
			return 0;
		}
	};
	if( (d == c1->R + c2->R) || (d == fabs(c1->R - c2->R)) ){ // окружности соприкасаются
		*x1 = p2x;
		*y1 = p2y;
		return 1;
	}
	// окружности пересекаются в двух точках
	*x1 = p2x + (h * dy) / d;
	*y1 = p2y - (h * dx) / d;
	*x2 = p2x - (h * dy) / d;
	*y2 = p2y + (h * dx) / d;
	return 2;
}

uint8_t is_real_arc(Arc_t* arc){
	//if( arc->dir>0 ){
	if( arc->dir!=0 ){
		if( (arc->a != NULL) &&  (arc->b != NULL) ) return 1;
	}
	return 0;
}


/*
* пересечение дуги с прямой
* Если p1 == NULL - точек пересечения нет
* Если p1 != NULL - точки пересечения есть
* Если p2 == NULL - Только одна точка пересечения (и это точка p1) и это именно пересечение, а не касание
* Если p2 != NULL - То то надо проверить дополнительно:
* Если p2 == p1   - Есть только одна точка, и это точка касания дуг.
* Если p2 != p1   - Есть две точки пересечения
*/
void create_p_of_line_x_arc(Line_t* l, Arc_t* crcl, Point_t** p1, Point_t** p2){
	if( l && l->a && l->b ){
		double p1x;
		double p1y;
		double p2x;
		double p2y;
		uint8_t ret = xy_of_line_x_arc( l->a->x, l->a->y, l->b->x, l->b->y, crcl, &p1x, &p1y, &p2x, &p2y );
		*p1 = NULL;
		*p2 = NULL;
		// printf("Точек пересечения %i \n", ret);
		if( ret == 1 ){ //  1 пересечение
			if( xy_eq_xy( l->a->x, l->a->y, p1x, p1y ) ){ *p1 = l->a; } //  printf("Конец A линии\n");
			if( xy_eq_xy( l->b->x, l->b->y, p1x, p1y ) ){ *p1 = l->b; } // printf("Конец B линии\n");
			if( is_real_arc(crcl) &&  xy_eq_xy( crcl->a->x, crcl->a->y, p1x, p1y ) ){ *p1 = crcl->a; }// printf("Конец A дуги\n");
			if( is_real_arc(crcl) && xy_eq_xy( crcl->b->x, crcl->b->y, p1x, p1y ) ){ *p1 = crcl->b; }// printf("Конец B дуги\n");
			if( !(*p1) ){ *p1 = create_p( p1x, p1y ); }//  printf("Новая точка p1\n");
		}else if( ret == 2 ){  // 1 - одно касание
			if( xy_eq_xy(l->a->x, l->a->y, p1x, p1y ) ){ *p1 = l->a; } // printf("Конец A линии\n");
			if( xy_eq_xy(l->b->x, l->b->y, p1x, p1y ) ){ *p1 = l->b; } // printf("Конец B линии\n");
			if( is_real_arc(crcl) &&  xy_eq_xy(crcl->a->x, crcl->a->y, p1x, p1y ) ){ *p1 = crcl->a; } // printf("Конец A дуги\n");
			if( is_real_arc(crcl) &&  xy_eq_xy(crcl->b->x, crcl->b->y, p1x, p1y ) ){ *p1 = crcl->b; } // printf("Конец B дуги\n");
			if( !(*p1) ){ *p1 = create_p( p1x, p1y ); } // printf("Новая точка p1\n");
			*p2 = *p1;
		}else if( ret == 3 ){
			if( xy_eq_xy(l->a->x, l->a->y, p1x, p1y ) ){ *p1 = l->a; }// printf("Конец A линии\n");
			if( xy_eq_xy(l->b->x, l->b->y, p1x, p1y ) ){ *p1 = l->b; }// printf("Конец B линии\n");
			if( is_real_arc(crcl) &&  xy_eq_xy(crcl->a->x, crcl->a->y, p1x, p1y ) ){ *p1 = crcl->a; }// printf("Конец A дуги\n");
			if( is_real_arc(crcl) &&  xy_eq_xy(crcl->b->x, crcl->b->y, p1x, p1y ) ){ *p1 = crcl->b; }// printf("Конец B дуги\n");
			if( !(*p1) ){ *p1 = create_p( p1x, p1y ); }// printf("Новая точка p1\n");
			//*p1 = create_p( p1x, p1y );
			if( xy_eq_xy(l->a->x, l->a->y, p2x, p2y ) ){ *p2 = l->a;}// printf("Конец A линии\n");
			if( xy_eq_xy(l->b->x, l->b->y, p2x, p2y ) ){ *p2 = l->b;}// printf("Конец B линии\n");
			if( is_real_arc(crcl) &&  xy_eq_xy(crcl->a->x, crcl->a->y, p2x, p2y ) ){ *p2 = crcl->a; }// printf("Конец A дуги\n");
			if( is_real_arc(crcl) &&  xy_eq_xy(crcl->b->x, crcl->b->y, p2x, p2y ) ){ *p2 = crcl->b; }// printf("Конец B дуги\n");
			if( !(*p2) ){ *p2 = create_p( p2x, p2y ); }// printf("Новая точка p2\n");
			//*p2 = create_p( p2x, p2y );
		}
		// printf("Точек пересечения ok \n");
	}
}

/*
uint8_t xy_of_line_x_arc(double ax, double ay, double bx, double by, Arc_t* crcl, double* p1x, double* p1y, double* p2x, double* p2y) {
    const double A = by - ay;
    const double B = ax - bx;
    const double C = -(A * ax + B * ay);
    const double cx = crcl->center.x, cy = crcl->center.y, R = crcl->R;
    // Координаты физических концов дуги из структуры
    const double arc_ax = crcl->a->x;
    const double arc_ay = crcl->a->y;
    const double arc_bx = crcl->b->x;
    const double arc_by = crcl->b->y;

    const double xmin = fmin(ax, bx), xmax = fmax(ax, bx);
    const double ymin = fmin(ay, by), ymax = fmax(ay, by);
    double xA = 0, yA = 0, xB = 0, yB = 0;
    uint8_t hasA = 0, hasB = 0, tangent = 0;

    // ---------------- 1. Поиск точек пересечения прямой и окружности ----------------
    if( fabs(B) < epsilon ){ // Вертикальная прямая
        double x = ax;
        double dx = x - cx;
        double det = R * R - dx * dx;
        if(det < -epsilon) return 0;
        if(fabs(det) < epsilon) tangent = 1;
        double sqrt_det = sqrt(fmax(0.0, det));
        double y1 = cy + sqrt_det;
        double y2 = cy - sqrt_det;
        if( (y1 >= ymin - epsilon) && (y1 <= ymax + epsilon) && is_xy_on_arc(crcl, x, y1) ){
            xA = x; yA = y1; hasA = 1;
        }
        if( !tangent && (y2 >= ymin - epsilon) && (y2 <= ymax + epsilon) && is_xy_on_arc(crcl, x, y2) ){
            xB = x; yB = y2; hasB = 1;
        }
    } else if( fabs(A) < epsilon ){ // Горизонтальная прямая
        double y = ay;
        double dy = y - cy;
        double det = R * R - dy * dy;
        if (det < -epsilon) return 0;
        if (fabs(det) < epsilon) tangent = 1;
        double sqrt_det = sqrt(fmax(0.0, det));
        double x1 = cx + sqrt_det;
        double x2 = cx - sqrt_det;
        if (x1 >= xmin - epsilon && x1 <= xmax + epsilon && is_xy_on_arc(crcl, x1, y)) {
            xA = x1; yA = y; hasA = 1;
        }
        if( !tangent && x2 >= xmin - epsilon && x2 <= xmax + epsilon && is_xy_on_arc(crcl, x2, y) ){
            xB = x2; yB = y; hasB = 1;
        }
    } else { // Общий случай
        double k = -A / B;
        double b = -C / B;
        double ka = 1 + k * k;
        double kb = 2 * (k * (b - cy) - cx);
        double kc = cx * cx + (b - cy) * (b - cy) - R * R;
        double D = kb * kb - 4 * ka * kc;
        if (D < -epsilon) return 0;
        if (fabs(D) < epsilon) tangent = 1;
        double sqrtD = sqrt(fmax(0.0, D));
        double x1 = (-kb + sqrtD) / (2 * ka);
        double x2 = (-kb - sqrtD) / (2 * ka);
        double y1 = k * x1 + b;
        double y2 = k * x2 + b;
        if( x1 >= xmin - epsilon && x1 <= xmax + epsilon && y1 >= ymin - epsilon && y1 <= ymax + epsilon && is_xy_on_arc(crcl, x1, y1) ){
            xA = x1; yA = y1; hasA = 1;
        }
        if( !tangent && x2 >= xmin - epsilon && x2 <= xmax + epsilon && y2 >= ymin - epsilon && y2 <= ymax + epsilon && is_xy_on_arc(crcl, x2, y2) ){
            xB = x2; yB = y2; hasB = 1;
        }
    }

    // ---------------- 2. Коррекция точек (Snap to Ends) ----------------
    // Если расчетная точка попала в epsilon-окрестность конца дуги, 
    // мы заменяем её точными координатами этого конца.

    if (hasA) {
        if (is_point_in_ray_neighborhood(ax, ay, bx, by, arc_ax, arc_ay)) {
            xA = arc_ax; yA = arc_ay;
        } else if (is_point_in_ray_neighborhood(ax, ay, bx, by, arc_bx, arc_by)) {
            xA = arc_bx; yA = arc_by;
        }
    }
    if (hasB) {
        if (is_point_in_ray_neighborhood(ax, ay, bx, by, arc_ax, arc_ay)) {
            xB = arc_ax; yB = arc_ay;
        } else if (is_point_in_ray_neighborhood(ax, ay, bx, by, arc_bx, arc_by)) {
            xB = arc_bx; yB = arc_by;
        }
    }

    // ---------------- 3. Формирование результата ----------------
    uint8_t ret = 0;
    if( tangent ){
        if( hasA ){ *p2x = *p1x = xA; *p2y = *p1y = yA; ret = 2; }
        else if( hasB ){ *p2x = *p1x = xB; *p2y = *p1y = yB; ret = 2; }
    } else {
        if( hasA && hasB ){
            *p1x = xA; *p1y = yA; *p2x = xB; *p2y = yB; ret = 3;
        } else {
            if( hasA ){ *p1x = xA; *p1y = yA; ret = 1; }
            if( hasB ){ *p1x = xB; *p1y = yB; ret = 1; }
        }
    }
    return ret;
}
*/


/*
uint8_t xy_of_line_x_arc(double ax, double ay, double bx, double by, Arc_t* crcl, double* p1x, double* p1y, double* p2x, double* p2y ){
    uint8_t nearA = is_point_in_ray_neighborhood(ax, ay, bx, by, crcl->a->x, crcl->a->y);
    uint8_t nearB = is_point_in_ray_neighborhood(ax, ay, bx, by, crcl->b->x, crcl->b->y);

    if (nearA && nearB) {
        *p1x = crcl->a->x; *p1y = crcl->a->y;
        *p2x = crcl->b->x; *p2y = crcl->b->y;
        return 3; // Две точки (совпадающие с узлами)
    }

	const double A = by - ay;
	const double B = ax - bx;
	const double C = -(A * ax + B * ay);
	const double cx = crcl->center.x, cy = crcl->center.y, R = crcl->R;
	const double xmin = fmin(ax, bx), xmax = fmax(ax, bx);
	const double ymin = fmin(ay, by), ymax = fmax(ay, by);
	double xA = 0, yA = 0, xB = 0, yB = 0;
	uint8_t hasA = 0, hasB = 0, tangent = 0;
	if( fabs(B) < epsilon ){ // ---------------- Вертикальная прямая ----------------
		double x = ax;
		double dx = x - cx;
		double det = R * R - dx * dx;
		if(det < -epsilon) return 0;
		if(fabs(det) < epsilon) tangent = 1;
		double sqrt_det = sqrt(fmax(0.0, det));
		double y1 = cy + sqrt_det;
		double y2 = cy - sqrt_det;
		if( (y1 >= ymin) && (y1 <= ymax) && is_xy_on_arc(crcl, x, y1) ){
			xA = x; yA = y1; hasA = 1;
		}
		if( !tangent && (y2 >= ymin) && (y2 <= ymax) && is_xy_on_arc(crcl, x, y2) ){
			xB = x; yB = y2; hasB = 1;
		}
	}else if( fabs(A) < epsilon ){ // ---------------- Горизонтальная прямая ----------------
		double y = ay;
		double dy = y - cy;
		double det = R * R - dy * dy;
		if (det < -epsilon) return 0;
		if (fabs(det) < epsilon) tangent = 1;
		double sqrt_det = sqrt(fmax(0.0, det));
		double x1 = cx + sqrt_det;
		double x2 = cx - sqrt_det;
		if (x1 >= xmin && x1 <= xmax && is_xy_on_arc(crcl, x1, y)) {
			xA = x1; yA = y; hasA = 1;
		}
		if( !tangent && x2 >= xmin && x2 <= xmax && is_xy_on_arc(crcl, x2, y) ){
			xB = x2; yB = y; hasB = 1;
		}
	}else{ // ---------------- Общий случай ----------------
		double k = -A / B;
		double b = -C / B;
		double ka = 1 + k * k;
		double kb = 2 * (k * (b - cy) - cx);
		double kc = cx * cx + (b - cy) * (b - cy) - R * R;
		double D = kb * kb - 4 * ka * kc;
		if (D < -epsilon) return 0;
		if (fabs(D) < epsilon) tangent = 1;
		double sqrtD = sqrt(fmax(0.0, D));
		double x1 = (-kb + sqrtD) / (2 * ka);
		double x2 = (-kb - sqrtD) / (2 * ka);
		double y1 = k * x1 + b;
		double y2 = k * x2 + b;
		//printf("x1: %f, y1: %f, x2: %f, y2: %f p1  on arc %i |  p2  on arc %i\n", x1, y1, x2, y2, is_xy_on_arc(crcl, x1, y1), is_xy_on_arc(crcl, x2, y2) );
		if( x1 >= xmin && x1 <= xmax && y1 >= ymin && y1 <= ymax && is_xy_on_arc(crcl, x1, y1) ){
			xA = x1; yA = y1; hasA = 1;
		}
		if( !tangent && x2 >= xmin && x2 <= xmax && y2 >= ymin && y2 <= ymax && is_xy_on_arc(crcl, x2, y2) ){
			xB = x2; yB = y2; hasB = 1;
		}
	}
	// ---------------- Создание точек ----------------

    if (!hasA && !hasB) {
        if (nearA) { *p1x = crcl->a->x; *p1y = crcl->a->y; return 1; }
        if (nearB) { *p1x = crcl->b->x; *p1y = crcl->b->y; return 1; }
        return 0;
    }


    // Если аналитика нашла точки, корректируем их по узлам
    if(hasA && (crcl->dir!=0) ){
        if (nearA && IS_NEAR(xA, yA, crcl->a->x, crcl->a->y)) { xA = crcl->a->x; yA = crcl->a->y; }
        else if (nearB && IS_NEAR(xA, yA, crcl->b->x, crcl->b->y)) { xA = crcl->b->x; yA = crcl->b->y; }
    }
    if(hasB && (crcl->dir!=0) ){
        if (nearA && IS_NEAR(xB, yB, crcl->a->x, crcl->a->y)) { xB = crcl->a->x; yB = crcl->a->y; }
        else if (nearB && IS_NEAR(xB, yB, crcl->b->x, crcl->b->y)) { xB = crcl->b->x; yB = crcl->b->y; }
    }

	uint8_t ret = 0;
	if( tangent ){ // касание — одна точка
		if( hasA ){
			*p2x = *p1x = xA;
			*p2y = *p1y = yA;
			ret = 2;
		}
		if( hasB ){
			*p2x = *p1x = xB;
			*p2y = *p1y = yB;
			ret = 2;
		}
	}else{
		if( hasA && hasB ){
			*p1x = xA;
			*p1y = yA;
			*p2x = xB;
			*p2y = yB;
			ret = 3;
		}else{
			if( hasA ){
				*p1x = xA;
				*p1y = yA;
				ret = 1;
			}
			if( hasB ){
				*p1x = xB;
				*p1y = yB;
				ret = 1;
			}
		}
	}
	return ret;
}
*/

/*
* вычисление точки пересечения окружностей
* логика заполнения точек такова:
*	if( p1 ){
*		if( p2 ){
*			if( p1 == p2 ){
*				// одно касание (в т. p1)
*			}else{
*				// два пересечения (в тт. p1, p2)
*			}
*		}else{
*			// одно пересечение (в т. p1)
*		}
*	}else{
*		// нет пересечений
*	}
*/
uint8_t create_p_of_arc_x_arc2(Arc_t* c1, Arc_t* c2, Point_t** p1, Point_t** p2 ){
	double d = points_distance( &c1->center, &c2->center );
	double dx = c2->center.x - c1->center.x;
	double dy = c2->center.y - c1->center.y;
	*p1 = NULL;
	*p2 = NULL;
	if (d > c1->R + c2->R) return 0;
	if (d < fabs(c1->R - c2->R)) return 0;
	if (d == 0 && (c1->R == c2->R) )return 0;
	double a = (c1->R * c1->R - c2->R * c2->R + d * d) / (2 * d);
	double h = sqrt(c1->R * c1->R - a * a);
	double p2x = c1->center.x + (a * dx) / d;
	double p2y = c1->center.y + (a * dy) / d;
	if( (d == c1->R + c2->R) || (d == fabs(c1->R - c2->R)) ){ // окружности соприкасаются
		if( !is_xy_on_arcs(c1, c2, p2x, p2y ) ) return 0;     // Точка должна попадать на обе дуги
		*p2 = *p1 = create_p(p2x, p2y);                       // только одна точка
		return 1;
	}
	// Первая точка пересечения
	if( is_xy_on_arcs(c1, c2, p2x + (h * dy) / d, p2y - (h * dx) / d ) ){
		*p1 = create_p(p2x + (h * dy) / d, p2y - (h * dx) / d);
	};
	// Вторая точка пересечения
	if( is_xy_on_arcs(c1, c2, p2x - (h * dy) / d, p2y + (h * dx) / d ) ){
		*p2 = create_p(p2x - (h * dy) / d, p2y + (h * dx) / d);
	}
	// Если дуги пересекаются только в одной точке, то это точка p1
	if( !*p1 && *p2 ){
		*p1 = *p2;
		*p2 = NULL;
	}
	return 2;
}


/*
* Возвращаемое значение отражает отношение точек с дугами, то есть попадают ли точки в середину дуг ( не совпадают ни с одим из концов)
* Возвращает набор точек (0-2) которыми может быть разбита дуга arc2 при пересечении или наложении c дугой arc1
* таким образом, полный набор точек пересечения дуг можно получить только при двукратном вызове функции
* create_p_of_arc_x_arc( arcA, arcB, &p1, &p2 )
* create_p_of_arc_x_arc( arcB, arcA, &p1, &p2 )
* хотя не коллиниарное расположение дуг будет возвращать одни и те же результаты при любом из вызовов.
*/
uint8_t create_p_of_arc_x_arc(Arc_t* arc1, Arc_t* arc2, Point_t** p1, Point_t** p2 ){
	double x1;
	double y1;
	double x2;
	double y2;
	*p1 = NULL;
	*p2 = NULL;
	uint8_t n = xy_of_arc_x_arc( arc1,  arc2, &x1,  &y1,  &x2,  &y2 );
	//printf(" n = %i\n", n);
	uint8_t ret = 0;
	if( n == 0 ) return 0;
	uint8_t p1arc1a = (arc1->dir!=0)?xy_eq_xy( arc1->a->x, arc1->a->y, x1, y1 ):0;
	uint8_t p1arc1b = (arc1->dir!=0)?xy_eq_xy( arc1->b->x, arc1->b->y, x1, y1 ):0;
	uint8_t p1arc2a = (arc2->dir!=0)?xy_eq_xy( arc2->a->x, arc2->a->y, x1, y1 ):0;
	uint8_t p1arc2b = (arc2->dir!=0)?xy_eq_xy( arc2->b->x, arc2->b->y, x1, y1 ):0;
	uint8_t p1on_arc1 = 0;
	uint8_t p1on_arc2 = 0;
	if( !p1arc1a && !p1arc1b ) p1on_arc1 = is_xy_on_arc(arc1, x1, y1);
	if( !p1arc2a && !p1arc2b ) p1on_arc2 = is_xy_on_arc(arc2, x1, y1);

	if( (n == 1) || (n == 2) ){
		if( p1arc1a || p1arc1b) ret = ret | (12 << 4); // 1100
		if( p1arc2a || p1arc2b) ret = ret | (3  << 4); // 0011
		if( p1on_arc1 ) ret = ret | (8 << 4);          // 1000
		if( p1on_arc2 ) ret = ret | (2 << 4);          // 0010
		if( p1arc1a ) *p1 = arc1->a;
		if( p1arc1b ) *p1 = arc1->b;
		if( p1arc2a ) *p1 = arc2->a;
		if( p1arc2b ) *p1 = arc2->b;
		if( (!*p1) && p1on_arc1 && p1on_arc2 ){
			*p1 = create_p( x1, y1 ); //  p1
//			printf("create p1  (%f %f)\n", x1, y1);
			ret = ret | (10 << 4);                     // 1010 0000
		}
	}

//	printf(" ret = %i\n", ret);

	if(n == 2){
		// здесь две точки пересечения
		uint8_t p2arc1a = (arc1->dir!=0)?xy_eq_xy(arc1->a->x, arc1->a->y, x2, y2):0;
		uint8_t p2arc1b = (arc1->dir!=0)?xy_eq_xy(arc1->b->x, arc1->b->y, x2, y2):0;
		uint8_t p2arc2a = (arc2->dir!=0)?xy_eq_xy(arc2->a->x, arc2->a->y, x2, y2):0;
		uint8_t p2arc2b = (arc2->dir!=0)?xy_eq_xy(arc2->b->x, arc2->b->y, x2, y2):0;
		uint8_t p2on_arc1 = 0;
		uint8_t p2on_arc2 = 0;
		if( !p2arc1a && !p2arc1b ) p2on_arc1 = is_xy_on_arc(arc1, x2, y2);
		if( !p2arc2a && !p2arc2b ) p2on_arc2 = is_xy_on_arc(arc2, x2, y2);
		if( p2arc1a || p2arc1b) ret = ret | ( 12 ); // 1100
		if( p2arc2a || p2arc2b) ret = ret | ( 3 );  // 0011
		if( p2on_arc1 ) ret = ret | ( 8 );          // 1000
		if( p2on_arc2 ) ret = ret | ( 2 );          // 0010
		if( p2arc1a ) *p2 = arc1->a;
		if( p2arc1b ) *p2 = arc1->b;
		if( p2arc2a ) *p2 = arc2->a;
		if( p2arc2b ) *p2 = arc2->b;
		if( !(*p2) && p2on_arc1 && p2on_arc2 ){
			*p2 = create_p( x2, y2 ); //  p2
//			printf("create p2  (%f %f)\n", x2, y2);
			ret = ret | ( 10 );    // 1010 0000
		}
	}else if(n == 3){
		//printf("// здесь, всё на одной окружности\n");
		uint8_t arc1a_on_arc2 = 0;
		uint8_t arc1b_on_arc2 = 0;
		uint8_t arc2a_on_arc1 = 0;
		uint8_t arc2b_on_arc1 = 0;
		if( arc1->dir!=0 ){
			arc1a_on_arc2 = is_xy_on_arc(arc2, arc1->a->x, arc1->a->y);
			arc1b_on_arc2 = is_xy_on_arc(arc2, arc1->b->x, arc1->b->y);
		}
		if( arc2->dir!=0 ){
			arc2a_on_arc1 = is_xy_on_arc(arc1, arc2->a->x, arc2->a->y);
			arc2b_on_arc1 = is_xy_on_arc(arc1, arc2->b->x, arc2->b->y);
		}
		if( arc1a_on_arc2 || arc1b_on_arc2 || arc2a_on_arc1 || arc2b_on_arc1 ){
			if( arc1->dir!=0 ){
				if( arc2->dir!=0 ){
					// две дуги. Разбиваем arc2 концами arc1
					if( arc1b_on_arc2 ){
						*p1 = arc1->b;
//						printf("!p1: ");
//						print_p(*p1);
						ret = ret | ( 12 << 4 );       // 1100 0000
						ret = ret | ( 2 << 4 );        // 0010 0000
//						printf("[1] 1110 0000\n");
						if( p_eq_p( *p1, arc2->a) || p_eq_p( *p1, arc2->b )){
							ret = ret | ( 1 << 4 );       // 0001 0000
//							printf("[2] 0001 0000\n");
						}
					}
					if( arc1a_on_arc2 ){
						*p2 = arc1->a;
//						printf("!p2: ");
//						print_p(*p2);
						ret = ret | ( 12 );       // 1100
						ret = ret | ( 2 );        // 0010
//						printf("[5] 0000 1110\n");
						if( p_eq_p( *p2, arc2->a) || p_eq_p( *p2, arc2->b )){
							ret = ret | ( 1 );   // 0001
//							printf("[6] 0000 0001\n");
						}
					}
				}else{
					// arc2 - окружность. arc1 - дуга
					*p1 = arc1->a;
					*p2 = arc1->b;
					ret = ret | 238;  // 1110 1110  - точка 1 разбивает arc2, точка 2 разбивает arc2
				}
			}else{
				if( arc2->dir!=0 ){
					// arc1 - окружность. arc2 - дуга
					*p1 = arc2->a;
					*p2 = arc2->b;
					ret = ret | 187;  // 1011 1011  - точка 1 разбивает arc1, точка 2 разбивает arc1
				}else{
					// две одинаковые окружности
				}
			}
		}
	}
	return ret;
}


uint8_t xy_of_line_x_arc(double rx, double ry, double mx, double my, Arc_t* crcl, double* p1x, double* p1y, double* p2x, double* p2y) {
    // 1. Параметры прямой
    const double A = my - ry;
    const double B = rx - mx;
    const double C = -(A * rx + B * ry);

    // 2. Параметры окружности
    const double cx = crcl->center.x, cy = crcl->center.y, R = crcl->R;

    // 3. ПРОВЕРКА УЗЛОВ (только если это не полная окружность)
    uint8_t nearA = 0, nearB = 0;
    if (crcl->dir != 0) {
        nearA = is_point_in_ray_neighborhood(rx, ry, mx, my, crcl->a->x, crcl->a->y);
        nearB = is_point_in_ray_neighborhood(rx, ry, mx, my, crcl->b->x, crcl->b->y);
    }

    //printf("nearA = %i ; nearB = %i \n", nearA, nearB );

    // 4. АНАЛИТИЧЕСКИЙ РАСЧЕТ
    double xA = 0, yA = 0, xB = 0, yB = 0;
    uint8_t hasA = 0, hasB = 0, tangent = 0;

    if (fabs(B) < 1e-12) { // Вертикальный луч
        double x = rx;
        double dx = x - cx;
        double det = R * R - dx * dx;
        if (det >= -epsilon) {
            if (fabs(det) < epsilon) tangent = 1;
            double sdet = sqrt(fmax(0.0, det));
            xA = x; yA = cy + sdet; hasA = 1;
            if (!tangent) { xB = x; yB = cy - sdet; hasB = 1; }
        }
    } else { // Общий и горизонтальный случай
        double k = -A / B;
        double b_line = -C / B;
        double ka = 1 + k * k;
        double kb = 2 * (k * (b_line - cy) - cx);
        double kc = cx * cx + (b_line - cy) * (b_line - cy) - R * R;
        double D = kb * kb - 4 * ka * kc;
        if (D >= -epsilon) {
            if (fabs(D) < epsilon) tangent = 1;
            double sD = sqrt(fmax(0.0, D));
            xA = (-kb + sD) / (2 * ka); yA = k * xA + b_line; hasA = 1;
            if (!tangent) { xB = (-kb - sD) / (2 * ka); yB = k * xB + b_line; hasB = 1; }
        }
    }

    // 5. ФИЛЬТРАЦИЯ ПО ДИАПАЗОНУ ЛУЧА И ДУГЕ
    double r_xmin = fmin(rx, mx) - epsilon, r_xmax = fmax(rx, mx) + epsilon;
    double r_ymin = fmin(ry, my) - epsilon, r_ymax = fmax(ry, my) + epsilon;

    if (hasA) {
        int on_arc = (crcl->dir == 0) || is_xy_on_arc(crcl, xA, yA);
        if (!(xA >= r_xmin && xA <= r_xmax && yA >= r_ymin && yA <= r_ymax && on_arc)) hasA = 0;
    }
    if (hasB) {
        int on_arc = (crcl->dir == 0) || is_xy_on_arc(crcl, xB, yB);
        if (!(xB >= r_xmin && xB <= r_xmax && yB >= r_ymin && yB <= r_ymax && on_arc)) hasB = 0;
    }

    // 6. ГИБРИДНАЯ КОРРЕКЦИЯ (только для дуг с концами)
    if (crcl->dir != 0) {
        double ax = crcl->a->x, ay = crcl->a->y;
        double bx = crcl->b->x, by = crcl->b->y;

        if (!hasA && !hasB) {
            if (nearA) { *p1x = ax; *p1y = ay; return 1; }
            if (nearB) { *p1x = bx; *p1y = by; return 1; }
            return 0;
        }

        if(hasA){
            if(nearA && xy_eq_xy(xA, yA, ax, ay)){ xA = ax; yA = ay; }
            else if(nearB && xy_eq_xy(xA, yA, bx, by)) { xA = bx; yA = by; }
        }
        if(hasB){
            if( nearA && xy_eq_xy(xB, yB, ax, ay) ){ 
            	xB = ax; yB = ay;
            }else if( nearB && xy_eq_xy(xB, yB, bx, by) ){
            	xB = bx; yB = by;
            }
        }
    }

    // 7. СБОРКА РЕЗУЛЬТАТА
    if (hasA && hasB) {
        if (xy_eq_xy(xA, yA, xB, yB)) { *p1x = xA; *p1y = yA; return 1; }
        *p1x = xA; *p1y = yA; *p2x = xB; *p2y = yB; return 3;
    }
    if (hasA) { *p1x = xA; *p1y = yA; return (tangent ? 2 : 1); }
    if (hasB) { *p1x = xB; *p1y = yB; return (tangent ? 2 : 1); }

    // Последний шанс для дуг
    if (crcl->dir != 0) {
        if (nearA) { *p1x = crcl->a->x; *p1y = crcl->a->y; return 1; }
        if (nearB) { *p1x = crcl->b->x; *p1y = crcl->b->y; return 1; }
    }

    return 0;
}
