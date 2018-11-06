/**
very tiny js engine in single file.
*/

#ifndef MARIO_VM
#define MARIO_VM

#include "mario_bc.h"

#ifdef __cplusplus /* __cplusplus */
extern "C" {
#endif

bool compile(bytecode_t *bc, const char* input);

//script var
#define V_UNDEF  0
#define V_INT    1
#define V_FLOAT  2
#define V_STRING 3
#define V_OBJECT 4
#define V_BLOCK  5
#define V_BOOL   6 

#define THIS "this"
#define PROTOTYPE "prototype"
#define SUPER "super"
#define CONSTRUCTOR "constructor"

typedef struct st_var {
	uint32_t magic: 8; //0 for var; 1 for node
	uint32_t refs:18;
	uint32_t type:4;
	uint32_t is_array:1;
	uint32_t is_func:1;
	uint32_t size;  // size for bytes type of value;

	void* value;
	free_func_t free_func; //how to free value
	free_func_t on_destroy; //before destroyed.

	m_array_t children;
} var_t;

struct st_vm;
typedef var_t* (*native_func_t)(struct st_vm *, var_t*, void*);

typedef struct st_func {
	native_func_t native;
	int8_t regular: 4;
	int8_t is_static: 4;
	PC pc;
	void *data;
	m_array_t args; //argument names
	var_t* owner;
} func_t;

//script node for var member children
typedef struct st_node {
	int16_t magic: 8; //1 for node
  int16_t be_const : 8;
	char* name;
	var_t* var;
} node_t;


#ifdef MARIO_THREAD
#include <pthread.h>

typedef struct st_isignal {
	var_t* obj;
	var_t* handle_func;
	str_t* handle_func_name;
	var_t* args;
	struct st_isignal* next;
	struct st_isignal* prev;
} isignal_t;
#endif

#ifdef MARIO_CACHE
#define VAR_CACHE_MAX 32
#define NODE_CACHE_MAX 16
typedef struct st_node_cache_t {
	node_t* node;
	var_t* sc_var;
	int name_id;
} node_cache_t;
#endif

#define VM_STACK_MAX 32
typedef struct st_vm {
	bytecode_t bc;
	bool (*compiler)(bytecode_t *bc, const char* input);

	m_array_t scopes;
	void* stack[VM_STACK_MAX];
	int32_t stack_top;
	PC pc;

	bool terminated;
	var_t* root;

	void (*on_init)(struct st_vm* vm);
	m_array_t init_natives;
	void (*on_close)(struct st_vm* vm);
	m_array_t close_natives;

	#ifdef MARIO_THREAD
	pthread_mutex_t interrupt_lock;
	isignal_t* isignal_head;
	isignal_t* isignal_tail;
	uint32_t isignal_num;
	bool interrupted;
	#endif

	#ifdef MARIO_CACHE
	var_t* var_cache[VAR_CACHE_MAX];
	uint32_t var_cache_used;

	node_cache_t node_cache[NODE_CACHE_MAX];
	#endif
	uint16_t this_strIndex;
	var_t* var_Object;
	var_t* var_true;
	var_t* var_false;
} vm_t;


node_t* node_new(const char* name);
void node_free(void* p, void* extra);
var_t* node_replace(vm_t* vm, node_t* node, var_t* v);

void var_dump(var_t* var);
void var_remove_all(vm_t* vm, var_t* var);
node_t* var_add(vm_t* vm, var_t* var, const char* name, var_t* add);
node_t* var_find(var_t* var, const char*name);
var_t* var_find_var(var_t* var, const char*name);
node_t* var_find_create(vm_t* vm, var_t* var, const char*name);
node_t* var_get(vm_t* vm, var_t* var, int32_t index);

void var_free(void* p, void* extra);

var_t* var_ref(var_t* var);
void var_unref(vm_t* vm, var_t* var, bool del);

//#define var_ref(var) ({ ++(var)->refs; var; })
//#define var_unref(var, del) ({ --(var)->refs; if((var)->refs <= 0 && (del)) var_free((var)); })

var_t* var_new();
var_t* var_new_block();
var_t* var_new_array();
var_t* var_new_int(int i);
var_t* var_new_bool(bool b);
var_t* var_new_obj(void*p, free_func_t fr);
var_t* var_new_float(float i);
var_t* var_new_str(const char* s);
const char* var_get_str(var_t* var);
int var_get_int(var_t* var);
bool var_get_bool(var_t* var);
float var_get_float(var_t* var);
func_t* var_get_func(var_t* var);

void var_to_json_str(vm_t* vm, var_t*, str_t*, int);
void var_to_str(vm_t* vm, var_t*, str_t*);
var_t* json_parse(vm_t* vm, const char* str);

void vm_push(vm_t* vm, var_t* var);
void vm_push_node(vm_t* vm, node_t* node);
vm_t* vm_new(bool (*compiler)(bytecode_t *bc, const char* input));

void vm_init(vm_t* vm,
	void (*on_init)(struct st_vm* vm),
	void (*on_close)(struct st_vm* vm)
);

vm_t* vm_from(vm_t* vm);
bool vm_load(vm_t* vm, const char* s);
bool vm_load_run(vm_t* vm, const char* s);
bool vm_load_run_native(vm_t* vm, const char* s);
void vm_dump(vm_t* vm);
void vm_run(vm_t* vm);
void vm_close(vm_t* vm);

var_t* new_obj(vm_t* vm, const char* cls_name, int arg_num);
node_t* vm_find(vm_t* vm, const char* name);
node_t* vm_find_in_class(var_t* var, const char* name);
node_t* vm_reg_var(vm_t* vm, const char* cls, const char* name, var_t* var, bool be_const);
node_t* vm_reg_static(vm_t* vm, const char* cls, const char* decl, native_func_t native, void* data);
node_t* vm_reg_native(vm_t* vm, const char* cls, const char* decl, native_func_t native, void* data);
void vm_reg_init(vm_t* vm, void (*func)(void*), void* data);
void vm_reg_close(vm_t* vm, void (*func)(void*), void* data);

node_t* find_member(var_t* obj, const char* name);
var_t* get_obj(var_t* obj, const char* name);
const char* get_str(var_t* obj, const char* name);
int get_int(var_t* obj, const char* name);
float get_float(var_t* obj, const char* name);
bool get_bool(var_t* obj, const char* name);
var_t* get_obj_member(var_t* obj, const char* name);
var_t* set_obj_member(vm_t* vm, var_t* obj, const char* name, var_t* var);

var_t* call_m_func(vm_t* vm, var_t* obj, var_t* func, var_t* args);
var_t* call_m_func_by_name(vm_t* vm, var_t* obj, const char* func_name, var_t* args);


#ifdef MARIO_THREAD
bool interrupt(vm_t* vm, var_t* obj, var_t* func, var_t* args);
bool interrupt_by_name(vm_t* vm, var_t* obj, const char* func_name, var_t* args);
#endif

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif