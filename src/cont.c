#include <math.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "../include/top.h"
#include "../include/reflist.h"
#include "../include/context.h"
#include "../include/point.h"
#include "../include/geom.h"
#include "../include/cont.h"
#include "../include/line.h"
#include "../include/arc.h"
#include "../include/mergcont.h"
#include "../include/gcode.h"
#include "../include/excellon.h"
#include "../include/export.h"

static int cont_counter = 1;

int is_seg( Refitem_t* item ){ // является ли элемент Refitem_t сегментом контура (дугой или отрезком)
	return ( is_line(item) || is_arc(item) )?1:0;
}

int is_cont( Refitem_t* item ){ // является ли элемент Refitem_t контуром
	return ( item->type == OBJ_TYPE_CONTUR )?1:0;
}


int contur_onunlink( Refitem_t* self, Refitem_t* ref ){
	return 1; // можно удалять точки
}

/*
* Создание замкнутого набора примитивов - контура
*/
Cont_t* create_cont(){
	int n = sizeof( Cont_t );
	Cont_t* cont = malloc( n );
	memset( cont, 0x00, n );
	cont->type = OBJ_TYPE_CONTUR;
	cont->onunlink = contur_onunlink;
	cont->num = cont_counter++;
	cont->xmax = -1000000;
	cont->ymax = -1000000;
	cont->xmin = 1000000;
	cont->ymin = 1000000;
	Context_t* ctx = get_context();
	linkobj2obj(ctx, cont);
	return cont;
}

/*
* удаление контура
*/
uint8_t remove_cont( Cont_t** ptr ){
	Cont_t* cont = *ptr;
	if( cont ){
		if( cont->links.arr ){
			for( int i = cont->links.count-1; i >= 0; i-- ){
				Refitem_t* item = cont->links.arr[i];
				if( item ){
					if( ( item->type == OBJ_TYPE_LINE ) || ( item->type == OBJ_TYPE_ARC ) ){
						if( item->cont == cont ) item->cont = NULL;
						if( item->cont_r == cont ) item->cont_r = NULL;
						if( item->cont_l == cont ) item->cont_l = NULL;
					}
				}
			}
		}
		Refholder_t* list = unlinkyouself( (Refitem_t*) cont );
		purge_by_list( list, (Refitem_t**) &cont );
		if( cont->links.count != 0 ) return 0;
		if( cont->links.arr ){
			free( cont->links.arr );
			cont->links.arr = NULL;
		}
		free(cont);
		cont = NULL;
	}
	*ptr = NULL;
	return 1;
}

/*
* Удаление контуров по списку (только контуров, без составляющих их сегментов)
* список так же очищается
*/
void remove_conts_by_list( Refholder_t** list ){
	Refholder_t* curr = *list;
	while(curr){
		Refholder_t* next = curr->next;
		remove_cont( (Cont_t**) &curr->refitem );
		curr->next = NULL;
		free( curr );
		curr = next;
	}
	*list = NULL;
}


/*
* Удаление всех элементов контура и его самого
*/
uint8_t purge_cont( Cont_t** ptr ){
	Cont_t* cont = *ptr;
	if( cont ){
		if( cont->links.arr ){
			for( int i = cont->links.count-1; i >= 0; i-- ){
				Refitem_t* item = cont->links.arr[i];
				if( item ){
					if( item->type == OBJ_TYPE_LINE ){
						Line_t* line = (Line_t*) item;
						remove_line( &line );
					}
					if( item->type == OBJ_TYPE_ARC ){
						Arc_t* arc = (Arc_t*) item;
						remove_arc( &arc );
					}
				}
			}
		}
		return remove_cont( ptr );
	}
	return 0;
}

/*
* Добавление примитива к списку контура
*/
Refitem_t* add_item2cont( Refitem_t* item, Cont_t* cont ){
	if(!item || !cont) return NULL;
	if( item->cont && (item->cont != cont) ){
		rm_item2cont( item );
	}
	item->cont = cont;
	linkobj2obj( cont, item );
	return item;
}

/*
* Добавление примитива к списку прямого контура (направление совпадает с направлением основного сегмента)
*/
Refitem_t* add_item2cont_r( Refitem_t* item, Cont_t* cont ){
	if(!item || !cont) return NULL;
//	printf("item %p set cont_r\n", item);
	if( item->cont_r && (item->cont_r != cont) ){
//		printf("BUSY!!!\n", item);
		rm_item2cont_r( item );
	}
	item->cont_r = cont;
	linkobj2obj( cont, item );
	return item;
}

/*
* Добавление примитива к списку прямого контура (направление противоположно основному сегменту)
*/
Refitem_t* add_item2cont_l( Refitem_t* item, Cont_t* cont ){
	if(!item || !cont) return NULL;
//	printf("item %p set cont_l\n", item);
	if( item->cont_l && (item->cont_l != cont) ){
//		printf("BUSY!!!\n", item);
		rm_item2cont_l( item );
	}
	item->cont_l = cont;
	linkobj2obj( cont, item );
	return item;
}

/*
* Удаление примитива из списка контура
*/
void rm_item2cont( Refitem_t* item ){
	if( item->cont ){
		Cont_t* cont = item->cont;
		item->cont = NULL;
		unlinkobj2obj( item, cont );
	}
	return;
}

/*
* Удаление примитива из списка прямого контура
*/
void rm_item2cont_r( Refitem_t* item ){
	if( item->cont_r ){
		Cont_t* cont = item->cont_r;
		item->cont_r = NULL;
		unlinkobj2obj( item, cont );
	}
	return;
}

/*
* Удаление примитива из списка прямого контура
*/
void rm_item2cont_l( Refitem_t* item ){
	if( item->cont_l ){
		Cont_t* cont = item->cont_l;
		item->cont_l = NULL;
		unlinkobj2obj( item, cont );
	}
	return;
}

