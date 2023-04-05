enum {
  one,
  two,
  three,
};

struct S {
    int x[3];
    int y;
};

static const struct S s[3] = {
  [one] = { .x = { 0 } },
  [two ... three] = { .x[one] = 1, .x[two] = 2 },
};
 
int main() {
  return s[1].x[2];
}
