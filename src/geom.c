#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>
#include <math.h>

#include "../include/top.h"
#include "../include/reflist.h"
#include "../include/point.h"
#include "../include/arc.h"
#include "../include/line.h"
#include "../include/geom.h"
#include "../include/context.h"

//const double epsilon = 0.00001f; // единное место определения

const double epsilon = 0.00001f; // единное место определения

double round_to_decimal(double value, int decimals) {
    double factor = pow(10, decimals);
    return round(value * factor) / factor;
}

uint8_t xy_eq_xy( double x1, double y1, double x2, double y2 ){
	return ( ( ((x1)-(x2))*((x1)-(x2)) + ((y1)-(y2))*((y1)-(y2)) ) < (epsilon * epsilon/2) );
}

uint8_t xy_ne_xy( double x1, double y1, double x2, double y2 ){
	return !( ( ((x1)-(x2))*((x1)-(x2)) + ((y1)-(y2))*((y1)-(y2)) ) < (epsilon * epsilon/2) );
}

/*
* вычисление расстояния между двумя заданными точками
*/
double points_distance(Point_t* a, Point_t* b){
	return distance( a->x, a->y, b->x, b->y );
}

/*
* вычисление расстояния между двумя точками, заданными их координатами
*/
double distance( double ax, double ay, double bx, double by ){
	return sqrt((bx - ax) * (bx - ax) + (by - ay) * (by - ay));
}

/*
*вспомогательная функция для arc_dir (нормализация угла - приведение угла к диапазону 0..2Pi)
*/
double normalize_angle(double angle){
	double normalized = fmodf(angle, 2 * M_PI);
	if (normalized < 0.0) {
		normalized += 2 * M_PI;
	}
	return normalized;
}

/*
* Вычисление середины отрезка, заданного координатами
*/
void middle( double ax, double ay, double bx, double by, double* px, double* py ){
	*px = (ax + bx) / 2.0;
	*py = (ay + by) / 2.0;
}

/*
* Проверка принадлежности координат px, py отрезку (ax, ay)-(bx, by);
*/
uint8_t is_xy_on_line(double ax, double ay, double bx, double by, double px, double py ){
	if( xy_eq_xy( ax, ay, px, py ) || xy_eq_xy( bx, by, px, py ) ) return 1;
	double cross = (bx - ax) * (py - ay) - (by - ay) * (px - ax);
	// допускаем маленькую погрешность для double
	if( fabs(cross) > epsilon ) return 0;
	// 2. Проверка, что p лежит в пределах отрезка
	if(px < fmin(ax, bx) - epsilon || px > fmax(ax, bx) + epsilon) return 0;
	if(py < fmin(ay, by) - epsilon || py > fmax(ay, by) + epsilon) return 0;
	return 1;
}

/*
* Определение попадание точки на окружность
*/
uint8_t is_point_on_circle(double center_x, double center_y, double radius, double x, double y){
	double dist = distance(center_x, center_y, x, y );
	return (fabs(dist - radius) <= (epsilon/2))?1:0;
}

