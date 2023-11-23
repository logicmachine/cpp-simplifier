struct s1 {
    int s1_field;
    char UNUSED;
};

typedef struct {
    int s2_field;
} s2;

enum e1 {
    ONE=5,
    TWO=(7 << (2-1))
};

typedef struct {
 int s3_field;
} __attribute__((__aligned__((1 << (6))))) s3;
/* multiline
   block
   comment */

extern struct s2 thing2; /* single-line block comment */

extern __attribute__((section(".data..percpu" "..shared_aligned"))) __typeof__(s3) irq_stat __attribute__((__aligned__((1 << (6)))));


typedef struct {
    s2 (*s4_field)(s3);
} s4;

enum e5 {
    THREE=3
};
