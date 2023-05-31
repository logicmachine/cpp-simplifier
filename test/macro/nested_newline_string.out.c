#define MACRO \
"hello"
//-
#define NESTED MACRO
//-
int main()
{
    char *x = NESTED;
    return x[0];
}
