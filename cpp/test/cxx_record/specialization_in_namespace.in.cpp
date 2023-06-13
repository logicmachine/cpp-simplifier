namespace a {
	template <typename T>
	struct A {
	};
	template <>
	struct A<int> {
	};
}
struct B {
};
int main(){
	a::A<int> x;
	return 0;
}
