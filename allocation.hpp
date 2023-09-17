#ifndef HEADERS_ALLOCATION_HPP
#define HEADERS_ALLOCATION_HPP

// Great talk that inspired this header:
// https://www.youtube.com/watch?v=LIb3L4vKZ7U
// - alignment
// - allocate(size_t)
// - allocate_aligned(size_t, size_t)
// - allocate_all()
// - deallocate(allocation)
// - deallocate_all()
// - grow(allocation &, size_t)
// - owns(allocation)
//
// - reallocate(allocation &, size_t)
// - reallocate_aligned(allocation &, size_t, size_t)

// TODO mallocator
//   malloc_usable_size, _msize on Windows, malloc_size on MacOS
// TODO aligned_mallocator
//   posix_memalign, _aligned_malloc on Windows
// TODO alignment
// TODO ctors
// TODO SFINAE using <member-function>

#include "utility.hpp"

#define   LIKELY [[  likely]]
#define UNLIKELY [[unlikely]]

namespace hdrs::alloc {

	struct allocation {
		void * address;
		size_t size;
	};

	template<size_t N>
	class stack_allocator {
	public:
		stack_allocator(stack_allocator &) = delete;

		constexpr allocation allocate(size_t size) noexcept {
			if(size > data + N - unused) UNLIKELY
				return {nullptr};
			allocation a{unused, size};
			unused += size;
			return a;
		}

		constexpr void deallocate(allocation a) noexcept {
			if(a.address == unused - a.size)
				unused = static_cast<unsigned char *>(a.address);
		}

		constexpr void deallocate_all(allocation a) noexcept {
			unused = data;
		}

		constexpr bool owns(allocation a) const noexcept {
			// technically UB that could be fixed with casting
			return data[0] <= a.address && a.address < unused;
		}
	private:
		unsigned char data[N];
		unsigned char * unused{data};
	};

	// TODO free list allocator

	template<typename A, typename... B>
	class try_allocator : A, B... {
	public:
		constexpr allocation allocate(size_t size)
			noexcept(noexcept(A::allocate(size).address || (B::allocate(size).address || ...)))
		{
			auto a = A::allocate(size);
			if(!a.address)
				((a = B::allocate(size), a.address) || ...);
			return a;
		}

		constexpr void deallocate(allocation a)
			noexcept(noexcept(A::owns(a) && (A::deallocate(a), true) || ((B::owns(a) && (B::deallocate(a), true)) || ...)))
		{
			noexcept(A::owns(a));
			if(A::owns(a))
				return A::deallocate(a);
			((B::owns(a) && (B::deallocate(a), true)) || ...);
		}

		constexpr bool owns(allocation a) const
			noexcept(noexcept(A::owns(a) || (B::owns(a) || ...)))
		{
			return A::owns(a) || (B::owns(a) || ...);
		}
	};

	class nop_allocation_manager {
	public:
		constexpr void before_allocation(size_t) const noexcept {}
		constexpr void after_allocation(size_t, allocation) const noexcept {}
		constexpr void before_deallocation(allocation) const noexcept {}
		constexpr void after_deallocation(allocation) const noexcept {}
	};

	template<typename A, typename Manager>
	class managed_allocator : A, Manager {
	public:
		constexpr allocation allocate(size_t size)
			noexcept(noexcept(Manager::before_allocation(size), A::allocate(size), Manager::before_allocation(size)))
		{
			Manager::before_allocation(size);
			auto a = A::allocate(size);
			Manager::after_allocation(size, a);
			return a;
		}

		constexpr void deallocate(allocation a)
			noexcept(noexcept(Manager::before_deallocation(a), A::deallocate(a), Manager::before_deallocation(a)))
		{
			Manager::before_deallocation(a);
			A::deallocate(a);
			Manager::after_deallocation(a);
		}

		using A::owns;
	};

	template<typename A, size_t Min, size_t Max>
	class limited_allocator : A {
		static_assert(Min <= Max, "Allocator would never allocate under given constraints");
	public:
		// using A::A;

		constexpr allocation allocate(size_t size)
			noexcept(noexcept(size < Min || size > Max || A::allocate(size)))
		{
			return size < Min || size > Max ? allocation{nullptr} : A::allocate(size);
		}

		using A::deallocate;
		using A::owns;
	};

	template<typename A, size_t Min>
	using min_allocator = limited_allocator<A, Min, static_cast<size_t>(-1)>;

	template<typename A, size_t Max>
	using max_allocator = limited_allocator<A, 0, Max>;

}

#endif
