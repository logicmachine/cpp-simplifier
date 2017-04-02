template <typename T, typename... Params>
struct A {
};
template <typename T>
struct A<T> {
};
int main(){
	A<int> a;
	return 0;
}
