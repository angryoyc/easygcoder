#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#include "../include/top.h"
#include "../include/reflist.h"
#include "../include/point.h"
#include "../include/cont.h"
#include "../include/line.h"
#include "../include/arc.h"
#include "../include/geom.h"
#include "../include/context.h"
#include "../include/mergcont.h"
#include "../include/excellon.h"
#include "../include/export.h"

#define MAX_CONTEXTS 20
#define MAX_NAME_LEN 32

Context_t* current_context = NULL;
Context_t* context_pool[10] = {NULL};
static int context_count = 0;

int context_onunlink( Refitem_t* self, Refitem_t* ref ){
	if( self == ref ) return 0;
	return 1; // можно удалять точки
}

/** Создает и регистрирует новый контекст. */
Context_t* create_context(const char *name){
	if(context_count >= MAX_CONTEXTS){
		fprintf(stderr, "Error: Context pool full.\n");
		return NULL;
	}
	for( int i=0; i<context_count; i++){
		if( strcmp(context_pool[i]->name, name) == 0 ) return context_pool[i];
	}
	int n = sizeof(Context_t);
	Context_t* new_ctx = (Context_t*) malloc(n);
	if (!new_ctx) return NULL;
	memset(new_ctx, 0x00, n);
	new_ctx->type = OBJ_TYPE_CONTEXT;
	new_ctx->onunlink = context_onunlink;
	strncpy(new_ctx->name, name, MAX_NAME_LEN - 1);
	new_ctx->name[MAX_NAME_LEN - 1] = '\0';
	new_ctx->xmax = (double) -1000000;
	new_ctx->xmin = (double) 1000000;
	new_ctx->ymax = (double) -1000000;
	new_ctx->ymin = (double) 1000000;
	context_pool[context_count++] = new_ctx;
	current_context = new_ctx;
	linkobj2obj(new_ctx, new_ctx);
	return new_ctx;
}

/** Выбирает текущий контекст **/
Context_t* select_context(const char *name) {
	for (int i = 0; i < context_count; ++i) {
		if (strcmp(context_pool[i]->name, name) == 0) {
			current_context = context_pool[i];
			return current_context;
		}
	}
	fprintf(stderr, "Error: Context '%s' not found.\n", name);
	return NULL;
}

/** получаем текущий контекст (если еще никакакого нет, то создаём новый "common") **/
Context_t* get_context(){
	if(current_context == NULL){
		if( create_context("common") ){
			if( select_context("common") ) return current_context;
		}
	}
	return current_context;
}

/** Удаляет контекст по имени и освобождает всю связанную память. **/
int free_context_by_name(const char *name){
	if( !name || context_count == 0 ) return -1;
	for( int i = 0; i < context_count; ++i ){
		if( strcmp(context_pool[i]->name, name) == 0 ){
			Context_t* ctx = context_pool[i];
			if( current_context == ctx ) current_context = NULL; // если это текущий контекст — сбросить
			if( ctx->links.count > 0){
				for( int i = ctx->links.count-1; i >= 0; i-- ){
					Refitem_t* item  = ctx->links.arr[i];
					if( item->type == OBJ_TYPE_POINT ){
						remove_p( (Point_t**) &item );
					}else if( item->type == OBJ_TYPE_LINE ){
						remove_line( (Line_t**) &item );
					}else if( item->type == OBJ_TYPE_ARC ){
						remove_arc( (Arc_t**) &item );
					}
				}
			}
			purge_obj( (Refitem_t**) &ctx );
			ctx->onunlink = NULL;
			purge_obj( (Refitem_t**) &ctx );
			if( ctx == NULL ){
				for( int j = i; j < context_count - 1; ++j ){
					context_pool[j] = context_pool[j + 1];
				}
				context_pool[context_count] = NULL;
				if( context_count>0 ) context_count--;
				return 0;
			} else {
				fprintf(stderr, "Warning: Context '%s' has live references and was not freed.\n", name);
				return 1;
			}
		}
	}
	fprintf(stderr, "Error: Context '%s' not found.\n", name);
	return -1;
}


void context_accept_extrem(Context_t* ctx, double xmin, double xmax, double ymin, double ymax){
	ctx->xmin = fminf(ctx->xmin, xmin);
	ctx->xmax = fmaxf(ctx->xmax, xmax);
	ctx->ymin = fminf(ctx->ymin, ymin);
	ctx->ymax = fmaxf(ctx->ymax, ymax);
}