/*
* Проверка принадлежности точки дуге (геометрическая)
*/
/*
uint8_t is_p_on_arc_geom(double cx, double cy, double R, double ax, double ay, double bx, double by, int dir, double px, double py){
	double d = hypot(px - cx, py - cy);
	if (fabs(d - R) > epsilon){
		return 0;
	}
	// 2. Вычисляем углы
	if( ((ax==bx) && (ay==by)) ) return 1; // если концы дуги совпадают, то любая точка принадлежащая окружности принадлежит и дуге
	double angA = normalize_angle(atan2(ay - cy, ax - cx));
	double angB = normalize_angle(atan2(by - cy, bx - cx));
	double angP = normalize_angle(atan2(py - cy, px - cx));

	if( dir != 0) {
		if( xy_eq_xy(ax, ay, px, py) ) return 1;
		if( xy_eq_xy(bx, by, px, py) ) return 1;
	}

	if(dir == 1) { // против часовой
		if (angA <= angB)
			return angA - epsilon <= angP && angP <= angB + epsilon;
		else
			return angP >= angA - epsilon || angP <= angB + epsilon;
	}else if (dir == -1) { // по часовой
		if (angB <= angA)
			return angB - epsilon <= angP && angP <= angA + epsilon;
		else
			return angP >= angB - epsilon || angP <= angA + epsilon;
	}
	return 1; // Полная окружность.
}
*/
uint8_t is_p_on_arc_geom(double cx, double cy, double R, double ax, double ay, double bx, double by, int dir, double px, double py) {
    // 1. Проверка на принадлежность окружности
    double d = hypot(px - cx, py - cy);
    if (fabs(d - R) > epsilon) return 0;

    // 2. Полная окружность
    if (dir == 0) return 1;

    // 3. ПРОВЕРКА ЧЕРЕЗ ФИЗИЧЕСКИЕ КОНЦЫ (Snapping logic)
    // Если точка физически близка к концам A или B, она ГАРАНТИРОВАННО на дуге.
    // Это устраняет ошибки округления atan2 на границах.
    if (xy_eq_xy(px, py, ax, ay) || xy_eq_xy(px, py, bx, by)) return 1;

    // 4. Вычисляем углы для проверки внутренней части дуги
    if (ax == bx && ay == by) return 1;

    double angA = normalize_angle(atan2(ay - cy, ax - cx));
    double angB = normalize_angle(atan2(by - cy, bx - cx));
    double angP = normalize_angle(atan2(py - cy, px - cx));

    // 5. Проверка углового диапазона
    // Здесь epsilon тоже важен, но пункт 3 уже подстраховал края.
    if (dir == 1) { // против часовой
        if (angA <= angB)
            return (angP >= angA - 1e-10) && (angP <= angB + 1e-10);
        else
            return (angP >= angA - 1e-10) || (angP <= angB + 1e-10);
    } else if (dir == -1) { // по часовой
        if (angB <= angA)
            return (angP >= angB - 1e-10) && (angP <= angA + 1e-10);
        else
            return (angP >= angB - 1e-10) || (angP <= angA + 1e-10);
    }
    return 1;
}



/*
* вычисление точки пересечения отрезков (геометрическое)
*/
uint8_t p_of_line_x_line(double ax, double ay, double bx, double by, double cx, double cy, double dx, double dy, double* x, double* y ){
	double D = (ax - bx)*(cy - dy) - (ay - by) * (cx - dx);
	if( D == 0 ) return 0;
	double t = ((ax - cx)*(cy - dy) - (ay - cy)*(cx - dx))/D;
	double u = ((ax - bx)*(ay - cy) - (ay - by)*(ax - cx))/D;
	if( fabs(u)>1 ) return 0;
	if( fabs(t)>1 ) return 0;
	*x = ax + t * (bx - ax);
	*y = ay + t * (by - ay);
	if( !is_xy_on_line( ax, ay, bx, by, *x, *y ) ) return 0;
	if( !is_xy_on_line( cx, cy, dx, dy, *x, *y ) ) return 0;
	return 1;
}

/*
* вычисление точек пересечения отрезков (геометрическое)
* работать должно так:
* отрезок A(0,0) - B(3,3) пересекает C(3,0) - D(0,3) в т.P1 (1.5,1.5)
* отрезок A(0,0) - B(3,3) пересекает C(3,0) - D(6,3) нет пересечений
* отрезок A(0,0) - B(3,0) пересекает C(3,0) - D(6,0) нет пересечений
* отрезок A(0,0) - B(3,0) пересекает C(3,0) - D(0,0) нет пересечений
* отрезок A(0,0) - B(3,0) пересекает C(3,0) - D(1,0) в т.P1 (1,0)
* отрезок A(0,0) - B(6,0) пересекает C(2,0) - D(4,0) в т.P1 (2,0) и т.P2 (4,0)
* отрезок A(0,0) - B(2,0) пересекает C(1,0) - D(3,0) в т.P1 (1,0) и т.P2 (2,0)
*/