/*
* Приведение показателей contcount примитивов контура к норме
void crop_contcount( Cont_t* cont ){
	if( cont ){
		if( cont->links.arr ){
			for( int i=0; i < cont->links.count; i++ ){
				Refitem_t* item = cont->links.arr[i];
				if( item && ( (item->type == OBJ_TYPE_LINE) || (item->type == OBJ_TYPE_ARC) ) ){
					item->contcount = item->contcount - cont->mincc;
					if( cont->dir > 0 ) item->contcount = item->contcount^1;			// Пробуем учитывать направление контура!!!
				}
			}
		}
	}
}
*/

/*
* Удаление из контура примитивов с нечётными показателями contcount
*/
void clean_cont( Cont_t* cont ){
	if( cont ){
		if( cont->links.arr && (cont->links.count>0) ){
			for( int i = cont->links.count-1; i >= 0; i-- ){
				Refitem_t* item = cont->links.arr[i];
				if( item && ( (item->type == OBJ_TYPE_LINE) || (item->type == OBJ_TYPE_ARC) ) ){
					if( (item->contcount & 1) == 1 ){
						if(item->type == OBJ_TYPE_LINE){
							remove_line( (Line_t**) &item );
						}else if(item->type == OBJ_TYPE_ARC){
							remove_arc( (Arc_t**) &item );
						}
					}
				}
			}
		}
	}
}

void cont_accept_extrem(Cont_t* cont, double xmin, double xmax, double ymin, double ymax){
	cont->xmin = fminf(cont->xmin, xmin);
	cont->xmax = fmaxf(cont->xmax, xmax);
	cont->ymin = fminf(cont->ymin, ymin);
	cont->ymax = fmaxf(cont->ymax, ymax);
}

/*
* вычисление экстремальных значений координат контура
*/
void cont_get_bounds( Cont_t* cont ){
	if( cont ){
		//printf("Обход контура %i\n", cont->num);
		//cont->dir = cont_dir(cont);
		// printf( "Обход контура %i направление %i\n", cont->num, cont->dir );
		if( cont->links.arr ){
			for( int i=0; i < cont->links.count; i++ ){
				Refitem_t* item = cont->links.arr[i];
				double xmin;
				double xmax;
				double ymin;
				double ymax;
				if( item ){
					if(item->type == OBJ_TYPE_LINE){
						Line_t* l = cont->links.arr[i];
						line_get_bounds( l, &xmin, &xmax, &ymin, &ymax );
						cont_accept_extrem( cont, xmin, xmax, ymin, ymax );
					}
					if(item->type == OBJ_TYPE_ARC){
						Arc_t* arc = cont->links.arr[i];
						arc_get_bounds( arc, &xmin, &xmax, &ymin, &ymax );
						cont_accept_extrem( cont, xmin, xmax, ymin, ymax );
					}
				}
			}
		}
	}
	return;
}



/*
* Дуга и линия режут друг друга по точками пересечений (их может быть 0, 1 или две) на соответствующее число частей
*/
int split_arc_by_line( Arc_t* arc, Line_t* l){
	// printf("// split_arc_by_line (Пересечение дуги и линии)\n");
	int s = 0;
	Point_t* p1; Point_t* p2;
	create_p_of_line_x_arc( l, arc, &p1, &p2 );
	if( p1 ){
		//if(debug) printf(" arc_by_line p1\n");
		if( p2 ){
			//if(debug) printf(" arc_by_line p2\n");
			if( p1 == p2 ){
				// одно касание (в т. p1)
				//if(debug) printf("// Одно касание (в т. p1)\n");
				//printf("// p1->links.count = %i\n", p1->links.count);
				if( p_ne_p(p1, arc->a) && p_ne_p(p1, arc->b) ){
					// 30.750020, -13.970020
					if( arc->dir == 0 ){
						// printf("break_the_circle\n");
						break_the_circle( arc, p1, arc->cont?(arc->cont->dir):-1 );
						s++;
					}else{
						split_arc_by_p( arc, p1 );
						s++;
					}
				}
				if( p_ne_p( p1, l->a ) && p_ne_p( p1, l->b ) ){
					// printf("split_line_by_p\n");
					split_line_by_p( l, p1 );
					s++;
				}
				if( p_eq_p(p1, l->a) && (p1 != l->a) ){ replace_a(l, p1);/* printf("<<La>>\n");*/}
				if( p_eq_p(p1, l->b) && (p1 != l->b) ){ replace_b(l, p1);/* printf("<<Lb>>\n");*/}
				if( p_eq_p(p1, arc->a) && (p1 != arc->a) ){ replace_arc_a(arc, p1);/* printf("<<Aa>>\n");*/}
				if( p_eq_p(p1, arc->b) && (p1 != arc->b) ){ replace_arc_b(arc, p1);/* printf("<<Ab>>\n");*/}
			}else{
				// два пересечения
				// printf("// два пересечения\n");
				if( p_ne_p(p1, arc->a) && p_ne_p(p1, arc->b) ){
					if( arc->dir==0 ){
						break_the_circle( arc, p1, arc->cont?(arc->cont->dir):-1 );
						s++;
					}else{
						split_arc_by_p( arc, p1 );
						s++;
					}
				}
				if( p_ne_p(p1, l->a) && p_ne_p(p1, l->b) ){
					split_line_by_p( l, p1 );
					s++;
				}
				if( p_ne_p(p2, arc->a) && p_ne_p(p2, arc->b) ){
					split_arc_by_p(  arc, p2 );
					s++;
				}
				if( p_ne_p(p2, l->a) && p_ne_p(p2, l->b) ){
					split_line_by_p( l,   p2 );
					s++;
				}
				if( p_eq_p(p1, l->a)   && (p1 != l->a) ) replace_a(l, p1);
				if( p_eq_p(p1, l->b)   && (p1 != l->b) ) replace_b(l, p1);
				if( p_eq_p(p1, arc->a) && (p1 != arc->a) ) replace_arc_a(arc, p1);
				if( p_eq_p(p1, arc->b) && (p1 != arc->b) ) replace_arc_b(arc, p1);

				if( p_eq_p(p2, l->a)   && (p2 != l->a) ) replace_a(l, p2);
				if( p_eq_p(p2, l->b)   && (p2 != l->b) ) replace_b(l, p2);
				if( p_eq_p(p2, arc->a) && (p2 != arc->a) ) replace_arc_a(arc, p2);
				if( p_eq_p(p2, arc->b) && (p2 != arc->b) ) replace_arc_b(arc, p2);

			}
		}else{
			// Одно пересечение (в т. p1)
			//printf("// Одно пересечение (в т. p1)\n");
			if( p_ne_p(p1, arc->a) && p_ne_p(p1, arc->b) ){
				//print_p(p1);
				if( arc->dir == 0 ){
					// printf("\nПолная окружность\n");
					break_the_circle( arc, p1, arc->cont?(arc->cont->dir):-1 );
					s++;
				}else{
					// printf("\nОбычная дуга %i\n", arc->dir);
					split_arc_by_p( arc, p1 );
					s++;
				}
			}
			if( p_ne_p( p1, l->a ) && p_ne_p( p1, l->b ) ){
				split_line_by_p( l, p1 );
				s++;
			}
			if( p_eq_p(p1, l->a) && (p1 != l->a) ) replace_a(l, p1);
			if( p_eq_p(p1, l->b) && (p1 != l->b) ) replace_b(l, p1);
			if( p_eq_p(p1, arc->a) && (p1 != arc->a) ) replace_arc_a(arc, p1);
			if( p_eq_p(p1, arc->b) && (p1 != arc->b) ) replace_arc_b(arc, p1);
		}
	}else{
		// нет пересечений (ничо не делаем)
	}
	return s;
}