/*
* Подсчёт границ контекста (макс. и мин. x, y)
*/
void context_get_bounds(){
	Context_t* ctx = get_context();
	if( ctx->links.arr ){
		for( int i=0; i < ctx->links.count; i++ ){
			Refitem_t* item1 = ctx->links.arr[i];
			if( item1->type == OBJ_TYPE_CONTUR ){
				Cont_t* cont = (Cont_t*) item1;
				cont->xmax = -1000000;
				cont->xmin = 1000000;
				cont->ymax = -1000000;
				cont->ymin = 1000000;
				cont_get_bounds( cont );
				context_accept_extrem( ctx, cont->xmin, cont->xmax, cont->ymin, cont->ymax);
			}
		}
	}
}

/*
* Разбиваем все контуры контекста точками пересечений
*/
Refholder_t* split_all(int debug){
	int s = 0;
	Context_t* ctx = get_context();
	context_get_bounds();            // Подсчёт границ всех контуров и границ всего контекста
	Refholder_t* list = NULL;
	if( ctx->links.arr ){
		for( int i=0; i < ctx->links.count; i++ ){
			Refitem_t* item1 = ctx->links.arr[i];
			if( is_cont(item1) ){
				for( int j = i+1; j < ctx->links.count; j++ ){
					Refitem_t* item2 = ctx->links.arr[j];
					if( is_cont(item2) ){
						int cont_s = split_cont_by_cont( (Cont_t*) item1, (Cont_t*) item2, debug );
						if( cont_s > 0 ){
							//printf("cont_s  = %i \n", cont_s);
							push2list( (Refitem_t*) item1, &list );
							push2list( (Refitem_t*) item2, &list );
							s = s + cont_s;
						}
					}
				}
			}
		}
	}
	return list;
}

uint8_t does_it_have_a_common_area(Cont_t* cont1, Cont_t* cont2){
	if(cont1->xmax < cont2->xmin) return 0;
	if(cont1->xmin > cont2->xmax) return 0;
	if(cont1->ymax < cont2->ymin) return 0;
	if(cont1->ymin > cont2->ymax) return 0;
	return 1;
}

