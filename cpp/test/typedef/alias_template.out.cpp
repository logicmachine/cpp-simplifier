template <typename T>
struct foo { };
template <typename T>
using bar = foo<T>;
//-template <typename T>
//-using baz = foo<T>;
int main(){
	bar<int> x;
	return 0;
}
