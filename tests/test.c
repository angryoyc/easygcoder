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


	CONTEXT("Contour merge");

	IT("Два разнонаправленных контура пересекаются", {
		Context_t* ctx = create_context("tmp");

		Cont_t* cont1 = create_cont();
		Point_t* a = create_p( 0, 4 );
		Point_t* b = create_p( 0, 8 );
		Point_t* c = create_p( 4, 8 );
		Point_t* d = create_p( 4, 4 );
		Line_t* l1 = create_line( a,  b );
		Line_t* l2 = create_line( b,  c );
		Line_t* l3 = create_line( c,  d );
		Line_t* l4 = create_line( d,  a );
		add_item2cont((Refitem_t*) l1, cont1);
		add_item2cont((Refitem_t*) l2, cont1);
		add_item2cont((Refitem_t*) l3, cont1);
		add_item2cont((Refitem_t*) l4, cont1);
		cont_reorder(cont1, -1);


		Cont_t* cont2 = create_cont();
		Point_t* e = create_p( 2, 2 );
		Point_t* f = create_p( 2, 6 );
		Point_t* g = create_p( 6, 6 );
		Point_t* h = create_p( 6, 2 );
		Line_t* m1 = create_line( e,  f );
		Line_t* m2 = create_line( f,  g );
		Line_t* m3 = create_line( g,  h );
		Line_t* m4 = create_line( h,  e );
		add_item2cont((Refitem_t*) m1, cont2);
		add_item2cont((Refitem_t*) m2, cont2);
		add_item2cont((Refitem_t*) m3, cont2);
		add_item2cont((Refitem_t*) m4, cont2);
		cont_reorder(cont2, -1);

		Refholder_t* list = split_all(0);
		int points = list_len(list);
		ASSERT_EQ_INT( 2,  points );
		find_areas( list );

		walk_around_all_cont("svg", stdout);

		clean_conts_by_list( &list );
		clean_all_cont();
		find_all_conts();

		walk_around_all_cont("svg", stdout);

		free_context_by_name(ctx->name);
	});


}
