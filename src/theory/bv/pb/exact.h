/******************************************************************************
 * Top contributors (to current version):
 *   Aina Niemetz, Mathias Preiner, Andrew Reynolds
 *
 * This file is part of the cvc5 project.
 *
 * Copyright (c) 2009-2026 by the authors listed in the file AUTHORS
 * in the top-level source directory and their institutional affiliations.
 * All rights reserved.  See the file COPYING in the top-level source
 * directory for licensing information.
 * ****************************************************************************
 *
 * Wrapper for the Exact PB Solver for cvc5 (bit-vectors).
 *
 */

#include "cvc5_private.h"

#ifdef CVC5_USE_EXACT

#ifndef CVC5__THEORY__BV__PB__EXACT_H
#define CVC5__THEORY__BV__PB__EXACT_H

#include <Exact.hpp>

// #include "context/cdhashset.h"
#include "smt/env_obj.h"
#include "theory/bv/pb/pb_solver.h"

namespace cvc5::internal {
namespace theory {
namespace bv {
namespace pb {

// class CadicalPropagator;

class ExactSolver : public PseudoBooleanSolver<Node>, protected EnvObj
{
  // friend class PbSolverFactory;

 public:
  ~ExactSolver() override = default;

  /* ExactSolver interface -------------------------------------------------- */
  void addConstraint(const Node constraint) override;
  void addVariable(const Node variable) override;
  PbSolveState solve() override;

  // private:   TODO: should the constructor be private (factory)?
  /**
   * Constructor.
   * Private to disallow creation outside of PbSolverFactory.
   * Function init() must be called after creation.
   * @param env       The associated environment.
   * @param registry  The associated statistics registry.
   * @param name      The name of the PB solver.
   * @param logProofs Whether to log proofs
   */
  ExactSolver(Env& env,
              StatisticsRegistry& registry,
              const std::string& name = "",
              bool logProofs = false);

 private:
  /**
   * Initialize PB solver instance.
   * Note: Split out to not call virtual functions in constructor.
   */
  void init();

  /** The wrapped Exact instance. */
  std::unique_ptr<Exact> d_solver;

  /** Whether we are logging proofs */
  bool d_logProofs;

  /** TODO: implement statistics */
  struct Statistics
  {
    Statistics(StatisticsRegistry& registry, const std::string& prefix);
  };
  Statistics d_statistics;

  /** Set of variables already in the solver */
  std::unordered_set<Node> d_variableSet;

  /** Set of constraints already in the solver */
  std::unordered_set<Node> d_constraintSet;
};

}  // namespace pb
}  // namespace bv
}  // namespace theory
}  // namespace cvc5::internal

#endif  // CVC5__THEORY__BV__PB__EXACT_H
#endif  // CVC5_USE_EXACT
