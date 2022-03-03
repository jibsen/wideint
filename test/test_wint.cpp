//
// test_wint - wideint unit test
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

using wint32 = wint<1>;
using wint64 = wint<2>;
using wint96 = wint<3>;
using wint128 = wint<4>;
using wint256 = wint<8>;

static constexpr auto wint32_80 = wint32::min();
static constexpr auto wint64_80 = wint64::min();
static constexpr auto wint96_80 = wint96::min();
static constexpr auto wint32_81 = -wint32::max();
static constexpr auto wint64_81 = -wint64::max();
static constexpr auto wint96_81 = -wint96::max();
static constexpr auto wint32_FF = wint32(-1);
static constexpr auto wint64_FF = wint64(-1);
static constexpr auto wint96_FF = wint96(-1);
static constexpr auto wint32_01 = wint32(1);
static constexpr auto wint64_01 = wint64(1);
static constexpr auto wint96_01 = wint96(1);
static constexpr auto wint32_7F = wint32::max();
static constexpr auto wint64_7F = wint64::max();
static constexpr auto wint96_7F = wint96::max();

constexpr std::int32_t int32_80 = std::numeric_limits<std::int32_t>::min();
constexpr std::int32_t int32_81 = -std::numeric_limits<std::int32_t>::max();
constexpr std::int32_t int32_FF = -1;
constexpr std::int32_t int32_01 = 1;
constexpr std::int32_t int32_7F = std::numeric_limits<std::int32_t>::max();

constexpr wint256 fac(std::int32_t n)
{
	wint256 res(n);

	for (std::int32_t i = 2; i < n; ++i) {
		res *= i;
	}

	return res;
}

template<std::size_t width>
constexpr wint<width> modinv(const wint<width> &a, const wint<width> &n)
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
		return wint<width>(0);
	}

	if (t.is_negative()) {
		t += n;
	}

	return t;
}

template<std::size_t width>
constexpr wint<width> modexp(const wint<width> &a, const wint<width> &x, const wint<width> &n)
{
	const wint<width> base(a % n);
	wint<width> res(1);

	for (std::size_t bit_i = bit_width(wuint<width>(x)); bit_i--; ) {
		res = (res * res) % n;

		if (x.getbit(bit_i)) {
			res = (res * base) % n;
		}
	}

	if (res.is_negative()) {
		res += n;
	}

	return res;
}

TEST_CASE("initialize wint from string", "[wint]") {
	constexpr wint32 zero32("0");
	constexpr wint64 zero64("0");
	constexpr wint96 zero96("0");

	REQUIRE(zero32.cells[0] == 0);
	REQUIRE(zero64.cells[0] == 0);
	REQUIRE(zero64.cells[1] == 0);
	REQUIRE(zero96.cells[0] == 0);
	REQUIRE(zero96.cells[1] == 0);
	REQUIRE(zero96.cells[2] == 0);

	constexpr wint32 one32("1");
	constexpr wint64 one64("1");
	constexpr wint96 one96("1");

	REQUIRE(one32.cells[0] == 1);
	REQUIRE(one64.cells[0] == 1);
	REQUIRE(one64.cells[1] == 0);
	REQUIRE(one96.cells[0] == 1);
	REQUIRE(one96.cells[1] == 0);
	REQUIRE(one96.cells[2] == 0);

	constexpr wint32 n_one32("-1");
	constexpr wint64 n_one64("-1");
	constexpr wint96 n_one96("-1");

	REQUIRE(n_one32.cells[0] == 0xFFFFFFFF);
	REQUIRE(n_one64.cells[0] == 0xFFFFFFFF);
	REQUIRE(n_one64.cells[1] == 0xFFFFFFFF);
	REQUIRE(n_one96.cells[0] == 0xFFFFFFFF);
	REQUIRE(n_one96.cells[1] == 0xFFFFFFFF);
	REQUIRE(n_one96.cells[2] == 0xFFFFFFFF);

	constexpr wint32 dec32("286335522");
	constexpr wint64 dec64("3689367580026693222");
	constexpr wint96 dec96("36973223102941133555797576908");

	REQUIRE(dec32.cells[0] == 0x11112222);
	REQUIRE(dec64.cells[0] == 0x55556666);
	REQUIRE(dec64.cells[1] == 0x33334444);
	REQUIRE(dec96.cells[0] == 0xBBBBCCCC);
	REQUIRE(dec96.cells[1] == 0x9999AAAA);
	REQUIRE(dec96.cells[2] == 0x77778888);

	constexpr wint32 n_dec32("-286335522");
	constexpr wint64 n_dec64("-3689367580026693222");
	constexpr wint96 n_dec96("-36973223102941133555797576908");

	REQUIRE(n_dec32.cells[0] == 0xEEEEDDDE);
	REQUIRE(n_dec64.cells[0] == 0xAAAA999A);
	REQUIRE(n_dec64.cells[1] == 0xCCCCBBBB);
	REQUIRE(n_dec96.cells[0] == 0x44443334);
	REQUIRE(n_dec96.cells[1] == 0x66665555);
	REQUIRE(n_dec96.cells[2] == 0x88887777);

	constexpr wint32 hex32("0x11112222");
	constexpr wint64 hex64("0x3333444455556666");
	constexpr wint96 hex96("0x777788889999AAAABBBBCCCC");

	REQUIRE(hex32.cells[0] == 0x11112222);
	REQUIRE(hex64.cells[0] == 0x55556666);
	REQUIRE(hex64.cells[1] == 0x33334444);
	REQUIRE(hex96.cells[0] == 0xBBBBCCCC);
	REQUIRE(hex96.cells[1] == 0x9999AAAA);
	REQUIRE(hex96.cells[2] == 0x77778888);

	constexpr wint32 n_hex32("-0x11112222");
	constexpr wint64 n_hex64("-0x3333444455556666");
	constexpr wint96 n_hex96("-0x777788889999AAAABBBBCCCC");

	REQUIRE(n_hex32.cells[0] == 0xEEEEDDDE);
	REQUIRE(n_hex64.cells[0] == 0xAAAA999A);
	REQUIRE(n_hex64.cells[1] == 0xCCCCBBBB);
	REQUIRE(n_hex96.cells[0] == 0x44443334);
	REQUIRE(n_hex96.cells[1] == 0x66665555);
	REQUIRE(n_hex96.cells[2] == 0x88887777);
}

TEST_CASE("wint int32_t equality", "[wint]") {
	REQUIRE(wint32("0") == 0);
	REQUIRE(wint32("1") == 1);
	REQUIRE(wint32("-1") == -1);
	REQUIRE(wint32("286335522") == 286335522);
	REQUIRE(wint32("-286335522") == -286335522);

	REQUIRE(wint32("0") != 1);
	REQUIRE(wint32("1") != 0);
	REQUIRE(wint32("-1") != 0);

	REQUIRE(wint64("0") == 0);
	REQUIRE(wint64("1") == 1);
	REQUIRE(wint64("286335522") == 286335522);

	REQUIRE(wint64("-1") == -1);
	REQUIRE(wint64("-1").cells[0] == -1);
	REQUIRE(wint64("-1").cells[1] == 0xFFFFFFFF);

	REQUIRE(wint64("-286335522") == -286335522);
	REQUIRE(wint64("-286335522").cells[0] == -286335522);
	REQUIRE(wint64("-286335522").cells[1] == 0xFFFFFFFF);

	REQUIRE(wint96("0") == 0);
	REQUIRE(wint96("1") == 1);
	REQUIRE(wint96("286335522") == 286335522);

	REQUIRE(wint96("-1") == -1);
	REQUIRE(wint96("-1").cells[0] == -1);
	REQUIRE(wint96("-1").cells[1] == 0xFFFFFFFF);
	REQUIRE(wint96("-1").cells[2] == 0xFFFFFFFF);

	REQUIRE(wint96("-286335522") == -286335522);
	REQUIRE(wint96("-286335522").cells[0] == -286335522);
	REQUIRE(wint96("-286335522").cells[1] == 0xFFFFFFFF);
	REQUIRE(wint96("-286335522").cells[2] == 0xFFFFFFFF);
}

TEST_CASE("wint wint less than", "[wint]") {
	REQUIRE(wint64::min() < -wint64::max());
	REQUIRE(-wint64::max() < wint64("-1"));
	REQUIRE(wint64("-1") < wint64("0"));
	REQUIRE(wint64("0") < wint64("1"));
	REQUIRE(wint64("1") < wint64::max());
}

TEST_CASE("wint int32_t less than", "[wint]") {
	REQUIRE(wint64::min() < -std::numeric_limits<std::int32_t>::max());
	REQUIRE(-wint64::max() < -1);
	REQUIRE(wint64("-1") < 0);
	REQUIRE(wint64("0") < 1);
	REQUIRE(wint64("1") < std::numeric_limits<std::int32_t>::max());
	REQUIRE(std::numeric_limits<std::int32_t>::max() < wint64::max());
}

TEST_CASE("assign from int32_t", "[wint]") {
	auto int32 = GENERATE(as<std::int32_t>{},
		std::numeric_limits<std::int32_t>::min(),
		std::numeric_limits<std::int32_t>::min() + 1,
		-1,
		0,
		1,
		std::numeric_limits<std::int32_t>::max()
	);

	wint32 value_32("0x11112222");
	wint64 value_64("0x3333444455556666");
	wint96 value_96("0x777788889999AAAABBBBCCCC");

	value_32 = int32;
	value_64 = int32;
	value_96 = int32;

	REQUIRE(value_32 == int32);
	REQUIRE(value_64 == int32);
	REQUIRE(value_96 == int32);
}

TEST_CASE("wint bitwise complement", "[wint]") {
	REQUIRE(~wint32("0") == wint32("0xFFFFFFFF"));
	REQUIRE(~wint64("0") == wint64("0xFFFFFFFFFFFFFFFF"));
	REQUIRE(~wint96("0") == wint96("0xFFFFFFFFFFFFFFFFFFFFFFFF"));

	REQUIRE(~wint32("0xFFFFFFFF") == wint32("0"));
	REQUIRE(~wint64("0xFFFFFFFFFFFFFFFF") == wint64("0"));
	REQUIRE(~wint96("0xFFFFFFFFFFFFFFFFFFFFFFFF") == wint96("0"));

	REQUIRE(~wint32("0xF0F0F0F0") == wint32("0x0F0F0F0F"));
	REQUIRE(~wint64("0xF0F0F0F0F0F0F0F0") == wint64("0x0F0F0F0F0F0F0F0F"));
	REQUIRE(~wint96("0xF0F0F0F0F0F0F0F0F0F0F0F0") == wint96("0x0F0F0F0F0F0F0F0F0F0F0F0F"));
}

TEST_CASE("wint unary minus", "[wint]") {
	REQUIRE(-wint32("0") == wint32("0"));
	REQUIRE(-wint64("0") == wint64("0"));
	REQUIRE(-wint96("0") == wint96("0"));

	REQUIRE(-wint32("1") == wint32("0xFFFFFFFF"));
	REQUIRE(-wint64("1") == wint64("0xFFFFFFFFFFFFFFFF"));
	REQUIRE(-wint96("1") == wint96("0xFFFFFFFFFFFFFFFFFFFFFFFF"));

	REQUIRE(-wint32("0x01234567") == wint32("-0x01234567"));
	REQUIRE(-wint64("0x0123456712345678") == wint64("-0x0123456712345678"));
	REQUIRE(-wint96("0x012345671234567823456789") == wint96("-0x012345671234567823456789"));

	REQUIRE(-wint32("0x81234567") == wint32("-0x81234567"));
	REQUIRE(-wint64("0x8123456712345678") == wint64("-0x8123456712345678"));
	REQUIRE(-wint96("0x812345671234567823456789") == wint96("-0x812345671234567823456789"));

	REQUIRE(-wint32("0x7FFFFFFF") == wint32("0x80000001"));
	REQUIRE(-wint64("0x7FFFFFFFFFFFFFFF") == wint64("0x8000000000000001"));
	REQUIRE(-wint96("0x7FFFFFFFFFFFFFFFFFFFFFFF") == wint96("0x800000000000000000000001"));

	REQUIRE(-wint32("0x80000000") == wint32("0x80000000"));
	REQUIRE(-wint64("0x8000000000000000") == wint64("0x8000000000000000"));
	REQUIRE(-wint96("0x800000000000000000000000") == wint96("0x800000000000000000000000"));
}

