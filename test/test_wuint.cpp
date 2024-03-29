//
// test_wuint - wideint unit test
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
// SPDX-License-Identifier: Apache-2.0
//

#include "wideint.hpp"

#include <cstdint>
#include <iostream>
#include <limits>
#include <sstream>
#include <unordered_set>
#include <vector>

#include "catch.hpp"

using wideint::wuint;
using wideint::wint;

using wuint32 = wuint<1>;
using wuint64 = wuint<2>;
using wuint96 = wuint<3>;
using wuint128 = wuint<4>;
using wuint256 = wuint<8>;

static constexpr auto wuint32_7F = wuint32::max() / wuint32(2);
static constexpr auto wuint64_7F = wuint64::max() / wuint64(2);
static constexpr auto wuint96_7F = wuint96::max() / wuint96(2);
static constexpr auto wuint32_80 = wuint32_7F + wuint32(1);
static constexpr auto wuint64_80 = wuint64_7F + wuint64(1);
static constexpr auto wuint96_80 = wuint96_7F + wuint96(1);
static constexpr auto wuint32_81 = wuint32_80 + wuint32(1);
static constexpr auto wuint64_81 = wuint64_80 + wuint64(1);
static constexpr auto wuint96_81 = wuint96_80 + wuint96(1);
static constexpr auto wuint32_FF = wuint32::max();
static constexpr auto wuint64_FF = wuint64::max();
static constexpr auto wuint96_FF = wuint96::max();

constexpr std::uint32_t uint32_80 = std::numeric_limits<std::int32_t>::min();
constexpr std::uint32_t uint32_81 = -std::numeric_limits<std::int32_t>::max();
constexpr std::uint32_t uint32_FF = -1;
constexpr std::uint32_t uint32_01 = 1;
constexpr std::uint32_t uint32_7F = std::numeric_limits<std::int32_t>::max();

constexpr wuint256 fac(std::uint32_t n)
{
	wuint256 res(n);

	for (std::uint32_t i = 2; i < n; ++i) {
		res *= i;
	}

	return res;
}

template<std::size_t width>
constexpr wuint<width> modinv(const wuint<width> &a, const wuint<width> &n)
{
	wint<width> new_r(a);
	wint<width> r(n);
	wint<width> new_t(1);
	wint<width> t(0);

	while (new_r != 0) {
		wint<width> quotient = r / new_r;

		t = std::exchange(new_t, t - quotient * new_t);
		r = std::exchange(new_r, r - quotient * new_r);
	}

	if (r > 1) {
		return wuint<width>(0);
	}

	if (t.is_negative()) {
		t += wint<width>(n);
	}

	return wuint<width>(t);
}

template<std::size_t width>
constexpr wuint<width> modexp(const wuint<width> &a, const wuint<width> &x, const wuint<width> &n)
{
	const wuint<width> base(a % n);
	wuint<width> res(1);

	for (std::size_t bit_i = bit_width(x); bit_i--; ) {
		res = (res * res) % n;

		if (x.getbit(bit_i)) {
			res = (res * base) % n;
		}
	}

	return res;
}

TEST_CASE("initialize wuint from string", "[wuint]") {
	constexpr wuint32 zero32("0");
	constexpr wuint64 zero64("0");
	constexpr wuint96 zero96("0");

	REQUIRE(zero32.cells[0] == 0);
	REQUIRE(zero64.cells[0] == 0);
	REQUIRE(zero64.cells[1] == 0);
	REQUIRE(zero96.cells[0] == 0);
	REQUIRE(zero96.cells[1] == 0);
	REQUIRE(zero96.cells[2] == 0);

	constexpr wuint32 one32("1");
	constexpr wuint64 one64("1");
	constexpr wuint96 one96("1");

	REQUIRE(one32.cells[0] == 1);
	REQUIRE(one64.cells[0] == 1);
	REQUIRE(one64.cells[1] == 0);
	REQUIRE(one96.cells[0] == 1);
	REQUIRE(one96.cells[1] == 0);
	REQUIRE(one96.cells[2] == 0);

	constexpr wuint32 n_one32("-1");
	constexpr wuint64 n_one64("-1");
	constexpr wuint96 n_one96("-1");

	REQUIRE(n_one32.cells[0] == 0xFFFFFFFF);
	REQUIRE(n_one64.cells[0] == 0xFFFFFFFF);
	REQUIRE(n_one64.cells[1] == 0xFFFFFFFF);
	REQUIRE(n_one96.cells[0] == 0xFFFFFFFF);
	REQUIRE(n_one96.cells[1] == 0xFFFFFFFF);
	REQUIRE(n_one96.cells[2] == 0xFFFFFFFF);

	constexpr wuint32 dec32("286335522");
	constexpr wuint64 dec64("3689367580026693222");
	constexpr wuint96 dec96("36973223102941133555797576908");

	REQUIRE(dec32.cells[0] == 0x11112222);
	REQUIRE(dec64.cells[0] == 0x55556666);
	REQUIRE(dec64.cells[1] == 0x33334444);
	REQUIRE(dec96.cells[0] == 0xBBBBCCCC);
	REQUIRE(dec96.cells[1] == 0x9999AAAA);
	REQUIRE(dec96.cells[2] == 0x77778888);

	constexpr wuint32 n_dec32("-286335522");
	constexpr wuint64 n_dec64("-3689367580026693222");
	constexpr wuint96 n_dec96("-36973223102941133555797576908");

	REQUIRE(n_dec32.cells[0] == 0xEEEEDDDE);
	REQUIRE(n_dec64.cells[0] == 0xAAAA999A);
	REQUIRE(n_dec64.cells[1] == 0xCCCCBBBB);
	REQUIRE(n_dec96.cells[0] == 0x44443334);
	REQUIRE(n_dec96.cells[1] == 0x66665555);
	REQUIRE(n_dec96.cells[2] == 0x88887777);

	constexpr wuint32 hex32("0x11112222");
	constexpr wuint64 hex64("0x3333444455556666");
	constexpr wuint96 hex96("0x777788889999AAAABBBBCCCC");

	REQUIRE(hex32.cells[0] == 0x11112222);
	REQUIRE(hex64.cells[0] == 0x55556666);
	REQUIRE(hex64.cells[1] == 0x33334444);
	REQUIRE(hex96.cells[0] == 0xBBBBCCCC);
	REQUIRE(hex96.cells[1] == 0x9999AAAA);
	REQUIRE(hex96.cells[2] == 0x77778888);

	constexpr wuint32 n_hex32("-0x11112222");
	constexpr wuint64 n_hex64("-0x3333444455556666");
	constexpr wuint96 n_hex96("-0x777788889999AAAABBBBCCCC");

	REQUIRE(n_hex32.cells[0] == 0xEEEEDDDE);
	REQUIRE(n_hex64.cells[0] == 0xAAAA999A);
	REQUIRE(n_hex64.cells[1] == 0xCCCCBBBB);
	REQUIRE(n_hex96.cells[0] == 0x44443334);
	REQUIRE(n_hex96.cells[1] == 0x66665555);
	REQUIRE(n_hex96.cells[2] == 0x88887777);
}

TEST_CASE("wuint uint32_t equality", "[wuint]") {
	REQUIRE(wuint32("0") == 0);
	REQUIRE(wuint32("1") == 1);
	REQUIRE(wuint32("-1") == -1);
	REQUIRE(wuint32("286335522") == 286335522);
	REQUIRE(wuint32("-286335522") == -286335522);

	REQUIRE(wuint32("0") != 1);
	REQUIRE(wuint32("1") != 0);
	REQUIRE(wuint32("-1") != 0);

	REQUIRE(wuint64("0") == 0);
	REQUIRE(wuint64("1") == 1);
	REQUIRE(wuint64("286335522") == 286335522);

	REQUIRE(wuint64("-1") != -1);
	REQUIRE(wuint64("-1").cells[0] == -1);
	REQUIRE(wuint64("-1").cells[1] == 0xFFFFFFFF);

	REQUIRE(wuint64("-286335522") != -286335522);
	REQUIRE(wuint64("-286335522").cells[0] == -286335522);
	REQUIRE(wuint64("-286335522").cells[1] == 0xFFFFFFFF);

	REQUIRE(wuint96("0") == 0);
	REQUIRE(wuint96("1") == 1);
	REQUIRE(wuint96("286335522") == 286335522);

	REQUIRE(wuint96("-1") != -1);
	REQUIRE(wuint96("-1").cells[0] == -1);
	REQUIRE(wuint96("-1").cells[1] == 0xFFFFFFFF);
	REQUIRE(wuint96("-1").cells[2] == 0xFFFFFFFF);

	REQUIRE(wuint96("-286335522") != -286335522);
	REQUIRE(wuint96("-286335522").cells[0] == -286335522);
	REQUIRE(wuint96("-286335522").cells[1] == 0xFFFFFFFF);
	REQUIRE(wuint96("-286335522").cells[2] == 0xFFFFFFFF);
}

TEST_CASE("wuint wuint less than", "[wuint]") {
	REQUIRE(wuint64::min() < wuint64("1"));
	REQUIRE(wuint64("1") < wuint64::max());
}

TEST_CASE("wuint uint32_t less than", "[wuint]") {
	REQUIRE(wuint64::min() < 1);
	REQUIRE(wuint64("1") < std::numeric_limits<std::uint32_t>::max());
	REQUIRE(std::numeric_limits<std::uint32_t>::max() < wuint64("0x100000000"));
}

TEST_CASE("assign from uint32_t", "[wuint]") {
	auto uint32 = GENERATE(as<std::uint32_t>{},
		0x00000001, 0x7FFFFFFF, 0x80000000, 0x80000001, 0xFFFFFFFF
	);

	wuint32 value_32("0x11112222");
	wuint64 value_64("0x3333444455556666");
	wuint96 value_96("0x777788889999AAAABBBBCCCC");

	SECTION("assign uint32") {
		value_32 = uint32;
		value_64 = uint32;
		value_96 = uint32;

		REQUIRE(value_32 == uint32);
		REQUIRE(value_64 == uint32);
		REQUIRE(value_96 == uint32);
	}
}

TEST_CASE("wuint bitwise complement", "[wuint]") {
	REQUIRE(~wuint32("0") == wuint32("0xFFFFFFFF"));
	REQUIRE(~wuint64("0") == wuint64("0xFFFFFFFFFFFFFFFF"));
	REQUIRE(~wuint96("0") == wuint96("0xFFFFFFFFFFFFFFFFFFFFFFFF"));

	REQUIRE(~wuint32("0xFFFFFFFF") == wuint32("0"));
	REQUIRE(~wuint64("0xFFFFFFFFFFFFFFFF") == wuint64("0"));
	REQUIRE(~wuint96("0xFFFFFFFFFFFFFFFFFFFFFFFF") == wuint96("0"));

	REQUIRE(~wuint32("0xF0F0F0F0") == wuint32("0x0F0F0F0F"));
	REQUIRE(~wuint64("0xF0F0F0F0F0F0F0F0") == wuint64("0x0F0F0F0F0F0F0F0F"));
	REQUIRE(~wuint96("0xF0F0F0F0F0F0F0F0F0F0F0F0") == wuint96("0x0F0F0F0F0F0F0F0F0F0F0F0F"));
}

TEST_CASE("wuint unary minus", "[wuint]") {
	REQUIRE(-wuint32("0") == wuint32("0"));
	REQUIRE(-wuint64("0") == wuint64("0"));
	REQUIRE(-wuint96("0") == wuint96("0"));

	REQUIRE(-wuint32("1") == wuint32("0xFFFFFFFF"));
	REQUIRE(-wuint64("1") == wuint64("0xFFFFFFFFFFFFFFFF"));
	REQUIRE(-wuint96("1") == wuint96("0xFFFFFFFFFFFFFFFFFFFFFFFF"));

	REQUIRE(-wuint32("0x01234567") == wuint32("-0x01234567"));
	REQUIRE(-wuint64("0x0123456712345678") == wuint64("-0x0123456712345678"));
	REQUIRE(-wuint96("0x012345671234567823456789") == wuint96("-0x012345671234567823456789"));

	REQUIRE(-wuint32("0x81234567") == wuint32("-0x81234567"));
	REQUIRE(-wuint64("0x8123456712345678") == wuint64("-0x8123456712345678"));
	REQUIRE(-wuint96("0x812345671234567823456789") == wuint96("-0x812345671234567823456789"));

	REQUIRE(-wuint32("0x7FFFFFFF") == wuint32("0x80000001"));
	REQUIRE(-wuint64("0x7FFFFFFFFFFFFFFF") == wuint64("0x8000000000000001"));
	REQUIRE(-wuint96("0x7FFFFFFFFFFFFFFFFFFFFFFF") == wuint96("0x800000000000000000000001"));

	REQUIRE(-wuint32("0x80000000") == wuint32("0x80000000"));
	REQUIRE(-wuint64("0x8000000000000000") == wuint64("0x8000000000000000"));
	REQUIRE(-wuint96("0x800000000000000000000000") == wuint96("0x800000000000000000000000"));
}

