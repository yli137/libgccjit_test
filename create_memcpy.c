#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <stddef.h>
#include <libgccjit.h>

void create_func( gcc_jit_context *ctxt, gcc_jit_function *func, 
                  int num, ptrdiff_t *disp, size_t *size )
{
    gcc_jit_type *char_type, *char_ptr_type, *void_type, *sizet_type, *int_type, *void_ptr_type;
    char_type = gcc_jit_context_get_type( ctxt, GCC_JIT_TYPE_CHAR );
    sizet_type = gcc_jit_context_get_type( ctxt, GCC_JIT_TYPE_SIZE_T );
    int_type = gcc_jit_context_get_type( ctxt, GCC_JIT_TYPE_INT );
    char_ptr_type = gcc_jit_type_get_pointer( char_type );

    void_type = gcc_jit_context_get_type( ctxt, GCC_JIT_TYPE_VOID );
    void_ptr_type = gcc_jit_type_get_pointer( void_type );

    gcc_jit_type *uint64_type = gcc_jit_context_get_type( ctxt, GCC_JIT_TYPE_UINT64_T );

    gcc_jit_param *param[2] = {
        gcc_jit_context_new_param( ctxt, NULL, char_ptr_type, "dst" ),
        gcc_jit_context_new_param( ctxt, NULL, char_ptr_type, "src" )
    };

    func = gcc_jit_context_new_function( ctxt, NULL,
	GCC_JIT_FUNCTION_EXPORTED,
	void_type,
	"my_memcpy",
	2, param,
	0 );

    gcc_jit_block *block = gcc_jit_function_new_block( func, "initial" );

    size_t total_disp = 0;

    gcc_jit_lvalue *dst_val = gcc_jit_function_new_local( func, NULL, char_ptr_type, "dst_val" ),
                   *src_val = gcc_jit_function_new_local( func, NULL, char_ptr_type, "src_val" ),
                   *dst_keep = gcc_jit_function_new_local( func, NULL, char_ptr_type, "dst_keep" ),
                   *src_keep = gcc_jit_function_new_local( func, NULL, char_ptr_type, "src_keep" );

    
    gcc_jit_block_add_assignment( block, NULL, dst_val, gcc_jit_param_as_rvalue( param[0] ) );
    gcc_jit_block_add_assignment( block, NULL, src_val, gcc_jit_param_as_rvalue( param[1] ) );

    ptrdiff_t diff = 0;
    for( int i = 0; i < num; i++ ){

	/* pointer arithmetic
	   have type mismatching error */
        gcc_jit_context_new_binary_op( ctxt, NULL,
            GCC_JIT_BINARY_OP_PLUS,
            char_ptr_type,
            gcc_jit_lvalue_as_rvalue( dst_val ), 
            gcc_jit_context_new_rvalue_from_long( ctxt, sizet_type, total_disp ) );

        gcc_jit_context_new_binary_op( ctxt, NULL,
            GCC_JIT_BINARY_OP_PLUS,
            char_ptr_type,
            gcc_jit_lvalue_as_rvalue( src_val ), 
            gcc_jit_context_new_rvalue_from_long( ctxt, sizet_type, disp[i] ) );

	/* alternative way to get address of ptr[index] */
        dst_val = gcc_jit_lvalue_get_address( 
                      gcc_jit_context_new_array_access( ctxt, NULL,
                          gcc_jit_lvalue_as_rvalue( dst_val ),
                          gcc_jit_context_new_rvalue_from_long( ctxt, sizet_type, total_disp - diff ) ),
                      NULL );

        src_val = gcc_jit_lvalue_get_address(
                      gcc_jit_context_new_array_access( ctxt, NULL,
                          gcc_jit_lvalue_as_rvalue( src_val ),
                          gcc_jit_context_new_rvalue_from_long( ctxt, sizet_type, disp[i] ) ),
                      NULL );

        diff = total_disp;
        total_disp += size[i];
        
        gcc_jit_rvalue *args[3] = {
            gcc_jit_context_new_cast( ctxt, NULL, 
                                      gcc_jit_lvalue_as_rvalue( dst_val ),
                                      void_ptr_type ),
            gcc_jit_context_new_cast( ctxt, NULL, 
                                      gcc_jit_lvalue_as_rvalue( src_val ),
                                      void_ptr_type ),
            gcc_jit_context_new_rvalue_from_long( ctxt, sizet_type, size[i] )
        };

	gcc_jit_function *builtin_memcpy = gcc_jit_context_get_builtin_function( ctxt, "__builtin_memcpy" );
        gcc_jit_rvalue *memcpy_call = gcc_jit_context_new_call(
            ctxt, NULL,
            builtin_memcpy,
            3,
            args );
        gcc_jit_block_add_eval( block, NULL, memcpy_call );
    }

    gcc_jit_block_end_with_void_return( block, NULL );
}

int main(void)
{
    int num = 8;
    double dst[8] = { 8, 1, 2, 3, 4, 5, 6, 7 },
           src[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };
    ptrdiff_t disp[8] = { 0, 8, 16, 24, 32, 40, 48, 56 };
    size_t size[8] = { 8, 8, 8, 8, 8, 8, 8, 8 };

    gcc_jit_context *ctxt;
    gcc_jit_function *func;

    ctxt = gcc_jit_context_acquire();

    create_func( ctxt, func, num, disp, size );

    gcc_jit_result *result = NULL;

    gcc_jit_context_compile_to_file( ctxt, GCC_JIT_OUTPUT_KIND_ASSEMBLER, "gcc_object_file" ); 

    result = gcc_jit_context_compile( ctxt );
    gcc_jit_context_release( ctxt );

    void *fn_ptr = gcc_jit_result_get_code( result, "my_memcpy" );
    typedef void (*fn_type)(char *d, char *s);
    fn_type copy = (fn_type)fn_ptr;

    printf("dst: ");
    for( int i = 0; i < num; i++ )
        printf("%.f ", dst[i]);
    printf("\n");

    printf("src: ");
    for( int i = 0; i < num; i++ )
        printf("%.f ", src[i]);
    printf("\n");

    copy( (char*)dst, (char*)src );

    printf("dst: ");
    for( int i = 0; i < num; i++ )
        printf("%.f ", dst[i]);
    printf("\n");

    printf("src: ");
    for( int i = 0; i < num; i++ )
        printf("%.f ", src[i]);
    printf("\n");

    return 0;
}

