enum {
  one,
  two,
  three,
  four,
  five,
};
//-
struct S {
    int x[4];
//-    int y;
};
//-
static const struct S s[4] = {
  [one] = { .x = { 0 } },
  [two ... four] = { .x[one] = 1, .x[two] = 2 },
};
//-
int main() {
  return s[1].x[2];
}
