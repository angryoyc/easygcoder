#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "../include/top.h"
#include "../include/reflist.h"
#include "../include/context.h"
#include "../include/geom.h"
#include "../include/point.h"

int point_onunlink( Refitem_t* self, Refitem_t* ref ){
	return 1; // можно удалять точки
}

Point_t* create_p(double x, double y){
	int n = sizeof( Point_t );
	Point_t* p = malloc( n );
	memset( p, 0x00, n );
	p->type = OBJ_TYPE_POINT;
	p->onunlink = point_onunlink;
	p->x = x;
	p->y = y;
	Context_t* ctx = get_context();
	linkobj2obj(ctx, p);
	return p;
}

/*
* удаление точки (отвязка точки от всех примитивов)
*/
uint8_t remove_p( Point_t** ptr ){
	Point_t* p = *ptr;
	if( p ){
		Refholder_t* list = unlinkyouself( (Refitem_t*) p );
		purge_by_list( list, (Refitem_t**) &p );
		if( p->links.count != 0 ) return 0;
		if( p->links.arr ){
			free( p->links.arr );
			p->links.arr = NULL;
		}
		free(p);
		p = NULL;
	}
	*ptr = NULL;
	return 1;
}

// Возвращает 1 если координаты точке совпадают
uint8_t p_eq_p(Point_t* p1, Point_t* p2){
	if( p1 && p2 ) return  (fabs(p1->x - p2->x) < epsilon) && ( fabs(p1->y - p2->y) < epsilon);
	return 0;
}

// Возвращает 1 если координаты точке НЕ совпадают
uint8_t p_ne_p(Point_t* p1, Point_t* p2){
	if( p1 && p2 ) return  (fabs(p1->x - p2->x)>=epsilon) || (fabs(p1->y - p2->y)>=epsilon);
	return 1;
}

