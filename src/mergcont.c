#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>
#include <math.h>

#include "../include/top.h"
#include "../include/reflist.h"
#include "../include/point.h"
#include "../include/line.h"
#include "../include/arc.h"
#include "../include/cont.h"
#include "../include/context.h"

#include "../include/excellon.h"
#include "../include/mergcont.h"
#include "../include/export.h"

struct {
	Refitem_t* seg;
	double angle;
} SegList_t;


double angle_factor(Point_t* from, Point_t* at, Point_t* to){
	double v_in_x = at->x - from->x;
	double v_in_y = at->y - from->y;
	double v_out_x = to->x - at->x;
	double v_out_y = to->y - at->y;
	double angle_in  = atan2( v_in_y, v_in_x );
	double angle_out = atan2( v_out_y, v_out_x );
	double diff = angle_out - angle_in;
	if( diff > M_PI )   diff -= 2 * M_PI;
	if( diff <= -M_PI ) diff += 2 * M_PI;  // не строгое неравенство для π
	return diff;
}

Vector_t line2vect( Line_t* line, Point_t* p, uint8_t inout ){
	Vector_t v;
	int k = 1;
	if( line->b == p ){ // Входящая
		if(inout==0) k=-k;
	}else{ // Исходящая
		if(inout==1) k = -k;
	}
	v.dx = (line->b->x - line->a->x) * k;
	v.dy = (line->b->y - line->a->y) * k;
	v.k = 0; // параметр кривизны
	return v;
}

Vector_t item2vect( Refitem_t* item, Point_t* p, uint8_t inout ){
	if( is_line(item) ){
		Line_t* line = (Line_t*) item;
		return line2vect( line, p, inout );
	}else if(is_arc(item)){
		Arc_t* arc = (Arc_t*) item;
		if( arc->dir != 0 ){
			return arc2vect( arc, p, inout );
		}else{
			fprintf(stderr, "Invalid arc state (arc->dir=0) in item2vect\n");
			exit(1);
		}
	}
}

double angle_between_vectors( Vector_t v1, Vector_t v2 ){
	double diff = atan2( v2.dy, v2.dx ) -  atan2( v1.dy, v1.dx ) ;
	if( diff > M_PI )   diff -= 2 * M_PI;
	if( diff <= -M_PI ) diff += 2 * M_PI;
	return diff;
}

Factor_t factor_between_vectors( Vector_t v1, Vector_t v2 ){
	Factor_t f = {0,0};
	double diff = angle_between_vectors( v1, v2 );
	f.dk = v2.k - v1.k;
	if( fabs(fabs(diff) - M_PI) < epsilon ){
		diff = M_PI;
		if( (-v2.k) < v1.k ){
			diff = -diff;
		}
	}
	f.dt = diff;
	return f;
}

/*
* 1 если left < rigth
*/
uint8_t factor_lt_factor( Factor_t left, Factor_t right ){
	if( fabs(left.dt - right.dt) < epsilon ){
		// сравниваем кривизну
		return (left.dk < right.dk)?1:0;
	}else{
		// сравниваем угол
		return (left.dt < right.dt)?1:0;
	}
}

/*
* 1 если left > rigth
*/
uint8_t factor_gt_factor( Factor_t left, Factor_t right ){
	return factor_lt_factor( left, right )?0:1;
}

Vector_t arc2vect(Arc_t* arc, Point_t* p, uint8_t inout){
    double Rx = p->x - arc->center.x;
    double Ry = p->y - arc->center.y;
    Vector_t v;
    if( arc->dir == 1  ){
        v.dx = -Ry;
        v.dy =  Rx;
    }else{
        v.dx =  Ry;
        v.dy = -Rx;
    }
    if( (p == arc->b && inout == 0) ||  (p == arc->a && inout == 1) ){
        v.dx = -v.dx;
        v.dy = -v.dy;
    }
    // вычисляем кривизну k
    // cross(T, C-P) = dx*(Ry) - dy*(Rx)
    // но Rx,Ry уже C-P
    double cross = v.dy * Rx - v.dx * Ry;
    v.k = (cross >= 0 ? 1.0 : -1.0) / arc->R;
    return v;
}

// Ищем первый неохваченный областью сегмент.
Refitem_t* get_first_free( Refholder_t* souce_list, char* side ){
	Refholder_t* curr = souce_list;
	while( curr ){
		if( curr->refitem ){
			if( curr->refitem->type == OBJ_TYPE_CONTUR ){
				Cont_t* cont = (Cont_t*) curr->refitem;
				if( cont->links.arr && (cont->links.count > 0) ){
					for( int i=0; i < cont->links.count; i++ ){
						Refitem_t* item = cont->links.arr[i];
						if( is_seg(item) && !item->inshadow ){
							if( strcmp(side, "r")==0 ){
								if( !item->cont_r ) return item;
							}else{
								if( !item->cont_l ) return item;
							}
						}
					}
				}else{
					printf("Пустой контур!\n");
				}
			}else{
				printf("В списке НЕ контур!\n");
			}
		}else{
			printf("Пустой холдер!\n");
		}
		curr = curr->next;
	}
	return NULL;
}

