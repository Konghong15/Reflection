#include <type_traits>

struct Typename
{
	using ElementType = std::remove_all_extents_t<T>; // 배열이면 base 타입 남김
};

#define MemberName(name) Typename<decltype(name)>

class A
{
public:
	int value[3];
};

// 타입을 뭐로부터 얻어야하는가?
// 스트링으로 저장된 타입으로부터 어떻게 /
// 형식은 컴파일 타임에 있는데.. 구체 타입을 런타임에 문자열로 얻어올 방법이 있을까? 라는 생각이드는데요?

int main()
{
	A a;
	MemberName(a.value)::ElementType b = 42; // int 타입이 추론됨

	return 0;
}
