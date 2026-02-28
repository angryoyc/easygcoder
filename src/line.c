#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "../include/top.h"
#include "../include/reflist.h"
#include "../include/context.h"
#include "../include/point.h"
#include "../include/cont.h"
#include "../include/geom.h"
#include "../include/line.h"

uint8_t is_line(Refitem_t* item){
	return (item->type == OBJ_TYPE_LINE);
}

int line_onunlink( Refitem_t* self, Refitem_t* ref ){
	Line_t* this = (Line_t*) self;
	if( (Refitem_t*) this->a == ref ) return 0;
	if( (Refitem_t*) this->b == ref ) return 0;
	return 1; // можно удалять точки
}

void line_ini(Line_t* l){ // инициализация математических параметров отрезка - коэффициентов прямой на которой лежит отрезок и его длина
	l->A = l->b->y - l->a->y;
	l->B = l->a->x - l->b->x;
	l->C = l->a->x * (l->b->y - l->a->y) + l->a->y * (l->b->x - l->a->x);
	l->len = points_distance(l->a, l->b);
}

Line_t* create_line( Point_t* a, Point_t* b ){
	int n = sizeof( Line_t );
	Line_t* l = malloc( n );
	memset( l, 0x00, n );
	l->type = OBJ_TYPE_LINE;
	l->onunlink = line_onunlink;
	l->a = a;
	l->b = b;
	l->id = prim_counter++;
	linkobj2obj( l, a );
	linkobj2obj( l, b );
	Context_t* ctx = get_context();
	linkobj2obj(ctx, l);
	line_ini(l);
	return l;
}


/*
* удаление линии (отвязка примитива от всех точек)
*/
uint8_t remove_line( Line_t** ptr ){
	Line_t* l = *ptr;
	if( l ){
		l->a = NULL;
		l->b = NULL;
		Refholder_t* list = unlinkyouself( (Refitem_t*) l );
		purge_by_list( list, (Refitem_t**) &l );
		if( l->links.count != 0 ) return 0;
		if( l->links.arr ){
			free( l->links.arr );
			l->links.arr = NULL;
		}
		free(l);
		l = NULL;
	}
	*ptr = NULL;
	return 1;
}

/*
* Проверка принадлежности точки p отрезку (a, b)
*/
uint8_t is_p_on_line(Point_t* a, Point_t* b, Point_t* p){
	if( !a && !b )  printf("Ссылка на концевую точку пуста\n");
	// 1. Проверка на коллинеарность через векторное произведение
/*
	double cross = (b->x - a->x) * (p->y - a->y) - (b->y - a->y) * (p->x - a->x);
	// допускаем маленькую погрешность для double
	if ( fabs(cross) > epsilon ) return 0;
	// 2. Проверка, что p лежит в пределах отрезка
	if (p->x < fmin(a->x, b->x) - epsilon || p->x > fmax(a->x, b->x) + epsilon) return 0;
	if (p->y < fmin(a->y, b->y) - epsilon || p->y > fmax(a->y, b->y) + epsilon) return 0;
	return 1;
*/
	return is_xy_on_line( a->x, a->y, b->x, b->y, p->x, p->y );
}



/*
* Разбить линию, на две части точкой.
*/
Line_t* split_line_by_p(Line_t* l, Point_t* p){
	if( (l->a) && (l->b) && (is_p_on_line(l->a, l->b, p)) ){ // разбиение отрезка на две части по точке p
		Point_t* last_point = l->b;
		l->b = p;
		unlinkobj2obj( l, last_point );
		linkobj2obj(l, l->b );
		Line_t* new_l = create_line( p, last_point );
		linkobj2obj(new_l, new_l->a );
		linkobj2obj(new_l, new_l->b );
		if( l->cont ){
			add_item2cont( (Refitem_t*) new_l, l->cont );
			put_after( (Refitem_t*) new_l, l->cont, (Refitem_t*) l );
		}
		// перенос точкек
		if( l->links.count > 0 ){
			for( int i = l->links.count-1; i>=0 ; i--){
				if( ((Refitem_t*) l->links.arr[i])->type == OBJ_TYPE_POINT ){
					Point_t* p = l->links.arr[i];
					if( (l->a != p) && (l->b != p) ){
						if( !is_p_on_line(l->a, l->b, p) ){ // если координаты точки больше не принадлежит отрезку то переносим точку на новый отрезок
							unlinkobj2obj( l, p );
							linkobj2obj( new_l, p );
						}
					}
				}
			}
		}
		return new_l;
	}else{
		return NULL;
	}
}

