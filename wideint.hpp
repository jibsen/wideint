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

#include <array>
#include <bit>
#include <compare>
#include <cstdint>

namespace wideint {

template<std::size_t width>
struct wuint {
	constexpr explicit wuint(std::uint32_t c) : cells{c} {}

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

	constexpr wuint<width> &operator<<=(unsigned int shift);
	constexpr wuint<width> &operator>>=(unsigned int shift);

	constexpr wuint<width> &operator+=(std::uint32_t c);
	constexpr wuint<width> &operator-=(std::uint32_t c);
	constexpr wuint<width> &operator*=(std::uint32_t c);
	constexpr wuint<width> &operator/=(std::uint32_t c);
	constexpr wuint<width> &operator%=(std::uint32_t c);

	constexpr bool is_zero() const {
		for (std::size_t i = 0; i != width; ++i) {
			if (cells[i]) {
				return false;
			}
		}

		return true;
	}

	constexpr bool is_negative() const {
		return cells.back() & (std::uint32_t(1) << 31);
	}

	constexpr std::size_t log2() const {
		for (std::size_t i = width; i-- != 0; ) {
			if (cells[i]) {
				return std::bit_width(cells[i]) + 32 * i;
			}
		}

		return 0;
	}

	constexpr std::uint32_t getbit(unsigned int bit) const {
		unsigned int pos = bit / 32;
		unsigned int offs = bit % 32;

		return (cells[pos] >> offs) & 1u;
	}

