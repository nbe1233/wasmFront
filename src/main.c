#include <iso646.h>
#include <raylib.h>
#include <stdio.h>
#include <string.h>

#include "backend.h"
#include "wasm.h"

extern const unsigned char doom_wasm[] ;
extern unsigned int doom_wasm_len;

struct frontend_ctx_t
{
    Texture framebuffer;
    struct backend_ctx_t backend_ctx;
};

void frontend_ctx_init(  struct frontend_ctx_t *out)
{
    Image image = GenImageColor(640, 400, LIME);
    out->framebuffer = LoadTextureFromImage(image);
    backend_ctx_init( &out->backend_ctx, (const unsigned char * ) doom_wasm, doom_wasm_len );
    UnloadImage(image);
};

void frontend_ctx_destroy( struct frontend_ctx_t *in)
{
    UnloadTexture(in->framebuffer);
    backend_ctx_destroy(&in->backend_ctx);
};

struct frontend_ctx_t * global_frontend_ctx_ptr = NULL; //should only be used for callback functions
wasm_trap_t* js_stdout( const wasm_val_vec_t* args, wasm_val_vec_t* results )
{
    printf("%.*s", args->data[1].of.i32, wasm_memory_data(global_frontend_ctx_ptr->backend_ctx.memory) + args->data[0].of.i32);
    return NULL;
};
wasm_trap_t* js_stderr( const wasm_val_vec_t* args, wasm_val_vec_t* results )
{
    fprintf(stderr, "%.*s", args->data[1].of.i32, wasm_memory_data(global_frontend_ctx_ptr->backend_ctx.memory) + args->data[0].of.i32);
    return NULL;
};
wasm_trap_t* js_console_log( const wasm_val_vec_t* args, wasm_val_vec_t* results )
{
    printf("%.*s\n", args->data[1].of.i32, wasm_memory_data(global_frontend_ctx_ptr->backend_ctx.memory) + args->data[0].of.i32);
    return NULL;
}
wasm_trap_t* js_milliseconds_since_start(const wasm_val_vec_t* args, wasm_val_vec_t* results)
{
    double seconds = GetTime();
    results->data[0].of.i32 = seconds * 1000;
    return NULL;
};
wasm_trap_t* js_draw_screen(const wasm_val_vec_t* args, wasm_val_vec_t* results)
{
    int offset = args->data[0].of.i32;
    UpdateTexture(global_frontend_ctx_ptr->framebuffer, wasm_memory_data(global_frontend_ctx_ptr->backend_ctx.memory) + offset);
    return NULL;
};
wasm_trap_t* frontend_callback( void* env, const wasm_val_vec_t* args, wasm_val_vec_t* results )
{
    if (global_frontend_ctx_ptr == NULL){__builtin_trap();}
    char * import_name = env;
    //{ printf("function %s called \n", import_name); }
    //TODO: Hashmap LUT of pointers
    if      ( strcmp( import_name, "js_console_log") == 0 ){ js_console_log(args, results);}
    else if ( strcmp( import_name, "js_stdout") == 0 ){ js_stdout(args, results);}
    else if ( strcmp( import_name, "js_stderr") == 0 ){ js_stderr(args, results);}
    else if ( strcmp( import_name, "js_milliseconds_since_start") == 0 ){ js_milliseconds_since_start(args, results);}
    else if ( strcmp( import_name, "js_draw_screen") == 0 ){ js_draw_screen(args, results);}
    else { printf("function %s called from WASM but no implementation\n", import_name); __builtin_trap(); }
    return NULL;
}