/*
* Вычисление показателя contcount
* (количество вхождений середины примитива в чужие контуры, отличные от контура примитива)
* для всех примитивов в контексте
*/
void calc_contcount4all( int new_cont_id, int debug ){
	clean_contcount( NULL );         // Сбрасываем показатель contcount для всех примитивов контекста
	context_get_bounds();            // Подсчёт границ всех контуров и границ всего контекста
	Context_t* ctx = get_context();  //
	if( ctx->links.arr ){
		for( int i=0; i < ctx->links.count; i++ ){
			Refitem_t* item1 = ctx->links.arr[i];
			if( item1->type == OBJ_TYPE_CONTUR ){
				Cont_t* cont1 = (Cont_t*) item1;
				cont1->mincc = -1;
				for( int j = 0; j < cont1->links.count; j++ ){
					Refitem_t* prim = ((Refitem_t*) cont1->links.arr[j]);
					if( (prim->type == OBJ_TYPE_LINE) || (prim->type == OBJ_TYPE_ARC) ){
						int flag = 0;
						prim->contcount1 = 0;
						prim->contcount2 = 0;
						Cont_t* cont2 = NULL;
						//int flag = 0;
						for( int k=0; k < ctx->links.count; k++ ){
							if( ((Refitem_t*) ctx->links.arr[k])->type == OBJ_TYPE_CONTUR ){
								cont2 = (Cont_t*) ctx->links.arr[k];
								if( (cont1 != cont2) && (!new_cont_id || (cont1->num==new_cont_id || cont2->num==new_cont_id) ) ){
									uint8_t in_cont1 = does_it_have_a_common_area(cont1, cont2);
									uint8_t in_cont2 = is_prim_in_cont( prim, cont2, flag );
									if( in_cont1 && in_cont2 ){
										if( cont2->dir == -1 ){
											prim->contcount1 = prim->contcount1 + 1;
										}else{
											prim->contcount2 = prim->contcount2 + 1;
										}
										if( debug ){
											//if( prim->contcount1 + prim->contcount2 >0 ) flag = 1;
											/*
											if( cont1->num == 2807 && cont2->num==2810 ){
												flag = 1;
											}
											*/
											if( prim->type == OBJ_TYPE_LINE ){
												Line_t* l = (Line_t*) prim;
												if( l->id == 170 ) flag = 1;
											}


											/*
											if( flag ){
												if( prim->type == OBJ_TYPE_ARC ){
													Arc_t* a = (Arc_t*) prim;
													print_arc(a);
												}
												if( prim->type == OBJ_TYPE_LINE ){
													Line_t* l = (Line_t*) prim;
													print_line(l);
												}
												prim->contcount1 = 0;
												prim->contcount2 = 0;
												prim->contcount = 0;
												
												in_cont2 = is_prim_in_cont( prim, cont2, 1 );
												if( in_cont2 ){
													if( cont2->dir == -1 ){
														prim->contcount1 = prim->contcount1 + 1;
													}else{
														prim->contcount2 = prim->contcount2 + 1;
													}
												}
												if( prim->contcount1 + prim->contcount2 >0 ){
													printf("попадание в контур: %i \n", cont2->num);
												}else{
													printf("НЕ попадание в контур: %i \n", cont2->num);
												}
												printf("В контуров По часовой: %i\n", prim->contcount1);
												printf("В контуров Против часовой: %i\n", prim->contcount2);
											}
											*/
										}
									}
								}
							}
						}
						if( flag ){
							printf("ИТОГ по PRIM: %i %i", prim->contcount1, prim->contcount2);
							//printf("В контуров По часовой: %i\n", prim->contcount1);
							//printf("В контуров Против часовой: %i\n", prim->contcount2);
							
						}
						if( prim->contcount2 < prim->contcount1){
							if( prim->contcount2 > 0 ){
								prim->contcount1 = prim->contcount1 - prim->contcount2;
								prim->contcount2 = 0;
							}
						}else{
							if( prim->contcount1 > 0 ){
								prim->contcount2 = prim->contcount2 - prim->contcount1;
								prim->contcount1 = 0;
							}
						}
						if( cont1->dir == -1 ){
							prim->contcount = ( (prim->contcount2 + prim->contcount1) > 0)?1:0;
						}else{
							prim->contcount = 1;
							prim->contcount = prim->contcount & ((( prim->contcount1 ) > 0)?(~1):(~0));
							prim->contcount = prim->contcount | ((( prim->contcount2 ) > 0)?1:0);
							prim->contcount = prim->contcount | ((( prim->contcount1 ) > 1)?1:0);
						}
						if( flag) printf(" prim->contcount =  %i\n", prim->contcount );
					}
				}
			}
		}
	}
}

/*
* Поиск контура начиная с заданного примитива
*/
uint8_t obhod( Refitem_t* _start, Cont_t* cont ){
//	printf("OBHOD\n");
	uint8_t ret = 0;
	Refitem_t* next = _start;
	Refitem_t* curr = NULL;
	while( next ){
		if( next->type == OBJ_TYPE_LINE ){
			Line_t* line = (Line_t*) next;
//			printf("LINE #%i\n", line->id);
		}
		if( next->type == OBJ_TYPE_ARC ){
			Arc_t* arc = (Arc_t*) next;
//			printf("ARC #%i\n", arc->id);
		}
		if( add_item2cont( next, cont )){
			ret++;
		}
		Point_t* p = NULL;
		curr = next;
		next = NULL;
		if( curr->type == OBJ_TYPE_LINE ){
			p = ( (Line_t*) curr )->b;
		}else if( curr->type == OBJ_TYPE_ARC ){
			Arc_t* arc = (Arc_t*) curr;
			if( arc->dir != 0 ){
				p = arc->b;
			}
		}
		if( p && p->links.arr ){
			for( int j=0; j < p->links.count; j++ ){
				if( p->links.arr[j] ){
					Refitem_t* item = p->links.arr[j];
					if( (item->type == OBJ_TYPE_LINE) || (item->type == OBJ_TYPE_ARC) ){
						if( item == curr ) continue;
						if( item == _start ) break;
						if( !item->cont){
							if( item->type == OBJ_TYPE_LINE ){
								//printf("Требуется переворот линии в контуре\n");
								Line_t* line = (Line_t*) item;
								if( p == line->b ){
									//printf("Требуется переворот линии в контуре\n");
									Point_t* tmp = line->a;
									line->a = line->b;
									line->b = tmp;
								}
							}
							if( item->type == OBJ_TYPE_ARC ){
								Arc_t* arc = (Arc_t*) item;
								if( p == arc->b ){
									//printf("Требуется переворот дуги в контуре\n");
									// print_p(p);
									if( arc->dir != 0 ){
										Point_t* tmp = arc->a;
										arc->a = arc->b;
										arc->b = tmp;
										arc->dir = -arc->dir;
									}
								}
							}
							next = item;
							break;
						}
					}
				}
			}
		}
	}
	return ret;
}

