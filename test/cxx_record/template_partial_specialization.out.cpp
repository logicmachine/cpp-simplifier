template <typename T, typename... Params>
struct A {
};
template <typename T>
struct A<T> {
	template <int X>
	int bar(){
		return X;
	}
};
int main(){
	A<int> a;
	a.bar<10>();
	return 0;
}
