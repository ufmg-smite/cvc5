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
 * PB-blasting solver. Supports RoundingSAT and Exact back-ends.
 */

#include "cvc5_private.h"

#ifndef CVC5__THEORY__BV__BV_SOLVER_PB_H
#define CVC5__THEORY__BV__BV_SOLVER_PB_H

#include <unordered_map>
#include <vector>

#include "context/cdqueue.h"
#include "theory/bv/bitblast/node_bitblaster.h"
#include "theory/bv/bv_solver.h"
#include "theory/bv/pb/clause_cnf_stream.h"
#include "theory/bv/pb/pb_blast_proof_generator.h"
#include "theory/bv/pb/pb_node_blaster.h"
#include "theory/bv/pb/pb_proof_manager.h"
#include "theory/bv/pb/pb_solver.h"

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
  void preRegisterTerm(CVC5_UNUSED TNode n) override {
  }  // same as BVSolverBitblast

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

  /**
   * Alternative post-check used when options::bvPbEagerCnf is set. Instead of
   * PB-blasting each atom, it clausifies the asserted facts with a
   * ClauseCnfStream and translates every resulting clause into a pseudo-Boolean
   * constraint (sum of literals >= 1) before solving.
   */
  void postCheckEager();
  /**
   * Translates a CNF clause into the PB constraint that requires the sum of its
   * literals to be at least one. A negated literal ~x is encoded as (1 - x),
   * i.e. it appears with coefficient -1 and lowers the right-hand side by one.
   */
  Node clauseToConstraint(const prop::SatClause& clause);
  /**
   * Returns the PB variable associated with the given SAT variable, allocating
   * a fresh one (via the blaster) the first time it is seen.
   */
  Node getPbVarForSatVar(prop::SatVariable var);

  /** PB solver back end (configured via options::bvSatSolver. */
  std::unique_ptr<PseudoBooleanSolver<Node>> d_pbSolver;
  /** Bit-blaster used to bit-blast atoms/terms. */
  std::unique_ptr<PseudoBooleanBlaster> d_pbBlaster;
  /**
   * Bit-blaster producing a Boolean encoding of atoms. Used by the eager-CNF
   * path (options::bvPbEagerCnf) to obtain a Boolean formula that is then
   * clausified and translated into PB constraints.
   */
  std::unique_ptr<NodeBitblaster> d_bitblaster;
  /**
   * CNF converter used by the eager-CNF path (options::bvPbEagerCnf). Stores the
   * clauses generated from the bit-blasted facts.
   */
  std::unique_ptr<ClauseCnfStream> d_cnfStream;
  /** Map from the CNF stream's SAT variables to their PB variables. */
  std::unordered_map<prop::SatVariable, Node> d_satVarToPbVar;

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
