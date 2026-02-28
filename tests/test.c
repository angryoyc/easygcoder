#include <stdint.h>
#include <stdio.h>

#include "../include/define.h"
#include "../include/top.h"
#include "../include/reflist.h"
#include "../include/point.h"
#include "../include/line.h"
#include "../include/cont.h"
#include "../include/geom.h"
#include "../include/context.h"
#include "../include/arc.h"
#include "../include/excellon.h"
#include "../include/milling.h"
#include "../include/mergcont.h"

#include "../include/gcode.h"
#include "../include/export.h"
#include "../include/conf.h"
#include "../include/apply_config.h"

#include "../include/bdd.h" // макросы
#define MAXLEN 20


DESCRIBE(Геометрия) {

	apply_config("/etc/easygcoder/easygcoder.conf");

	CONTEXT("Геометрические вычисленя");
	IT("Совпадение координат", {
		Context_t* ctx = create_context("tmp");
		ASSERT_EQ_INT( 1, xy_eq_xy( 50.530020, -10.922020, 50.530020, -10.922021) );
		free_context_by_name(ctx->name);
	});

	IT("Совпадение координат2", {
		ASSERT_EQ_INT( 0, xy_eq_xy( -10.519995, -30.478511, -10.519995, -30.478529) );
		double x1 = 0;
		double y1 = 0;
		middle(  -10.519995, -30.478511, -10.519995, -30.478529, &x1, &y1 );
		ASSERT_EQ_INT( 0, xy_eq_xy( -10.519995, -30.478511, x1, y1) );
		ASSERT_EQ_INT( 0, xy_eq_xy( x1, y1, -10.519995, -30.478529) );
	});

	IT("Дистанция между двумя точками заданными координатами", {
		Context_t* ctx = create_context("tmp");
		double len = distance(0, 0, 3, 4);
		ASSERT_EQ_DOUBLE( 5, len );
		free_context_by_name(ctx->name);
	});

	IT("Середина между двумя точками по координатам", {
		Context_t* ctx = create_context("tmp");
		double x;
		double y;
		middle(0, 0, 6, 8, &x, &y);
		ASSERT_EQ_DOUBLE( 3, x );
		ASSERT_EQ_DOUBLE( 4, y );
		free_context_by_name(ctx->name);
	});

	IT("Точка середины между двумя заданными точками", {
		Context_t* ctx = create_context("tmp");
		ASSERT_TRUE( ctx != NULL );
		Point_t* p = create_p_of_line_mid(create_p(0,0), create_p(6 ,8) );
		ASSERT_EQ_DOUBLE( 3, p->x );
		ASSERT_EQ_DOUBLE( 4, p->y );
		free_context_by_name(ctx->name);
	});

	IT("Нормализация углов", {
		double ang = normalize_angle(2 * M_PI);
		ASSERT_EQ_DOUBLE( 0, ang );
	});

	IT("Определение попадания точки на дугу (геометрическое)", {
		Context_t* ctx = create_context("tmp");
		Arc_t* arc = create_arc( 3, 3, 3 );
		Point_t* p1 = create_p( 3, 6 );
		Point_t* p2 = create_p( 3, 0 );
		int res1 = is_p_on_arc_geom( 3, 3, 3, 0, 3, 6, 3, -1, 3, 6 );
		ASSERT_EQ_INT( 1, res1 );
		int res2 = is_p_on_arc_geom( 3, 3, 3, 0, 3, 6, 3, -1, 3, 0 );
		ASSERT_EQ_INT( 0, res2 );
		free_context_by_name(ctx->name);
	});

	IT("Пересечение отрезков. Пересечений нет. Не коллиниарное.(mode 1)", {
		Context_t* ctx = create_context("tmp");
		double x1=0;
		double y1=0;
		double x2=0;
		double y2=0;
		uint8_t ret = xy_of_line_x_line(0, 0, 3, 3, 8, 0, 5, 3,  &x1, &y1, &x2, &y2);
		ASSERT_EQ_INT( 1, ret );
		ASSERT_EQ_DOUBLE( 0, x1 );
		ASSERT_EQ_DOUBLE( 0, y1 );
		ASSERT_EQ_DOUBLE( 0, x2 );
		ASSERT_EQ_DOUBLE( 0, y2 );
		free_context_by_name(ctx->name);
	});

	IT("Пересечение отрезков. Одна точка пересечений. Не коллиниарное.(mode 2)", {
		Context_t* ctx = create_context("tmp");
		double x1=0;
		double y1=0;
		double x2=0;
		double y2=0;
		uint8_t ret = xy_of_line_x_line(0, 0, 3, 3, 3, 0, 0, 3,  &x1, &y1, &x2, &y2);
		ASSERT_EQ_INT( 2, ret );
		ASSERT_EQ_DOUBLE( 1.5, x1 );
		ASSERT_EQ_DOUBLE( 1.5, y1 );
		ASSERT_EQ_DOUBLE( 0, x2 );
		ASSERT_EQ_DOUBLE( 0, y2 );
		free_context_by_name(ctx->name);
	});

	IT("Пересечение отрезков. Одна точка пересечений. Пересечение концами. Не коллиниарное.(mode 3)", {
		Context_t* ctx = create_context("tmp");
		double x1 = 0;
		double y1 = 0;
		double x2 = 0;
		double y2 = 0;
		uint8_t ret = xy_of_line_x_line(0, 0, 3, 3, 6, 0, 3, 3,  &x1, &y1, &x2, &y2);
		ASSERT_EQ_INT( 3, ret );
		ASSERT_EQ_DOUBLE( 3, x1 );
		ASSERT_EQ_DOUBLE( 3, y1 );
		ASSERT_EQ_DOUBLE( 0, x2 );
		ASSERT_EQ_DOUBLE( 0, y2 );
		free_context_by_name(ctx->name);
	});

	IT("Пересечение отрезков. Одна точка пересечений. Касание одним концом. Не коллиниарное.(mode 4)", {
		Context_t* ctx = create_context("tmp");
		double x1 = 0;
		double y1 = 0;
		double x2 = 0;
		double y2 = 0;
		uint8_t ret = xy_of_line_x_line(0, 0, 3, 3, 3, 0, 1.5, 1.5,  &x1, &y1, &x2, &y2);
		ASSERT_EQ_INT( 4, ret );
		ASSERT_EQ_DOUBLE( 1.5, x1 );
		ASSERT_EQ_DOUBLE( 1.5, y1 );
		ASSERT_EQ_DOUBLE( 0, x2 );
		ASSERT_EQ_DOUBLE( 0, y2 );
		free_context_by_name(ctx->name);
	});

	IT("Пересечение отрезков. Коллиниарное. Нет пересечений. Отрезки параллельны и на разных прямых. (mode 5)", {
		Context_t* ctx = create_context("tmp");
		double x1 = 0;
		double y1 = 0;
		double x2 = 0;
		double y2 = 0;
		uint8_t ret = xy_of_line_x_line(0, 0, 3, 3, 3, 0, 6, 3,  &x1, &y1, &x2, &y2);
		ASSERT_EQ_INT( 5, ret );
		ASSERT_EQ_DOUBLE( 0, x1 );
		ASSERT_EQ_DOUBLE( 0, y1 );
		ASSERT_EQ_DOUBLE( 0, x2 );
		ASSERT_EQ_DOUBLE( 0, y2 );
		free_context_by_name(ctx->name);
	});

	IT("Пересечение отрезков. Коллиниарное. Нет пересечений. Отрезки на одной прямой, без перекрытий. (mode 6)", {
		Context_t* ctx = create_context("tmp");
		double x1 = 0;
		double y1 = 0;
		double x2 = 0;
		double y2 = 0;
		uint8_t ret = xy_of_line_x_line(0, 0, 3, 3, 5, 5, 6, 6,  &x1, &y1, &x2, &y2);
		ASSERT_EQ_INT( 6, ret );
		ASSERT_EQ_DOUBLE( 0, x1 );
		ASSERT_EQ_DOUBLE( 0, y1 );
		ASSERT_EQ_DOUBLE( 0, x2 );
		ASSERT_EQ_DOUBLE( 0, y2 );
		free_context_by_name(ctx->name);
	});

	IT("Пересечение отрезков. Коллиниарное. Пересечений - 1. Отрезки на одной прямой, касаются концами. (mode 7)", {
		Context_t* ctx = create_context("tmp");
		double x1 = 0;
		double y1 = 0;
		double x2 = 0;
		double y2 = 0;
		uint8_t ret = xy_of_line_x_line(0, 0, 3, 3, 3, 3, 6, 6,  &x1, &y1, &x2, &y2);
		ASSERT_EQ_INT( 7, ret );
		ASSERT_EQ_DOUBLE( 3, x1 );
		ASSERT_EQ_DOUBLE( 3, y1 );
		ASSERT_EQ_DOUBLE( 0, x2 );
		ASSERT_EQ_DOUBLE( 0, y2 );
		free_context_by_name(ctx->name);
	});

	IT("Пересечение отрезков. Коллиниарное. Пересечений - 2. Отрезки на одной прямой, имеют область наложения . (mode 8)", {
		Context_t* ctx = create_context("tmp");
		double x1 = 0;
		double y1 = 0;
		double x2 = 0;
		double y2 = 0;
		uint8_t ret = xy_of_line_x_line(0, 0, 6, 6, 3, 3, 9, 9,  &x1, &y1, &x2, &y2);
		ASSERT_EQ_INT( 8, ret );
		ASSERT_EQ_DOUBLE( 3, x1 );
		ASSERT_EQ_DOUBLE( 3, y1 );
		ASSERT_EQ_DOUBLE( 6, x2 );
		ASSERT_EQ_DOUBLE( 6, y2 );
		free_context_by_name(ctx->name);
	});

	IT("Пересечение отрезков. Коллиниарное. Пересечений - 2. Один вписан в другой, концы не совпадают  (mode 9)", {
		Context_t* ctx = create_context("tmp");
		double x1 = 0;
		double y1 = 0;
		double x2 = 0;
		double y2 = 0;
//		uint8_t ret = xy_of_line_x_line(1, 0, 6, 0, 1, 0, 6, 0,  &x1, &y1, &x2, &y2);
		uint8_t ret = xy_of_line_x_line(0, 0, 9, 9, 3, 3, 6, 6,  &x1, &y1, &x2, &y2);
		ASSERT_EQ_INT( 9, ret );
		ASSERT_EQ_DOUBLE( 3, x1 );
		ASSERT_EQ_DOUBLE( 3, y1 );
		ASSERT_EQ_DOUBLE( 6, x2 );
		ASSERT_EQ_DOUBLE( 6, y2 );
		free_context_by_name(ctx->name);
	});


	IT("Пересечение отрезков. Коллиниарное. Пересечений - 2. Один вписан в другой, концы совпадают с одой стороны  (mode 10)", {
		Context_t* ctx = create_context("tmp");
		double x1 = 0;
		double y1 = 0;
		double x2 = 0;
		double y2 = 0;
		uint8_t ret = xy_of_line_x_line(0, 0, 9, 9, 3, 3, 9, 9,  &x1, &y1, &x2, &y2);
		ASSERT_EQ_INT( 10, ret );
		ASSERT_EQ_DOUBLE( 3, x1 );
		ASSERT_EQ_DOUBLE( 3, y1 );
		ASSERT_EQ_DOUBLE( 9, x2 );
		ASSERT_EQ_DOUBLE( 9, y2 );
		free_context_by_name(ctx->name);
	});

	IT("Пересечение отрезков. Коллиниарное. Пересечений - 2. Один вписан в другой, концы совпадают с двух сторон (mode 11)", {
		Context_t* ctx = create_context("tmp");
		double x1 = 0;
		double y1 = 0;
		double x2 = 0;
		double y2 = 0;
		uint8_t ret = xy_of_line_x_line(0, 0, 9, 9, 0, 0, 9, 9,  &x1, &y1, &x2, &y2);
		ASSERT_EQ_INT( 11, ret );
		ASSERT_EQ_DOUBLE( 0, x1 );
		ASSERT_EQ_DOUBLE( 0, y1 );
		ASSERT_EQ_DOUBLE( 9, x2 );
		ASSERT_EQ_DOUBLE( 9, y2 );
		free_context_by_name(ctx->name);
	});


	IT("Пересечение отрезка и дуги", {
		Context_t* ctx = create_context("tmp");
	// (конт. 2192) луч из (50.030020, -11.922020)  ( в 51.530022, -8.922020 ) с отрезком (50.530020, -11.922020)-(50.530020, -10.922020)  (mode 4)  /+/ в т.(50.530020 -10.922021) конец B пропускаем
	// (конт. 2192) луч из (50.030020, -11.922020)  ( в 51.530022, -8.922020 ) с дугой (50.530020, -10.922020)-(49.530020, -9.922020) центр:(49.530020, -10.922020) R: 1.000000  (mode 0
	// x1: 50.530020, y1: -10.922021,
		Arc_t* arc = create_arc( 49.530020, -10.922020, 1 );
		Point_t* a = create_p( 50.530020, -10.922021 );
		Point_t* b = create_p( 49.530020, -9.922020 );
		break_the_circle( arc, a, 1 );
		Arc_t* new_arc = split_arc_by_p( arc, b );
		double p1x = 0;
		double p1y = 0;
		double p2x = 0;
		double p2y = 0;
		uint8_t r = xy_of_line_x_arc( 50.030020, -11.922020, 51.530022, -8.922020, (Arc_t*) arc, &p1x, &p1y, &p2x, &p2y );
		ASSERT_EQ_INT( 1, r );
		free_context_by_name(ctx->name);
	});

	IT("Положение точки относительно прямой", {
		Context_t* ctx = create_context("tmp");
		Point_t* a = create_p( 0, 0 );
		Point_t* b = create_p( 9, 9 );
		double dist1 =  p_to_line_distance(0, 3, a->x, a->y, b->x, b->y);
		double dist2 =  p_to_line_distance(3, 0, a->x, a->y, b->x, b->y);
		double dist3 =  p_to_line_distance(3, 3, a->x, a->y, b->x, b->y);
		ASSERT_EQ_DOUBLE( 2.121320, dist1 );
		ASSERT_EQ_DOUBLE( -2.121320, dist2 );
		ASSERT_EQ_DOUBLE( 0, dist3 );
		free_context_by_name(ctx->name);
	});

	IT("Точка пересечения двух отрезков (v1) (геометрическое)", {
		Context_t* ctx = create_context("tmp");
		double x1 = 0;
		double y1 = 0;
		double x2 = 0;
		double y2 = 0;
		uint8_t ret = xy_of_line_x_line(0, 0, 3, 3, 3, 0, 0, 3,  &x1, &y1, &x2, &y2);
		ASSERT_EQ_INT( 2, ret );
		ASSERT_EQ_DOUBLE( 1.5, x1 );
		ASSERT_EQ_DOUBLE( 1.5, y1 );
		ASSERT_EQ_DOUBLE( 0, x2 );
		ASSERT_EQ_DOUBLE( 0, y2 );
		free_context_by_name(ctx->name);
	});



/*

	IT("Точки пересечения двух отрезков (один внутри другого на одной прямой)", {
		Context_t* ctx = create_context("tmp");
		double x1 = 0;
		double y1 = 0;
		double x2 = 0;
		double y2 = 0;
		uint8_t ret = xy_of_line_x_line(0, 0, 13, 13, 3, 3, 5, 5,  &x1, &y1, &x2, &y2);
		ASSERT_EQ_INT( 2, ret );
		ASSERT_EQ_DOUBLE( 3, x1 );
		ASSERT_EQ_DOUBLE( 3, y1 );
		ASSERT_EQ_DOUBLE( 5, x2 );
		ASSERT_EQ_DOUBLE( 5, y2 );
		free_context_by_name(ctx->name);
	});


	IT("Точки пересечения двух отрезков (отрезки частично перекрываются и лежат на одной прямой)", {
		Context_t* ctx = create_context("tmp");
		double x1 = 0;
		double y1 = 0;
		double x2 = 0;
		double y2 = 0;
		uint8_t ret = xy_of_line_x_line(0, 0, 4, 4, 3, 3, 5, 5,  &x1, &y1, &x2, &y2);
		ASSERT_EQ_INT( 2, ret );
		ASSERT_EQ_DOUBLE( 3, x1 );
		ASSERT_EQ_DOUBLE( 3, y1 );
		ASSERT_EQ_DOUBLE( 4, x2 );
		ASSERT_EQ_DOUBLE( 4, y2 );
		free_context_by_name(ctx->name);
	});

	IT("Точка пересечения двух отрезков (отрезки имеют один общий конец и лежат на одной прямой)", {
		Context_t* ctx = create_context("tmp");
		double x1 = 0;
		double y1 = 0;
		double x2 = 0;
		double y2 = 0;
		uint8_t ret = xy_of_line_x_line(0, 0, 4, 4, 4 , 4, 5, 5,  &x1, &y1, &x2, &y2);
		ASSERT_EQ_INT( 0, ret );
		free_context_by_name(ctx->name);
	});

	IT("Точка пересечения двух отрезков (отрезки имеют только один общий конец, второй конеу внутри другого отрезка)", {
		Context_t* ctx = create_context("tmp");
		double x1 = 0;
		double y1 = 0;
		double x2 = 0;
		double y2 = 0;
		uint8_t ret = xy_of_line_x_line( 0, 0,  4, 4,  0, 0,  5, 5,  &x1, &y1, &x2, &y2 );
		ASSERT_EQ_INT( 1, ret );
		ASSERT_EQ_DOUBLE( 4, x1 );
		ASSERT_EQ_DOUBLE( 4, y1 );
		free_context_by_name(ctx->name);
	});

	IT("Точки пересечения двух отрезков", {
		Context_t* ctx = create_context("tmp");
		Line_t* l1 = create_line( create_p(0, 0), create_p(3, 3) );
		Line_t* l2 = create_line( create_p(3, 0), create_p(0, 3) );
		Line_t* l3 = create_line( create_p(3, 0), create_p(6, 3) );
		Point_t* p1;
		Point_t* p2;
		create_p_of_line_x_line( l1->a, l1->b, l2->a, l2->b, &p1, &p2 );
		ASSERT_TRUE( p1 != NULL );
		create_p_of_line_x_line( l1->a, l1->b, l3->a, l3->b, &p1, &p2 );
		ASSERT_TRUE( p1 == NULL );
		ASSERT_TRUE( p2 == NULL );

		create_p_of_line_x_line( l2->a, l2->b, l3->a, l3->b, &p1, &p2 );
		ASSERT_TRUE( p1 == NULL );

		create_p_of_line_x_line( l1->a, l1->b, l1->a, l1->b, &p1, &p2 );
		ASSERT_TRUE( p1 == NULL );

//		ASSERT_EQ_DOUBLE( 3, l3->a->x );
//		ASSERT_EQ_DOUBLE( 0, l3->a->y );
		free_context_by_name(ctx->name);
	});
*/


	CONTEXT("Базовые элементы (контекст, точка, отрезок, дуга) и операции с ними");

	IT("Создание и удаление контекста", {
		Context_t* ctx = create_context("tmp");
		ASSERT_TRUE( ctx != NULL );
		ASSERT_EQ_INT( 1, ctx->links.count );
		ASSERT_EQ_INT( MAXLEN, ctx->links.len );
		free_context_by_name(ctx->name);
	});

	IT("Одна точка на плоскости", {
		Context_t* ctx = create_context("tmp");
		Point_t* a = create_p( 0, 0 );
		ASSERT_TRUE( a != NULL );
		ASSERT_EQ_INT( 1, a->links.count );
		ASSERT_EQ_INT( 2, ctx->links.count );
		unlinkobj2obj(ctx, a);
		ASSERT_EQ_INT( 0, a->links.count );
		ASSERT_EQ_INT( 1, ctx->links.count );
		remove_p(&a);
		free_context_by_name(ctx->name);
	});

	IT("Одна прямая на плоскости", {
		Context_t* ctx = create_context("tmp");
		Point_t* a = create_p( 0, 0 );
		Point_t* b = create_p( 2, 3 );
		Line_t* l = create_line( a, b );
		ASSERT_TRUE( l != NULL );
		ASSERT_TRUE( l->a != NULL );
		ASSERT_TRUE( l->b != NULL );
		ASSERT_EQ_INT( 2, a->links.count );
		ASSERT_EQ_INT( 2, b->links.count );
		ASSERT_EQ_INT( 3, l->links.count );
		ASSERT_EQ_INT( 4, ctx->links.count );
		free_context_by_name(ctx->name);
	});

	IT("Два отрезка с одним общим концом", {
		Context_t* ctx = create_context("tmp");
		Point_t* a = create_p( 0, 0 );
		Point_t* b = create_p( 2, 3 );
		Point_t* с = create_p( 3, 2 );
		Line_t* l1 = create_line( a, b );
		Line_t* l2 = create_line( a, с );
		free_context_by_name(ctx->name);
	});

	IT("Одна окружность на плоскости", {
		Context_t* ctx = create_context("tmp");
		Arc_t* arc = create_arc( 0, 0, 3 );
		ASSERT_TRUE( arc != NULL );
		ASSERT_EQ_INT( 1, arc->links.count );
		free_context_by_name(ctx->name);
	});

	CONTEXT("Лини");

	IT("Определение попадания точки на отрезок", {
		Context_t* ctx = create_context("tmp");
		Point_t* a = create_p( 0, 0 );
		Point_t* b = create_p( 6, 6 );
		Point_t* p1 = create_p( 3, 3 );
		Point_t* p2 = create_p( 3, 0 );
		int res1 = is_p_on_line( a, b, p1 );
		ASSERT_EQ_INT( 1, res1 );
		int res2 = is_p_on_line( a, b, p2 );
		ASSERT_EQ_INT( 0, res2 );
		free_context_by_name(ctx->name);
	});

	IT("Разбиение линии точкой на две части (с переносом точек при необходимости)", {
		Context_t* ctx = create_context("tmp");
		Point_t* a = create_p( 0, 0 );
		Point_t* b = create_p( 6, 6 );
		Line_t* l = create_line( a, b );
		Point_t* p1 = create_p( 3, 3 );
		Line_t* new_l = split_line_by_p( l, p1 );

		ASSERT_EQ_DOUBLE( 0, l->a->x );
		ASSERT_EQ_DOUBLE( 0, l->a->y );
		ASSERT_EQ_DOUBLE( 3, l->b->x );
		ASSERT_EQ_DOUBLE( 3, l->b->y );

		ASSERT_TRUE( new_l != NULL );
		ASSERT_EQ_DOUBLE( 3, new_l->a->x );
		ASSERT_EQ_DOUBLE( 3, new_l->a->y );
		ASSERT_EQ_DOUBLE( 6, new_l->b->x );
		ASSERT_EQ_DOUBLE( 6, new_l->b->y );


		free_context_by_name(ctx->name);
	});


	CONTEXT("Дуги");

	IT("Вычисление направления дуги", {
		Context_t* ctx = create_context("tmp");
		Arc_t* arc = create_arc( 3, 3, 3 );
		Point_t* a = create_p( 0, 3 );
		Point_t* b = create_p( 6, 3 );
		Point_t* c = create_p( 3, 6 );
		int dir = arc_dir( arc, a, b, c );
		ASSERT_EQ_INT( 1, dir );
		dir = arc_dir( arc, b, b, c );
		ASSERT_EQ_INT( 0, dir );
		free_context_by_name(ctx->name);
	});

	IT("Разбиение окружности точкой (превращение в дугу)", {
		Context_t* ctx = create_context("tmp");
		Arc_t* arc = create_arc( 3, 3, 3 );
		Point_t* p1 = create_p( 0, 3 );
		Point_t* p2 = create_p( 1, 3 );
		int res1 = break_the_circle( arc, p1, 2); // Основная проверяемая функция
		ASSERT_EQ_INT( 1, res1 );
		int res2 = break_the_circle( arc, p2, 2); // Основная проверяемая функция
		ASSERT_EQ_INT( 0, res2 );
		free_context_by_name(ctx->name);
	});

	IT("Разбиение дуги точкой на две части (с переносом точек при необходимости)", {
		Context_t* ctx = create_context("tmp");
		Arc_t* arc = create_arc( 3, 3, 3 );
		Point_t* p1 = create_p( 0, 3 );
		int res1 = break_the_circle( arc, p1, 1);
		Point_t* p3 = create_p( 3, 6 );
		linkobj2obj(arc, p3);
		Point_t* p2 = create_p( 6, 3 );
		Arc_t* new_arc = split_arc_by_p( arc, p2 );
		ASSERT_TRUE( new_arc != NULL );
		ASSERT_TRUE( arc != NULL );
		ASSERT_EQ_INT( 2, p3->links.count );
		ASSERT_EQ_INT( 4, new_arc->links.count );
		free_context_by_name(ctx->name);
	});


	IT("Определение попадания точки на дугу", {
		Context_t* ctx = create_context("tmp");
		Arc_t* arc = create_arc( 3, 3, 3 );
		Point_t* a = create_p( 0, 3 );
		Point_t* b = create_p( 6, 3 );
		Point_t* p1 = create_p( 3, 6 );
		Point_t* p2 = create_p( 3, 0 );
		break_the_circle( arc, a, -1);
		Arc_t* new_arc = split_arc_by_p( arc, b );
		int res1 = is_p_on_arc( arc, p1 );
		ASSERT_EQ_INT( 1, res1 );
		int res2 = is_p_on_arc( arc, p2 );
		ASSERT_EQ_INT( 0, res2 );
		free_context_by_name(ctx->name);
	});

	IT("Определение границ фигуры (дуга)", {
		Context_t* ctx = create_context("tmp");
		Arc_t* arc = create_arc( 3, 3, 3 );
		Point_t* a = create_p( 0, 3 );
		Point_t* b = create_p( 6, 3 );
		break_the_circle( arc, a, 1);
		Arc_t* new_arc = split_arc_by_p( arc, b );
		double xmax = 0;
		double xmin = 0;
		double ymax = 0;
		double ymin = 0;
		arc_get_bounds( arc, &xmin, &xmax, &ymin, &ymax);
		ASSERT_EQ_DOUBLE( 0, xmin );
		ASSERT_EQ_DOUBLE( 6, xmax );
		ASSERT_EQ_DOUBLE( 0, ymin );
		ASSERT_EQ_DOUBLE( 3, ymax );
		free_context_by_name(ctx->name);
	});



	IT("Пересечение окружностей. Две разные окружности пересекаются.", {
		Context_t* ctx = create_context("tmp");
		Arc_t* arc1 = create_arc( 3, 3, 3 );
		Arc_t* arc2 = create_arc( 0, 3, 3 );
		double x1;
		double y1;
		double x2;
		double y2;
		uint8_t ret = xy_of_arc_x_arc( arc1, arc2, &x1, &y1, &x2, &y2 );
		ASSERT_EQ_DOUBLE( 2, ret );
		ASSERT_EQ_DOUBLE( 1.5, x1 );
		ASSERT_EQ_DOUBLE( 5.598076, y1 );
		ASSERT_EQ_DOUBLE( 1.5, x2 );
		ASSERT_EQ_DOUBLE( 0.401924, y2 );
		free_context_by_name(ctx->name);
	});


	IT("Пересечение окружностей. Две разные окружности не пересекаются (одна внутри другой)", {
		Context_t* ctx = create_context("tmp");
		Arc_t* arc1 = create_arc( 3, 3, 3 );
		Arc_t* arc2 = create_arc( 3, 3, 2 );
		double x1;
		double y1;
		double x2;
		double y2;
		uint8_t ret = xy_of_arc_x_arc( arc1, arc2, &x1, &y1, &x2, &y2 );
		ASSERT_EQ_DOUBLE( 0, ret );
		ASSERT_EQ_DOUBLE( 0, x1 );
		ASSERT_EQ_DOUBLE( 0, y1 );
		ASSERT_EQ_DOUBLE( 0, x2 );
		ASSERT_EQ_DOUBLE( 0, y2 );
		free_context_by_name(ctx->name);
	});

	IT("Пересечение окружностей. Две разные окружности касаются", {
		Context_t* ctx = create_context("tmp");
		Arc_t* arc1 = create_arc( -3, 3, 3 );
		Arc_t* arc2 = create_arc( 3, 3, 3 );
		double x1;
		double y1;
		double x2;
		double y2;
		uint8_t ret = xy_of_arc_x_arc( arc1, arc2, &x1, &y1, &x2, &y2 );
		ASSERT_EQ_DOUBLE( 1, ret );
		ASSERT_EQ_DOUBLE( 0, x1 );
		ASSERT_EQ_DOUBLE( 3, y1 );
		ASSERT_EQ_DOUBLE( 0, x2 );
		ASSERT_EQ_DOUBLE( 0, y2 );
		free_context_by_name(ctx->name);
	});


	IT("Пересечение окружностей. Две окружности совпадают", {
		Context_t* ctx = create_context("tmp");
		Arc_t* arc = create_arc( 3, 3, 3 );
		double x1;
		double y1;
		double x2;
		double y2;
		uint8_t ret = xy_of_arc_x_arc( arc, arc, &x1, &y1, &x2, &y2 );
		ASSERT_EQ_DOUBLE( 3, ret );
		ASSERT_EQ_DOUBLE( 0, x1 );
		ASSERT_EQ_DOUBLE( 0, y1 );
		ASSERT_EQ_DOUBLE( 0, x2 );
		ASSERT_EQ_DOUBLE( 0, y2 );
		free_context_by_name(ctx->name);
	});

	IT("Точки пересечения дуг. Две окружности пересекаются", {
		Context_t* ctx = create_context("tmp");
		Arc_t* arc1 = create_arc( 0, 3, 3 );
		Arc_t* arc2 = create_arc( 3, 3, 3 );
		Point_t* p1;
		Point_t* p2;
		uint8_t ret = create_p_of_arc_x_arc( arc1, arc2, &p1, &p2 );
		ASSERT_EQ_DOUBLE( 170, ret ); // 1010 1010
		free_context_by_name(ctx->name);
	});

	IT("Точки пересечения дуг. Окружность пересекается с дугой, концы на окружности", {
		Context_t* ctx = create_context("tmp");
		Arc_t* arc1 = create_arc( 0, 3, 3 );
		Arc_t* arc2 = create_arc( 3, 3, 3 );
		Point_t* a = create_p(1.5, 5.598076);
		Point_t* b = create_p(1.5, 0.401924);
		break_the_circle( arc2, a, -1);
		Arc_t* new_arc2 = split_arc_by_p( arc2, b );
		Point_t* p1;
		Point_t* p2;
		uint8_t ret = create_p_of_arc_x_arc( arc1, arc2, &p1, &p2 );
		ASSERT_EQ_DOUBLE( 187, ret ); // 1011 1011
		ASSERT_TRUE( p1 == b );
		ASSERT_TRUE( p2 == a );
		free_context_by_name(ctx->name);
	});


	IT("Точки пересечения дуг. Окружность пересекается с дугой, только конец B на окружности", {
		Context_t* ctx = create_context("tmp");
		Arc_t* arc1 = create_arc( 0, 3, 3 );
		Arc_t* arc2 = create_arc( 3, 3, 3 );
		Point_t* a = create_p( 0, 3 );
		Point_t* b = create_p( 1.5, 0.401924 );
		break_the_circle( arc2, a, -1 );
		Arc_t* new_arc2 = split_arc_by_p( arc2, b );
		Point_t* p1;
		Point_t* p2;
		uint8_t ret = create_p_of_arc_x_arc( arc1, arc2, &p1, &p2 );
		ASSERT_EQ_DOUBLE( 186, ret ); // 1011 1010
		ASSERT_TRUE( p1 == b );
		ASSERT_TRUE( p2 != NULL );
		free_context_by_name(ctx->name);
	});

	IT("Точки пересечения дуг. Окружность пересекается с дугой, только конец A на окружности", {
		Context_t* ctx = create_context("tmp");
		Arc_t* arc1 = create_arc( 0, 3, 3 );
		Arc_t* arc2 = create_arc( 3, 3, 3 );
		Point_t* a = create_p( 1.5, 5.598076);
		Point_t* b = create_p( 0, 3 );;
		break_the_circle( arc2, a, -1 );
		Arc_t* new_arc2 = split_arc_by_p( arc2, b );
		Point_t* p1;
		Point_t* p2;
		uint8_t ret = create_p_of_arc_x_arc( arc1, arc2, &p1, &p2 );
		ASSERT_EQ_DOUBLE( 171, ret ); // 1010 1011
		ASSERT_TRUE( p2 == a );
		ASSERT_TRUE( p1 != NULL );
		free_context_by_name(ctx->name);
	});

	IT("Точки пересечения дуг. Дуги пересекаются. Концы совпадают" , {
		Context_t* ctx = create_context("tmp");
		Arc_t* arc1 = create_arc( 0, 3, 3 );
		Arc_t* arc2 = create_arc( 3, 3, 3 );
		Point_t* a = create_p( 1.5, 5.598076 );
		Point_t* b = create_p( 1.5, 0.401924 );
		break_the_circle( arc1, b, -1 );
		Arc_t* new_arc1 = split_arc_by_p( arc1, a );
		break_the_circle( arc2, a, -1 );
		Arc_t* new_arc2 = split_arc_by_p( arc2, b );
		Point_t* p1;
		Point_t* p2;
		uint8_t ret = create_p_of_arc_x_arc( arc1, arc2, &p1, &p2 );
		ASSERT_EQ_DOUBLE( 255, ret );
		ASSERT_TRUE( p1 != NULL );
		ASSERT_TRUE( p2 != NULL );
		ASSERT_TRUE( p1 == b );
		ASSERT_TRUE( p2 == a );
		free_context_by_name(ctx->name);
	});


	IT("Точки пересечения дуг. Окружность и дуга" , {
		Context_t* ctx = create_context("tmp");
		Arc_t* arc1 = create_arc( 3, 3, 3 );
		Arc_t* arc2 = create_arc( 3, 3, 3 );
		Point_t* a = create_p( 1.5, 5.598076 );
		Point_t* b = create_p( 1.5, 0.401924 );
		break_the_circle( arc1, b, -1 );
		Arc_t* new_arc1 = split_arc_by_p( arc1, a );
		Point_t* p1;
		Point_t* p2;
		uint8_t ret = create_p_of_arc_x_arc( arc1, arc2, &p1, &p2 );
		ASSERT_EQ_DOUBLE( 238, ret );
		ASSERT_TRUE( p1 != NULL );
		ASSERT_TRUE( p2 != NULL );
		ASSERT_TRUE( p1 == b );
		ASSERT_TRUE( p2 == a );
		free_context_by_name(ctx->name);
	});

	IT("Точки пересечения дуг. Дуга и окружность" , {
		Context_t* ctx = create_context("tmp");
		Arc_t* arc1 = create_arc( 3, 3, 3 );
		Arc_t* arc2 = create_arc( 3, 3, 3 );
		Point_t* a = create_p( 1.5, 5.598076 );
		Point_t* b = create_p( 1.5, 0.401924 );
		break_the_circle( arc1, b, -1 );
		Arc_t* new_arc1 = split_arc_by_p( arc1, a );
		Point_t* p1;
		Point_t* p2;
		uint8_t ret = create_p_of_arc_x_arc( arc2, arc1, &p1, &p2 );
		ASSERT_EQ_DOUBLE( 187, ret );
		ASSERT_TRUE( p1 != NULL );
		ASSERT_TRUE( p2 != NULL );
		ASSERT_TRUE( p1 == b );
		ASSERT_TRUE( p2 == a );
		free_context_by_name(ctx->name);
	});


	IT("Точки пересечения дуг. На одной окружности. Концы совпадают" , {
		Context_t* ctx = create_context("tmp");
		Arc_t* arc1 = create_arc( 3, 3, 3 );
		Point_t* a = create_p( 1.5, 5.598076 );
		Point_t* b = create_p( 1.5, 0.401924 );
		break_the_circle( arc1, b, -1 );
		Arc_t* new_arc1 = split_arc_by_p( arc1, a );
		Point_t* p1;
		Point_t* p2;
		uint8_t ret = create_p_of_arc_x_arc( arc1, new_arc1, &p1, &p2 );
		ASSERT_EQ_DOUBLE( 255, ret );
		ASSERT_TRUE( p1 != NULL );
		ASSERT_TRUE( p2 != NULL );
		ASSERT_TRUE( p1 == a );
		ASSERT_TRUE( p2 == b );
		free_context_by_name(ctx->name);
	});



	IT("Точки пересечения дуг. Дуги. Совпадает один конец " , {
		Context_t* ctx = create_context("tmp");
		Arc_t* arc1 = create_arc( 3, 3, 3 );
		Arc_t* arc2 = create_arc( 3, 3, 3 );
		Point_t* a = create_p( 0, 3 );
		Point_t* b = create_p( 6, 3 );
		Point_t* c = create_p( 6, 3 );
		Point_t* d = create_p( 3, 0 );
		break_the_circle( arc1, a, -1 );
		Arc_t* new_arc1 = split_arc_by_p( arc1, b );
		break_the_circle( arc2, c, -1 );
		Arc_t* new_arc2 = split_arc_by_p( arc2, d );
		Point_t* p1;
		Point_t* p2;
		uint8_t ret = create_p_of_arc_x_arc( arc1, arc2, &p1, &p2 );
		ASSERT_EQ_DOUBLE( 240, ret );
		ASSERT_TRUE( p1 != NULL );
		ASSERT_TRUE( p2 == NULL );
		ret = create_p_of_arc_x_arc( arc2, arc1, &p1, &p2 );
		ASSERT_EQ_DOUBLE( 15, ret );
		ASSERT_TRUE( p2 != NULL );
		ASSERT_TRUE( p1 == NULL );
		free_context_by_name(ctx->name);
	});

	IT("Точки пересечения дуг. Дуги. Налагаются (4 точки пересечений). " , {
		Context_t* ctx = create_context("tmp");
		Arc_t* arc1 = create_arc( 3, 3, 3 );
		Arc_t* arc2 = create_arc( 3, 3, 3 );
		Point_t* a = create_p( 4.5, 0.401924 );
		Point_t* b = create_p( 4.5, 5.598076 );
		Point_t* c = create_p( 3, 6 );
		Point_t* d = create_p( 3, 0 );
		break_the_circle( arc1, a, -1 );
		Arc_t* new_arc1 = split_arc_by_p( arc1, b );
		break_the_circle( arc2, c, -1 );
		Arc_t* new_arc2 = split_arc_by_p( arc2, d );
		Point_t* p1;
		Point_t* p2;
		uint8_t ret;
		ret = create_p_of_arc_x_arc( arc1, arc2, &p1, &p2 ); // точки разбиения arc2
		ASSERT_EQ_DOUBLE( 238, ret );
		ASSERT_TRUE( p1 != NULL );
		ASSERT_TRUE( p2 != NULL );
		ASSERT_TRUE( p1 == b );
		ASSERT_TRUE( p2 == a );
		ret = create_p_of_arc_x_arc( arc2, arc1, &p1, &p2 ); // точки разбиения arc1
		ASSERT_EQ_DOUBLE( 238, ret );
		ASSERT_TRUE( p1 != NULL );
		ASSERT_TRUE( p2 != NULL );
		ASSERT_TRUE( p1 == d );
		ASSERT_TRUE( p2 == c );
		free_context_by_name(ctx->name);
	});

	IT("Точки пересечения дуг. Дуги. Налагаются (2 точки пересечений). " , {
		Context_t* ctx = create_context("tmp");
		Arc_t* arc1 = create_arc( 3, 3, 3 );
		Arc_t* arc2 = create_arc( 3, 3, 3 );
		Point_t* a = create_p( 4.5, 0.401924 );
		Point_t* b = create_p( 4.5, 5.598076 );
		Point_t* c = create_p( 0, 3 );
		Point_t* d = create_p( 6, 3 );
		break_the_circle( arc1, a, -1 );
		Arc_t* new_arc1 = split_arc_by_p( arc1, b );
		break_the_circle( arc2, c, -1 );
		Arc_t* new_arc2 = split_arc_by_p( arc2, d );
		Point_t* p1;
		Point_t* p2;
		uint8_t ret;
		ret = create_p_of_arc_x_arc( arc1, arc2, &p1, &p2 ); // точки разбиения arc2
		ASSERT_EQ_DOUBLE( 224, ret );
		ASSERT_TRUE( p1 != NULL );
		ASSERT_TRUE( p2 == NULL );
		ASSERT_TRUE( p1 == b );
		ret = create_p_of_arc_x_arc( arc2, arc1, &p1, &p2 ); // точки разбиения arc1
		ASSERT_EQ_DOUBLE( 14, ret );
		ASSERT_TRUE( p1 == NULL );
		ASSERT_TRUE( p2 != NULL );
		ASSERT_TRUE( p2 == c );
		free_context_by_name(ctx->name);
	});


	IT("Точки пересечения дуг. Дуги. Налагаются (одна вложена в другую, 2 точки). " , {
		Context_t* ctx = create_context("tmp");
		Arc_t* arc1 = create_arc( 3, 3, 3 );
		Arc_t* arc2 = create_arc( 3, 3, 3 );
		Point_t* a = create_p( 4.5, 0.401924 );
		Point_t* b = create_p( 4.5, 5.598076 );
		Point_t* c = create_p( 3, 0 );
		Point_t* d = create_p( 3, 6 );
		break_the_circle( arc1, a, -1 );
		Arc_t* new_arc1 = split_arc_by_p( arc1, b );
		break_the_circle( arc2, c, -1 );
		Arc_t* new_arc2 = split_arc_by_p( arc2, d );
		Point_t* p1;
		Point_t* p2;
		uint8_t ret;
		ret = create_p_of_arc_x_arc( arc1, arc2, &p1, &p2 ); // точки разбиения arc2
		ASSERT_EQ_DOUBLE( 0, ret );
		ASSERT_TRUE( p1 == NULL );
		ASSERT_TRUE( p2 == NULL );
		ret = create_p_of_arc_x_arc( arc2, arc1, &p1, &p2 ); // точки разбиения arc1
		ASSERT_EQ_DOUBLE( 238, ret );
		ASSERT_TRUE( p1 != NULL );
		ASSERT_TRUE( p2 != NULL );
		ASSERT_TRUE( p1 == d );
		ASSERT_TRUE( p2 == c );
		free_context_by_name(ctx->name);
	});

// (конт. 1204) луч из (13.970000, -15.722600)  ( в 64.804798, -8.347200 ) с дугой (40.370020, -12.138534)-(40.640000, -11.853179) центр:(39.370000, -10.922000) R: 1.574800, dir: 1  (mode 0) 
// (конт. 1204) луч из (13.970000, -15.722600)  ( в 64.804798, -8.347200 ) с дугой (40.640000, -11.853179)-(40.910020, -12.138567) центр:(41.910000, -10.922000) R: 1.574800, dir: 1  (mode 1)  /+/ в т.(40.639991 -11.853167) P 

	IT("Луч пересекается почти в точке сопряжения двух дуг " , {
		Context_t* ctx = create_context("tmp");
		Point_t* r = create_p( 13.970000, -15.722600 );
		Point_t* m = create_p( 64.804798, -8.347200  );
		Arc_t* arc1 = create_arc( 39.370000, -10.922000, 1.574800 );
		Arc_t* arc2 = create_arc( 41.910000, -10.922000, 1.574800 );
		Point_t* a = create_p(40.370020, -12.138534);
		Point_t* b = create_p(40.640000, -11.853179);
		Point_t* c = create_p(40.910020, -12.138567);
		break_the_circle( arc1, a, 1 );
		Arc_t* new_arc1 = split_arc_by_p( arc1, b );
		break_the_circle( arc2, b, 1 );
		Arc_t* new_arc2 = split_arc_by_p( arc2, c );
		double p1x=0;
		double p1y=0;
		double p2x=0;
		double p2y=0;
		uint8_t ret = xy_of_line_x_arc( r->x, r->y, m->x, m->y, (Arc_t*) arc1, &p1x, &p1y, &p2x, &p2y );
		ASSERT_EQ_INT( 0, ret );
		ret = xy_of_line_x_arc( r->x, r->y, m->x, m->y, (Arc_t*) arc2, &p1x, &p1y, &p2x, &p2y );
		ASSERT_EQ_INT( 0, ret );
		free_context_by_name(ctx->name);
	});




	CONTEXT("Межобъектные операции");

	IT("Точки пересечения окружностей", {
		Context_t* ctx = create_context("tmp");
		Arc_t* arc1 = create_arc( 0, 3, 3 );
		Arc_t* arc2 = create_arc( 3, 3, 3 );
		Point_t* p1;
		Point_t* p2;
		create_p_of_arc_x_arc( arc1, arc2, &p1, &p2 );
		ASSERT_TRUE( p1 != NULL );
		ASSERT_TRUE( p2 != NULL );
		ASSERT_EQ_DOUBLE( 1.5, p1->x );
		ASSERT_EQ_DOUBLE( 1.5, p2->x );
		ASSERT_EQ_DOUBLE( 0.401924, p1->y );
		ASSERT_EQ_DOUBLE( 5.598076, p2->y );
		break_the_circle( arc1, p1, -1);
		break_the_circle( arc2, p1, -1);
		Arc_t* new_arc1 = split_arc_by_p( arc1, p2 );
		Arc_t* new_arc2 = split_arc_by_p( arc2, p2 );
		ASSERT_TRUE( new_arc1 != NULL );
		ASSERT_TRUE( new_arc2 != NULL );
		free_context_by_name(ctx->name);
	});


	IT("Точки пересечения отрезка и дуги (проверка вычислений)", {
		Context_t* ctx = create_context("tmp");
		Line_t* l1 = create_line( create_p(-3, 3), create_p(6, 6) );
		Arc_t* arc1 = create_arc( 3, 3, 3 );
		Point_t* p1;
		Point_t* p2;

		create_p_of_line_x_arc( l1, arc1, &p1, &p2);

		ASSERT_TRUE( p1 != NULL );
		ASSERT_TRUE( p2 != NULL );
		ASSERT_EQ_DOUBLE( 4.604541, p1->x );
		ASSERT_EQ_DOUBLE( 5.534847, p1->y );
		ASSERT_EQ_DOUBLE( 0.195459, p2->x );
		ASSERT_EQ_DOUBLE( 4.065153, p2->y );
		free_context_by_name(ctx->name);
	});

	IT("Точки пересечения отрезка и дуги", {
		Context_t* ctx = create_context("tmp");
		Line_t* l1 = create_line( create_p(0, 0), create_p(3, 3) );
		Arc_t* arc1 = create_arc( 3, 3, 3 );
		Point_t* p1;
		Point_t* p2;
		create_p_of_line_x_arc( l1, arc1, &p1, &p2);
		ASSERT_TRUE( p1 != NULL );
		ASSERT_TRUE( p2 == NULL );
		ASSERT_EQ_DOUBLE( 0.878680, p1->x );
		ASSERT_EQ_DOUBLE( 0.878680, p1->y );
		free_context_by_name(ctx->name);

		ctx = create_context("tmp");
		Line_t* l2 = create_line( create_p(0, 0), create_p(6, 6) );
		Arc_t* arc2 = create_arc( 3, 3, 3 );
		Point_t* p3;
		Point_t* p4;
		create_p_of_line_x_arc( l2, arc2, &p3, &p4);
		ASSERT_TRUE( p3 != NULL );
		ASSERT_TRUE( p3 != NULL );
		ASSERT_EQ_DOUBLE( 5.121320, p3->x );
		ASSERT_EQ_DOUBLE( 5.121320, p3->y );
		ASSERT_EQ_DOUBLE( 0.878680, p4->x );
		ASSERT_EQ_DOUBLE( 0.878680, p4->y );
		free_context_by_name(ctx->name);

		ctx = create_context("tmp");
		Line_t* l3 = create_line( create_p(0, 0), create_p(6, 0) );
		Arc_t* arc3 = create_arc( 3, 3, 3 );
		Point_t* p5;
		Point_t* p6;
		create_p_of_line_x_arc( l3, arc3, &p5, &p6);
		ASSERT_TRUE( p5 != NULL );
		ASSERT_TRUE( p6 != NULL );
		ASSERT_TRUE( p5 == p6 );
		ASSERT_EQ_DOUBLE( 3, p5->x );
		ASSERT_EQ_DOUBLE( 0, p5->y );
		free_context_by_name(ctx->name);

		ctx = create_context("tmp");
		Arc_t* arc4 = create_arc( 3, 3, 3 );
		Line_t* l4 = create_line( create_p(0, 0), create_p(6, 0) );
		Point_t* p7 = create_p( 0, 3 );
		Point_t* p8 = create_p( 6, 3 );
		break_the_circle( arc4, p7, -1);
		Arc_t* new_arc = split_arc_by_p( arc4, p8 );
		Point_t* p9;
		Point_t* p10;
		create_p_of_line_x_arc( l4, arc4, &p9, &p10);
		ASSERT_TRUE( p9 == NULL );
		free_context_by_name(ctx->name);

		ctx = create_context("tmp");
		Arc_t* arc5 = create_arc( 3, 3, 3 );
		Line_t* l5 = create_line( create_p(0, 1), create_p(6, 1) );
		Point_t* p11 = create_p( 0, 3 );
		Point_t* p12 = create_p( 6, 3 );
		break_the_circle( arc5, p11, 1);
		split_arc_by_p( arc5, p12 );
		Point_t* p13;
		Point_t* p14;
		create_p_of_line_x_arc( l5, arc5, &p13, &p14);
		ASSERT_TRUE( p13 != NULL );
		ASSERT_TRUE( p14 != NULL );
		free_context_by_name(ctx->name);

	});



	CONTEXT("Контуры");

	IT("Создание контура", {
		Context_t* ctx = create_context("tmp");
		Line_t* l = create_line( create_p(0, 0), create_p(6, 6) );
		ASSERT_TRUE( l->a != NULL );
		ASSERT_TRUE( l->b != NULL );
		Point_t* p1 = create_p( 3, 3 );
		Cont_t* cont = create_cont();
		ASSERT_TRUE( cont != NULL );
		add_item2cont( (Refitem_t*) l, cont );
		ASSERT_TRUE( is_linked( (Refitem_t*) l, (Refitem_t*) cont) );
		free_context_by_name(ctx->name);
	});

	IT("Наследование контура прямой при разбиении её точкой", {
		Context_t* ctx = create_context("tmp");
		Line_t* l = create_line( create_p(0, 0), create_p(6, 6) );
		Point_t* p1 = create_p( 3, 3 );
		Cont_t* cont = create_cont();
		add_item2cont( (Refitem_t*) l, cont );
		Line_t* l2 = split_line_by_p( l, p1 );
		ASSERT_TRUE( l2 != NULL );
		ASSERT_TRUE( is_linked( (Refitem_t*) l2, (Refitem_t*) cont) );
		free_context_by_name(ctx->name);
	});


	IT("Сохранение порядка сегментов при разбиении одного из них (линии) точкой",{
		Context_t* ctx = create_context("tmp");

		Point_t* a = create_p( 0, 0 );
		Point_t* b = create_p( 6, 6 );
		Point_t* c = create_p( 12, 0 );
		Point_t* p1 = create_p( 3, 3 );

		Cont_t* cont = create_cont();
		Line_t* l1 = create_line( a, b );
		add_item2cont( (Refitem_t*) l1, cont );
		Line_t* l2= create_line( b, c);
		add_item2cont( (Refitem_t*) l2, cont );
		Line_t* l3= create_line( c, a);
		add_item2cont( (Refitem_t*) l3, cont );

		Line_t* l1_new = split_line_by_p( l1, p1 );
		ASSERT_TRUE( l1_new != NULL );
		ASSERT_TRUE( is_linked( (Refitem_t*) l1_new, (Refitem_t*) cont) );

		//printf("\n");
		//walk_around_all_cont("svg", stdout);

		free_context_by_name(ctx->name);
	});

	IT("Сохранение порядка сегментов при разбиении одного из них (дуги) точкой",{
		Context_t* ctx = create_context("tmp");

		Point_t* a = create_p( 0, 0 );
		Point_t* b = create_p( 6, 6 );
		Point_t* c = create_p( 12, 0 );
		Point_t* p1 = create_p( 6, -6 );

		Cont_t* cont = create_cont();

		Arc_t* arc1 = create_arc( 6, 0, 6 );
		break_the_circle( arc1, a, 1);
		Arc_t* new_arc = split_arc_by_p( arc1, c );
		add_item2cont( (Refitem_t*) arc1, cont );
		remove_arc(&new_arc);

		Line_t* l2 = create_line( c, b );
		add_item2cont( (Refitem_t*) l2, cont );
		Line_t* l1= create_line( b, a);
		add_item2cont( (Refitem_t*) l1, cont );

		Arc_t* l1_arc = split_arc_by_p( arc1, p1 );
		ASSERT_TRUE( l1_arc != NULL );
		ASSERT_TRUE( is_linked( (Refitem_t*) l1_arc, (Refitem_t*) cont) );

		//printf("\n");
		//walk_around_all_cont("svg", stdout);

		free_context_by_name(ctx->name);
	});

	IT("angle_factor",{
		Context_t* ctx = create_context("tmp");

		Point_t* o = create_p( 0, 0 );
		Point_t* s = create_p( 0, -3 );
		Point_t* t = create_p( 0, 3 );

		double cf_t = angle_factor( s, o, t );
		ASSERT_EQ_DOUBLE( 0, cf_t );

		double cf_s = angle_factor( s, o, s );
		ASSERT_EQ_DOUBLE( 3.141593, cf_s );

		Point_t* a = create_p( -3, -3 );
		double cf_a = angle_factor( s, o, a );
		ASSERT_EQ_DOUBLE( 2.356194, cf_a );

		Point_t* b = create_p( -3, 0 );
		double cf_b = angle_factor( s, o, b );
		ASSERT_EQ_DOUBLE( 1.570796, cf_b );

		Point_t* c = create_p( -1, 50 );
		double cf_c = angle_factor( s, o, c );
		ASSERT_EQ_DOUBLE( 0.019997, cf_c );

		Point_t* d = create_p( 1, 50 );
		double cf_d = angle_factor( s, o, d );
		ASSERT_EQ_DOUBLE( -0.019997, cf_d );

		Point_t* e = create_p( 3, 0 );
		double cf_e = angle_factor( s, o, e );
		ASSERT_EQ_DOUBLE( -1.570796, cf_e );

		Point_t* f = create_p( 3, -3 );
		double cf_f = angle_factor( s, o, f );
		ASSERT_EQ_DOUBLE( -2.356194, cf_f );

		free_context_by_name(ctx->name);
	});

	IT("angle_between_vectors",{
		Context_t* ctx = create_context("tmp");
		Point_t* o = create_p( 0, -6 );
		Point_t* a = create_p( 0, 0 );
		Point_t* b = create_p( 0, 6 );
		Point_t* d = create_p( 3, 4 );
		Arc_t* arc = create_arc( -4, 3, 5 );
		break_the_circle( arc, b, -1);
		Arc_t* new_arc = split_arc_by_p( arc, a );
		remove_arc( &new_arc );
		Line_t* line1 = create_line(o, a);
		Line_t* line2 = create_line(a, d);
		double cf0 = angle_factor( o, a, b );
		ASSERT_EQ_DOUBLE( 0, cf0 );
		//		printf("angle_factor\n");
		double cf = angle_factor( o, a, d );
		ASSERT_EQ_DOUBLE( -0.643501, cf );
		//		printf("angle_between_vectors s\n");
		double ang = angle_between_vectors( line2vect( line1, a, 1 ), line2vect( line2, a, 0 ) );
		//		printf("angle_between_vectors e\n");
		ASSERT_EQ_DOUBLE( -0.643501, ang );
		//		printf("angle_between_vectors + arc\n");
		Vector_t v = arc2vect( arc, a, 0 );
		//printf("v = (v.dx, v.dy) = (%f, %f)\n", v.dx, v.dy );
		double ang2 = angle_between_vectors( line2vect( line1, a, 1 ), arc2vect( arc, a, 0 ) );
		ASSERT_EQ_DOUBLE( -0.643501, ang2 );
		free_context_by_name(ctx->name);
	});

	IT("Наследование контура дугой при разбиении её точкой", {
		Context_t* ctx = create_context("tmp");
		Arc_t* arc = create_arc( 3, 3, 3 );
		Point_t* p1 = create_p( 0, 3 );
		int res1 = break_the_circle( arc, p1, 1);
		Cont_t* cont = create_cont();
		add_item2cont( (Refitem_t*) arc, cont );
		Point_t* p2 = create_p( 6, 3 );
		Arc_t* new_arc = split_arc_by_p( arc, p2 );
		ASSERT_TRUE( new_arc != NULL );
		ASSERT_TRUE( is_linked( (Refitem_t*) new_arc, (Refitem_t*) cont) );
		free_context_by_name(ctx->name);
	});

	IT("Вычисление границ контура", {
		Context_t* ctx = create_context("tmp");
		Arc_t* arc = create_arc( 4, 4, 3 );
		Point_t* p1 = create_p( 0, 3 );
		int res1 = break_the_circle( arc, p1, 1);
		Cont_t* cont = create_cont();
		add_item2cont( (Refitem_t*) arc, cont );
		Point_t* p2 = create_p( 6, 3 );
		Arc_t* new_arc = split_arc_by_p( arc, p2 );
		cont_get_bounds( cont );
		ASSERT_EQ_DOUBLE( 1,  cont->xmin );
		ASSERT_EQ_DOUBLE( 7,  cont->xmax );
		ASSERT_EQ_DOUBLE( 1,  cont->ymin );
		ASSERT_EQ_DOUBLE( 7,  cont->ymax );
		free_context_by_name(ctx->name);
	});



	IT("Два контура пересекаются", {
		Context_t* ctx = create_context("tmp");
		Cont_t* cont1 = create_cont();
		Cont_t* cont2 = create_cont();
		//CIRCLE: (62.2300,-10.9220) -> (62.2300,-10.9220) dia=1.5748
		//CIRCLE: (59.6900,-10.9220) -> (59.6900,-10.9220) dia=1.5748
		Arc_t* arc1 = create_arc( 62.2300,-10.9220, 1.5748 );
		Arc_t* arc2 = create_arc( 59.6900,-10.9220, 1.5748 );

		add_item2cont( (Refitem_t*) arc1, cont1 );
		add_item2cont( (Refitem_t*) arc2, cont2 );
		ASSERT_TRUE( cont1 != NULL );
		ASSERT_TRUE( cont2 != NULL );

		cont_reorder(cont1, -1);
		cont_reorder(cont2, -1);

		Refholder_t* list = split_all(0);
		int points = list_len(list);
		ASSERT_EQ_INT( 2,  points );

		find_areas( list );
		clean_conts_by_list( &list );
		//calc_contcount4all(0, 0);
		//marking_of_imposed(); // Маркируем совпадающие грани контуров
		//clean_all_cont();
		find_all_conts();

		//walk_around_all_cont("svg", stdout);
		free_context_by_name(ctx->name);
	});

	IT("Два контура Не пересекаются", {
		Context_t* ctx = create_context("tmp");
		Cont_t* cont1 = create_cont();
		Cont_t* cont2 = create_cont();
		Arc_t* arc1 = create_arc( 3, 3, 2 );
		Arc_t* arc2 = create_arc( 3, 3, 3 );
		add_item2cont( (Refitem_t*) arc1, cont1 );
		add_item2cont( (Refitem_t*) arc2, cont2 );
		ASSERT_TRUE( cont1 != NULL );
		ASSERT_TRUE( cont2 != NULL );
		free_context_by_name(ctx->name);
	});


	IT("Два контура касаются гранью", {
		Context_t* ctx = create_context("tmp");
		Cont_t* cont1 = create_cont();
		Cont_t* cont2 = create_cont();
		ASSERT_TRUE( cont1 != NULL );
		ASSERT_TRUE( cont2 != NULL );

		Point_t* a = create_p( 1, 0 );
		Point_t* b = create_p( 7, 0 );
		Point_t* c = create_p( 4, -4 );

		Line_t* l21 = create_line( a,  b );
		Line_t* l22 = create_line( b,  c );
		Line_t* l23 = create_line( c,  a );
		add_item2cont( (Refitem_t*) l21, cont1 );
		add_item2cont( (Refitem_t*) l22, cont1 );
		add_item2cont( (Refitem_t*) l23, cont1 );
		cont_reorder(cont1, -1);

		int dir = cont_dir(cont1);
		ASSERT_EQ_INT( -1,  dir );

		Point_t* d = create_p( 0, 0 );
		Point_t* e = create_p( 8, 0 );
		Point_t* f = create_p( 4, 4 );

		Line_t* l31 = create_line( d, f );
		Line_t* l32 = create_line( f, e );
		Line_t* l33 = create_line( e, d );

		add_item2cont( (Refitem_t*) l31, cont2 );
		add_item2cont( (Refitem_t*) l32, cont2 );
		add_item2cont( (Refitem_t*) l33, cont2 );
		cont_reorder(cont2, -1);

		dir = cont_dir(cont2);
		ASSERT_EQ_INT( -1,  dir );

		Refholder_t* list = split_all(0);
		int points = list_len(list);
		//printf("points = %i\n", points);
		find_areas( list );



		clean_conts_by_list( &list );
		find_all_conts();

//		calc_contcount4all(0, 0); // Подсчитываем contcount
//		marking_of_imposed(); // Маркируем совпадающие грани контуров
//		clean_all_cont(); // удаляем помеченное на удаление (contcount & 1) == 1
//		find_all_conts();

		int contcount = obj_count( (Refitem_t*) ctx, OBJ_TYPE_CONTUR );
		ASSERT_EQ_INT( 1,  contcount );

		//walk_around_all_cont("svg", stdout);


		free_context_by_name(ctx->name);
	});



	IT("Контур из двух дуг. Восстановление порядка следования сегментов", {
		Context_t* ctx = create_context("tmp");
		Cont_t* cont1 = create_cont();
		Arc_t* arc1 = create_arc( 4, 0, 5 );
		Arc_t* arc2 = create_arc( -4, 0, 5 );

		Point_t* a = create_p( 0, 3 );
		Point_t* b = create_p( 0, -3 );

		add_item2cont( (Refitem_t*) arc1, cont1 );
		add_item2cont( (Refitem_t*) arc2, cont1 );

		break_the_circle( arc1, a, -1 );
		break_the_circle( arc2, b, 1 );

		Arc_t* rest_of_arc1 = split_arc_by_p( arc1, b );
		Arc_t* rest_of_arc2 = split_arc_by_p( arc2, a );

		remove_arc( &rest_of_arc1 );
		remove_arc( &rest_of_arc2 );

		int dir = cont_dir( cont1 );

		ASSERT_EQ_INT( -1, dir );

		cont_reorder( cont1, 1 );			// Выворачиваем мехом наружу

		dir = cont_dir( cont1 );

		ASSERT_EQ_INT( 1, dir );
		ASSERT_EQ_INT( 1, cont1->dir );
		ASSERT_EQ_INT( 3, cont1->links.count );

		clean_all_cont();
		find_all_conts();

		cont1 = first_cont( ctx );
		dir = cont_dir( cont1 );
		ASSERT_EQ_INT( 1, dir );

//		walk_around_all_cont("svg", stdout);

		free_context_by_name(ctx->name);
	});

	IT("Контур из дуги и отрезка. Определение/восстановление направления контура", {
		Context_t* ctx = create_context("tmp");
		Cont_t* cont1 = create_cont();
		Arc_t* arc1 = create_arc( 4, 0, 5 );
		Point_t* a = create_p( 0, 3 );
		Point_t* b = create_p( 0, -3 );
		add_item2cont( (Refitem_t*) arc1, cont1 );
		break_the_circle( arc1, a, -1 );
		Arc_t* rest_of_arc1 = split_arc_by_p( arc1, b );
		remove_arc( &rest_of_arc1 );
		Line_t* line1 = create_line( b, a );
		add_item2cont( (Refitem_t*) line1, cont1 );
		int dir = cont_dir( cont1 );
		ASSERT_EQ_INT( -1, dir );
		cont_reorder( cont1, 1 );			// Выворачиваем мехом наружу
		dir = cont_dir( cont1 );
		ASSERT_EQ_INT( 1, dir );
		free_context_by_name(ctx->name);
	});

	IT("Контур из одной замкнутой дуги. Определение/восстановление направления контура", {
		Context_t* ctx = create_context("tmp");
		Cont_t* cont1 = create_cont();
		Arc_t* arc1 = create_arc( 4, 0, 5 );
		Point_t* a = create_p( 0, 3 );
		break_the_circle( arc1, a, -1 );
		add_item2cont( (Refitem_t*) arc1, cont1 );
		int dir = cont_dir( cont1 );
		ASSERT_EQ_INT( -1, dir );
		cont_reorder( cont1, 1 );			// Выворачиваем мехом наружу
		dir = cont_dir( cont1 );
		ASSERT_EQ_INT( 1, dir );
		free_context_by_name(ctx->name);
	});
	CONTEXT("Контекст");

	IT("Два контекста по контуру в каждом. Копируем.", {
		Context_t* ctx1 = create_context("tmp1");
		Cont_t* cont1 = create_cont();
		Arc_t* arc1 = create_arc( 3, 3, 2 );
		add_item2cont( (Refitem_t*) arc1, cont1 );
		Context_t* ctx2 = create_context("tmp2");
		Cont_t* cont2 = create_cont();
		Arc_t* arc2 = create_arc( 3, 3, 3 );
		add_item2cont( (Refitem_t*) arc2, cont2 );
		copy_ctx2ctx("tmp1", "tmp2", NULL);
		select_context("tmp2");
		Context_t* ctx = get_context();
		ASSERT_EQ_INT( 5,  ctx->links.count ); // одна ссылка на себя, две ссылки на контуры и две ссылки на окружности.
//		walk_around_all_cont("svg", stdout);
		free_context_by_name(ctx1->name);
		free_context_by_name(ctx2->name);
	});


	CONTEXT("Gerber");

	IT("Рисуем круглым инструментом", {
		Context_t* ctx = create_context("tmp");
		Cont_t* cont = NULL;
		line_milling( 0, 0, 25, 25, 5, -1, &cont );
		if( !cont ){
			printf(" = нет контуров = \n");
		}else{
			ASSERT_EQ_INT( 5, cont->links.count );
//			walk_around_all_cont("svg", stdout);
		}
		free_context_by_name(ctx->name);
	});


	IT("Рисуем круглым инструментом2", {
		Context_t* ctx = create_context("tmp");
		Cont_t* cont = NULL;
		line_milling( 62.2300, -10.9220, 62.2300, -10.9220, 1.5748, -1, &cont );
		ASSERT_TRUE( cont != NULL );
		if( !cont ){
			printf(" = нет контуров = \n");
		}else{
			ASSERT_EQ_INT( 2, cont->links.count );
//			walk_around_all_cont("svg", stdout);
		}
		free_context_by_name(ctx->name);
	});

	IT("Рисуем круглым инструментом3", {
		Context_t* ctx = create_context("tmp");
		Cont_t* cont = NULL;
		line_milling( 62.2300, -10.9220, 63.2300, -11.9220, 1.5748, -1, &cont );
		ASSERT_TRUE( cont != NULL );
		if( !cont ){
			printf(" = нет контуров = \n");
		}else{
			ASSERT_EQ_INT( 5, cont->links.count );
//			walk_around_all_cont("svg", stdout);
		}
		free_context_by_name(ctx->name);
	});

	IT("Рисуем круглым инструментом4", {
		// CIRCLE: (59.6900,-10.9220) -> (59.6900,-10.9220) dia=1.5748
		Context_t* ctx = create_context("tmp");
		Cont_t* cont = NULL;
		line_milling( 59.6900, -10.9220, 59.6900, -10.9220, 1.5748, -1, &cont );
		ASSERT_TRUE( cont != NULL );
		if( !cont ){
			printf(" = нет контуров = \n");
		}else{
			ASSERT_EQ_INT( 2, cont->links.count );
//			walk_around_all_cont("svg", stdout);
		}
		free_context_by_name(ctx->name);
	});


	IT("Рисуем квадратным инструментом. По диагонали.", {
		Context_t* ctx = create_context("tmp");
		Cont_t* cont = ra_line( 0, 0, 3, 3, 10, 10, 1 );
		ASSERT_TRUE( cont != NULL );
		if( !cont ){
			printf(" = нет контуров = \n");
		}else{
			ASSERT_EQ_INT( 7, cont->links.count );
			ASSERT_EQ_INT( 1, cont->dir );
//			walk_around_all_cont("svg", stdout);
		}
		free_context_by_name(ctx->name);
	});

	IT("Рисуем квадратным инструментом. По горизонтали.", {
		Context_t* ctx = create_context("tmp");
		Cont_t* cont = ra_line( 0, 0, 30, 0, 10, 10, -1 );
		ASSERT_TRUE( cont != NULL );
		if( !cont ){
			printf(" = нет контуров = \n");
		}else{
			ASSERT_EQ_INT( 5, cont->links.count );
			ASSERT_EQ_INT( -1, cont->dir );
//			walk_around_all_cont("svg", stdout);
		}
		free_context_by_name(ctx->name);
	});

	IT("Рисуем нечто подобное дорожки и контактной площадки. ", {
		Context_t* ctx = create_context("tmp");
		Cont_t* cont1 = NULL;
		line_milling( 0, 0, 0, 25, 1, -1, &cont1 );
		Cont_t* cont2 = NULL;
		line_milling( 0, 25, 25, 50, 1, -1, &cont2 );

		Refholder_t* list;

		list = split_all(0);
		find_areas( list );
		clean_conts_by_list( &list );
		find_all_conts();

		Cont_t* cont3 = ra_line( 25, 50, 25, 50, 3, 3, -1 );

		list = split_all(0);
		find_areas( list );
		clean_conts_by_list( &list );
		find_all_conts();

		Cont_t* cont4 = ra_line( 0, 0, 0, 0, 3, 3, -1 );
		Cont_t* cont44 = create_cont();
		Arc_t* arc1 = create_arc( 0, 0, 1 );
		add_item2cont( (Refitem_t*) arc1, cont44 );
		cont44->dir= -1;

		list = split_all(0);
		find_areas( list );
		clean_conts_by_list( &list );
		find_all_conts();
		//printf("\n");
		//walk_around_all_cont("svg", stdout);

		free_context_by_name(ctx->name);
	});

	IT("Рисуем два круга, один мехом внутрь, другой наружу. Объединяем.", {
		Context_t* ctx = create_context("tmp");
		Cont_t* cont1 = create_cont();
		Arc_t* arc1 = create_arc( 0, 0, 3 );
		add_item2cont( (Refitem_t*) arc1, cont1 );
		cont_reorder(cont1, -1);

		Cont_t* cont2 = create_cont();
		Arc_t* arc2 = create_arc( 3, 0, 4 );
		add_item2cont( (Refitem_t*) arc2, cont2 );
		cont_reorder(cont2, 1);

		Refholder_t* list = split_all(0);
		find_areas( list );
		clean_conts_by_list( &list );
		find_all_conts();

//		printf("\n");
//		walk_around_all_cont("svg", stdout);

		free_context_by_name(ctx->name);
	});

	IT("Рисуем квадрат, и фрезеруем его по контуру радиусом r", {
		Context_t* ctx_dst = create_context("dst");
		Context_t* ctx = create_context("tmp");
		Cont_t* cont = ra_line( 0, 0, 0, 0, 5, 5, -1 );

		outline_milling( cont, ctx_dst, 0.5 );
		select_context(ctx_dst->name);

		ASSERT_EQ_INT( 1, obj_count( (Refitem_t*) ctx_dst, OBJ_TYPE_CONTUR) );

//		printf("\n");
//		walk_around_all_cont("svg", stdout);
		free_context_by_name(ctx_dst->name);
		free_context_by_name(ctx->name);
	});



	IT("Рисуем круг и кольцо и пытаемся это объеденить", {
		Context_t* dst = create_context("dst");
		Context_t* ctx = create_context("tmp");
		Cont_t* cont1 = create_cont();
		Cont_t* cont2 = create_cont();
		Arc_t* arc1 = create_arc( 0, 0, 7 );
		Arc_t* arc2 = create_arc( 0, 0, 4 );
		add_item2cont( (Refitem_t*) arc1, cont1 );
		add_item2cont( (Refitem_t*) arc2, cont2 );
		cont_reorder(cont1, -1);
		cont_reorder(cont2, 1);
		outline_milling( cont1, dst, 0.5 );
		outline_milling( cont2, dst, 0.5 );
		select_context( dst->name );
		int contcount = obj_count( (Refitem_t*) ctx, OBJ_TYPE_CONTUR );
		ASSERT_EQ_INT( 2, contcount );

//		printf("\n");
//		walk_around_all_cont("svg", stdout);
		free_context_by_name(dst->name);
		free_context_by_name(ctx->name);
	});

	IT("Фрезеровка полной окружности", {
		Context_t* ctx = create_context("tmp");
		Context_t* dst = create_context("dst");

		Cont_t* cont1 = create_cont();
		Cont_t* cont2 = create_cont();

		Arc_t* arc1 = create_arc( 0, 0, 5 );
		add_item2cont( (Refitem_t*) arc1, cont1 );
		cont_reorder(cont1, -1);

		Arc_t* arc2 = create_arc( 0, 0, 3 );
		add_item2cont( (Refitem_t*) arc2, cont2 );
		cont_reorder(cont2, 1);

		outline_milling( cont1, dst, 0.5 );
		select_context( dst->name );
		split_all(0);
		calc_contcount4all(0, 0);
		marking_of_imposed();

		clean_all_cont();
		find_all_conts();

		ASSERT_EQ_INT( 1, obj_count( (Refitem_t*) dst, OBJ_TYPE_CONTUR) );

//		printf("\n");
//		walk_around_all_cont("svg", stdout);
		free_context_by_name(dst->name);
		free_context_by_name(ctx->name);
	});


	IT("Рисуем четыре круга, мехом внутрь/наружу через один. Объединяем.", {
		Context_t* ctx = create_context("tmp");
		int dir;
		// task1
		Cont_t* cont1_1 = create_cont();
		Cont_t* cont1_2 = create_cont();
		Cont_t* cont1_3 = create_cont();
		Cont_t* cont1_4 = create_cont();

		Arc_t* arc1_1 = create_arc( 0, 0, 20 );
		break_the_circle( arc1_1, create_p(arc1_1->center.x, arc1_1->center.y + arc1_1->R), 1);

		Arc_t* arc1_2 = create_arc( 0, 0, 15 );
		break_the_circle( arc1_2, create_p(arc1_2->center.x, arc1_2->center.y + arc1_2->R), 1);

		Arc_t* arc1_3 = create_arc( 0, 0, 10 );
		break_the_circle( arc1_3, create_p(arc1_3->center.x, arc1_3->center.y + arc1_3->R), 1);

		Arc_t* arc1_4 = create_arc( 0, 0, 5 );
		break_the_circle( arc1_4, create_p(arc1_4->center.x, arc1_4->center.y + arc1_4->R), 1);

		add_item2cont( (Refitem_t*) arc1_1, cont1_1 );
		add_item2cont( (Refitem_t*) arc1_2, cont1_2 );
		add_item2cont( (Refitem_t*) arc1_3, cont1_3 );
		add_item2cont( (Refitem_t*) arc1_4, cont1_4 );

		dir = cont_reorder( cont1_1, -1 );
		ASSERT_EQ_INT( -1, dir );

		dir = cont_reorder(cont1_2, 1);
		ASSERT_EQ_INT( 1, dir );

		dir = cont_reorder(cont1_3, -1);
		ASSERT_EQ_INT( -1, dir );

		dir = cont_reorder(cont1_4, 1);
		ASSERT_EQ_INT( 1, dir );

		split_all(0);

		clean_all_cont();
		find_all_conts();

		calc_contcount4all(0, 0);
		marking_of_imposed(); // Маркируем совпадающие грани контуров

		clean_all_cont();
		find_all_conts();

		// task2
		Cont_t* cont2_1 = create_cont();
		Cont_t* cont2_2 = create_cont();

		Arc_t* arc2_1 = create_arc( -40, 40, 15 );
		Arc_t* arc2_2 = create_arc( -35, 40, 15 );

		add_item2cont( (Refitem_t*) arc2_1, cont2_1 );
		add_item2cont( (Refitem_t*) arc2_2, cont2_2 );

		dir = cont_reorder(cont2_1, -1);
		ASSERT_EQ_INT( -1, dir );
		dir = cont_reorder(cont2_2, 1);
		ASSERT_EQ_INT( 1, dir );
		split_all(0);
		calc_contcount4all(0, 0);
		marking_of_imposed(); // Маркируем совпадающие грани контуров
		clean_all_cont();
		find_all_conts();
		int contcount = obj_count( (Refitem_t*) ctx, OBJ_TYPE_CONTUR );
		ASSERT_EQ_INT( 5, contcount );

//		printf("\n");
//		walk_around_all_cont("svg", stdout);

		free_context_by_name(ctx->name);
	});


	IT("Рисуем круглым инструментом по дуге (обводим контур)", {
		Context_t* ctx2 = create_context("default");
		Cont_t* cont43 = create_cont();

		double R = distance(1, 4, 0, 0);
		Arc_t* arc = create_arc( 1, 4, R );
		add_item2cont( (Refitem_t*) arc, cont43 );

		Point_t* a = create_p( 0, 0 );
		Point_t* b = create_p( 1 + R, 4 );

		break_the_circle( arc, a, 1);
		Arc_t* new_arc = split_arc_by_p( arc, b );
		remove_arc( &new_arc );

		Context_t* ctx1 = create_context("tmp");
		select_context("tmp");

		Cont_t* cont44 = NULL;
		Cont_t* cont45 = NULL;
		arc_milling( arc, 0.5, -1, &cont44, &cont45 ); // Фрезеруем

		if( cont44 ) copy_ctx2ctx("tmp", "default", cont44);
		if( cont45 ) copy_ctx2ctx("tmp", "default", cont45);
		select_context("default");

		remove_arc( &arc );

		split_all(0);

		calc_contcount4all(0, 0);
		marking_of_imposed();

		clean_all_cont();
		find_all_conts();

		Cont_t* cont = first_cont(ctx2);
		ASSERT_TRUE( cont != NULL );
		if( cont ) ASSERT_EQ_INT( 5, cont->links.count );

		//	printf("\n");
		//	walk_around_all_cont("svg", stdout);
		free_context_by_name(ctx2->name);
		free_context_by_name(ctx1->name);
	});

	IT("Фрезеруем дорожку, и дополнительно фрезеруем его по контуру", {
		Context_t* dst = create_context("dst");
		Context_t* ctx = create_context("tmp");
		Cont_t* cont = NULL;
		line_milling( 0, 0, 25, 25, 5, -1, &cont );
		outline_milling( cont, dst, 1 );
		select_context( dst->name );
		ASSERT_EQ_INT( 1, obj_count( (Refitem_t*) dst, OBJ_TYPE_CONTUR) );

		//printf("\n");
		//walk_around_all_cont("svg", stdout);

		free_context_by_name(dst->name);
		free_context_by_name(ctx->name);
	});

}
