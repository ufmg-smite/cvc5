/******************************************************************************
 * Top contributors (to current version):
 *   Alan Prado
 *
 * This file is part of the cvc5 project.
 *
 * Copyright (c) 2009-2024 by the authors listed in the file AUTHORS
 * in the top-level source directory and their institutional affiliations.
 * All rights reserved.  See the file COPYING in the top-level source
 * directory for licensing information.
 * ****************************************************************************
 *
 * PB-blasting solver. Supports RoundingSAT and Exact back-ends.
 */

#include "cvc5_private.h"

#ifndef CVC5__THEORY__BV__BV_SOLVER_PB_H
#define CVC5__THEORY__BV__BV_SOLVER_PB_H

#include <vector>

#include "context/cdqueue.h"
#include "theory/bv/bv_solver.h"
#include "theory/bv/pb/pb_solver.h"
#include "theory/bv/pb/pb_node_blaster.h"
#include "theory/bv/pb/pb_blast_proof_generator.h"
#include "theory/bv/pb/pb_proof_manager.h"

namespace cvc5::internal {

namespace theory {
namespace bv {
namespace pb {

class NotifyResetAssertions;
class BBRegistrar;

/**
 * PB-blasting solver for the theory of bit-vectors
 */
class BVSolverPseudoBoolean : public BVSolver
{
 public:
  BVSolverPseudoBoolean(Env& env,
                        TheoryState* state,
                        TheoryInferenceManager& inferMgr);
  ~BVSolverPseudoBoolean() = default;

  /**
   * Whether the theory needs an equality engine. In our case, not.
   */
  bool needsEqualityEngine(EeSetupInfo&) override;

  /**
   * Not used by this decision procedure. Why is it an abstract function if
   * BVSolverBitblast does not use it as well?
   */
  void preRegisterTerm(CVC5_UNUSED TNode n) override {}  // same as BVSolverBitblast

  /** TODO(alanctprado): document */
  void postCheck(Theory::Effort level) override;

  /** TODO(alanctprado): document */
  bool preNotifyFact(TNode atom,
                     bool pol,
                     TNode fact,
                     bool isPrereg,
                     bool isInternal) override;

  /** TODO(alanctprado): document */
  TrustNode explain(TNode n) override;

  /** Solver identifier. For debugging purposes. */
  std::string identify() const override { return "BVSolverPseudoBoolean"; }

  /** TODO(alanctprado): document */
  void computeRelevantTerms(std::set<Node>& termSet) override;

  /** TODO(alanctprado): document */
  bool collectModelValues(TheoryModel* m,
                          const std::set<Node>& termSet) override;

  /** TODO(alanctprado): document */
  Node getValue(TNode node, bool initialize) override;

 private:
  /** Initialize pseudo-boolean solver. */
  void initPbSolver();

  /** PB solver back end (configured via options::bvSatSolver. */
  std::unique_ptr<PseudoBooleanSolver<Node>> d_pbSolver;
  /** Bit-blaster used to bit-blast atoms/terms. */
  std::unique_ptr<PseudoBooleanBlaster> d_pbBlaster;

  /**
   * PB-blast queue for facts sent to this solver.
   * Gets populated on preNotifyFact().
   */
  context::CDQueue<Node> d_facts;
  /**
   * PB-blast list for facts sent to this solver.
   * Used as input for the PB Solver.
   * Gets populated on postCheck().
   */
  context::CDList<Node> d_assumptions;

  /** Debugging */
  std::string getTermVariables(TNode term);
  void debugSatisfiedAtom(TNode atom);
  void debugSatisfiedTerm(TNode term);

  /** Proof logging */
  bool d_isProofProducing;
  // TODO(alanctprado): improve the names below
  PbBlastProofGenerator* d_pbbpg;
  PbProofManager* d_pbpm;
};

}  // namespace pb
}  // namespace bv
}  // namespace theory
}  // namespace cvc5::internal

#endif