/*
* Возвращает точку a|b на конце сегмента, независимо от типа сегмента
*/
Point_t* get_seg_end(Refitem_t* item, char* endname ){
	if( item ){
		if( is_line(item) ){
			Line_t* line = (Line_t*) item;
			if( strcmp(endname, "a") == 0 ){
				return line->a;
			}else if( strcmp(endname, "b") == 0 ){
				return line->b;
			}
		}else if( is_arc(item) ){
			Arc_t* arc = (Arc_t*) item;
			if( arc->dir !=0 ){
				if( strcmp(endname, "a") == 0 ){
					return arc->a;
				}else if( strcmp(endname, "b") == 0 ){
					return arc->b;
				}
			}
		}
	}
	return NULL;
};

/*
* Подсчёт количества сегментов не в тени в заданной точке
*/
int count_point_links( Point_t* p ){
	int s = 0;
	if( p && p->links.arr && (p->links.count>0) ){
		for( int i = 0; i < p->links.count; i++ ){
			Refitem_t* item = p->links.arr[i];
			if( (!item->inshadow) && is_seg(item) ) s++; // если не в тени и если это дуга или отрезок, то считаем
		}
	}
	return s;
}

/*
* Поиск следующего сегмента в точке p относительно сегмента from
*/
Refitem_t* get_next_seg( Refitem_t* from, Point_t* p, int debug ){
	Refitem_t* selected = NULL;
	double angle = 0;
	Factor_t factor;
	if( p && p->links.arr && (p->links.count>0) ){
		for( int i = 0; i < p->links.count; i++ ){
			Refitem_t* item = p->links.arr[i];
			if( !item->inshadow && is_seg(item) && (item != from) ){
				// Берём угол между двума сегментами - from и item.
				// При этом целевой конец для from берём тот который совпадает с исследуемой точкой (то есть во входящем направлении)
				// а для item берём конец, котороый НЕ совпадают с исследуемой точкой (то есть в исходящем направлении)
//				char* cn1 = endname(from, p, 1);
//				char* cn2 = endname(item, p, 0);
				//double a = angle_between_vectors( item2vect(from, p, 1 ), item2vect(item, p, 0 ) );
				Factor_t f = factor_between_vectors( item2vect(from, p, 1 ), item2vect(item, p, 0 ) );
/*
				if(debug){
					printf("\n [T] " );
					print_item(item);
					Vector_t v1 = item2vect(from, p, 1 );
					printf("v1 = (v.dx, v.dy) = (%f, %f) ", v1.dx, v1.dy );
					Vector_t v2 = item2vect(item, p, 0 );
					printf("v2 = (v.dx, v.dy) = (%f, %f) ", v2.dx, v2.dy );
					printf("\n");
					printf(" dt = %f\n", f.dt);
					printf(" dk = %f\n", f.dk);
				}
*/
				if( !selected ){ // если это первый сегмент, то берём его
					selected = item;
					factor = f;
				}else if( factor_lt_factor(f, factor) ){ // если это сегмент с наименьшим фактором, запоминаем его.
					selected = item;
					factor = f;
				}
			}
		}
	}
	return selected;
}

int obhod_to_dir( Refitem_t* start, Cont_t* cont,  char* _side ){
	char* side = _side;
	//printf("\nновый контур _side=%s start = %p\n", side, start);
	Refitem_t* item = start;
	while(1){
		if( strcmp( side, "r" )==0 ){
			add_item2cont_r( item, cont );
		}else{
			add_item2cont_l( item, cont );
		}
		Point_t* p = NULL;
		if( strcmp( side, "r" )==0 ){
			p = get_seg_end( item, "b" );
		}else{
			p = get_seg_end( item, "a" );
		}
		int cpl = count_point_links( p );
		if( cpl > 1 ){ // если не упёрлись в конец контура
			// здесь надо выбрать следующий item
			item = get_next_seg( item, p, 0 );
			if( item == start ) break;
			side = ( get_seg_end( item, "a" ) == p)?"r":"l";
		}else{ // иначе  выходим (так вообще-то быть не должно)
			break;
		}
		
	}
	return 0;
}

