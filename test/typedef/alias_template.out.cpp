template <typename T>
struct foo { };
template <typename T>
using bar = foo<T>;
int main(){
	bar<int> x;
	return 0;
}