/*
void print4list(Point_t* list[], int* index){
	for( int i=0; i < *index; i++){
		printf("Точка для разбиения arc1: ");
		print_p( list[i] );
		printf("\n");
	}
}
*/

void append2list(Point_t* list[], int* index, Point_t* p){
	int flag = 1;
	for( int i=0; i < *index; i++){
		if( p_eq_p( list[i], p ) ){
			flag = 0;
			break;
		}
	}
	if( flag ){
		list[ *index ] = p;
		*index = (*index) + 1;
	}
}

// режем дугу точками из списка (длина списка в index)
void to_chop_arc( Point_t* list[], int* index, Arc_t* arc ){
	Arc_t* arc_list[5]; // список дуг образованных в результате реза
	arc_list[0] = arc;  // сразу пишем туда исходную дугу
	int arc_count = 1;
	for( int i=0; i < *index; i++){           // перебираем все точки в списке.
		Point_t* next_p = list[i];
		for( int j=0; j < arc_count; j++){    // ищем дугу, которой принадлежит точка
			Arc_t* a = arc_list[j];
			if( is_xy_on_arc( a, next_p->x, next_p->y) ){     // точка принадлежит дуге, но не совпадает с концами
				if( arc->dir==0 ){
					break_the_circle( arc, next_p, arc->cont?(arc->cont->dir):-1);
				}else{
					if( p_ne_p(next_p, arc->a) &&  p_ne_p(next_p, arc->b) ){
						Arc_t* new_arc = split_arc_by_p( arc, next_p );
						arc_list[arc_count++] = new_arc;
					}
				}
				break;
			}
		}
	}
}

