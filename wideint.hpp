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
#include <charconv>
#include <compare>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <limits>
#include <string>
#include <string_view>

#if !defined(WIDEINT_NO_IOSTREAMS)
#  include <iostream>
#endif

namespace wideint {

template<std::size_t width>
struct wint;

template<std::size_t width>
struct wuint;

namespace detail {

// Negate c without possibly undefined behavior on minimum value
constexpr std::int32_t safe_negate(std::int32_t c)
{
	return std::bit_cast<std::int32_t>(
		static_cast<std::uint32_t>(
			0U - static_cast<std::uint32_t>(c)
		)
	);
}

constexpr std::int32_t safe_abs(std::int32_t c)
{
	std::int32_t neg_c = safe_negate(c);
	return neg_c < 0 ? c : neg_c;
}

inline constexpr auto to_char_table = [] {
	std::array<char, 36> res = {};

	for (std::size_t i = 0; i != 10; ++i) {
		res[i] = static_cast<char>('0' + i);
	}

	for (std::size_t i = 10; i != 36; ++i) {
		res[i] = static_cast<char>('a' + (i - 10));
	}

	return res;
}();

static_assert(to_char_table[10] == 'a');
static_assert(to_char_table[35] == 'z');

inline constexpr auto from_char_table = [] {
	std::array<unsigned char, 256> res = {};

	res.fill(255);

	for (std::size_t i = 0; i != 10; ++i) {
		res['0' + i] = static_cast<unsigned char>(i);
	}

	for (std::size_t i = 10; i != 36; ++i) {
		res['a' + (i - 10)] = static_cast<unsigned char>(i);
		res['A' + (i - 10)] = static_cast<unsigned char>(i);
	}

	return res;
}();

static_assert(from_char_table[static_cast<unsigned char>('0')] == 0);
static_assert(from_char_table[static_cast<unsigned char>('9')] == 9);
static_assert(from_char_table[static_cast<unsigned char>('a')] == 10);
static_assert(from_char_table[static_cast<unsigned char>('z')] == 35);
static_assert(from_char_table[static_cast<unsigned char>('A')] == 10);
static_assert(from_char_table[static_cast<unsigned char>('Z')] == 35);

} // namespace detail

template<std::size_t width>
struct wuint {
	static constexpr wuint<width> min() {
		return wuint<width>(0);
	}

	static constexpr wuint<width> max() {
		wuint<width> res(0);
		res.cells.fill(std::numeric_limits<std::uint32_t>::max());
		return res;
	}

	constexpr wuint() = default;

	constexpr explicit wuint(std::uint32_t c) : cells{c} {}

	constexpr explicit wuint(std::string_view sv);

	constexpr explicit wuint(const wint<width> &other) : cells{other.cells} {}

	template<std::size_t other_width>
	constexpr explicit wuint(const wuint<other_width> &other) {
		if constexpr (width > other_width) {
			for (std::size_t i = 0; i != other_width; ++i) {
				cells[i] = other.cells[i];
			}

			for (std::size_t i = other_width; i != width; ++i) {
				cells[i] = 0;
			}
		}
		else {
			for (std::size_t i = 0; i != width; ++i) {
				cells[i] = other.cells[i];
			}
		}
	}

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

	constexpr wuint<width> &operator<<=(std::size_t shift);
	constexpr wuint<width> &operator>>=(std::size_t shift);

	constexpr wuint<width> &operator+=(std::uint32_t c);
	constexpr wuint<width> &operator-=(std::uint32_t c);
	constexpr wuint<width> &operator*=(std::uint32_t c);
	constexpr wuint<width> &operator/=(std::uint32_t c);
	constexpr wuint<width> &operator%=(std::uint32_t c);

	constexpr wuint<width> &operator&=(std::uint32_t c);
	constexpr wuint<width> &operator|=(std::uint32_t c);
	constexpr wuint<width> &operator^=(std::uint32_t c);

	[[nodiscard]] constexpr bool is_zero() const {
		for (std::size_t i = 0; i != width; ++i) {
			if (cells[i]) {
				return false;
			}
		}

		return true;
	}

	constexpr explicit operator bool() const { return !is_zero(); }

	[[nodiscard]] constexpr bool is_negative() const { return false; }

	[[nodiscard]] constexpr std::uint32_t getbit(std::size_t bit) const {
		std::size_t pos = bit / 32;
		std::size_t offs = bit % 32;

		return (cells[pos] >> offs) & 1U;
	}

