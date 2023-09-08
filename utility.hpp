#ifndef HEADERS_UTILITY_HPP
#define HEADERS_UTILITY_HPP

#include <type_traits>

namespace hdrs {

	template<typename T>
	T declval() noexcept;

	template<typename T, T t>
	struct constant {
		static constexpr T value{t};
	};

	template<bool b>
	using bool_ = constant<bool, b>;
	using true_t = bool_<true>;
	using false_t = bool_<false>;

	using size_t = decltype(sizeof(nullptr));
	template<size_t s>
	using size_t_ = constant<size_t, s>;

	template<size_t Size>
	using uint = std::conditional_t<
		Size <= sizeof(char), unsigned char, std::conditional_t<
		Size <= sizeof(short), unsigned short, std::conditional_t<
		Size <= sizeof(int), unsigned, std::conditional_t<
		Size <= sizeof(long), unsigned long, std::enable_if_t<
		Size <= sizeof(long long), unsigned long long
	>>>>>;

	using uintptr = uint<sizeof(void *)>;

}

#endif
