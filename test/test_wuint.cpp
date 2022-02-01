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
	if (x == 1) {
		return wuint<width>(0);
	}

	const wuint<width> base(a % n);
	wuint<width> res(1);

	for (std::size_t bit_i = x.log2(); bit_i--; ) {
		res = (res * res) % n;

		if (x.getbit(static_cast<unsigned int>(bit_i))) {
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
	constexpr auto wuint32_7F = wuint32::max() / wuint32("2");
	constexpr auto wuint64_7F = wuint64::max() / wuint64("2");
	constexpr auto wuint96_7F = wuint96::max() / wuint96("2");
	constexpr auto wuint32_80 = wuint32_7F + wuint32("1");
	constexpr auto wuint64_80 = wuint64_7F + wuint64("1");
	constexpr auto wuint96_80 = wuint96_7F + wuint96("1");
	constexpr auto wuint32_81 = wuint32_80 + wuint32("1");
	constexpr auto wuint64_81 = wuint64_80 + wuint64("1");
	constexpr auto wuint96_81 = wuint96_80 + wuint96("1");
	constexpr auto wuint32_FF = wuint32::max();
	constexpr auto wuint64_FF = wuint64::max();
	constexpr auto wuint96_FF = wuint96::max();

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
	constexpr auto wuint32_7F = wuint32::max() / wuint32("2");
	constexpr auto wuint64_7F = wuint64::max() / wuint64("2");
	constexpr auto wuint96_7F = wuint96::max() / wuint96("2");
	constexpr auto wuint32_80 = wuint32_7F + wuint32("1");
	constexpr auto wuint64_80 = wuint64_7F + wuint64("1");
	constexpr auto wuint96_80 = wuint96_7F + wuint96("1");
	constexpr auto wuint32_81 = wuint32_80 + wuint32("1");
	constexpr auto wuint64_81 = wuint64_80 + wuint64("1");
	constexpr auto wuint96_81 = wuint96_80 + wuint96("1");
	constexpr auto wuint32_FF = wuint32::max();
	constexpr auto wuint64_FF = wuint64::max();
	constexpr auto wuint96_FF = wuint96::max();

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
	constexpr auto wuint32_7F = wuint32::max() / wuint32("2");
	constexpr auto wuint64_7F = wuint64::max() / wuint64("2");
	constexpr auto wuint96_7F = wuint96::max() / wuint96("2");
	constexpr auto wuint32_80 = wuint32_7F + wuint32("1");
	constexpr auto wuint64_80 = wuint64_7F + wuint64("1");
	constexpr auto wuint96_80 = wuint96_7F + wuint96("1");
	constexpr auto wuint32_81 = wuint32_80 + wuint32("1");
	constexpr auto wuint64_81 = wuint64_80 + wuint64("1");
	constexpr auto wuint96_81 = wuint96_80 + wuint96("1");
	constexpr auto wuint32_FF = wuint32::max();
	constexpr auto wuint64_FF = wuint64::max();
	constexpr auto wuint96_FF = wuint96::max();

	std::uint32_t uint32_80 = std::numeric_limits<std::int32_t>::min();
	std::uint32_t uint32_81 = -std::numeric_limits<std::int32_t>::max();
	std::uint32_t uint32_FF = -1;
	std::uint32_t uint32_01 = 1;
	std::uint32_t uint32_7F = std::numeric_limits<std::int32_t>::max();

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
	constexpr auto wuint32_7F = wuint32::max() / wuint32("2");
	constexpr auto wuint64_7F = wuint64::max() / wuint64("2");
	constexpr auto wuint96_7F = wuint96::max() / wuint96("2");
	constexpr auto wuint32_80 = wuint32_7F + wuint32("1");
	constexpr auto wuint64_80 = wuint64_7F + wuint64("1");
	constexpr auto wuint96_80 = wuint96_7F + wuint96("1");
	constexpr auto wuint32_81 = wuint32_80 + wuint32("1");
	constexpr auto wuint64_81 = wuint64_80 + wuint64("1");
	constexpr auto wuint96_81 = wuint96_80 + wuint96("1");
	constexpr auto wuint32_FF = wuint32::max();
	constexpr auto wuint64_FF = wuint64::max();
	constexpr auto wuint96_FF = wuint96::max();

	std::uint32_t uint32_80 = std::numeric_limits<std::int32_t>::min();
	std::uint32_t uint32_81 = -std::numeric_limits<std::int32_t>::max();
	std::uint32_t uint32_FF = -1;
	std::uint32_t uint32_01 = 1;
	std::uint32_t uint32_7F = std::numeric_limits<std::int32_t>::max();

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
	constexpr auto wuint32_7F = wuint32::max() / wuint32("2");
	constexpr auto wuint64_7F = wuint64::max() / wuint64("2");
	constexpr auto wuint96_7F = wuint96::max() / wuint96("2");
	constexpr auto wuint32_80 = wuint32_7F + wuint32("1");
	constexpr auto wuint64_80 = wuint64_7F + wuint64("1");
	constexpr auto wuint96_80 = wuint96_7F + wuint96("1");
	constexpr auto wuint32_81 = wuint32_80 + wuint32("1");
	constexpr auto wuint64_81 = wuint64_80 + wuint64("1");
	constexpr auto wuint96_81 = wuint96_80 + wuint96("1");
	constexpr auto wuint32_FF = wuint32::max();
	constexpr auto wuint64_FF = wuint64::max();
	constexpr auto wuint96_FF = wuint96::max();

	std::uint32_t uint32_80 = std::numeric_limits<std::int32_t>::min();
	std::uint32_t uint32_81 = -std::numeric_limits<std::int32_t>::max();
	std::uint32_t uint32_FF = -1;
	std::uint32_t uint32_01 = 1;
	std::uint32_t uint32_7F = std::numeric_limits<std::int32_t>::max();

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
	constexpr auto wuint32_7F = wuint32::max() / wuint32("2");
	constexpr auto wuint64_7F = wuint64::max() / wuint64("2");
	constexpr auto wuint96_7F = wuint96::max() / wuint96("2");
	constexpr auto wuint32_80 = wuint32_7F + wuint32("1");
	constexpr auto wuint64_80 = wuint64_7F + wuint64("1");
	constexpr auto wuint96_80 = wuint96_7F + wuint96("1");
	constexpr auto wuint32_81 = wuint32_80 + wuint32("1");
	constexpr auto wuint64_81 = wuint64_80 + wuint64("1");
	constexpr auto wuint96_81 = wuint96_80 + wuint96("1");
	constexpr auto wuint32_FF = wuint32::max();
	constexpr auto wuint64_FF = wuint64::max();
	constexpr auto wuint96_FF = wuint96::max();

	std::uint32_t uint32_80 = std::numeric_limits<std::int32_t>::min();
	std::uint32_t uint32_81 = -std::numeric_limits<std::int32_t>::max();
	std::uint32_t uint32_FF = -1;
	std::uint32_t uint32_01 = 1;
	std::uint32_t uint32_7F = std::numeric_limits<std::int32_t>::max();

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

TEST_CASE("wuint log2", "[wuint]") {
	REQUIRE(wuint96("0").log2() == 0);
	REQUIRE(wuint96("1").log2() == 1);
	REQUIRE(wuint96("2").log2() == 2);
	REQUIRE(wuint96("3").log2() == 2);
	REQUIRE(wuint96("0x800000000000").log2() == 48);
	REQUIRE(wuint96("-1").log2() == 96);
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
	REQUIRE(abs(wuint96("0x7FFFFFFFFFFFFFFFFFFFFFFF")) == wuint96("0x7FFFFFFFFFFFFFFFFFFFFFFF"));
	REQUIRE(abs(wuint96("0x800000000000000000000000")) == wuint96("0x800000000000000000000000"));
	REQUIRE(abs(wuint96("0x800000000000000000000001")) == wuint96("0x800000000000000000000001"));
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

TEST_CASE("wuint stream output", "[wuint]") {
	auto str = GENERATE(as<std::string>{},
		"0",
		"1",
		"286335522",
		"3689367580026693222",
		"36973223102941133555797576908",
		"79228162514264337593543950335"
	);

	const auto value = wuint96(str);

	SECTION("output value") {
		std::ostringstream out;

		out << value;

		REQUIRE(out.str() == str);
	}
}

TEST_CASE("wuint stream input", "[wuint]") {
	auto str = GENERATE(as<std::string>{},
		"0",
		"1",
		"-1",
		"36973223102941133555797576908",
		"-36973223102941133555797576908",
		"79228162514264337593543950335"
	);

	std::istringstream in(str);

	SECTION("input value") {
		wuint96 value(42);

		REQUIRE(in >> value);

		REQUIRE(value == wuint96(str));
	}
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

	constexpr wuint128 n("9223372036854775337");

	wuint128 value(str);

	wuint128 value_inv = modinv(value, n);

	REQUIRE(value_inv > 0);
	REQUIRE(value_inv < n);
	REQUIRE((value * value_inv) % n == 1);
}

TEST_CASE("wuint crypt", "[wuint]") {
	constexpr wuint256 p("9223372036854775337");
	constexpr wuint256 q("4611686018427387847");

	wuint256 n = p * q;

	wuint256 e(65537);

	wuint256 phi = (p - 1) * (q - 1);

	wuint256 d = modinv(e, phi);

	wuint256 message(42);

	wuint256 ciphertext = modexp(message, e, n);

	wuint256 plaintext = modexp(ciphertext, d, n);

	REQUIRE(plaintext == message);
}