uint8_t obhod2( Refitem_t* _next, Cont_t* cont ){
	Refitem_t* next = _next;
	uint8_t ret = 0;
	if( next ){
		add_item2cont( next, cont );
		ret = 1;
		Point_t* p = NULL;
		if( next->type == OBJ_TYPE_LINE ){
			p = ( (Line_t*) next )->b;
		}else if( next->type == OBJ_TYPE_ARC ){
			Arc_t* arc = (Arc_t*) next;
			if( arc->dir != 0 ){
				p = arc->b;
			}
		}
		if( p ){
			if( p->links.arr ){
				for( int j=0; j < p->links.count; j++ ){
					Refitem_t* item = p->links.arr[j];
					if( item != next ){
						if( (item->type == OBJ_TYPE_LINE) || (item->type == OBJ_TYPE_ARC) ){
							if( is_linked( item, (Refitem_t*) cont ) > 0 ){
								return ret;
							}else{
								add_item2cont( item, cont );
								if( (item->type == OBJ_TYPE_LINE) && (p == ( (Line_t*) item )->b) ){
									Line_t* line = (Line_t*) item;
									Point_t* tmp = line->a;
									line->a = line->b;
									line->b = tmp;
								}
								if( (item->type == OBJ_TYPE_ARC) && (p == ( (Arc_t*) item )->b) ){
									Arc_t* arc = (Arc_t*) item;
									if( arc->dir != 0 ){
										Point_t* tmp = arc->a;
										arc->a = arc->b;
										arc->b = tmp;
										arc->dir = -arc->dir;
									}
								}
								ret = 1 + obhod2( item, cont );
							}
						}
					}
				}
			}
		}
	}
	return ret;
}

/*
* Поиск замкнутого контура начиная с первого "свбодного" примитива
*/
uint8_t find_closed_cont( Cont_t* cont ){
	Context_t* ctx = get_context();
	if( ctx->links.arr ){
		//printf("Элементов в контексте %s всего: .. %i\n",ctx->name, ctx->links.count );
		for( int i=0; i < ctx->links.count; i++ ){
			Refitem_t* item1 = ctx->links.arr[i];
			int id = 0;
			if( item1->type == OBJ_TYPE_LINE ){
				id = ((Line_t*) item1)->id;
				//printf("найден отрезок %i\n", id);
			}
			if( item1->type == OBJ_TYPE_ARC ){
				id = ((Arc_t*) item1)->id;
				//printf("найдена дуга %i\n", id);
			}
			if( id >0 ){ // Перебираем все примитивы в контексте и ищем первый, который не принадлежит ни одному контуру
				if( !item1->cont ){
					//printf( "Найден свободный сегмент .. %i\n", id );
					// C этого места надо перемещаться по связанным через точки примитивам. Пока не выйдем на этот стартовый примитив;
					return obhod( item1, cont );
				}
			}
		}
	}
	return 0;
}

/*
* Поиск всех замкнутых контуров  в котексте
*/
uint8_t find_all_conts(){
	Cont_t* next;
	while(1){
		next = create_cont();
//		printf("Ищем контур...\n");
		int r = find_closed_cont( next );
//		printf("найден контур %i элементов: %i\n", next->num, r);
		if( r  <= 0 ) break;
		next->dir = cont_dir(next);
	}
	remove_cont( &next );
}


/*
 * Проверяем, не состоит ли контур из единственной полной окружности,
 * и если так, то добавляем точку на неё для сохранения направления контура
*/
void fix_single_arc_cont( Cont_t* cont ){
	if( cont && cont->links.arr && (cont->links.count==2) ){ // один элемент в контуре. По идее, так может быть если этот контур - полная окружность
		for( int i = 0; i < cont->links.count; i++ ){
			if( ((Refitem_t*) cont->links.arr[i])->type == OBJ_TYPE_ARC ){
				Arc_t* arc = (Arc_t*) cont->links.arr[i];
				if( arc->dir == 0 ){
					break_the_circle( arc, create_p(arc->center.x, arc->center.y+arc->R), cont->dir );
				}
			}
		}
	}
}

