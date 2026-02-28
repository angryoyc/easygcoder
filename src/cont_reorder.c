void cont_reverse( Cont_t* cont ){
	if( !cont ) return;
	// 1. Реверсируем порядок сегментов в массиве
	for( int i = 0; i < cont->links.count / 2; i++ ){
		void* temp = cont->links.arr[i];
		cont->links.arr[i] = cont->links.arr[cont->links.count - 1 - i];
		cont->links.arr[cont->links.count - 1 - i] = temp;
	}
	// 2. Разворачиваем каждый сегмент
	for( int i = 0; i < cont->links.count; i++ ){
		Refitem_t* item = (Refitem_t*)cont->links.arr[i];
		if( !item ) continue;
		if( item->type == OBJ_TYPE_LINE ){
			Line_t* line = (Line_t*)item;
			Point_t* temp = line->a;
			line->a = line->b;
			line->b = temp;
		}else if( item->type == OBJ_TYPE_ARC ){
			Arc_t* arc = (Arc_t*)item;
			Point_t* temp = arc->a;
			arc->a = arc->b;
			arc->b = temp;
			arc->dir = -arc->dir;
		}
	}
	cont->dir = -cont->dir;
}

int next_prim_idx( Cont_t* cont, int start, Point_t* p ){
	for( int i = start; i < cont->links.count; i++ ){
		if( cont->links.arr ){
			Refitem_t* item = (Refitem_t*) cont->links.arr[i];
			if( !item ) continue;
			if( item->type == OBJ_TYPE_LINE ){
				if( p == NULL) return i;
				Line_t* line = (Line_t*) item;
				if( (line->a == p) || (line->b == p) ) return i;
			}
			if (item->type == OBJ_TYPE_ARC ){
				if( p == NULL ) return i;
				Arc_t* arc = (Arc_t*) item;
				if( arc->dir != 0 ){
					if( (arc->a == p) || (arc->b == p) ) return i;
				}
			}
		}
	}
	return -1;
}

struct p {
	float x;
	float y;
};

int lacing(struct p points[], int point_count){
	double area = 0.0;
	for( int i = 0; i < point_count; i++ ){
		int j = (i + 1) % point_count;
		double term = (points[i].x * points[j].y - points[j].x * points[i].y);
		area += term;
		//printf("  %d: %.1f*%.1f - %.1f*%.1f = %.1f\n", i, points[i].x, points[j].y, points[j].x, points[i].y, term);
	}
	area /= 2.0;
	if( area > 0.0 ) return 1;   // CCW - против часовой
	if( area < 0.0 ) return -1;  // CW - по часовой
	return 0;                    // Площадь нулевая - неопределено
}

// Возвращает:
//  1 - обход против часовой стрелки (CCW)
// -1 - обход по часовой стрелке (CW)
//  0 - ошибка или неопределено
int cont_dir(Cont_t* cont ){
	if( !cont ) return 0;
	// Собираем все точки контура в порядке обхода
	int count = 0;
	for(int i = 0; i < cont->links.count; i++){
		Refitem_t* item = (Refitem_t*)cont->links.arr[i];
		if(!item) continue;
		if( item->type == OBJ_TYPE_LINE ) count++;
		if( item->type == OBJ_TYPE_ARC  ) count=count+2;
	}
	struct p points[count];
	int point_count = 0;
	for(int i = 0; i < cont->links.count; i++){                          // Проходим по всем элементам контура и собираем точки
		Refitem_t* item = (Refitem_t*)cont->links.arr[i];
		if(!item) continue;
		if(item->type == OBJ_TYPE_LINE){
			Line_t* line = (Line_t*) item;
			if(line->a && point_count < count ){
				//points[point_count++] = line->a;
				points[point_count++] = (struct p){
					.x = line->a->x,
					.y = line->a->y
				};
			}
		}else if( item->type == OBJ_TYPE_ARC ){
			Arc_t* arc = (Arc_t*) item;
			if( arc->a && (arc->dir!=0) && (point_count < count) ){
				points[point_count++] = (struct p){
					.x = arc->a->x,
					.y = arc->a->y
				};
				//print_arc(arc);
				double x=0;
				double y=0;
				int r = get_xy_of_arc_mid( arc, &x, &y );
				//printf(" mid = ( %f, %f ) (exitcode = %i)\n", x, y, r);
				points[point_count++] = (struct p){
					.x = x,
					.y = y
				};
			}
		}
	}

	// ОТЛАДКА: выводим собранные точки
	// printf("Точки в cont_dir (%d точек):\n", point_count);
	// for(int i = 0; i < point_count; i++){
	//     printf("  [%d] (%.1f, %.1f)\n", i, points[i]->x, points[i]->y);
	// }
	// Если точек меньше 3, не можем определить направление
	//	printf("нашлось точек: %i\n", point_count);

	if( point_count < 3 ){
		int first = next_prim_idx( cont, 0, NULL );
		if( first >=0 ){
			//printf("по крайней мере один примитив в контуре\n");
			int second = ((first+1)<cont->links.count)?next_prim_idx( cont, first+1, NULL ):-1;
			if( second >= 0 ){
				//printf("// всего два примитива\n");
				Refitem_t* first_item = cont->links.arr[first];
				Refitem_t* second_item = cont->links.arr[second];
				// Line + Line
				if( (first_item->type == OBJ_TYPE_LINE) && (second_item->type == OBJ_TYPE_LINE) ){
//					Line_t* line1 = (Line_t*) first_item;
//					Line_t* line2 = (Line_t*) second_item;
//					if( (line1->a == line2->a) && (line1->b == line2->b) ){
//						line2->a = line1->b;
//						line2->b = line1->a;
//					}
					return 0; //два элемента и оба - линия. Установит направление не получится
				}
				// Дуга + дуга
				if( (first_item->type == OBJ_TYPE_ARC) && (second_item->type == OBJ_TYPE_ARC) ){
					Arc_t* arc1 = (Arc_t*) first_item;
					Arc_t* arc2 = (Arc_t*) second_item;
					if( (arc1->a == arc2->a) && (arc1->b == arc2->b) ) return 0;
					struct p points2[4];
					points2[0] = points[0];
					points2[2] = points[1];
					double x = 0;
					double y = 0;
					get_xy_of_arc_mid( arc1, &x, &y );
					points2[1] = (struct p){ .x=x, .y=y };
					get_xy_of_arc_mid( arc2, &x, &y );
					points2[3] = (struct p){ .x=x, .y=y };
					return lacing( points2, 4 );

//					if( (arc1->a == arc2->a) && (arc1->b == arc2->b) && (arc1->dir != arc2->dir) ){
//						Point_t* tmp = arc2->a;
//						arc2->a = arc2->b;
//						arc2->b = tmp;
//						//arc2->dir = (arc2->dir==1)?2:1;
//						arc2->dir = -arc2->dir;
//					}

				}

				Arc_t* arc = NULL;
				Line_t* line = NULL;

				if( (first_item->type == OBJ_TYPE_ARC) && (second_item->type == OBJ_TYPE_LINE) ){
					//printf("// Дуга + линия\n");
					arc = (Arc_t*) first_item;
					line = (Line_t*) second_item;
				}

				if( (first_item->type == OBJ_TYPE_LINE) && (second_item->type == OBJ_TYPE_ARC) ){
					//printf("// Линия + Дуга\n");
					line = (Line_t*) first_item;
					arc = (Arc_t*) second_item;
				}

				if( arc && line ){
					if( (arc->a == line->a) && (arc->b == line->b) ) return 0;
					return arc->dir;
				}else{
					return 0;
				}
			}else{
//				printf("// всего один примитив\n");
				Refitem_t* item = cont->links.arr[first];
				if( item->type == OBJ_TYPE_LINE ) return 0; // единственный примитив. Нельзя установить направление
				Arc_t* arc = (Arc_t*) item;
				if( arc->dir == 0 ) return 0; // единственный примитив - полная окружность. Нельзя установить направление
				return arc->dir;
			}
		}
		return 0;
	}
	return lacing( points, point_count );
}