int split_arc_by_arc( Arc_t* arc1, Arc_t* arc2 ){
	Point_t* list1[4]={NULL, NULL, NULL, NULL};
	Point_t* list2[4]={NULL, NULL, NULL, NULL};
	int listlen1 = 0;
	int listlen2 = 0;
	int s = 0;
	uint8_t bitmap = 0;
	Point_t* p1 = NULL;
	Point_t* p2 = NULL;
	bitmap = create_p_of_arc_x_arc( arc1, arc2, &p1, &p2 );
	if( bitmap>0 ){ // если есть хоть какие-нибудь пересечения
		uint8_t p1a2 = (1 << 5); // p1 on a2
		uint8_t p1a1 = (1 << 7); // p1 on a1
		if( ( (bitmap & p1a2) == p1a2) && ((bitmap & p1a1) == p1a1) ){ // p1 принадлежит и arc1 и arc2
			p1a2 = p1a2 >> 1;
			if( (bitmap & p1a2) == 0 ){ // p1 не совпадает с концами дуги a2
				//list2[listlen2++] = p1;
				append2list(list2, &listlen2, p1);
				s++;
			}else{
				if( p_eq_p(p1, arc2->a) && (arc2->a != p1) ) replace_arc_a(arc2, p1);
				if( p_eq_p(p1, arc2->b) && (arc2->b != p1) ) replace_arc_b(arc2, p1);
			}
			p1a1 = p1a1 >> 1;
			if( (bitmap & p1a1) == 0 ){ // p1 не совпадает с концами дуги a1
				//list1[listlen1++] = p1;
				append2list(list1, &listlen1, p1);
				s++;
			}else{
				if( p_eq_p(p1, arc1->a) && (arc1->a != p1) ) replace_arc_a(arc1, p1);
				if( p_eq_p(p1, arc1->b) && (arc1->b != p1) ) replace_arc_b(arc1, p1);
			}
		}
		uint8_t p2a2 = (1 << 1); // p2 on a2
		uint8_t p2a1 = (1 << 3); // p2 on a1
		//&& p_ne_p( *p1, *p2 )
		if( ( (bitmap & p2a2) == p2a2) && ((bitmap & p2a1) == p2a1) ){ // p2 принадлежит и arc1 и arc2 но не равно p1
			p2a2 = p2a2 >> 1;
			if( (bitmap & p2a2) == 0 ){ // p2 не совпадает с концами дуги a2
				//list2[listlen2++] = p2;
				append2list(list2, &listlen2, p2);
				s++;
			}else{
				if( p_eq_p(p2, arc2->a) && (arc2->a != p2) ) replace_arc_a(arc2, p2);
				if( p_eq_p(p2, arc2->b) && (arc2->b != p2) ) replace_arc_b(arc2, p2);
			}
			p2a1 = p2a1 >> 1;
			if( (bitmap & p2a1) == 0 ){ // p2 не совпадает с концами дуги a1
				//list1[listlen1++] = p2;
				append2list(list1, &listlen1, p2);
				s++;
			}else{
				if( p_eq_p(p2, arc1->a) && (arc1->a != p2) ) replace_arc_a(arc1, p2);
				if( p_eq_p(p2, arc1->b) && (arc1->b != p2) ) replace_arc_b(arc1, p2);
			}
		}
	}
	p1 = NULL;
	p2 = NULL;
	bitmap = create_p_of_arc_x_arc( arc2, arc1, &p1, &p2 );
	if( bitmap>0 ){ // если есть хоть какие-нибудь пересечения
		uint8_t p1a2 = (1 << 5); // p1 on a2
		uint8_t p1a1 = (1 << 7); // p1 on a1
		if( ( (bitmap & p1a2) == p1a2) && ((bitmap & p1a1) == p1a1) ){ // p1 принадлежит и arc1 и arc2
			p1a2 = p1a2 >> 1;
			if( (bitmap & p1a2) == 0 ){ // p1 не совпадает с концами дуги a1
				//list2[listlen2++] = p1;
				append2list(list1, &listlen1, p1);
				s++;
			}else{
				if( p_eq_p(p1, arc1->a) && (arc1->a != p1) ) replace_arc_a(arc1, p1);
				if( p_eq_p(p1, arc1->b) && (arc1->b != p1) ) replace_arc_b(arc1, p1);
			}
			p1a1 = p1a1 >> 1;
			if( (bitmap & p1a1) == 0 ){ // p1 не совпадает с концами дуги a2
				//list1[listlen1++] = p1;
				append2list(list2, &listlen2, p1);
				s++;
			}else{
				if( p_eq_p(p1, arc2->a) && (arc2->a != p1) ) replace_arc_a(arc2, p1);
				if( p_eq_p(p1, arc2->b) && (arc2->b != p1) ) replace_arc_b(arc2, p1);
			}
		}
		uint8_t p2a2 = (1 << 1); // p2 on a2
		uint8_t p2a1 = (1 << 3); // p2 on a1
		//&& p_ne_p( *p1, *p2 )
		if( ( (bitmap & p2a2) == p2a2) && ((bitmap & p2a1) == p2a1) ){ // p2 принадлежит и arc1 и arc2 но не равно p1
			p2a2 = p2a2 >> 1;
			if( (bitmap & p2a2) == 0 ){ // p2 не совпадает с концами дуги a1
				//list2[listlen2++] = p2;
				append2list(list1, &listlen1, p2);
				s++;
			}else{
				if( p_eq_p(p2, arc1->a) && (arc1->a != p2) ) replace_arc_a(arc1, p2);
				if( p_eq_p(p2, arc1->b) && (arc1->b != p2) ) replace_arc_b(arc1, p2);
			}
			p2a1 = p2a1 >> 1;
			if( (bitmap & p2a1) == 0 ){ // p2 не совпадает с концами дуги a2
				//list1[listlen1++] = p2;
				append2list(list2, &listlen2, p2);
				s++;
			}else{
				if( p_eq_p(p2, arc2->a) && (arc2->a != p2) ) replace_arc_a(arc2, p2);
				if( p_eq_p(p2, arc2->b) && (arc2->b != p2) ) replace_arc_b(arc2, p2);
			}
		}
	}
	to_chop_arc(list1, &listlen1, arc1);
	to_chop_arc(list2, &listlen2, arc2);
	return listlen1 + listlen2;
}

