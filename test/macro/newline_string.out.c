#define MACRO \
"hello"
//-
int main()
{
    char *x = MACRO;
    return x[0];
}
