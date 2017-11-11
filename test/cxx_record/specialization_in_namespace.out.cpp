namespace a {
	template <typename T>
	struct A {
	};
	template <>
	struct A<int> {
	};
}
int main(){
	a::A<int> x;
	return 0;
}
