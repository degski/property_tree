
// MIT License
//
// Copyright (c) 2020 degski
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#include "includes.hpp"

void compile_command ( ) {}
void link_command ( ) {}
void lib_command ( ) {}

struct command {
    sax::atom_type name;
    void ( *function ) ( void );
};

#define COMMAND( NAME )                                                                                                            \
    command { #NAME, NAME##_command }

struct command commands[] = { COMMAND ( compile ), COMMAND ( link ), COMMAND ( lib ) };

//

#define PROPERTY( property_name, ... )                                                                                             \
                                                                                                                                   \
    struct property_##property_name {                                                                                              \
        static constexpr sax::atom_type name                                      = QUOTE_PARAM ( property_name );                 \
        static constexpr std::array<sax::atom_type, NARGS ( __VA_ARGS__ )> option = { QUOTE_PARAMS ( __VA_ARGS__ ) };              \
        int value                                                                 = 0;                                             \
        [[nodiscard]] sax::atom_type get ( ) noexcept { return option[ value ]; }                                                  \
    };

/*
    template<sax::atom_type Name, typename... Args>

    struct property_property_name {
        static constexpr sax::atom_type name                                   = Name;
        static constexpr std::array<sax::atom_type, sizeof ( Args )...> option = { Args... };
        int value                                                    = 0;
        [[nodiscard]] std::string_view get ( ) noexcept { return option[ value ]; }
    };
*/

// clang-format off
PROPERTY ( architecture, x64, x86, arm )
PROPERTY ( configuration, debug, release )
PROPERTY ( language, latest, cpp20, cpp17, cpp14, cpp11, cpp03, cpp98, c11, c99, c89 )
PROPERTY ( compiler, clang_cl, cl )
PROPERTY ( linker, lld_link, link )
PROPERTY ( librarian, llvm_lib, lib )
PROPERTY ( warnings, w3, w0, w1, w2, w3, w4 )
// clang-format on

/*
    We are given 10 individuals say,
    0, 1, 2, 3, 4, 5, 6, 7, 8, 9
    a, b, c, d, e, f, g, h, i, j

    Following are relationships to be added.
    b <-> d
    c <-> f
    g <-> j
    a <-> b
    c <-> i
    e <-> j



    And given queries like whether a is a friend of d or not.
    We basically need to create following 4 groups
    and maintain a quickly accessible connection
    among group items:
    G1 = {a, b, d}
    G2 = {c, f, i}
    G3 = {e, g, j}
    G4 = {h}
*/

#define USE_MIMALLOC_LTO 1

#include <pector/malloc_allocator.h>
#include <pector/mimalloc_allocator.h>
#include <pector/pector.h>

#include <memory>
#include <type_traits> // true_type

template<class T>
struct xmi_stl_allocator : public std::allocator_traits<xmi_stl_allocator<T>> {

    using value_type = typename std::allocator_traits<xmi_stl_allocator<T>>::value_type;

    using propagate_on_container_copy_assignment = std::true_type;
    using propagate_on_container_move_assignment = std::true_type;
    using propagate_on_container_swap            = std::true_type;
    using is_always_equal                        = std::true_type;

    xmi_stl_allocator ( ) mi_attr_noexcept {}
    xmi_stl_allocator ( const xmi_stl_allocator & ) mi_attr_noexcept {}
    template<class U>
    xmi_stl_allocator ( const xmi_stl_allocator<U> & ) mi_attr_noexcept {}

    void deallocate ( T * p, size_t /* count */ ) { mi_free ( p ); }
    T * allocate ( size_t count ) { return ( T * ) mi_new_n ( count, sizeof ( T ) ); }
};

template<class T1, class T2>
bool operator== ( const xmi_stl_allocator<T1> &, const xmi_stl_allocator<T2> & ) mi_attr_noexcept {
    return true;
}
template<class T1, class T2>
bool operator!= ( const xmi_stl_allocator<T1> &, const xmi_stl_allocator<T2> & ) mi_attr_noexcept {
    return false;
}

template<typename T, typename S>
using mi_vector = pt::pector<T, xmi_stl_allocator<T>, S, pt::default_recommended_size, false>;

template<typename ValueType, typename SizeType>
struct spaghetti_stack {

    using value_type      = ValueType;
    using size_type       = SizeType;
    using difference_type = size_type;

