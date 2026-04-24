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


	CONTEXT("Gerber");

	IT("Рисуем круглым инструментом", {
		Context_t* ctx = create_context("tmp");
		Cont_t* cont = NULL;
		line_milling( 0, 0, 25, 25, 5, 1, &cont );
		if( !cont ){
			printf(" = нет контуров = \n");
		}else{
			ASSERT_EQ_INT( 5, cont->links.count );
			walk_around_all_cont("svg", stdout);
		}
		free_context_by_name(ctx->name);
	});


//walk_around_all_cont("svg", stdout);


}