/*
// Проверка, совпадают ли две точки
static int same_point(double x1, double y1, double x2, double y2) {
    return (fabs(x1 - x2) < epsilon) && (fabs(y1 - y2) < epsilon);
}

// Проверка: является ли точка общим концом обоих отрезков
static int is_common_endpoint( double px, double py,  double ax, double ay, double bx, double by, double cx, double cy, double dx, double dy){
    int is_end_ab = same_point(px, py, ax, ay) || same_point(px, py, bx, by);
    int is_end_cd = same_point(px, py, cx, cy) || same_point(px, py, dx, dy);
    return is_end_ab && is_end_cd;
}
*/

uint8_t xy_of_line_x_line( double ax, double ay, double bx, double by, double cx, double cy, double dx, double dy, double *x1, double *y1, double *x2, double *y2){
	*x1=0;
	*y1=0;
	*x2=0;
	*y2=0;
    double ABx = bx - ax;
    double ABy = by - ay;
    double CDx = dx - cx;
    double CDy = dy - cy;
    double ACx = cx - ax;
    double ACy = cy - ay;
    double D = ABx * CDy - ABy * CDx;
    // ============================================================
    // 1. Обычное пересечение (отрезки НЕ параллельны)
    // ============================================================
    if (fabs(D) > epsilon) {
        double t = (ACx * CDy - ACy * CDx) / D;
        double u = (ACx * ABy - ACy * ABx) / D;

		int t_ok = (t >= -epsilon) && (t <= 1.0 + epsilon);
		// u находится в [0, 1] с учетом допуска:
		int u_ok = (u >= -epsilon) && (u <= 1.0 + epsilon);
		if (!t_ok || !u_ok) {
		    return 1; // Точка пересечения вне отрезков (с учетом допусков)
		}
		// Корректировка, чтобы избавиться от ошибок округления для дальнейших сравнений (xy_eq_xy)
//		if (t < 0) t = 0; else if (t > 1) t = 1;
//		if (u < 0) u = 0; else if (u > 1) u = 1;
		if (t < 0 || t > 1 || u < 0 || u > 1){
			double px = ax + t * ABx;
			double py = ay + t * ABy;
			*x1 = px;
			*y1 = py;
			if( xy_eq_xy(px, py, ax, ay) && xy_eq_xy(px, py, cx, cy) ) return 3;
			if( xy_eq_xy(px, py, ax, ay) && xy_eq_xy(px, py, dx, dy) ) return 3;
			if( xy_eq_xy(px, py, bx, by) && xy_eq_xy(px, py, cx, cy) ) return 3;
			if( xy_eq_xy(px, py, bx, by) && xy_eq_xy(px, py, dx, dy) ) return 3;
			if( xy_eq_xy(px, py, ax, ay) && xy_ne_xy(px, py, cx, cy) && xy_ne_xy(px, py, dx, dy) ) return 4;
			if( xy_eq_xy(px, py, bx, by) && xy_ne_xy(px, py, cx, cy) && xy_ne_xy(px, py, dx, dy) ) return 4;
			if( xy_eq_xy(px, py, cx, cy) && xy_ne_xy(px, py, ax, ay) && xy_ne_xy(px, py, bx, by) ) return 4;
			if( xy_eq_xy(px, py, dx, dy) && xy_ne_xy(px, py, ax, ay) && xy_ne_xy(px, py, bx, by) ) return 4;

			return 1; // Точка пересечения вне отрезков
		}

		double px = ax + t * ABx;
		double py = ay + t * ABy;

		*x1 = px;
		*y1 = py;

		// Если точка — общий конец двух отрезков
		if( xy_eq_xy(px, py, ax, ay) && xy_eq_xy(px, py, cx, cy) ) return 3;
		if( xy_eq_xy(px, py, ax, ay) && xy_eq_xy(px, py, dx, dy) ) return 3;
		if( xy_eq_xy(px, py, bx, by) && xy_eq_xy(px, py, cx, cy) ) return 3;
		if( xy_eq_xy(px, py, bx, by) && xy_eq_xy(px, py, dx, dy) ) return 3;
		if( xy_eq_xy(px, py, ax, ay) && xy_ne_xy(px, py, cx, cy) && xy_ne_xy(px, py, dx, dy) ) return 4;
		if( xy_eq_xy(px, py, bx, by) && xy_ne_xy(px, py, cx, cy) && xy_ne_xy(px, py, dx, dy) ) return 4;
		if( xy_eq_xy(px, py, cx, cy) && xy_ne_xy(px, py, ax, ay) && xy_ne_xy(px, py, bx, by) ) return 4;
		if( xy_eq_xy(px, py, dx, dy) && xy_ne_xy(px, py, ax, ay) && xy_ne_xy(px, py, bx, by) ) return 4;

		return 2;
	}
	// ============================================================
	// 2. Параллельные. Проверяем коллинеарность.
	// ============================================================
	if (fabs(ABx * ACy - ABy * ACx) > epsilon) return 5; // Параллельны, но не совпадают по линии

	// ============================================================
	// 3. Коллинеарные. Ищем перекрытие проекций.
	// ============================================================
	double lenAB2 = ABx * ABx + ABy * ABy;
	// Параметры C и D на AB
	double tC = (ACx * ABx + ACy * ABy) / lenAB2;
	double tD = ((dx - ax) * ABx + (dy - ay) * ABy) / lenAB2;
	double tmin = fmin(tC, tD);
	double tmax = fmax(tC, tD);
	// Перекрытие параметров
	double ts = fmax(0.0, tmin);
	double te = fmin(1.0, tmax);

	if (ts > te + epsilon)  return 6; // Перекрытия нет

	// Вычисляем границы перекрытия
	double px1 = ax + ts * ABx;
	double py1 = ay + ts * ABy;
	double px2 = ax + te * ABx;
	double py2 = ay + te * ABy;

	if( xy_eq_xy(px1, py1, px2, py2) ){
		if(
			( xy_eq_xy(px1, py1, ax, ay) && xy_eq_xy(px1, py1, cx, cy) ) ||
			( xy_eq_xy(px1, py1, ax, ay) && xy_eq_xy(px1, py1, dx, dy) ) ||
			( xy_eq_xy(px1, py1, bx, by) && xy_eq_xy(px1, py1, cx, cy) ) ||
			( xy_eq_xy(px1, py1, bx, by) && xy_eq_xy(px1, py1, dx, dy) )
		){
			*x1 = px1;
			*y1 = py1;
			return 7;
		}
	}

	*x1 = px1;
	*y1 = py1;
	*x2 = px2;
	*y2 = py2;
	if(
		(xy_eq_xy(px1, py1, ax, ay) && xy_eq_xy(px2, py2, bx, by)) ||
		(xy_eq_xy(px1, py1, cx, cy) && xy_eq_xy(px2, py2, dx, dy))
	){
		// Режим вписывания
		uint8_t r = 0;
		if(
			( xy_eq_xy(px1, py1, ax, ay) && ( ( xy_eq_xy(px1, py1, cx, cy) || xy_eq_xy(px1, py1, dx, dy) ) ) ) ||
			( xy_eq_xy(px1, py1, bx, by) && ( ( xy_eq_xy(px1, py1, cx, cy) || xy_eq_xy(px1, py1, dx, dy) ) ) )
		){
			//printf("// Есть совпадение со стороны p1\n");
			r = r | 1;
		}
		if(
			( xy_eq_xy(px2, py2, ax, ay) && ( ( xy_eq_xy(px2, py2, cx, cy) || xy_eq_xy(px2, py2, dx, dy) ) ) ) ||
			( xy_eq_xy(px2, py2, bx, by) && ( ( xy_eq_xy(px2, py2, cx, cy) || xy_eq_xy(px2, py2, dx, dy) ) ) )
		){
			//printf("// Есть совпадение со стороны p2\n");
			r = r | 2;
		}
		return 9 + (r?(( (r==1) || (r==2) )?1:2):0);
	}else{
		// Режим наложения 8
		return 8;
	}
	return 13;
}