/*
 * Проверяем, не состоит ли контур из единственной полной окружности, и если так, то возвращаем
 * ссылку на эту единственную дугу, иначе, возвращаем NULL
*/
Arc_t* is_single_arc_cont( Cont_t* cont ){
	if( cont && cont->links.arr && (cont->links.count==2) ){ // один элемент в контуре. По идее, так может быть если этот контур - полная окружность
		for( int i = 0; i < cont->links.count; i++ ){
			if( ((Refitem_t*) cont->links.arr[i])->type == OBJ_TYPE_ARC ){
				Arc_t* arc = (Arc_t*) cont->links.arr[i];
				if( arc->dir == 0 ){
					return arc;
				}
			}
		}
	}
	return NULL;
}

/*
* Очистка контекста от всех контуров
*/
void clean_all_cont(){
	Context_t* ctx = get_context();
	if( ctx->links.arr && (ctx->links.count>0) ){
		for( int i = ctx->links.count-1; i >= 0 ; i-- ){
			if( ctx->links.arr[i] ){
				Refitem_t* item1 = ctx->links.arr[i];
				if( item1->type == OBJ_TYPE_CONTUR ){
					Cont_t* cont = (Cont_t*) ctx->links.arr[i];
					clean_cont( cont );
					fix_single_arc_cont( cont );
					remove_cont( &cont );
				}
			}
		}
	}
}

void clean_conts_by_list(Refholder_t** list){
	Refholder_t* curr = *list;
	while( curr ){
		Refholder_t* next = curr->next;
		clean_cont( (Cont_t*) curr->refitem );
		fix_single_arc_cont( (Cont_t*) curr->refitem );
		remove_cont( (Cont_t**) &curr->refitem );
		free(curr);
		curr = next;
	}
	*list = NULL;
}


/*
* Контекст по имени
*/
Context_t* context_by_name(const char *name){
	if(name == NULL)  return NULL;
	for(int i = 0; i < context_count; i++){
		if(context_pool[i] != NULL && strcmp(context_pool[i]->name, name) == 0){
			return context_pool[i];
		}
	}
	return NULL;
}

/*
* Копирование примитивов из контекста в контекст
* Возвращаемое значение:
* < 0  - Критическая ошибка при копировании - невозможность создания элемента и т.п.
* 0    - Ничего не было скопировано
* > 0   - Количество скопированных элементов
*/
int8_t copy_ctx2ctx( char* src, char* dst, Cont_t* src_cont ){
	Context_t* current_ctx = get_context();
	Context_t* src_ctx = context_by_name(src);
	Context_t* dst_ctx = context_by_name(dst);
	int retval = 0;
	if( src_ctx && dst_ctx && (src_ctx != dst_ctx) ){
		select_context( dst );
		if( src_ctx->links.arr ){
			for( int i = 0; i < src_ctx->links.count; i++ ){ // прямой обход в поисках Контура
				Refitem_t* item1 = src_ctx->links.arr[i];
				if( item1->type == OBJ_TYPE_CONTUR ){
					if( (src_cont == NULL) || ( ((Cont_t*) item1) == src_cont) ){
						if( item1->links.arr ){
							Cont_t* cont = create_cont();			 // если контур найден, создаём контур в целевом контексте
							if( !cont ){
								printf("Ошибка создания нового элемента: Cont_t cont при копировании контекста в контекст\n");
								return -1;
							}
							cont->dir = ((Cont_t*) item1)->dir;
							Point_t* first_p = NULL;
							Point_t* prev_p = NULL;
							Point_t* next_p = NULL;
							for( int j = 0; j < item1->links.count; j++ ){
								Refitem_t* item2 = item1->links.arr[j];
								if( item2->type == OBJ_TYPE_LINE ){
									Line_t* src_line = (Line_t*) item2;
									next_p = create_p( src_line->b->x, src_line->b->y );
									if( !next_p ){
										printf("Ошибка создания нового элемента: Point_t next_p при копировании контекта в контекст\n");
										return -1;
									}
									if( !prev_p ){
										prev_p = create_p( src_line->a->x, src_line->a->y );
									}
									if( !first_p ) first_p = prev_p;
									if( p_eq_p( first_p, next_p ) ){
										remove_p( &next_p );
										next_p = first_p;
									}
									Line_t* new_line = create_line( prev_p, next_p );
									if( !new_line ){
										printf("Ошибка создания нового элемента: Line_t new_line при копировании контекта в контекст\n");
										return -1;
									}
									add_item2cont( (Refitem_t*) new_line, cont );
									retval++;
								}
								if( item2->type == OBJ_TYPE_ARC ){
									Arc_t* src_arc = (Arc_t*) item2;
									Arc_t* arc = create_arc( src_arc->center.x, src_arc->center.y, src_arc->R );
									if( !arc ){
										printf("Ошибка создания нового элемента: Arc_t arc при копировании контекта в контекст\n");
										return -1;
									}
									add_item2cont( (Refitem_t*) arc, cont );
									retval++;
									if( src_arc->dir != 0 ){
										next_p = create_p( src_arc->b->x, src_arc->b->y );
										if( !next_p ){
											printf("Ошибка создания нового элемента: Point_t next_p при копировании контекта в контекст\n");
											return -1;
										}
										if( !prev_p ){
											prev_p = create_p( src_arc->a->x, src_arc->a->y );
											if( !prev_p ){
												printf("Ошибка создания нового элемента: Point_t prev_p при копировании контекта в контекст\n");
												return -1;
											}
										}
										if( !first_p ) first_p = prev_p;
										if( p_eq_p( first_p, next_p ) ){
											remove_p( &next_p );
											next_p = first_p;
										}
										break_the_circle( arc, prev_p, src_arc->dir );
										Arc_t* new_arc = split_arc_by_p( arc, next_p );
										remove_arc( &new_arc );
									}else{
										continue;
									}
								}
								prev_p = next_p;
							}
						}
					}
				}
			}
		}
		select_context(current_ctx->name);
	};
	return retval;
}