TEST_CASE("wuint increment/decrement 32", "[wuint]") {
	auto str = GENERATE(as<std::string>{},
		"0x00000001", "0x7FFFFFFF", "0x80000000", "0x80000001", "0xFFFFFFFF"
	);

	const auto org_var = wuint32(str);
	auto var = org_var;

	SECTION("Prefix increment") {
		auto retval = ++var;

		REQUIRE(retval == org_var + 1);
		REQUIRE(var == org_var + 1);
	}
	SECTION("Prefix decrement") {
		auto retval = --var;

		REQUIRE(retval == org_var - 1);
		REQUIRE(var == org_var - 1);
	}
	SECTION("Postfix increment") {
		auto retval = var++;

		REQUIRE(retval == org_var);
		REQUIRE(var == org_var + 1);
	}
	SECTION("Postfix decrement") {
		auto retval = var--;

		REQUIRE(retval == org_var);
		REQUIRE(var == org_var - 1);
	}
}

TEST_CASE("wuint increment/decrement 96", "[wuint]") {
	auto str = GENERATE(as<std::string>{},
		"0x000000000000000000000001",
		"0x7FFFFFFFFFFFFFFFFFFFFFFF",
		"0x800000000000000000000000",
		"0x800000000000000000000001",
		"0xFFFFFFFFFFFFFFFFFFFFFFFF"
	);

	const auto org_var = wuint96(str);
	auto var = org_var;

	SECTION("Prefix increment") {
		auto retval = ++var;

		REQUIRE(retval == org_var + 1);
		REQUIRE(var == org_var + 1);
	}
	SECTION("Prefix decrement") {
		auto retval = --var;

		REQUIRE(retval == org_var - 1);
		REQUIRE(var == org_var - 1);
	}
	SECTION("Postfix increment") {
		auto retval = var++;

		REQUIRE(retval == org_var);
		REQUIRE(var == org_var + 1);
	}
	SECTION("Postfix decrement") {
		auto retval = var--;

		REQUIRE(retval == org_var);
		REQUIRE(var == org_var - 1);
	}
}

TEST_CASE("wuint left shift", "[wuint]") {
	REQUIRE((wuint32("0x01234567") << 4) == wuint32("0x12345670"));
	REQUIRE((wuint64("0x0123456712345678") << 4) == wuint64("0x1234567123456780"));
	REQUIRE((wuint96("0x012345671234567823456789") << 4) == wuint96("0x123456712345678234567890"));

	REQUIRE((wuint64("0x0123456712345678") << 32) == wuint64("0x1234567800000000"));
	REQUIRE((wuint96("0x012345671234567823456789") << 32) == wuint96("0x123456782345678900000000"));
	REQUIRE((wuint64("0x0123456712345678") << 36) == wuint64("0x2345678000000000"));
	REQUIRE((wuint96("0x012345671234567823456789") << 36) == wuint96("0x234567823456789000000000"));

	REQUIRE((wuint96("0x012345671234567823456789") << 64) == wuint96("0x234567890000000000000000"));
	REQUIRE((wuint96("0x012345671234567823456789") << 68) == wuint96("0x345678900000000000000000"));

	REQUIRE((wuint256("0x0123456712345678234567893456789A456789AB56789ABC6789ABCD789ABCDE") << 224) == wuint256("0x789ABCDE00000000000000000000000000000000000000000000000000000000"));
	REQUIRE((wuint256("0x0123456712345678234567893456789A456789AB56789ABC6789ABCD789ABCDE") << 228) == wuint256("0x89ABCDE000000000000000000000000000000000000000000000000000000000"));
}

TEST_CASE("wuint right shift", "[wuint]") {
	REQUIRE((wuint32("0x01234567") >> 4) == wuint32("0x00123456"));
	REQUIRE((wuint64("0x0123456712345678") >> 4) == wuint64("0x0012345671234567"));
	REQUIRE((wuint96("0x012345671234567823456789") >> 4) == wuint96("0x001234567123456782345678"));

	REQUIRE((wuint64("0x0123456712345678") >> 32) == wuint64("0x01234567"));
	REQUIRE((wuint96("0x012345671234567823456789") >> 32) == wuint96("0x0123456712345678"));
	REQUIRE((wuint64("0x0123456712345678") >> 36) == wuint64("0x00123456"));
	REQUIRE((wuint96("0x012345671234567823456789") >> 36) == wuint96("0x0012345671234567"));

	REQUIRE((wuint96("0x012345671234567823456789") >> 64) == wuint96("0x01234567"));
	REQUIRE((wuint96("0x012345671234567823456789") >> 68) == wuint96("0x00123456"));

	REQUIRE((wuint256("0x0123456712345678234567893456789A456789AB56789ABC6789ABCD789ABCDE") >> 224) == wuint256("0x1234567"));
	REQUIRE((wuint256("0x0123456712345678234567893456789A456789AB56789ABC6789ABCD789ABCDE") >> 228) == wuint256("0x123456"));
}

TEST_CASE("wuint wuint plus", "[wuint]") {
	REQUIRE(wuint32("1000000000") + wuint32("1") == wuint32("1000000001"));
	REQUIRE(wuint64("1000000000000000000") + wuint64("1") == wuint64("1000000000000000001"));
	REQUIRE(wuint96("1000000000000000000000000000") + wuint96("1") == wuint96("1000000000000000000000000001"));

	REQUIRE(wuint32("1000000000") + wuint32("1000000000") == wuint32("2000000000"));
	REQUIRE(wuint64("1000000000000000000") + wuint64("1000000000000000000") == wuint64("2000000000000000000"));
	REQUIRE(wuint96("1000000000000000000000000000") + wuint96("1000000000000000000000000000") == wuint96("2000000000000000000000000000"));

	REQUIRE(wuint32("1") + wuint32("-1") == wuint32("0"));
	REQUIRE(wuint64("1") + wuint64("-1") == wuint64("0"));
	REQUIRE(wuint96("1") + wuint96("-1") == wuint96("0"));

	REQUIRE(wuint32("-1") + wuint32("-1") == wuint32("-2"));
	REQUIRE(wuint64("-1") + wuint64("-1") == wuint64("-2"));
	REQUIRE(wuint96("-1") + wuint96("-1") == wuint96("-2"));
}

TEST_CASE("wuint wuint minus", "[wuint]") {
	REQUIRE(wuint32("1000000001") - wuint32("1") == wuint32("1000000000"));
	REQUIRE(wuint64("1000000000000000001") - wuint64("1") == wuint64("1000000000000000000"));
	REQUIRE(wuint96("1000000000000000000000000001") - wuint96("1") == wuint96("1000000000000000000000000000"));

	REQUIRE(wuint32("2000000000") - wuint32("1000000000") == wuint32("1000000000"));
	REQUIRE(wuint64("2000000000000000000") - wuint64("1000000000000000000") == wuint64("1000000000000000000"));
	REQUIRE(wuint96("2000000000000000000000000000") - wuint96("1000000000000000000000000000") == wuint96("1000000000000000000000000000"));

	REQUIRE(wuint32("0") - wuint32("1") == wuint32("-1"));
	REQUIRE(wuint64("0") - wuint64("1") == wuint64("-1"));
	REQUIRE(wuint96("0") - wuint96("1") == wuint96("-1"));

	REQUIRE(wuint32("-1") - wuint32("-1") == wuint32("0"));
	REQUIRE(wuint64("-1") - wuint64("-1") == wuint64("0"));
	REQUIRE(wuint96("-1") - wuint96("-1") == wuint96("0"));
}

TEST_CASE("wuint wuint multiply", "[wuint]") {
	REQUIRE(wuint32("1000000000") * wuint32("1") == wuint32("1000000000"));
	REQUIRE(wuint64("1000000000000000000") * wuint64("1") == wuint64("1000000000000000000"));
	REQUIRE(wuint96("1000000000000000000000000000") * wuint96("1") == wuint96("1000000000000000000000000000"));

	REQUIRE(wuint32("1000000000") * wuint32("2") == wuint32("2000000000"));
	REQUIRE(wuint64("1000000000000000000") * wuint64("2") == wuint64("2000000000000000000"));
	REQUIRE(wuint96("1000000000000000000000000000") * wuint96("2") == wuint96("2000000000000000000000000000"));

	REQUIRE(wuint32("10000") * wuint32("10000") == wuint32("100000000"));
	REQUIRE(wuint64("1000000000") * wuint64("1000000000") == wuint64("1000000000000000000"));
	REQUIRE(wuint96("10000000000000") * wuint96("10000000000000") == wuint96("100000000000000000000000000"));

	REQUIRE(wuint32("-1") * wuint32("0") == wuint32("0"));
	REQUIRE(wuint64("-1") * wuint64("0") == wuint64("0"));
	REQUIRE(wuint96("-1") * wuint96("0") == wuint96("0"));

	REQUIRE(wuint32("-1") * wuint32("-1") == wuint32("1"));
	REQUIRE(wuint64("-1") * wuint64("-1") == wuint64("1"));
	REQUIRE(wuint96("-1") * wuint96("-1") == wuint96("1"));

	REQUIRE(wuint32("10000") * -wuint32("10000") == -wuint32("100000000"));
	REQUIRE(wuint64("1000000000") * -wuint64("1000000000") == -wuint64("1000000000000000000"));
	REQUIRE(wuint96("10000000000000") * -wuint96("10000000000000") == -wuint96("100000000000000000000000000"));
}

TEST_CASE("wuint wuint divide", "[wuint]") {
	REQUIRE(wuint32("1000000000") / wuint32("1") == wuint32("1000000000"));
	REQUIRE(wuint64("1000000000000000000") / wuint64("1") == wuint64("1000000000000000000"));
	REQUIRE(wuint96("1000000000000000000000000000") / wuint96("1") == wuint96("1000000000000000000000000000"));

	REQUIRE(wuint32("1000000000") / wuint32("2") == wuint32("500000000"));
	REQUIRE(wuint64("1000000000000000000") / wuint64("2") == wuint64("500000000000000000"));
	REQUIRE(wuint96("1000000000000000000000000000") / wuint96("2") == wuint96("500000000000000000000000000"));

	REQUIRE(wuint32("9999") / wuint32("10000") == wuint32("0"));
	REQUIRE(wuint64("999999999") / wuint64("1000000000") == wuint64("0"));
	REQUIRE(wuint96("9999999999999") / wuint96("10000000000000") == wuint96("0"));

	REQUIRE(wuint32("10000") / wuint32("10000") == wuint32("1"));
	REQUIRE(wuint64("1000000000") / wuint64("1000000000") == wuint64("1"));
	REQUIRE(wuint96("10000000000000") / wuint96("10000000000000") == wuint96("1"));

	REQUIRE(wuint32("10001") / wuint32("10000") == wuint32("1"));
	REQUIRE(wuint64("1000000001") / wuint64("1000000000") == wuint64("1"));
	REQUIRE(wuint96("10000000000001") / wuint96("10000000000000") == wuint96("1"));

	REQUIRE(wuint32("19999") / wuint32("10000") == wuint32("1"));
	REQUIRE(wuint64("1999999999") / wuint64("1000000000") == wuint64("1"));
	REQUIRE(wuint96("19999999999999") / wuint96("10000000000000") == wuint96("1"));

	REQUIRE(wuint32("20000") / wuint32("10000") == wuint32("2"));
	REQUIRE(wuint64("2000000000") / wuint64("1000000000") == wuint64("2"));
	REQUIRE(wuint96("20000000000000") / wuint96("10000000000000") == wuint96("2"));

	REQUIRE(wuint32("10000") / wuint32("100000000") == wuint32("0"));
	REQUIRE(wuint64("10000") / wuint64("10000000000000000") == wuint64("0"));
	REQUIRE(wuint96("10000") / wuint96("1000000000000000000000000") == wuint96("0"));
}

