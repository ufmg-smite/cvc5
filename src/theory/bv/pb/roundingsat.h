/******************************************************************************
 * Top contributors (to current version):
 *   Alan Prado, Pedro Saccomani
 *
 * This file is part of the cvc5 project.
 *
 * Copyright (c) 2009-2026 by the authors listed in the file AUTHORS
 * in the top-level source directory and their institutional affiliations.
 * All rights reserved.  See the file COPYING in the top-level source
 * directory for licensing information.
 * ****************************************************************************
 *
 * Wrapper for the RoundingSat PB Solver for cvc5 (bit-vectors).
 *
 */

#include "cvc5_private.h"

#ifdef CVC5_USE_ROUNDINGSAT

#ifndef CVC5__THEORY__BV__PB__ROUNDINGSAT_H
#define CVC5__THEORY__BV__PB__ROUNDINGSAT_H

#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "smt/env_obj.h"
#include "theory/bv/pb/pb_solver.h"
#include "util/integer.h"

namespace rs::api {
class PbSolver;
}

namespace cvc5::internal {
namespace theory {
namespace bv {
namespace pb {

/**
 * In-process wrapper around RoundingSat's `rs::api::PbSolver` library. Constraint
 * Nodes (linear forms over Boolean PB variables) are translated into the solver's
 * native representation and solved without spawning an external process.
 */
class RoundingSatSolver : public PseudoBooleanSolver<Node>, protected EnvObj
{
  // friend class PbSolverFactory;

 public:
  ~RoundingSatSolver() override;

  /* RoundingSatSolver interface -------------------------------------------- */
  void addConstraint(const Node constraint) override;
  void addConstraint(const Node constraint, const Node selector) override;
  void addVariable(const Node variable) override;
  PbSolveState solve() override;
  PbSolveState solve(const std::vector<Node>& assumptions) override;
  std::vector<Node> getUnsatCore() override;
  bool supportsCores() const override { return true; }
  PbValue modelValue(const VariableId variable) override;
  void reset() override;
  std::vector<std::string> getProof() override;

  // private:   TODO: should the constructor be private (factory)?
  /**
   * Constructor.
   * @param env       The associated environment.
   * @param registry  The associated statistics registry.
   * @param name      The name of the PB solver.
   * @param logProofs Whether to log proofs.
   */
  RoundingSatSolver(Env& env,
                    StatisticsRegistry& registry,
                    const std::string& name = "",
                    bool logProofs = false);

 private:
  /** rs::api::Var, kept as int to avoid leaking the RoundingSat header. */
  using RsVar = int;

  /** Return the RoundingSat variable for `node`, allocating it if needed. */
  RsVar toRsVar(const Node& node);

  /**
   * Decompose a constraint Node into integer (coefficient, variable) terms and
   * its right-hand side, allocating RoundingSat variables as needed. Returns
   * the relation kind (Kind::GEQ or Kind::EQUAL).
   */
  Kind parseConstraint(const Node& constraint,
                       std::vector<std::pair<Integer, RsVar>>& terms,
                       Integer& rhs);

  /**
   * Add the guarded constraint `sum(terms) >= rhs`, relaxed when `selector` is
   * false. Encoded as `terms ∪ {(L - rhs, selector)} >= L`, where L is the sum
   * of the negative coefficients (the minimum of the left-hand side): with the
   * selector true this is the original constraint, with it false the
   * constraint reduces to `sum >= L`, which always holds.
   */
  void addGuardedGeq(const std::vector<std::pair<Integer, RsVar>>& terms,
                     const Integer& rhs,
                     RsVar selector);

  /** The underlying RoundingSat solver. */
  std::unique_ptr<rs::api::PbSolver> d_solver;

  /** Whether we are logging proofs. */
  bool d_logProofs;

  /** TODO: implement statistics */
  struct Statistics
  {
    Statistics(StatisticsRegistry& registry, const std::string& prefix);
  };
  Statistics d_statistics;

  /** Maps each cvc5 PB variable to its RoundingSat variable id. */
  std::unordered_map<Node, RsVar> d_nodeToVar;

  /** Reverse of d_nodeToVar, for mapping unsat-core literals back to Nodes. */
  std::unordered_map<RsVar, Node> d_varToNode;

  /** Maps a variable's string id (its toString()) to its RoundingSat variable. */
  std::unordered_map<VariableId, RsVar> d_nameToVar;

  /** Set of constraints already in the solver. */
  std::unordered_set<Node> d_constraintSet;
};

}  // namespace pb
}  // namespace bv
}  // namespace theory
}  // namespace cvc5::internal

#endif  // CVC5__THEORY__BV__PB__ROUNDINGSAT_H
#endif  // CVC5_USE_ROUNDINGSAT