/*
* создание точки пересечения отрезков
* Если отрезки пересекаются концами, то возвращаются ссылки на существующие точки-концы, а не создаются новые
*/
uint8_t create_p_of_line_x_line(Point_t* a, Point_t* b, Point_t* c, Point_t* d, Point_t** p1, Point_t** p2){
	double x1;
	double y1;
	double x2;
	double y2;
	*p1 = NULL;
	*p2 = NULL;
	int ret = xy_of_line_x_line( a->x, a->y, b->x, b->y, c->x, c->y, d->x, d->y, &x1, &y1, &x2, &y2 );
	if( ret == 1  ){
	}else if( ret == 2 ){
		*p1 = create_p( x1, y1 );
	}else if( ret == 3 ){
		if( xy_eq_xy( a->x, a->y, x1, y1 ) ) *p1 = a;
		if( xy_eq_xy( b->x, b->y, x1, y1 ) ) *p1 = b;
		if( xy_eq_xy( c->x, c->y, x1, y1 ) ) *p2 = c;
		if( xy_eq_xy( d->x, d->y, x1, y1 ) ) *p2 = d;
	}else if( ret == 4 ){
		if( xy_eq_xy( a->x, a->y, x1, y1 ) ) *p1 = a;
		if( xy_eq_xy( b->x, b->y, x1, y1 ) ) *p1 = b;
		if( xy_eq_xy( c->x, c->y, x1, y1 ) ) *p1 = c;
		if( xy_eq_xy( d->x, d->y, x1, y1 ) ) *p1 = d;
	}else if( ret == 5 ){
	}else if( ret == 6 ){
	}else if( ret == 7 ){
		if( xy_eq_xy( a->x, a->y, x1, y1 ) ) *p1 = a;
		if( xy_eq_xy( b->x, b->y, x1, y1 ) ) *p1 = b;
		if( xy_eq_xy( c->x, c->y, x1, y1 ) ) *p2 = c;
		if( xy_eq_xy( d->x, d->y, x1, y1 ) ) *p2 = d;
	}else if( ret == 8 ){
		if( xy_eq_xy( a->x, a->y, x1, y1 ) ) *p1 = a;
		if( xy_eq_xy( b->x, b->y, x1, y1 ) ) *p1 = b;
		if( xy_eq_xy( c->x, c->y, x1, y1 ) ) *p1 = c;
		if( xy_eq_xy( d->x, d->y, x1, y1 ) ) *p1 = d;

		if( xy_eq_xy( c->x, c->y, x2, y2 ) ) *p2 = c;
		if( xy_eq_xy( d->x, d->y, x2, y2 ) ) *p2 = d;
		if( xy_eq_xy( c->x, c->y, x2, y2 ) ) *p2 = c;
		if( xy_eq_xy( d->x, d->y, x2, y2 ) ) *p2 = d;
	}else if( ret == 9 ){

		if( xy_eq_xy( a->x, a->y, x1, y1 ) ) *p1 = a;
		if( xy_eq_xy( b->x, b->y, x1, y1 ) ) *p1 = b;
		if( xy_eq_xy( c->x, c->y, x1, y1 ) ) *p1 = c;
		if( xy_eq_xy( d->x, d->y, x1, y1 ) ) *p1 = d;

		if( xy_eq_xy( a->x, a->y, x2, y2 ) ) *p2 = a;
		if( xy_eq_xy( b->x, b->y, x2, y2 ) ) *p2 = b;
		if( xy_eq_xy( c->x, c->y, x2, y2 ) ) *p2 = c;
		if( xy_eq_xy( d->x, d->y, x2, y2 ) ) *p2 = d;

	}else if( ret == 10 ){
		if( xy_eq_xy( a->x, a->y, x1, y1 ) ) *p1 = a;
		if( xy_eq_xy( b->x, b->y, x1, y1 ) ) *p1 = b;
		if( xy_eq_xy( c->x, c->y, x1, y1 ) ) *p1 = c;
		if( xy_eq_xy( d->x, d->y, x1, y1 ) ) *p1 = d;

		if( xy_eq_xy( c->x, c->y, x2, y2 ) ) *p2 = c;
		if( xy_eq_xy( d->x, d->y, x2, y2 ) ) *p2 = d;
		if( xy_eq_xy( c->x, c->y, x2, y2 ) ) *p2 = c;
		if( xy_eq_xy( d->x, d->y, x2, y2 ) ) *p2 = d;
	}else if( ret == 11 ){
		if( xy_eq_xy( a->x, a->y, x1, y1 ) ) *p1 = a;
		if( xy_eq_xy( b->x, b->y, x1, y1 ) ) *p1 = b;
		if( xy_eq_xy( c->x, c->y, x1, y1 ) ) *p1 = c;
		if( xy_eq_xy( d->x, d->y, x1, y1 ) ) *p1 = d;

		if( xy_eq_xy( a->x, a->y, x2, y2 ) ) *p2 = a;
		if( xy_eq_xy( b->x, b->y, x2, y2 ) ) *p2 = b;
		if( xy_eq_xy( c->x, c->y, x2, y2 ) ) *p2 = c;
		if( xy_eq_xy( d->x, d->y, x2, y2 ) ) *p2 = d;
	}
	return ret;
}
/*
void create_p_of_line_x_line(Point_t* a, Point_t* b, Point_t* c, Point_t* d, Point_t** p1, Point_t** p2){
	double x1;
	double y1;
	double x2;
	double y2;
	*p1 = NULL;
	*p2 = NULL;
	int ret = xy_of_line_x_line( a->x, a->y, b->x, b->y, c->x, c->y, d->x, d->y, &x1, &y1, &x2, &y2 );
	if( ret == 1  ){
		if( (a->x == x1) && (a->y == y1) ){
			*p1 = a;
			if( (c->x == x1) && (c->y == y1) ){
				*p2 = c;
			}else if( (d->x == x1) && (d->y == y1) ){
				*p2 = d;
			}
		}else if( (b->x == x1) && (b->y == y1) ){
			*p1 = b;
			if( (c->x == x1) && (c->y == y1) ){
				*p2 = c;
			}else if( (d->x == x1) && (d->y == y1) ){
				*p2 = d;
			}
		}else if( (c->x == x1) && (c->y == y1) ){
			*p1 = c;
			if( (a->x == x1) && (a->y == y1) ){
				*p2 = a;
			}else if( (b->x == x1) && (b->y == y1) ){
				*p2 = b;
			}
		}else if( (d->x == x1) && (d->y == y1) ){
			*p1 = d;
			if( (a->x == x1) && (a->y == y1) ){
				*p2 = a;
			}else if( (b->x == x1) && (b->y == y1) ){
				*p2 = b;
			}
		}else{
			*p1 = create_p( x1, y1 );
		}
	}else if( ret == 2 ){
		if( (a->x == x1) && (a->y == y1) ){
			*p1 = a;
			if( (b->x == x2) && (b->y == y2) ){
				*p2 = b;
			}else if( (c->x == x2) && (c->y == y2) ){
				*p2 = c;
			}else if( (d->x == x2) && (d->y == y2) ){
				*p2 = d;
			}else{
				*p2 = create_p( x2, y2 );
			}
		}else if( (b->x == x1) && (b->y == y1) ){
			*p1 = b;
			if( (a->x == x2) && (a->y == y2) ){
				*p2 = a;
			}else if( (c->x == x2) && (c->y == y2) ){
				*p2 = c;
			}else if( (d->x == x2) && (d->y == y2) ){
				*p2 = d;
			}else{
				*p2 = create_p( x2, y2 );
			}
		}else if( (c->x == x1) && (c->y == y1) ){
			*p1 = c;
			if( (a->x == x2) && (a->y == y2) ){
				*p2 = a;
			}else if( (b->x == x2) && (b->y == y2) ){
				*p2 = b;
			}else if( (d->x == x2) && (d->y == y2) ){
				*p2 = d;
			}else{
				*p2 = create_p( x2, y2 );
			}
		}else if( (d->x == x1) && (d->y == y1) ){
			*p1 = d;
			if( (a->x == x2) && (a->y == y2) ){
				*p2 = a;
			}else if( (b->x == x2) && (b->y == y2) ){
				*p2 = b;
			}else if( (c->x == x2) && (c->y == y2) ){
				*p2 = c;
			}else{
				*p2 = create_p( x2, y2 );
			}
		}else{
			*p1 = create_p( x1, y1 );
			*p2 = create_p( x2, y2 );
		}
	}
}

*/
/*
* Создание точки середины отрезка
*/
Point_t* create_p_of_line_mid(Point_t* a, Point_t* b){
	double x;
	double y;
	middle( a->x, a->y, b->x, b->y, &x, &y );
	return create_p(x, y);
}