int main()
{
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(1280, 720, "wasmFront");
    SetTargetFPS(60);

    struct frontend_ctx_t frontend_ctx = { };
    frontend_ctx_init(&frontend_ctx);
    global_frontend_ctx_ptr = &frontend_ctx;

    wasm_extern_vec_t exports;
    wasm_instance_exports(frontend_ctx.backend_ctx.instance, &exports);


    //put in scope so it'll free up stack space
    {
        wasm_func_t * doom_main_func = wasm_extern_as_func(exports.data[37]);
        wasm_val_t args[2]  = { WASM_I32_VAL(0), WASM_I32_VAL(0)};
        wasm_val_t rets[1]  = { WASM_I32_VAL(0)};
        wasm_val_vec_t args_vec = WASM_ARRAY_VEC(args);
        wasm_val_vec_t rets_vec = WASM_ARRAY_VEC(rets);
        frontend_ctx.backend_ctx.trap = wasm_func_call(doom_main_func, &args_vec, &rets_vec);
        if ( frontend_ctx.backend_ctx.trap != NULL) {__builtin_trap();}
    }

    const wasm_func_t * doom_loop_step = wasm_extern_as_func(exports.data[27]);
    const wasm_func_t * add_browser_event = wasm_extern_as_func(exports.data[24]);


    wasm_val_vec_t empty_val_vec = WASM_EMPTY_VEC;

    wasm_val_t input[2] = {WASM_I32_VAL(0), WASM_I32_VAL(0)};
    wasm_val_vec_t input_val_vec = WASM_ARRAY_VEC(input);
    while( not WindowShouldClose())
    {

        // handle input
        //DOS codes
        //KEY_ENTER: 13, KEY_BACKSPACE: 127, KEY_SPACE: 32,
        // KEY_LEFT: 0xac, KEY_RIGHT: 0xae, KEY_UP: 0xad, KEY_DOWN: 0xaf,
        // KEY_CTRL: 0x80+0x1d, KEY_ALT: 0x80+0x38, KEY_ESCAPE: 27, KEY_TAB: 9, KEY_SHIFT: 16 }
        if (IsKeyPressed(KEY_ENTER)) { input[0].of.i32 = 0; input[1].of.i32 = 13; wasm_func_call(add_browser_event, &input_val_vec, &empty_val_vec); }
        if (IsKeyReleased(KEY_ENTER)) { input[0].of.i32 = 1; input[1].of.i32 = 13; wasm_func_call(add_browser_event, &input_val_vec, &empty_val_vec); }

        if (IsKeyPressed(KEY_SPACE)) { input[0].of.i32 = 0; input[1].of.i32 = 32; wasm_func_call(add_browser_event, &input_val_vec, &empty_val_vec); }
        if (IsKeyReleased(KEY_SPACE)) { input[0].of.i32 = 1; input[1].of.i32 = 32; wasm_func_call(add_browser_event, &input_val_vec, &empty_val_vec); }

        if (IsKeyPressed(KEY_LEFT_CONTROL)) { input[0].of.i32 = 0; input[1].of.i32 = 0x80+0x1d; wasm_func_call(add_browser_event, &input_val_vec, &empty_val_vec); }
        if (IsKeyReleased(KEY_LEFT_CONTROL)) { input[0].of.i32 = 1; input[1].of.i32 = 0x80+0x1d; wasm_func_call(add_browser_event, &input_val_vec, &empty_val_vec); }

        if (IsKeyPressed(KEY_LEFT_ALT)) { input[0].of.i32 = 0; input[1].of.i32 = 0x80+0x38; wasm_func_call(add_browser_event, &input_val_vec, &empty_val_vec); }
        if (IsKeyReleased(KEY_LEFT_ALT)) { input[0].of.i32 = 1; input[1].of.i32 = 0x80+0x38; wasm_func_call(add_browser_event, &input_val_vec, &empty_val_vec); }

        if (IsKeyPressed(KEY_TAB)) { input[0].of.i32 = 0; input[1].of.i32 = 9; wasm_func_call(add_browser_event, &input_val_vec, &empty_val_vec); }
        if (IsKeyReleased(KEY_TAB)) { input[0].of.i32 = 1; input[1].of.i32 = 9; wasm_func_call(add_browser_event, &input_val_vec, &empty_val_vec); }

        if (IsKeyPressed(KEY_LEFT_SHIFT)) { input[0].of.i32 = 0; input[1].of.i32 = 16; wasm_func_call(add_browser_event, &input_val_vec, &empty_val_vec); }
        if (IsKeyReleased(KEY_LEFT_SHIFT)) { input[0].of.i32 = 1; input[1].of.i32 = 16; wasm_func_call(add_browser_event, &input_val_vec, &empty_val_vec); }

        if (IsKeyPressed(KEY_LEFT)) { input[0].of.i32 = 0; input[1].of.i32 = 0xac; wasm_func_call(add_browser_event, &input_val_vec, &empty_val_vec); }
        if (IsKeyReleased(KEY_LEFT)) { input[0].of.i32 = 1; input[1].of.i32 = 0xac; wasm_func_call(add_browser_event, &input_val_vec, &empty_val_vec); }
        if (IsKeyPressed(KEY_RIGHT)) { input[0].of.i32 = 0; input[1].of.i32 = 0xae; wasm_func_call(add_browser_event, &input_val_vec, &empty_val_vec); }
        if (IsKeyReleased(KEY_RIGHT)) { input[0].of.i32 = 1; input[1].of.i32 = 0xae; wasm_func_call(add_browser_event, &input_val_vec, &empty_val_vec); }
        if (IsKeyPressed(KEY_UP)) { input[0].of.i32 = 0; input[1].of.i32 = 0xad; wasm_func_call(add_browser_event, &input_val_vec, &empty_val_vec); }
        if (IsKeyReleased(KEY_UP)) { input[0].of.i32 = 1; input[1].of.i32 = 0xad; wasm_func_call(add_browser_event, &input_val_vec, &empty_val_vec); }
        if (IsKeyPressed(KEY_DOWN)) { input[0].of.i32 = 0; input[1].of.i32 = 0xaf; wasm_func_call(add_browser_event, &input_val_vec, &empty_val_vec); }
        if (IsKeyReleased(KEY_DOWN)) { input[0].of.i32 = 1; input[1].of.i32 = 0xaf; wasm_func_call(add_browser_event, &input_val_vec, &empty_val_vec); }

        if (IsGamepadButtonPressed(0, GAMEPAD_BUTTON_MIDDLE_RIGHT)) { input[0].of.i32 = 0; input[1].of.i32 = 13; wasm_func_call(add_browser_event, &input_val_vec, &empty_val_vec); }
        if (IsGamepadButtonReleased(0, GAMEPAD_BUTTON_MIDDLE_RIGHT)) { input[0].of.i32 = 1; input[1].of.i32 = 13; wasm_func_call(add_browser_event, &input_val_vec, &empty_val_vec); }

        if (IsGamepadButtonPressed(0, GAMEPAD_BUTTON_RIGHT_FACE_DOWN)) { input[0].of.i32 = 0; input[1].of.i32 = 32; wasm_func_call(add_browser_event, &input_val_vec, &empty_val_vec); }
        if (IsGamepadButtonReleased(0, GAMEPAD_BUTTON_RIGHT_FACE_DOWN)) { input[0].of.i32 = 1; input[1].of.i32 = 32; wasm_func_call(add_browser_event, &input_val_vec, &empty_val_vec); }

        if (IsGamepadButtonPressed(0, GAMEPAD_BUTTON_RIGHT_TRIGGER_2)) { input[0].of.i32 = 0; input[1].of.i32 = 0x80+0x1d; wasm_func_call(add_browser_event, &input_val_vec, &empty_val_vec); }
        if (IsGamepadButtonReleased(0, GAMEPAD_BUTTON_RIGHT_TRIGGER_2)) { input[0].of.i32 = 1; input[1].of.i32 = 0x80+0x1d; wasm_func_call(add_browser_event, &input_val_vec, &empty_val_vec); }

        //if (IsGamepadButtonPressed(0, KEY_LEFT_ALT)) { input[0].of.i32 = 0; input[1].of.i32 = 0x80+0x38; wasm_func_call(add_browser_event, &input_val_vec, &empty_val_vec); }
        //if (IsGamepadButtonReleased(0, KEY_LEFT_ALT)) { input[0].of.i32 = 1; input[1].of.i32 = 0x80+0x38; wasm_func_call(add_browser_event, &input_val_vec, &empty_val_vec); }

        if (IsGamepadButtonPressed(0, GAMEPAD_BUTTON_RIGHT_FACE_UP)) { input[0].of.i32 = 0; input[1].of.i32 = 9; wasm_func_call(add_browser_event, &input_val_vec, &empty_val_vec); }
        if (IsGamepadButtonReleased(0, GAMEPAD_BUTTON_RIGHT_FACE_UP)) { input[0].of.i32 = 1; input[1].of.i32 = 9; wasm_func_call(add_browser_event, &input_val_vec, &empty_val_vec); }

        if (IsGamepadButtonPressed(0, GAMEPAD_BUTTON_RIGHT_TRIGGER_1)) { input[0].of.i32 = 0; input[1].of.i32 = 16; wasm_func_call(add_browser_event, &input_val_vec, &empty_val_vec); }
        if (IsGamepadButtonReleased(0, GAMEPAD_BUTTON_RIGHT_TRIGGER_1)) { input[0].of.i32 = 1; input[1].of.i32 = 16; wasm_func_call(add_browser_event, &input_val_vec, &empty_val_vec); }

        if (IsGamepadButtonPressed(0, GAMEPAD_BUTTON_LEFT_FACE_LEFT)) { input[0].of.i32 = 0; input[1].of.i32 = 0xac; wasm_func_call(add_browser_event, &input_val_vec, &empty_val_vec); }
        if (IsGamepadButtonReleased(0, GAMEPAD_BUTTON_LEFT_FACE_LEFT)) { input[0].of.i32 = 1; input[1].of.i32 = 0xac; wasm_func_call(add_browser_event, &input_val_vec, &empty_val_vec); }
        if (IsGamepadButtonPressed(0, GAMEPAD_BUTTON_LEFT_FACE_RIGHT)) { input[0].of.i32 = 0; input[1].of.i32 = 0xae; wasm_func_call(add_browser_event, &input_val_vec, &empty_val_vec); }
        if (IsGamepadButtonReleased(0, GAMEPAD_BUTTON_LEFT_FACE_RIGHT)) { input[0].of.i32 = 1; input[1].of.i32 = 0xae; wasm_func_call(add_browser_event, &input_val_vec, &empty_val_vec); }
        if (IsGamepadButtonPressed(0, GAMEPAD_BUTTON_LEFT_FACE_UP)) { input[0].of.i32 = 0; input[1].of.i32 = 0xad; wasm_func_call(add_browser_event, &input_val_vec, &empty_val_vec); }
        if (IsGamepadButtonReleased(0, GAMEPAD_BUTTON_LEFT_FACE_UP)) { input[0].of.i32 = 1; input[1].of.i32 = 0xad; wasm_func_call(add_browser_event, &input_val_vec, &empty_val_vec); }
        if (IsGamepadButtonPressed(0, GAMEPAD_BUTTON_LEFT_FACE_DOWN)) { input[0].of.i32 = 0; input[1].of.i32 = 0xaf; wasm_func_call(add_browser_event, &input_val_vec, &empty_val_vec); }
        if (IsGamepadButtonReleased(0, GAMEPAD_BUTTON_LEFT_FACE_DOWN)) { input[0].of.i32 = 1; input[1].of.i32 = 0xaf; wasm_func_call(add_browser_event, &input_val_vec, &empty_val_vec); }


        //do step
        wasm_func_call(doom_loop_step, &empty_val_vec, &empty_val_vec);

        BeginDrawing();
            ClearBackground(MAGENTA);
            DrawTexturePro(frontend_ctx.framebuffer, (Rectangle) {0,0,640, 400}, (Rectangle) {0, 0, GetScreenWidth(), GetScreenHeight()}, (Vector2){0,0}, 0, WHITE);

            DrawFPS(10, 10);
        EndDrawing();
    }
    CloseWindow();
}
