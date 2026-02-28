#ifndef EPSILON_H
#define EPSILON_H
extern const double epsilon;
#endif

#include <stdint.h>

double points_distance(Point_t* a, Point_t* b);
double distance( double ax, double ay, double bx, double by );
double normalize_angle(double angle);
Point_t* create_p_of_line_mid(Point_t* a, Point_t* b);
void middle( double ax, double ay, double bx, double by, double* px, double* py );
uint8_t is_point_on_circle(double center_x, double center_y, double radius, double x, double y);

uint8_t is_p_on_arc_geom(double cx, double cy, double R, double ax, double ay, double bx, double by, int dir, double px, double py);
uint8_t p_of_line_x_line(double ax, double ay, double bx, double by, double cx, double cy, double dx, double dy, double* x , double* y );
uint8_t xy_of_line_x_line(double ax, double ay, double bx, double by, double cx, double cy, double dx, double dy, double* x1, double* y1, double* x2, double* y2);

uint8_t get_xy_of_arc_mid( Arc_t* arc, double* x, double* y );
uint8_t create_px4_of_rect_by_2p( double ax, double ay, double bx, double by, double R, double* cx, double* cy, double* dx, double* dy, double* ex, double* ey, double* fx, double* fy);
uint8_t arc_dir_by_xy( Arc_t* arc, double ax, double ay, double bx, double by, double cx, double cy );
uint8_t is_xy_on_line(double ax, double ay, double bx, double by, double px, double py );

uint8_t xy_eq_xy( double x1, double y1, double x2, double y2 );

uint8_t is_point_in_ray_neighborhood(double rx, double ry, double mx, double my, double ax, double ay);

double p_to_line_distance(double x, double y, double ax, double ay, double bx, double by);

double round_to_decimal(double value, int decimals);

double angle_factor(Point_t* from, Point_t* at, Point_t* to);