/*
* Возвращает первый (и, надеюсь, единственный) контур в контексте
*/
Cont_t* first_cont( Context_t* _ctx ){
	Context_t* ctx = (_ctx != NULL)?_ctx:get_context();
	return (Cont_t*) first_obj_by_type( (Refitem_t*) ctx, OBJ_TYPE_CONTUR );
}

/*
* Сбросить показатель contcount для всех примитивов контекста.
*/
void clean_contcount( Context_t* _ctx ){
	Context_t* ctx = (_ctx != NULL)?_ctx:get_context();
	if( ctx && ctx->links.arr ){
		for( int i = 0; i < ctx->links.count; i++ ){
			if( ( (Refitem_t*) ctx->links.arr[i])->type == OBJ_TYPE_LINE ){
				Line_t* line = (Line_t*) ctx->links.arr[ i ];
				line->contcount = 0;
			}
			if( ( (Refitem_t*) ctx->links.arr[i])->type == OBJ_TYPE_ARC ){
				Arc_t* arc = (Arc_t*) ctx->links.arr[ i ];
				arc->contcount = 0;
			}
		}
	}
}

void marking_of_imposed_by_list( Refholder_t* list ){
	Refholder_t* curr = list;
	while( curr ){
		Cont_t* cont1 = (Cont_t*) curr->refitem;
		//Cont_t* cont1 = (Cont_t*) item1;
		for( int k = 0; k < cont1->links.count; k++ ){
			Line_t* line1 = NULL;
			Arc_t* arc1 = NULL;
			//if( ((Refitem_t*) cont1->links.arr[k])->type == OBJ_TYPE_LINE) line1 = (Line_t*) cont1->links.arr[k];
			if( is_line( cont1->links.arr[k] ) ) line1 = (Line_t*) cont1->links.arr[k];
			//if( ((Refitem_t*) cont1->links.arr[k])->type == OBJ_TYPE_ARC)  arc1 =  (Arc_t*)  cont1->links.arr[k];
			if( is_arc( cont1->links.arr[k] )  ) arc1  = (Arc_t*)  cont1->links.arr[k];
			if( arc1 || line1 ){
				Refitem_t* list1 = NULL;
				Refitem_t* list2 = NULL;
				int count1 = 0;
				int count2 = 0;

				Refholder_t* curr2 = curr->next;  // перебираем только уникальные пары контуров
				while( curr2 ){
					Cont_t* cont2 = (Cont_t*) curr2->refitem;
					//Cont_t* cont2 = (Cont_t*) item2;
					//printf("// cont1= %i   cont2= %i\n", cont1->num, cont2->num);
					for( int l = 0; l < cont2->links.count; l++ ){
						Line_t* line2 = NULL;
						Arc_t* arc2 = NULL;
						//if( ((Refitem_t*) cont2->links.arr[l])->type == OBJ_TYPE_LINE) line2 = (Line_t*) cont2->links.arr[l];
						if( is_line(cont2->links.arr[l]) ) line2 = (Line_t*) cont2->links.arr[l];
						//if( ((Refitem_t*) cont2->links.arr[l])->type == OBJ_TYPE_ARC)  arc2 =  (Arc_t*)  cont2->links.arr[l];
						if( is_arc(cont2->links.arr[l])  ) arc2  = (Arc_t*)  cont2->links.arr[l];
						if( line1 && line2 ){
							if( (line1->a == line2->a) && (line1->b == line2->b) ){
								//printf("// Совпадающие отрезки (совпадают по направлению) \n");
								if( !list1 ){
									list1 = (Refitem_t*) line1;
									line1->inshadow=1;
									count1++;
								}
								list1 = (Refitem_t*) line2;
								line2->inshadow = 1;
								count1++;
							}else if( (line1->a == line2->b) && (line1->b == line2->a) ){
								//printf("// Совпадающие отрезки (противоположены по направлению) \n");
								if( !list1 ){
									list1 = (Refitem_t*) line1;
									line1->inshadow = 1;
									count1++;
								}
								list2 = (Refitem_t*) line2;
								line2->inshadow = 1;
								count2++;
							}
						}else if( arc1 && arc2 ){
							if( xy_eq_xy(arc1->center.x, arc1->center.y, arc2->center.x, arc2->center.y) && (arc1->dir!=0) && (arc2->dir!=0) && (arc1->a == arc2->a) && (arc1->b == arc2->b) && (arc1->dir == arc2->dir) ){
								if( !list1 ){
									list1 = (Refitem_t*) arc1;
									arc1->inshadow = 1;
									count1++;
								}
								list1 = (Refitem_t*) arc2;
								arc2->inshadow = 1;
								count1++;
							}else if( xy_eq_xy(arc1->center.x, arc1->center.y, arc2->center.x, arc2->center.y) && (arc1->dir!=0) && (arc2->dir!=0) && (arc1->a == arc2->b) && (arc1->b == arc2->a) && (arc1->dir != arc2->dir) ){
//								printf("// Совпадающие дуги (противоположены по направлению) \n");
								if( !list1 ){
									list1 = (Refitem_t*) arc1;
									arc1->inshadow = 1;
									count1++;
								}
								list2 = (Refitem_t*) arc2;
								arc2->inshadow = 1;
								count2++;
							}
						}
					}
					curr2 = curr2->next;
				}
				if( count1 > count2 ){
					if( list1 ){
						if( is_line(list1) ) ((Line_t*) list1)->inshadow = 0;
						if( is_arc(list1)  ) ((Arc_t*)  list1)->inshadow = 0;
					}
				}else if(count1 < count2){
					if( list2 ){
						//if( ((Refitem_t*) list2)->type == OBJ_TYPE_LINE ) ((Line_t*) list2)->inshadow = 0;
						//if( ((Refitem_t*) list2)->type == OBJ_TYPE_ARC ) ((Arc_t*) list2)->inshadow = 0;
						if( is_line(list2) ) ((Line_t*) list2)->inshadow = 0;
						if( is_arc(list2)  ) ((Arc_t*)  list2)->inshadow = 0;
					}
				}
				list1 = NULL;
				list2 = NULL;
			}
		}
		curr = curr->next;
	}
}