TEST_CASE("wuint wuint divide limits", "[wuint]") {
	// 7F / 7F
	REQUIRE(wuint32_7F / wuint32_7F == wuint32("1"));
	REQUIRE(wuint64_7F / wuint64_7F == wuint64("1"));
	REQUIRE(wuint96_7F / wuint96_7F == wuint96("1"));

	// 7F / 80
	REQUIRE(wuint32_7F / wuint32_80 == wuint32("0"));
	REQUIRE(wuint64_7F / wuint64_80 == wuint64("0"));
	REQUIRE(wuint96_7F / wuint96_80 == wuint96("0"));

	// 7F / 81
	REQUIRE(wuint32_7F / wuint32_81 == wuint32("0"));
	REQUIRE(wuint64_7F / wuint64_81 == wuint64("0"));
	REQUIRE(wuint96_7F / wuint96_81 == wuint96("0"));

	// 7F / FF
	REQUIRE(wuint32_7F / wuint32_FF == wuint32("0"));
	REQUIRE(wuint64_7F / wuint64_FF == wuint64("0"));
	REQUIRE(wuint96_7F / wuint96_FF == wuint96("0"));

	// 80 / 7F
	REQUIRE(wuint32_80 / wuint32_7F == wuint32("1"));
	REQUIRE(wuint64_80 / wuint64_7F == wuint64("1"));
	REQUIRE(wuint96_80 / wuint96_7F == wuint96("1"));

	// 80 / 80
	REQUIRE(wuint32_80 / wuint32_80 == wuint32("1"));
	REQUIRE(wuint64_80 / wuint64_80 == wuint64("1"));
	REQUIRE(wuint96_80 / wuint96_80 == wuint96("1"));

	// 80 / 81
	REQUIRE(wuint32_80 / wuint32_81 == wuint32("0"));
	REQUIRE(wuint64_80 / wuint64_81 == wuint64("0"));
	REQUIRE(wuint96_80 / wuint96_81 == wuint96("0"));

	// 80 / FF
	REQUIRE(wuint32_80 / wuint32_FF == wuint32("0"));
	REQUIRE(wuint64_80 / wuint64_FF == wuint64("0"));
	REQUIRE(wuint96_80 / wuint96_FF == wuint96("0"));

	// 81 / 7F
	REQUIRE(wuint32_81 / wuint32_7F == wuint32("1"));
	REQUIRE(wuint64_81 / wuint64_7F == wuint64("1"));
	REQUIRE(wuint96_81 / wuint96_7F == wuint96("1"));

	// 81 / 80
	REQUIRE(wuint32_81 / wuint32_80 == wuint32("1"));
	REQUIRE(wuint64_81 / wuint64_80 == wuint64("1"));
	REQUIRE(wuint96_81 / wuint96_80 == wuint96("1"));

	// 81 / 81
	REQUIRE(wuint32_81 / wuint32_81 == wuint32("1"));
	REQUIRE(wuint64_81 / wuint64_81 == wuint64("1"));
	REQUIRE(wuint96_81 / wuint96_81 == wuint96("1"));

	// 81 / FF
	REQUIRE(wuint32_81 / wuint32_FF == wuint32("0"));
	REQUIRE(wuint64_81 / wuint64_FF == wuint64("0"));
	REQUIRE(wuint96_81 / wuint96_FF == wuint96("0"));

	// FF / 7F
	REQUIRE(wuint32_FF / wuint32_7F == wuint32("2"));
	REQUIRE(wuint64_FF / wuint64_7F == wuint64("2"));
	REQUIRE(wuint96_FF / wuint96_7F == wuint96("2"));

	// FF / 80
	REQUIRE(wuint32_FF / wuint32_80 == wuint32("1"));
	REQUIRE(wuint64_FF / wuint64_80 == wuint64("1"));
	REQUIRE(wuint96_FF / wuint96_80 == wuint96("1"));

	// FF / 81
	REQUIRE(wuint32_FF / wuint32_81 == wuint32("1"));
	REQUIRE(wuint64_FF / wuint64_81 == wuint64("1"));
	REQUIRE(wuint96_FF / wuint96_81 == wuint96("1"));

	// FF / FF
	REQUIRE(wuint32_FF / wuint32_FF == wuint32("1"));
	REQUIRE(wuint64_FF / wuint64_FF == wuint64("1"));
	REQUIRE(wuint96_FF / wuint96_FF == wuint96("1"));
}

TEST_CASE("wuint wuint divide special", "[wuint]") {
	// Test cases from Hacker's Delight by Henry S. Warren, Jr.
	REQUIRE(wuint96("0x000080000000FFFE00000000") / wuint96("0x000080000000FFFF") == wuint96("0xFFFFFFFF"));
	REQUIRE(wuint128("0x000080000000FFFE00000000") / wuint128("0x000080000000FFFF") == wuint128("0xFFFFFFFF"));

	REQUIRE(wuint96("0x800000000000000000000003") / wuint96("0x200000000000000000000001") == wuint96("3"));
	REQUIRE(wuint128("0x800000000000000000000003") / wuint128("0x200000000000000000000001") == wuint128("3"));
	REQUIRE(wuint96("0x000080000000000000000003") / wuint96("0x000020000000000000000001") == wuint96("3"));
	REQUIRE(wuint128("0x000080000000000000000003") / wuint128("0x000020000000000000000001") == wuint128("3"));
	REQUIRE(wuint128("0x00007FFF000080000000000000000000") / wuint128("0x000080000000000000000001") == wuint128("0xFFFE0000"));
	REQUIRE(wuint256("0x00007FFF000080000000000000000000") / wuint256("0x000080000000000000000001") == wuint256("0xFFFE0000"));

	REQUIRE(wuint128("0x00008000000000000000FFFE00000000") / wuint128("0x00008000000000000000FFFF") == wuint128("0x0FFFFFFFF"));
	REQUIRE(wuint256("0x00008000000000000000FFFE00000000") / wuint256("0x00008000000000000000FFFF") == wuint256("0x0FFFFFFFF"));
	REQUIRE(wuint128("0x8000000000000000FFFFFFFE00000000") / wuint128("0x80000000000000000000FFFF") == wuint128("0x100000000"));
	REQUIRE(wuint256("0x8000000000000000FFFFFFFE00000000") / wuint256("0x80000000000000000000FFFF") == wuint256("0x100000000"));
	REQUIRE(wuint128("0x8000000000000000FFFFFFFE00000000") / wuint128("0x8000000000000000FFFFFFFF") == wuint128("0x0FFFFFFFF"));
	REQUIRE(wuint256("0x8000000000000000FFFFFFFE00000000") / wuint256("0x8000000000000000FFFFFFFF") == wuint256("0x0FFFFFFFF"));
}

TEST_CASE("wuint wuint modulus", "[wuint]") {
	REQUIRE(wuint32("9999") % wuint32("10000") == 9999);
	REQUIRE(wuint32("10000") % wuint32("10000") == 0);
	REQUIRE(wuint32("10001") % wuint32("10000") == 1);
	REQUIRE(wuint32("19999") % wuint32("10000") == 9999);
	REQUIRE(wuint32("20000") % wuint32("10000") == 0);

	REQUIRE(wuint32("100000000") % wuint32("10000") == 0);
	REQUIRE(wuint64("10000000000000000") % wuint64("10000") == 0);
	REQUIRE(wuint96("1000000000000000000000000") % wuint96("10000") == 0);

	REQUIRE(wuint32("10000") % wuint32("100000000") == 10000);
	REQUIRE(wuint64("10000") % wuint64("10000000000000000") == 10000);
	REQUIRE(wuint96("10000") % wuint96("1000000000000000000000000") == 10000);
}

TEST_CASE("wuint wuint modulus limits", "[wuint]") {
	// 7F % 7F
	REQUIRE(wuint32_7F % wuint32_7F == wuint32("0"));
	REQUIRE(wuint64_7F % wuint64_7F == wuint64("0"));
	REQUIRE(wuint96_7F % wuint96_7F == wuint96("0"));

	// 7F % 80
	REQUIRE(wuint32_7F % wuint32_80 == wuint32_7F);
	REQUIRE(wuint64_7F % wuint64_80 == wuint64_7F);
	REQUIRE(wuint96_7F % wuint96_80 == wuint96_7F);

	// 7F % 81
	REQUIRE(wuint32_7F % wuint32_81 == wuint32_7F);
	REQUIRE(wuint64_7F % wuint64_81 == wuint64_7F);
	REQUIRE(wuint96_7F % wuint96_81 == wuint96_7F);

	// 7F % FF
	REQUIRE(wuint32_7F % wuint32_FF == wuint32_7F);
	REQUIRE(wuint64_7F % wuint64_FF == wuint64_7F);
	REQUIRE(wuint96_7F % wuint96_FF == wuint96_7F);

	// 80 % 7F
	REQUIRE(wuint32_80 % wuint32_7F == wuint32("1"));
	REQUIRE(wuint64_80 % wuint64_7F == wuint64("1"));
	REQUIRE(wuint96_80 % wuint96_7F == wuint96("1"));

	// 80 % 80
	REQUIRE(wuint32_80 % wuint32_80 == wuint32("0"));
	REQUIRE(wuint64_80 % wuint64_80 == wuint64("0"));
	REQUIRE(wuint96_80 % wuint96_80 == wuint96("0"));

	// 80 % 81
	REQUIRE(wuint32_80 % wuint32_81 == wuint32_80);
	REQUIRE(wuint64_80 % wuint64_81 == wuint64_80);
	REQUIRE(wuint96_80 % wuint96_81 == wuint96_80);

	// 80 % FF
	REQUIRE(wuint32_80 % wuint32_FF == wuint32_80);
	REQUIRE(wuint64_80 % wuint64_FF == wuint64_80);
	REQUIRE(wuint96_80 % wuint96_FF == wuint96_80);

	// 81 % 7F
	REQUIRE(wuint32_81 % wuint32_7F == wuint32("2"));
	REQUIRE(wuint64_81 % wuint64_7F == wuint64("2"));
	REQUIRE(wuint96_81 % wuint96_7F == wuint96("2"));

	// 81 % 80
	REQUIRE(wuint32_81 % wuint32_80 == wuint32("1"));
	REQUIRE(wuint64_81 % wuint64_80 == wuint64("1"));
	REQUIRE(wuint96_81 % wuint96_80 == wuint96("1"));

	// 81 % 81
	REQUIRE(wuint32_81 % wuint32_81 == wuint32("0"));
	REQUIRE(wuint64_81 % wuint64_81 == wuint64("0"));
	REQUIRE(wuint96_81 % wuint96_81 == wuint96("0"));

	// 81 % FF
	REQUIRE(wuint32_81 % wuint32_FF == wuint32_81);
	REQUIRE(wuint64_81 % wuint64_FF == wuint64_81);
	REQUIRE(wuint96_81 % wuint96_FF == wuint96_81);

	// FF % 7F
	REQUIRE(wuint32_FF % wuint32_7F == wuint32("1"));
	REQUIRE(wuint64_FF % wuint64_7F == wuint64("1"));
	REQUIRE(wuint96_FF % wuint96_7F == wuint96("1"));

	// FF % 80
	REQUIRE(wuint32_FF % wuint32_80 == wuint32_7F);
	REQUIRE(wuint64_FF % wuint64_80 == wuint64_7F);
	REQUIRE(wuint96_FF % wuint96_80 == wuint96_7F);

	// FF % 81
	REQUIRE(wuint32_FF % wuint32_81 == wuint32_7F - wuint32("1"));
	REQUIRE(wuint64_FF % wuint64_81 == wuint64_7F - wuint64("1"));
	REQUIRE(wuint96_FF % wuint96_81 == wuint96_7F - wuint96("1"));

	// FF % FF
	REQUIRE(wuint32_FF % wuint32_FF == wuint32("0"));
	REQUIRE(wuint64_FF % wuint64_FF == wuint64("0"));
	REQUIRE(wuint96_FF % wuint96_FF == wuint96("0"));
}

TEST_CASE("wuint wuint modulus special", "[wuint]") {
	// Test cases from Hacker's Delight by Henry S. Warren, Jr.
	REQUIRE(wuint96("0x000080000000FFFE00000000") % wuint96("0x000080000000FFFF") == wuint96("0x00007FFF0000FFFF"));
	REQUIRE(wuint128("0x000080000000FFFE00000000") % wuint128("0x000080000000FFFF") == wuint128("0x00007FFF0000FFFF"));

	REQUIRE(wuint96("0x800000000000000000000003") % wuint96("0x200000000000000000000001") == wuint96("0x200000000000000000000000"));
	REQUIRE(wuint128("0x800000000000000000000003") % wuint128("0x200000000000000000000001") == wuint128("0x200000000000000000000000"));
	REQUIRE(wuint96("0x000080000000000000000003") % wuint96("0x000020000000000000000001") == wuint96("0x000020000000000000000000"));
	REQUIRE(wuint128("0x000080000000000000000003") % wuint128("0x000020000000000000000001") == wuint128("0x000020000000000000000000"));
	REQUIRE(wuint128("0x00007FFF000080000000000000000000") % wuint128("0x000080000000000000000001") == wuint128("0x00007FFFFFFFFFFF00020000"));
	REQUIRE(wuint256("0x00007FFF000080000000000000000000") % wuint256("0x000080000000000000000001") == wuint256("0x00007FFFFFFFFFFF00020000"));

	REQUIRE(wuint128("0x00008000000000000000FFFE00000000") % wuint128("0x00008000000000000000FFFF") == wuint128("0x00007FFFFFFFFFFF0000FFFF"));
	REQUIRE(wuint256("0x00008000000000000000FFFE00000000") % wuint256("0x00008000000000000000FFFF") == wuint256("0x00007FFFFFFFFFFF0000FFFF"));
	REQUIRE(wuint128("0x8000000000000000FFFFFFFE00000000") % wuint128("0x80000000000000000000FFFF") == wuint128("0xFFFEFFFF00000000"));
	REQUIRE(wuint256("0x8000000000000000FFFFFFFE00000000") % wuint256("0x80000000000000000000FFFF") == wuint256("0xFFFEFFFF00000000"));
	REQUIRE(wuint128("0x8000000000000000FFFFFFFE00000000") % wuint128("0x8000000000000000FFFFFFFF") == wuint128("0x7FFFFFFFFFFFFFFFFFFFFFFF"));
	REQUIRE(wuint256("0x8000000000000000FFFFFFFE00000000") % wuint256("0x8000000000000000FFFFFFFF") == wuint256("0x7FFFFFFFFFFFFFFFFFFFFFFF"));
}

TEST_CASE("wuint wuint bitwise and", "[wuint]") {
	REQUIRE((wuint32("-1") & wuint32("0")) == wuint32("0"));
	REQUIRE((wuint64("-1") & wuint64("0")) == wuint64("0"));
	REQUIRE((wuint96("-1") & wuint96("0")) == wuint96("0"));

	REQUIRE((wuint32("-1") & wuint32("-1")) == wuint32("-1"));
	REQUIRE((wuint64("-1") & wuint64("-1")) == wuint64("-1"));
	REQUIRE((wuint96("-1") & wuint96("-1")) == wuint96("-1"));

	REQUIRE((wuint32("0x1F2F3F4F") & wuint32("0xF0F0F0F0")) == wuint32("0x10203040"));
	REQUIRE((wuint64("0x1F2F3F4F5F6F7F8F") & wuint64("0xF0F0F0F0F0F0F0F0")) == wuint64("0x1020304050607080"));
	REQUIRE((wuint96("0x1F2F3F4F5F6F7F8F9FAFBFCF") & wuint96("0xF0F0F0F0F0F0F0F0F0F0F0F0")) == wuint96("0x102030405060708090A0B0C0"));
}

