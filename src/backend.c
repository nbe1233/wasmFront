#include <stdio.h>
#include <iso646.h>
#include "backend.h"
#include <stdlib.h>
#include <string.h>

extern wasm_trap_t* frontend_callback( void* env, const wasm_val_vec_t* args, wasm_val_vec_t* results );
//wasm_trap_t* stub_callback( const wasm_val_vec_t* args, wasm_val_vec_t* results )
//{
//    printf("stubbed!\n");
//    return nullptr;
//};

void backend_ctx_init( struct backend_ctx_t * out, const unsigned char * wasm_file, int wasm_file_len)
{
    out->engine = wasm_engine_new();
    out->store = wasm_store_new(out->engine);
    out->memory_page_limits = (wasm_limits_t) {.min = 102, .max = 500}; //should probably get limits from wasm file. 1 page = 65536 bytes
    out->memorytype = wasm_memorytype_new(&out->memory_page_limits);
    out->memory = wasm_memory_new(out->store, out->memorytype );
    out->trap = NULL;


    wasm_byte_vec_t binary;
    wasm_byte_vec_new_uninitialized(&binary, wasm_file_len);
    
    for (int i = 0; i < binary.size; i++)
    {
        binary.data[i] = wasm_file[i];
    }

    //validate
    printf("Validating Wasm\n");
    if ( not wasm_module_validate(out->store, &binary) )
    {
        fprintf(stderr, ">>>>Not valid wasm passed to backend_ctx_init");
        __builtin_trap();
    }
    else
    {
        printf(">>>>Wasm is valid\n");
    }

    printf("Compiling Wasm module\n");
    out->module = wasm_module_new(out->store, &binary);
    if (out->module == NULL)
    {
        fprintf(stderr, ">>>>Failed to compile wasm module!\n");
        __builtin_trap();
    }
    else
    {
        printf(">>>>Compiled wasm module\n");
    }

    wasm_importtype_vec_t importtypes;
    wasm_module_imports(out->module, &importtypes);

    wasm_extern_vec_t import_bindings;
    wasm_extern_vec_new_uninitialized(&import_bindings, importtypes.size);

    out->import_names_len = importtypes.size;
    out->import_names = calloc(out->import_names_len, sizeof(char *));
    printf("Imports:\n");
    for (int i = 0; i < importtypes.size; i++)
    {
        //TODO: handle deallocation
        const wasm_name_t * name = wasm_importtype_name( importtypes.data[i] );
        const wasm_externtype_t * externtype = wasm_importtype_type(importtypes.data[i]);
        wasm_externkind_t kind = wasm_externtype_kind( externtype );
        switch( kind)
        {
            case WASM_EXTERN_FUNC:
                printf("\t%d. FUNC: %.*s\n", i, (int) name->size, name->data );
                out->import_names[i] = calloc( name->size + 1, sizeof(char));
                strncpy(out->import_names[i], name->data, name->size);
                wasm_functype_t *functype = wasm_externtype_as_functype( (wasm_externtype_t *) externtype);
                wasm_func_t * func =  wasm_func_new_with_env(out->store, functype, frontend_callback, (void *) out->import_names[i], NULL);
                //wasm_func_t * func =  wasm_func_new(out->store, functype, stub_callback);
                import_bindings.data[i] = wasm_func_as_extern(func);
                break;
            case WASM_EXTERN_MEMORY:
                printf("\t%d. MEMORY: %.*s\n", i, (int) name->size, name->data );
                import_bindings.data[i] = wasm_memory_as_extern(out->memory);
                break;
            default:
                printf("\t%d. UNKNOWN: %.*s\n", i, (int) name->size, name->data );
                break;
        }
    }

    printf("Instantiating module\n");
    out->instance = wasm_instance_new(out->store, out->module, &import_bindings, &out->trap);
    if(out->trap != NULL)
    {
        wasm_message_t message;
        wasm_trap_message(out->trap, &message);
        fprintf(stderr, "Failed to Instantiate module: %.*s\n", (int) message.size, message.data );
    }
    else
    {
        printf(">>>>Module Instantiation sucessfull\n");
    }

    wasm_byte_vec_delete(&binary);
    wasm_importtype_vec_delete(&importtypes);
};

void backend_ctx_destroy(struct backend_ctx_t * in)
{
    //TODO:
    //printf("%s: stubbed!\n", __func__);
    for (int i = 0; i < in->import_names_len; i++)
    {
        free(in->import_names[i]);
    }
    free(in->import_names);
    wasm_instance_delete(in->instance);
    wasm_module_delete(in->module);
    wasm_memory_delete(in->memory);
    wasm_store_delete(in->store);
    wasm_engine_delete(in->engine);
}
