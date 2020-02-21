
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
#include <sax/iostream.hpp>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <vector>

#define FMT_USE_GRISU 1
#include <fmt/core.h>
#include <fmt/format.h>

#include <boost/preprocessor/iteration/local.hpp>

#include <sax/utf8conv.hpp>

#include <plf/plf_nanotimer.h>

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

[[nodiscard]] fs::path data_path ( std::string const & relative_path_ ) noexcept {
    wchar_t * value;
    std::size_t len;
    _wdupenv_s ( &value, &len, L"USERPROFILE" );
    fs::path return_value ( std::wstring ( value ) +
                            std::wstring ( L"\\AppData\\Roaming\\" + sax::utf8_to_utf16 ( relative_path_ ) ) );
    fs::create_directory ( return_value ); // no error raised, if directory exists.
    return return_value;
}

[[nodiscard]] fs::path exe_path ( ) noexcept {
    TCHAR exename[ 1'024 ];
    GetModuleFileName ( NULL, exename, 1'024 );
    return fs::path ( exename ).parent_path ( );
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

// add quotes.
#define Q_( x ) #x
#define QUOTE_PARAM( x ) Q_ ( x )
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
// concatenate two tokens.
#define CONCAT( x, y ) CONCAT_ ( x, y )
// clang-format off
#define CONCAT_( x, y ) x ## y
// clang-format on
using atom = char const *;

#define TERMINATE( x ) ( atom ) #x
// quote all.
#define QTE_1( x ) TERMINATE ( x )
#define QTE_2( x, ... ) TERMINATE ( x ), QTE_1 ( __VA_ARGS__ )
#define QTE_3( x, ... ) TERMINATE ( x ), QTE_2 ( __VA_ARGS__ )
#define QTE_4( x, ... ) TERMINATE ( x ), QTE_3 ( __VA_ARGS__ )
#define QTE_5( x, ... ) TERMINATE ( x ), QTE_4 ( __VA_ARGS__ )
#define QTE_6( x, ... ) TERMINATE ( x ), QTE_5 ( __VA_ARGS__ )
#define QTE_7( x, ... ) TERMINATE ( x ), QTE_6 ( __VA_ARGS__ )
#define QTE_8( x, ... ) TERMINATE ( x ), QTE_7 ( __VA_ARGS__ )
#define QTE_9( x, ... ) TERMINATE ( x ), QTE_8 ( __VA_ARGS__ )
#define QTE_10( x, ... ) TERMINATE ( x ), QTE_9 ( __VA_ARGS__ )
#define QTE_11( x, ... ) TERMINATE ( x ), QTE_10 ( __VA_ARGS__ )
#define QTE_12( x, ... ) TERMINATE ( x ), QTE_11 ( __VA_ARGS__ )
#define QTE_13( x, ... ) TERMINATE ( x ), QTE_12 ( __VA_ARGS__ )
#define QTE_14( x, ... ) TERMINATE ( x ), QTE_13 ( __VA_ARGS__ )
#define QTE_15( x, ... ) TERMINATE ( x ), QTE_14 ( __VA_ARGS__ )
#define QTE_16( x, ... ) TERMINATE ( x ), QTE_15 ( __VA_ARGS__ )
#define QTE_17( x, ... ) TERMINATE ( x ), QTE_16 ( __VA_ARGS__ )
#define QTE_18( x, ... ) TERMINATE ( x ), QTE_17 ( __VA_ARGS__ )
#define QTE_19( x, ... ) TERMINATE ( x ), QTE_18 ( __VA_ARGS__ )
#define QTE_20( x, ... ) TERMINATE ( x ), QTE_19 ( __VA_ARGS__ )
#define QTE_21( x, ... ) TERMINATE ( x ), QTE_20 ( __VA_ARGS__ )
#define QTE_22( x, ... ) TERMINATE ( x ), QTE_21 ( __VA_ARGS__ )
#define QTE_23( x, ... ) TERMINATE ( x ), QTE_22 ( __VA_ARGS__ )
#define QTE_24( x, ... ) TERMINATE ( x ), QTE_23 ( __VA_ARGS__ )
#define QTE_25( x, ... ) TERMINATE ( x ), QTE_24 ( __VA_ARGS__ )
#define QTE_26( x, ... ) TERMINATE ( x ), QTE_25 ( __VA_ARGS__ )
#define QTE_27( x, ... ) TERMINATE ( x ), QTE_26 ( __VA_ARGS__ )
#define QTE_28( x, ... ) TERMINATE ( x ), QTE_27 ( __VA_ARGS__ )
#define QTE_29( x, ... ) TERMINATE ( x ), QTE_28 ( __VA_ARGS__ )
#define QTE_30( x, ... ) TERMINATE ( x ), QTE_29 ( __VA_ARGS__ )
#define QTE_31( x, ... ) TERMINATE ( x ), QTE_30 ( __VA_ARGS__ )
#define QTE_32( x, ... ) TERMINATE ( x ), QTE_31 ( __VA_ARGS__ )
#define QTE_33( x, ... ) TERMINATE ( x ), QTE_32 ( __VA_ARGS__ )
#define QTE_34( x, ... ) TERMINATE ( x ), QTE_33 ( __VA_ARGS__ )
#define QTE_35( x, ... ) TERMINATE ( x ), QTE_34 ( __VA_ARGS__ )
#define QTE_36( x, ... ) TERMINATE ( x ), QTE_35 ( __VA_ARGS__ )
#define QTE_37( x, ... ) TERMINATE ( x ), QTE_36 ( __VA_ARGS__ )
#define QTE_38( x, ... ) TERMINATE ( x ), QTE_37 ( __VA_ARGS__ )
#define QTE_39( x, ... ) TERMINATE ( x ), QTE_38 ( __VA_ARGS__ )
#define QTE_40( x, ... ) TERMINATE ( x ), QTE_39 ( __VA_ARGS__ )
#define QTE_41( x, ... ) TERMINATE ( x ), QTE_40 ( __VA_ARGS__ )
#define QTE_42( x, ... ) TERMINATE ( x ), QTE_41 ( __VA_ARGS__ )
#define QTE_43( x, ... ) TERMINATE ( x ), QTE_42 ( __VA_ARGS__ )
#define QTE_44( x, ... ) TERMINATE ( x ), QTE_43 ( __VA_ARGS__ )
#define QTE_45( x, ... ) TERMINATE ( x ), QTE_44 ( __VA_ARGS__ )
#define QTE_46( x, ... ) TERMINATE ( x ), QTE_45 ( __VA_ARGS__ )
#define QTE_47( x, ... ) TERMINATE ( x ), QTE_46 ( __VA_ARGS__ )
#define QTE_48( x, ... ) TERMINATE ( x ), QTE_47 ( __VA_ARGS__ )
#define QTE_49( x, ... ) TERMINATE ( x ), QTE_48 ( __VA_ARGS__ )
#define QTE_50( x, ... ) TERMINATE ( x ), QTE_49 ( __VA_ARGS__ )
#define QTE_51( x, ... ) TERMINATE ( x ), QTE_50 ( __VA_ARGS__ )
#define QTE_52( x, ... ) TERMINATE ( x ), QTE_51 ( __VA_ARGS__ )
#define QTE_53( x, ... ) TERMINATE ( x ), QTE_52 ( __VA_ARGS__ )
#define QTE_54( x, ... ) TERMINATE ( x ), QTE_53 ( __VA_ARGS__ )
#define QTE_55( x, ... ) TERMINATE ( x ), QTE_54 ( __VA_ARGS__ )
#define QTE_56( x, ... ) TERMINATE ( x ), QTE_55 ( __VA_ARGS__ )
#define QTE_57( x, ... ) TERMINATE ( x ), QTE_56 ( __VA_ARGS__ )
#define QTE_58( x, ... ) TERMINATE ( x ), QTE_57 ( __VA_ARGS__ )
#define QTE_59( x, ... ) TERMINATE ( x ), QTE_58 ( __VA_ARGS__ )
#define QTE_60( x, ... ) TERMINATE ( x ), QTE_59 ( __VA_ARGS__ )
#define QTE_61( x, ... ) TERMINATE ( x ), QTE_60 ( __VA_ARGS__ )
#define QTE_62( x, ... ) TERMINATE ( x ), QTE_61 ( __VA_ARGS__ )
#define QTE_63( x, ... ) TERMINATE ( x ), QTE_62 ( __VA_ARGS__ )
#define QTE_64( x, ... ) TERMINATE ( x ), QTE_63 ( __VA_ARGS__ )

#define QUOTE_PARAMS( ... ) EXPAND ( CONCAT ( QTE_, NARGS ( __VA_ARGS__ ) ) ) ( __VA_ARGS__ )

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