TEST_CASE("wuint wuint bitwise or", "[wuint]") {
	REQUIRE((wuint32("0") | wuint32("-1")) == wuint32("-1"));
	REQUIRE((wuint64("0") | wuint64("-1")) == wuint64("-1"));
	REQUIRE((wuint96("0") | wuint96("-1")) == wuint96("-1"));

	REQUIRE((wuint32("-1") | wuint32("-1")) == wuint32("-1"));
	REQUIRE((wuint64("-1") | wuint64("-1")) == wuint64("-1"));
	REQUIRE((wuint96("-1") | wuint96("-1")) == wuint96("-1"));

	REQUIRE((wuint32("0x10203040") | wuint32("0x0F0F0F0F")) == wuint32("0x1F2F3F4F"));
	REQUIRE((wuint64("0x1020304050607080") | wuint64("0x0F0F0F0F0F0F0F0F")) == wuint64("0x1F2F3F4F5F6F7F8F"));
	REQUIRE((wuint96("0x102030405060708090A0B0C0") | wuint96("0x0F0F0F0F0F0F0F0F0F0F0F0F")) == wuint96("0x1F2F3F4F5F6F7F8F9FAFBFCF"));
}

TEST_CASE("wuint wuint bitwise xor", "[wuint]") {
	REQUIRE((wuint32("-1") ^ wuint32("0")) == wuint32("-1"));
	REQUIRE((wuint64("-1") ^ wuint64("0")) == wuint64("-1"));
	REQUIRE((wuint96("-1") ^ wuint96("0")) == wuint96("-1"));

	REQUIRE((wuint32("-1") ^ wuint32("-1")) == wuint32("0"));
	REQUIRE((wuint64("-1") ^ wuint64("-1")) == wuint64("0"));
	REQUIRE((wuint96("-1") ^ wuint96("-1")) == wuint96("0"));

	REQUIRE((wuint32("0xAAAAAAAA") ^ wuint32("0x3C3C3C3C")) == wuint32("0x96969696"));
	REQUIRE((wuint64("0xAAAAAAAAAAAAAAAA") ^ wuint64("0x3C3C3C3C3C3C3C3C")) == wuint64("0x9696969696969696"));
	REQUIRE((wuint96("0xAAAAAAAAAAAAAAAAAAAAAAAA") ^ wuint96("0x3C3C3C3C3C3C3C3C3C3C3C3C")) == wuint96("0x969696969696969696969696"));
}

TEST_CASE("wuint uint32 plus", "[wuint]") {
	REQUIRE(wuint32("1000000000") + 1 == wuint32("1000000001"));
	REQUIRE(wuint64("1000000000000000000") + 1 == wuint64("1000000000000000001"));
	REQUIRE(wuint96("1000000000000000000000000000") + 1 == wuint96("1000000000000000000000000001"));

	REQUIRE(wuint32("999999999") + 1 == wuint32("1000000000"));
	REQUIRE(wuint64("999999999999999999") + 1 == wuint64("1000000000000000000"));
	REQUIRE(wuint96("999999999999999999999999999") + 1 == wuint96("1000000000000000000000000000"));

	REQUIRE(wuint32("0xEFFFFFFF") + 1 == wuint32("0xF0000000"));
	REQUIRE(wuint64("0xEFFFFFFFFFFFFFFF") + 1 == wuint64("0xF000000000000000"));
	REQUIRE(wuint96("0xEFFFFFFFFFFFFFFFFFFFFFFF") + 1 == wuint96("0xF00000000000000000000000"));

	REQUIRE(wuint32("1000000000") + 123456789 == wuint32("1123456789"));
	REQUIRE(wuint64("100000000000000000") + 123456789 == wuint64("100000000123456789"));
	REQUIRE(wuint96("100000000000000000000000000") + 123456789 == wuint96("100000000000000000123456789"));

	REQUIRE(wuint32("-1") + 1 == wuint32("0"));
	REQUIRE(wuint64("-1") + 1 == wuint64("0"));
	REQUIRE(wuint96("-1") + 1 == wuint96("0"));

	REQUIRE(1 + wuint32("1000000000") == wuint32("1000000001"));
	REQUIRE(1 + wuint64("1000000000000000000") == wuint64("1000000000000000001"));
	REQUIRE(1 + wuint96("1000000000000000000000000000") == wuint96("1000000000000000000000000001"));

	REQUIRE(1 + wuint32("999999999") == wuint32("1000000000"));
	REQUIRE(1 + wuint64("999999999999999999") == wuint64("1000000000000000000"));
	REQUIRE(1 + wuint96("999999999999999999999999999") == wuint96("1000000000000000000000000000"));

	REQUIRE(1 + wuint32("0xEFFFFFFF") == wuint32("0xF0000000"));
	REQUIRE(1 + wuint64("0xEFFFFFFFFFFFFFFF") == wuint64("0xF000000000000000"));
	REQUIRE(1 + wuint96("0xEFFFFFFFFFFFFFFFFFFFFFFF") == wuint96("0xF00000000000000000000000"));

	REQUIRE(123456789 + wuint32("1000000000") == wuint32("1123456789"));
	REQUIRE(123456789 + wuint64("100000000000000000") == wuint64("100000000123456789"));
	REQUIRE(123456789 + wuint96("100000000000000000000000000") == wuint96("100000000000000000123456789"));

	REQUIRE(1 + wuint32("-1") == wuint32("0"));
	REQUIRE(1 + wuint64("-1") == wuint64("0"));
	REQUIRE(1 + wuint96("-1") == wuint96("0"));
}

TEST_CASE("wuint uint32 minus", "[wuint]") {
	REQUIRE(wuint32("1000000001") - 1 == wuint32("1000000000"));
	REQUIRE(wuint64("1000000000000000001") - 1 == wuint64("1000000000000000000"));
	REQUIRE(wuint96("1000000000000000000000000001") - 1 == wuint96("1000000000000000000000000000"));

	REQUIRE(wuint32("1000000000") - 1 == wuint32("999999999"));
	REQUIRE(wuint64("1000000000000000000") - 1 == wuint64("999999999999999999"));
	REQUIRE(wuint96("1000000000000000000000000000") - 1 == wuint96("999999999999999999999999999"));

	REQUIRE(wuint32("0") - 1 == wuint32("-1"));
	REQUIRE(wuint64("0") - 1 == wuint64("-1"));
	REQUIRE(wuint96("0") - 1 == wuint96("-1"));

	REQUIRE(wuint32("1123456789") - 123456789 == wuint32("1000000000"));
	REQUIRE(wuint64("100000000123456789") - 123456789 == wuint64("100000000000000000"));
	REQUIRE(wuint96("100000000000000000123456789") - 123456789 == wuint96("100000000000000000000000000"));

	REQUIRE(0 - wuint32("1") == wuint32("-1"));
	REQUIRE(0 - wuint64("1") == wuint64("-1"));
	REQUIRE(0 - wuint96("1") == wuint96("-1"));

	REQUIRE(123456789 - wuint32("100000000") == wuint32("23456789"));
	REQUIRE(123456789 - wuint64("100000000") == wuint64("23456789"));
	REQUIRE(123456789 - wuint96("100000000") == wuint96("23456789"));

	REQUIRE(123456789 - wuint32("999999999") == wuint32("-876543210"));
	REQUIRE(123456789 - wuint64("999999999") == wuint64("-876543210"));
	REQUIRE(123456789 - wuint96("999999999") == wuint96("-876543210"));
}

TEST_CASE("wuint uint32 multiply", "[wuint]") {
	REQUIRE(wuint32("-1") * 0 == wuint32("0"));
	REQUIRE(wuint64("-1") * 0 == wuint64("0"));
	REQUIRE(wuint96("-1") * 0 == wuint96("0"));

	REQUIRE(wuint32("1000000000") * 1 == wuint32("1000000000"));
	REQUIRE(wuint64("1000000000000000000") * 1 == wuint64("1000000000000000000"));
	REQUIRE(wuint96("1000000000000000000000000000") * 1 == wuint96("1000000000000000000000000000"));

	REQUIRE(wuint32("1000000000") * 2 == wuint32("2000000000"));
	REQUIRE(wuint64("1000000000000000000") * 2 == wuint64("2000000000000000000"));
	REQUIRE(wuint96("1000000000000000000000000000") * 2 == wuint96("2000000000000000000000000000"));

	REQUIRE(wuint32("10000") * 10000 == wuint32("100000000"));
	REQUIRE(wuint64("100000000") * 10000 == wuint64("1000000000000"));
	REQUIRE(wuint96("1000000000000") * 10000 == wuint96("10000000000000000"));

	REQUIRE(1 * wuint32("1000000000") == wuint32("1000000000"));
	REQUIRE(1 * wuint64("1000000000000000000") == wuint64("1000000000000000000"));
	REQUIRE(1 * wuint96("1000000000000000000000000000") == wuint96("1000000000000000000000000000"));

	REQUIRE(2 * wuint32("1000000000") == wuint32("2000000000"));
	REQUIRE(2 * wuint64("1000000000000000000") == wuint64("2000000000000000000"));
	REQUIRE(2 * wuint96("1000000000000000000000000000") == wuint96("2000000000000000000000000000"));

	REQUIRE(10000 * wuint32("10000") == wuint32("100000000"));
	REQUIRE(10000 * wuint64("100000000") == wuint64("1000000000000"));
	REQUIRE(10000 * wuint96("1000000000000") == wuint96("10000000000000000"));

	REQUIRE(10000 * wuint32("-1") == wuint32("-10000"));
	REQUIRE(10000 * wuint64("-1") == wuint64("-10000"));
	REQUIRE(10000 * wuint96("-1") == wuint96("-10000"));

	REQUIRE(10000 * wuint32("-10000") == wuint32("-100000000"));
	REQUIRE(10000 * wuint64("-100000000") == wuint64("-1000000000000"));
	REQUIRE(10000 * wuint96("-1000000000000") == wuint96("-10000000000000000"));
}

TEST_CASE("wuint uint32 divide", "[wuint]") {
	REQUIRE(wuint32("1000000000") / 1 == wuint32("1000000000"));
	REQUIRE(wuint64("1000000000000000000") / 1 == wuint64("1000000000000000000"));
	REQUIRE(wuint96("1000000000000000000000000000") / 1 == wuint96("1000000000000000000000000000"));

	REQUIRE(wuint32("1000000000") / 2 == wuint32("500000000"));
	REQUIRE(wuint64("1000000000000000000") / 2 == wuint64("500000000000000000"));
	REQUIRE(wuint96("1000000000000000000000000000") / 2 == wuint96("500000000000000000000000000"));

	REQUIRE(wuint32("9999") / 10000 == wuint32("0"));
	REQUIRE(wuint32("10000") / 10000 == wuint32("1"));
	REQUIRE(wuint32("10001") / 10000 == wuint32("1"));
	REQUIRE(wuint32("19999") / 10000 == wuint32("1"));
	REQUIRE(wuint32("20000") / 10000 == wuint32("2"));

	REQUIRE(wuint32("100000000") / 10000 == wuint32("10000"));
	REQUIRE(wuint64("10000000000000000") / 10000 == wuint64("1000000000000"));
	REQUIRE(wuint96("1000000000000000000000000") / 10000 == wuint96("100000000000000000000"));

	REQUIRE(9999 / wuint32("10000") == wuint32("0"));
	REQUIRE(10000 / wuint32("10000") == wuint32("1"));
	REQUIRE(10001 / wuint32("10000") == wuint32("1"));
	REQUIRE(19999 / wuint32("10000") == wuint32("1"));
	REQUIRE(20000 / wuint32("10000") == wuint32("2"));

	REQUIRE(10000 / wuint32("100000000") == wuint32("0"));
	REQUIRE(10000 / wuint64("10000000000000000") == wuint64("0"));
	REQUIRE(10000 / wuint96("1000000000000000000000000") == wuint96("0"));
}

