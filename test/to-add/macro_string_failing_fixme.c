#define SOME_MACRO \
"hello" // this works if you insert a space at the start of the line

int main()
{
    char *x = SOME_MACRO;
    return x[0];
}
