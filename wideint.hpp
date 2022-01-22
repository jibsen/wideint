//
// wideint - wide exact-width integer types
//
// Copyright (c) 2022 Joergen Ibsen
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//

#pragma once

#include <algorithm>
#include <array>
#include <bit>
#include <compare>
#include <cstdint>
#include <functional>
#include <iostream>
#include <string>
#include <string_view>

namespace wideint {

template<std::size_t width>
struct wuint {
	static constexpr wuint<width> min() {
		return wuint<width>(0);
	}

	static constexpr wuint<width> max() {
		wuint<width> res(0);
		res.cells.fill(-1);
		return res;
	}

	constexpr explicit wuint(std::uint32_t c) : cells{c} {}
	constexpr explicit wuint(std::string_view sv);

	constexpr wuint<width> &operator=(std::uint32_t c) {
		cells.fill(0);
		cells[0] = c;

		return *this;
	}

	constexpr wuint<width> &operator+=(const wuint<width> &rhs);
	constexpr wuint<width> &operator-=(const wuint<width> &rhs);
	constexpr wuint<width> &operator*=(const wuint<width> &rhs);
	constexpr wuint<width> &operator/=(const wuint<width> &rhs);
	constexpr wuint<width> &operator%=(const wuint<width> &rhs);

	constexpr wuint<width> &operator&=(const wuint<width> &rhs);
	constexpr wuint<width> &operator|=(const wuint<width> &rhs);
	constexpr wuint<width> &operator^=(const wuint<width> &rhs);

	constexpr wuint<width> &operator<<=(unsigned int shift);
	constexpr wuint<width> &operator>>=(unsigned int shift);

	constexpr wuint<width> &operator+=(std::uint32_t c);
	constexpr wuint<width> &operator-=(std::uint32_t c);
	constexpr wuint<width> &operator*=(std::uint32_t c);
	constexpr wuint<width> &operator/=(std::uint32_t c);
	constexpr wuint<width> &operator%=(std::uint32_t c);

	constexpr wuint<width> &operator&=(std::uint32_t c);
	constexpr wuint<width> &operator|=(std::uint32_t c);
	constexpr wuint<width> &operator^=(std::uint32_t c);

	constexpr bool is_zero() const {
		for (std::size_t i = 0; i != width; ++i) {
			if (cells[i]) {
				return false;
			}
		}

		return true;
	}

	constexpr explicit operator bool() const { return !is_zero(); }

	constexpr bool is_negative() const {
		return cells.back() & (std::uint32_t(1) << 31);
	}

	constexpr std::size_t log2() const {
		for (std::size_t i = width; i--; ) {
			if (cells[i]) {
				return std::bit_width(cells[i]) + 32 * i;
			}
		}

		return 0;
	}

	constexpr std::uint32_t getbit(unsigned int bit) const {
		unsigned int pos = bit / 32;
		unsigned int offs = bit % 32;

		return (cells[pos] >> offs) & 1U;
	}

	constexpr wuint<width> &setbit(unsigned int bit) {
		unsigned int pos = bit / 32;
		unsigned int offs = bit % 32;

		cells[pos] |= std::uint32_t(1) << offs;

		return *this;
	}

	constexpr wuint<width> &operator++() {
		std::uint32_t carry = 1;

		for (std::size_t i = 0; carry && i != width; ++i) {
			std::uint64_t w = static_cast<std::uint64_t>(cells[i]) + carry;
			cells[i] = static_cast<std::uint32_t>(w);
			carry = static_cast<std::uint32_t>(w >> 32);
		}

        	return *this;
	}

	constexpr wuint<width> operator++(int) {
		wuint<width> old = *this;
		operator++();
		return old;
	}

	constexpr wuint<width> &operator--() {
		std::uint32_t borrow = 1;

		for (std::size_t i = 0; borrow && i != width; ++i) {
			std::uint64_t w = static_cast<std::uint64_t>(cells[i]) - borrow;
			cells[i] = static_cast<std::uint32_t>(w);
			borrow = static_cast<std::uint32_t>(w >> 32) ? 1 : 0;
		}

		return *this;
	}