    private:
    struct spaghetti_type {
        using value_type = ValueType;
        size_type prev   = 0;
        value_type value = { };
    };

    struct segment_type {
        size_type prev_tail = 0, tail = 0;
    };

    struct list_type {
        size_type index = -1;
        segment_type block;
    };

    using spaghetti = mi_vector<spaghetti_type, size_type>;
    using segment   = mi_vector<segment_type, size_type>;
    using list      = mi_vector<list_type, size_type>;

    public:
    using iterator               = typename spaghetti::iterator;
    using const_iterator         = typename spaghetti::const_iterator;
    using reverse_iterator       = typename spaghetti::reverse_iterator;
    using const_reverse_iterator = typename spaghetti::const_reverse_iterator;

    using pointer         = value_type *;
    using const_pointer   = value_type const *;
    using reference       = value_type &;
    using const_reference = value_type const &;
    using rv_reference    = value_type &&;

    // Emplace/Pop.

    public:
    template<typename... Args>
    [[maybe_unused]] reference emplace ( size_type i_, Args &&... args_ ) {
        validate_tail ( i_ );
        stack.emplace_back ( { std::exchange ( frame[ i_ ].tail, tail_index ( ) ), std::forward<Args> ( args_ )... } );
    }
    [[maybe_unused]] reference push ( size_type i_, const_reference v_ ) { return emplace ( i_, value_type{ v_ } ); }

    // Returns a pair, a reference to the stacked value and the index of the 'new' stack.
    template<typename... Args>
    [[maybe_unused]] sax::pair<reference, size_type> emplace_stack ( Args &&... args_ ) {
        if ( free.empty ( ) ) {
            size_type i = frame.size ( );
            return { stack.emplace_back ( { frame.emplace_back ( i - 1, i ), std::forward<Args> ( args_ )... } ), i };
        }
        else {
            size_type i = pop_free ( ).index;
            return { stack.emplace_back ( { frame.emplace ( frame.begin ( ) + i, i - 1, i ), std::forward<Args> ( args_ )... } ),
                     i };
        }
    }
    [[maybe_unused]] sax::pair<reference, size_type> push_stack ( const_reference v_ ) {
        return emplace_stack ( value_type{ v_ } );
    }

    void remove_stack ( size_type i_ ) {
        segment_type & f     = frame[ i_ ];
        stack[ f.tail ].prev = free.push_back ( i_ );
    }

    [[nodiscard]] size_type find_child ( size_type ) const noexcept {}

    [[maybe_unused]] value_type pop ( size_type i_ ) noexcept {
        assert ( tail_index ( ) );
        if ( segment_type & f = frame[ i_ ]; size_type{ 1 } == ( f.tail - f.prev_tail ) ) {
            stack[ f.tail + 1 ].prev = f.prev_tail;
            free.push_back ( i_, f );
            return stack[ f.tail ].value;
        }
        else {
            return stack[ f.tail-- ].value;
        }
    }

    [[maybe_unused]] value_type pop ( ) noexcept {
        assert ( tail_index ( ) );
        pop_back_after_exit pop_back ( stack );
        frame[ 0 ] = stack.back ( ).prev;
        return stack.back ( ).value;
    }

    // Returns the number of spaghetti-stacks.
    [[nodiscard]] size_type size ( ) const noexcept { return static_cast<size_type> ( frame.size ( ) ); }
    [[nodiscard]] size_type tail_index ( ) const noexcept { return static_cast<size_type> ( stack.size ( ) ); }

    [[nodiscard]] bool validate_tail ( size_type i_ ) const noexcept { assert ( 0 <= i_ and i_ < frame.size ( ) ); }

    template<typename VectorLike>
    struct pop_back_after_exit final {
        pop_back_after_exit ( VectorLike & ptr_ ) noexcept : object{ ptr_ } {}
        ~pop_back_after_exit ( ) noexcept { object.pop_back ( ); }
        VectorLike & object;
    };

    [[nodiscard]] size_type pop_free ( ) noexcept {
        assert ( free.size ( ) );
        pop_back_after_exit pop_back ( free );
        return free.back ( ).index;
    }

    spaghetti stack;
    segment frame = [] { return segment{ }; }( );
    list free;
};

