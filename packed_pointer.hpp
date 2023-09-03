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
	struct pointer_traits_base {
		static_assert(!(Alignment & Alignment - 1), "alignment must be a power of 2");
		using type = remove_ref_t<decltype(*declval<PtrT>())>; // possibly cv-qualified
		static constexpr auto free_bits = log2(Alignment);
	};

	template<typename PtrT, size_t Alignment>
	struct pointer_traits : pointer_traits_base<PtrT, Alignment> {
		static constexpr uintptr to_int(PtrT p)
			noexcept(noexcept(&*p))
		{
			return uintptr(&*p);
		}

		static constexpr PtrT from_int(uintptr i)
			noexcept(noexcept(decltype(&*declval<PtrT>())(i)))
		{
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
		constexpr packed_pointer(PtrT p, uintptr i = 0)
			noexcept(noexcept(Traits::to_int(p)))
			: _value{Traits::to_int(p) | i} {}

		constexpr typename Traits::type * pointer() const
			noexcept(noexcept(Traits::from_int(uintptr{})))
		{
			return Traits::from_int(_value & ptr_mask());
		}

		constexpr uintptr integer() const noexcept {
			return _value & int_mask();
		}
	private:
		static constexpr uintptr int_mask() noexcept {
			return (1 << Traits::free_bits) - 1;
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

	template<typename RAIIPtrT, size_t Alignment>
	struct raii_pointer_traits : pointer_traits_base<RAIIPtrT, Alignment> {
		using typename pointer_traits_base<RAIIPtrT, Alignment>::type;

		static constexpr RAIIPtrT construct(type * ptr)
			noexcept(noexcept(RAIIPtrT{ptr}))
		{
			return RAIIPtrT{ptr};
		}

		static constexpr type * release(RAIIPtrT ptr)
			noexcept(noexcept(ptr.release()))
		{
			return ptr.release();
		}
	};

	template<typename PtrT>
	using aligned_raii_pointer_traits = raii_pointer_traits<PtrT, alignof(typename raii_pointer_traits<PtrT, 1>::type)>;

	template<typename PtrT>
	using new_aligned_raii_pointer_traits = raii_pointer_traits<PtrT, __STDCPP_DEFAULT_NEW_ALIGNMENT__>;

	template<typename Traits>
	using packed_raii_pointer_base = packed_pointer<typename Traits::type *, pointer_traits<typename Traits::type *, (1 << Traits::free_bits)>>;

	template<typename RAIIPtrT, typename Traits = aligned_raii_pointer_traits<RAIIPtrT>>
	class packed_raii_pointer : public packed_raii_pointer_base<Traits> {
	public:
		constexpr packed_raii_pointer(RAIIPtrT ptr, uintptr i = 0)
			noexcept(noexcept(packed_raii_pointer_base<Traits>{Traits::release(static_cast<RAIIPtrT &&>(ptr)), i}))
			: packed_raii_pointer_base<Traits>{Traits::release(static_cast<RAIIPtrT &&>(ptr)), i} {}

#if __cplusplus >= 202002
		constexpr
#endif
		~packed_raii_pointer() {
			Traits::construct(this->pointer());
		}
	};

#if __cplusplus >= 201703
	template<typename PtrT>
	packed_raii_pointer(PtrT) -> packed_raii_pointer<PtrT>;

	template<typename PtrT, typename Int>
	packed_raii_pointer(PtrT, Int) -> packed_raii_pointer<PtrT>;
#endif

} // pp
} // hdrs

#endif