	constexpr wuint<width> operator--(int) {
		wuint<width> old = *this;
		operator--();
		return old;
	}

	constexpr std::strong_ordering operator<=>(const wuint<width> &rhs) const {
		for (std::size_t i = width; i--; ) {
			if (cells[i] != rhs.cells[i]) {
				return cells[i] <=> rhs.cells[i];
			}
		}

		return std::strong_ordering::equal;
	}

	constexpr std::strong_ordering operator<=>(std::uint32_t c) const
	{
		for (std::size_t i = width - 1; i != 0; --i) {
			if (cells[i] != 0) {
				return std::strong_ordering::greater;
			}
		}

		return cells[0] <=> c;
	}

	constexpr bool operator==(const wuint<width> &) const = default;

	constexpr bool operator==(std::uint32_t c) const {
		if (cells.front() != c) {
			return false;
		}

		for (std::size_t i = 1; i != width; ++i) {
			if (cells[i]) {
				return false;
			}
		}

		return true;
	}

	std::array<std::uint32_t, width> cells;
};

template<std::size_t width>
constexpr wuint<width> operator~(const wuint<width> &obj)
{
	wuint<width> res(obj);

	for (auto &cell : res.cells) {
		cell = ~cell;
	}

	return res;
}

template<std::size_t width>
constexpr wuint<width> operator-(const wuint<width> &obj)
{
	wuint<width> res(obj);
	std::uint32_t carry = 1;

	for (std::size_t i = 0; i != width; ++i) {
		std::uint64_t w = static_cast<std::uint64_t>(static_cast<std::uint32_t>(~res.cells[i])) + carry;
		res.cells[i] = static_cast<std::uint32_t>(w);
		carry = static_cast<std::uint32_t>(w >> 32);
	}

       	return res;
}

template<std::size_t width>
constexpr wuint<width> &wuint<width>::operator+=(const wuint<width> &rhs)
{
	std::uint32_t carry = 0;

	for (std::size_t i = 0; i != width; ++i) {
		std::uint64_t w = static_cast<std::uint64_t>(cells[i]) + rhs.cells[i] + carry;
		cells[i] = static_cast<std::uint32_t>(w);
		carry = static_cast<std::uint32_t>(w >> 32);
	}

	return *this;
}

template<std::size_t width>
constexpr wuint<width> operator+(const wuint<width> &lhs, const wuint<width> &rhs)
{
	wuint<width> res(lhs);
	res += rhs;
	return res;
}

template<std::size_t width>
constexpr wuint<width> &wuint<width>::operator-=(const wuint<width> &rhs)
{
	std::uint32_t borrow = 0;

	for (std::size_t i = 0; i != width; ++i) {
		std::uint64_t w = static_cast<std::uint64_t>(cells[i]) - rhs.cells[i] - borrow;
		cells[i] = static_cast<std::uint32_t>(w);
		borrow = static_cast<std::uint32_t>(w >> 32) ? 1 : 0;
	}

	return *this;
}

template<std::size_t width>
constexpr wuint<width> operator-(const wuint<width> &lhs, const wuint<width> &rhs)
{
	wuint<width> res(lhs);
	res -= rhs;
	return res;
}

template<std::size_t width>
constexpr wuint<width> &wuint<width>::operator*=(const wuint<width> &rhs)
{
	wuint<width> res(0);

	for (std::size_t i = 0; i != width; ++i) {
		if (cells[i]) {
			std::uint32_t carry = 0;

			for (std::size_t j = 0; i + j != width; ++j) {
				std::uint64_t w = static_cast<std::uint64_t>(cells[i]) * rhs.cells[j] + res.cells[i + j] + carry;
				res.cells[i + j] = static_cast<std::uint32_t>(w);
				carry = static_cast<std::uint32_t>(w >> 32);
			}
		}
	}

	*this = res;

	return *this;
}

template<std::size_t width>
constexpr wuint<width> operator*(const wuint<width> &lhs, const wuint<width> &rhs)
{
	wuint<width> res(0);

	for (std::size_t i = 0; i != width; ++i) {
		if (lhs.cells[i]) {
			std::uint32_t carry = 0;

			for (std::size_t j = 0; i + j != width; ++j) {
				std::uint64_t w = static_cast<std::uint64_t>(lhs.cells[i]) * rhs.cells[j] + res.cells[i + j] + carry;
				res.cells[i + j] = static_cast<std::uint32_t>(w);
				carry = static_cast<std::uint32_t>(w >> 32);
			}
		}
	}

	return res;
}

template<std::size_t width>
constexpr wuint<width> &wuint<width>::operator/=(const wuint<width> &rhs)
{
	auto lhs_bit_size = log2();
	auto rhs_bit_size = rhs.log2();

	auto adjust = lhs_bit_size < rhs_bit_size ? 0 : lhs_bit_size - rhs_bit_size;

	wuint<width> quot(0);
	wuint<width> rem(*this);
	wuint<width> rhs_adjusted = rhs << static_cast<unsigned int>(adjust);

	for (std::size_t bit_i = adjust + 1; bit_i--; ) {
		auto cmp = rem <=> rhs_adjusted;

		if (cmp >= 0) {
			quot.setbit(static_cast<unsigned int>(bit_i));

			rem -= rhs_adjusted;

			if (cmp == 0) {
				break;
			}
		}

		rhs_adjusted >>= 1;
	}

	*this = quot;

	return *this;
}

template<std::size_t width>
constexpr wuint<width> operator/(const wuint<width> &lhs, const wuint<width> &rhs)
{
	wuint<width> res(lhs);
	res /= rhs;
	return res;
}

template<std::size_t width>
constexpr wuint<width> &wuint<width>::operator%=(const wuint<width> &rhs)
{
	auto lhs_bit_size = log2();
	auto rhs_bit_size = rhs.log2();

	auto adjust = lhs_bit_size < rhs_bit_size ? 0 : lhs_bit_size - rhs_bit_size;

	wuint<width> rem(*this);
	wuint<width> rhs_adjusted = rhs << static_cast<unsigned int>(adjust);

	for (std::size_t bit_i = adjust + 1; bit_i--; ) {
		auto cmp = rem <=> rhs_adjusted;

		if (cmp >= 0) {
			rem -= rhs_adjusted;

			if (cmp == 0) {
				break;
			}
		}

		rhs_adjusted >>= 1;
	}

	*this = rem;

	return *this;
}

template<std::size_t width>
constexpr wuint<width> operator%(const wuint<width> &lhs, const wuint<width> &rhs)
{
	wuint<width> res(lhs);
	res %= rhs;
	return res;
}

template<std::size_t width>
constexpr wuint<width> &wuint<width>::operator&=(const wuint<width> &rhs)
{
	for (std::size_t i = 0; i != width; ++i) {
		cells[i] &= rhs.cells[i];
	}

	return *this;
}

template<std::size_t width>
constexpr wuint<width> operator&(const wuint<width> &lhs, const wuint<width> &rhs)
{
	wuint<width> res(lhs);
	res &= rhs;
	return res;
}

template<std::size_t width>
constexpr wuint<width> &wuint<width>::operator|=(const wuint<width> &rhs)
{
	for (std::size_t i = 0; i != width; ++i) {
		cells[i] |= rhs.cells[i];
	}

	return *this;
}

template<std::size_t width>
constexpr wuint<width> operator|(const wuint<width> &lhs, const wuint<width> &rhs)
{
	wuint<width> res(lhs);
	res |= rhs;
	return res;
}

template<std::size_t width>
constexpr wuint<width> &wuint<width>::operator^=(const wuint<width> &rhs)
{
	for (std::size_t i = 0; i != width; ++i) {
		cells[i] ^= rhs.cells[i];
	}

	return *this;
}

template<std::size_t width>
constexpr wuint<width> operator^(const wuint<width> &lhs, const wuint<width> &rhs)
{
	wuint<width> res(lhs);
	res ^= rhs;
	return res;
}

template<std::size_t width>
constexpr wuint<width> &wuint<width>::operator<<=(unsigned int shift)
{
	std::size_t pos = shift / 32;
	std::size_t offs = shift % 32;

	for (std::size_t i = pos; i-- > width - pos; ) {
		cells[i] = 0;
	}

	std::uint64_t w = cells[width - pos - 1];
	cells[width - pos - 1] = 0;

	for (std::size_t i = width - pos - 1; i--; ) {
		w = (w << 32) + cells[i];
		cells[i] = 0;

		cells[i + pos + 1] = static_cast<std::uint32_t>(w >> (32 - offs));
	}

	cells[pos] = static_cast<std::uint32_t>(w << offs);

	return *this;
}

template<std::size_t width>
constexpr wuint<width> operator<<(const wuint<width> &lhs, unsigned int shift)
{
	wuint<width> res(lhs);
	res <<= shift;
	return res;
}

template<std::size_t width>
constexpr wuint<width> &wuint<width>::operator>>=(unsigned int shift)
{
	std::size_t pos = shift / 32;
	std::size_t offs = shift % 32;

	std::uint64_t w = static_cast<std::uint64_t>(cells[pos]) << 32;
	cells[pos] = 0;

	for (std::size_t i = 0; i != width - pos - 1; ++i) {
		w = (w >> 32) + (static_cast<std::uint64_t>(cells[i + pos + 1]) << 32);
		cells[i + pos + 1] = 0;

		cells[i] = static_cast<std::uint32_t>(w >> offs);
	}

	cells[width - pos - 1] = static_cast<std::uint32_t>(w >> (32 + offs));

	for (std::size_t i = width - pos; i < pos; ++i) {
		cells[i] = 0;
	}

	return *this;
}

template<std::size_t width>
constexpr wuint<width> operator>>(const wuint<width> &lhs, unsigned int shift)
{
	wuint<width> res(lhs);
	res >>= shift;
	return res;
}

template<std::size_t width>
constexpr wuint<width> &wuint<width>::operator+=(std::uint32_t c)
{
	std::uint32_t carry = c;

	for (std::size_t i = 0; i != width; ++i) {
		std::uint64_t w = static_cast<std::uint64_t>(cells[i]) + carry;
		cells[i] = static_cast<std::uint32_t>(w);
		carry = static_cast<std::uint32_t>(w >> 32);
	}

	return *this;
}

template<std::size_t width>
constexpr wuint<width> operator+(const wuint<width> &lhs, std::uint32_t c)
{
	wuint<width> res(lhs);
	res += c;
	return res;
}

template<std::size_t width>
constexpr wuint<width> operator+(std::uint32_t c, const wuint<width> &rhs)
{
	wuint<width> res(rhs);
	res += c;
	return res;
}

template<std::size_t width>
constexpr wuint<width> &wuint<width>::operator-=(std::uint32_t c)
{
	std::uint32_t borrow = c;

	for (std::size_t i = 0; i != width; ++i) {
		std::uint64_t w = static_cast<std::uint64_t>(cells[i]) - borrow;
		cells[i] = static_cast<std::uint32_t>(w);
		borrow = static_cast<std::uint32_t>(w >> 32) ? 1 : 0;
	}

	return *this;
}

template<std::size_t width>
constexpr wuint<width> operator-(const wuint<width> &lhs, std::uint32_t c)
{
	wuint<width> res(lhs);
	res -= c;
	return res;
}

template<std::size_t width>
constexpr wuint<width> operator-(std::uint32_t c, const wuint<width> &rhs)
{
	wuint<width> res(c);
	res -= rhs;
	return res;
}

template<std::size_t width>
constexpr wuint<width> &wuint<width>::operator*=(std::uint32_t c)
{
	std::uint32_t carry = 0;

	for (std::size_t i = 0; i != width; ++i) {
		std::uint64_t w = static_cast<std::uint64_t>(cells[i]) * c + carry;
		cells[i] = static_cast<std::uint32_t>(w);
		carry = static_cast<std::uint32_t>(w >> 32);
	}

	return *this;
}

template<std::size_t width>
constexpr wuint<width> operator*(const wuint<width> &lhs, std::uint32_t c)
{
	wuint<width> res(lhs);
	res *= c;
	return res;
}

template<std::size_t width>
constexpr wuint<width> operator*(std::uint32_t c, const wuint<width> &rhs)
{
	wuint<width> res(rhs);
	res *= c;
	return res;
}

template<std::size_t width>
constexpr wuint<width> &wuint<width>::operator/=(std::uint32_t c)
{
	std::uint64_t w = 0;

	for (std::size_t i = width; i--; ) {
		w = (w << 32) + cells[i];
		cells[i] = static_cast<std::uint32_t>(w / static_cast<std::uint64_t>(c));
		w %= static_cast<std::uint64_t>(c);
	}

	return *this;
}

template<std::size_t width>
constexpr wuint<width> operator/(const wuint<width> &lhs, std::uint32_t c)
{
	wuint<width> res(lhs);
	res /= c;
	return res;
}

template<std::size_t width>
constexpr wuint<width> operator/(std::uint32_t c, const wuint<width> &rhs)
{
	wuint<width> res(0);
	if (c >= rhs) {
		res.cells[0] = c / rhs.cells[0];
	}
	return res;
}

template<std::size_t width>
constexpr wuint<width> &wuint<width>::operator%=(std::uint32_t c)
{
	std::uint64_t w = 0;

	for (std::size_t i = width; i--; ) {
		w = (w << 32) + cells[i];
		w %= static_cast<std::uint64_t>(c);
	}

	*this = static_cast<std::uint32_t>(w);

	return *this;
}

template<std::size_t width>
constexpr std::uint32_t operator%(const wuint<width> &lhs, std::uint32_t c)
{
	std::uint64_t w = 0;

	for (std::size_t i = width; i--; ) {
		w = (w << 32) + lhs.cells[i];
		w %= static_cast<std::uint64_t>(c);
	}

	return static_cast<std::uint32_t>(w);
}

template<std::size_t width>
constexpr wuint<width> operator%(std::uint32_t c, const wuint<width> &rhs)
{
	wuint<width> res(c);
	if (c >= rhs) {
		res.cells[0] = c % rhs.cells[0];
	}
	return res;
}

template<std::size_t width>
constexpr wuint<width> &wuint<width>::operator&=(std::uint32_t c)
{
	*this = cells[0] & c;

	return *this;
}

template<std::size_t width>
constexpr std::uint32_t operator&(const wuint<width> &lhs, std::uint32_t c)
{
	return lhs.cells[0] & c;
}

template<std::size_t width>
constexpr wuint<width> operator&(std::uint32_t c, const wuint<width> &rhs)
{
	wuint<width> res(0);
	res.cells[0] = c & rhs.cells[0];
	return res;
}

template<std::size_t width>
constexpr wuint<width> &wuint<width>::operator|=(std::uint32_t c)
{
	cells[0] |= c;

	return *this;
}

template<std::size_t width>
constexpr wuint<width> operator|(const wuint<width> &lhs, std::uint32_t c)
{
	wuint<width> res(lhs);
	res |= c;
	return res;
}

template<std::size_t width>
constexpr wuint<width> operator|(std::uint32_t c, const wuint<width> &rhs)
{
	wuint<width> res(rhs);
	res |= c;
	return res;
}

template<std::size_t width>
constexpr wuint<width> &wuint<width>::operator^=(std::uint32_t c)
{
	cells[0] ^= c;

	return *this;
}

template<std::size_t width>
constexpr wuint<width> operator^(const wuint<width> &lhs, std::uint32_t c)
{
	wuint<width> res(lhs);
	res ^= c;
	return res;
}

template<std::size_t width>
constexpr wuint<width> operator^(std::uint32_t c, const wuint<width> &rhs)
{
	wuint<width> res(rhs);
	res ^= c;
	return res;
}

template<std::size_t width>
constexpr wuint<width> abs(const wuint<width> &obj)
{
	return obj.is_negative() ? -obj : obj;
}

template<std::size_t width>
constexpr wuint<width> idiv(const wuint<width> &lhs, const wuint<width> &rhs)
{
	wuint<width> res = abs(lhs) / abs(rhs);

	return lhs.is_negative() != rhs.is_negative() ? -res : res;
}

template<std::size_t width>
constexpr wuint<width> imod(const wuint<width> &lhs, const wuint<width> &rhs)
{
	wuint<width> res = abs(lhs) % abs(rhs);

	return lhs.is_negative() ? -res : res;
}

template<std::size_t width>
constexpr wuint<width> shiftar(const wuint<width> &lhs, unsigned int shift)
{
	wuint<width> res(lhs);

	std::size_t pos = shift / 32;
	std::size_t offs = shift % 32;

	std::uint32_t fill = res.is_negative() ? -1 : 0;

	std::uint64_t w = static_cast<std::uint64_t>(res.cells[pos]) << 32;
	res.cells[pos] = fill;

	for (std::size_t i = 0; i != width - pos - 1; ++i) {
		w = (w >> 32) + (static_cast<std::uint64_t>(res.cells[i + pos + 1]) << 32);
		res.cells[i + pos + 1] = fill;

		res.cells[i] = static_cast<std::uint32_t>(w >> offs);
	}

	res.cells[width - pos - 1] = static_cast<std::uint32_t>(
		((w >> 32) | (static_cast<std::uint64_t>(fill) << 32)) >> offs
	);

	for (std::size_t i = width - pos; i < pos; ++i) {
		res.cells[i] = fill;
	}

	return res;
}

template<std::size_t width>
constexpr std::string to_string(const wuint<width> &obj)
{
	if (obj.is_zero()) {
		return "0";
	}

	std::string res;

	for (wuint<width> tmp = obj; !tmp.is_zero(); tmp /= 10) {
		res.push_back((tmp % 10) + '0');
	}

	std::ranges::reverse(res);

	return res;
}

template<std::size_t width>
constexpr wuint<width> from_string(std::string_view sv)
{
	wuint<width> res(0);

	bool negative = false;

	if (sv.starts_with('-')) {
		negative = true;
		sv.remove_prefix(1);
	}

	if (sv.starts_with("0x") || sv.starts_with("0X"))
	{
		sv.remove_prefix(2);

		for (auto ch : sv) {
			if (ch >= '0' && ch <= '9') {
				res = res * 16 + (ch - '0');
			}
			else if (ch >= 'a' && ch <= 'f') {
				res = res * 16 + (ch - 'a' + 10);
			}
			else if (ch >= 'A' && ch <= 'F') {
				res = res * 16 + (ch - 'A' + 10);
			}
			else {
				break;
			}
		}
	}
	else {
		for (auto ch : sv) {
			if (ch < '0' || ch > '9') {
				break;
			}

			res = res * 10 + (ch - '0');
		}
	}

	return negative ? -res : res;
}

template<std::size_t width>
constexpr wuint<width>::wuint(std::string_view sv)
{
	*this = from_string<width>(sv);
}

template<std::size_t width>
std::ostream &operator<<(std::ostream &os, const wuint<width> &obj)
{
	return os << to_string(obj);
}

template<std::size_t width>
std::istream &operator>>(std::istream &is, wuint<width> &obj)
{
	char ch = '\0';

	if (!(is >> ch)) {
		obj = 0;
		return is;
	}

	std::string s;

	if (ch == '-') {
		s.push_back(ch);

		if (!is.get(ch)) {
			obj = 0;
			return is;
		}
	}

	if (!(ch >= '0' && ch <= '9')) {
		obj = 0;
		is.setstate(std::ios::failbit);
		return is;
	}

	s.push_back(ch);

	while (std::istream::traits_type::not_eof(is.peek()) && is.get(ch)) {
		if (ch >= '0' && ch <= '9') {
			s.push_back(ch);
		}
		else {
			is.unget();
			break;
		}
	}

	obj = from_string<width>(s);

	return is;
}

} // namespace wideint

template<std::size_t width>
struct std::hash<wideint::wuint<width>>
{
	std::size_t operator()(const wideint::wuint<width> &obj) const noexcept
	{
		std::size_t hash = 17;
		for (const auto &cell : obj.cells) {
			hash = hash * 37 + std::hash<std::uint32_t>()(cell);
		}
		return hash;
	}
};
