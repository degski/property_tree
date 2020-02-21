
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

#pragma once

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstdlib>

#include <array>
#include <filesystem>
#include <fstream>
#include <initializer_list>
#include <optional>
#include <random>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <vector>

#define FMT_USE_GRISU 1
#include <fmt/core.h>
#include <fmt/format.h>

#include <sax/iostream.hpp>

#include <boost/preprocessor/iteration/local.hpp>

#include <sax/utf8conv.hpp>

#include <plf/plf_nanotimer.h>

#include "detail/const_expr_string.hpp"

// Disk-files and JSON -----------------------------------------------------------------------------------------------------------//

#include <nlohmann/json.hpp>

using json   = nlohmann::json;
namespace fs = std::filesystem;

void json_to_file ( json const & j_, std::string const & path_ ) {
    std::ofstream o ( sax::utf8_to_utf16 ( path_ ) + L".json" );
    o << std::setw ( 4 ) << j_ << std::endl;
    o.flush ( );
    o.close ( );
}

void json_from_file ( json & j_, std::string const & path_ ) {
    std::ifstream i ( sax::utf8_to_utf16 ( path_ ) + L".json" );
    i >> j_;
    i.close ( );
}
[[nodiscard]] json json_from_file ( std::string const & path_ ) {
    json j;
    json_from_file ( j, path_ );
    return j;
}
[[nodiscard]] json json_from_file ( fs::path const & path_ ) { return json_from_file ( path_.string ( ) ); }

[[nodiscard]] std::string string_from_file ( std::string const & path_ ) {
    std::string str;
    if ( std::ifstream is{ sax::utf8_to_utf16 ( path_ ), std::ios::ate } ) {
        std::size_t const size = static_cast<std::size_t> ( is.tellg ( ) );
        str.resize ( size + 1 );
        is.seekg ( 0 );
        is.read ( str.data ( ), size ); // C++17 only.
        str.back ( ) = 0;               // make string zero-terminated.
    }
    return str;
}

// System ------------------------------------------------------------------------------------------------------------------------//