/*
uint8_t xy_of_line_x_line( double ax, double ay, double bx, double by, double cx, double cy, double dx, double dy, double *x1, double *y1, double *x2, double *y2){
    double ABx = bx - ax;
    double ABy = by - ay;
    double CDx = dx - cx;
    double CDy = dy - cy;
    double D = ABx * CDy - ABy * CDx;
    double ACx = cx - ax;
    double ACy = cy - ay;

    // ---------- 1. Обычный случай (не параллельные) ----------
    if (fabs(D) > epsilon) {
        double t = (ACx * CDy - ACy * CDx) / D;
        double u = (ACx * ABy - ACy * ABx) / D;
        if (t >= 0 && t <= 1 && u >= 0 && u <= 1) {
            *x1 = ax + t * ABx;
            *y1 = ay + t * ABy;
            return 1;
        }
        return 0;
    }

    // ---------- 2. Параллельные! Проверяем коллинеарность ----------
    if (fabs(ABx * ACy - ABy * ACx) > epsilon) return 0; // параллельны, но не лежат на одной прямой

    // ---------- 3. Коллинеарные ----------
    double lenAB2 = ABx * ABx + ABy * ABy;
    // Если AB — точка
    if(lenAB2 < epsilon) {
        // Тогда проверяем C и D как точки
        double distAC2 = (ax - cx)*(ax - cx) + (ay - cy)*(ay - cy);
        double distAD2 = (ax - dx)*(ax - dx) + (ay - dy)*(ay - dy);
        if (distAC2 < epsilon) { *x1 = ax; *y1 = ay; return 1; }
        if (distAD2 < epsilon) { *x1 = ax; *y1 = ay; return 1; }
        return 0;
    }

    // Параметры C и D относительно отрезка AB
    double tC = (ACx * ABx + ACy * ABy) / lenAB2;
    double tD = ((dx - ax) * ABx + (dy - ay) * ABy) / lenAB2;
    double tmin = fmin(tC, tD);
    double tmax = fmax(tC, tD);
    double ts = fmax(0, tmin);
    double te = fmin(1, tmax);

    if(ts > te + epsilon)  return 0; // нет пересечения

    // одна точка (касание)
    if (fabs(ts - te) < epsilon) {
        *x1 = ax + ts * ABx;
        *y1 = ay + ts * ABy;
        return 1;
    }

    // отрезки перекрываются — две точки
    *x1 = ax + ts * ABx;
    *y1 = ay + ts * ABy;
    *x2 = ax + te * ABx;
    *y2 = ay + te * ABy;
    return 2;
}
*/
/*

uint8_t xy_of_line_x_line(double ax, double ay, double bx, double by, double cx, double cy, double dx, double dy, double* x1, double* y1, double* x2, double* y2) {
    // Сначала проверяем общий случай (отрезки не параллельны)
    double D = (ax - bx) * (cy - dy) - (ay - by) * (cx - dx);
    if(fabs(D)>=epsilon){  // D != 0
        // Общий случай - отрезки не параллельны
        double t = ((ax - cx) * (cy - dy) - (ay - cy) * (cx - dx)) / D;
        double u = -((ax - bx) * (ay - cy) - (ay - by) * (ax - cx)) / D;
        if (t >= 0 && t <= 1 && u >= 0 && u <= 1) {
            *x1 = ax + t * (bx - ax);
            *y1 = ay + t * (by - ay);
            return 1; // Одна точка пересечения
        }
        return 0; // Нет пересечения
    }
    // Отрезки параллельны или лежат на одной прямой
    // Проверяем, лежат ли они на одной прямой
    double ABx = bx - ax;
    double ABy = by - ay;
    double ACx = cx - ax;
    double ACy = cy - ay;
    // Если вектор AC не коллинеарен вектору AB, то отрезки на разных прямых
    if (fabs(ABx * ACy - ABy * ACx) > epsilon) return 0; // Параллельны, но на разных прямых
    // Отрезки лежат на одной прямой
    // Нормируем параметры точек на прямой AB
    double lenAB_sq = ABx * ABx + ABy * ABy;
    // Если отрезок AB вырожден в точку
    if (lenAB_sq < epsilon) {
        // Проверяем, совпадает ли точка A с каким-либо из концов CD
        if (fabs(ax - cx) < 1e-10 && fabs(ay - cy) < epsilon) {
            *x1 = ax;
            *y1 = ay;
            return 1;
        }
        if (fabs(ax - dx) < 1e-10 && fabs(ay - dy) < epsilon) {
            *x1 = ax;
            *y1 = ay;
            return 1;
        }
        return 0;
    }
    // Вычисляем параметры точек C и D относительно AB
    double tC = ((cx - ax) * ABx + (cy - ay) * ABy) / lenAB_sq;
    double tD = ((dx - ax) * ABx + (dy - ay) * ABy) / lenAB_sq;
    // Упорядочиваем параметры
    double t_min_CD = fmin(tC, tD);
    double t_max_CD = fmax(tC, tD);
    // Пересечение отрезков [0,1] и [t_min_CD, t_max_CD]
    double t_start = fmax(0, t_min_CD);
    double t_end = fmin(1, t_max_CD);
    if (t_start > t_end) {
        return 0; // Нет пересечения
    }
    if( fabs(t_start - t_end) < epsilon ){
        // Одна точка пересечения
        *x1 = ax + t_start * ABx;
        *y1 = ay + t_start * ABy;
        return 1;
    }else{
        // Две точки пересечения (пересечение отрезками)
        *x1 = ax + t_start * ABx;
        *y1 = ay + t_start * ABy;
        *x2 = ax + t_end * ABx;
        *y2 = ay + t_end * ABy;
        return 2;
    }
}
*/