TEST_CASE("wuint uint32 divide limits", "[wuint]") {
	// 01 / 01
	REQUIRE(wuint32("1") / uint32_01 == wuint32("1"));
	REQUIRE(wuint64("1") / uint32_01 == wuint64("1"));
	REQUIRE(wuint96("1") / uint32_01 == wuint96("1"));

	// 01 / 7F
	REQUIRE(wuint32("1") / uint32_7F == wuint32("0"));
	REQUIRE(wuint64("1") / uint32_7F == wuint64("0"));
	REQUIRE(wuint96("1") / uint32_7F == wuint96("0"));

	// 01 / 80
	REQUIRE(wuint32("1") / uint32_80 == wuint32("0"));
	REQUIRE(wuint64("1") / uint32_80 == wuint64("0"));
	REQUIRE(wuint96("1") / uint32_80 == wuint96("0"));

	// 01 / 81
	REQUIRE(wuint32("1") / uint32_81 == wuint32("0"));
	REQUIRE(wuint64("1") / uint32_81 == wuint64("0"));
	REQUIRE(wuint96("1") / uint32_81 == wuint96("0"));

	// 01 / FF
	REQUIRE(wuint32("1") / uint32_FF == wuint32("0"));
	REQUIRE(wuint64("1") / uint32_FF == wuint64("0"));
	REQUIRE(wuint96("1") / uint32_FF == wuint96("0"));

	// 7F / 01
	REQUIRE(wuint32_7F / uint32_01 == wuint32_7F);
	REQUIRE(wuint64_7F / uint32_01 == wuint64_7F);
	REQUIRE(wuint96_7F / uint32_01 == wuint96_7F);

	// 7F / 7F
	REQUIRE(wuint32_7F / uint32_7F == wuint32("1"));
	REQUIRE(wuint64_7F / uint32_7F == wuint64("0x100000002"));
	REQUIRE(wuint96_7F / uint32_7F == wuint96("0x10000000200000004"));

	// 7F / 80
	REQUIRE(wuint32_7F / uint32_80 == wuint32("0"));
	REQUIRE(wuint64_7F / uint32_80 == wuint64("0xFFFFFFFF"));
	REQUIRE(wuint96_7F / uint32_80 == wuint96("0xFFFFFFFFFFFFFFFF"));

	// 7F / 81
	REQUIRE(wuint32_7F / uint32_81 == wuint32("0"));
	REQUIRE(wuint64_7F / uint32_81 == wuint64("0xFFFFFFFE"));
	REQUIRE(wuint96_7F / uint32_81 == wuint96("0xFFFFFFFE00000003"));

	// 7F / FF
	REQUIRE(wuint32_7F / uint32_FF == wuint32("0"));
	REQUIRE(wuint64_7F / uint32_FF == wuint64("0x80000000"));
	REQUIRE(wuint96_7F / uint32_FF == wuint96("0x8000000080000000"));

	// 80 / 01
	REQUIRE(wuint32_80 / uint32_01 == wuint32_80);
	REQUIRE(wuint64_80 / uint32_01 == wuint64_80);
	REQUIRE(wuint96_80 / uint32_01 == wuint96_80);

	// 80 / 7F
	REQUIRE(wuint32_80 / uint32_7F == wuint32("1"));
	REQUIRE(wuint64_80 / uint32_7F == wuint64("0x100000002"));
	REQUIRE(wuint96_80 / uint32_7F == wuint96("0x10000000200000004"));

	// 80 / 80
	REQUIRE(wuint32_80 / uint32_80 == wuint32("1"));
	REQUIRE(wuint64_80 / uint32_80 == wuint64("0x100000000"));
	REQUIRE(wuint96_80 / uint32_80 == wuint96("0x10000000000000000"));

	// 80 / 81
	REQUIRE(wuint32_80 / uint32_81 == wuint32("0"));
	REQUIRE(wuint64_80 / uint32_81 == wuint64("0xFFFFFFFE"));
	REQUIRE(wuint96_80 / uint32_81 == wuint96("0xFFFFFFFE00000003"));

	// 80 / FF
	REQUIRE(wuint32_80 / uint32_FF == wuint32("0"));
	REQUIRE(wuint64_80 / uint32_FF == wuint64("0x80000000"));
	REQUIRE(wuint96_80 / uint32_FF == wuint96("0x8000000080000000"));

	// 81 / 01
	REQUIRE(wuint32_81 / uint32_01 == wuint32_81);
	REQUIRE(wuint64_81 / uint32_01 == wuint64_81);
	REQUIRE(wuint96_81 / uint32_01 == wuint96_81);

	// 81 / 7F
	REQUIRE(wuint32_81 / uint32_7F == wuint32("1"));
	REQUIRE(wuint64_81 / uint32_7F == wuint64("0x100000002"));
	REQUIRE(wuint96_81 / uint32_7F == wuint96("0x10000000200000004"));

	// 81 / 80
	REQUIRE(wuint32_81 / uint32_80 == wuint32("1"));
	REQUIRE(wuint64_81 / uint32_80 == wuint64("0x100000000"));
	REQUIRE(wuint96_81 / uint32_80 == wuint96("0x10000000000000000"));

	// 81 / 81
	REQUIRE(wuint32_81 / uint32_81 == wuint32("1"));
	REQUIRE(wuint64_81 / uint32_81 == wuint64("0xFFFFFFFE"));
	REQUIRE(wuint96_81 / uint32_81 == wuint96("0xFFFFFFFE00000003"));

	// 81 / FF
	REQUIRE(wuint32_81 / uint32_FF == wuint32("0"));
	REQUIRE(wuint64_81 / uint32_FF == wuint64("0x80000000"));
	REQUIRE(wuint96_81 / uint32_FF == wuint96("0x8000000080000000"));

	// FF / 01
	REQUIRE(wuint32_FF / uint32_01 == wuint32_FF);
	REQUIRE(wuint64_FF / uint32_01 == wuint64_FF);
	REQUIRE(wuint96_FF / uint32_01 == wuint96_FF);

	// FF / 7F
	REQUIRE(wuint32_FF / uint32_7F == wuint32("2"));
	REQUIRE(wuint64_FF / uint32_7F == wuint64("0x200000004"));
	REQUIRE(wuint96_FF / uint32_7F == wuint96("0x20000000400000008"));

	// FF / 80
	REQUIRE(wuint32_FF / uint32_80 == wuint32("1"));
	REQUIRE(wuint64_FF / uint32_80 == wuint64("0x1FFFFFFFF"));
	REQUIRE(wuint96_FF / uint32_80 == wuint96("0x1FFFFFFFFFFFFFFFF"));

	// FF / 81
	REQUIRE(wuint32_FF / uint32_81 == wuint32("1"));
	REQUIRE(wuint64_FF / uint32_81 == wuint64("0x1FFFFFFFC"));
	REQUIRE(wuint96_FF / uint32_81 == wuint96("0x1FFFFFFFC00000007"));

	// FF / FF
	REQUIRE(wuint32_FF / uint32_FF == wuint32("1"));
	REQUIRE(wuint64_FF / uint32_FF == wuint64("0x100000001"));
	REQUIRE(wuint96_FF / uint32_FF == wuint96("0x10000000100000001"));
}

TEST_CASE("uint32 wuint divide limits", "[wuint]") {
	// 01 / 01
	REQUIRE(uint32_01 / wuint32("1") == wuint32("1"));
	REQUIRE(uint32_01 / wuint64("1") == wuint64("1"));
	REQUIRE(uint32_01 / wuint96("1") == wuint96("1"));

	// 01 / 7F
	REQUIRE(uint32_01 / wuint32_7F == wuint32("0"));

	// 01 / 80
	REQUIRE(uint32_01 / wuint32_80 == wuint32("0"));

	// 01 / 81
	REQUIRE(uint32_01 / wuint32_81 == wuint32("0"));

	// 01 / FF
	REQUIRE(uint32_01 / wuint32_FF == wuint32("0"));

	// 7F / 01
	REQUIRE(uint32_7F / wuint32("1") == wuint32_7F);

	// 7F / 7F
	REQUIRE(uint32_7F / wuint32_7F == wuint32("1"));

	// 7F / 80
	REQUIRE(uint32_7F / wuint32_80 == wuint32("0"));

	// 7F / 81
	REQUIRE(uint32_7F / wuint32_81 == wuint32("0"));

	// 7F / FF
	REQUIRE(uint32_7F / wuint32_FF == wuint32("0"));

	// 80 / 01
	REQUIRE(uint32_80 / wuint32("1") == wuint32_80);

	// 80 / 7F
	REQUIRE(uint32_80 / wuint32_7F == wuint32("1"));

	// 80 / 80
	REQUIRE(uint32_80 / wuint32_80 == wuint32("1"));

	// 80 / 81
	REQUIRE(uint32_80 / wuint32_81 == wuint32("0"));

	// 80 / FF
	REQUIRE(uint32_80 / wuint32_FF == wuint32("0"));

	// 81 / 01
	REQUIRE(uint32_81 / wuint32("1") == wuint32_81);

	// 81 / 7F
	REQUIRE(uint32_81 / wuint32_7F == wuint32("1"));

	// 81 / 80
	REQUIRE(uint32_81 / wuint32_80 == wuint32("1"));

	// 81 / 81
	REQUIRE(uint32_81 / wuint32_81 == wuint32("1"));

	// 81 / FF
	REQUIRE(uint32_81 / wuint32_FF == wuint32("0"));

	// FF / 01
	REQUIRE(uint32_FF / wuint32("1") == wuint32_FF);

	// FF / 7F
	REQUIRE(uint32_FF / wuint32_7F == wuint32("2"));

	// FF / 80
	REQUIRE(uint32_FF / wuint32_80 == wuint32("1"));

	// FF / 81
	REQUIRE(uint32_FF / wuint32_81 == wuint32("1"));

	// FF / FF
	REQUIRE(uint32_FF / wuint32_FF == wuint32("1"));
}

TEST_CASE("wuint uint32 modulus", "[wuint]") {
	REQUIRE(wuint32("9999") % 10000 == 9999);
	REQUIRE(wuint32("10000") % 10000 == 0);
	REQUIRE(wuint32("10001") % 10000 == 1);
	REQUIRE(wuint32("19999") % 10000 == 9999);
	REQUIRE(wuint32("20000") % 10000 == 0);

	REQUIRE(wuint32("-1") % 2 == 1);
	REQUIRE(wuint64("-1") % 2 == 1);
	REQUIRE(wuint96("-1") % 2 == 1);

	REQUIRE(wuint32("100000000") % 10000 == 0);
	REQUIRE(wuint64("10000000000000000") % 10000 == 0);
	REQUIRE(wuint96("1000000000000000000000000") % 10000 == 0);

	REQUIRE(9999 % wuint32("10000") == wuint32("9999"));
	REQUIRE(10000 % wuint32("10000") == wuint32("0"));
	REQUIRE(10001 % wuint32("10000") == wuint32("1"));
	REQUIRE(19999 % wuint32("10000") == wuint32("9999"));
	REQUIRE(20000 % wuint32("10000") == wuint32("0"));

	REQUIRE(10000 % wuint32("100000000") == wuint32("10000"));
	REQUIRE(10000 % wuint64("10000000000000000") == wuint64("10000"));
	REQUIRE(10000 % wuint96("1000000000000000000000000") == wuint96("10000"));
}