/*
int split_arc_by_arc2( Arc_t* arc1, Arc_t* arc2, Point_t** p1, Point_t** p2 ){
	int s = 0;
	Arc_t* new_arc2 = NULL;
	Arc_t* new_arc1 = NULL;
	uint8_t bitmap = create_p_of_arc_x_arc( arc1, arc2, p1, p2 );
	//printf("bitmap = %i\n", bitmap );
	// -- ОТЛАДКА
	printf("\n\n");
	if( *p1 || *p2 ){
		printf("//\narc1: ");
		print_arc(arc1);
		printf("\n и\narc2: ");
		print_arc(arc2);
		printf("\n");
		if( *p1 ){
			printf("* пересекаются в ");
			print_p( *p1 );
		}
		if( *p2 ){
			printf(" и в ");
			print_p( *p2 );
		}
		printf("bitmap = %i\n", bitmap);
	}
	// --
	if( bitmap>0 ){
		uint8_t p1a2 = (1 << 5); // p1 on a2
		uint8_t p1a1 = (1 << 7); // p1 on a1
		if( ( (bitmap & p1a2) == p1a2) && ((bitmap & p1a1) == p1a1) ){ //
			//printf("// p1a2 = %i\n", p1a2);
			//printf("// p1a1 = %i\n", p1a1);
			p1a2 = p1a2 >> 1;
			p1a1 = p1a1 >> 1;
			if( (bitmap & p1a2) == 0 ){
				// здесь надо разбить arc2 точкой p1
				//printf("// здесь надо разбить arc2 точкой p1\n");
				if( arc2->dir==0 ){
					printf("* разбиваем arc2 точкой ");
					print_p(*p1);
					printf("\n");
					break_the_circle( arc2, *p1, arc2->cont?(arc2->cont->dir):-1);
				}else{
					printf("* режем arc2 точкой ");
					print_p(*p1);
					printf("\n");
					new_arc2 = split_arc_by_p( arc2, *p1 );
				}
				s++;
			}
			if( (bitmap & p1a1) == 0 ){
				// здесь надо разбить arc1 точкой p1
				//printf("// здесь надо разбить arc1 точкой p1\n");
				if( arc1->dir==0 ){
					printf("* разбиваем  arc1 точкой ");
					print_p(*p1);
					printf("\n");
					break_the_circle( arc1, *p1, arc1->cont?(arc1->cont->dir):-1);
				}else{
					printf("* режем arc1 точкой ");
					print_p(*p1);
					printf("\n");
					new_arc1 = split_arc_by_p( arc1, *p1 );
				}
				s++;
			}
			// заменить точки совпадающие с p1 на p1 в arc1 и arc2
			if( p_eq_p(*p1, arc1->a) && (arc1->a != *p1) ) replace_arc_a(arc1, *p1);
			if( p_eq_p(*p1, arc1->b) && (arc1->b != *p1) ) replace_arc_b(arc1, *p1);
			if( p_eq_p(*p1, arc2->a) && (arc2->a != *p1) ) replace_arc_a(arc2, *p1);
			if( p_eq_p(*p1, arc2->b) && (arc2->b != *p1) ) replace_arc_b(arc2, *p1);
		}
		uint8_t p2a2 = (1 << 1); // p2 on a2
		uint8_t p2a1 = (1 << 3); // p2 on a1
		if( ( (bitmap & p2a2) == p2a2) && ((bitmap & p2a1) == p2a1) && p_ne_p( *p1, *p2 ) ){ // 
			p2a2 = p2a2 >> 1;
			p2a1 = p2a1 >> 1;
			if( (bitmap & p2a2) == 0 ){
				// здесь надо разбить arc2 точкой p2
				//printf("// здесь надо разбить arc2 точкой p2\n");
				if( new_arc2 ){ // если дуга уже была разбита т.p1, то возможно резать надо вторую половина
					if( xy_eq_xy(new_arc2->a->x, new_arc2->a->y, (*p2)->x, (*p2)->y )) arc2 = new_arc2;
				}
				if( arc2->dir==0 ){
					printf("* разбиваем arc2 точкой ");
					print_p(*p2);
					printf("\n");
					break_the_circle( arc2, *p2, arc2->cont?(arc2->cont->dir):-1 );
				}else{
					printf("* режем arc2 точкой ");
					print_p(*p2);
					printf("\n");
					split_arc_by_p( arc2, *p2 );
				}
				s++;
			}
			if( (bitmap & p2a1) == 0 ){
				// здесь надо разбить arc1 точкой p2
				//printf("// здесь надо разбить arc1 точкой p2\n");
				if( new_arc1 ){ // если дуга уже была разбита т.p1, то возможно резать надо вторую половина
					if( xy_eq_xy(new_arc1->a->x, new_arc1->a->y, (*p2)->x, (*p2)->y )) arc1 = new_arc1;
				}
				if( arc1->dir==0 ){
					printf("* разбиваем arc1 точкой ");
					print_p(*p2);
					printf("\n");
					break_the_circle( arc1, *p2, arc1->cont?(arc1->cont->dir):-1 );
				}else{
					printf("* режем arc1 точкой ");
					print_p(*p2);
					printf("\n");
					split_arc_by_p( arc1, *p2 );
				}
				s++;
			}
			// заменить точки совпадающие с p2 на p2 в arc1
			if( p_eq_p(*p2, arc1->a) && (arc1->a != *p2)  ) replace_arc_a(arc1, *p2);
			if( p_eq_p(*p2, arc1->b) && (arc1->b != *p2)  ) replace_arc_b(arc1, *p2);
			// заменить точки совпадающие с p2 на p2 в arc2
			if( p_eq_p(*p2, arc2->a) && (arc2->a != *p2)  ) replace_arc_a(arc2, *p2);
			if( p_eq_p(*p2, arc2->b) && (arc2->b != *p2)  ) replace_arc_b(arc2, *p2);
		}
	}else{
		// нет пересечений (ничо не делаем)
	}
	return s;
}
*/

