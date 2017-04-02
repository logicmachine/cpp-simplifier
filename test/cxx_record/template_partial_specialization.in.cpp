template <typename T, typename... Params>
struct A {
	void foo(){ }
};
template <typename T>
struct A<T> {
	void foo(){ }
};
int main(){
	A<int> a;
	return 0;
}