// сброс признака видимости для всех областей (контуров, face)
void clea_areas_visabiliny( Refholder_t* list ){
	Refholder_t* curr = list;
	while(curr){
		Cont_t* cont = (Cont_t*) curr->refitem;
		cont->mincc = 0;
		curr = curr->next;
	}
}

int find_determinated_areas( Refholder_t* list ){
	Refholder_t* curr = list;
	while(curr){
		Cont_t* cont = (Cont_t*) curr->refitem;
		if( cont->mincc == 0 ){
			int cont_r = 0;
			int cont_l = 0;
			for( int i=0; i<cont->links.count; i++ ){
				if( is_seg( cont->links.arr[i] ) ){
					Refitem_t* seg = cont->links.arr[i];
					if( seg->cont_r == cont ){
						cont_r++;
					}else if( seg->cont_l == cont ){
						cont_l++;
					}
				}
			}
			if( (cont_r == 0) && (cont_l>0)  ) cont->mincc = -1;
			if( (cont_l == 0) && (cont_r>0) ) cont->mincc = 1;
			if( cont->mincc != 0 ) printf("Determinated Contour %i\n", cont->mincc);
		}
		curr = curr->next;
	}
}

int calc_areas_visabiliny_iterate( Refholder_t* list ){
	printf("calc_areas_visabiliny_iterate:\n");
	Refholder_t* curr = list;
	int s = 0;
	while(curr){
		Cont_t* cont = (Cont_t*) curr->refitem;
		printf("Contour:\n");
		if( cont->mincc == 0 ){
			for( int i=0; i<cont->links.count; i++ ){
				if( is_seg( cont->links.arr[i] ) ){
					Refitem_t* seg = cont->links.arr[i];
					Line_t* l = (Line_t*) seg;
					printf("Line %i ", l->id);
					if( seg->cont_l == cont ){
						if( seg->cont->dir == 1 ){
							cont->mincc = -1;
							printf(" break " );
							break;
						}
					}
				}
			}
			printf(" cont->mincc = %i ", cont->mincc );
			if( cont->mincc == 0 ){
				printf(" no ok ");
				for( int i=0; i<cont->links.count; i++ ){
					if( is_seg( cont->links.arr[i] ) ){
						Refitem_t* seg = cont->links.arr[i];
						Line_t* l = (Line_t*) seg;
						printf("Line %i  ", l->id);
						if( seg->cont_r == cont ){
							if( seg->cont->dir == -1 ){
								cont->mincc = 1;
								printf(" break " );
								break;
							}
						}
					}
				}
			}
		}
		if( cont->mincc == 0 ){ // после всего неопределённая закрашенность может быть только у внешнего контура. Примем его незакрашенным.
			cont->mincc = -1;
		}
		s+=(cont->mincc == 0)?1:0; // считаем количество областей с неопределённым признаком видимости

		printf("cont->mincc = %i\n\n", cont->mincc );

		curr = curr->next;
	}
	return s;
}


int calc_areas_visabiliny_iterate2( Refholder_t* list ){
	printf("calc_areas_visabiliny_iterate:\n");
	Refholder_t* curr = list;
	int s = 0;
	while(curr){
		Cont_t* cont = (Cont_t*) curr->refitem;
		printf("Contour:\n");
		if( cont->mincc == 0 ){
			int l_count = 0;
			int r_count = 0;
			for( int i=0; i<cont->links.count; i++ ){
				if( is_seg( cont->links.arr[i] ) ){
					Refitem_t* seg = cont->links.arr[i];
					Line_t* l = (Line_t*) seg;
					print_line(l);
					if( seg->cont_r == cont ){
						if( seg->cont_l->mincc == -1 ){
							l_count++;
						}else{
							r_count++;
						}
					}
					if( seg->cont_l == cont ){
						if( seg->cont_r->mincc == 1 ){
							r_count++;
						}else{
							l_count++;
						}
					}
					printf("| %s | l_count:%i;  r_count:(%i) mincc:%i \n" ,(seg->cont_r == cont)?"R":"L", l_count, r_count, cont->mincc);
				}
			}
			if( (l_count == 0) && (r_count>0) ) cont->mincc = 1;
			if( (l_count>0) ) cont->mincc = -1;
/*
			for( int i=0; i<cont->links.count; i++ ){
				if( is_seg( cont->links.arr[i] ) ){
					Refitem_t* seg = cont->links.arr[i];
					Cont_t* nbh = NULL;
					if( seg->cont_r == cont ){
						nbh = seg->cont_l;
					}else{
						nbh = seg->cont_r;
					}
					if( nbh->mincc == -1 ) cont->mincc = 1;
				}
			}
			for( int i=0; i<cont->links.count; i++ ){
				if( is_seg( cont->links.arr[i] ) ){
					Refitem_t* seg = cont->links.arr[i];
					Line_t* l = (Line_t*) seg;
					print_line(l);
					printf("| %s | l_count:%i;  r_count:(%i) mincc:%i \n" ,(seg->cont_r == cont)?"R":"L", l_count, r_count, cont->mincc);
				}
			}
*/
		}

		printf("cont->mincc = %i\n\n", cont->mincc );
		s+=(cont->mincc == 0)?1:0; // считаем количество областей с неопределённым признаком видимости
		curr = curr->next;
	}
	return s;
}

