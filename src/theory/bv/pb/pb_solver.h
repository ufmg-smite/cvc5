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
  /**
   * Assert a constraint guarded by `selector`: when `selector` is assumed
   * false, the constraint is relaxed (trivially satisfied). Backends that
   * support unsat cores use this to make each constraint toggleable; the
   * default ignores the selector and asserts the constraint unconditionally.
   */
  virtual void addConstraint(const T constraint, const T /*selector*/)
  {
    addConstraint(constraint);
  }
  /** Check the satisfiability of the added constraints. */
  virtual PbSolveState solve() = 0;
  /**
   * Check satisfiability under the given assumption literals (e.g. selectors).
   * The default ignores the assumptions and solves unconditionally.
   */
  virtual PbSolveState solve(const std::vector<T>& /*assumptions*/)
  {
    return solve();
  }
  /**
   * The subset of the last solve()'s assumptions that forms an unsat core.
   * Valid after solve(assumptions) returned PB_UNSAT. Empty if unsupported.
   */
  virtual std::vector<T> getUnsatCore() { return {}; }
  /** Whether this backend can produce unsat cores via solve(assumptions). */
  virtual bool supportsCores() const { return false; }
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
