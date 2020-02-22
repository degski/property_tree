
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

#include "detail/includes.hpp"

void compile_command ( ) {}
void link_command ( ) {}
void lib_command ( ) {}

struct command {
    atom name;
    void ( *function ) ( void );
};

#define COMMAND( NAME )                                                                                                            \
    command { #NAME, NAME##_command }

struct command commands[] = { COMMAND ( compile ), COMMAND ( link ), COMMAND ( lib ) };

//

#define PROPERTY( property_name, ... )                                                                                             \
                                                                                                                                   \
    struct property_##property_name {                                                                                              \
        static constexpr atom name                                      = QUOTE_PARAM ( property_name );                           \
        static constexpr std::array<atom, NARGS ( __VA_ARGS__ )> option = { QUOTE_PARAMS ( __VA_ARGS__ ) };                        \
        int value                                                       = 0;                                                       \
        [[nodiscard]] atom get ( ) noexcept { return option[ value ]; }                                                            \
    };

/*
    template<atom Name, typename... Args>

    struct property_property_name {
        static constexpr atom name                                   = Name;
        static constexpr std::array<atom, sizeof ( Args )...> option = { Args... };
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

int main ( ) {

    disjoint_set<int, 10> s;

    s.unite ( 1, 3 );
    s.unite ( 2, 5 );
    s.unite ( 6, 9 );
    s.unite ( 0, 1 );
    s.unite ( 2, 8 );
    s.unite ( 4, 9 );

    for ( int i = 0; i < 10; ++i )
        std::cout << s.find ( i ) << '\n';
    std::cout << '\n';

    exit ( 0 );

    property_architecture p;

    p.value = 2;

    std::cout << p.get ( ) << '\n';

    return EXIT_SUCCESS;
}