TEST_CASE("wuint uint32 modulus limits", "[wuint]") {
	// 01 % 01
	REQUIRE(wuint32("1") % uint32_01 == wuint32("0"));
	REQUIRE(wuint64("1") % uint32_01 == wuint64("0"));
	REQUIRE(wuint96("1") % uint32_01 == wuint96("0"));

	// 01 % 7F
	REQUIRE(wuint32("1") % uint32_7F == wuint32("1"));
	REQUIRE(wuint64("1") % uint32_7F == wuint64("1"));
	REQUIRE(wuint96("1") % uint32_7F == wuint96("1"));

	// 01 % 80
	REQUIRE(wuint32("1") % uint32_80 == wuint32("1"));
	REQUIRE(wuint64("1") % uint32_80 == wuint64("1"));
	REQUIRE(wuint96("1") % uint32_80 == wuint96("1"));

	// 01 % 81
	REQUIRE(wuint32("1") % uint32_81 == wuint32("1"));
	REQUIRE(wuint64("1") % uint32_81 == wuint64("1"));
	REQUIRE(wuint96("1") % uint32_81 == wuint96("1"));

	// 01 % FF
	REQUIRE(wuint32("1") % uint32_FF == wuint32("1"));
	REQUIRE(wuint64("1") % uint32_FF == wuint64("1"));
	REQUIRE(wuint96("1") % uint32_FF == wuint96("1"));

	// 7F % 01
	REQUIRE(wuint32_7F % uint32_01 == wuint32("0"));
	REQUIRE(wuint64_7F % uint32_01 == wuint64("0"));
	REQUIRE(wuint96_7F % uint32_01 == wuint96("0"));

	// 7F % 7F
	REQUIRE(wuint32_7F % uint32_7F == wuint32("0"));
	REQUIRE(wuint64_7F % uint32_7F == wuint64("1"));
	REQUIRE(wuint96_7F % uint32_7F == wuint96("3"));

	// 7F % 80
	REQUIRE(wuint32_7F % uint32_80 == wuint32_7F);
	REQUIRE(wuint64_7F % uint32_80 == wuint64("0x7FFFFFFF"));
	REQUIRE(wuint96_7F % uint32_80 == wuint96("0x7FFFFFFF"));

	// 7F % 81
	REQUIRE(wuint32_7F % uint32_81 == wuint32_7F);
	REQUIRE(wuint64_7F % uint32_81 == wuint64("1"));
	REQUIRE(wuint96_7F % uint32_81 == wuint96("0x7FFFFFFC"));

	// 7F % FF
	REQUIRE(wuint32_7F % uint32_FF == wuint32_7F);
	REQUIRE(wuint64_7F % uint32_FF == wuint64("0x7FFFFFFF"));
	REQUIRE(wuint96_7F % uint32_FF == wuint96("0x7FFFFFFF"));

	// 80 % 01
	REQUIRE(wuint32_80 % uint32_01 == wuint32("0"));
	REQUIRE(wuint64_80 % uint32_01 == wuint64("0"));
	REQUIRE(wuint96_80 % uint32_01 == wuint96("0"));

	// 80 % 7F
	REQUIRE(wuint32_80 % uint32_7F == wuint32("1"));
	REQUIRE(wuint64_80 % uint32_7F == wuint64("2"));
	REQUIRE(wuint96_80 % uint32_7F == wuint96("4"));

	// 80 % 80
	REQUIRE(wuint32_80 % uint32_80 == wuint32("0"));
	REQUIRE(wuint64_80 % uint32_80 == wuint64("0"));
	REQUIRE(wuint96_80 % uint32_80 == wuint96("0"));

	// 80 % 81
	REQUIRE(wuint32_80 % uint32_81 == wuint32_80);
	REQUIRE(wuint64_80 % uint32_81 == wuint64("2"));
	REQUIRE(wuint96_80 % uint32_81 == wuint96("0x7FFFFFFD"));

	// 80 % FF
	REQUIRE(wuint32_80 % uint32_FF == wuint32_80);
	REQUIRE(wuint64_80 % uint32_FF == wuint64("0x80000000"));
	REQUIRE(wuint96_80 % uint32_FF == wuint96("0x80000000"));

	// 81 % 01
	REQUIRE(wuint32_81 % uint32_01 == wuint32("0"));
	REQUIRE(wuint64_81 % uint32_01 == wuint64("0"));
	REQUIRE(wuint96_81 % uint32_01 == wuint96("0"));

	// 81 % 7F
	REQUIRE(wuint32_81 % uint32_7F == wuint32("2"));
	REQUIRE(wuint64_81 % uint32_7F == wuint64("3"));
	REQUIRE(wuint96_81 % uint32_7F == wuint96("5"));

	// 81 % 80
	REQUIRE(wuint32_81 % uint32_80 == wuint32("1"));
	REQUIRE(wuint64_81 % uint32_80 == wuint64("1"));
	REQUIRE(wuint96_81 % uint32_80 == wuint96("1"));

	// 81 % 81
	REQUIRE(wuint32_81 % uint32_81 == wuint32("0"));
	REQUIRE(wuint64_81 % uint32_81 == wuint64("3"));
	REQUIRE(wuint96_81 % uint32_81 == wuint96("0x7FFFFFFE"));

	// 81 % FF
	REQUIRE(wuint32_81 % uint32_FF == wuint32_81);
	REQUIRE(wuint64_81 % uint32_FF == wuint64("0x80000001"));
	REQUIRE(wuint96_81 % uint32_FF == wuint96("0x80000001"));

	// FF % 01
	REQUIRE(wuint32_FF % uint32_01 == wuint32("0"));
	REQUIRE(wuint64_FF % uint32_01 == wuint64("0"));
	REQUIRE(wuint96_FF % uint32_01 == wuint96("0"));

	// FF % 7F
	REQUIRE(wuint32_FF % uint32_7F == wuint32("1"));
	REQUIRE(wuint64_FF % uint32_7F == wuint64("3"));
	REQUIRE(wuint96_FF % uint32_7F == wuint96("7"));

	// FF % 80
	REQUIRE(wuint32_FF % uint32_80 == wuint32_7F);
	REQUIRE(wuint64_FF % uint32_80 == wuint64("0x7FFFFFFF"));
	REQUIRE(wuint96_FF % uint32_80 == wuint96("0x7FFFFFFF"));

	// FF % 81
	REQUIRE(wuint32_FF % uint32_81 == wuint32("0x7FFFFFFE"));
	REQUIRE(wuint64_FF % uint32_81 == wuint64("3"));
	REQUIRE(wuint96_FF % uint32_81 == wuint96("0x7FFFFFF8"));

	// FF % FF
	REQUIRE(wuint32_FF % uint32_FF == wuint32("0"));
	REQUIRE(wuint64_FF % uint32_FF == wuint64("0"));
	REQUIRE(wuint96_FF % uint32_FF == wuint96("0"));
}

TEST_CASE("uint32 wuint modulus limits", "[wuint]") {
	// 01 % 01
	REQUIRE(uint32_01 % wuint32("1") == wuint32("0"));
	REQUIRE(uint32_01 % wuint64("1") == wuint64("0"));
	REQUIRE(uint32_01 % wuint96("1") == wuint96("0"));

	// 7F % 01
	REQUIRE(uint32_7F % wuint32("1") == wuint32("0"));

	// 7F % 7F
	REQUIRE(uint32_7F % wuint32_7F == wuint32("0"));

	// 80 % 01
	REQUIRE(uint32_80 % wuint32("1") == wuint32("0"));

	// 80 % 7F
	REQUIRE(uint32_80 % wuint32_7F == wuint32("1"));

	// 80 % 80
	REQUIRE(uint32_80 % wuint32_80 == wuint32("0"));

	// 81 % 01
	REQUIRE(uint32_81 % wuint32("1") == wuint32("0"));

	// 81 % 7F
	REQUIRE(uint32_81 % wuint32_7F == wuint32("2"));

	// 81 % 80
	REQUIRE(uint32_81 % wuint32_80 == wuint32("1"));

	// 81 % 81
	REQUIRE(uint32_81 % wuint32_81 == wuint32("0"));

	// FF % 01
	REQUIRE(uint32_FF % wuint32("1") == wuint32("0"));

	// FF % 7F
	REQUIRE(uint32_FF % wuint32_7F == wuint32("1"));

	// FF % 80
	REQUIRE(uint32_FF % wuint32_80 == wuint32_7F);

	// FF % 81
	REQUIRE(uint32_FF % wuint32_81 == wuint32("0x7FFFFFFE"));

	// FF % FF
	REQUIRE(uint32_FF % wuint32_FF == wuint32("0"));
}

TEST_CASE("wuint uint32 bitwise and", "[wuint]") {
	REQUIRE((wuint32("-1") & 0) == 0);
	REQUIRE((wuint64("-1") & 0) == 0);
	REQUIRE((wuint96("-1") & 0) == 0);

	REQUIRE((wuint32("-1") & -1) == std::uint32_t(-1));
	REQUIRE((wuint64("-1") & -1) == std::uint32_t(-1));
	REQUIRE((wuint96("-1") & -1) == std::uint32_t(-1));

	REQUIRE((wuint32("0x1F2F3F4F") & 0xF0F0F0F0) == 0x10203040);
	REQUIRE((wuint64("0x1F2F3F4F5F6F7F8F") & 0xF0F0F0F0) == 0x50607080);
	REQUIRE((wuint96("0x1F2F3F4F5F6F7F8F9FAFBFCF") & 0xF0F0F0F0) == 0x90A0B0C0);

	REQUIRE((0xFFFFFFFF & wuint32("0")) == wuint32("0"));
	REQUIRE((0xFFFFFFFF & wuint64("0")) == wuint64("0"));
	REQUIRE((0xFFFFFFFF & wuint96("0")) == wuint96("0"));

	REQUIRE((0xFFFFFFFF & wuint32("-1")) == wuint32("0xFFFFFFFF"));
	REQUIRE((0xFFFFFFFF & wuint64("-1")) == wuint64("0xFFFFFFFF"));
	REQUIRE((0xFFFFFFFF & wuint96("-1")) == wuint96("0xFFFFFFFF"));

	REQUIRE((0xF0F0F0F0 & wuint32("0x1F2F3F4F")) == wuint32("0x10203040"));
	REQUIRE((0xF0F0F0F0 & wuint64("0x1F2F3F4F5F6F7F8F")) == wuint64("0x50607080"));
	REQUIRE((0xF0F0F0F0 & wuint96("0x1F2F3F4F5F6F7F8F9FAFBFCF")) == wuint96("0x90A0B0C0"));
}

TEST_CASE("wuint uint32 bitwise or", "[wuint]") {
	REQUIRE((wuint32("0") | -1) == wuint32("0xFFFFFFFF"));
	REQUIRE((wuint64("0") | -1) == wuint64("0xFFFFFFFF"));
	REQUIRE((wuint96("0") | -1) == wuint96("0xFFFFFFFF"));

	REQUIRE((wuint32("-1") | -1) == wuint32("-1"));
	REQUIRE((wuint64("-1") | -1) == wuint64("-1"));
	REQUIRE((wuint96("-1") | -1) == wuint96("-1"));

	REQUIRE((wuint32("0x10203040") | 0x0F0F0F0F) == wuint32("0x1F2F3F4F"));
	REQUIRE((wuint64("0x1020304050607080") | 0x0F0F0F0F) == wuint64("0x102030405F6F7F8F"));
	REQUIRE((wuint96("0x102030405060708090A0B0C0") | 0x0F0F0F0F) == wuint96("0x10203040506070809FAFBFCF"));

	REQUIRE((0 | wuint32("-1")) == wuint32("-1"));
	REQUIRE((0 | wuint64("-1")) == wuint64("-1"));
	REQUIRE((0 | wuint96("-1")) == wuint96("-1"));

	REQUIRE((0xFFFFFFFF | wuint32("-1")) == wuint32("-1"));
	REQUIRE((0xFFFFFFFF | wuint64("-1")) == wuint64("-1"));
	REQUIRE((0xFFFFFFFF | wuint96("-1")) == wuint96("-1"));

	REQUIRE((0x0F0F0F0F | wuint32("0x10203040")) == wuint32("0x1F2F3F4F"));
	REQUIRE((0x0F0F0F0F | wuint64("0x1020304050607080")) == wuint64("0x102030405F6F7F8F"));
	REQUIRE((0x0F0F0F0F | wuint96("0x102030405060708090A0B0C0")) == wuint96("0x10203040506070809FAFBFCF"));
}

TEST_CASE("wuint uint32 bitwise xor", "[wuint]") {
	REQUIRE((wuint32("-1") ^ 0) == wuint32("-1"));
	REQUIRE((wuint64("-1") ^ 0) == wuint64("-1"));
	REQUIRE((wuint96("-1") ^ 0) == wuint96("-1"));

	REQUIRE((wuint32("-1") ^ 0xFFFFFFFF) == wuint32("0"));
	REQUIRE((wuint64("-1") ^ 0xFFFFFFFF) == wuint64("0xFFFFFFFF00000000"));
	REQUIRE((wuint96("-1") ^ 0xFFFFFFFF) == wuint96("0xFFFFFFFFFFFFFFFF00000000"));

	REQUIRE((wuint32("0xAAAAAAAA") ^ 0x3C3C3C3C) == wuint32("0x96969696"));
	REQUIRE((wuint64("0xAAAAAAAAAAAAAAAA") ^ 0x3C3C3C3C) == wuint64("0xAAAAAAAA96969696"));
	REQUIRE((wuint96("0xAAAAAAAAAAAAAAAAAAAAAAAA") ^ 0x3C3C3C3C) == wuint96("0xAAAAAAAAAAAAAAAA96969696"));

	REQUIRE((0 ^ wuint32("-1")) == wuint32("-1"));
	REQUIRE((0 ^ wuint64("-1")) == wuint64("-1"));
	REQUIRE((0 ^ wuint96("-1")) == wuint96("-1"));

	REQUIRE((0xFFFFFFFF ^ wuint32("-1")) == wuint32("0"));
	REQUIRE((0xFFFFFFFF ^ wuint64("-1")) == wuint64("0xFFFFFFFF00000000"));
	REQUIRE((0xFFFFFFFF ^ wuint96("-1")) == wuint96("0xFFFFFFFFFFFFFFFF00000000"));

	REQUIRE((0x3C3C3C3C ^ wuint32("0xAAAAAAAA")) == wuint32("0x96969696"));
	REQUIRE((0x3C3C3C3C ^ wuint64("0xAAAAAAAAAAAAAAAA")) == wuint64("0xAAAAAAAA96969696"));
	REQUIRE((0x3C3C3C3C ^ wuint96("0xAAAAAAAAAAAAAAAAAAAAAAAA")) == wuint96("0xAAAAAAAAAAAAAAAA96969696"));
}

TEST_CASE("wuint is_zero", "[wuint]") {
	REQUIRE(!wuint32("1").is_zero());
	REQUIRE(!wuint64("1").is_zero());
	REQUIRE(!wuint96("1").is_zero());

	REQUIRE(!wuint32("0x80000000").is_zero());
	REQUIRE(!wuint64("0x8000000000000000").is_zero());
	REQUIRE(!wuint96("0x800000000000000000000000").is_zero());

	REQUIRE(wuint32("0").is_zero());
	REQUIRE(wuint64("0").is_zero());
	REQUIRE(wuint96("0").is_zero());
}

TEST_CASE("wuint is_negative", "[wuint]") {
	REQUIRE(!wuint32("0").is_negative());
	REQUIRE(!wuint64("0").is_negative());
	REQUIRE(!wuint96("0").is_negative());

	REQUIRE(!wuint32("-1").is_negative());
	REQUIRE(!wuint64("-1").is_negative());
	REQUIRE(!wuint96("-1").is_negative());

	REQUIRE(!wuint32("0x7FFFFFFF").is_negative());
	REQUIRE(!wuint64("0x7FFFFFFFFFFFFFFF").is_negative());
	REQUIRE(!wuint96("0x7FFFFFFFFFFFFFFFFFFFFFFF").is_negative());

	REQUIRE(!wuint32("0x80000000").is_negative());
	REQUIRE(!wuint64("0x8000000000000000").is_negative());
	REQUIRE(!wuint96("0x800000000000000000000000").is_negative());
}

