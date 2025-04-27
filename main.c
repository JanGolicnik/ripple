#define RIPPLE_IMPLEMENTATION
#define RIPPLE_WIDGETS
#include "src/ripple.h"

#define CENTERED(...) do {\
        RIPPLE(DISTURBANCE, AETHER);\
        RIPPLE(IDEA( FORM( .direction = cld_VERTICAL ) ), AETHER) {\
            RIPPLE(DISTURBANCE, AETHER);\
            { __VA_ARGS__ }\
            RIPPLE(DISTURBANCE, AETHER);\
        }\
        RIPPLE(DISTURBANCE, AETHER);\
    } while(false)

// heirarchy or growing elements, some fixed some not
void flex_test(void)
{
    RIPPLE( IDEA (
                LABEL("1"),
                FORM (
                    .direction = cld_VERTICAL,
                    //.height = FIXED(100),
                    .height = DEPTH(1.0f, FOUNDATION),
                    .width = DEPTH(1.0f, FOUNDATION),
                    .min_width = DEPTH(1.0f, REFINEMENT),
                ),
                .accept_input = true,
            ),
            CONSEQUENCE ( .color = 0xF8F8E1 )
    ){
        RIPPLE( IDEA (
                    LABEL(2),
                    FORM ( .height = DEPTH(3.0f/4.0f, FOUNDATION) ),
                ), AETHER
        ){
            RIPPLE( IDEA ( LABEL(3), FORM (.width = DEPTH(1.0f/3.0f, FOUNDATION), .direction = cld_VERTICAL ) ), AETHER )
            {
                RIPPLE( IDEA (FORM (.height = DEPTH(3.0f/4.0f, FOUNDATION ))), AETHER);
                RIPPLE( IDEA ( FORM ( .width = DEPTH(.25f, FOUNDATION)) ),
                        CONSEQUENCE ( .color = 0xFFC1DA ) );

                RIPPLE( DISTURBANCE, CONSEQUENCE ( .color = 0xFF90BB ) );
            }

            CENTERED(
                RIPPLE( IDEA( FORM( .width = FIXED(150), .height = FIXED(150) ) ),
                        CONSEQUENCE ( .color = 0x393E46 )
                );
            );
        }

        RIPPLE( DISTURBANCE, CONSEQUENCE ( .color = 0x8ACCD5 ) );
    }
}


// has 4 elements set to grow around the middle element
void centering_test(void)
{
    RIPPLE( DISTURBANCE,
            CONSEQUENCE (
                .color = 0xf8f8e1
            )
    ){
        RIPPLE( DISTURBANCE, AETHER );

        RIPPLE( IDEA ( FORM (.direction = cld_VERTICAL) ), AETHER )
        {
            RIPPLE( DISTURBANCE, AETHER );

            RIPPLE( IDEA (
                        LABEL(2),
                        FORM (
                            .width = FIXED(200),
                            .height = FIXED(200),
                        ),
                    ),
                    CONSEQUENCE ( .color = 0xff90bb )
            );

            RIPPLE( DISTURBANCE, AETHER );
        }

        RIPPLE( DISTURBANCE, AETHER );
    }
}

typedef struct { void *data; usize data_size; usize ptr; } LinearAllocator;

void *linear_allocator_alloc(void *ctx, usize size)
{
    LinearAllocator* allocator = (LinearAllocator*)ctx;
    void* ret = allocator->ptr + size <= allocator->data_size ? (u8*)allocator->data + allocator->ptr : nullptr;
    allocator->ptr += size;
    return ret;
}

void linear_allocator_free(void* ctx, void* ptr, usize size) { return; }

int main(int argc, char* argv[])
{
    LinearAllocator allocator = (LinearAllocator){ .data = malloc(1024), .data_size = 1024 };

    SURFACE( .title = "surface", .width = 800, .height = 300,
            .cursor_data = (RippleCursorData){.x = 100, .y = 100, .left_click = true },
            .allocator = (Allocator){ .context = &allocator, .alloc = &linear_allocator_alloc, .free = &linear_allocator_free })
    {
        flex_test();

        //centering_test();
    }

    free(allocator.data);

    return 0;
}