/*
* Два примитива режут друг друга по точками  пересечений (их может быть 0, 1 или две) на соответствующее число
*/
int split_item_by_item( Refitem_t* item1, Refitem_t* item2, int debug){
	//if( debug ) printf("split_item_by_item\n");
	int s = 0;
	if( (item1->type == OBJ_TYPE_ARC) && (item2->type == OBJ_TYPE_ARC) ){ // Пересечение дуг
		Arc_t* arc1 = (Arc_t*) item1;
		Arc_t* arc2 = (Arc_t*) item2;
		s = s + split_arc_by_arc( arc1, arc2 );
//		printf("// Пересечение дуг %i\n", s);
	}else if( (item1->type == OBJ_TYPE_LINE) && (item2->type == OBJ_TYPE_LINE) ){ // Пересечение линий
		Line_t* l1 = (Line_t*) item1;
		Line_t* l2 = (Line_t*) item2;
		Line_t* new_l1;
		Line_t* new_l2;
		Point_t* p1;
		Point_t* p2;
		uint8_t ret = create_p_of_line_x_line( l1->a, l1->b, l2->a, l2->b, &p1, &p2 );
		if( ret==1 ){
		}else if( ret==2 ){
			//if(debug && ret) printf("// Пересечение линий %i\n", ret);
			if( p_ne_p(l1->a, p1) &&  p_ne_p(l1->b, p1) ){
				Line_t* new_l1 = split_line_by_p( l1, p1 );
				s++;
			}
			if( p_ne_p(l2->a, p1) && p_ne_p(l2->b, p1) ){
				Line_t* new_l2 = split_line_by_p( l2, p1 );
				s++;
			}
		}else if( ret==3 ){
			//if(debug && ret) printf("// Пересечение линий %i\n", ret);
			if( p_ne_p(l1->a, p1) &&  p_ne_p(l1->b, p1) ){
				Line_t* new_l1 = split_line_by_p( l1, p1 );
				s++;
			}
			if( p_ne_p(l2->a, p1) && p_ne_p(l2->b, p1) ){
				Line_t* new_l2 = split_line_by_p( l2, p1 );
				s++;
			}
		}else if( ret==4 ){
			//if(debug && ret) printf("// Пересечение линий %i\n", ret);
			if( p_ne_p(l1->a, p1) &&  p_ne_p(l1->b, p1) ){
				Line_t* new_l1 = split_line_by_p( l1, p1 );
				s++;
			}
			if( p_ne_p(l2->a, p1) && p_ne_p(l2->b, p1) ){
				Line_t* new_l2 = split_line_by_p( l2, p1 );
				s++;
			}
		}else if( ret==5 ){
		}else if( ret==6 ){
		}else if( ret==7 ){
			//if(debug && ret) printf("// Пересечение линий %i\n", ret);
			replace_same_p2lines(l1, l2);
		}else if( ret==8 ){
			//if(debug && ret) printf("// Пересечение линий %i\n", ret);
			if( p1 && p_ne_p( p1, l1->a) && p_ne_p( p1, l1->b) ){
				split_line_by_p( l1, p1 );
				s++;
			}
			if( p2 && p_ne_p( p2, l1->a) && p_ne_p( p2, l1->b) ){
				split_line_by_p( l1, p2 );
				s++;
			}
			if( p1 && p_ne_p( p1, l2->a) && p_ne_p( p1, l2->b) ){
				split_line_by_p( l2, p1 );
				s++;
			}
			if( p2 && p_ne_p( p2, l2->a) && p_ne_p( p2, l2->b) ){
				split_line_by_p( l2, p2 );
				s++;
			}
		}else if( ret==9 ){ // НЕ Оттестировано
			//if(debug && ret) printf("// Пересечение линий %i\n", ret);
			if( p_eq_p( p1, l1->a) && p_eq_p( p2, l1->b) ){ // l1 внутри l2 => резать надо l2 на три куска а l1 не трогаем
				new_l2 = split_line_by_p( l2, p1 );
				s++;
				if( p_ne_p( p1, l2->a ) && p_eq_p( p1, l2->b ) ){
					if( p_ne_p( p2, l2->a)      && p_ne_p( p2, l2->b)      && is_p_on_line(l2->a, l2->b, p2) ){
						new_l2 = split_line_by_p( l2, p2 );
						s++;
					}
					if( p_ne_p( p2, new_l2->a ) && p_ne_p( p2, new_l2->b ) && is_p_on_line( new_l2->a, new_l2->b, p2 )  ){
						split_line_by_p( new_l2, p2 );
						s++;
					}
				}
			}
			if( p_eq_p( p1, l2->a) && p_eq_p( p2, l2->b) ){ // l2 внутри l1 => резать надо l1 на три куска а l2 не трогаем
				Line_t* new_l1 = split_line_by_p( l1, p1 );
				s++;
				if( p_ne_p( p2, l1->a)     && p_ne_p( p2, l1->b)     && is_p_on_line( l1->a, l1->b, p2 ) ){
					new_l1 =  split_line_by_p( l1, p2 );
					s++;
				}
				if( p_ne_p( p2, new_l1->a) && p_ne_p( p2, new_l1->b) && is_p_on_line( new_l1->a, new_l1->b, p2 ) ){
					split_line_by_p( new_l1, p2 );
					s++;
				}
			}
		}else if( ret==10 ){     // настоящая жопа начинается здесь.
			//if(debug && ret) printf("// Пересечение линий %i\n", ret);
			if( p_eq_p( p1, l1->a) && p_eq_p( p2, l1->b) ){ // l1 внутри l2 => резать надо l2 на три куска а l1 не трогаем
				//printf("// l1 внутри l2 => резать надо l2 на три куска а l1 не трогаем\n");
				// находим точку которая не принадлежит обоим концам другого (l2) отрезка
				if( p_ne_p( p1, l2->a) && p_ne_p( p1, l2->b) ){ // Это точка p1 ею надо резать l2
					replace_same_p2lines(l1, l2);
					split_line_by_p( l2, p1 );
					s++;
				}
				if( p_ne_p( p2, l2->a) && p_ne_p( p2, l2->b) ){ // Это точка p2 ею надо резать l2
					replace_same_p2lines(l1, l2);
					split_line_by_p( l2, p2 );
					s++;
				}
			}
			if( p_eq_p( p1, l2->a) && p_eq_p( p2, l2->b) ){ // l2 внутри l1 => резать надо l1 на три куска а l2 не трогаем
				//printf("// l2 внутри l1 => резать надо l1 на три куска а l2 не трогаем\n");
				// находим точку которая не принадлежит обоим концам другого (l1) отрезка
				if( p_ne_p( p1, l1->a) && p_ne_p( p1, l1->b) ){ // Это точка p1 ею надо резать l1
					//printf("// p1 не совпадает с концами l1\n");
					replace_same_p2lines(l1, l2);
					split_line_by_p( l1, p1 );
					s++;
				}
				if( p_ne_p( p2, l1->a) && p_ne_p( p2, l1->b) ){ // Это точка p2 ею надо резать l1
					//printf("// p2 не совпадает с концами l1\n");
					replace_same_p2lines(l1, l2);
					split_line_by_p( l1, p2 );
					s++;
				}
			}
		}else if( ret==11 ){
			//if(debug && ret) printf("// Пересечение линий %i\n", ret);
			replace_same_p2lines(l1, l2);
			//split_line_by_p( l1, p2 );
		}
		//if( debug ) printf("// Пересечение линий done\n");
	}else if( (item1->type == OBJ_TYPE_ARC) && (item2->type == OBJ_TYPE_LINE) ){ // Пересечение дуги и линии
		Arc_t* arc = (Arc_t*) item1;
		Line_t* l = (Line_t*) item2;
		s = s + split_arc_by_line( arc, l );
//		printf("// Пересечение дуги с отрезком %i\n", s);
	}else if( (item1->type == OBJ_TYPE_LINE) && (item2->type == OBJ_TYPE_ARC) ){ // Пересечение линии и дуги
		Line_t* l = (Line_t*) item1;
		Arc_t* arc = (Arc_t*) item2;
		s = s + split_arc_by_line( arc, l );
//		printf("// Пересечение дуги с отрезком %i\n", s);
	}
//	printf("// расчёт окончен \n");
	return s;
}

