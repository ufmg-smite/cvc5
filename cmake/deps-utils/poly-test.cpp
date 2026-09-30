/******************************************************************************
 * Top contributors (to current version):
 *   Pedro Saccomani
 *
 * This file is part of the cvc5 project.
 *
 * Copyright (c) 2009-2025 by the authors listed in the file AUTHORS
 * in the top-level source directory and their institutional affiliations.
 * All rights reserved.  See the file COPYING in the top-level source
 * directory for licensing information.
 * ****************************************************************************
 *
 * Test that the LibPoly libraries provide the functions used by cvc5 that are
 * not available in older versions. The declarations are given here instead of
 * including the LibPoly headers, since the headers may belong to a different
 * installation than the libraries, which is exactly what this test detects.
 */

#include <cstddef>
#include <vector>

extern "C" {
struct lp_upolynomial_struct;
void lp_upolynomial_signed_remainder_sequence(
    const lp_upolynomial_struct* f,
    const lp_upolynomial_struct* g,
    lp_upolynomial_struct*** S,
    std::size_t* size);
}

namespace poly {
class UPolynomial;
std::vector<UPolynomial> signed_remainder_sequence(const UPolynomial& p,
                                                   const UPolynomial& q);
}  // namespace poly

int main()
{
  void* c = reinterpret_cast<void*>(&lp_upolynomial_signed_remainder_sequence);
  void* cxx = reinterpret_cast<void*>(&poly::signed_remainder_sequence);
  return c != nullptr && cxx != nullptr ? 0 : 1;
}
