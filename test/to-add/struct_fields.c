//#define PLUS_FIVE(x) (x+5)

#define MACRO_TEST 1

#include "struct_fields.h"

s3 g(struct s1 s1, s4 arr[THREE])
{
    return irq_stat;
}

struct s6;

int f(enum e1 z, struct s6* z2) {
    return z;
}

struct s6 {
    int s6_field;
};

// Typedef-ing on the struct type directly creates a new type
typedef struct s6 s6;

int main() {
    enum e1 main1 = ONE;
    struct s6 main6 = { .s6_field = MACRO_TEST };
    struct s1 main2 = { .s1_field = f(main1, &main6) };
    s3 main3 = { .s3_field = 1 };
    s4 main4 = { .s4_field = (void*)0 };
    s4 main5[3];
    return g(main2, main5).s3_field;
}
