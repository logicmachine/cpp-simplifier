//-// An admittedly silly example.
//-// Yet, shows --omit-lines is unsound.
//-// No clear way to fix with Clang API.
//-// Can be worked around by preprocessing
//-// before simplifying, or using a lexer-
//-// based tool to check for this pattern.
#define EMPTY(x,y) \
//-
int main()
{
    int x[] = { 0 };
    EMPTY(0,0);
    return x[0];
}