	constexpr wuint<width> &setbit(std::size_t bit) {
		std::size_t pos = bit / 32;
		std::size_t offs = bit % 32;

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

	std::array<std::uint32_t, width> cells = {};
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
	auto lhs_bit_size = bit_width(*this);
	auto rhs_bit_size = bit_width(rhs);

	if (lhs_bit_size < rhs_bit_size) {
		cells.fill(0);
		return *this;
	}

	if (lhs_bit_size == rhs_bit_size) {
		if (*this >= rhs) {
			cells.fill(0);
			cells.front() = 1;
		}
		else {
			cells.fill(0);
		}

		return *this;
	}

	if (lhs_bit_size <= 32) {
		*this = cells.front() / rhs.cells.front();

		return *this;
	}

	if (rhs_bit_size <= 32) {
		if (std::has_single_bit(rhs.cells.front())) {
			*this >>= static_cast<std::size_t>(std::countr_zero(rhs.cells.front()));
		}
		else {
			*this /= rhs.cells.front();
		}

		return *this;
	}

	std::size_t adjust = lhs_bit_size - rhs_bit_size;

	if (adjust < 4) {
		wuint<width> quot(0);
		wuint<width> rem(*this);
		wuint<width> rhs_adjusted = rhs << adjust;

		for (std::size_t bit_i = adjust + 1; bit_i--; ) {
			auto cmp = rem <=> rhs_adjusted;

			if (cmp >= 0) {
				quot.setbit(bit_i);

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

	std::size_t n = (lhs_bit_size - 1) / 32;
	std::size_t t = (rhs_bit_size - 1) / 32;

	std::size_t shift = static_cast<std::size_t>(std::countl_zero(rhs.cells[t]));

	// Normalize
	auto x = wuint<width + 1>(*this) << shift;
	auto y = rhs << shift;

	wuint<width> quot(0);

	for (std::size_t i = n + 1; i != t; --i) {
		const std::uint64_t x_head = (static_cast<std::uint64_t>(x.cells[i]) << 32) + x.cells[i - 1];

		std::uint32_t q_hat = 0;

		// Calculate trial quotient q_hat
		if (x.cells[i] == y.cells[t]) {
			q_hat = std::numeric_limits<std::uint32_t>::max();
		}
		else {
			q_hat = static_cast<std::uint32_t>(x_head / y.cells[t]);
		}

		std::uint64_t diff = x_head - static_cast<std::uint64_t>(q_hat) * y.cells[t];

		// Adjust q_hat if too large
		while ((diff >> 32) == 0
		    && (diff << 32) + x.cells[i - 2] < static_cast<std::uint64_t>(q_hat) * y.cells[t - 1]) {
			--q_hat;
			diff += y.cells[t];
		}

		std::uint32_t borrow = 0;

		// Multiply and subtract
		{
			for (std::size_t j = 0; j <= t; ++j) {
				std::uint64_t prod = static_cast<std::uint64_t>(q_hat) * y.cells[j];
				std::uint64_t w = static_cast<std::uint64_t>(x.cells[i - t - 1 + j]) - static_cast<std::uint32_t>(prod) - borrow;
				x.cells[i - t - 1 + j] = static_cast<std::uint32_t>(w);
				borrow = static_cast<std::uint32_t>(prod >> 32) - static_cast<std::uint32_t>(w >> 32);
			}

			std::uint64_t w = static_cast<std::uint64_t>(x.cells[i]) - borrow;
			x.cells[i] = static_cast<std::uint32_t>(w);
			borrow = static_cast<std::uint32_t>(w >> 32);
		}

		// Add back if negative
		if (borrow) {
			std::uint32_t carry = 0;

			for (std::size_t j = 0; j <= t; ++j) {
				std::uint64_t w = static_cast<std::uint64_t>(x.cells[i - t - 1 + j]) + y.cells[j] + carry;
				x.cells[i - t - 1 + j] = static_cast<std::uint32_t>(w);
				carry = static_cast<std::uint32_t>(w >> 32);
			}

			std::uint64_t w = static_cast<std::uint64_t>(x.cells[i]) + carry;
			x.cells[i] = static_cast<std::uint32_t>(w);

			--q_hat;
		}

		quot.cells[i - t - 1] = q_hat;
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
	auto lhs_bit_size = bit_width(*this);
	auto rhs_bit_size = bit_width(rhs);

	if (lhs_bit_size < rhs_bit_size) {
		return *this;
	}

	if (lhs_bit_size == rhs_bit_size) {
		if (*this >= rhs) {
			*this -= rhs;
		}

		return *this;
	}

	if (lhs_bit_size <= 32) {
		*this = cells.front() % rhs.cells.front();

		return *this;
	}

	if (rhs_bit_size <= 32) {
		*this %= rhs.cells.front();

		return *this;
	}

	std::size_t adjust = lhs_bit_size - rhs_bit_size;

	if (adjust < 4) {
		wuint<width> rem(*this);
		wuint<width> rhs_adjusted = rhs << adjust;

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

	std::size_t n = (lhs_bit_size - 1) / 32;
	std::size_t t = (rhs_bit_size - 1) / 32;

	std::size_t shift = static_cast<std::size_t>(std::countl_zero(rhs.cells[t]));

	// Normalize
	auto x = wuint<width + 1>(*this) << shift;
	auto y = rhs << shift;

	for (std::size_t i = n + 1; i != t; --i) {
		const std::uint64_t x_head = (static_cast<std::uint64_t>(x.cells[i]) << 32) + x.cells[i - 1];

		std::uint32_t q_hat = 0;

		// Calculate trial quotient q_hat
		if (x.cells[i] == y.cells[t]) {
			q_hat = std::numeric_limits<std::uint32_t>::max();
		}
		else {
			q_hat = static_cast<std::uint32_t>(x_head / y.cells[t]);
		}

		std::uint64_t diff = x_head - static_cast<std::uint64_t>(q_hat) * y.cells[t];

		// Adjust q_hat if too large
		while ((diff >> 32) == 0
		    && (diff << 32) + x.cells[i - 2] < static_cast<std::uint64_t>(q_hat) * y.cells[t - 1]) {
			--q_hat;
			diff += y.cells[t];
		}

		std::uint32_t borrow = 0;

		// Multiply and subtract
		{
			for (std::size_t j = 0; j <= t; ++j) {
				std::uint64_t prod = static_cast<std::uint64_t>(q_hat) * y.cells[j];
				std::uint64_t w = static_cast<std::uint64_t>(x.cells[i - t - 1 + j]) - static_cast<std::uint32_t>(prod) - borrow;
				x.cells[i - t - 1 + j] = static_cast<std::uint32_t>(w);
				borrow = static_cast<std::uint32_t>(prod >> 32) - static_cast<std::uint32_t>(w >> 32);
			}

			std::uint64_t w = static_cast<std::uint64_t>(x.cells[i]) - borrow;
			x.cells[i] = static_cast<std::uint32_t>(w);
			borrow = static_cast<std::uint32_t>(w >> 32);
		}

		// Add back if negative
		if (borrow) {
			std::uint32_t carry = 0;

			for (std::size_t j = 0; j <= t; ++j) {
				std::uint64_t w = static_cast<std::uint64_t>(x.cells[i - t - 1 + j]) + y.cells[j] + carry;
				x.cells[i - t - 1 + j] = static_cast<std::uint32_t>(w);
				carry = static_cast<std::uint32_t>(w >> 32);
			}

			std::uint64_t w = static_cast<std::uint64_t>(x.cells[i]) + carry;
			x.cells[i] = static_cast<std::uint32_t>(w);

			--q_hat;
		}
	}

	// Unnormalize
	*this = wuint<width>(x >> shift);

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
constexpr wuint<width> &wuint<width>::operator<<=(std::size_t shift)
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
constexpr wuint<width> operator<<(const wuint<width> &lhs, std::size_t shift)
{
	wuint<width> res(lhs);
	res <<= shift;
	return res;
}

template<std::size_t width>
constexpr wuint<width> &wuint<width>::operator>>=(std::size_t shift)
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
constexpr wuint<width> operator>>(const wuint<width> &lhs, std::size_t shift)
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
	return obj;
}

template<std::size_t width>
constexpr wuint<width> min(const wuint<width> &lhs, const wuint<width> &rhs)
{
	return rhs < lhs ? rhs : lhs;
}

template<std::size_t width>
constexpr wuint<width> max(const wuint<width> &lhs, const wuint<width> &rhs)
{
	return rhs < lhs ? lhs : rhs;
}

template<std::size_t width>
constexpr std::from_chars_result from_chars(const char *first, const char *last, wuint<width> &value, int base = 10)
{
	constexpr auto muleq_with_carry = [](wuint<width> &lhs, std::uint32_t c, std::uint32_t carry) -> std::uint32_t {
		for (std::size_t i = 0; i != width; ++i) {
			std::uint64_t w = static_cast<std::uint64_t>(lhs.cells[i]) * c + carry;
			lhs.cells[i] = static_cast<std::uint32_t>(w);
			carry = static_cast<std::uint32_t>(w >> 32);
		}

		return carry;
	};

	if (first == last
	 || detail::from_char_table[static_cast<unsigned char>(*first)] >= base) {
		return {first, std::errc::invalid_argument};
	}

	wuint<width> res(0);

	bool overflow = false;

	const std::uint32_t ubase = static_cast<std::uint32_t>(base);
	const std::uint32_t digits_base_limit = std::numeric_limits<std::uint32_t>::max() / ubase;

	std::uint32_t digits_base = 1;
	std::uint32_t digits = 0;

	auto cur = first;

	for (; cur != last; ++cur) {
		std::uint32_t next_digit = detail::from_char_table[static_cast<unsigned char>(*cur)];

		if (next_digit >= ubase) {
			break;
		}

		digits = digits * ubase + next_digit;
		digits_base *= ubase;

		if (digits_base >= digits_base_limit) {
			auto carry = muleq_with_carry(res, digits_base, digits);

			overflow |= !!carry;

			digits = 0;
			digits_base = 1;
		}
	}

	if (digits_base != 1) {
		auto carry = muleq_with_carry(res, digits_base, digits);

		overflow |= !!carry;
	}

	if (overflow) {
		return {cur, std::errc::result_out_of_range};
	}

	value = res;

	return {cur, std::errc()};
}

template<std::size_t width>
constexpr std::to_chars_result to_chars(char *first, char *last, const wuint<width> &value, int base = 10)
{
	constexpr auto diveq_r = [](wuint<width> &lhs, std::uint32_t c) -> std::uint32_t {
		std::uint64_t w = 0;

		for (std::size_t i = width; i--; ) {
			w = (w << 32) + lhs.cells[i];
			lhs.cells[i] = static_cast<std::uint32_t>(w / static_cast<std::uint64_t>(c));
			w %= static_cast<std::uint64_t>(c);
		}

		return static_cast<std::uint32_t>(w);
	};

	if (first == last) {
		return {last, std::errc::value_too_large};
	}

	auto cur = first;

	if (value == 0) {
		*cur++ = '0';
		return {cur, std::errc()};
	}

	if (base == 10) {
		wuint<width> tmp(value);

		while (tmp > std::numeric_limits<std::uint32_t>::max()) {
			if (last - cur < 9) {
				return {last, std::errc::value_too_large};
			}

			std::uint32_t digits = diveq_r(tmp, 1'000'000'000UL);

			for (std::size_t i = 0; i != 9; ++i) {
				*cur++ = static_cast<char>('0' + (digits % 10));
				digits /= 10;
			}
		}

		for (std::uint32_t c = tmp.cells.front(); c != 0; c /= 10) {
			if (cur == last) {
				return {last, std::errc::value_too_large};
			}

			*cur++ = static_cast<char>('0' + (c % 10));
		}
	}
	else if (base == 16) {
		std::size_t last_cell = 0;

		for (std::size_t i = width; i--; ) {
			if (value.cells[i]) {
				last_cell = i;
				break;
			}
		}

		for (std::size_t i = 0; i != last_cell; ++i) {
			if (last - cur < 8) {
				return {last, std::errc::value_too_large};
			}

			std::uint32_t digits = value.cells[i];

			for (std::size_t i = 0; i < 8; ++i) {
				*cur++ = detail::to_char_table[digits % 16];
				digits /= 16;
			}
		}

		for (std::uint32_t digits = value.cells[last_cell]; digits != 0; digits /= 16) {
			if (cur == last) {
				return {last, std::errc::value_too_large};
			}

			*cur++ = detail::to_char_table[digits % 16];
		}
	}
	else {
		const std::uint32_t ubase = static_cast<std::uint32_t>(base);
		const std::uint32_t digits_base_limit = std::numeric_limits<std::uint32_t>::max() / ubase;

		std::uint32_t digits_base = 1;
		std::uint32_t num_digits = 0;

		while (digits_base < digits_base_limit) {
			digits_base *= ubase;
			++num_digits;
		}

		wuint<width> tmp(value);

		while (tmp > std::numeric_limits<std::uint32_t>::max()) {
			if (last - cur < num_digits) {
				return {last, std::errc::value_too_large};
			}

			std::uint32_t digits = diveq_r(tmp, digits_base);

			for (std::size_t i = 0; i != num_digits; ++i) {
				*cur++ = detail::to_char_table[digits % ubase];
				digits /= ubase;
			}
		}

		for (std::uint32_t c = tmp.cells.front(); c != 0; c /= ubase) {
			if (cur == last) {
				return {last, std::errc::value_too_large};
			}

			*cur++ = detail::to_char_table[c % ubase];
		}
	}

	std::reverse(first, cur);

	return {cur, std::errc()};
}

template<std::size_t width>
std::string to_string(const wuint<width> &obj)
{
	std::array<char, width * 11> buffer;

	auto [ptr, ec] = to_chars(buffer.data(), buffer.data() + buffer.size(), obj, 10);

	return std::string(buffer.data(), ptr);
}

template<std::size_t width>
constexpr wuint<width>::wuint(std::string_view sv)
{
	constexpr auto muleq_add = [](wuint<width> &lhs, std::uint32_t c, std::uint32_t carry) {
		for (std::size_t i = 0; i != width; ++i) {
			std::uint64_t w = static_cast<std::uint64_t>(lhs.cells[i]) * c + carry;
			lhs.cells[i] = static_cast<std::uint32_t>(w);
			carry = static_cast<std::uint32_t>(w >> 32);
		}
	};

	wuint<width> res(0);

	bool negative = false;

	if (sv.starts_with('-')) {
		negative = true;
		sv.remove_prefix(1);
	}

	std::uint32_t base = 10;

	if (sv.starts_with("0x") || sv.starts_with("0X"))
	{
		sv.remove_prefix(2);

		base = 16;
	}

	const std::uint32_t digits_base_limit = std::numeric_limits<std::uint32_t>::max() / base;

	std::uint32_t digits_base = 1;
	std::uint32_t digits = 0;

	for (auto ch : sv) {
		std::uint32_t next_digit = detail::from_char_table[static_cast<unsigned char>(ch)];

		if (next_digit >= base) {
			break;
		}

		digits = digits * base + next_digit;
		digits_base *= base;

		if (digits_base >= digits_base_limit) {
			muleq_add(res, digits_base, digits);

			digits = 0;
			digits_base = 1;
		}
	}

	if (digits_base != 1) {
		muleq_add(res, digits_base, digits);
	}

	*this = negative ? -res : res;
}

#if !defined(WIDEINT_NO_IOSTREAMS)
template<std::size_t width>
std::ostream &operator<<(std::ostream &os, const wuint<width> &obj)
{
	std::array<char, width * 12> buffer;

	std::ios_base::fmtflags ff = os.flags();

	int base = (ff & std::ios_base::hex) ? 16
	         : (ff & std::ios_base::oct) ? 8
	         : 10;

	auto [ptr, ec] = to_chars(buffer.data(), buffer.data() + buffer.size(), obj, base);

	return os << std::string_view(buffer.data(), ptr);
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

	obj = wuint<width>(s);

	return is;
}
#endif // !defined(WIDEINT_NO_IOSTREAMS)

template<std::size_t width>
constexpr bool has_single_bit(const wuint<width> &x)
{
	return popcount(x) == 1;
}

template<std::size_t width>
constexpr wuint<width> bit_ceil(const wuint<width> &x)
{
	wuint<width> res(0);

	if (x <= 1) {
		res.cells.front() = 1;
		return res;
	}

	res.setbit(bit_width(x - 1));

	return res;
}

template<std::size_t width>
constexpr wuint<width> bit_floor(const wuint<width> &x)
{
	wuint<width> res(0);

	std::size_t n = bit_width(x);

	if (n != 0) {
		res.setbit(n - 1);
	}

	return res;
}

template<std::size_t width>
constexpr std::size_t bit_width(const wuint<width> &x)
{
	for (std::size_t i = width; i--; ) {
		if (x.cells[i]) {
			return std::bit_width(x.cells[i]) + 32 * i;
		}
	}

	return 0;
}

template<std::size_t width>
constexpr std::size_t countl_zero(const wuint<width> &x)
{
	for (std::size_t i = width; i--; ) {
		if (x.cells[i]) {
			return static_cast<std::size_t>(std::countl_zero(x.cells[i]))
			     + 32 * (width - i - 1);
		}
	}

	return 32 * width;
}

template<std::size_t width>
constexpr std::size_t countl_one(const wuint<width> &x)
{
	for (std::size_t i = width; i--; ) {
		if (x.cells[i] != static_cast<std::uint32_t>(-1)) {
			return static_cast<std::size_t>(std::countl_one(x.cells[i]))
			     + 32 * (width - i - 1);
		}
	}

	return 32 * width;
}

template<std::size_t width>
constexpr std::size_t countr_zero(const wuint<width> &x)
{
	for (std::size_t i = 0; i != width; ++i) {
		if (x.cells[i]) {
			return static_cast<std::size_t>(std::countr_zero(x.cells[i]))
			     + 32 * i;
		}
	}

	return 32 * width;
}

template<std::size_t width>
constexpr std::size_t countr_one(const wuint<width> &x)
{
	for (std::size_t i = 0; i != width; ++i) {
		if (x.cells[i] != static_cast<std::uint32_t>(-1)) {
			return static_cast<std::size_t>(std::countr_one(x.cells[i]))
			     + 32 * i;
		}
	}

	return 32 * width;
}

template<std::size_t width>
constexpr std::size_t popcount(const wuint<width> &x)
{
	std::size_t count = 0;

	for (const auto &cell : x.cells) {
		count += static_cast<std::size_t>(std::popcount(cell));
	}

	return count;
}

// Binary GCD algorithm adapted from Wikipedia
template<std::size_t width>
constexpr wuint<width> gcd(const wuint<width> &x, const wuint<width> &y)
{
	if (x == 0) {
		return y;
	}
	if (y == 0) {
		return x;
	}

	auto a = x;
	auto b = y;

	std::size_t atz = countr_zero(a);
	std::size_t btz = countr_zero(b);

	a >>= atz;
	b >>= btz;

	std::size_t k = std::min(atz, btz);

	for (;;) {
		auto cmp = a <=> b;

		if (cmp < 0) {
			b -= a;
			b >>= countr_zero(b);
		}
		else {
			if (cmp == 0) {
				break;
			}

			a -= b;
			a >>= countr_zero(a);
		}
	}

	return b << k;
}

template<std::size_t width>
constexpr wuint<width> lcm(const wuint<width> &x, const wuint<width> &y)
{
	auto cmp = x <=> y;

	if (cmp == 0) {
		return x;
	}

	return cmp < 0 ? (x / gcd(x, y)) * y
	               : (y / gcd(x, y)) * x;
}

// Heron's method based on Wikipedia implementation
template<std::size_t width>
constexpr wuint<width> sqrt(const wuint<width> &x)
{
	std::size_t bit_size = bit_width(x);

	if (bit_size < 2) {
		return x;
	}

	auto r = wuint<width>(0).setbit((bit_size + 1) / 2);

	for (;;) {
		auto new_r = (r + x / r) >> 1;

		if (new_r >= r) {
			break;
		}

		r = new_r;
	}

	return r;
}

template<std::size_t width>
struct wint {
	static constexpr wint<width> min() {
		wint<width> res(0);
		res.cells.back() = static_cast<std::uint32_t>(std::numeric_limits<std::int32_t>::min());
		return res;
	}

	static constexpr wint<width> max() {
		wint<width> res(-1);
		res.cells.back() = static_cast<std::uint32_t>(std::numeric_limits<std::int32_t>::max());
		return res;
	}

	constexpr wint() = default;

	constexpr explicit wint(std::int32_t c) {
		if (c < 0) {
			cells.fill(static_cast<std::uint32_t>(-1));
		}
		cells[0] = static_cast<std::uint32_t>(c);
	}

	constexpr explicit wint(std::string_view sv) {
		*this = wint<width>(wuint<width>(sv));
	}

	constexpr explicit wint(const wuint<width> &other) : cells{other.cells} {}

	template<std::size_t other_width>
	constexpr explicit wint(const wint<other_width> &other) {
		if constexpr (width > other_width) {
			std::uint32_t fill = static_cast<std::uint32_t>(other.is_negative() ? -1 : 0);

			for (std::size_t i = 0; i != other_width; ++i) {
				cells[i] = other.cells[i];
			}

			for (std::size_t i = other_width; i != width; ++i) {
				cells[i] = fill;
			}
		}
		else {
			for (std::size_t i = 0; i != width; ++i) {
				cells[i] = other.cells[i];
			}
		}
	}

	constexpr wint<width> &operator=(std::int32_t c) {
		cells.fill(static_cast<std::uint32_t>(c < 0 ? -1 : 0));
		cells[0] = static_cast<std::uint32_t>(c);

		return *this;
	}

	constexpr wint<width> &operator+=(const wint<width> &rhs);
	constexpr wint<width> &operator-=(const wint<width> &rhs);
	constexpr wint<width> &operator*=(const wint<width> &rhs);
	constexpr wint<width> &operator/=(const wint<width> &rhs);
	constexpr wint<width> &operator%=(const wint<width> &rhs);

	constexpr wint<width> &operator&=(const wint<width> &rhs);
	constexpr wint<width> &operator|=(const wint<width> &rhs);
	constexpr wint<width> &operator^=(const wint<width> &rhs);

	constexpr wint<width> &operator<<=(std::size_t shift);
	constexpr wint<width> &operator>>=(std::size_t shift);

	constexpr wint<width> &operator+=(std::int32_t c);
	constexpr wint<width> &operator-=(std::int32_t c);
	constexpr wint<width> &operator*=(std::int32_t c);
	constexpr wint<width> &operator/=(std::int32_t c);
	constexpr wint<width> &operator%=(std::int32_t c);

	constexpr wint<width> &operator&=(std::uint32_t c);
	constexpr wint<width> &operator|=(std::uint32_t c);
	constexpr wint<width> &operator^=(std::uint32_t c);

	[[nodiscard]] constexpr bool is_zero() const {
		for (std::size_t i = 0; i != width; ++i) {
			if (cells[i]) {
				return false;
			}
		}

		return true;
	}

	constexpr explicit operator bool() const { return !is_zero(); }

	[[nodiscard]] constexpr bool is_negative() const {
		return cells.back() & (std::uint32_t(1) << 31);
	}

	[[nodiscard]] constexpr std::uint32_t getbit(std::size_t bit) const {
		std::size_t pos = bit / 32;
		std::size_t offs = bit % 32;

		return (cells[pos] >> offs) & 1U;
	}

	constexpr wint<width> &setbit(std::size_t bit) {
		std::size_t pos = bit / 32;
		std::size_t offs = bit % 32;

		cells[pos] |= std::uint32_t(1) << offs;

		return *this;
	}

	constexpr wint<width> &operator++() {
		std::uint32_t carry = 1;

		for (std::size_t i = 0; carry && i != width; ++i) {
			std::uint64_t w = static_cast<std::uint64_t>(cells[i]) + carry;
			cells[i] = static_cast<std::uint32_t>(w);
			carry = static_cast<std::uint32_t>(w >> 32);
		}

        	return *this;
	}

	constexpr wint<width> operator++(int) {
		wint<width> old = *this;
		operator++();
		return old;
	}

	constexpr wint<width> &operator--() {
		std::uint32_t borrow = 1;

		for (std::size_t i = 0; borrow && i != width; ++i) {
			std::uint64_t w = static_cast<std::uint64_t>(cells[i]) - borrow;
			cells[i] = static_cast<std::uint32_t>(w);
			borrow = static_cast<std::uint32_t>(w >> 32) ? 1 : 0;
		}

		return *this;
	}

	constexpr wint<width> operator--(int) {
		wint<width> old = *this;
		operator--();
		return old;
	}

	constexpr std::strong_ordering operator<=>(const wint<width> &rhs) const {
		bool negative = is_negative();

		if (negative == rhs.is_negative()) {
			for (std::size_t i = width; i--; ) {
				if (cells[i] != rhs.cells[i]) {
					return cells[i] <=> rhs.cells[i];
				}
			}

			return std::strong_ordering::equal;
		}

		return negative ? std::strong_ordering::less : std::strong_ordering::greater;
	}

	constexpr std::strong_ordering operator<=>(std::int32_t c) const
	{
		bool negative = is_negative();

		if (negative == (c < 0)) {
			std::uint32_t fill = static_cast<std::uint32_t>(c < 0 ? -1 : 0);

			for (std::size_t i = width - 1; i != 0; --i) {
				if (cells[i] != fill) {
					return cells[i] <=> fill;
				}
			}

			return cells[0] <=> static_cast<std::uint32_t>(c);
		}

		return negative ? std::strong_ordering::less : std::strong_ordering::greater;
	}

	constexpr bool operator==(const wint<width> &) const = default;

	constexpr bool operator==(std::int32_t c) const {
		if (cells.front() != static_cast<std::uint32_t>(c)) {
			return false;
		}

		std::uint32_t fill = static_cast<std::uint32_t>(c < 0 ? -1 : 0);

		for (std::size_t i = 1; i != width; ++i) {
			if (cells[i] != fill) {
				return false;
			}
		}

		return true;
	}

	std::array<std::uint32_t, width> cells = {};
};

template<std::size_t width>
constexpr wint<width> operator~(const wint<width> &obj)
{
	wint<width> res(obj);

	for (auto &cell : res.cells) {
		cell = ~cell;
	}

	return res;
}

template<std::size_t width>
constexpr wint<width> operator-(const wint<width> &obj)
{
	wint<width> res(obj);
	std::uint32_t carry = 1;

	for (std::size_t i = 0; i != width; ++i) {
		std::uint64_t w = static_cast<std::uint64_t>(static_cast<std::uint32_t>(~res.cells[i])) + carry;
		res.cells[i] = static_cast<std::uint32_t>(w);
		carry = static_cast<std::uint32_t>(w >> 32);
	}

       	return res;
}

template<std::size_t width>
constexpr wint<width> &wint<width>::operator+=(const wint<width> &rhs)
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
constexpr wint<width> operator+(const wint<width> &lhs, const wint<width> &rhs)
{
	wint<width> res(lhs);
	res += rhs;
	return res;
}

template<std::size_t width>
constexpr wint<width> &wint<width>::operator-=(const wint<width> &rhs)
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
constexpr wint<width> operator-(const wint<width> &lhs, const wint<width> &rhs)
{
	wint<width> res(lhs);
	res -= rhs;
	return res;
}

template<std::size_t width>
constexpr wint<width> &wint<width>::operator*=(const wint<width> &rhs)
{
	wint<width> res(0);

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
constexpr wint<width> operator*(const wint<width> &lhs, const wint<width> &rhs)
{
	wint<width> res(0);

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
constexpr wint<width> &wint<width>::operator/=(const wint<width> &rhs)
{
	wint<width> quot(wuint<width>(abs(*this)) / wuint<width>(abs(rhs)));

	*this = is_negative() != rhs.is_negative() ? -quot : quot;

	return *this;
}

template<std::size_t width>
constexpr wint<width> operator/(const wint<width> &lhs, const wint<width> &rhs)
{
	wint<width> res(lhs);
	res /= rhs;
	return res;
}

template<std::size_t width>
constexpr wint<width> &wint<width>::operator%=(const wint<width> &rhs)
{
	wint<width> rem(wuint<width>(abs(*this)) % wuint<width>(abs(rhs)));

	*this = is_negative() ? -rem : rem;

	return *this;
}

template<std::size_t width>
constexpr wint<width> operator%(const wint<width> &lhs, const wint<width> &rhs)
{
	wint<width> res(lhs);
	res %= rhs;
	return res;
}

template<std::size_t width>
constexpr wint<width> &wint<width>::operator&=(const wint<width> &rhs)
{
	for (std::size_t i = 0; i != width; ++i) {
		cells[i] &= rhs.cells[i];
	}

	return *this;
}

template<std::size_t width>
constexpr wint<width> operator&(const wint<width> &lhs, const wint<width> &rhs)
{
	wint<width> res(lhs);
	res &= rhs;
	return res;
}

template<std::size_t width>
constexpr wint<width> &wint<width>::operator|=(const wint<width> &rhs)
{
	for (std::size_t i = 0; i != width; ++i) {
		cells[i] |= rhs.cells[i];
	}

	return *this;
}

template<std::size_t width>
constexpr wint<width> operator|(const wint<width> &lhs, const wint<width> &rhs)
{
	wint<width> res(lhs);
	res |= rhs;
	return res;
}

template<std::size_t width>
constexpr wint<width> &wint<width>::operator^=(const wint<width> &rhs)
{
	for (std::size_t i = 0; i != width; ++i) {
		cells[i] ^= rhs.cells[i];
	}

	return *this;
}

template<std::size_t width>
constexpr wint<width> operator^(const wint<width> &lhs, const wint<width> &rhs)
{
	wint<width> res(lhs);
	res ^= rhs;
	return res;
}

template<std::size_t width>
constexpr wint<width> &wint<width>::operator<<=(std::size_t shift)
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
constexpr wint<width> operator<<(const wint<width> &lhs, std::size_t shift)
{
	wint<width> res(lhs);
	res <<= shift;
	return res;
}

template<std::size_t width>
constexpr wint<width> &wint<width>::operator>>=(std::size_t shift)
{
	std::size_t pos = shift / 32;
	std::size_t offs = shift % 32;

	std::uint32_t fill = static_cast<std::uint32_t>(is_negative() ? -1 : 0);

	std::uint64_t w = static_cast<std::uint64_t>(cells[pos]) << 32;
	cells[pos] = fill;

	for (std::size_t i = 0; i != width - pos - 1; ++i) {
		w = (w >> 32) + (static_cast<std::uint64_t>(cells[i + pos + 1]) << 32);
		cells[i + pos + 1] = fill;

		cells[i] = static_cast<std::uint32_t>(w >> offs);
	}

	cells[width - pos - 1] = static_cast<std::uint32_t>(
		((w >> 32) | (static_cast<std::uint64_t>(fill) << 32)) >> offs
	);

	for (std::size_t i = width - pos; i < pos; ++i) {
		cells[i] = fill;
	}

	return *this;
}

template<std::size_t width>
constexpr wint<width> operator>>(const wint<width> &lhs, std::size_t shift)
{
	wint<width> res(lhs);
	res >>= shift;
	return res;
}

template<std::size_t width>
constexpr wint<width> &wint<width>::operator+=(std::int32_t c)
{
	if (c < 0) {
		std::uint32_t borrow = static_cast<std::uint32_t>(detail::safe_negate(c));

		for (std::size_t i = 0; i != width; ++i) {
			std::uint64_t w = static_cast<std::uint64_t>(cells[i]) - borrow;
			cells[i] = static_cast<std::uint32_t>(w);
			borrow = static_cast<std::uint32_t>(w >> 32) ? 1 : 0;
		}

		return *this;
	}

	std::uint32_t carry = static_cast<std::uint32_t>(c);

	for (std::size_t i = 0; i != width; ++i) {
		std::uint64_t w = static_cast<std::uint64_t>(cells[i]) + carry;
		cells[i] = static_cast<std::uint32_t>(w);
		carry = static_cast<std::uint32_t>(w >> 32);
	}

	return *this;
}

template<std::size_t width>
constexpr wint<width> operator+(const wint<width> &lhs, std::int32_t c)
{
	wint<width> res(lhs);
	res += c;
	return res;
}

template<std::size_t width>
constexpr wint<width> operator+(std::int32_t c, const wint<width> &rhs)
{
	wint<width> res(rhs);
	res += c;
	return res;
}

template<std::size_t width>
constexpr wint<width> &wint<width>::operator-=(std::int32_t c)
{
	if (c < 0) {
		std::uint32_t carry = static_cast<std::uint32_t>(detail::safe_negate(c));

		for (std::size_t i = 0; i != width; ++i) {
			std::uint64_t w = static_cast<std::uint64_t>(cells[i]) + carry;
			cells[i] = static_cast<std::uint32_t>(w);
			carry = static_cast<std::uint32_t>(w >> 32);
		}

		return *this;
	}

	std::uint32_t borrow = static_cast<std::uint32_t>(c);

	for (std::size_t i = 0; i != width; ++i) {
		std::uint64_t w = static_cast<std::uint64_t>(cells[i]) - borrow;
		cells[i] = static_cast<std::uint32_t>(w);
		borrow = static_cast<std::uint32_t>(w >> 32) ? 1 : 0;
	}

	return *this;
}

template<std::size_t width>
constexpr wint<width> operator-(const wint<width> &lhs, std::int32_t c)
{
	wint<width> res(lhs);
	res -= c;
	return res;
}

template<std::size_t width>
constexpr wint<width> operator-(std::int32_t c, const wint<width> &rhs)
{
	wint<width> res(c);
	res -= rhs;
	return res;
}

template<std::size_t width>
constexpr wint<width> &wint<width>::operator*=(std::int32_t c)
{
	std::uint32_t abs_c = static_cast<std::uint32_t>(detail::safe_abs(c));

	std::uint32_t carry = 0;

	for (std::size_t i = 0; i != width; ++i) {
		std::uint64_t w = static_cast<std::uint64_t>(cells[i]) * abs_c + carry;
		cells[i] = static_cast<std::uint32_t>(w);
		carry = static_cast<std::uint32_t>(w >> 32);
	}

	if (c < 0) {
		*this = -*this;
	}

	return *this;
}

template<std::size_t width>
constexpr wint<width> operator*(const wint<width> &lhs, std::int32_t c)
{
	wint<width> res(lhs);
	res *= c;
	return res;
}

template<std::size_t width>
constexpr wint<width> operator*(std::int32_t c, const wint<width> &rhs)
{
	wint<width> res(rhs);
	res *= c;
	return res;
}

template<std::size_t width>
constexpr wint<width> &wint<width>::operator/=(std::int32_t c)
{
	std::uint32_t abs_c = static_cast<std::uint32_t>(detail::safe_abs(c));

	wint<width> quot(wuint<width>(abs(*this)) / abs_c);

	*this = is_negative() != (c < 0) ? -quot : quot;

	return *this;
}

template<std::size_t width>
constexpr wint<width> operator/(const wint<width> &lhs, std::int32_t c)
{
	wint<width> res(lhs);
	res /= c;
	return res;
}

template<std::size_t width>
constexpr wint<width> operator/(std::int32_t c, const wint<width> &rhs)
{
	std::uint32_t abs_c = static_cast<std::uint32_t>(detail::safe_abs(c));

	wint<width> quot(abs_c / wuint<width>(abs(rhs)));

	return (c < 0) != rhs.is_negative() ? -quot : quot;
}

template<std::size_t width>
constexpr wint<width> &wint<width>::operator%=(std::int32_t c)
{
	std::uint32_t abs_c = static_cast<std::uint32_t>(detail::safe_abs(c));

	wint<width> rem(wuint<width>(abs(*this)) % abs_c);

	*this = is_negative() ? -rem : rem;

	return *this;
}

template<std::size_t width>
constexpr std::int32_t operator%(const wint<width> &lhs, std::int32_t c)
{
	std::uint32_t abs_c = static_cast<std::uint32_t>(detail::safe_abs(c));

	std::int32_t rem = std::bit_cast<std::int32_t>(wuint<width>(abs(lhs)) % abs_c);

	return lhs.is_negative() ? detail::safe_negate(rem) : rem;
}

template<std::size_t width>
constexpr wint<width> operator%(std::int32_t c, const wint<width> &rhs)
{
	std::uint32_t abs_c = static_cast<std::uint32_t>(detail::safe_abs(c));

	wint<width> rem(abs_c % wuint<width>(abs(rhs)));

	return c < 0 ? -rem : rem;
}

template<std::size_t width>
constexpr wint<width> &wint<width>::operator&=(std::uint32_t c)
{
	*this = cells[0] & c;

	return *this;
}

template<std::size_t width>
constexpr std::uint32_t operator&(const wint<width> &lhs, std::uint32_t c)
{
	return lhs.cells[0] & c;
}

template<std::size_t width>
constexpr wint<width> operator&(std::uint32_t c, const wint<width> &rhs)
{
	wint<width> res(0);
	res.cells[0] = c & rhs.cells[0];
	return res;
}

template<std::size_t width>
constexpr wint<width> &wint<width>::operator|=(std::uint32_t c)
{
	cells[0] |= c;

	return *this;
}

template<std::size_t width>
constexpr wint<width> operator|(const wint<width> &lhs, std::uint32_t c)
{
	wint<width> res(lhs);
	res |= c;
	return res;
}

template<std::size_t width>
constexpr wint<width> operator|(std::uint32_t c, const wint<width> &rhs)
{
	wint<width> res(rhs);
	res |= c;
	return res;
}

template<std::size_t width>
constexpr wint<width> &wint<width>::operator^=(std::uint32_t c)
{
	cells[0] ^= c;

	return *this;
}

template<std::size_t width>
constexpr wint<width> operator^(const wint<width> &lhs, std::uint32_t c)
{
	wint<width> res(lhs);
	res ^= c;
	return res;
}

template<std::size_t width>
constexpr wint<width> operator^(std::uint32_t c, const wint<width> &rhs)
{
	wint<width> res(rhs);
	res ^= c;
	return res;
}

template<std::size_t width>
constexpr wint<width> abs(const wint<width> &obj)
{
	return obj.is_negative() ? -obj : obj;
}

template<std::size_t width>
constexpr wint<width> min(const wint<width> &lhs, const wint<width> &rhs)
{
	return rhs < lhs ? rhs : lhs;
}

template<std::size_t width>
constexpr wint<width> max(const wint<width> &lhs, const wint<width> &rhs)
{
	return rhs < lhs ? lhs : rhs;
}

template<std::size_t width>
constexpr std::from_chars_result from_chars(const char *first, const char *last, wint<width> &value, int base = 10)
{
	bool negative = false;

	auto cur = first;

	if (cur != last && *cur == '-') {
		negative = true;
		++cur;
	}

	wuint<width> ures(0);

	auto [ptr, ec] = from_chars(cur, last, ures, base);

	if (ec == std::errc::invalid_argument) {
		return {first, std::errc::invalid_argument};
	}

	wint<width> res(ures);

	if (ec == std::errc::result_out_of_range
	 || (res.is_negative() && !(negative && res == wint<width>::min()))) {
		return {ptr, std::errc::result_out_of_range};
	}

	value = negative ? -res : res;

	return {ptr, std::errc()};
}

template<std::size_t width>
constexpr std::to_chars_result to_chars(char *first, char *last, const wint<width> &value, int base = 10)
{
	if (first == last) {
		return {last, std::errc::value_too_large};
	}

	auto cur = first;

	if (value.is_negative()) {
		*cur++ = '-';
	}

	return to_chars(cur, last, wuint<width>(abs(value)), base);
}

template<std::size_t width>
std::string to_string(const wint<width> &obj)
{
	std::array<char, width * 11> buffer;

	auto [ptr, ec] = to_chars(buffer.data(), buffer.data() + buffer.size(), obj, 10);

	return std::string(buffer.data(), ptr);
}

#if !defined(WIDEINT_NO_IOSTREAMS)
template<std::size_t width>
std::ostream &operator<<(std::ostream &os, const wint<width> &obj)
{
	std::array<char, width * 12> buffer;

	std::ios_base::fmtflags ff = os.flags();

	int base = (ff & std::ios_base::hex) ? 16
	         : (ff & std::ios_base::oct) ? 8
	         : 10;

	auto [ptr, ec] = to_chars(buffer.data(), buffer.data() + buffer.size(), obj, base);

	return os << std::string_view(buffer.data(), ptr);
}

template<std::size_t width>
std::istream &operator>>(std::istream &is, wint<width> &obj)
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

	obj = wint<width>(wuint<width>(s));

	return is;
}
#endif // !defined(WIDEINT_NO_IOSTREAMS)

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

template<std::size_t width>
struct std::hash<wideint::wint<width>>
{
	std::size_t operator()(const wideint::wint<width> &obj) const noexcept
	{
		std::size_t hash = 17;
		for (const auto &cell : obj.cells) {
			hash = hash * 37 + std::hash<std::uint32_t>()(cell);
		}
		return hash;
	}
};