TEST_CASE("wuint getbit", "[wuint]") {
	REQUIRE(wuint96("0").getbit(0) == 0);
	REQUIRE(wuint96("1").getbit(0) == 1);
	REQUIRE(wuint96("2").getbit(0) == 0);

	REQUIRE(wuint96("0x800000000000").getbit(46) == 0);
	REQUIRE(wuint96("0x800000000000").getbit(47) == 1);
	REQUIRE(wuint96("0x800000000000").getbit(48) == 0);

	REQUIRE(wuint96("0x800000000000000000000000").getbit(94) == 0);
	REQUIRE(wuint96("0x800000000000000000000000").getbit(95) == 1);
}

TEST_CASE("wuint setbit", "[wuint]") {
	REQUIRE(wuint96("0").setbit(0) == wuint96("1"));
	REQUIRE(wuint96("0").setbit(1) == wuint96("2"));
	REQUIRE(wuint96("1").setbit(0) == wuint96("1"));
	REQUIRE(wuint96("2").setbit(0) == wuint96("3"));

	REQUIRE(wuint96("0").setbit(47) == wuint96("0x800000000000"));

	REQUIRE(wuint96("0").setbit(95) == wuint96("0x800000000000000000000000"));
}

TEST_CASE("wuint abs", "[wuint]") {
	REQUIRE(abs(wuint96("0")) == wuint96("0"));
	REQUIRE(abs(wuint96("1")) == wuint96("1"));
	REQUIRE(abs(wuint96("-1")) == wuint96("-1"));
	REQUIRE(abs(wuint96_7F) == wuint96_7F);
	REQUIRE(abs(wuint96_80) == wuint96_80);
	REQUIRE(abs(wuint96_81) == wuint96_81);
}

TEST_CASE("wuint from_chars 10", "[wuint]") {
	using record = std::pair<std::string, std::string>;

	auto data = GENERATE(
		record{"0", "0"},
		record{"1", "1"},
		record{"0001", "1"},
		record{"286335522", "286335522"},
		record{"3689367580026693222", "3689367580026693222"},
		record{"36973223102941133555797576908", "36973223102941133555797576908"},
		record{"39614081257132168796771975167", "39614081257132168796771975167"},
		record{"39614081257132168796771975168", "39614081257132168796771975168"},
		record{"39614081257132168796771975169", "39614081257132168796771975169"},
		record{"79228162514264337593543950335", "79228162514264337593543950335"}
	);

	const auto [value, expected] = data;

	wuint96 res(42);

	auto [ptr, ec] = from_chars(value.data(), value.data() + value.size(), res, 10);

	REQUIRE(ec == std::errc());
	REQUIRE(ptr == value.data() + value.size());
	REQUIRE(res == wuint96(expected));
}

TEST_CASE("wuint from_chars 10 end", "[wuint]") {
	auto str = GENERATE(as<std::string>{},
		"0abc",
		"1abc",
		"286335522abc",
		"79228162514264337593543950335abc"
	);

	wuint96 res(0);

	auto [ptr, ec] = from_chars(str.data(), str.data() + str.size(), res, 10);

	REQUIRE(ec == std::errc());
	REQUIRE(ptr == str.data() + str.size() - 3);
	REQUIRE(res == wuint96(str.substr(0, str.size() - 3)));
}

TEST_CASE("wuint from_chars 10 overflow", "[wuint]") {
	auto str = GENERATE(as<std::string>{},
		"79228162514264337593543950336",
		"10000000000000000000000000000000",
		"100000000000000000000000000000000000"
	);

	wuint96 res(42);

	auto [ptr, ec] = from_chars(str.data(), str.data() + str.size(), res, 10);

	REQUIRE(ec == std::errc::result_out_of_range);
	REQUIRE(ptr == str.data() + str.size());
	REQUIRE(res == 42);
}

TEST_CASE("wuint from_chars 10 invalid", "[wuint]") {
	auto str = GENERATE(as<std::string>{},
		"",
		"abc",
		"-1",
		"+1",
		" 1"
	);

	wuint96 res(42);

	auto [ptr, ec] = from_chars(str.data(), str.data() + str.size(), res, 10);

	REQUIRE(ec == std::errc::invalid_argument);
	REQUIRE(ptr == str.data());
	REQUIRE(res == 42);
}

TEST_CASE("wuint from_chars 16", "[wuint]") {
	using record = std::pair<std::string, std::string>;

	auto data = GENERATE(
		record{"0", "0"},
		record{"1", "1"},
		record{"11112222", "286335522"},
		record{"3333444455556666", "3689367580026693222"},
		record{"777788889999AaAaBbBbCcCc", "36973223102941133555797576908"},
		record{"7fffffffffffffffffffffff", "39614081257132168796771975167"},
		record{"800000000000000000000000", "39614081257132168796771975168"},
		record{"800000000000000000000001", "39614081257132168796771975169"},
		record{"FFFFFFFFFFFFFFFFFFFFFFFF", "79228162514264337593543950335"}
	);

	const auto [value, expected] = data;

	wuint96 res(0);

	auto [ptr, ec] = from_chars(value.data(), value.data() + value.size(), res, 16);

	REQUIRE(ec == std::errc());
	REQUIRE(ptr == value.data() + value.size());
	REQUIRE(res == wuint96(expected));
}

TEST_CASE("wuint from_chars 16 overflow", "[wuint]") {
	auto str = GENERATE(as<std::string>{},
		"1000000000000000000000000",
		"8000000000000000000000000000",
		"80000000000000000000000000000000"
	);

	wuint96 res(42);

	auto [ptr, ec] = from_chars(str.data(), str.data() + str.size(), res, 16);

	REQUIRE(ec == std::errc::result_out_of_range);
	REQUIRE(ptr == str.data() + str.size());
	REQUIRE(res == 42);
}

TEST_CASE("wuint from_chars 7", "[wuint]") {
	using record = std::pair<std::string, std::string>;

	auto data = GENERATE(
		record{"0", "0"},
		record{"1", "1"},
		record{"10044545304", "286335522"},
		record{"6414422622333331211340", "3689367580026693222"},
		record{"4532246320532121443535152360536011", "36973223102941133555797576908"},
		record{"5060360422412213131405631055526153", "39614081257132168796771975167"},
		record{"5060360422412213131405631055526154", "39614081257132168796771975168"},
		record{"5060360422412213131405631055526155", "39614081257132168796771975169"},
		record{"13151051145124426263114562144355340", "79228162514264337593543950335"}
	);

	const auto [value, expected] = data;

	wuint96 res(42);

	auto [ptr, ec] = from_chars(value.data(), value.data() + value.size(), res, 7);

	REQUIRE(ec == std::errc());
	REQUIRE(ptr == value.data() + value.size());
	REQUIRE(res == wuint96(expected));
}

TEST_CASE("wuint to_chars 10", "[wuint]") {
	using record = std::pair<std::string, std::string>;

	auto data = GENERATE(
		record{"0", "0"},
		record{"1", "1"},
		record{"286335522", "286335522"},
		record{"3689367580026693222", "3689367580026693222"},
		record{"36973223102941133555797576908", "36973223102941133555797576908"},
		record{"39614081257132168796771975167", "39614081257132168796771975167"},
		record{"39614081257132168796771975168", "39614081257132168796771975168"},
		record{"39614081257132168796771975169", "39614081257132168796771975169"},
		record{"79228162514264337593543950335", "79228162514264337593543950335"}
	);

	const auto [value, expected] = data;

	std::string res(expected.size(), '?');

	auto [ptr, ec] = to_chars(res.data(), res.data() + res.size(), wuint96(value), 10);

	REQUIRE(ec == std::errc());
	REQUIRE(ptr == res.data() + expected.size());
	REQUIRE(res == expected);
}

TEST_CASE("wuint to_chars 10 end", "[wuint]") {
	using record = std::pair<std::string, std::string>;

	auto data = GENERATE(
		record{"0", "0"},
		record{"1", "1"},
		record{"286335522", "286335522"},
		record{"3689367580026693222", "3689367580026693222"},
		record{"36973223102941133555797576908", "36973223102941133555797576908"},
		record{"39614081257132168796771975167", "39614081257132168796771975167"},
		record{"39614081257132168796771975168", "39614081257132168796771975168"},
		record{"39614081257132168796771975169", "39614081257132168796771975169"},
		record{"79228162514264337593543950335", "79228162514264337593543950335"}
	);

	const auto [value, expected] = data;

	std::string res(expected.size() + 1, '?');

	auto [ptr, ec] = to_chars(res.data(), res.data() + res.size(), wuint96(value), 10);

	REQUIRE(ec == std::errc());
	REQUIRE(ptr == res.data() + expected.size());
	REQUIRE(res.substr(0, expected.size()) == expected);
	REQUIRE(res[expected.size()] == '?');
}

TEST_CASE("wuint to_chars 10 size", "[wuint]") {
	using record = std::pair<std::string, std::string>;

	auto data = GENERATE(
		record{"0", "0"},
		record{"1", "1"},
		record{"286335522", "286335522"},
		record{"3689367580026693222", "3689367580026693222"},
		record{"36973223102941133555797576908", "36973223102941133555797576908"},
		record{"39614081257132168796771975167", "39614081257132168796771975167"},
		record{"39614081257132168796771975168", "39614081257132168796771975168"},
		record{"39614081257132168796771975169", "39614081257132168796771975169"},
		record{"79228162514264337593543950335", "79228162514264337593543950335"}
	);

	const auto [value, expected] = data;

	std::string res(expected.size() - 1, '?');

	auto [ptr, ec] = to_chars(res.data(), res.data() + res.size(), wuint96(value), 10);

	REQUIRE(ec == std::errc::value_too_large);
	REQUIRE(ptr == res.data() + res.size());
}

TEST_CASE("wuint to_chars 16", "[wuint]") {
	using record = std::pair<std::string, std::string>;

	auto data = GENERATE(
		record{"0", "0"},
		record{"1", "1"},
		record{"286335522", "11112222"},
		record{"3689367580026693222", "3333444455556666"},
		record{"36973223102941133555797576908", "777788889999aaaabbbbcccc"},
		record{"39614081257132168796771975167", "7fffffffffffffffffffffff"},
		record{"39614081257132168796771975168", "800000000000000000000000"},
		record{"39614081257132168796771975169", "800000000000000000000001"},
		record{"79228162514264337593543950335", "ffffffffffffffffffffffff"}
	);

	const auto [value, expected] = data;

	std::string res(expected.size(), '?');

	auto [ptr, ec] = to_chars(res.data(), res.data() + res.size(), wuint96(value), 16);

	REQUIRE(ec == std::errc());
	REQUIRE(ptr == res.data() + expected.size());
	REQUIRE(res == expected);
}

TEST_CASE("wuint to_chars 7", "[wuint]") {
	using record = std::pair<std::string, std::string>;

	auto data = GENERATE(
		record{"0", "0"},
		record{"1", "1"},
		record{"286335522", "10044545304"},
		record{"3689367580026693222", "6414422622333331211340"},
		record{"36973223102941133555797576908", "4532246320532121443535152360536011"},
		record{"39614081257132168796771975167", "5060360422412213131405631055526153"},
		record{"39614081257132168796771975168", "5060360422412213131405631055526154"},
		record{"39614081257132168796771975169", "5060360422412213131405631055526155"},
		record{"79228162514264337593543950335", "13151051145124426263114562144355340"}
	);

	const auto [value, expected] = data;

	std::string res(expected.size(), '?');

	auto [ptr, ec] = to_chars(res.data(), res.data() + res.size(), wuint96(value), 7);

	REQUIRE(ec == std::errc());
	REQUIRE(ptr == res.data() + expected.size());
	REQUIRE(res == expected);
}

TEST_CASE("wuint to_string", "[wuint]") {
	auto str = GENERATE(as<std::string>{},
		"0",
		"1",
		"286335522",
		"3689367580026693222",
		"36973223102941133555797576908",
		"39614081257132168796771975167",
		"39614081257132168796771975168",
		"79228162514264337593543950335"
	);

	const auto value = wuint96(str);

	REQUIRE(to_string(value) == str);
}

TEST_CASE("wuint to_string max digits", "[wuint]") {
	REQUIRE(to_string(wuint32_FF) == "4294967295");
}

TEST_CASE("wuint stream output", "[wuint]") {
	auto str = GENERATE(as<std::string>{},
		"0",
		"1",
		"286335522",
		"3689367580026693222",
		"36973223102941133555797576908",
		"79228162514264337593543950335"
	);

	std::ostringstream out;

	out << wuint96(str);

	REQUIRE(out.str() == str);
}

TEST_CASE("wuint stream output hex", "[wuint]") {
	using record = std::pair<std::string, std::string>;

	auto data = GENERATE(
		record{"0", "0"},
		record{"1", "1"},
		record{"286335522", "11112222"},
		record{"3689367580026693222", "3333444455556666"},
		record{"36973223102941133555797576908", "777788889999aaaabbbbcccc"},
		record{"39614081257132168796771975167", "7fffffffffffffffffffffff"},
		record{"39614081257132168796771975168", "800000000000000000000000"},
		record{"39614081257132168796771975169", "800000000000000000000001"},
		record{"79228162514264337593543950335", "ffffffffffffffffffffffff"}
	);

	const auto [value, expected] = data;

	std::ostringstream out;

	out << std::hex << wuint96(value);

	REQUIRE(out.str() == expected);
}