// Получение экстремальных значений координат отрезка
void line_get_bounds(const Line_t* l, double* xmin, double* xmax, double* ymin, double* ymax){
	*xmin = *ymin = -1000000;
	*xmax = *ymax =  1000000;
	if( l ){
		*xmin = fminf(l->a->x, l->b->x);
		*xmax = fmaxf(l->a->x, l->b->x);
		*ymin = fminf(l->a->y, l->b->y);
		*ymax = fmaxf(l->a->y, l->b->y);
	}
};

void replace_a( Line_t* l, Point_t* p){
	Point_t* old = l->a;
	cp_links( (Refitem_t*) old, (Refitem_t*) p);
	l->a = NULL;
	remove_p( &old );
	l->a = p;
	linkobj2obj( l, l->a );
}

void replace_b( Line_t* l, Point_t* p){
	Point_t* old = l->b;
	cp_links( (Refitem_t*) old, (Refitem_t*) p);
	l->b = NULL;
	remove_p( &old );
	l->b = p;
	linkobj2obj( l, l->b );
}

void replace_same_p2lines( Line_t* l1, Line_t* l2 ){
	if( p_eq_p( l1->a, l2->a ) && (l1->a != l2->a) ) replace_a( l2, l1->a);
	if( p_eq_p( l1->a, l2->b ) && (l1->a != l2->b) ) replace_b( l2, l1->a);
	if( p_eq_p( l1->b, l2->a ) && (l1->b != l2->a) ) replace_a( l2, l1->b);
	if( p_eq_p( l1->b, l2->b ) && (l1->b != l2->a) ) replace_b( l2, l1->b);
}