void calc_areas_visabiliny( Refholder_t* list ){
	clea_areas_visabiliny( list );
	find_determinated_areas( list );
	int c=2;
	while(c){
		if( !calc_areas_visabiliny_iterate( list ) ) break;
		c--;
	} // крутим цикл пока есть хотя бы одна область с неопределённой видимостью.
}

/*
void calc_areas_visabiliny( Refholder_t* list ){
	Refholder_t* curr = list;
	while(curr){
		Cont_t* cont = (Cont_t*) curr->refitem;
		printf("Contour:\n");
		int cont_r = 0;
		int cont_l = 0;
		for( int i=0; i<cont->links.count; i++ ){
			if( is_seg( cont->links.arr[i] ) ){
				Refitem_t* seg = cont->links.arr[i];
				Line_t* l = (Line_t*) seg;
				print_line(l);
				if( seg->cont_r == cont ) cont_r++;
				if( seg->cont_l == cont ) cont_l++;
				printf("l:(%i), r:(%i) \n", ( seg->cont_l == cont ), ( seg->cont_r == cont ));
			}
		}	
//		cont->mincc = ( (cont_l > 0) && (cont_r == 0) )?0:1;
        cont->mincc = 0;
		if( cont_r == 0 ) cont->mincc = 1;
		if( cont_l == 0 ) cont->mincc = -1;
		//cont->mincc = ( (cont_l > 0) )?0:1;
		//printf("cont->mincc = %i\n\n", cont->mincc );
		curr = curr->next;
	}
*/

/*
	curr = list;
	while(curr){
		Cont_t* cont = (Cont_t*) curr->refitem;
		if( cont->mincc != 0 ){
			
		}
		curr = curr->next;
	}
*/



void find_areas( Refholder_t* souce_list ){
	Refholder_t* list = NULL;
	marking_of_imposed_by_list( souce_list );
	Cont_t* next_cont;
	Refitem_t* curr;
	while(1){
		next_cont = create_cont();
		curr = get_first_free( souce_list, "r" );
		if( !curr ) break;
		//printf("Найден свободный ITEM (R):\n");
		//print_item( curr );
		int count_of_items = obhod_to_dir( curr, next_cont, "r" );
		push2list( (Refitem_t*) next_cont, &list );
		//printf("Контур r ok: %p\n", list);
	}
	remove_cont(&next_cont);
	while(1){
		next_cont = create_cont();
		curr = get_first_free( souce_list, "l" );
		if( !curr ) break;
		//printf("Найден свободный ITEM (L):\n");
		//print_item( curr );
		int count_of_items = obhod_to_dir( curr, next_cont, "l" );
		push2list( (Refitem_t*) next_cont, &list );
		//printf("Контур l ok %p:\n", list);
	}
	remove_cont( &next_cont );
	printf("\n\nРасчёт окончен!\n\n");
	calc_areas_visabiliny( list );
	Refholder_t* cur = souce_list;
	while(cur){
		Cont_t* cont = (Cont_t*) cur->refitem;
		for( int i=0; i<cont->links.count; i++ ){
			if( is_seg( cont->links.arr[i] ) ){
				Refitem_t* item = cont->links.arr[i];
				//Line_t* l = (Line_t*) item;
				//print_line(l);
				//printf("inshadow: %i; item->cont_r->mincc=%i; item->cont_l->mincc=%i; item->contcount: %i\n", item->inshadow, item->cont_r->mincc, item->cont_l->mincc, item->contcount);
				if( item->inshadow ){
					//printf("| inshadow: %i; ", item->inshadow);
					item->contcount = 1;
				}else{
					//printf("| item->cont_r->mincc=%i; ", item->cont_r->mincc);
					//printf("| item->cont_l->mincc=%i; ", item->cont_l->mincc);
					if( item->cont_r->mincc != item->cont_l->mincc ){
						item->contcount = 0;
					}else{
						item->contcount = 1;
					}
				}
				//printf("|  item->contcount=%i; ",  item->contcount);
				//printf("\n");
			}
		}
		cur = cur->next;
	}
	remove_conts_by_list( &list );
}