TEST_CASE("wuint stream output oct", "[wuint]") {
	using record = std::pair<std::string, std::string>;

	auto data = GENERATE(
		record{"0", "0"},
		record{"1", "1"},
		record{"286335522", "2104221042"},
		record{"3689367580026693222", "314632104212525263146"},
		record{"36973223102941133555797576908", "35673610421146315252527356746314"},
		record{"39614081257132168796771975167", "37777777777777777777777777777777"},
		record{"39614081257132168796771975168", "40000000000000000000000000000000"},
		record{"39614081257132168796771975169", "40000000000000000000000000000001"},
		record{"79228162514264337593543950335", "77777777777777777777777777777777"}
	);

	const auto [value, expected] = data;

	std::ostringstream out;

	out << std::oct << wuint96(value);

	REQUIRE(out.str() == expected);
}

TEST_CASE("wuint has_single_bit", "[wuint]") {
	REQUIRE(has_single_bit(wuint64("0x8000000000000000")));
	REQUIRE(has_single_bit(wuint64("0x4000000000000000")));
	REQUIRE(has_single_bit(wuint64("1")));
	REQUIRE(has_single_bit(wuint64("2")));
	REQUIRE(!has_single_bit(wuint64("0x8000000000000001")));
	REQUIRE(!has_single_bit(wuint64("0x9000000000000000")));
	REQUIRE(!has_single_bit(wuint64("3")));
	REQUIRE(!has_single_bit(wuint64("-1")));
}

TEST_CASE("wuint bit_ceil", "[wuint]") {
	REQUIRE(bit_ceil(wuint64("0")) == wuint64("1"));
	REQUIRE(bit_ceil(wuint64("1")) == wuint64("1"));
	REQUIRE(bit_ceil(wuint64("2")) == wuint64("2"));
	REQUIRE(bit_ceil(wuint64("3")) == wuint64("4"));
	REQUIRE(bit_ceil(wuint64("4")) == wuint64("4"));
	REQUIRE(bit_ceil(wuint64("5")) == wuint64("8"));
	REQUIRE(bit_ceil(wuint64("0x4000000000000000")) == wuint64("0x4000000000000000"));
	REQUIRE(bit_ceil(wuint64("0x4000000000000001")) == wuint64("0x8000000000000000"));
	REQUIRE(bit_ceil(wuint64("0x7FFFFFFFFFFFFFFF")) == wuint64("0x8000000000000000"));
}

TEST_CASE("wuint bit_floor", "[wuint]") {
	REQUIRE(bit_floor(wuint64("0")) == wuint64("0"));
	REQUIRE(bit_floor(wuint64("1")) == wuint64("1"));
	REQUIRE(bit_floor(wuint64("2")) == wuint64("2"));
	REQUIRE(bit_floor(wuint64("3")) == wuint64("2"));
	REQUIRE(bit_floor(wuint64("4")) == wuint64("4"));
	REQUIRE(bit_floor(wuint64("5")) == wuint64("4"));
	REQUIRE(bit_floor(wuint64("0x4000000000000000")) == wuint64("0x4000000000000000"));
	REQUIRE(bit_floor(wuint64("0x4000000000000001")) == wuint64("0x4000000000000000"));
	REQUIRE(bit_floor(wuint64("0x7FFFFFFFFFFFFFFF")) == wuint64("0x4000000000000000"));
	REQUIRE(bit_floor(wuint64("0x8000000000000000")) == wuint64("0x8000000000000000"));
	REQUIRE(bit_floor(wuint64("0x8FFFFFFFFFFFFFFF")) == wuint64("0x8000000000000000"));
	REQUIRE(bit_floor(wuint64("0xFFFFFFFFFFFFFFFF")) == wuint64("0x8000000000000000"));
}

TEST_CASE("wuint bit_width", "[wuint]") {
	REQUIRE(bit_width(wuint96("0")) == 0);
	REQUIRE(bit_width(wuint96("1")) == 1);
	REQUIRE(bit_width(wuint96("2")) == 2);
	REQUIRE(bit_width(wuint96("3")) == 2);
	REQUIRE(bit_width(wuint96("0x800000000000")) == 48);
	REQUIRE(bit_width(wuint96_7F) == 95);
	REQUIRE(bit_width(wuint96_80) == 96);
	REQUIRE(bit_width(wuint96_81) == 96);
	REQUIRE(bit_width(wuint96_FF) == 96);
}

TEST_CASE("wuint countl_zero", "[wuint]") {
	REQUIRE(countl_zero(wuint64("0")) == 64);
	REQUIRE(countl_zero(wuint64("1")) == 63);
	REQUIRE(countl_zero(wuint64("2")) == 62);
	REQUIRE(countl_zero(wuint64("0x0000000080000000")) == 32);
	REQUIRE(countl_zero(wuint64("0x4000000000000000")) == 1);
	REQUIRE(countl_zero(wuint64("0x7FFFFFFFFFFFFFFF")) == 1);
	REQUIRE(countl_zero(wuint64("0x8000000000000000")) == 0);
	REQUIRE(countl_zero(wuint64("0xFFFFFFFFFFFFFFFF")) == 0);
}

TEST_CASE("wuint countl_one", "[wuint]") {
	REQUIRE(countl_one(wuint64("0")) == 0);
	REQUIRE(countl_one(wuint64("1")) == 0);
	REQUIRE(countl_one(wuint64("2")) == 0);
	REQUIRE(countl_one(wuint64("0x0000000080000000")) == 0);
	REQUIRE(countl_one(wuint64("0x4000000000000000")) == 0);
	REQUIRE(countl_one(wuint64("0x7FFFFFFFFFFFFFFF")) == 0);
	REQUIRE(countl_one(wuint64("0x8000000000000000")) == 1);
	REQUIRE(countl_one(wuint64("0xBFFFFFFFFFFFFFFF")) == 1);
	REQUIRE(countl_one(wuint64("0xC000000000000000")) == 2);
	REQUIRE(countl_one(wuint64("0xFFFFFFFFFFFFFFFE")) == 63);
	REQUIRE(countl_one(wuint64("0xFFFFFFFFFFFFFFFF")) == 64);
}

TEST_CASE("wuint countr_zero", "[wuint]") {
	REQUIRE(countr_zero(wuint64("0")) == 64);
	REQUIRE(countr_zero(wuint64("1")) == 0);
	REQUIRE(countr_zero(wuint64("2")) == 1);
	REQUIRE(countr_zero(wuint64("0x0000000080000000")) == 31);
	REQUIRE(countr_zero(wuint64("0x4000000000000000")) == 62);
	REQUIRE(countr_zero(wuint64("0x7FFFFFFFFFFFFFFF")) == 0);
	REQUIRE(countr_zero(wuint64("0x8000000000000000")) == 63);
	REQUIRE(countr_zero(wuint64("0xFFFFFFFFFFFFFFFF")) == 0);
}

TEST_CASE("wuint countr_one", "[wuint]") {
	REQUIRE(countr_one(wuint64("0")) == 0);
	REQUIRE(countr_one(wuint64("1")) == 1);
	REQUIRE(countr_one(wuint64("2")) == 0);
	REQUIRE(countr_one(wuint64("0x0000000080000000")) == 0);
	REQUIRE(countr_one(wuint64("0x4000000000000000")) == 0);
	REQUIRE(countr_one(wuint64("0x7FFFFFFFFFFFFFFF")) == 63);
	REQUIRE(countr_one(wuint64("0x8000000000000000")) == 0);
	REQUIRE(countr_one(wuint64("0xBFFFFFFFFFFFFFFF")) == 62);
	REQUIRE(countr_one(wuint64("0xC000000000000000")) == 0);
	REQUIRE(countr_one(wuint64("0xFFFFFFFFFFFFFFFE")) == 0);
	REQUIRE(countr_one(wuint64("0xFFFFFFFFFFFFFFFF")) == 64);
}

TEST_CASE("wuint popcount", "[wuint]") {
	REQUIRE(popcount(wuint64("0")) == 0);
	REQUIRE(popcount(wuint64("1")) == 1);
	REQUIRE(popcount(wuint64("2")) == 1);
	REQUIRE(popcount(wuint64("0x0000000080000000")) == 1);
	REQUIRE(popcount(wuint64("0x4000000000000000")) == 1);
	REQUIRE(popcount(wuint64("0x7FFFFFFFFFFFFFFF")) == 63);
	REQUIRE(popcount(wuint64("0x8000000000000000")) == 1);
	REQUIRE(popcount(wuint64("0x5555555555555555")) == 32);
	REQUIRE(popcount(wuint64("0xAAAAAAAAAAAAAAAA")) == 32);
	REQUIRE(popcount(wuint64("0xBFFFFFFFFFFFFFFF")) == 63);
	REQUIRE(popcount(wuint64("0xC000000000000000")) == 2);
	REQUIRE(popcount(wuint64("0xFFFFFFFFFFFFFFFE")) == 63);
	REQUIRE(popcount(wuint64("0xFFFFFFFFFFFFFFFF")) == 64);
}

TEST_CASE("wuint gcd", "[wuint]") {
	static constexpr wuint128 x("9223372036854775399");
	static constexpr wuint128 y("4611686018427387787");
	static constexpr wuint128 r("2305843009213693613");

	REQUIRE(gcd(wuint64("0"), wuint64("0")) == 0);
	REQUIRE(gcd(wuint64("0"), wuint64("1")) == 1);
	REQUIRE(gcd(wuint64("1"), wuint64("0")) == 1);
	REQUIRE(gcd(wuint64("1"), wuint64("1")) == 1);
	REQUIRE(gcd(wuint64("48"), wuint64("18")) == 6);
	REQUIRE(gcd(x, y) == 1);
	REQUIRE(gcd(x * r, y * r) == r);
}

TEST_CASE("wuint lcm", "[wuint]") {
	static constexpr wuint128 x("288230376151711607");
	static constexpr wuint128 y("144115188075855509");

	REQUIRE(lcm(wuint64("0"), wuint64("0")) == 0);
	REQUIRE(lcm(wuint64("0"), wuint64("1")) == 0);
	REQUIRE(lcm(wuint64("1"), wuint64("0")) == 0);
	REQUIRE(lcm(wuint64("1"), wuint64("1")) == 1);
	REQUIRE(lcm(wuint64("21"), wuint64("6")) == 42);
	REQUIRE(lcm(x, x) == x);
	REQUIRE(lcm(x , y) == x * y);
	REQUIRE(lcm(2 * 3 * x, 3 * 5 * y) == 2 * 3 * 5 * x * y);
}

TEST_CASE("wuint sqrt", "[wuint]") {
	static constexpr wuint128 x("576460752303422881");

	REQUIRE(sqrt(wuint64(0)) == 0);
	REQUIRE(sqrt(wuint64(1)) == 1);
	REQUIRE(sqrt(wuint64(2)) == 1);
	REQUIRE(sqrt(wuint64(3)) == 1);
	REQUIRE(sqrt(wuint64(4)) == 2);
	REQUIRE(sqrt(wuint64(8)) == 2);
	REQUIRE(sqrt(wuint64(9)) == 3);
	REQUIRE(sqrt(x * x) == x);
	REQUIRE(sqrt(x * x + 1) == x);
	REQUIRE(sqrt(x * x - 1) == x - 1);
}

TEST_CASE("std::hash<wuint>", "[wuint]") {
	REQUIRE(std::hash<wuint32>()(wuint32("123")) == std::hash<wuint32>()(wuint32("123")));
	REQUIRE(std::hash<wuint64>()(wuint64("123")) == std::hash<wuint64>()(wuint64("123")));
	REQUIRE(std::hash<wuint96>()(wuint96("123")) == std::hash<wuint96>()(wuint96("123")));

	REQUIRE(std::hash<wuint32>()(wuint32("123")) != std::hash<wuint32>()(wuint32("456")));
	REQUIRE(std::hash<wuint64>()(wuint64("123")) != std::hash<wuint64>()(wuint64("456")));
	REQUIRE(std::hash<wuint96>()(wuint96("123")) != std::hash<wuint96>()(wuint96("456")));

	std::unordered_set<wuint64> set;

	set.insert(wuint64("0"));
	set.insert(wuint64("1"));
	set.insert(wuint64("-1"));
	set.insert(wuint64("0x8000000000000000"));

	REQUIRE(set.size() == 4);
}

TEST_CASE("wuint factorial", "[wuint]") {
	REQUIRE(fac(50) == wuint256("30414093201713378043612608166064768844377641568960512000000000000"));

	REQUIRE(fac(34) / fac(30) == 34 * 33 * 32 * 31);
}

TEST_CASE("wuint modinv", "[wuint]") {
	auto str = GENERATE(as<std::string>{},
		"1",
		"2",
		"286335522",
		"3689367580026693222",
		"9223372036854775336"
	);

	static constexpr wuint128 n("9223372036854775337");

	wuint128 value(str);

	wuint128 value_inv = modinv(value, n);

	REQUIRE(value_inv > 0);
	REQUIRE(value_inv < n);
	REQUIRE((value * value_inv) % n == 1);
}

TEST_CASE("wuint crypt", "[wuint]") {
	static constexpr wuint256 p("9223372036854775337");
	static constexpr wuint256 q("4611686018427387847");

	wuint256 n = p * q;

	wuint256 e(65537);

	wuint256 phi = (p - 1) * (q - 1);

	wuint256 d = modinv(e, phi);

	wuint256 message(42);

	wuint256 ciphertext = modexp(message, e, n);

	wuint256 plaintext = modexp(ciphertext, d, n);

	REQUIRE(plaintext == message);
}
