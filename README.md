
wideint - wide exact-width integer types
========================================

Copyright (c) 2022 Joergen Ibsen


About
-----

wideint is a C++ implementation of wide *exact-width* unsigned integer types.

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

wideint is not production quality. It has not been suitably tested, and
some of the algorithms used are slow compared to what you would get with
one of the established big number libraries. Also, being exact-width and
stack-allocated means it is only suitable for bounded computations on
relatively small values.


Requirements
------------

wideint requires `std::uint32_t` and `std::uint64_t` to be available. It
uses some C++20 features which may not be available in all compilers at
the moment.


Details
-------

The unsigned integer type `wuint` stores its value in a `std::array` of
`std::uint32_t`. It takes the size of this array as a template argument.
So a `wuint<3>`, for example, stores a 96-bit value, while a `wuint<4>`
stores a 128-bit value.

While it is possible to create a `wuint` with 32- or 64 bits, use the
built-in types `std::uint32_t` and `std::uint64_t` instead.

As special cases, modulus (`wuint % uint32_t`) and bitwise AND
(`wuint & uint32_t`) with a `std::uint32_t` on the right return a
`std::uint32_t` instead of a `wuint`.

Most operations are `constexpr`.


Signed values
-------------

The current version of wideint does not include a signed integer type. But
it is possible to perform some signed computations using `wuint`, because
the bit patterns can be interpreted as two's complement representations.

Specifically, operations like plus, minus, and multiply between wuints should
work. For other computations there are functions `iabs()`, `idiv()`,
`imod()`, `shiftar()`, and a `is_inegative()` method.

~~~.cpp
#include <iostream>
#include <utility>
#include "wideint.hpp"

using wideint::wuint;

template<std::size_t width>
constexpr wuint<width> gcd(const wuint<width> &x, const wuint<width> &y)
{
	auto a = x;
	auto b = y;

	while (b != 0) {
		a = std::exchange(b, imod(a, b));
	}

	return iabs(a);
}

int main() {
	using uint128 = wideint::wuint<4>;

	constexpr auto p = uint128("-9223372036854775337");
	constexpr auto q = uint128("4611686018427387847");
	constexpr auto r = uint128("2305843009213693907");

	// prints 2305843009213693907
	std::cout << gcd(p * r, q * r) << '\n';
}
~~~


Output
------

There is a `to_string()` function that converts a `wuint` to a `std::string`.

There are operator overloads for stream input and output.


Alternatives
------------

  - [GMP](https://gmplib.org/)
  - [InfInt](https://github.com/sercantutar/infint)
  - [mp++](https://github.com/bluescarni/mppp)
  - [MPIR](https://github.com/wbhart/mpir)
  - [wide-integer](https://github.com/ckormanyos/wide-integer)
