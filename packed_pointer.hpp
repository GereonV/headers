#ifndef HEADERS_PACKED_POINTER_HPP
#define HEADERS_PACKED_POINTER_HPP

namespace hdrs {

	template<typename T, T>
	struct constant {};

	template<typename T, T t>
	constexpr T value(constant<T, t>) noexcept {
		return t;
	}

	using size_t = decltype(sizeof(nullptr));
	template<size_t s>
	using size_t_ = constant<size_t, s>;

	template<typename T, T t>
	constexpr T log2() noexcept {
		static_assert(!(t & t - 1), "must be exact power of 2");
		return t == 1 ? 0 : log2<T, (t >> 1)>() + 1;
	}

	template<typename PtrT>
	struct pointer_traits;

	template<typename T>
	struct pointer_traits<T *> {
		using free_bits = size_t_<log2<size_t, alignof(T)>()>;
	};

}

#endif