/*
* Маркировка наложенных сегментов
* (удаляем пары сегментов с противоположенным направлением, несколько сегментов одного направления заменяем одним)
*/
void marking_of_imposed(){
//	printf("!!! marking_of_imposed !!!\n");
//	walk_around_all_cont("log", 10);
	Context_t* ctx = get_context();
	if( ctx->links.arr ){
		for( int i = 0; i < ctx->links.count; i++ ){
			Refitem_t* item1 = ctx->links.arr[i];
			if( item1->type == OBJ_TYPE_CONTUR ){
				Cont_t* cont1 = (Cont_t*) item1;
				for( int k = 0; k < cont1->links.count; k++ ){
					Line_t* line1 = NULL;
					Arc_t* arc1 = NULL;
					if( ((Refitem_t*) cont1->links.arr[k])->type == OBJ_TYPE_LINE) line1 = (Line_t*) cont1->links.arr[k];
					if( ((Refitem_t*) cont1->links.arr[k])->type == OBJ_TYPE_ARC)  arc1 =  (Arc_t*)  cont1->links.arr[k];
					if( arc1 || line1 ){
						Refitem_t* list1 = NULL;
						Refitem_t* list2 = NULL;
						int count1 = 0;
						int count2 = 0;
						for( int j = i + 1; j < ctx->links.count; j++ ){ // перебираем только уникальные пары контуров
							Refitem_t* item2 = ((Refitem_t*) ctx->links.arr[j]);
							if( item2->type == OBJ_TYPE_CONTUR ){
								Cont_t* cont2 = (Cont_t*) item2;
								//printf("// cont1= %i   cont2= %i\n", cont1->num, cont2->num);
								for( int l = 0; l < cont2->links.count; l++ ){
									Line_t* line2 = NULL;
									Arc_t* arc2 = NULL;
									if( ((Refitem_t*) cont2->links.arr[l])->type == OBJ_TYPE_LINE) line2 = (Line_t*) cont2->links.arr[l];
									if( ((Refitem_t*) cont2->links.arr[l])->type == OBJ_TYPE_ARC)  arc2 =  (Arc_t*)  cont2->links.arr[l];
									if( line1 && line2 ){
										if( (line1->a == line2->a) && (line1->b == line2->b) ){
//											printf("// Совпадающие отрезки (совпадают по направлению) \n");
											if( !list1 ){
												list1 = (Refitem_t*) line1;
												line1->contcount=1;
												count1++;
											}
											list1 = (Refitem_t*) line2;
											line2->contcount = 1;
											count1++;
										}else if( (line1->a == line2->b) && (line1->b == line2->a) ){
//											printf("// Совпадающие отрезки (противоположены по направлению) \n");
											if( !list1 ){
												list1 = (Refitem_t*) line1;
												line1->contcount = 1;
												count1++;
											}
											list2 = (Refitem_t*) line2;
											line2->contcount = 1;
											count2++;
										}
									}else if( arc1 && arc2 ){
										if( xy_eq_xy(arc1->center.x, arc1->center.y, arc2->center.x, arc2->center.y) && (arc1->dir!=0) && (arc2->dir!=0) && (arc1->a == arc2->a) && (arc1->b == arc2->b) && (arc1->dir == arc2->dir) ){
//											printf("// Совпадающие дуги (совпадают по направлению) \n");
											// print_arc(arc1);
											// printf("\n");
											// print_arc(arc2);
											if( !list1 ){
												list1 = (Refitem_t*) arc1;
												arc1->contcount = 1;
												count1++;
											}
											list1 = (Refitem_t*) arc2;
											arc2->contcount = 1;
											count1++;
										}else if( xy_eq_xy(arc1->center.x, arc1->center.y, arc2->center.x, arc2->center.y) && (arc1->dir!=0) && (arc2->dir!=0) && (arc1->a == arc2->b) && (arc1->b == arc2->a) && (arc1->dir != arc2->dir) ){
//											printf("// Совпадающие дуги (противоположены по направлению) \n");
											if( !list1 ){
												list1 = (Refitem_t*) arc1;
												arc1->contcount = 1;
												count1++;
											}
											list2 = (Refitem_t*) arc2;
											arc2->contcount = 1;
											count2++;
										}
									}
								}
							}
						}
						if( count1 > count2 ){
							if( list1 ){
								if( ((Refitem_t*) list1)->type == OBJ_TYPE_LINE ) ((Line_t*) list1)->contcount = 0;
								if( ((Refitem_t*) list1)->type == OBJ_TYPE_ARC ) ((Arc_t*) list1)->contcount = 0;
							}
						}else if(count1 < count2){
							if( list2 ){
								if( ((Refitem_t*) list2)->type == OBJ_TYPE_LINE ) ((Line_t*) list2)->contcount = 0;
								if( ((Refitem_t*) list2)->type == OBJ_TYPE_ARC ) ((Arc_t*) list2)->contcount = 0;
							}
						}
						list1 = NULL;
						list2 = NULL;
					}
				}
			}
		}
	}
//	printf("!!! end of marking_of_imposed !!!\n");
}


