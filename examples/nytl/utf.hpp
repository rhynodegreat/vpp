/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2016 nyorain
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#pragma once

#include <string>
#include <array>
#include <locale>
#include <codecvt>
#include <stdexcept>

namespace nytl
{

///Returns the number of characters in a utf8-encoded unicode string.
///This differs from std::string::size because it does not return the bytes in the
///string, but the count of utf8-encoded characters.
inline std::size_t charCount(const std::string& utf8);

///Returns the nth chracter from a utf8-encoded unicode string.
///In difference to the builtin std::string access function this does not return
///the nth byte, but the nth utf8 character.
///Since every (unicode) utf8 character can take up to 4 bytes, an array holding
///4 chars is returned.
///\exception std::out_of_range if n > charCount(utf8)
inline std::array<char, 4> nth(const std::string& utf8, std::size_t n);

///\{
///Returns a reference to the nth character from a utf8-encoded unicode stirng.
///\param size [out] Will hold the number of bytes of the returned character.
///Will be not greater than 4.
///\exception std::out_of_range if n > charCount(utf8)
inline const char& nth(const std::string& utf8, std::size_t n, std::uint8_t& size);
inline char& nth(std::string& utf8, std::size_t n, std::uint8_t& size);
///\}

///\{
///Various conversion functions between different utf unicode encodings.
inline std::u16string utf8to16(const std::string& utf8);
inline std::u32string utf8to32(const std::string& utf8);
inline std::string utf16to8(const std::u16string& utf16);
inline std::u32string utf16to32(const std::u16string& utf16);
inline std::string utf32to8(const std::u32string& utf32);
inline std::u16string utf32to16(const std::u32string& utf32);
///\}

#include <nytl/bits/utf.inl>

}
