#ifndef HEADERS_UTILITY_HPP
#define HEADERS_UTILITY_HPP

namespace hdrs {

	template<typename...>
	using void_t = void;

	template<typename T>
	T declval() noexcept;

	template<typename T, T>
	struct constant {};

	template<typename T, T t>
	constexpr T value(constant<T, t>) noexcept {
		return t;
	}

	template<bool b>
	using bool_ = constant<bool, b>;
	using true_t = bool_<true>;
	using false_t = bool_<false>;

	using size_t = decltype(sizeof(nullptr));
	template<size_t s>
	using size_t_ = constant<size_t, s>;

	template<bool, typename A, typename>
	struct conditional { using type = A; };

	template<typename A, typename B>
	struct conditional<false, A, B> { using type = B; };

	template<bool Cond, typename A, typename B>
	using conditional_t = typename conditional<Cond, A, B>::type;

	template<bool, typename = void>
	struct enable_if {};

	template<typename T>
	struct enable_if<true, T> { using type = T; };

	template<bool Cond, typename T = void>
	using enable_if_t = typename enable_if<Cond, T>::type;

	template<typename T>
	struct remove_ref { using type = T; };

	template<typename T>
	struct remove_ref<T &> { using type = T; };

	template<typename T>
	struct remove_ref<T &&> { using type = T; };

	template<typename T>
	using remove_ref_t = typename remove_ref<T>::type;

	template<size_t Size>
	using uint = conditional_t<
		Size <= sizeof(char), unsigned char, conditional_t<
		Size <= sizeof(short), unsigned short, conditional_t<
		Size <= sizeof(int), unsigned, conditional_t<
		Size <= sizeof(long), unsigned long, enable_if_t<
		Size <= sizeof(long long), unsigned long long
	>>>>>;

	using uintptr = uint<sizeof(void *)>;

}

#endif
