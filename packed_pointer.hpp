#ifndef HEADERS_PACKED_POINTER_HPP
#define HEADERS_PACKED_POINTER_HPP

#include "utility.hpp"

namespace hdrs {
namespace pp {

	template<typename T>
	constexpr T log2(T t) noexcept {
		return t == 1 ? 0 : log2(t >> 1) + 1;
	}

	template<typename PtrT, size_t Alignment>
	struct pointer_traits {
		static_assert(!(Alignment & Alignment - 1), "alignment must be a power of 2");
		using type = remove_ref_t<decltype(*declval<PtrT>())>; // possibly cv-qualified
		using free_bits = size_t_<log2(Alignment)>;

		static constexpr uintptr to_int(PtrT p) noexcept {
			return uintptr(&*p);
		}

		static constexpr PtrT from_int(uintptr i) noexcept {
			return decltype(&*declval<PtrT>())(i);
		}
	};

	template<typename PtrT>
	using aligned_pointer_traits = pointer_traits<PtrT, alignof(typename pointer_traits<PtrT, 1>::type)>;

	template<typename PtrT>
	using new_aligned_pointer_traits = pointer_traits<PtrT, __STDCPP_DEFAULT_NEW_ALIGNMENT__>;

	template<typename PtrT, typename Traits = aligned_pointer_traits<PtrT>>
	class packed_pointer {
	public:
		constexpr packed_pointer(PtrT p, uintptr i = 0) noexcept
			: _value{Traits::to_int(p) | i} {}

		constexpr typename Traits::type * pointer() const noexcept {
			return Traits::from_int(_value & ptr_mask());
		}

		constexpr uintptr integer() const noexcept {
			return _value & int_mask();
		}
	private:
		static constexpr uintptr int_mask() noexcept {
			return (1 << value(typename Traits::free_bits{})) - 1;
		}

		static constexpr uintptr ptr_mask() noexcept {
			return ~int_mask();
		}

		uintptr _value;
	};

#if __cplusplus >= 201703
	template<typename PtrT>
	packed_pointer(PtrT) -> packed_pointer<PtrT>;

	template<typename PtrT, typename Int>
	packed_pointer(PtrT, Int) -> packed_pointer<PtrT>;
#endif

} // pp
} // hdrs

#endif