/*
Поиск середины дуги
*/
uint8_t get_xy_of_arc_mid( Arc_t* arc, double* x, double* y ){
	if (arc->R <= 0) {
		fprintf(stderr, "Ошибка: Радиус должен быть положительным.\n");
		return 0;
	}
	if( (arc->dir != 1) && (arc->dir != -1) ){
		fprintf(stderr, "Ошибка: Неверное направление обхода (должно быть -1 или 1).\n");
		return 0;
	}
	double angle_a = atan2(arc->a->y - arc->center.y, arc->a->x - arc->center.x);
	double angle_b = atan2(arc->b->y - arc->center.y, arc->b->x - arc->center.x);
	double delta_angle; // Угол, который нужно добавить к начальному углу
	double diff = angle_b - angle_a;
	// 2. Корректируем разницу углов в зависимости от направления
	if(arc->dir == 1) {
		if(diff < 0) diff += 2.0 * M_PI;
	}else{
		if(diff > 0) diff -= 2.0 * M_PI;
	}
	delta_angle = diff / 2.0;
	double mid_angle = angle_a + delta_angle;
	*x = arc->center.x + arc->R * cos(mid_angle);
	*y = arc->center.y + arc->R * sin(mid_angle);
	return 1;
}

/*
* Создаём 4 точки описывающие прямоугольник, проведённый так, что заданые точки
* точки a и b делят две его противолежащие стороны длины L попалам.
* ax, ay, bx, by - координаты базового отрезка
* double R - половина длины генерируемых отрезков
* cx, cy, dx, dy, ex, ey, fx, fy - координаты концов результирующих орезков
*/
uint8_t create_px4_of_rect_by_2p( double ax, double ay, double bx, double by, double R, double* cx, double* cy, double* dx, double* dy, double* ex, double* ey, double* fx, double* fy){
	// 1. Вычисляем вектор AB и его длину
	double vec_x = bx - ax;
	double vec_y = by - ay;
	double dist_AB = sqrt(vec_x * vec_x + vec_y * vec_y);
	if (dist_AB == 0) return 0;                 // Если точки совпадают, невозможно определить перпендикулярный вектор.
	double perp_unit_x = -vec_y / dist_AB;
	double perp_unit_y = vec_x / dist_AB;
	*cx = ax + R * perp_unit_x;
	*cy = ay + R * perp_unit_y;
	*dx = ax - R * perp_unit_x;
	*dy = ay - R * perp_unit_y;
	*ex = bx + R * perp_unit_x;
	*ey = by + R * perp_unit_y;
	*fx = bx - R * perp_unit_x;
	*fy = by - R * perp_unit_y;
	return 1;
}