TEST_CASE("wint increment/decrement 32", "[wint]") {
	auto str = GENERATE(as<std::string>{},
		"0x00000001", "0x7FFFFFFF", "0x80000000", "0x80000001", "0xFFFFFFFF"
	);

	const auto org_var = wint32(str);
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

TEST_CASE("wint increment/decrement 96", "[wint]") {
	auto str = GENERATE(as<std::string>{},
		"0x000000000000000000000001",
		"0x7FFFFFFFFFFFFFFFFFFFFFFF",
		"0x800000000000000000000000",
		"0x800000000000000000000001",
		"0xFFFFFFFFFFFFFFFFFFFFFFFF"
	);

	const auto org_var = wint96(str);
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

TEST_CASE("wint left shift", "[wint]") {
	REQUIRE((wint32("0x01234567") << 4) == wint32("0x12345670"));
	REQUIRE((wint64("0x0123456712345678") << 4) == wint64("0x1234567123456780"));
	REQUIRE((wint96("0x012345671234567823456789") << 4) == wint96("0x123456712345678234567890"));

	REQUIRE((wint64("0x0123456712345678") << 32) == wint64("0x1234567800000000"));
	REQUIRE((wint96("0x012345671234567823456789") << 32) == wint96("0x123456782345678900000000"));
	REQUIRE((wint64("0x0123456712345678") << 36) == wint64("0x2345678000000000"));
	REQUIRE((wint96("0x012345671234567823456789") << 36) == wint96("0x234567823456789000000000"));

	REQUIRE((wint96("0x012345671234567823456789") << 64) == wint96("0x234567890000000000000000"));
	REQUIRE((wint96("0x012345671234567823456789") << 68) == wint96("0x345678900000000000000000"));

	REQUIRE((wint256("0x0123456712345678234567893456789A456789AB56789ABC6789ABCD789ABCDE") << 224) == wint256("0x789ABCDE00000000000000000000000000000000000000000000000000000000"));
	REQUIRE((wint256("0x0123456712345678234567893456789A456789AB56789ABC6789ABCD789ABCDE") << 228) == wint256("0x89ABCDE000000000000000000000000000000000000000000000000000000000"));
}

TEST_CASE("wint right shift", "[wint]") {
	REQUIRE((wint32("0x01234567") >> 4) == wint32("0x00123456"));
	REQUIRE((wint64("0x0123456712345678") >> 4) == wint64("0x0012345671234567"));
	REQUIRE((wint96("0x012345671234567823456789") >> 4) == wint96("0x001234567123456782345678"));

	REQUIRE((wint64("0x0123456712345678") >> 32) == wint64("0x01234567"));
	REQUIRE((wint96("0x012345671234567823456789") >> 32) == wint96("0x0123456712345678"));

	REQUIRE((wint64("0x0123456712345678") >> 36) == wint64("0x00123456"));
	REQUIRE((wint96("0x012345671234567823456789") >> 36) == wint96("0x0012345671234567"));

	REQUIRE((wint96("0x012345671234567823456789") >> 64) == wint96("0x01234567"));

	REQUIRE((wint96("0x012345671234567823456789") >> 68) == wint96("0x00123456"));

	REQUIRE((wint32("0x81234567") >> 4) == wint32("0xF8123456"));
	REQUIRE((wint64("0x8123456712345678") >> 4) == wint64("0xF812345671234567"));
	REQUIRE((wint96("0x812345671234567823456789") >> 4) == wint96("0xF81234567123456782345678"));

	REQUIRE((wint64("0x8123456712345678") >> 32) == wint64("0xFFFFFFFF81234567"));
	REQUIRE((wint96("0x812345671234567823456789") >> 32) == wint96("0xFFFFFFFF8123456712345678"));

	REQUIRE((wint64("0x8123456712345678") >> 36) == wint64("0xFFFFFFFFF8123456"));
	REQUIRE((wint96("0x812345671234567823456789") >> 36) == wint96("0xFFFFFFFFF812345671234567"));

	REQUIRE((wint96("0x812345671234567823456789") >> 64) == wint96("0xFFFFFFFFFFFFFFFF81234567"));

	REQUIRE((wint96("0x812345671234567823456789") >> 68) == wint96("0xFFFFFFFFFFFFFFFFF8123456"));

	REQUIRE((wint256("0x8123456712345678234567893456789A456789AB56789ABC6789ABCD789ABCDE") >> 224) == wint256("0xFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF81234567"));

	REQUIRE((wint256("0x8123456712345678234567893456789A456789AB56789ABC6789ABCD789ABCDE") >> 228) == wint256("0xFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF8123456"));
}

TEST_CASE("wint wint plus", "[wint]") {
	REQUIRE(wint32("1000000000") + wint32("1") == wint32("1000000001"));
	REQUIRE(wint64("1000000000000000000") + wint64("1") == wint64("1000000000000000001"));
	REQUIRE(wint96("1000000000000000000000000000") + wint96("1") == wint96("1000000000000000000000000001"));

	REQUIRE(wint32("1000000000") + wint32("1000000000") == wint32("2000000000"));
	REQUIRE(wint64("1000000000000000000") + wint64("1000000000000000000") == wint64("2000000000000000000"));
	REQUIRE(wint96("1000000000000000000000000000") + wint96("1000000000000000000000000000") == wint96("2000000000000000000000000000"));

	REQUIRE(wint32("1") + wint32("-1") == wint32("0"));
	REQUIRE(wint64("1") + wint64("-1") == wint64("0"));
	REQUIRE(wint96("1") + wint96("-1") == wint96("0"));

	REQUIRE(wint32("-1") + wint32("-1") == wint32("-2"));
	REQUIRE(wint64("-1") + wint64("-1") == wint64("-2"));
	REQUIRE(wint96("-1") + wint96("-1") == wint96("-2"));
}

TEST_CASE("wint wint minus", "[wint]") {
	REQUIRE(wint32("1000000001") - wint32("1") == wint32("1000000000"));
	REQUIRE(wint64("1000000000000000001") - wint64("1") == wint64("1000000000000000000"));
	REQUIRE(wint96("1000000000000000000000000001") - wint96("1") == wint96("1000000000000000000000000000"));

	REQUIRE(wint32("2000000000") - wint32("1000000000") == wint32("1000000000"));
	REQUIRE(wint64("2000000000000000000") - wint64("1000000000000000000") == wint64("1000000000000000000"));
	REQUIRE(wint96("2000000000000000000000000000") - wint96("1000000000000000000000000000") == wint96("1000000000000000000000000000"));

	REQUIRE(wint32("0") - wint32("1") == wint32("-1"));
	REQUIRE(wint64("0") - wint64("1") == wint64("-1"));
	REQUIRE(wint96("0") - wint96("1") == wint96("-1"));

	REQUIRE(wint32("-1") - wint32("-1") == wint32("0"));
	REQUIRE(wint64("-1") - wint64("-1") == wint64("0"));
	REQUIRE(wint96("-1") - wint96("-1") == wint96("0"));
}

TEST_CASE("wint wint multiply", "[wint]") {
	REQUIRE(wint32("1000000000") * wint32("1") == wint32("1000000000"));
	REQUIRE(wint64("1000000000000000000") * wint64("1") == wint64("1000000000000000000"));
	REQUIRE(wint96("1000000000000000000000000000") * wint96("1") == wint96("1000000000000000000000000000"));

	REQUIRE(wint32("1000000000") * wint32("2") == wint32("2000000000"));
	REQUIRE(wint64("1000000000000000000") * wint64("2") == wint64("2000000000000000000"));
	REQUIRE(wint96("1000000000000000000000000000") * wint96("2") == wint96("2000000000000000000000000000"));

	REQUIRE(wint32("10000") * wint32("10000") == wint32("100000000"));
	REQUIRE(wint64("1000000000") * wint64("1000000000") == wint64("1000000000000000000"));
	REQUIRE(wint96("10000000000000") * wint96("10000000000000") == wint96("100000000000000000000000000"));

	REQUIRE(wint32("-1") * wint32("0") == wint32("0"));
	REQUIRE(wint64("-1") * wint64("0") == wint64("0"));
	REQUIRE(wint96("-1") * wint96("0") == wint96("0"));

	REQUIRE(wint32("-1") * wint32("-1") == wint32("1"));
	REQUIRE(wint64("-1") * wint64("-1") == wint64("1"));
	REQUIRE(wint96("-1") * wint96("-1") == wint96("1"));

	REQUIRE(wint32("10000") * -wint32("10000") == -wint32("100000000"));
	REQUIRE(wint64("1000000000") * -wint64("1000000000") == -wint64("1000000000000000000"));
	REQUIRE(wint96("10000000000000") * -wint96("10000000000000") == -wint96("100000000000000000000000000"));
}

TEST_CASE("wint wint divide", "[wint]") {
	REQUIRE(wint32("1000000000") / wint32("1") == wint32("1000000000"));
	REQUIRE(wint64("1000000000000000000") / wint64("1") == wint64("1000000000000000000"));
	REQUIRE(wint96("1000000000000000000000000000") / wint96("1") == wint96("1000000000000000000000000000"));

	REQUIRE(wint32("1000000000") / wint32("2") == wint32("500000000"));
	REQUIRE(wint64("1000000000000000000") / wint64("2") == wint64("500000000000000000"));
	REQUIRE(wint96("1000000000000000000000000000") / wint96("2") == wint96("500000000000000000000000000"));

	REQUIRE(wint32("9999") / wint32("10000") == wint32("0"));
	REQUIRE(wint64("999999999") / wint64("1000000000") == wint64("0"));
	REQUIRE(wint96("9999999999999") / wint96("10000000000000") == wint96("0"));

	REQUIRE(wint32("10000") / wint32("10000") == wint32("1"));
	REQUIRE(wint64("1000000000") / wint64("1000000000") == wint64("1"));
	REQUIRE(wint96("10000000000000") / wint96("10000000000000") == wint96("1"));

	REQUIRE(wint32("10001") / wint32("10000") == wint32("1"));
	REQUIRE(wint64("1000000001") / wint64("1000000000") == wint64("1"));
	REQUIRE(wint96("10000000000001") / wint96("10000000000000") == wint96("1"));

	REQUIRE(wint32("19999") / wint32("10000") == wint32("1"));
	REQUIRE(wint64("1999999999") / wint64("1000000000") == wint64("1"));
	REQUIRE(wint96("19999999999999") / wint96("10000000000000") == wint96("1"));

	REQUIRE(wint32("20000") / wint32("10000") == wint32("2"));
	REQUIRE(wint64("2000000000") / wint64("1000000000") == wint64("2"));
	REQUIRE(wint96("20000000000000") / wint96("10000000000000") == wint96("2"));

	REQUIRE(wint32("10000") / wint32("100000000") == wint32("0"));
	REQUIRE(wint64("10000") / wint64("10000000000000000") == wint64("0"));
	REQUIRE(wint96("10000") / wint96("1000000000000000000000000") == wint96("0"));

	REQUIRE(wint32("456") / wint32("123") == 456 / 123);
	REQUIRE(wint32("456") / -wint32("123") == 456 / -123);
	REQUIRE(-wint32("456") / wint32("123") == -456 / 123);
	REQUIRE(-wint32("456") / -wint32("123") == -456 / -123);
}

TEST_CASE("wint wint divide limits", "[wint]") {
	// 80 / 80
	REQUIRE(wint32_80 / wint32_80 == wint32("1"));
	REQUIRE(wint64_80 / wint64_80 == wint64("1"));
	REQUIRE(wint96_80 / wint96_80 == wint96("1"));

	// 80 / 81
	REQUIRE(wint32_80 / wint32_81 == wint32("1"));
	REQUIRE(wint64_80 / wint64_81 == wint64("1"));
	REQUIRE(wint96_80 / wint96_81 == wint96("1"));

	// 80 / FF overflows

	// 80 / 01
	REQUIRE(wint32_80 / wint32_01 == wint32_80);
	REQUIRE(wint64_80 / wint64_01 == wint64_80);
	REQUIRE(wint96_80 / wint96_01 == wint96_80);

	// 80 / 7F
	REQUIRE(wint32_80 / wint32_7F == wint32("-1"));
	REQUIRE(wint64_80 / wint64_7F == wint64("-1"));
	REQUIRE(wint96_80 / wint96_7F == wint96("-1"));

	// 81 / 80
	REQUIRE(wint32_81 / wint32_80 == wint32("0"));
	REQUIRE(wint64_81 / wint64_80 == wint64("0"));
	REQUIRE(wint96_81 / wint96_80 == wint96("0"));

	// 81 / 81
	REQUIRE(wint32_81 / wint32_81 == wint32("1"));
	REQUIRE(wint64_81 / wint64_81 == wint64("1"));
	REQUIRE(wint96_81 / wint96_81 == wint96("1"));

	// 81 / FF
	REQUIRE(wint32_81 / wint32_FF == wint32_7F);
	REQUIRE(wint64_81 / wint64_FF == wint64_7F);
	REQUIRE(wint96_81 / wint96_FF == wint96_7F);

	// 81 / 01
	REQUIRE(wint32_81 / wint32_01 == wint32_81);
	REQUIRE(wint64_81 / wint64_01 == wint64_81);
	REQUIRE(wint96_81 / wint96_01 == wint96_81);

	// 81 / 7F
	REQUIRE(wint32_81 / wint32_7F == wint32("-1"));
	REQUIRE(wint64_81 / wint64_7F == wint64("-1"));
	REQUIRE(wint96_81 / wint96_7F == wint96("-1"));

	// FF / 80
	REQUIRE(wint32_FF / wint32_80 == wint32("0"));
	REQUIRE(wint64_FF / wint64_80 == wint64("0"));
	REQUIRE(wint96_FF / wint96_80 == wint96("0"));

	// FF / 81
	REQUIRE(wint32_FF / wint32_81 == wint32("0"));
	REQUIRE(wint64_FF / wint64_81 == wint64("0"));
	REQUIRE(wint96_FF / wint96_81 == wint96("0"));

	// FF / FF
	REQUIRE(wint32_FF / wint32_FF == wint32("1"));
	REQUIRE(wint64_FF / wint64_FF == wint64("1"));
	REQUIRE(wint96_FF / wint96_FF == wint96("1"));

	// FF / 01
	REQUIRE(wint32_FF / wint32_01 == wint32_FF);
	REQUIRE(wint64_FF / wint64_01 == wint64_FF);
	REQUIRE(wint96_FF / wint96_01 == wint96_FF);

	// FF / 7F
	REQUIRE(wint32_FF / wint32_7F == wint32("0"));
	REQUIRE(wint64_FF / wint64_7F == wint64("0"));
	REQUIRE(wint96_FF / wint96_7F == wint96("0"));

	// 01 / 80
	REQUIRE(wint32_01 / wint32_80 == wint32("0"));
	REQUIRE(wint64_01 / wint64_80 == wint64("0"));
	REQUIRE(wint96_01 / wint96_80 == wint96("0"));

	// 01 / 81
	REQUIRE(wint32_01 / wint32_81 == wint32("0"));
	REQUIRE(wint64_01 / wint64_81 == wint64("0"));
	REQUIRE(wint96_01 / wint96_81 == wint96("0"));

	// 01 / FF
	REQUIRE(wint32_01 / wint32_FF == wint32("-1"));
	REQUIRE(wint64_01 / wint64_FF == wint64("-1"));
	REQUIRE(wint96_01 / wint96_FF == wint96("-1"));

	// 01 / 01
	REQUIRE(wint32_01 / wint32_01 == wint32("1"));
	REQUIRE(wint64_01 / wint64_01 == wint64("1"));
	REQUIRE(wint96_01 / wint96_01 == wint96("1"));

	// 01 / 7F
	REQUIRE(wint32_01 / wint32_7F == wint32("0"));
	REQUIRE(wint64_01 / wint64_7F == wint64("0"));
	REQUIRE(wint96_01 / wint96_7F == wint96("0"));

	// 7F / 80
	REQUIRE(wint32_7F / wint32_80 == wint32("0"));
	REQUIRE(wint64_7F / wint64_80 == wint64("0"));
	REQUIRE(wint96_7F / wint96_80 == wint96("0"));

	// 7F / 81
	REQUIRE(wint32_7F / wint32_81 == wint32("-1"));
	REQUIRE(wint64_7F / wint64_81 == wint64("-1"));
	REQUIRE(wint96_7F / wint96_81 == wint96("-1"));

	// 7F / FF
	REQUIRE(wint32_7F / wint32_FF == wint32_81);
	REQUIRE(wint64_7F / wint64_FF == wint64_81);
	REQUIRE(wint96_7F / wint96_FF == wint96_81);

	// 7F / 01
	REQUIRE(wint32_7F / wint32_01 == wint32_7F);
	REQUIRE(wint64_7F / wint64_01 == wint64_7F);
	REQUIRE(wint96_7F / wint96_01 == wint96_7F);

	// 7F / 7F
	REQUIRE(wint32_7F / wint32_7F == wint32("1"));
	REQUIRE(wint64_7F / wint64_7F == wint64("1"));
	REQUIRE(wint96_7F / wint96_7F == wint96("1"));
}

TEST_CASE("wint wint modulus", "[wint]") {
	REQUIRE(wint32("9999") % wint32("10000") == 9999);
	REQUIRE(wint32("10000") % wint32("10000") == 0);
	REQUIRE(wint32("10001") % wint32("10000") == 1);
	REQUIRE(wint32("19999") % wint32("10000") == 9999);
	REQUIRE(wint32("20000") % wint32("10000") == 0);

	REQUIRE(wint32("100000000") % wint32("10000") == 0);
	REQUIRE(wint64("10000000000000000") % wint64("10000") == 0);
	REQUIRE(wint96("1000000000000000000000000") % wint96("10000") == 0);

	REQUIRE(wint32("10000") % wint32("100000000") == 10000);
	REQUIRE(wint64("10000") % wint64("10000000000000000") == 10000);
	REQUIRE(wint96("10000") % wint96("1000000000000000000000000") == 10000);
}

TEST_CASE("wint wint modulus limits", "[wint]") {
	// 80 % 80
	REQUIRE(wint32_80 % wint32_80 == wint32("0"));
	REQUIRE(wint64_80 % wint64_80 == wint64("0"));
	REQUIRE(wint96_80 % wint96_80 == wint96("0"));

	// 80 % 81
	REQUIRE(wint32_80 % wint32_81 == wint32("-1"));
	REQUIRE(wint64_80 % wint64_81 == wint64("-1"));
	REQUIRE(wint96_80 % wint96_81 == wint96("-1"));

	// 80 % FF overflows

	// 80 % 01
	REQUIRE(wint32_80 % wint32_01 == wint32("0"));
	REQUIRE(wint64_80 % wint64_01 == wint64("0"));
	REQUIRE(wint96_80 % wint96_01 == wint96("0"));

	// 80 % 7F
	REQUIRE(wint32_80 % wint32_7F == wint32("-1"));
	REQUIRE(wint64_80 % wint64_7F == wint64("-1"));
	REQUIRE(wint96_80 % wint96_7F == wint96("-1"));

	// 81 % 80
	REQUIRE(wint32_81 % wint32_80 == wint32_81);
	REQUIRE(wint64_81 % wint64_80 == wint64_81);
	REQUIRE(wint96_81 % wint96_80 == wint96_81);

	// 81 % 81
	REQUIRE(wint32_81 % wint32_81 == wint32("0"));
	REQUIRE(wint64_81 % wint64_81 == wint64("0"));
	REQUIRE(wint96_81 % wint96_81 == wint96("0"));

	// 81 % FF
	REQUIRE(wint32_81 % wint32_FF == wint32("0"));
	REQUIRE(wint64_81 % wint64_FF == wint64("0"));
	REQUIRE(wint96_81 % wint96_FF == wint96("0"));

	// 81 % 01
	REQUIRE(wint32_81 % wint32_01 == wint32("0"));
	REQUIRE(wint64_81 % wint64_01 == wint64("0"));
	REQUIRE(wint96_81 % wint96_01 == wint96("0"));

	// 81 % 7F
	REQUIRE(wint32_81 % wint32_7F == wint32("0"));
	REQUIRE(wint64_81 % wint64_7F == wint64("0"));
	REQUIRE(wint96_81 % wint96_7F == wint96("0"));

	// FF % 80
	REQUIRE(wint32_FF % wint32_80 == wint32("-1"));
	REQUIRE(wint64_FF % wint64_80 == wint64("-1"));
	REQUIRE(wint96_FF % wint96_80 == wint96("-1"));

	// FF % 81
	REQUIRE(wint32_FF % wint32_81 == wint32("-1"));
	REQUIRE(wint64_FF % wint64_81 == wint64("-1"));
	REQUIRE(wint96_FF % wint96_81 == wint96("-1"));

	// FF % FF
	REQUIRE(wint32_FF % wint32_FF == wint32("0"));
	REQUIRE(wint64_FF % wint64_FF == wint64("0"));
	REQUIRE(wint96_FF % wint96_FF == wint96("0"));

	// FF % 01
	REQUIRE(wint32_FF % wint32_01 == wint32("0"));
	REQUIRE(wint64_FF % wint64_01 == wint64("0"));
	REQUIRE(wint96_FF % wint96_01 == wint96("0"));

	// FF % 7F
	REQUIRE(wint32_FF % wint32_7F == wint32("-1"));
	REQUIRE(wint64_FF % wint64_7F == wint64("-1"));
	REQUIRE(wint96_FF % wint96_7F == wint96("-1"));

	// 01 % 80
	REQUIRE(wint32_01 % wint32_80 == wint32("1"));
	REQUIRE(wint64_01 % wint64_80 == wint64("1"));
	REQUIRE(wint96_01 % wint96_80 == wint96("1"));

	// 01 % 81
	REQUIRE(wint32_01 % wint32_81 == wint32("1"));
	REQUIRE(wint64_01 % wint64_81 == wint64("1"));
	REQUIRE(wint96_01 % wint96_81 == wint96("1"));

	// 01 % FF
	REQUIRE(wint32_01 % wint32_FF == wint32("0"));
	REQUIRE(wint64_01 % wint64_FF == wint64("0"));
	REQUIRE(wint96_01 % wint96_FF == wint96("0"));

	// 01 % 01
	REQUIRE(wint32_01 % wint32_01 == wint32("0"));
	REQUIRE(wint64_01 % wint64_01 == wint64("0"));
	REQUIRE(wint96_01 % wint96_01 == wint96("0"));

	// 01 % 7F
	REQUIRE(wint32_01 % wint32_7F == wint32("1"));
	REQUIRE(wint64_01 % wint64_7F == wint64("1"));
	REQUIRE(wint96_01 % wint96_7F == wint96("1"));

	// 7F % 80
	REQUIRE(wint32_7F % wint32_80 == wint32_7F);
	REQUIRE(wint64_7F % wint64_80 == wint64_7F);
	REQUIRE(wint96_7F % wint96_80 == wint96_7F);

	// 7F % 81
	REQUIRE(wint32_7F % wint32_81 == wint32("0"));
	REQUIRE(wint64_7F % wint64_81 == wint64("0"));
	REQUIRE(wint96_7F % wint96_81 == wint96("0"));

	// 7F % FF
	REQUIRE(wint32_7F % wint32_FF == wint32("0"));
	REQUIRE(wint64_7F % wint64_FF == wint64("0"));
	REQUIRE(wint96_7F % wint96_FF == wint96("0"));

	// 7F % 01
	REQUIRE(wint32_7F % wint32_01 == wint32("0"));
	REQUIRE(wint64_7F % wint64_01 == wint64("0"));
	REQUIRE(wint96_7F % wint96_01 == wint96("0"));

	// 7F % 7F
	REQUIRE(wint32_7F % wint32_7F == wint32("0"));
	REQUIRE(wint64_7F % wint64_7F == wint64("0"));
	REQUIRE(wint96_7F % wint96_7F == wint96("0"));
}

TEST_CASE("wint wint bitwise and", "[wint]") {
	REQUIRE((wint32("-1") & wint32("0")) == wint32("0"));
	REQUIRE((wint64("-1") & wint64("0")) == wint64("0"));
	REQUIRE((wint96("-1") & wint96("0")) == wint96("0"));

	REQUIRE((wint32("-1") & wint32("-1")) == wint32("-1"));
	REQUIRE((wint64("-1") & wint64("-1")) == wint64("-1"));
	REQUIRE((wint96("-1") & wint96("-1")) == wint96("-1"));

	REQUIRE((wint32("0x1F2F3F4F") & wint32("0xF0F0F0F0")) == wint32("0x10203040"));
	REQUIRE((wint64("0x1F2F3F4F5F6F7F8F") & wint64("0xF0F0F0F0F0F0F0F0")) == wint64("0x1020304050607080"));
	REQUIRE((wint96("0x1F2F3F4F5F6F7F8F9FAFBFCF") & wint96("0xF0F0F0F0F0F0F0F0F0F0F0F0")) == wint96("0x102030405060708090A0B0C0"));
}

TEST_CASE("wint wint bitwise or", "[wint]") {
	REQUIRE((wint32("0") | wint32("-1")) == wint32("-1"));
	REQUIRE((wint64("0") | wint64("-1")) == wint64("-1"));
	REQUIRE((wint96("0") | wint96("-1")) == wint96("-1"));

	REQUIRE((wint32("-1") | wint32("-1")) == wint32("-1"));
	REQUIRE((wint64("-1") | wint64("-1")) == wint64("-1"));
	REQUIRE((wint96("-1") | wint96("-1")) == wint96("-1"));

	REQUIRE((wint32("0x10203040") | wint32("0x0F0F0F0F")) == wint32("0x1F2F3F4F"));
	REQUIRE((wint64("0x1020304050607080") | wint64("0x0F0F0F0F0F0F0F0F")) == wint64("0x1F2F3F4F5F6F7F8F"));
	REQUIRE((wint96("0x102030405060708090A0B0C0") | wint96("0x0F0F0F0F0F0F0F0F0F0F0F0F")) == wint96("0x1F2F3F4F5F6F7F8F9FAFBFCF"));
}

TEST_CASE("wint wint bitwise xor", "[wint]") {
	REQUIRE((wint32("-1") ^ wint32("0")) == wint32("-1"));
	REQUIRE((wint64("-1") ^ wint64("0")) == wint64("-1"));
	REQUIRE((wint96("-1") ^ wint96("0")) == wint96("-1"));

	REQUIRE((wint32("-1") ^ wint32("-1")) == wint32("0"));
	REQUIRE((wint64("-1") ^ wint64("-1")) == wint64("0"));
	REQUIRE((wint96("-1") ^ wint96("-1")) == wint96("0"));

	REQUIRE((wint32("0xAAAAAAAA") ^ wint32("0x3C3C3C3C")) == wint32("0x96969696"));
	REQUIRE((wint64("0xAAAAAAAAAAAAAAAA") ^ wint64("0x3C3C3C3C3C3C3C3C")) == wint64("0x9696969696969696"));
	REQUIRE((wint96("0xAAAAAAAAAAAAAAAAAAAAAAAA") ^ wint96("0x3C3C3C3C3C3C3C3C3C3C3C3C")) == wint96("0x969696969696969696969696"));
}

TEST_CASE("wint int32 plus", "[wint]") {
	REQUIRE(wint32("1000000000") + 1 == wint32("1000000001"));
	REQUIRE(wint64("1000000000000000000") + 1 == wint64("1000000000000000001"));
	REQUIRE(wint96("1000000000000000000000000000") + 1 == wint96("1000000000000000000000000001"));

	REQUIRE(wint32("999999999") + 1 == wint32("1000000000"));
	REQUIRE(wint64("999999999999999999") + 1 == wint64("1000000000000000000"));
	REQUIRE(wint96("999999999999999999999999999") + 1 == wint96("1000000000000000000000000000"));

	REQUIRE(wint32("0xEFFFFFFF") + 1 == wint32("0xF0000000"));
	REQUIRE(wint64("0xEFFFFFFFFFFFFFFF") + 1 == wint64("0xF000000000000000"));
	REQUIRE(wint96("0xEFFFFFFFFFFFFFFFFFFFFFFF") + 1 == wint96("0xF00000000000000000000000"));

	REQUIRE(wint32("1000000000") + 123456789 == wint32("1123456789"));
	REQUIRE(wint64("100000000000000000") + 123456789 == wint64("100000000123456789"));
	REQUIRE(wint96("100000000000000000000000000") + 123456789 == wint96("100000000000000000123456789"));

	REQUIRE(wint32("-1") + 1 == wint32("0"));
	REQUIRE(wint64("-1") + 1 == wint64("0"));
	REQUIRE(wint96("-1") + 1 == wint96("0"));

	REQUIRE(wint32::min() + std::numeric_limits<std::int32_t>::min() == wint32("0"));
	REQUIRE(wint64("0x80000000") + std::numeric_limits<std::int32_t>::min() == wint64("0"));

	REQUIRE(1 + wint32("1000000000") == wint32("1000000001"));
	REQUIRE(1 + wint64("1000000000000000000") == wint64("1000000000000000001"));
	REQUIRE(1 + wint96("1000000000000000000000000000") == wint96("1000000000000000000000000001"));

	REQUIRE(1 + wint32("999999999") == wint32("1000000000"));
	REQUIRE(1 + wint64("999999999999999999") == wint64("1000000000000000000"));
	REQUIRE(1 + wint96("999999999999999999999999999") == wint96("1000000000000000000000000000"));

	REQUIRE(1 + wint32("0xEFFFFFFF") == wint32("0xF0000000"));
	REQUIRE(1 + wint64("0xEFFFFFFFFFFFFFFF") == wint64("0xF000000000000000"));
	REQUIRE(1 + wint96("0xEFFFFFFFFFFFFFFFFFFFFFFF") == wint96("0xF00000000000000000000000"));

	REQUIRE(123456789 + wint32("1000000000") == wint32("1123456789"));
	REQUIRE(123456789 + wint64("100000000000000000") == wint64("100000000123456789"));
	REQUIRE(123456789 + wint96("100000000000000000000000000") == wint96("100000000000000000123456789"));

	REQUIRE(1 + wint32("-1") == wint32("0"));
	REQUIRE(1 + wint64("-1") == wint64("0"));
	REQUIRE(1 + wint96("-1") == wint96("0"));
}

TEST_CASE("wint int32 minus", "[wint]") {
	REQUIRE(wint32("1000000001") - 1 == wint32("1000000000"));
	REQUIRE(wint64("1000000000000000001") - 1 == wint64("1000000000000000000"));
	REQUIRE(wint96("1000000000000000000000000001") - 1 == wint96("1000000000000000000000000000"));

	REQUIRE(wint32("1000000000") - 1 == wint32("999999999"));
	REQUIRE(wint64("1000000000000000000") - 1 == wint64("999999999999999999"));
	REQUIRE(wint96("1000000000000000000000000000") - 1 == wint96("999999999999999999999999999"));

	REQUIRE(wint32("0") - 1 == wint32("-1"));
	REQUIRE(wint64("0") - 1 == wint64("-1"));
	REQUIRE(wint96("0") - 1 == wint96("-1"));

	REQUIRE(wint32("1123456789") - 123456789 == wint32("1000000000"));
	REQUIRE(wint64("100000000123456789") - 123456789 == wint64("100000000000000000"));
	REQUIRE(wint96("100000000000000000123456789") - 123456789 == wint96("100000000000000000000000000"));

	REQUIRE(wint32::min() - std::numeric_limits<std::int32_t>::min() == wint32("0"));
	REQUIRE(wint64("0x80000000") - std::numeric_limits<std::int32_t>::min() == wint64("0x100000000"));

	REQUIRE(0 - wint32("1") == wint32("-1"));
	REQUIRE(0 - wint64("1") == wint64("-1"));
	REQUIRE(0 - wint96("1") == wint96("-1"));

	REQUIRE(123456789 - wint32("100000000") == wint32("23456789"));
	REQUIRE(123456789 - wint64("100000000") == wint64("23456789"));
	REQUIRE(123456789 - wint96("100000000") == wint96("23456789"));

	REQUIRE(123456789 - wint32("999999999") == wint32("-876543210"));
	REQUIRE(123456789 - wint64("999999999") == wint64("-876543210"));
	REQUIRE(123456789 - wint96("999999999") == wint96("-876543210"));
}

TEST_CASE("wint int32 multiply", "[wint]") {
	REQUIRE(wint32("-1") * 0 == wint32("0"));
	REQUIRE(wint64("-1") * 0 == wint64("0"));
	REQUIRE(wint96("-1") * 0 == wint96("0"));

	REQUIRE(wint32("1000000000") * 1 == wint32("1000000000"));
	REQUIRE(wint64("1000000000000000000") * 1 == wint64("1000000000000000000"));
	REQUIRE(wint96("1000000000000000000000000000") * 1 == wint96("1000000000000000000000000000"));

	REQUIRE(wint32("1000000000") * 2 == wint32("2000000000"));
	REQUIRE(wint64("1000000000000000000") * 2 == wint64("2000000000000000000"));
	REQUIRE(wint96("1000000000000000000000000000") * 2 == wint96("2000000000000000000000000000"));

	REQUIRE(wint32("10000") * 10000 == wint32("100000000"));
	REQUIRE(wint64("100000000") * 10000 == wint64("1000000000000"));
	REQUIRE(wint96("1000000000000") * 10000 == wint96("10000000000000000"));

	REQUIRE(wint32("1") * std::numeric_limits<std::int32_t>::min() == wint32::min());
	REQUIRE(wint64("1") * std::numeric_limits<std::int32_t>::min() == wint64("-0x80000000"));

	REQUIRE(1 * wint32("1000000000") == wint32("1000000000"));
	REQUIRE(1 * wint64("1000000000000000000") == wint64("1000000000000000000"));
	REQUIRE(1 * wint96("1000000000000000000000000000") == wint96("1000000000000000000000000000"));

	REQUIRE(2 * wint32("1000000000") == wint32("2000000000"));
	REQUIRE(2 * wint64("1000000000000000000") == wint64("2000000000000000000"));
	REQUIRE(2 * wint96("1000000000000000000000000000") == wint96("2000000000000000000000000000"));

	REQUIRE(10000 * wint32("10000") == wint32("100000000"));
	REQUIRE(10000 * wint64("100000000") == wint64("1000000000000"));
	REQUIRE(10000 * wint96("1000000000000") == wint96("10000000000000000"));

	REQUIRE(10000 * wint32("-1") == wint32("-10000"));
	REQUIRE(10000 * wint64("-1") == wint64("-10000"));
	REQUIRE(10000 * wint96("-1") == wint96("-10000"));

	REQUIRE(10000 * wint32("-10000") == wint32("-100000000"));
	REQUIRE(10000 * wint64("-100000000") == wint64("-1000000000000"));
	REQUIRE(10000 * wint96("-1000000000000") == wint96("-10000000000000000"));
}

TEST_CASE("wint int32 divide", "[wint]") {
	REQUIRE(wint32("1000000000") / 1 == wint32("1000000000"));
	REQUIRE(wint64("1000000000000000000") / 1 == wint64("1000000000000000000"));
	REQUIRE(wint96("1000000000000000000000000000") / 1 == wint96("1000000000000000000000000000"));

	REQUIRE(wint32("1000000000") / 2 == wint32("500000000"));
	REQUIRE(wint64("1000000000000000000") / 2 == wint64("500000000000000000"));
	REQUIRE(wint96("1000000000000000000000000000") / 2 == wint96("500000000000000000000000000"));

	REQUIRE(wint32("9999") / 10000 == wint32("0"));
	REQUIRE(wint32("10000") / 10000 == wint32("1"));
	REQUIRE(wint32("10001") / 10000 == wint32("1"));
	REQUIRE(wint32("19999") / 10000 == wint32("1"));
	REQUIRE(wint32("20000") / 10000 == wint32("2"));

	REQUIRE(wint32("100000000") / 10000 == wint32("10000"));
	REQUIRE(wint64("10000000000000000") / 10000 == wint64("1000000000000"));
	REQUIRE(wint96("1000000000000000000000000") / 10000 == wint96("100000000000000000000"));

	REQUIRE(wint32("456") / 123 == 456 / 123);
	REQUIRE(wint32("456") / -123 == 456 / -123);
	REQUIRE(-wint32("456") / 123 == -456 / 123);
	REQUIRE(-wint32("456") / -123 == -456 / -123);

	REQUIRE(9999 / wint32("10000") == wint32("0"));
	REQUIRE(10000 / wint32("10000") == wint32("1"));
	REQUIRE(10001 / wint32("10000") == wint32("1"));
	REQUIRE(19999 / wint32("10000") == wint32("1"));
	REQUIRE(20000 / wint32("10000") == wint32("2"));

	REQUIRE(10000 / wint32("100000000") == wint32("0"));
	REQUIRE(10000 / wint64("10000000000000000") == wint64("0"));
	REQUIRE(10000 / wint96("1000000000000000000000000") == wint96("0"));
}

TEST_CASE("wint int32 divide limits", "[wint]") {
	// 80 / 80
	REQUIRE(wint32_80 / int32_80 == wint32("1"));
	REQUIRE(wint64_80 / int32_80 == wint64("0x100000000"));
	REQUIRE(wint96_80 / int32_80 == wint96("0x10000000000000000"));

	// 80 / 81
	REQUIRE(wint32_80 / int32_81 == wint32("1"));
	REQUIRE(wint64_80 / int32_81 == wint64("0x100000002"));
	REQUIRE(wint96_80 / int32_81 == wint96("0x10000000200000004"));

	// 80 / FF overflows

	// 80 / 01
	REQUIRE(wint32_80 / int32_01 == wint32_80);
	REQUIRE(wint64_80 / int32_01 == wint64_80);
	REQUIRE(wint96_80 / int32_01 == wint96_80);

	// 80 / 7F
	REQUIRE(wint32_80 / int32_7F == wint32("-1"));
	REQUIRE(wint64_80 / int32_7F == -wint64("0x100000002"));
	REQUIRE(wint96_80 / int32_7F == -wint96("0x10000000200000004"));

	// 81 / 80
	REQUIRE(wint32_81 / int32_80 == wint32("0"));
	REQUIRE(wint64_81 / int32_80 == wint64("0xFFFFFFFF"));
	REQUIRE(wint96_81 / int32_80 == wint96("0xFFFFFFFFFFFFFFFF"));

	// 81 / 81
	REQUIRE(wint32_81 / int32_81 == wint32("1"));
	REQUIRE(wint64_81 / int32_81 == wint64("0x100000002"));
	REQUIRE(wint96_81 / int32_81 == wint96("0x10000000200000004"));

	// 81 / FF
	REQUIRE(wint32_81 / int32_FF == wint32_7F);
	REQUIRE(wint64_81 / int32_FF == wint64_7F);
	REQUIRE(wint96_81 / int32_FF == wint96_7F);

	// 81 / 01
	REQUIRE(wint32_81 / int32_01 == wint32_81);
	REQUIRE(wint64_81 / int32_01 == wint64_81);
	REQUIRE(wint96_81 / int32_01 == wint96_81);

	// 81 / 7F
	REQUIRE(wint32_81 / int32_7F == wint32("-1"));
	REQUIRE(wint64_81 / int32_7F == wint64("-0x100000002"));
	REQUIRE(wint96_81 / int32_7F == wint96("-0x10000000200000004"));

	// FF / 80
	REQUIRE(wint32_FF / int32_80 == wint32("0"));
	REQUIRE(wint64_FF / int32_80 == wint64("0"));
	REQUIRE(wint96_FF / int32_80 == wint96("0"));

	// FF / 81
	REQUIRE(wint32_FF / int32_81 == wint32("0"));
	REQUIRE(wint64_FF / int32_81 == wint64("0"));
	REQUIRE(wint96_FF / int32_81 == wint96("0"));

	// FF / FF
	REQUIRE(wint32_FF / int32_FF == wint32("1"));
	REQUIRE(wint64_FF / int32_FF == wint64("1"));
	REQUIRE(wint96_FF / int32_FF == wint96("1"));

	// FF / 01
	REQUIRE(wint32_FF / int32_01 == wint32("-1"));
	REQUIRE(wint64_FF / int32_01 == wint64("-1"));
	REQUIRE(wint96_FF / int32_01 == wint96("-1"));

	// FF / 7F
	REQUIRE(wint32_FF / int32_7F == wint32("0"));
	REQUIRE(wint64_FF / int32_7F == wint64("0"));
	REQUIRE(wint96_FF / int32_7F == wint96("0"));

	// 01 / 80
	REQUIRE(wint32_01 / int32_80 == wint32("0"));
	REQUIRE(wint64_01 / int32_80 == wint64("0"));
	REQUIRE(wint96_01 / int32_80 == wint96("0"));

	// 01 / 81
	REQUIRE(wint32_01 / int32_81 == wint32("0"));
	REQUIRE(wint64_01 / int32_81 == wint64("0"));
	REQUIRE(wint96_01 / int32_81 == wint96("0"));

	// 01 / FF
	REQUIRE(wint32_01 / int32_FF == wint32("-1"));
	REQUIRE(wint64_01 / int32_FF == wint64("-1"));
	REQUIRE(wint96_01 / int32_FF == wint96("-1"));

	// 01 / 01
	REQUIRE(wint32_01 / int32_01 == wint32("1"));
	REQUIRE(wint64_01 / int32_01 == wint64("1"));
	REQUIRE(wint96_01 / int32_01 == wint96("1"));

	// 01 / 7F
	REQUIRE(wint32_01 / int32_7F == wint32("0"));
	REQUIRE(wint64_01 / int32_7F == wint64("0"));
	REQUIRE(wint96_01 / int32_7F == wint96("0"));

	// 7F / 80
	REQUIRE(wint32_7F / int32_80 == wint32("0"));
	REQUIRE(wint64_7F / int32_80 == wint64("-0xFFFFFFFF"));
	REQUIRE(wint96_7F / int32_80 == wint96("-0xFFFFFFFFFFFFFFFF"));

	// 7F / 81
	REQUIRE(wint32_7F / int32_81 == wint32("-1"));
	REQUIRE(wint64_7F / int32_81 == wint64("-0x100000002"));
	REQUIRE(wint96_7F / int32_81 == wint96("-0x10000000200000004"));

	// 7F / FF
	REQUIRE(wint32_7F / int32_FF == wint32_81);
	REQUIRE(wint64_7F / int32_FF == wint64_81);
	REQUIRE(wint96_7F / int32_FF == wint96_81);

	// 7F / 01
	REQUIRE(wint32_7F / int32_01 == wint32_7F);
	REQUIRE(wint64_7F / int32_01 == wint64_7F);
	REQUIRE(wint96_7F / int32_01 == wint96_7F);

	// 7F / 7F
	REQUIRE(wint32_7F / int32_7F == wint32("1"));
	REQUIRE(wint64_7F / int32_7F == wint64("0x100000002"));
	REQUIRE(wint96_7F / int32_7F == wint96("0x10000000200000004"));
}

TEST_CASE("int32 wint divide limits", "[wint]") {
	// 80 / 80
	REQUIRE(int32_80 / wint32_80 == wint32("1"));

	// 80 / 81
	REQUIRE(int32_80 / wint32_81 == wint32("1"));

	// 80 / FF overflows

	// 80 / 01
	REQUIRE(int32_80 / wint32("1") == wint32_80);

	// 80 / 7F
	REQUIRE(int32_80 / wint32_7F == wint32("-1"));

	// 81 / 80
	REQUIRE(int32_81 / wint32_80 == wint32("0"));

	// 81 / 81
	REQUIRE(int32_81 / wint32_81 == wint32("1"));

	// 81 / FF
	REQUIRE(int32_81 / wint32_FF == -wint32_81);

	// 81 / 01
	REQUIRE(int32_81 / wint32("1") == wint32_81);

	// 81 / 7F
	REQUIRE(int32_81 / wint32_7F == wint32("-1"));

	// FF / 80
	REQUIRE(int32_FF / wint32_80 == wint32("0"));
	REQUIRE(int32_FF / wint64_80 == wint64("0"));
	REQUIRE(int32_FF / wint96_80 == wint96("0"));

	// FF / 81
	REQUIRE(int32_FF / wint32_81 == wint32("0"));
	REQUIRE(int32_FF / wint64_81 == wint64("0"));
	REQUIRE(int32_FF / wint96_81 == wint96("0"));

	// FF / FF
	REQUIRE(int32_FF / wint32_FF == wint32("1"));
	REQUIRE(int32_FF / wint64_FF == wint64("1"));
	REQUIRE(int32_FF / wint96_FF == wint96("1"));

	// FF / 01
	REQUIRE(int32_FF / wint32("1") == wint32_FF);
	REQUIRE(int32_FF / wint64("1") == wint64_FF);
	REQUIRE(int32_FF / wint96("1") == wint96_FF);

	// FF / 7F
	REQUIRE(int32_FF / wint32_7F == wint32("0"));
	REQUIRE(int32_FF / wint64_7F == wint64("0"));
	REQUIRE(int32_FF / wint96_7F == wint96("0"));

	// 01 / 80
	REQUIRE(int32_01 / wint32_80 == wint32("0"));
	REQUIRE(int32_01 / wint64_80 == wint64("0"));
	REQUIRE(int32_01 / wint96_80 == wint96("0"));

	// 01 / 81
	REQUIRE(int32_01 / wint32_81 == wint32("0"));
	REQUIRE(int32_01 / wint64_81 == wint64("0"));
	REQUIRE(int32_01 / wint96_81 == wint96("0"));

	// 01 / FF
	REQUIRE(int32_01 / wint32_FF == wint32("-1"));
	REQUIRE(int32_01 / wint64_FF == wint64("-1"));
	REQUIRE(int32_01 / wint96_FF == wint96("-1"));

	// 01 / 01
	REQUIRE(int32_01 / wint32("1") == wint32("1"));
	REQUIRE(int32_01 / wint64("1") == wint64("1"));
	REQUIRE(int32_01 / wint96("1") == wint96("1"));

	// 01 / 7F
	REQUIRE(int32_01 / wint32_7F == wint32("0"));
	REQUIRE(int32_01 / wint64_7F == wint64("0"));
	REQUIRE(int32_01 / wint96_7F == wint96("0"));

	// 7F / 80
	REQUIRE(int32_7F / wint32_80 == wint32("0"));

	// 7F / 81
	REQUIRE(int32_7F / wint32_81 == wint32("-1"));

	// 7F / FF
	REQUIRE(int32_7F / wint32_FF == -wint32_7F);

	// 7F / 01
	REQUIRE(int32_7F / wint32("1") == wint32_7F);

	// 7F / 7F
	REQUIRE(int32_7F / wint32_7F == wint32("1"));
}

TEST_CASE("wint int32 modulus", "[wint]") {
	REQUIRE(wint32("9999") % 10000 == 9999);
	REQUIRE(wint32("10000") % 10000 == 0);
	REQUIRE(wint32("10001") % 10000 == 1);
	REQUIRE(wint32("19999") % 10000 == 9999);
	REQUIRE(wint32("20000") % 10000 == 0);

	REQUIRE(wint32("-1") % 2 == -1);
	REQUIRE(wint64("-1") % 2 == -1);
	REQUIRE(wint96("-1") % 2 == -1);

	REQUIRE(wint32("100000000") % 10000 == 0);
	REQUIRE(wint64("10000000000000000") % 10000 == 0);
	REQUIRE(wint96("1000000000000000000000000") % 10000 == 0);

	REQUIRE(wint32("456") % 123 == 456 % 123);
	REQUIRE(wint32("456") % -123 == 456 % -123);
	REQUIRE(-wint32("456") % 123 == -456 % 123);
	REQUIRE(-wint32("456") % -123 == -456 % -123);

	REQUIRE(wint32("-1") % 2 == wint32("-1"));
	REQUIRE(wint64("-1") % 2 == wint64("-1"));
	REQUIRE(wint96("-1") % 2 == wint96("-1"));

	REQUIRE(wint32("1") % -2 == wint32("1"));
	REQUIRE(wint64("1") % -2 == wint64("1"));
	REQUIRE(wint96("1") % -2 == wint96("1"));

	REQUIRE(wint32("-1") % -2 == wint32("-1"));
	REQUIRE(wint64("-1") % -2 == wint64("-1"));
	REQUIRE(wint96("-1") % -2 == wint96("-1"));

	REQUIRE(9999 % wint32("10000") == wint32("9999"));
	REQUIRE(10000 % wint32("10000") == wint32("0"));
	REQUIRE(10001 % wint32("10000") == wint32("1"));
	REQUIRE(19999 % wint32("10000") == wint32("9999"));
	REQUIRE(20000 % wint32("10000") == wint32("0"));

	REQUIRE(10000 % wint32("100000000") == wint32("10000"));
	REQUIRE(10000 % wint64("10000000000000000") == wint64("10000"));
	REQUIRE(10000 % wint96("1000000000000000000000000") == wint96("10000"));

	REQUIRE(456 % wint32("123") == 456 % 123);
	REQUIRE(456 % -wint32("123") == 456 % -123);
	REQUIRE(-456 % wint32("123") == -456 % 123);
	REQUIRE(-456 % -wint32("123") == -456 % -123);

	REQUIRE(-1 % wint32("2") == wint32("-1"));
	REQUIRE(-1 % wint64("2") == wint64("-1"));
	REQUIRE(-1 % wint96("2") == wint96("-1"));

	REQUIRE(1 % wint32("-2") == wint32("1"));
	REQUIRE(1 % wint64("-2") == wint64("1"));
	REQUIRE(1 % wint96("-2") == wint96("1"));

	REQUIRE(-1 % wint32("-2") == wint32("-1"));
	REQUIRE(-1 % wint64("-2") == wint64("-1"));
	REQUIRE(-1 % wint96("-2") == wint96("-1"));
}

TEST_CASE("wint int32 modulus limits", "[wint]") {
	// 80 % 80
	REQUIRE(wint32_80 % int32_80 == wint32("0"));
	REQUIRE(wint64_80 % int32_80 == wint64("0"));
	REQUIRE(wint96_80 % int32_80 == wint96("0"));

	// 80 % 81
	REQUIRE(wint32_80 % int32_81 == wint32("-1"));
	REQUIRE(wint64_80 % int32_81 == wint64("-2"));
	REQUIRE(wint96_80 % int32_81 == wint96("-4"));

	// 80 % FF overflows

	// 80 % 01
	REQUIRE(wint32_80 % int32_01 == wint32("0"));
	REQUIRE(wint64_80 % int32_01 == wint32("0"));
	REQUIRE(wint96_80 % int32_01 == wint32("0"));

	// 80 % 7F
	REQUIRE(wint32_80 % int32_7F == wint32("-1"));
	REQUIRE(wint64_80 % int32_7F == wint32("-2"));
	REQUIRE(wint96_80 % int32_7F == wint96("-4"));

	// 81 % 80
	REQUIRE(wint32_81 % int32_80 == wint32("-0x7FFFFFFF"));
	REQUIRE(wint64_81 % int32_80 == wint64("-0x7FFFFFFF"));
	REQUIRE(wint96_81 % int32_80 == wint96("-0x7FFFFFFF"));

	// 81 % 81
	REQUIRE(wint32_81 % int32_81 == wint32("0"));
	REQUIRE(wint64_81 % int32_81 == wint64("-1"));
	REQUIRE(wint96_81 % int32_81 == wint96("-3"));

	// 81 % FF
	REQUIRE(wint32_81 % int32_FF == wint32("0"));
	REQUIRE(wint64_81 % int32_FF == wint32("0"));
	REQUIRE(wint96_81 % int32_FF == wint32("0"));

	// 81 % 01
	REQUIRE(wint32_81 % int32_01 == wint32("0"));
	REQUIRE(wint64_81 % int32_01 == wint32("0"));
	REQUIRE(wint96_81 % int32_01 == wint32("0"));

	// 81 % 7F
	REQUIRE(wint32_81 % int32_7F == wint32("0"));
	REQUIRE(wint64_81 % int32_7F == wint64("-1"));
	REQUIRE(wint96_81 % int32_7F == wint96("-3"));

	// FF % 80
	REQUIRE(wint32_FF % int32_80 == wint32("-1"));
	REQUIRE(wint64_FF % int32_80 == wint64("-1"));
	REQUIRE(wint96_FF % int32_80 == wint96("-1"));

	// FF % 81
	REQUIRE(wint32_FF % int32_81 == wint32("-1"));
	REQUIRE(wint64_FF % int32_81 == wint64("-1"));
	REQUIRE(wint96_FF % int32_81 == wint96("-1"));

	// FF % FF
	REQUIRE(wint32_FF % int32_FF == wint32("0"));
	REQUIRE(wint64_FF % int32_FF == wint64("0"));
	REQUIRE(wint96_FF % int32_FF == wint96("0"));

	// FF % 01
	REQUIRE(wint32_FF % int32_01 == wint32("0"));
	REQUIRE(wint64_FF % int32_01 == wint64("0"));
	REQUIRE(wint96_FF % int32_01 == wint96("0"));

	// FF % 7F
	REQUIRE(wint32_FF % int32_7F == wint32("-1"));
	REQUIRE(wint64_FF % int32_7F == wint64("-1"));
	REQUIRE(wint96_FF % int32_7F == wint96("-1"));

	// 01 % 80
	REQUIRE(wint32_01 % int32_80 == wint32("1"));
	REQUIRE(wint64_01 % int32_80 == wint64("1"));
	REQUIRE(wint96_01 % int32_80 == wint96("1"));

	// 01 % 81
	REQUIRE(wint32_01 % int32_81 == wint32("1"));
	REQUIRE(wint64_01 % int32_81 == wint64("1"));
	REQUIRE(wint96_01 % int32_81 == wint96("1"));

	// 01 % FF
	REQUIRE(wint32_01 % int32_FF == wint32("0"));
	REQUIRE(wint64_01 % int32_FF == wint64("0"));
	REQUIRE(wint96_01 % int32_FF == wint96("0"));

	// 01 % 01
	REQUIRE(wint32_01 % int32_01 == wint32("0"));
	REQUIRE(wint64_01 % int32_01 == wint64("0"));
	REQUIRE(wint96_01 % int32_01 == wint96("0"));

	// 01 % 7F
	REQUIRE(wint32_01 % int32_7F == wint32("1"));
	REQUIRE(wint64_01 % int32_7F == wint64("1"));
	REQUIRE(wint96_01 % int32_7F == wint96("1"));

	// 7F % 80
	REQUIRE(wint32_7F % int32_80 == wint32_7F);
	REQUIRE(wint64_7F % int32_80 == wint64("0x7FFFFFFF"));
	REQUIRE(wint96_7F % int32_80 == wint96("0x7FFFFFFF"));

	// 7F % 81
	REQUIRE(wint32_7F % int32_81 == wint32("0"));
	REQUIRE(wint64_7F % int32_81 == wint64("1"));
	REQUIRE(wint96_7F % int32_81 == wint96("3"));

	// 7F % FF
	REQUIRE(wint32_7F % int32_FF == wint32("0"));
	REQUIRE(wint64_7F % int32_FF == wint32("0"));
	REQUIRE(wint96_7F % int32_FF == wint32("0"));

	// 7F % 01
	REQUIRE(wint32_7F % int32_01 == wint32("0"));
	REQUIRE(wint64_7F % int32_01 == wint32("0"));
	REQUIRE(wint96_7F % int32_01 == wint32("0"));

	// 7F % 7F
	REQUIRE(wint32_7F % int32_7F == wint32("0"));
	REQUIRE(wint64_7F % int32_7F == wint64("1"));
	REQUIRE(wint96_7F % int32_7F == wint96("3"));
}

TEST_CASE("int32 wint modulus limits", "[wint]") {
	// 80 % 80
	REQUIRE(int32_80 % wint32_80 == wint32("0"));

	// 80 % 81
	REQUIRE(int32_80 % wint32_81 == wint32("-1"));

	// 80 % FF overflows

	// 80 % 01
	REQUIRE(int32_80 % wint32("1") == wint32("0"));

	// 80 % 7F
	REQUIRE(int32_80 % wint32_7F == wint32("-1"));

	// 81 % 80
	REQUIRE(int32_81 % wint32_80 == int32_81);

	// 81 % 81
	REQUIRE(int32_81 % wint32_81 == wint32("0"));

	// 81 % FF
	REQUIRE(int32_81 % wint32_FF == wint32("0"));

	// 81 % 01
	REQUIRE(int32_81 % wint32("1") == wint32("0"));

	// 81 % 7F
	REQUIRE(int32_81 % wint32_7F == wint32("0"));

	// FF % 80
	REQUIRE(int32_FF % wint32_80 == wint32("-1"));
	REQUIRE(int32_FF % wint64_80 == wint64("-1"));
	REQUIRE(int32_FF % wint96_80 == wint96("-1"));

	// FF % 81
	REQUIRE(int32_FF % wint32_81 == wint32("-1"));
	REQUIRE(int32_FF % wint64_81 == wint64("-1"));
	REQUIRE(int32_FF % wint96_81 == wint96("-1"));

	// FF % FF
	REQUIRE(int32_FF % wint32_FF == wint32("0"));
	REQUIRE(int32_FF % wint64_FF == wint64("0"));
	REQUIRE(int32_FF % wint96_FF == wint96("0"));

	// FF % 01
	REQUIRE(int32_FF % wint32("1") == wint32("0"));
	REQUIRE(int32_FF % wint64("1") == wint64("0"));
	REQUIRE(int32_FF % wint96("1") == wint96("0"));

	// FF % 7F
	REQUIRE(int32_FF % wint32_7F == wint32("-1"));
	REQUIRE(int32_FF % wint64_7F == wint64("-1"));
	REQUIRE(int32_FF % wint96_7F == wint96("-1"));

	// 01 % 80
	REQUIRE(int32_01 % wint32_80 == wint32("1"));
	REQUIRE(int32_01 % wint64_80 == wint64("1"));
	REQUIRE(int32_01 % wint96_80 == wint96("1"));

	// 01 % 81
	REQUIRE(int32_01 % wint32_81 == wint32("1"));
	REQUIRE(int32_01 % wint64_81 == wint64("1"));
	REQUIRE(int32_01 % wint96_81 == wint96("1"));

	// 01 % FF
	REQUIRE(int32_01 % wint32_FF == wint32("0"));
	REQUIRE(int32_01 % wint64_FF == wint64("0"));
	REQUIRE(int32_01 % wint96_FF == wint96("0"));

	// 01 % 01
	REQUIRE(int32_01 % wint32("1") == wint32("0"));
	REQUIRE(int32_01 % wint64("1") == wint64("0"));
	REQUIRE(int32_01 % wint96("1") == wint96("0"));

	// 01 % 7F
	REQUIRE(int32_01 % wint32_7F == wint32("1"));
	REQUIRE(int32_01 % wint64_7F == wint64("1"));
	REQUIRE(int32_01 % wint96_7F == wint96("1"));

	// 7F % 80
	REQUIRE(int32_7F % wint32_80 == wint32_7F);

	// 7F % 81
	REQUIRE(int32_7F % wint32_81 == wint32("0"));

	// 7F % FF
	REQUIRE(int32_7F % wint32_FF == wint32("0"));

	// 7F % 01
	REQUIRE(int32_7F % wint32("1") == wint32("0"));

	// 7F % 7F
	REQUIRE(int32_7F % wint32_7F == wint32("0"));
}

TEST_CASE("wint int32 bitwise and", "[wint]") {
	REQUIRE((wint32("-1") & 0) == 0);
	REQUIRE((wint64("-1") & 0) == 0);
	REQUIRE((wint96("-1") & 0) == 0);

	REQUIRE((wint32("-1") & -1) == std::int32_t(-1));
	REQUIRE((wint64("-1") & -1) == std::int32_t(-1));
	REQUIRE((wint96("-1") & -1) == std::int32_t(-1));

	REQUIRE((wint32("0x1F2F3F4F") & 0xF0F0F0F0) == 0x10203040);
	REQUIRE((wint64("0x1F2F3F4F5F6F7F8F") & 0xF0F0F0F0) == 0x50607080);
	REQUIRE((wint96("0x1F2F3F4F5F6F7F8F9FAFBFCF") & 0xF0F0F0F0) == 0x90A0B0C0);

	REQUIRE((0xFFFFFFFF & wint32("0")) == wint32("0"));
	REQUIRE((0xFFFFFFFF & wint64("0")) == wint64("0"));
	REQUIRE((0xFFFFFFFF & wint96("0")) == wint96("0"));

	REQUIRE((0xFFFFFFFF & wint32("-1")) == wint32("0xFFFFFFFF"));
	REQUIRE((0xFFFFFFFF & wint64("-1")) == wint64("0xFFFFFFFF"));
	REQUIRE((0xFFFFFFFF & wint96("-1")) == wint96("0xFFFFFFFF"));

	REQUIRE((0xF0F0F0F0 & wint32("0x1F2F3F4F")) == wint32("0x10203040"));
	REQUIRE((0xF0F0F0F0 & wint64("0x1F2F3F4F5F6F7F8F")) == wint64("0x50607080"));
	REQUIRE((0xF0F0F0F0 & wint96("0x1F2F3F4F5F6F7F8F9FAFBFCF")) == wint96("0x90A0B0C0"));
}

TEST_CASE("wint int32 bitwise or", "[wint]") {
	REQUIRE((wint32("0") | -1) == wint32("0xFFFFFFFF"));
	REQUIRE((wint64("0") | -1) == wint64("0xFFFFFFFF"));
	REQUIRE((wint96("0") | -1) == wint96("0xFFFFFFFF"));

	REQUIRE((wint32("-1") | -1) == wint32("-1"));
	REQUIRE((wint64("-1") | -1) == wint64("-1"));
	REQUIRE((wint96("-1") | -1) == wint96("-1"));

	REQUIRE((wint32("0x10203040") | 0x0F0F0F0F) == wint32("0x1F2F3F4F"));
	REQUIRE((wint64("0x1020304050607080") | 0x0F0F0F0F) == wint64("0x102030405F6F7F8F"));
	REQUIRE((wint96("0x102030405060708090A0B0C0") | 0x0F0F0F0F) == wint96("0x10203040506070809FAFBFCF"));

	REQUIRE((0 | wint32("-1")) == wint32("-1"));
	REQUIRE((0 | wint64("-1")) == wint64("-1"));
	REQUIRE((0 | wint96("-1")) == wint96("-1"));

	REQUIRE((0xFFFFFFFF | wint32("-1")) == wint32("-1"));
	REQUIRE((0xFFFFFFFF | wint64("-1")) == wint64("-1"));
	REQUIRE((0xFFFFFFFF | wint96("-1")) == wint96("-1"));

	REQUIRE((0x0F0F0F0F | wint32("0x10203040")) == wint32("0x1F2F3F4F"));
	REQUIRE((0x0F0F0F0F | wint64("0x1020304050607080")) == wint64("0x102030405F6F7F8F"));
	REQUIRE((0x0F0F0F0F | wint96("0x102030405060708090A0B0C0")) == wint96("0x10203040506070809FAFBFCF"));
}

TEST_CASE("wint int32 bitwise xor", "[wint]") {
	REQUIRE((wint32("-1") ^ 0) == wint32("-1"));
	REQUIRE((wint64("-1") ^ 0) == wint64("-1"));
	REQUIRE((wint96("-1") ^ 0) == wint96("-1"));

	REQUIRE((wint32("-1") ^ 0xFFFFFFFF) == wint32("0"));
	REQUIRE((wint64("-1") ^ 0xFFFFFFFF) == wint64("0xFFFFFFFF00000000"));
	REQUIRE((wint96("-1") ^ 0xFFFFFFFF) == wint96("0xFFFFFFFFFFFFFFFF00000000"));

	REQUIRE((wint32("0xAAAAAAAA") ^ 0x3C3C3C3C) == wint32("0x96969696"));
	REQUIRE((wint64("0xAAAAAAAAAAAAAAAA") ^ 0x3C3C3C3C) == wint64("0xAAAAAAAA96969696"));
	REQUIRE((wint96("0xAAAAAAAAAAAAAAAAAAAAAAAA") ^ 0x3C3C3C3C) == wint96("0xAAAAAAAAAAAAAAAA96969696"));

	REQUIRE((0 ^ wint32("-1")) == wint32("-1"));
	REQUIRE((0 ^ wint64("-1")) == wint64("-1"));
	REQUIRE((0 ^ wint96("-1")) == wint96("-1"));

	REQUIRE((0xFFFFFFFF ^ wint32("-1")) == wint32("0"));
	REQUIRE((0xFFFFFFFF ^ wint64("-1")) == wint64("0xFFFFFFFF00000000"));
	REQUIRE((0xFFFFFFFF ^ wint96("-1")) == wint96("0xFFFFFFFFFFFFFFFF00000000"));

	REQUIRE((0x3C3C3C3C ^ wint32("0xAAAAAAAA")) == wint32("0x96969696"));
	REQUIRE((0x3C3C3C3C ^ wint64("0xAAAAAAAAAAAAAAAA")) == wint64("0xAAAAAAAA96969696"));
	REQUIRE((0x3C3C3C3C ^ wint96("0xAAAAAAAAAAAAAAAAAAAAAAAA")) == wint96("0xAAAAAAAAAAAAAAAA96969696"));
}

TEST_CASE("wint is_zero", "[wint]") {
	REQUIRE(!wint32("1").is_zero());
	REQUIRE(!wint64("1").is_zero());
	REQUIRE(!wint96("1").is_zero());

	REQUIRE(!wint32("0x80000000").is_zero());
	REQUIRE(!wint64("0x8000000000000000").is_zero());
	REQUIRE(!wint96("0x800000000000000000000000").is_zero());

	REQUIRE(wint32("0").is_zero());
	REQUIRE(wint64("0").is_zero());
	REQUIRE(wint96("0").is_zero());
}

TEST_CASE("wint is_negative", "[wint]") {
	REQUIRE(!wint32("0").is_negative());
	REQUIRE(!wint64("0").is_negative());
	REQUIRE(!wint96("0").is_negative());

	REQUIRE(wint32("-1").is_negative());
	REQUIRE(wint64("-1").is_negative());
	REQUIRE(wint96("-1").is_negative());

	REQUIRE(!wint32("0x7FFFFFFF").is_negative());
	REQUIRE(!wint64("0x7FFFFFFFFFFFFFFF").is_negative());
	REQUIRE(!wint96("0x7FFFFFFFFFFFFFFFFFFFFFFF").is_negative());

	REQUIRE(wint32("0x80000000").is_negative());
	REQUIRE(wint64("0x8000000000000000").is_negative());
	REQUIRE(wint96("0x800000000000000000000000").is_negative());
}

TEST_CASE("wint getbit", "[wint]") {
	REQUIRE(wint96("0").getbit(0) == 0);
	REQUIRE(wint96("1").getbit(0) == 1);
	REQUIRE(wint96("2").getbit(0) == 0);

	REQUIRE(wint96("0x800000000000").getbit(46) == 0);
	REQUIRE(wint96("0x800000000000").getbit(47) == 1);
	REQUIRE(wint96("0x800000000000").getbit(48) == 0);

	REQUIRE(wint96("0x800000000000000000000000").getbit(94) == 0);
	REQUIRE(wint96("0x800000000000000000000000").getbit(95) == 1);
}

TEST_CASE("wint setbit", "[wint]") {
	REQUIRE(wint96("0").setbit(0) == wint96("1"));
	REQUIRE(wint96("0").setbit(1) == wint96("2"));
	REQUIRE(wint96("1").setbit(0) == wint96("1"));
	REQUIRE(wint96("2").setbit(0) == wint96("3"));

	REQUIRE(wint96("0").setbit(47) == wint96("0x800000000000"));

	REQUIRE(wint96("0").setbit(95) == wint96("0x800000000000000000000000"));
}

TEST_CASE("wint abs", "[wint]") {
	REQUIRE(abs(wint96("0")) == wint96("0"));
	REQUIRE(abs(wint96("1")) == wint96("1"));
	REQUIRE(abs(wint96("-1")) == wint96("1"));
	REQUIRE(abs(wint96_80) == wint96_80);
	REQUIRE(abs(wint96_81) == wint96_7F);
	REQUIRE(abs(wint96_7F) == wint96_7F);
}

TEST_CASE("wint min", "[wint]") {
	REQUIRE(min(wint96("0"), wint96("0")) == wint96("0"));
	REQUIRE(min(wint96("1"), wint96("0")) == wint96("0"));
	REQUIRE(min(wint96("0"), wint96("1")) == wint96("0"));
	REQUIRE(min(wint96("0"), wint96("-1")) == wint96("-1"));
	REQUIRE(min(wint96("-1"), wint96("-2")) == wint96("-2"));
	REQUIRE(min(wint96("0x1000000000000"), wint96("0xFFFFFFFFFFFF")) == wint96("0xFFFFFFFFFFFF"));
	REQUIRE(min(wint96("0x1000000000000"), wint96("0x1000000000001")) == wint96("0x1000000000000"));
	REQUIRE(min(wint96_80, wint96_81) == wint96_80);
	REQUIRE(min(wint96_81, wint96_FF) == wint96_81);
	REQUIRE(min(wint96_FF, wint96_7F) == wint96_FF);
}

TEST_CASE("wint max", "[wint]") {
	REQUIRE(max(wint96("0"), wint96("0")) == wint96("0"));
	REQUIRE(max(wint96("1"), wint96("0")) == wint96("1"));
	REQUIRE(max(wint96("0"), wint96("1")) == wint96("1"));
	REQUIRE(max(wint96("0"), wint96("-1")) == wint96("0"));
	REQUIRE(max(wint96("-1"), wint96("-2")) == wint96("-1"));
	REQUIRE(max(wint96("0x1000000000000"), wint96("0xFFFFFFFFFFFF")) == wint96("0x1000000000000"));
	REQUIRE(max(wint96("0x1000000000000"), wint96("0x1000000000001")) == wint96("0x1000000000001"));
	REQUIRE(max(wint96_80, wint96_81) == wint96_81);
	REQUIRE(max(wint96_81, wint96_FF) == wint96_FF);
	REQUIRE(max(wint96_FF, wint96_7F) == wint96_7F);
}

TEST_CASE("wint from_chars 10", "[wint]") {
	using record = std::pair<std::string, std::string>;

	auto data = GENERATE(
		record{"0", "0"},
		record{"1", "1"},
		record{"0001", "1"},
		record{"286335522", "286335522"},
		record{"3689367580026693222", "3689367580026693222"},
		record{"36973223102941133555797576908", "36973223102941133555797576908"},
		record{"39614081257132168796771975167", "39614081257132168796771975167"},
		record{"-1", "-1"},
		record{"-0001", "-1"},
		record{"-39614081257132168796771975167", "-39614081257132168796771975167"},
		record{"-39614081257132168796771975168", "-39614081257132168796771975168"}
	);

	const auto [value, expected] = data;

	wint96 res(42);

	auto [ptr, ec] = from_chars(value.data(), value.data() + value.size(), res, 10);

	REQUIRE(ec == std::errc());
	REQUIRE(ptr == value.data() + value.size());
	REQUIRE(res == wint96(expected));
}

TEST_CASE("wint from_chars 10 end", "[wint]") {
	auto str = GENERATE(as<std::string>{},
		"0abc",
		"1abc",
		"286335522abc",
		"-39614081257132168796771975168abc",
		"-1abc"
	);

	wint96 res(0);

	auto [ptr, ec] = from_chars(str.data(), str.data() + str.size(), res, 10);

	REQUIRE(ec == std::errc());
	REQUIRE(ptr == str.data() + str.size() - 3);
	REQUIRE(res == wint96(str.substr(0, str.size() - 3)));
}

TEST_CASE("wint from_chars 10 overflow", "[wint]") {
	auto str = GENERATE(as<std::string>{},
		"39614081257132168796771975168",
		"-39614081257132168796771975169"
	);

	wint96 res(42);

	auto [ptr, ec] = from_chars(str.data(), str.data() + str.size(), res, 10);

	REQUIRE(ec == std::errc::result_out_of_range);
	REQUIRE(ptr == str.data() + str.size());
	REQUIRE(res == 42);
}

TEST_CASE("wint from_chars 10 invalid", "[wint]") {
	auto str = GENERATE(as<std::string>{},
		"",
		"abc",
		"-abc",
		"+1",
		" 1"
	);

	wint96 res(42);

	auto [ptr, ec] = from_chars(str.data(), str.data() + str.size(), res, 10);

	REQUIRE(ec == std::errc::invalid_argument);
	REQUIRE(ptr == str.data());
	REQUIRE(res == 42);
}

TEST_CASE("wint from_chars 16", "[wint]") {
	using record = std::pair<std::string, std::string>;

	auto data = GENERATE(
		record{"0", "0"},
		record{"1", "1"},
		record{"11112222", "286335522"},
		record{"3333444455556666", "3689367580026693222"},
		record{"777788889999AaAaBbBbCcCc", "36973223102941133555797576908"},
		record{"7fffffffffffffffffffffff", "39614081257132168796771975167"},
		record{"-1", "-1"},
		record{"-7fffffffffffffffffffffff", "-39614081257132168796771975167"},
		record{"-800000000000000000000000", "-39614081257132168796771975168"}
	);

	const auto [value, expected] = data;

	wint96 res(0);

	auto [ptr, ec] = from_chars(value.data(), value.data() + value.size(), res, 16);

	REQUIRE(ec == std::errc());
	REQUIRE(ptr == value.data() + value.size());
	REQUIRE(res == wint96(expected));
}

TEST_CASE("wint from_chars 7", "[wint]") {
	using record = std::pair<std::string, std::string>;

	auto data = GENERATE(
		record{"0", "0"},
		record{"1", "1"},
		record{"10044545304", "286335522"},
		record{"6414422622333331211340", "3689367580026693222"},
		record{"4532246320532121443535152360536011", "36973223102941133555797576908"},
		record{"5060360422412213131405631055526153", "39614081257132168796771975167"},
		record{"-1", "-1"},
		record{"-5060360422412213131405631055526153", "-39614081257132168796771975167"},
		record{"-5060360422412213131405631055526154", "-39614081257132168796771975168"}
	);

	const auto [value, expected] = data;

	wint96 res(42);

	auto [ptr, ec] = from_chars(value.data(), value.data() + value.size(), res, 7);

	REQUIRE(ec == std::errc());
	REQUIRE(ptr == value.data() + value.size());
	REQUIRE(res == wint96(expected));
}

TEST_CASE("wint to_chars 10", "[wint]") {
	using record = std::pair<std::string, std::string>;

	auto data = GENERATE(
		record{"0", "0"},
		record{"1", "1"},
		record{"286335522", "286335522"},
		record{"3689367580026693222", "3689367580026693222"},
		record{"36973223102941133555797576908", "36973223102941133555797576908"},
		record{"39614081257132168796771975167", "39614081257132168796771975167"},
		record{"-1", "-1"},
		record{"-39614081257132168796771975167", "-39614081257132168796771975167"},
		record{"-39614081257132168796771975168", "-39614081257132168796771975168"}
	);

	const auto [value, expected] = data;

	std::string res(expected.size(), '?');

	auto [ptr, ec] = to_chars(res.data(), res.data() + res.size(), wint96(value), 10);

	REQUIRE(ec == std::errc());
	REQUIRE(ptr == res.data() + expected.size());
	REQUIRE(res == expected);
}

TEST_CASE("wint to_chars 10 end", "[wint]") {
	using record = std::pair<std::string, std::string>;

	auto data = GENERATE(
		record{"0", "0"},
		record{"1", "1"},
		record{"286335522", "286335522"},
		record{"3689367580026693222", "3689367580026693222"},
		record{"36973223102941133555797576908", "36973223102941133555797576908"},
		record{"39614081257132168796771975167", "39614081257132168796771975167"},
		record{"-1", "-1"},
		record{"-39614081257132168796771975167", "-39614081257132168796771975167"},
		record{"-39614081257132168796771975168", "-39614081257132168796771975168"}
	);

	const auto [value, expected] = data;

	std::string res(expected.size() + 1, '?');

	auto [ptr, ec] = to_chars(res.data(), res.data() + res.size(), wint96(value), 10);

	REQUIRE(ec == std::errc());
	REQUIRE(ptr == res.data() + expected.size());
	REQUIRE(res.substr(0, expected.size()) == expected);
	REQUIRE(res[expected.size()] == '?');
}

TEST_CASE("wint to_chars 10 size", "[wint]") {
	using record = std::pair<std::string, std::string>;

	auto data = GENERATE(
		record{"0", "0"},
		record{"1", "1"},
		record{"286335522", "286335522"},
		record{"3689367580026693222", "3689367580026693222"},
		record{"36973223102941133555797576908", "36973223102941133555797576908"},
		record{"39614081257132168796771975167", "39614081257132168796771975167"},
		record{"-1", "-1"},
		record{"-39614081257132168796771975167", "-39614081257132168796771975167"},
		record{"-39614081257132168796771975168", "-39614081257132168796771975168"}
	);

	const auto [value, expected] = data;

	std::string res(expected.size() - 1, '?');

	auto [ptr, ec] = to_chars(res.data(), res.data() + res.size(), wint96(value), 10);

	REQUIRE(ec == std::errc::value_too_large);
	REQUIRE(ptr == res.data() + res.size());
}

TEST_CASE("wint to_chars 16", "[wint]") {
	using record = std::pair<std::string, std::string>;

	auto data = GENERATE(
		record{"0", "0"},
		record{"1", "1"},
		record{"286335522", "11112222"},
		record{"3689367580026693222", "3333444455556666"},
		record{"36973223102941133555797576908", "777788889999aaaabbbbcccc"},
		record{"39614081257132168796771975167", "7fffffffffffffffffffffff"},
		record{"-1", "-1"},
		record{"-39614081257132168796771975167", "-7fffffffffffffffffffffff"},
		record{"-39614081257132168796771975168", "-800000000000000000000000"}
	);

	const auto [value, expected] = data;

	std::string res(expected.size(), '?');

	auto [ptr, ec] = to_chars(res.data(), res.data() + res.size(), wint96(value), 16);

	REQUIRE(ec == std::errc());
	REQUIRE(ptr == res.data() + expected.size());
	REQUIRE(res == expected);
}

TEST_CASE("wint to_chars 7", "[wint]") {
	using record = std::pair<std::string, std::string>;

	auto data = GENERATE(
		record{"0", "0"},
		record{"1", "1"},
		record{"286335522", "10044545304"},
		record{"3689367580026693222", "6414422622333331211340"},
		record{"36973223102941133555797576908", "4532246320532121443535152360536011"},
		record{"39614081257132168796771975167", "5060360422412213131405631055526153"},
		record{"-1", "-1"},
		record{"-39614081257132168796771975167", "-5060360422412213131405631055526153"},
		record{"-39614081257132168796771975168", "-5060360422412213131405631055526154"}
	);

	const auto [value, expected] = data;

	std::string res(expected.size(), '?');

	auto [ptr, ec] = to_chars(res.data(), res.data() + res.size(), wint96(value), 7);

	REQUIRE(ec == std::errc());
	REQUIRE(ptr == res.data() + expected.size());
	REQUIRE(res == expected);
}

TEST_CASE("wint to_string", "[wint]") {
	auto str = GENERATE(as<std::string>{},
		"0",
		"1",
		"-1",
		"286335522",
		"-286335522",
		"3689367580026693222",
		"-3689367580026693222",
		"36973223102941133555797576908",
		"39614081257132168796771975167",
		"-39614081257132168796771975167",
		"-39614081257132168796771975168"
	);

	const auto value = wint96(str);

	REQUIRE(to_string(value) == str);
}

TEST_CASE("wint to_string max digits", "[wint]") {
	REQUIRE(to_string(wint32_7F) == "2147483647");
	REQUIRE(to_string(wint32_80) == "-2147483648");
}

TEST_CASE("wint stream output", "[wint]") {
	auto str = GENERATE(as<std::string>{},
		"0",
		"1",
		"-1",
		"286335522",
		"-286335522",
		"3689367580026693222",
		"-3689367580026693222",
		"36973223102941133555797576908",
		"39614081257132168796771975167",
		"-39614081257132168796771975167",
		"-39614081257132168796771975168"
	);

	std::ostringstream out;

	out << wint96(str);

	REQUIRE(out.str() == str);
}

TEST_CASE("wint stream output hex", "[wint]") {
	using record = std::pair<std::string, std::string>;

	auto data = GENERATE(
		record{"0", "0"},
		record{"1", "1"},
		record{"286335522", "11112222"},
		record{"3689367580026693222", "3333444455556666"},
		record{"36973223102941133555797576908", "777788889999aaaabbbbcccc"},
		record{"39614081257132168796771975167", "7fffffffffffffffffffffff"},
		record{"-1", "-1"},
		record{"-39614081257132168796771975167", "-7fffffffffffffffffffffff"},
		record{"-39614081257132168796771975168", "-800000000000000000000000"}
	);

	const auto [value, expected] = data;

	std::ostringstream out;

	out << std::hex << wint96(value);

	REQUIRE(out.str() == expected);
}

TEST_CASE("wint stream output oct", "[wint]") {
	using record = std::pair<std::string, std::string>;

	auto data = GENERATE(
		record{"0", "0"},
		record{"1", "1"},
		record{"286335522", "2104221042"},
		record{"3689367580026693222", "314632104212525263146"},
		record{"36973223102941133555797576908", "35673610421146315252527356746314"},
		record{"39614081257132168796771975167", "37777777777777777777777777777777"},
		record{"-1", "-1"},
		record{"-39614081257132168796771975167", "-37777777777777777777777777777777"},
		record{"-39614081257132168796771975168", "-40000000000000000000000000000000"}
	);

	const auto [value, expected] = data;

	std::ostringstream out;

	out << std::oct << wint96(value);

	REQUIRE(out.str() == expected);
}

TEST_CASE("wint stream input", "[wint]") {
	auto str = GENERATE(as<std::string>{},
		"0",
		"1",
		"-1",
		"286335522",
		"-286335522",
		"3689367580026693222",
		"-3689367580026693222",
		"36973223102941133555797576908",
		"39614081257132168796771975167",
		"-39614081257132168796771975167",
		"-39614081257132168796771975168"
	);

	std::istringstream in(str);

	wint96 value(42);

	REQUIRE(in >> value);

	REQUIRE(value == wint96(str));
}

TEST_CASE("std::hash<wint>", "[wint]") {
	REQUIRE(std::hash<wint32>()(wint32("123")) == std::hash<wint32>()(wint32("123")));
	REQUIRE(std::hash<wint64>()(wint64("123")) == std::hash<wint64>()(wint64("123")));
	REQUIRE(std::hash<wint96>()(wint96("123")) == std::hash<wint96>()(wint96("123")));

	REQUIRE(std::hash<wint32>()(wint32("123")) != std::hash<wint32>()(wint32("456")));
	REQUIRE(std::hash<wint64>()(wint64("123")) != std::hash<wint64>()(wint64("456")));
	REQUIRE(std::hash<wint96>()(wint96("123")) != std::hash<wint96>()(wint96("456")));

	std::unordered_set<wint64> set;

	set.insert(wint64("0"));
	set.insert(wint64("1"));
	set.insert(wint64("-1"));
	set.insert(wint64("0x8000000000000000"));

	REQUIRE(set.size() == 4);
}

TEST_CASE("wint factorial", "[wint]") {
	REQUIRE(fac(50) == wint256("30414093201713378043612608166064768844377641568960512000000000000"));

	REQUIRE(fac(34) / fac(30) == 34 * 33 * 32 * 31);
}

TEST_CASE("wint modinv", "[wint]") {
	auto str = GENERATE(as<std::string>{},
		"1",
		"2",
		"286335522",
		"3689367580026693222",
		"9223372036854775336"
	);

	static constexpr wint128 n("9223372036854775337");

	wint128 value(str);

	wint128 value_inv = modinv(value, n);

	REQUIRE(value_inv > 0);
	REQUIRE(value_inv < n);
	REQUIRE((value * value_inv) % n == 1);
}

TEST_CASE("wint crypt", "[wint]") {
	static constexpr wint256 p("9223372036854775337");
	static constexpr wint256 q("4611686018427387847");

	wint256 n = p * q;

	wint256 e(65537);

	wint256 phi = (p - 1) * (q - 1);

	wint256 d = modinv(e, phi);

	wint256 message(42);

	wint256 ciphertext = modexp(message, e, n);

	wint256 plaintext = modexp(ciphertext, d, n);

	REQUIRE(plaintext == message);
}
