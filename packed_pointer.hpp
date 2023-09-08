#ifndef HEADERS_PACKED_POINTER_HPP
#define HEADERS_PACKED_POINTER_HPP

#include "utility.hpp"

namespace hdrs::pp {

	template<typename T>
	constexpr T log2(T t) noexcept {
		return t == 1 ? 0 : log2(t >> 1) + 1;
	}

	template<typename PtrT, size_t Alignment>
	struct pointer_traits_base {
		static_assert(!(Alignment & Alignment - 1), "alignment must be a power of 2");
		using type = std::remove_reference_t<decltype(*declval<PtrT>())>; // possibly cv-qualified
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
		class int_reference {
			friend packed_pointer;
		public:
			constexpr int_reference & operator=(uintptr i) const noexcept {
				return _ref = _ref & ptr_mask() | i, *this;
			}

			constexpr operator uintptr() const noexcept {
				return _ref & int_mask();
			}
		private:
			constexpr int_reference(uintptr & v) noexcept
				: _ref{v} {}

			uintptr & _ref;
		};

		class ptr_reference {
			friend packed_pointer;
		public:
			constexpr ptr_reference & operator=(PtrT p) const noexcept {
				return _ref = _ref & int_mask() | Traits::to_int(p), *this;
			}

			constexpr operator PtrT() const
				noexcept(noexcept(Traits::from_int(uintptr{})))
			{
				return Traits::from_int(_ref & ptr_mask());
			}
		private:
			constexpr ptr_reference(uintptr & v) noexcept
				: _ref{v} {}

			uintptr & _ref;
		};

		constexpr packed_pointer(PtrT p, uintptr i = 0)
			noexcept(noexcept(Traits::to_int(p)))
			: _value{Traits::to_int(p) | i} {}

		constexpr typename Traits::type * pointer() const
			noexcept(noexcept(Traits::from_int(uintptr{})))
		{
			return Traits::from_int(_value & ptr_mask());
		}

		constexpr ptr_reference pointer() noexcept {
			return _value;
		}

		constexpr uintptr integer() const noexcept {
			return _value & int_mask();
		}

		constexpr int_reference integer() noexcept {
			return _value;
		}

		friend constexpr void swap(packed_pointer & a, packed_pointer & b) noexcept {
			auto const t = a._value;
			a._value = b._value;
			b._value = t;
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

	template<typename PtrT>
	packed_pointer(PtrT) -> packed_pointer<PtrT>;

	template<typename PtrT, typename Int>
	packed_pointer(PtrT, Int) -> packed_pointer<PtrT>;

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

	template<typename RAIIPtrT, typename Traits = aligned_raii_pointer_traits<RAIIPtrT>>
	class packed_raii_pointer {
		using pp = packed_pointer<typename Traits::type *, pointer_traits<typename Traits::type *, (1 << Traits::free_bits)>>;
	public:
		class int_reference : public pp::int_reference {
			friend packed_raii_pointer;

			constexpr int_reference(pp & v) noexcept
				: pp::int_reference{v.integer()} {}
		};

		class ptr_reference {
			friend packed_raii_pointer;
		public:
			constexpr ptr_reference & operator=(RAIIPtrT p) const
				noexcept(noexcept(Traits::construct(Traits::release(static_cast<RAIIPtrT &&>(p)))))
			{
				return Traits::construct(_ref.pointer()),
				       _ref.pointer() = Traits::release(static_cast<RAIIPtrT &&>(p)),
				       *this;
			}

			constexpr operator typename Traits::type *() const
				noexcept(noexcept(_ref.pointer()))
			{
				return _ref.pointer();
			}
		private:
			constexpr ptr_reference(pp & v) noexcept
				: _ref{v} {}

			pp & _ref;
		};

		constexpr packed_raii_pointer(RAIIPtrT ptr, uintptr i = 0)
			noexcept(noexcept(Traits::release(static_cast<RAIIPtrT &&>(ptr))))
			: _pp{Traits::release(static_cast<RAIIPtrT &&>(ptr)), i} {}

		constexpr packed_raii_pointer(packed_raii_pointer const & other)
			noexcept(noexcept(
				packed_raii_pointer{static_cast<RAIIPtrT const &>(
					Traits::construct(other.pointer())
				)}
			))
			: packed_raii_pointer{static_cast<RAIIPtrT const &>(
				Traits::construct(other.pointer())
			), other.integer()} {}

		constexpr packed_raii_pointer(packed_raii_pointer && other)
			noexcept(noexcept(
				Traits::release(Traits::construct(other.pointer()))
			))
			: _pp{nullptr}
		{
			auto t = Traits::construct(other.pointer());
			_pp = pp{Traits::release(static_cast<RAIIPtrT &&>(t)), other.integer()};
			other._pp = pp{Traits::release(static_cast<RAIIPtrT &&>(t))};
		}

		#if __cplusplus >= 202002
		constexpr
		#endif
		~packed_raii_pointer() {
			Traits::construct(this->pointer());
		}

		constexpr typename Traits::type * pointer() const
			noexcept(noexcept(_pp.pointer()))
		{
			return _pp.pointer();
		}

		constexpr ptr_reference pointer() noexcept {
			return _pp;
		}

		constexpr uintptr integer() const noexcept {
			return _pp.integer();
		}

		constexpr int_reference integer() noexcept {
			return _pp;
		}

		friend constexpr void swap(packed_raii_pointer & a, packed_raii_pointer & b) noexcept {
			swap(a._pp, b._pp);
		}
	private:
		pp _pp;
	};

	template<typename PtrT>
	packed_raii_pointer(PtrT) -> packed_raii_pointer<PtrT>;

	template<typename PtrT, typename Int>
	packed_raii_pointer(PtrT, Int) -> packed_raii_pointer<PtrT>;

}

#endif
