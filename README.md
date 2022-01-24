
wideint - wide exact-width integer types
========================================

Copyright (c) 2022 Joergen Ibsen


About
-----

wideint is a C++ implementation of wide *exact-width* integer types.

~~~.cpp
#include <iostream>
#include "wideint.hpp"

int main() {
	using uint128 = wideint::wuint<4>;

	constexpr auto p = uint128("9223372036854775337");
	constexpr auto q = uint128("4611686018427387847");

	// prints 42535295865117305235085505148949129439
	std::cout << p * q << '\n';
}
~~~

While doing [Advent of Code](https://adventofcode.com/) to pick up some
C++20, I came across a problem where I wanted to do a computation on values
larger than what fits in an `unsigned long long`. The C++ standard library
does not include a big integer type yet, so I would either have to use one
of the stable, well-tested, highly efficient libraries available, or write
my own ad hoc solution, and hopefully learn a few things along the way.


Caveat
------

wideint is not production quality. It has not been thoroughly tested, and
some of the algorithms used may be slow compared to what you would get with
one of the established big number libraries. Also, being exact-width and
stack-allocated means it is only suitable for bounded computations on
relatively small values.


Requirements
------------

wideint requires `std::uint32_t` and `std::uint64_t` to be available. It
uses some C++20 features which may not be available in all compilers.


Details
-------

A wideint stores its value in a `std::array` of `std::uint32_t`. It takes
the size of this array as a template argument. So a `wuint<3>`, for example,
stores a 96-bit value, while a `wuint<4>` stores a 128-bit value.

While it is possible to create a wideint with 32- or 64 bits, use the
built-in types `std::uint32_t` and `std::uint64_t` instead.

There is no implicit conversion from `std::uint32_t` or `std::string_view`
to wideint. If you want to initialize a wideint with either, you will have
to do so explicitly:

~~~.cpp
// This does not compile, there is no implicit conversion
// uint128 a = 5;

// Use either of these instead
uint128 b(5);
uint128 c = uint128(5);
auto d = uint128(5);
~~~

As special cases, modulus (`wuint % uint32_t`) and bitwise AND
(`wuint & uint32_t`) with a `std::uint32_t` on the right return a
`std::uint32_t` instead of a `wuint`.

Most operations are `constexpr`.


Signed values
-------------

There is a signed wideint type, `wideint::wint`, which interprets the value
it stores as a two's complement representation.

~~~.cpp
#include <iostream>
#include <utility>
#include "wideint.hpp"

using wideint::wint;

template<std::size_t width>
constexpr wint<width> gcd(const wint<width> &x, const wint<width> &y)
{
	auto a = x;
	auto b = y;

	while (b != 0) {
		a = std::exchange(b, a % b);
	}

	return abs(a);
}

int main() {
	using int128 = wideint::wint<4>;

	constexpr auto p = int128("-9223372036854775337");
	constexpr auto q = int128("4611686018427387847");
	constexpr auto r = int128("2305843009213693907");

	// prints 2305843009213693907
	std::cout << gcd(p * r, q * r) << '\n';
}
~~~


Output
------

There is a `to_string()` function that converts a wideint to a `std::string`.

There are operator overloads for stream input and output.


Alternatives
------------

  - [GMP](https://gmplib.org/)
  - [InfInt](https://github.com/sercantutar/infint)
  - [mp++](https://github.com/bluescarni/mppp)
  - [MPIR](https://github.com/wbhart/mpir)
  - [wide-integer](https://github.com/ckormanyos/wide-integer)
