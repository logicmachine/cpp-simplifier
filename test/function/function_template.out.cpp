//-template <typename T>
//-T a(){
//-	return T();
//-}
template <typename T>
T b(){
	return T();
}
//-template <typename T>
//-T c(){
//-	return T();
//-}
int main(){
	b<int>();
	return 0;
}
