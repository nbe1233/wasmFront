#include "wasm.h"
struct backend_ctx_t {
    wasm_engine_t * engine;
    wasm_store_t * store;
    wasm_limits_t memory_page_limits;
    wasm_memorytype_t * memorytype;
    wasm_memory_t * memory;
    wasm_module_t * module;
    wasm_instance_t * instance;
    wasm_trap_t * trap;
    char * * import_names;
    int import_names_len;
};

void backend_ctx_init( struct backend_ctx_t * out, const unsigned char * wasm_file, int wasm_file_len);
void backend_ctx_destroy(struct backend_ctx_t * in);