// Восстанавливает правильный порядок сегментов в массиве контура
int cont_reorder( Cont_t* cont, int _dir ){
//	printf("\nВосстанавливаем порядок в контуре %i элементов %i dir=%i set dir=%i\n", cont->num, cont->links.count, cont->dir, _dir );
	if (!cont || cont->links.count < 2) return 0;
	int desired_dir = _dir?_dir:cont->dir;
	desired_dir = (desired_dir==0)?1:desired_dir;
	// Находим стартовый сегмент
	int curr = next_prim_idx( cont, 0, NULL );
// -- обход
	while( curr>=0 ){
		int next = -1;
		int linked = -1;
		Refitem_t* item = cont->links.arr[curr];
		Point_t* seg_a = NULL;
		Point_t* seg_b = NULL;
//		printf("Текущий элемент: ");
		if(item->type == OBJ_TYPE_LINE){
//			print_line((Line_t*)item);
			seg_a = ((Line_t*)item)->a;
			seg_b = ((Line_t*)item)->b;
		}else if(item->type == OBJ_TYPE_ARC){
//			print_arc((Arc_t*)item);
			seg_a = ((Arc_t*)item)->a;
			seg_b = ((Arc_t*)item)->b;
//		}else{
//			printf(" not found");
		}
//		printf("\n");
		next = next_prim_idx( cont, curr+1, NULL );
//		printf("next = %i\n", next);
		if( next>=0 ){
			linked = next_prim_idx( cont, curr+1, seg_b );
//			printf("linked = %i\n", linked);
			if( linked>=0 ){
				if( next != linked ){
					Refitem_t* tmp = cont->links.arr[next];
					cont->links.arr[next] = cont->links.arr[linked];
					cont->links.arr[linked] = tmp;
				}
				Refitem_t* next_item = cont->links.arr[next];
				if (next_item->type == OBJ_TYPE_LINE){
					Line_t* next_line = (Line_t*) next_item;
					if( seg_b == next_line->b ){
						Point_t* tmp = next_line->b;
						next_line->b = next_line->a;
						next_line->a = tmp;
					}
				}else{
					Arc_t* next_arc = (Arc_t*) next_item;
					if( seg_b == next_arc->b ){
						Point_t* tmp = next_arc->b;
						next_arc->b = next_arc->a;
						next_arc->a = tmp;
						next_arc->dir = -next_arc->dir;
					}
				}
			}
		}
		curr = next;
	}
// --
//	printf("вычисление направления контура\n");
	int dir = cont_dir( cont );
//	printf(" вычисленное направление %i желаемое направление: %i\n", dir, desired_dir );
	if( dir != 0 ){
		if( dir != desired_dir ){
//			printf(" переворачиваем\n");
			cont_reverse( cont );
			cont->dir = desired_dir;
//			dir = cont_dir( cont );
//			printf(" направление после разворота %i, %i\n", dir, cont->dir );
		}else{
			cont->dir = desired_dir;
		}
	}else{
		if( is_single_arc_cont(cont) ){
			cont->dir = desired_dir;
		}else{
			printf("Не определился единственная окружность\n");
		}
	}
//	printf("Cont_reorder в контуре %i OK элементов %i dir=%i\n", cont->num, cont->links.count, cont->dir );
	return cont->dir;
}