uint8_t arc_dir_by_xy( Arc_t* arc, double ax, double ay, double bx, double by, double cx, double cy ){
	double angleA = atan2(ay - arc->center.y, ax - arc->center.x);
	double angleB = atan2(by - arc->center.y, bx - arc->center.x);
	double angleC = atan2(cy - arc->center.y, cx - arc->center.x);
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

typedef struct {
    int large_arc_flag; // 0 или 1
    int sweep_flag;     // 0 или 1
} SvgArcFlags;


/**
 * Возвращает 1, если луч RM проходит через окрестность точки A радиусом epsilon.
 * Возвращает 0 в противном случае.
 */
uint8_t is_point_in_ray_neighborhood(double rx, double ry, double mx, double my, double ax, double ay){
    // Вектор луча V = M - R
    double vx = mx - rx;
    double vy = my - ry;
    // Вектор от начала луча до точки A: W = A - R
    double wx = ax - rx;
    double wy = ay - ry;
    // 1. Проверка направления (скалярное произведение W * V)
    // Это определяет, находится ли точка "впереди" начала луча.
    // Если t_numerator < 0, точка находится "позади" точки R.
    double t_numerator = wx * vx + wy * vy;
    // Если точка сзади луча, проверяем расстояние до начальной точки R
    if (t_numerator < 0) {
        double dist_sq = wx * wx + wy * wy;
        return dist_sq < (epsilon * epsilon/2.0);
    }
    // 2. Нахождение расстояния от точки до прямой.
    // Используем формулу: d = |det(V, W)| / |V|
    // Квадрат определителя (псевдоскалярное произведение в 2D)
    double det = vx * wy - vy * wx;
    double det_sq = det * det;
    // Квадрат длины вектора луча V
    double v_mag_sq = vx * vx + vy * vy;
    // d^2 = det^2 / |V|^2
    // Чтобы избежать деления на ноль (если R и M совпали), добавим проверку
    if (v_mag_sq < 1e-18) { // Очень малая величина
        double dist_sq = wx * wx + wy * wy;
        return dist_sq < (epsilon * epsilon/2.0);
    }
    // Проверка условия d < epsilon  =>  d^2 < epsilon^2
    // Переносим знаменатель: det_sq < epsilon^2 * v_mag_sq
    return det_sq < (epsilon * epsilon * v_mag_sq/2.0);
}



/**
 * Возвращает ориентированное расстояние от точки (x,y) до прямой AB.
 * Положительное значение — точка слева от вектора AB.
 * Отрицательное значение — точка справа от вектора AB.
 * Абсолютное значение — фактическое расстояние.
 */
double p_to_line_distance(double x, double y, double ax, double ay, double bx, double by) {
    // Длина отрезка AB (основание треугольника)
    double dx = bx - ax;
    double dy = by - ay;
    double AB_len = sqrt(dx * dx + dy * dy);
    // Если точки A и B совпали, возвращаем расстояние до точки A
    if (AB_len < 1e-12) { // Используем очень малый EPS
        double dpx = x - ax;
        double dpy = y - ay;
        return sqrt(dpx * dpx + dpy * dpy);
    }
    // Векторное произведение (Z-компонента) векторов AB и AP
    // Это удвоенная площадь треугольника ABP (со знаком)
    double cross_product = (bx - ax) * (y - ay) - (by - ay) * (x - ax);
    // Расстояние h = (2 * Площадь) / Основание
    return cross_product / AB_len;
}
