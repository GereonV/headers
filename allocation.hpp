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
// - resize(allocation &, size_t)
// - owns(allocation)
//
// - reallocate(allocation &, size_t)
// - reallocate_aligned(allocation &, size_t, size_t)

// TODO mallocator
//   malloc_usable_size, _msize on Windows, malloc_size on MacOS
// TODO aligned_mallocator
//   posix_memalign, _aligned_malloc on Windows
// TODO free list allocator
// TODO alignment
// TODO ctors

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

		constexpr allocation allocate_aligned(size_t size, size_t alignment_log2) noexcept {
			auto const alignment = 1 << alignment_log2;
			auto align_offset = alignment - (reinterpret_cast<size_t>(unused) & alignment - 1);
			if(size > data + N - unused - align_offset) UNLIKELY
				return {nullptr};
			unused += align_offset;
			allocation a{unused, size};
			unused += size;
			return a;
		}

		constexpr void deallocate(allocation a) noexcept {
			if(a.address == unused - a.size) UNLIKELY
				unused = static_cast<unsigned char *>(a.address);
		}

		constexpr void deallocate_all(allocation a) noexcept {
			unused = data;
		}

		constexpr bool resize(allocation & a, size_t new_size) noexcept {
			if(a.address != unused - a.size || new_size > data + N - a.address) UNLIKELY
				return false;
			unused -= a.size;
			a.size = new_size;
			unused += new_size;
			return true;
		}

		constexpr bool owns(allocation a) const noexcept {
			// technically UB that could be fixed with casting
			return data[0] <= a.address && a.address < unused;
		}
	private:
		unsigned char data[N];
		unsigned char * unused{data};
	};

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

		constexpr allocation allocate_aligned(size_t size, size_t al)
			noexcept(noexcept(A::allocate_aligned(size, al).address || (B::allocate_aligned(size, al).address || ...)))
		{
			auto a = A::allocate_aligned(size, al);
			if(!a.address)
				((a = B::allocate_aligned(size, al), a.address) || ...);
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
		constexpr void before_allocate(size_t) const noexcept {}
		constexpr void after_allocate(size_t, allocation) const noexcept {}
		constexpr void before_allocate_aligned(size_t, size_t) const noexcept {}
		constexpr void after_allocate_aligned(size_t, size_t, allocation) const noexcept {}
		constexpr void before_allocate_all() const noexcept {}
		constexpr void after_allocate_all(allocation) const noexcept {}
		constexpr void before_deallocate(allocation) const noexcept {}
		constexpr void after_deallocate(allocation) const noexcept {}
		constexpr void before_deallocate_all(allocation) const noexcept {}
		constexpr void after_deallocate_all(allocation) const noexcept {}
		constexpr void before_resize(allocation, size_t) const noexcept {}
		constexpr void after_resize(allocation, size_t, bool) const noexcept {}
	};

	template<typename A, typename Manager>
	class managed_allocator : A, Manager {
	public:
		constexpr allocation allocate(size_t size)
			noexcept(noexcept(Manager::before_allocate(size), Manager::after_allocate(size, A::allocate(size))))
		{
			Manager::before_allocate(size);
			auto a = A::allocate(size);
			Manager::after_allocate(size, a);
			return a;
		}

		constexpr allocation allocate_aligned(size_t size, size_t al)
			noexcept(noexcept(Manager::before_allocate_aligned(size, al), Manager::after_allocate_aligned(size, al, A::allocate_aligned(size, al))))
		{
			Manager::before_allocate_aligned(size, al);
			auto a = A::allocate_aligned(size, al);
			Manager::after_allocate_aligned(size, a);
			return a;
		}

		constexpr allocation allocate_all()
			noexcept(noexcept(Manager::before_allocate_all(), Manager::after_allocate_all(A::allocate_all())))
		{
			Manager::before_allocate_all();
			auto a = A::allocate_all();
			Manager::after_allocat_all(a);
			return a;
		}

		constexpr void deallocate(allocation a)
			noexcept(noexcept(Manager::before_deallocate(a), A::deallocate(a), Manager::after_deallocate(a)))
		{
			Manager::before_deallocate(a);
			A::deallocate(a);
			Manager::after_deallocate(a);
		}

		constexpr void deallocate_all()
			noexcept(noexcept(Manager::before_deallocate_all(), A::deallocate_all(), Manager::after_deallocate_all()))
		{
			Manager::before_deallocate_all();
			A::deallocate_all();
			Manager::after_deallocate_all();
		}

		constexpr bool resize(allocation & a, size_t new_size)
			noexcept(noexcept(Manager::before_resize(a, new_size), Manager::after_resize(a, new_size, A::resize(a, new_size))))
		{
			Manager::before_resize(a, new_size);
			auto r = A::resize(a, new_size);
			Manager::after_resize(a, new_size, r);
			return r;
		}

		constexpr bool owns(allocation a)
			noexcept(noexcept(A::owns(a)))
		{
			return A::owns(a);
		}
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

		constexpr allocation allocate_aligned(size_t size, size_t al)
			noexcept(noexcept(size < Min || size > Max || A::allocate_aligned(size, al)))
		{
			return size < Min || size > Max ? allocation{nullptr} : A::allocate_aligned(size, al);
		}

		using A::deallocate;

		constexpr bool resize(allocation & a, size_t new_size)
			noexcept(noexcept(new_size >= Min && new_size <= Max && A::resize(a, new_size)))
		{
			return new_size >= Min && new_size <= Max && A::resize(a, new_size);
		}

		constexpr bool owns(allocation a)
			noexcept(noexcept(A::owns(a)))
		{
			return a.size >= Min && a.size <= Max && A::owns(a);
		}
	};

	template<typename A, size_t Min>
	using min_allocator = limited_allocator<A, Min, static_cast<size_t>(-1)>;

	template<typename A, size_t Max>
	using max_allocator = limited_allocator<A, 0, Max>;

}

#endif
