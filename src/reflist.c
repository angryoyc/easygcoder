#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "../include/top.h"
#include "../include/reflist.h"
#include "../include/point.h"
#include "../include/arc.h"
#include "../include/line.h"

void* linkobj2obj( void* _ref1, void* _ref2){
	Refitem_t* ref1 = (Refitem_t*) _ref1;
	Refitem_t* ref2 = (Refitem_t*) _ref2;
	if( ref1 && ref2 ){
		linkobj( &ref1->links, ref2 );
		linkobj( &ref2->links, ref1 );
	}
}

int unlinkobj2obj( void* _ref1, void* _ref2){
	Refitem_t* ref1 = (Refitem_t*) _ref1;
	Refitem_t* ref2 = (Refitem_t*) _ref2;
	if( ref1 && ref2 ){
		uint8_t confirm1 = 1;
		uint8_t confirm2 = 1;
		if( ref1->onunlink ) confirm1=ref1->onunlink(ref1, ref2);
		if( ref2->onunlink ) confirm2=ref2->onunlink(ref2, ref1);
//		printf("unlinkobj2obj f1 = %s | f1 = %s\n", ref1->onunlink?"notNULL":"NULL", ref2->onunlink?"notNULL":"NULL");
		if( confirm1 && confirm2 ){
			unlinkobj( &ref1->links, ref2 );
			unlinkobj( &ref2->links, ref1 );
			return 1;
		}else{
			return 0;
		}
	}
	return 0;
}

uint8_t is_ref_in_list( Reflist_t* reflist, void* _ref){
	Refitem_t* ref = (Refitem_t*) _ref;
	if( !ref || !reflist ) return 0;
	for( int i=0; i<reflist->count; i++ ){
		if( reflist->arr[i] == ref ) return 1;
	}
	return 0;
}

void* linkobj( Reflist_t* reflist, void* _ref){
	Refitem_t* ref = (Refitem_t*) _ref;
	if( !ref || !reflist ) return NULL;
	if( is_ref_in_list( reflist, ref) ) return ref;
	if( !reflist->arr ){
		reflist->arr = malloc(sizeof(void*) * MAXLEN);
		reflist->count = 0;
		reflist->len = MAXLEN;
	}
	if( reflist->count >= reflist->len ){
		void* old = reflist->arr;
		int newlen = (reflist->len + MAXLEN);
		reflist->arr = malloc( sizeof(void*) * newlen );
		memset( reflist->arr, 0x00, sizeof(void*) * newlen );
		memcpy ( reflist->arr, old, sizeof(void*) * reflist->len );
		reflist->len = newlen;
		free( old );
	}
	reflist->arr[ reflist->count++ ] = ref;
	return ref;
}

void unlinkobj( Reflist_t* reflist, void* ref ){
	if( ref ){
		for( int i=0; i<reflist->count; i++ ){
			if( reflist->arr[i] == ref ){
				for( int j = i; j < reflist->count - 1; ++j ){        // сдвигаем массив
					reflist->arr[j] = reflist->arr[j + 1];
				}
				reflist->arr[--reflist->count] = NULL;
				break;
			}
		}
	}
	return;
}

/*
* Удаляем все связи объекта с другими объектами. Возвращает список Item для которых операция unlink была выполнена
* этот список может быть использован для освобождения объектов, которые больше не связаны ни с одним объектом
*/
Refholder_t* unlinkyouself(Refitem_t* ref){
	Refholder_t* result = NULL; //
	if( ref ){
		Reflist_t* reflist = &ref->links; // удаляем себя у всем моих пиров
		if( reflist->count>0 ){
			for( int i = reflist->count-1; i>=0; i-- ){
				Refitem_t* peer = reflist->arr[i];
				if( peer ){
					if( unlinkobj2obj( peer, ref ) ){
						// Здесь нужно добавить peer в список удалённых, который должен вернуться из функции.
						int n = sizeof(Refholder_t);
						Refholder_t* newholder = malloc(n);
						if( newholder ){
							memset( newholder, 0x00, n );
							newholder->refitem = peer;
							newholder->next = result;
							result = newholder;
						}else{
							return result;
						}
					};
				}else{
					for( int j = i; j < reflist->count - 1; ++j ){        // сдвигаем массив
						reflist->arr[j] = reflist->arr[j + 1];
					}
					reflist->arr[--reflist->count] = NULL;
				}
			}
		}
	}
	return result;
}