// Пошинковать примитивы контуров точками их пересечений
int split_cont_by_cont(Cont_t* cont1, Cont_t* cont2, int debug){
	int s = 0;
	if( cont1 && cont2 && cont1->links.arr && cont2->links.arr ){
		//int n = cont1->links.count;
		if( 
			(cont1->xmax < cont2->xmin) ||
			(cont1->xmin > cont2->xmax) ||
			(cont2->xmax < cont1->xmin) ||
			(cont2->xmin > cont1->xmax) ||
			(cont1->ymax < cont2->ymin) ||
			(cont1->ymin > cont2->ymax) ||
			(cont2->ymax < cont1->ymin) ||
			(cont2->ymin > cont1->ymax)
		){
			return s;
		}else{
			for( int i=0; i < cont1->links.count; i++ ){
				Refitem_t* item1 = cont1->links.arr[i];
				if( item1 && is_seg(item1) ){
					//int m = cont2->links.count;
					for( int j = 0; j < cont2->links.count; j++ ){
						Refitem_t* item2 = cont2->links.arr[j];
						if( item2 ){
							s = s + split_item_by_item( item1, item2, debug );
							//if( debug && is_line(item1) && is_line(item2) ){
								//print_line( (Line_t*) item1 );
								//print_line( (Line_t*) item2 );
								//printf(" [%i, %i]  S = %i\n", i, j, s);
							//}
						}
					}
				}
			}
		}
	}
	return s;
}

/*
* Подсчёт показателя contcount для заданного примитива относительно заданного контура
*/
uint8_t is_prim_in_cont( Refitem_t* item, Cont_t* cont, int debug ){
	if(debug) printf("// -- is_prim_in_cont \n");
	int s = 0;
	int id = 0;
	if( item && cont ){
		double x1;
		double y1;
		Line_t* line1 = NULL;
		Arc_t* arc1 = NULL;
		if( item->type == OBJ_TYPE_LINE ){
			line1 = (Line_t*) item;
			middle( line1->a->x, line1->a->y, line1->b->x, line1->b->y, &x1, &y1 );
			id = line1->id;
		}else if( item->type == OBJ_TYPE_ARC ){
			arc1 = (Arc_t*) item;
			if( arc1->dir == 0 ){
				x1 = arc1->center.x+arc1->R;
				y1 = arc1->center.y;
			}else{
				if( !get_xy_of_arc_mid( (Arc_t*) arc1, &x1, &y1) ){
					return 0;
				}
			}
			id = arc1->id;
		}else{
			return 0;
		}

		if( cont && cont->links.arr ){
			double cont_max_x = cont->xmax + 1;
			double cont_max_y = cont->ymax + 1;
//			if( debug ){
//				printf("\n// -- checking by ray: \n");
//				svg_line( x1, y1, cont_max_x, cont_max_y, env, stdout );
//				printf("\n");
//			}
			for( int i=0; i < cont->links.count; i++ ){
				Refitem_t* peer = cont->links.arr[i];
				if( peer ){
					if(debug) printf("PRIM #%i ", i);
					double p1x;
					double p1y;
					double p2x;
					double p2y;
					Line_t* line2 = NULL;
					Arc_t* arc2 = NULL;
					int mode = 0;
					int ds = 0;
					if( peer->type == OBJ_TYPE_LINE ){
						line2 = (Line_t*) peer;
						if( line2->a && line2->b ){
							mode = xy_of_line_x_line( x1, y1, cont_max_x, cont_max_y, line2->a->x, line2->a->y, line2->b->x, line2->b->y, &p1x, &p1y, &p2x, &p2y );
							if(debug) printf(" is LINE%i cross mode = %i | (%f, %f) (%f, %f) (%f, %f) (%f, %f) ", line2->id, mode, x1, y1, cont_max_x, cont_max_y, line2->a->x, line2->a->y, line2->b->x, line2->b->y );
							if( mode == 2 ){
								if(debug) printf(" (+) ");
								ds++;
							}else if( (mode == 4) || (mode == 3)  || (mode == 10) ){
								if( xy_eq_xy(line2->a->x, line2->a->y, p1x, p1y) ){
									if(debug) printf(" [A] так как (%f, %f) (%f, %f) -->  %i", line2->a->x, line2->a->y, p1x, p1y, xy_eq_xy(line2->a->x, line2->a->y, p1x, p1y));
									// если попадаем на конец a отрезка, то считаем только если это его нижний край.
									ds++;
								}else if( xy_eq_xy(line2->b->x, line2->b->y, p1x, p1y) ){
									if(debug) printf(" [B] (skip) ");
									// если попадаем на конец b отрезка, то считаем только если это его нижний край.
								}else{
									if(debug) printf("Пропускаем так как (%f, %f)<>(%f, %f)<>(%f, %f) -->  %i %i ", line2->a->x, line2->a->y, p1x, p1y, line2->b->x, line2->b->y, xy_eq_xy(line2->a->x, line2->a->y, p1x, p1y), xy_eq_xy(line2->b->x, line2->b->y, p1x, p1y) );
								}
							};
							if( ds ){
								s = s + ds;
								//if( debugmode ) svg_point( p1x, p1y, 10, stdout );
							}
						}
					}else if( peer->type == OBJ_TYPE_ARC ){
						arc2 = (Arc_t*) peer;
						uint8_t r = xy_of_line_x_arc( x1, y1, cont_max_x, cont_max_y, (Arc_t*) arc2, &p1x, &p1y, &p2x, &p2y );
						mode = r;
						if( r == 2 ) r = 0;
						if( r == 3 ) r = 2;
						if( arc2->dir != 0 ){
							if(debug) printf(" is ARC%i cross mode = %i | (%f, %f) (%f, %f) (%f, %f) (%f, %f) ", arc2->id, mode, x1, y1, cont_max_x, cont_max_y, arc2->a->x, arc2->a->y, arc2->b->x, arc2->b->y );
							if( r == 1 ){
								if( is_xy_on_arc( arc2, p1x, p1y) ){
									//if( debugmode ) printf(" /+/ в т.(%f %f)", p1x, p1y );
									if( xy_eq_xy(arc2->a->x, arc2->a->y, p1x, p1y) ){
										//if( debugmode ) printf(" A ");
										// если попадаем на конец a отрезка, то считаем только если это его A конец
										//ds = ds + is_lower_edge( arc2->b,  arc2->a );
										if(debug) printf(" [A] (+) ");
										ds++;
									}else if( xy_eq_xy(arc2->b->x, arc2->b->y, p1x, p1y) ){
										if(debug) printf(" [skip B] ");
										//if( debugmode ) printf(" B ");
										// если попадаем на конец b отрезка, то считаем только если это его A конец
										//ds = ds + is_lower_edge( arc2->a,  arc2->b );
									}else{
										//if( debugmode ) printf(" P ");
										if(debug) printf(" (+) ");
										ds++;
									}
								}
							}else if( r==2 ){
								if( is_xy_on_arc( arc2, p1x, p1y) ){
									//if( debugmode ) printf(" /+/  в т.(%f %f)", p1x, p1y );
									if( xy_eq_xy( arc2->a->x, arc2->a->y, p1x, p1y ) ){
										//if( debugmode ) printf(" A ");
										// если попадаем на конец a отрезка, то считаем только если это его A конец
										//ds = ds + is_lower_edge( arc2->b,  arc2->a );
										if(debug) printf(" [A] (+) ");
										ds++;
									}else if( xy_eq_xy(arc2->b->x, arc2->b->y, p1x, p1y ) ){
										if(debug) printf(" [skip B] ");
										// если попадаем на конец b отрезка, то считаем только если это его A конец
										//if( debugmode ) printf(" B ");
										//ds = ds + is_lower_edge( arc2->a,  arc2->b );
									}else{
										//if( debugmode ) printf(" P ");
										if(debug) printf(" (+) ");
										ds++;
									}
									// if( debugmode && (ds>0) ) svg_point( p1x, p1y, 10 );
								}
								if( is_xy_on_arc( arc2, p2x, p2y) ){
									//if( debugmode ) printf(" /+/ и в т.(%f %f)", p2x, p2y );
									if( xy_eq_xy( arc2->a->x, arc2->a->y, p2x, p2y ) ){
										// если попадаем на конец a отрезка, то считаем только если это его A конец
										//if( debugmode ) printf(" A ");
										//ds = ds + is_lower_edge( arc2->b,  arc2->a );
										if(debug) printf(" [A] (+) ");
										ds++;
									}else if( xy_eq_xy( arc2->b->x, arc2->b->y, p2x, p2y ) ){
										if(debug) printf(" [skip B] ");
										// если попадаем на конец b отрезка, то считаем только если это его A конец
										//if( debugmode ) printf(" B ");
										//ds = ds + is_lower_edge( arc2->a,  arc2->b );
									}else{
										//if( debugmode ) printf(" P ");
										if(debug) printf(" (+) ");
										ds++;
									}
									// if( debugmode && (ds>0) ) svg_point( p2x, p2y, 10 );
								}
							}
						}else{
							ds = r;
						}
						if( ds ) s = s + ds;
					}
					if(debug) printf("\n");
				}
			}
		}
	}
	if( debug && ((s & 1)==1) ){
		printf("Итог по контуру %i S=%i \n\n\n", cont->num, s );
	}
	return ((s & 1)==1)?1:0;
}

