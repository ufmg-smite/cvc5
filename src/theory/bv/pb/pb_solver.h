/******************************************************************************
 * Top contributors (to current version):
 *   Alan Prado
 *
 * This file is part of the cvc5 project.
 *
 * Copyright (c) 2009-2026 by the authors listed in the file AUTHORS
 * in the top-level source directory and their institutional affiliations.
 * All rights reserved.  See the file COPYING in the top-level source
 * directory for licensing information.
 * ****************************************************************************
 *
 * Pb Solver.
 */

#include "cvc5_private.h"

#ifndef CVC5__THEORY__BV__PB__PB_SOLVER_H
#define CVC5__THEORY__BV__PB__PB_SOLVER_H

#include <string>
#include <vector>

#include "theory/bv/pb/pb_solver_types.h"

namespace cvc5::internal {
namespace theory {
namespace bv {
namespace pb {

template <class T>
class PseudoBooleanSolver
{
 public:
  /** Virtual destructor */
  virtual ~PseudoBooleanSolver() {}
  /** Add a variable to the solver. */
  virtual void addVariable(const T variable) = 0;
  /** Assert a constraint to the solver. */
  virtual void addConstraint(const T constraint) = 0;
  /** Check the satisfiability of the added clauses */
  virtual PbSolveState solve() = 0;
  /** Call modelValue() when the search is done. */
  virtual PbValue modelValue(const VariableId variable) = 0;
  /** Reset solver. */
  virtual void reset() = 0;
  /** Get proof lines. */
  virtual std::vector<std::string> getProof() = 0;

 private:
  void init();
};

}  // namespace pb
}  // namespace bv
}  // namespace theory
}  // namespace cvc5::internal

#endif  // CVC5__THEORY__BV__PB__PB_SOLVER_H