void purge_by_list( Refholder_t* list, Refitem_t** _ref ){
	Refholder_t* curr = list;
	while(curr){
		Refholder_t* next = curr->next;
		Refitem_t* peer = curr->refitem;
		if( peer ){
			if( peer->links.count == 0 ){
				if( peer->links.arr ){
					free( peer->links.arr );
					peer->links.arr = NULL;
				}
				if( peer == *_ref ){
					*_ref = NULL;
				}
				free( peer );
				peer = NULL;
			}
		}
		free(curr);
		curr = next;
	}
}

void purge_obj( Refitem_t** _ref ){
	Refitem_t* ref = (Refitem_t*) *_ref;
	if( ref ){
		Refholder_t* list = unlinkyouself( ref ); //пытаемся удалить все ссылки
		purge_by_list( list, &ref ); // очищаем объекты без ссылок
		if( ref ){
			if( ref->links.count == 0 ){
				free( ref->links.arr );
				ref->links.arr = NULL;
				free( ref );
				ref = NULL;
				*_ref = NULL;
			}
		}else{
			*_ref = NULL;
		}
	}else{
		*_ref = NULL;
	}
}

/*
* Определение связности объектов. Возвращает количество связей (для нормально связанных объектов должно возвращаться 2)
*/
uint8_t is_linked( Refitem_t* obj1, Refitem_t* obj2){
	int s = 0;
	if( obj1 && obj2 ){
		if( obj1->links.arr ){
			for( int i = 0; i < obj1->links.count; i++){
				if( (Refitem_t*) obj1->links.arr[i] == obj2 ){
					s++;
					break;
				}
			}
		}
		if( obj2->links.arr ){
			for( int j = 0; j < obj2->links.count; j++){
				if( (Refitem_t*) obj2->links.arr[j] == obj1 ){
					s++;
					break;
				}
			}
		}
	}
	return s;
}

void cp_links( Refitem_t* src, Refitem_t* dst ){
	for( int i = 0; i < src->links.count; i++ ){
		Refitem_t* item = src->links.arr[i];
		linkobj2obj( item, dst );
		if( (src->type==OBJ_TYPE_POINT) && (src->type==OBJ_TYPE_POINT) ){
			Point_t* old = (Point_t*) src;
			Point_t* new = (Point_t*) dst;
			if( item->type == OBJ_TYPE_ARC ){
				Arc_t* arc = (Arc_t*) item;
				if( arc->a == old ) arc->a = new;
				if( arc->b == old ) arc->b = new;
			}
			if( item->type == OBJ_TYPE_LINE ){
				Line_t* line = (Line_t*) item;
				if( line->a == old ) line->a = new;
				if( line->b == old ) line->b = new;
			}
		}
	}
};

int obj_count(Refitem_t* item, Obj_Type_t type){
	int s=0;
	for( int i = 0; i < item->links.count; i++ ){
		if( ((Refitem_t*) item->links.arr[i])->type == type ) s++;
	}
	return s;
}

char* obj_type(Refitem_t* item){
	if( item->type == OBJ_TYPE_POINT ) return "point";
	if( item->type == OBJ_TYPE_LINE ) return "line";
	if( item->type == OBJ_TYPE_ARC ) return "arc";
	if( item->type == OBJ_TYPE_CONTUR ) return "contur";
	if( item->type == OBJ_TYPE_CONTEXT ) return "conttext";
}

Refitem_t* first_obj_by_type( Refitem_t* item, Obj_Type_t type ){
	Refitem_t* obj = NULL;
	if( item && item->links.arr ){
		for( int i = 0; i < item->links.count; i++ ){
			if( ( (Refitem_t*) item->links.arr[i])->type == type ){
				obj = (Refitem_t*) item->links.arr[i];
				break;
			}
		}
	}
	return obj;
}

int list_len( Refholder_t* list ){
	int len = 0;
	Refholder_t* curr = list;
	while( curr ){
		len++;
		curr = curr->next;
	}
	return len;
}

void push2list( Refitem_t* item, Refholder_t** list ){
	int refholder_len = sizeof(Refholder_t);
	int flag = 1;
	Refholder_t* curr = *list;
	while( curr ){
		if( curr->refitem == item ){
			flag = 0;
			break;
		}
		curr = curr->next;
	}
	if( flag ){
		Refholder_t* newholder = malloc( refholder_len );
		if( newholder ){
			memset( newholder, 0x00, refholder_len );
			newholder->refitem = (Refitem_t*) item;
			newholder->next = *list;
			*list = newholder;
		}else{
			return;
		}
	}
}
