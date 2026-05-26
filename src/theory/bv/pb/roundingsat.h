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
 * Wrapper for the RoundingSat PB Solver for cvc5 (bit-vectors).
 *
 */

#include "cvc5_private.h"

#ifdef CVC5_USE_ROUNDINGSAT

#ifndef CVC5__THEORY__BV__PB__ROUNDINGSAT_H
#define CVC5__THEORY__BV__PB__ROUNDINGSAT_H

#include "smt/env_obj.h"
#include "theory/bv/pb/pb_solver.h"

namespace cvc5::internal {
namespace theory {
namespace bv {
namespace pb {

class RoundingSatSolver : public PseudoBooleanSolver<Node>, protected EnvObj
{
  // friend class PbSolverFactory;

 public:
  ~RoundingSatSolver() override = default;

  /* RoundingSatSolver interface -------------------------------------------- */
  void addConstraint(const Node constraint) override;
  void addVariable(const Node variable) override;
  PbSolveState solve() override;
  PbValue modelValue(const VariableId variable) override;
  void reset() override;
  std::vector<std::string> getProof() override;

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
  RoundingSatSolver(std::string solverPath,
                    Env& env,
                    StatisticsRegistry& registry,
                    const std::string& name = "",
                    bool logProofs = false);

 private:
  std::string buildCliCommand(std::string input_path,
                              std::string output_path,
                              std::string proof_path);
  void computeSatisfyingAssignment();
  PbSolveState parseOutput(std::string output_path);
  void parseProof(std::string proof_path);
  void writeInput(std::string input_path);

  std::string d_binPath;

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

  /** TODO(alanctprado) */
  std::vector<std::string> d_pboConstraints;

  /** TODO(alanctprado) */
  std::vector<std::string> d_proofLines;

  /** Assignment map */
  std::unordered_map<VariableId, PbValue> d_assignmentMap;
};

}  // namespace pb
}  // namespace bv
}  // namespace theory
}  // namespace cvc5::internal

#endif  // CVC5__THEORY__BV__PB__ROUDNINGSAT_H
#endif  // CVC5_USE_ROUDNINGSAT
