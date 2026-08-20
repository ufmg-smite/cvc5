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
 * Pseudo-Boolean / cutting-planes proof checker.
 */

#include "cvc5_private.h"

#ifndef CVC5__THEORY__BV__PB__PROOF_CHECKER_H
#define CVC5__THEORY__BV__PB__PROOF_CHECKER_H

#include "expr/node.h"
#include "proof/proof_checker.h"
#include "proof/proof_node.h"

namespace cvc5::internal {
namespace theory {
namespace bv {
namespace pb {

/**
 * Proof rule checker for the cutting-planes calculus over pseudo-Boolean
 * constraints. Registers and checks the CUTTING_PLANES_* primitive rules and
 * the MACRO_CUTTING_PLANES_RESOLUTION macro used when translating VeriPB
 * proofs from the PB-blasting backend, plus the coarse-grained VERIPB_PROOF
 * rule that carries an untranslated VeriPB proof.
 *
 * The current implementation registers the rules but trusts their stated
 * conclusion (passed as args[0]). Full semantic checking (recomputing the
 * conclusion from the children + arithmetic args) is a follow-up.
 */
class PbProofRuleChecker : public ProofRuleChecker
{
 public:
  PbProofRuleChecker(NodeManager* nm);

  /** Register all rules owned by this rule checker into pc. */
  void registerTo(ProofChecker* pc) override;

 protected:
  /** Return the conclusion of the given proof step, or null if invalid. */
  Node checkInternal(ProofRule id,
                     const std::vector<Node>& children,
                     const std::vector<Node>& args) override;
};

}  // namespace pb
}  // namespace bv
}  // namespace theory
}  // namespace cvc5::internal

#endif /* CVC5__THEORY__BV__PB__PROOF_CHECKER_H */
