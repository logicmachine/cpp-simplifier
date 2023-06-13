struct A { static const int x = 10; };
struct B { static const int y = 20; };
int main(){ return A::x; }