	constexpr wuint<width> &operator++() {
		std::uint32_t carry = 1;

		for (std::size_t i = 0; carry && i != width; ++i) {
			std::uint64_t w = std::uint64_t(cells[i]) + carry;
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
			std::uint64_t w = std::uint64_t(cells[i]) - borrow;
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
		for (std::size_t i = width; i-- != 0; ) {
			if (cells[i] != rhs.cells[i]) {
				return cells[i] <=> rhs.cells[i];
			}
		}

		return std::strong_ordering::equal;
	}

	bool operator==(const wuint<width> &) const = default;

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
constexpr wuint<width> operator~(wuint<width> obj)
{
	for (auto &cell : obj.cells) {
		cell = ~cell;
	}

	return obj;
}

template<std::size_t width>
constexpr wuint<width> operator-(wuint<width> obj)
{
	std::uint32_t carry = 1;

	for (std::size_t i = 0; i != width; ++i) {
		std::uint64_t w = std::uint64_t(std::uint32_t(~obj.cells[i])) + carry;
		obj.cells[i] = static_cast<std::uint32_t>(w);
		carry = static_cast<std::uint32_t>(w >> 32);
	}

       	return obj;
}

template<std::size_t width>
constexpr wuint<width> &wuint<width>::operator+=(const wuint<width> &rhs)
{
	std::uint32_t carry = 0;

	for (std::size_t i = 0; i != width; ++i) {
		std::uint64_t w = std::uint64_t(cells[i]) + rhs.cells[i] + carry;
		cells[i] = static_cast<std::uint32_t>(w);
		carry = static_cast<std::uint32_t>(w >> 32);
	}

	return *this;
}

template<std::size_t width>
constexpr wuint<width> operator+(wuint<width> lhs, const wuint<width> &rhs)
{
	return lhs += rhs;
}

template<std::size_t width>
constexpr wuint<width> &wuint<width>::operator-=(const wuint<width> &rhs)
{
	std::uint32_t borrow = 0;

	for (std::size_t i = 0; i != width; ++i) {
		std::uint64_t w = std::uint64_t(cells[i]) - rhs.cells[i] - borrow;
		cells[i] = static_cast<std::uint32_t>(w);
		borrow = static_cast<std::uint32_t>(w >> 32) ? 1 : 0;
	}

	return *this;
}

template<std::size_t width>
constexpr wuint<width> operator-(wuint<width> lhs, const wuint<width> &rhs)
{
	return lhs -= rhs;
}

template<std::size_t width>
constexpr wuint<width> &wuint<width>::operator*=(const wuint<width> &rhs)
{
	wuint<width> t(0);

	for (std::size_t i = 0; i != width; ++i) {
		if (cells[i]) {
			std::uint32_t carry = 0;

			for (std::size_t j = 0; i + j != width; ++j) {
				std::uint64_t w = std::uint64_t(cells[i]) * rhs.cells[j] + t.cells[i + j] + carry;
				t.cells[i + j] = static_cast<std::uint32_t>(w);
				carry = static_cast<std::uint32_t>(w >> 32);
			}
		}
	}

	*this = t;

	return *this;
}

template<std::size_t width>
constexpr wuint<width> operator*(wuint<width> lhs, const wuint<width> &rhs)
{
	return lhs *= rhs;
}

template<std::size_t width>
constexpr wuint<width> &wuint<width>::operator%=(const wuint<width> &rhs)
{
	wuint<width> r(0);

	for (std::size_t bit_i = log2(); bit_i-- != 0; ) {
		std::uint32_t carry = (cells[bit_i / 32] & (std::uint32_t(1) << (bit_i % 32))) ? 1 : 0;

		for (std::size_t i = 0; i != width; ++i) {
			std::uint64_t w = (std::uint64_t(r.cells[i]) << 1) + carry;
			r.cells[i] = static_cast<std::uint32_t>(w);
			carry = static_cast<std::uint32_t>(w >> 32);
		}

		if (r >= rhs) {
			r -= rhs;
		}
	}

	*this = r;

	return *this;
}

template<std::size_t width>
constexpr wuint<width> operator%(wuint<width> lhs, const wuint<width> &rhs)
{
	return lhs %= rhs;
}

template<std::size_t width>
constexpr wuint<width> &wuint<width>::operator/=(const wuint<width> &rhs)
{
	wuint<width> q(0);
	wuint<width> r(0);

	for (std::size_t bit_i = log2(); bit_i-- != 0; ) {
		std::uint32_t carry = (cells[bit_i / 32] & (std::uint32_t(1) << (bit_i % 32))) ? 1 : 0;

		for (std::size_t i = 0; i != width; ++i) {
			std::uint64_t w = (std::uint64_t(r.cells[i]) << 1) + carry;
			r.cells[i] = static_cast<std::uint32_t>(w);
			carry = static_cast<std::uint32_t>(w >> 32);
		}

		if (r >= rhs) {
			r -= rhs;
			q.cells[bit_i / 32] |= std::uint32_t(1) << (bit_i % 32);
		}
	}

	*this = q;

	return *this;
}

template<std::size_t width>
constexpr wuint<width> operator/(wuint<width> lhs, const wuint<width> &rhs)
{
	return lhs /= rhs;
}

template<std::size_t width>
constexpr wuint<width> &wuint<width>::operator<<=(unsigned int shift)
{
	std::size_t pos = shift / 32;
	std::size_t offs = shift % 32;

	std::uint64_t w = cells[width - pos - 1];
	cells[width - pos - 1] = 0;

	for (std::size_t i = width - pos - 1; i-- != 0; ) {
		w = (w << 32) + cells[i];
		cells[i] = 0;

		cells[i + pos + 1] = static_cast<std::uint32_t>(w >> (32 - offs));
	}

	cells[pos] = static_cast<std::uint32_t>(w << offs);

	return *this;
}

template<std::size_t width>
constexpr wuint<width> operator<<(wuint<width> lhs, unsigned int shift)
{
	return lhs <<= shift;
}

template<std::size_t width>
constexpr wuint<width> &wuint<width>::operator>>=(unsigned int shift)
{
	std::size_t pos = shift / 32;
	std::size_t offs = shift % 32;

	std::uint64_t w = std::uint64_t(cells[pos]) << 32;
	cells[pos] = 0;

	for (std::size_t i = 0; i != width - pos - 1; ++i) {
		w = (w >> 32) + (std::uint64_t(cells[i + pos + 1]) << 32);
		cells[i + pos + 1] = 0;

		cells[i] = static_cast<std::uint32_t>(w >> offs);
	}

	cells[width - pos - 1] = static_cast<std::uint32_t>(w >> (32 + offs));

	return *this;
}

template<std::size_t width>
constexpr wuint<width> operator>>(wuint<width> lhs, unsigned int shift)
{
	return lhs >>= shift;
}

template<std::size_t width>
constexpr wuint<width> &wuint<width>::operator+=(std::uint32_t c)
{
	std::uint32_t carry = c;

	for (std::size_t i = 0; i != width; ++i) {
		std::uint64_t w = std::uint64_t(cells[i]) + carry;
		cells[i] = static_cast<std::uint32_t>(w);
		carry = static_cast<std::uint32_t>(w >> 32);
	}

	return *this;
}

template<std::size_t width>
constexpr wuint<width> operator+(wuint<width> lhs, std::uint32_t c)
{
	return lhs += c;
}

template<std::size_t width>
constexpr wuint<width> operator+(std::uint32_t c, wuint<width> rhs)
{
	return rhs += c;
}

template<std::size_t width>
constexpr wuint<width> &wuint<width>::operator-=(std::uint32_t c)
{
	std::uint32_t borrow = c;

	for (std::size_t i = 0; i != width; ++i) {
		std::uint64_t w = std::uint64_t(cells[i]) - borrow;
		cells[i] = static_cast<std::uint32_t>(w);
		borrow = static_cast<std::uint32_t>(w >> 32) ? 1 : 0;
	}

	return *this;
}

template<std::size_t width>
constexpr wuint<width> operator-(wuint<width> lhs, std::uint32_t c)
{
	return lhs -= c;
}

template<std::size_t width>
constexpr wuint<width> &wuint<width>::operator*=(std::uint32_t c)
{
	std::uint32_t carry = 0;

	for (std::size_t i = 0; i != width; ++i) {
		std::uint64_t w = std::uint64_t(cells[i]) * c + carry;
		cells[i] = static_cast<std::uint32_t>(w);
		carry = static_cast<std::uint32_t>(w >> 32);
	}

	return *this;
}

template<std::size_t width>
constexpr wuint<width> operator*(wuint<width> lhs, std::uint32_t c)
{
	return lhs *= c;
}

template<std::size_t width>
constexpr wuint<width> operator*(std::uint32_t c, wuint<width> rhs)
{
	return rhs *= c;
}

template<std::size_t width>
constexpr wuint<width> &wuint<width>::operator/=(std::uint32_t c)
{
	std::uint64_t w = 0;

	for (std::size_t i = width; i-- != 0; ) {
		w = (w << 32) + cells[i];
		cells[i] = std::uint32_t(w / std::uint64_t(c));
		w %= std::uint64_t(c);
	}

	return *this;
}

template<std::size_t width>
constexpr wuint<width> operator/(wuint<width> lhs, std::uint32_t c)
{
	return lhs /= c;
}

template<std::size_t width>
constexpr wuint<width> &wuint<width>::operator%=(std::uint32_t c)
{
	std::uint64_t w = 0;

	for (std::size_t i = width; i-- != 0; ) {
		w = (w << 32) + cells[i];
		w %= std::uint64_t(c);
	}

	return *this = std::uint32_t(w);
}

template<std::size_t width>
constexpr std::uint32_t operator%(const wuint<width> &lhs, std::uint32_t c)
{
	std::uint64_t w = 0;

	for (std::size_t i = width; i-- != 0; ) {
		w = (w << 32) + lhs.cells[i];
		w %= std::uint64_t(c);
	}

	return static_cast<std::uint32_t>(w);
}

template<std::size_t width>
constexpr std::uint32_t operator&(const wuint<width> &lhs, std::uint32_t c)
{
	return lhs.cells[0] & c;
}

template<std::size_t width>
constexpr wuint<width> abs(const wuint<width> &obj)
{
	return obj.is_negative() ? -obj : obj;
}

template<std::size_t width>
constexpr wuint<width> idiv(wuint<width> lhs, wuint<width> rhs)
{
	bool negative = false;

	if (lhs.is_negative()) {
		lhs = -lhs;
		negative = true;
	}

	if (rhs.is_negative()) {
		rhs = -rhs;
		negative = !negative;
	}

	return negative ? -(lhs / rhs) : lhs / rhs;
}

template<std::size_t width>
constexpr wuint<width> imod(wuint<width> lhs, wuint<width> rhs)
{
	bool lhs_negative = false;

	if (lhs.is_negative()) {
		lhs = -lhs;
		lhs_negative = true;
	}

	if (rhs.is_negative()) {
		rhs = -rhs;
	}

	return lhs_negative ? -(lhs % rhs) : lhs % rhs;
}

} // namespace wideint
