#define FOO(X) 42 + X
#define BAR(X,Y) X ## Y
//-
//-
int main()
{
  return BAR(FO,O)(10);
}
