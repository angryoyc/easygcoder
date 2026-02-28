
static int prim_counter = 1;
static int debugmode = 0;

typedef enum {
	OBJ_TYPE_CONTEXT,
	OBJ_TYPE_POINT,
	OBJ_TYPE_LINE,
	OBJ_TYPE_ARC,
	OBJ_TYPE_CONTUR
} Obj_Type_t;

typedef struct Reflist Reflist_t;
typedef struct Refitem Refitem_t;
typedef struct Point Point_t;
typedef struct Arc Arc_t;
typedef struct Cont Cont_t;
typedef struct Line Line_t;

struct Reflist{
	int count;
	int len;
	void** arr;
};