/*
* Проверка попадания координат на контур при слиянии контуров.
* Предполагается, что координаты - это середина либо дуги, либо отрезка (определяется параметром type)
* Точка будет считаться попавшей на контур, только если заданный тип соответствует элементу контура, на который она попала.
* Только в этом случае сегмент будет совпадать с контуром и должен быть убран из результируещего контура.
*/
uint8_t is_xy_on_cont( double x, double y, Obj_Type_t type, Cont_t* cont ){
	if( cont && cont->links.arr ){
		for( int i=0; i < cont->links.count; i++ ){
			Refitem_t* peer = cont->links.arr[i];
			if( peer ){
				if(peer->type == type){
					if( type == OBJ_TYPE_LINE ){
						Line_t* line = (Line_t*) peer;
						if( is_xy_on_line( line->a->x, line->a->y, line->b->x, line->b->y, x, y ) ) return 1;
					}else if( type == OBJ_TYPE_ARC ){
						Arc_t* arc = (Arc_t*) peer;
						if( is_xy_on_arc( arc, x,  y) ) return 1;
					}
				}
			}
		}
		return 0;
	}
}

/*
* Помещяем вторую часть (новый кусок) сразу после старого
*/
void put_after( Refitem_t* newitem, Cont_t* cont, Refitem_t* olditem ){
	if( cont->links.arr && (cont->links.count > 0) ){
		int newitem_idx = -1;
		int olditem_idx = -1;
		for( int i = cont->links.count-1; i>=0; i--){
			if( newitem == cont->links.arr[i] ){
				newitem_idx = i;
				break;
			}
		}
		if( newitem_idx >= 0 ){ // проверили новый кусок - он существует и его индекс - newitem_idx
			for( int i = 0; i < cont->links.count; i++){
				if( olditem == ((Refitem_t*) cont->links.arr[i]) ){
					olditem_idx = i;
					break;
				}
			}
			if( olditem_idx >= 0 && (olditem_idx < newitem_idx) ){ // проверили новый кусок - он существует и его индекс - newitem_idx
				int i = newitem_idx;
				while( i > (olditem_idx+1) ){
					Refitem_t* tmp = cont->links.arr[ i ];
					cont->links.arr[ i ] = cont->links.arr[ i-1 ];
					cont->links.arr[ i-1 ] = tmp;
					i--;
				}
			}
		}
	}
}


#include "./cont_reorder.c"