int main ( ) {

    sax::disjoint_set<10, 3> s;

    s.unite ( 1, 3, ATOMIZE ( drinkers ) );
    s.unite ( 0, 1 );
    s.unite ( 2, 5, ATOMIZE ( stoners ) );
    s.unite ( 2, 8 );
    std::cout << s.unite_name ( 6, 9, ATOMIZE ( tea_totalers ) ) << '\n';
    s.unite ( 4, 9 );

    for ( int i = 0; i < 10; ++i )
        std::cout << s.find_name ( i ) << '\n';
    std::cout << '\n';

    exit ( 0 );

    property_architecture p;

    p.value = 2;

    std::cout << p.get ( ) << '\n';

    return EXIT_SUCCESS;
}

/*

There are several ways of implementing such tree-like structures.


(A) One is to allocate each activation record on the heap, and link it
back to the logically surrounding activation record. This is the
method used by most Simula 67 implementations, I believe. The
original implementations of Lisp 1.5 used the heap for the name
records (the a-list), but not for control activations. The advantage
of this technique is that it is simple and uniform, and never runs out
of memory for any particular thread unless there is no more memory
left globally. It also is parsimonious of address space. On the
other hand, activation record allocation and deallocation are more
expensive than in stack-based approaches. It may also have poorer
locality (cache) properties than the other approaches.


(B) Another is to allocate a fixed-size stack whenever you spawn a new
parallel thread. This has all the efficiency advantages of an
ordinary stack, but means that each thread has its own allocation
limit, which may run out long before global memory is exhausted. This
is the method that is implicitly assumed in Ada, for instance, by the
length clause for tasks. It is also the model imposed by library
extensions to stack implementations (like C's pthreads), since each
thread operates exactly as on a single linear stack. It will work
well if the stack requirements of a thread can be predicted
accurately, or if address space is cheap (as on many virtual memory
architectures). It does impose, however, a minimal cost of one
virtual memory page per stack. At the time the Bobrow and Wegbreit
paper was written, both address space and memory were expensive. (I
believe both were working on PDP-10's at the time, with an 18-bit word
address; Bobrow, at BBN, presumably on Tenex, which had paging, but
Wegbreit, at Harvard, on TOPS-10, which did not.)


(C) Yet another technique combines the advantages of these two
techniques by basically allocating activation records in a stack-like
way, but also providing for links among records. It uses a stack-like
piece of memory in a heap-like way. This is the technique described
in the Bobrow and Wegbreit paper. This has the advantage of being
relatively parsimonious of address space, and of being just about as
efficient as a pure stack when multiple stacks are not in use. On the
other hand, it can be wasteful of address space and of memory with
certain patterns of use, e.g. when two co-routined threads recurse
deeply, calling each other, and then one returns, leaving the other
one far into the stack.


I haven'f been able to find an early use of the terms "cactus stack"
and "spaghetti stack" in a quick search of the literature. My
intuition is that (B) is a "cactus stack", because it has linear
pieces of stack connected at their bases, while (C) is a "spaghetti
stack" because the pieces of various stacks are intermixed. (A) is
not a stack approach at all, but a heap approach.


Bobrow and Wegbreit mention approach (A) as "fairly straightforward";
and in fact it had been used in Simula for several years before their
paper. They do not mention approach (B), probably because, although
obvious, it was prohibitively inefficient in their computational
environment. And they use neither the term "spaghetii" nor "cactus"
in their CACM paper. However, later papers (e.g. Kearns82) do refer
to their approach as "spaghetti stacks".


Which approach is appropriate depends (as usual) on the context. In a
context of large numbers of small, short-lived threads, the heap or
the spaghetti approach are probably best. For large, long-lived
threads, the dedicated stack ("cactus") approach (B) is probably best
when virtual memory allows preallocation of large chunks of address
space cheaply. I suspect that most implementations in production
environments these days are cactus and not spaghetti or heap.
Experimental or academic languages, on the other hand, generally use
heaps. I am not aware of current use of spaghetti stacks, but I would
be interested to hear of any.


In summary, Bobrow and Wegbreit _should_ be credited with the concept
but perhaps not the name "spaghetti stacks", but _not_ with the
concept of "cactus stacks".



Kearns82: John P. Kearns, Carol J. Meier, Mary Lou Soffa, _The
Performance Evaluation of Control Implementations_, IEEE Trans. on
Soft. Eng. SE-8:2:89 (March 1982).

*/
