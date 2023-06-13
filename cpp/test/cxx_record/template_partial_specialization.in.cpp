template <typename T, typename... Params>
struct A {
	void foo(){ }
};
template <typename T>
struct A<T> {
	void foo(){ }
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