namespace detail {

[[nodiscard]] fs::path data_path_impl ( std::string const & relative_path_ ) noexcept {
    wchar_t * value;
    std::size_t len;
    _wdupenv_s ( &value, &len, L"USERPROFILE" );
    fs::path return_value ( std::wstring ( value ) +
                            std::wstring ( L"\\AppData\\Roaming\\" + sax::utf8_to_utf16 ( relative_path_ ) ) );
    fs::create_directory ( return_value ); // no error raised, if directory exists.
    return return_value;
}
[[nodiscard]] fs::path exe_path_impl ( ) noexcept {
    TCHAR exename[ 1'024 ];
    GetModuleFileName ( NULL, exename, 1'024 );
    return fs::path ( exename ).parent_path ( );
}

} // namespace detail

[[nodiscard]] fs::path const & data_path ( std::string const & relative_path_ = std::string{ } ) noexcept {
    static fs::path const value = detail::data_path_impl ( relative_path_ );
    return value;
}
[[nodiscard]] fs::path const & exe_path ( ) noexcept {
    static fs::path const value = detail::exe_path_impl ( );
    return value;
}

// Example to make conversions ---------------------------------------------------------------------------------------------------//

struct location_t {
    std::string lat, lng;
};

inline void to_json ( json & j_, location_t const & l_ ) { j_ = json{ { "lat", l_.lat }, { "lng", l_.lng } }; }
inline void from_json ( json const & j_, location_t & l_ ) {
    j_.at ( "lat" ).get_to ( l_.lat );
    j_.at ( "lng" ).get_to ( l_.lng );
}

struct place_t {
    location_t location;
    std::string elevation;
    std::string place, country;
    std::string place_country;
};

inline void to_json ( json & j_, place_t const & p_ ) {
    j_ = json{ { "location", p_.location },
               { "elevation", p_.elevation },
               { "place", p_.place },
               { "country", p_.country },
               { "place_country", p_.place_country } };
}
inline void from_json ( json const & j_, place_t & p_ ) {
    j_.at ( "location" ).get_to ( p_.location );
    j_.at ( "elevation" ).get_to ( p_.elevation );
    j_.at ( "place" ).get_to ( p_.place );
    j_.at ( "country" ).get_to ( p_.country );
    j_.at ( "place_country" ).get_to ( p_.place_country );
}

//--------------------------------------------------------------------------------------------------------------------------------//

using atom = char const *;

#define ATOMIZE( x ) ( atom ) #x

// add quotes.
#define QUOTE_PARAM( x ) ATOMIZE ( x )
// trick to get the number of arguments passed to a macro.
#define NARGS_( _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22, _23, _24,     \
                _25, _26, _27, _28, _29, _30, _31, _32, _33, _34, _35, _36, _37, _38, _39, _40, _51, _52, _53, _54, _55, _56, _57, \
                _58, _59, _60, _61, _62, _63, _64, N, ... )                                                                        \
    N
#define NARGS( ... )                                                                                                               \
    NARGS_ ( __VA_ARGS__, 64, 63, 62, 61, 60, 59, 58, 57, 56, 55, 54, 53, 52, 51, 40, 39, 38, 37, 36, 35, 34, 33, 32, 31, 30, 29,  \
             28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1 )
// makes easier to expand the expressions.
#define EXPAND( ... ) __VA_ARGS__
// clang-format off
#define CONCAT_IMPL( x, y ) EXPAND ( x ## y )
// clang-format on
// concatenate two tokens.
#define CONCAT( x, y ) CONCAT_IMPL ( x, y )
// quote all.
#define QUOTE1( x ) ATOMIZE ( x )
#define QUOTE2( x, ... ) ATOMIZE ( x ), QUOTE1 ( __VA_ARGS__ )
#define QUOTE3( x, ... ) ATOMIZE ( x ), QUOTE2 ( __VA_ARGS__ )
#define QUOTE4( x, ... ) ATOMIZE ( x ), QUOTE3 ( __VA_ARGS__ )
#define QUOTE5( x, ... ) ATOMIZE ( x ), QUOTE4 ( __VA_ARGS__ )
#define QUOTE6( x, ... ) ATOMIZE ( x ), QUOTE5 ( __VA_ARGS__ )
#define QUOTE7( x, ... ) ATOMIZE ( x ), QUOTE6 ( __VA_ARGS__ )
#define QUOTE8( x, ... ) ATOMIZE ( x ), QUOTE7 ( __VA_ARGS__ )
#define QUOTE9( x, ... ) ATOMIZE ( x ), QUOTE8 ( __VA_ARGS__ )
#define QUOTE10( x, ... ) ATOMIZE ( x ), QUOTE9 ( __VA_ARGS__ )
#define QUOTE11( x, ... ) ATOMIZE ( x ), QUOTE10 ( __VA_ARGS__ )
#define QUOTE12( x, ... ) ATOMIZE ( x ), QUOTE11 ( __VA_ARGS__ )
#define QUOTE13( x, ... ) ATOMIZE ( x ), QUOTE12 ( __VA_ARGS__ )
#define QUOTE14( x, ... ) ATOMIZE ( x ), QUOTE13 ( __VA_ARGS__ )
#define QUOTE15( x, ... ) ATOMIZE ( x ), QUOTE14 ( __VA_ARGS__ )
#define QUOTE16( x, ... ) ATOMIZE ( x ), QUOTE15 ( __VA_ARGS__ )
#define QUOTE17( x, ... ) ATOMIZE ( x ), QUOTE16 ( __VA_ARGS__ )
#define QUOTE18( x, ... ) ATOMIZE ( x ), QUOTE17 ( __VA_ARGS__ )
#define QUOTE19( x, ... ) ATOMIZE ( x ), QUOTE18 ( __VA_ARGS__ )
#define QUOTE20( x, ... ) ATOMIZE ( x ), QUOTE19 ( __VA_ARGS__ )
#define QUOTE21( x, ... ) ATOMIZE ( x ), QUOTE20 ( __VA_ARGS__ )
#define QUOTE22( x, ... ) ATOMIZE ( x ), QUOTE21 ( __VA_ARGS__ )
#define QUOTE23( x, ... ) ATOMIZE ( x ), QUOTE22 ( __VA_ARGS__ )
#define QUOTE24( x, ... ) ATOMIZE ( x ), QUOTE23 ( __VA_ARGS__ )
#define QUOTE25( x, ... ) ATOMIZE ( x ), QUOTE24 ( __VA_ARGS__ )
#define QUOTE26( x, ... ) ATOMIZE ( x ), QUOTE25 ( __VA_ARGS__ )
#define QUOTE27( x, ... ) ATOMIZE ( x ), QUOTE26 ( __VA_ARGS__ )
#define QUOTE28( x, ... ) ATOMIZE ( x ), QUOTE27 ( __VA_ARGS__ )
#define QUOTE29( x, ... ) ATOMIZE ( x ), QUOTE28 ( __VA_ARGS__ )
#define QUOTE30( x, ... ) ATOMIZE ( x ), QUOTE29 ( __VA_ARGS__ )
#define QUOTE31( x, ... ) ATOMIZE ( x ), QUOTE30 ( __VA_ARGS__ )
#define QUOTE32( x, ... ) ATOMIZE ( x ), QUOTE31 ( __VA_ARGS__ )
#define QUOTE33( x, ... ) ATOMIZE ( x ), QUOTE32 ( __VA_ARGS__ )
#define QUOTE34( x, ... ) ATOMIZE ( x ), QUOTE33 ( __VA_ARGS__ )
#define QUOTE35( x, ... ) ATOMIZE ( x ), QUOTE34 ( __VA_ARGS__ )
#define QUOTE36( x, ... ) ATOMIZE ( x ), QUOTE35 ( __VA_ARGS__ )
#define QUOTE37( x, ... ) ATOMIZE ( x ), QUOTE36 ( __VA_ARGS__ )
#define QUOTE38( x, ... ) ATOMIZE ( x ), QUOTE37 ( __VA_ARGS__ )
#define QUOTE39( x, ... ) ATOMIZE ( x ), QUOTE38 ( __VA_ARGS__ )
#define QUOTE40( x, ... ) ATOMIZE ( x ), QUOTE39 ( __VA_ARGS__ )
#define QUOTE41( x, ... ) ATOMIZE ( x ), QUOTE40 ( __VA_ARGS__ )
#define QUOTE42( x, ... ) ATOMIZE ( x ), QUOTE41 ( __VA_ARGS__ )
#define QUOTE43( x, ... ) ATOMIZE ( x ), QUOTE42 ( __VA_ARGS__ )
#define QUOTE44( x, ... ) ATOMIZE ( x ), QUOTE43 ( __VA_ARGS__ )
#define QUOTE45( x, ... ) ATOMIZE ( x ), QUOTE44 ( __VA_ARGS__ )
#define QUOTE46( x, ... ) ATOMIZE ( x ), QUOTE45 ( __VA_ARGS__ )
#define QUOTE47( x, ... ) ATOMIZE ( x ), QUOTE46 ( __VA_ARGS__ )
#define QUOTE48( x, ... ) ATOMIZE ( x ), QUOTE47 ( __VA_ARGS__ )
#define QUOTE49( x, ... ) ATOMIZE ( x ), QUOTE48 ( __VA_ARGS__ )
#define QUOTE50( x, ... ) ATOMIZE ( x ), QUOTE49 ( __VA_ARGS__ )
#define QUOTE51( x, ... ) ATOMIZE ( x ), QUOTE50 ( __VA_ARGS__ )
#define QUOTE52( x, ... ) ATOMIZE ( x ), QUOTE51 ( __VA_ARGS__ )
#define QUOTE53( x, ... ) ATOMIZE ( x ), QUOTE52 ( __VA_ARGS__ )
#define QUOTE54( x, ... ) ATOMIZE ( x ), QUOTE53 ( __VA_ARGS__ )
#define QUOTE55( x, ... ) ATOMIZE ( x ), QUOTE54 ( __VA_ARGS__ )
#define QUOTE56( x, ... ) ATOMIZE ( x ), QUOTE55 ( __VA_ARGS__ )
#define QUOTE57( x, ... ) ATOMIZE ( x ), QUOTE56 ( __VA_ARGS__ )
#define QUOTE58( x, ... ) ATOMIZE ( x ), QUOTE57 ( __VA_ARGS__ )
#define QUOTE59( x, ... ) ATOMIZE ( x ), QUOTE58 ( __VA_ARGS__ )
#define QUOTE60( x, ... ) ATOMIZE ( x ), QUOTE59 ( __VA_ARGS__ )
#define QUOTE61( x, ... ) ATOMIZE ( x ), QUOTE60 ( __VA_ARGS__ )
#define QUOTE62( x, ... ) ATOMIZE ( x ), QUOTE61 ( __VA_ARGS__ )
#define QUOTE63( x, ... ) ATOMIZE ( x ), QUOTE62 ( __VA_ARGS__ )
#define QUOTE64( x, ... ) ATOMIZE ( x ), QUOTE63 ( __VA_ARGS__ )

#define QUOTE_PARAMS( ... ) EXPAND ( CONCAT ( QUOTE, NARGS ( __VA_ARGS__ ) ) ) ( __VA_ARGS__ )

//--------------------------------------------------------------------------------------------------------------------------------//

// Here is my solution:

#define FRUITS( fruit ) fruit ( Apple ) fruit ( Orange ) fruit ( Banana )

#define CREATE_ENUM( name ) F_##name,

#define CREATE_STRINGS( name ) #name,

/*

    // The trick is that 'fruit' is an argument of the macro 'FRUITS' and will be replaced by what ever you pass to. For example:

    FRUITS ( CREATE_ENUM )

    // will expand to this: F_Apple, F_Orange, F_Banana,

    // Lets create the enum and the string array:

    enum fruit { FRUITS ( CREATE_ENUM ) };

    const char * fruit_names[] = { FRUITS ( CREATE_STRINGS ) };

*/
