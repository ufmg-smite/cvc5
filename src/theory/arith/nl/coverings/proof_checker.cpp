/******************************************************************************
 * Top contributors (to current version):
 *   Gereon Kremer, Andrew Reynolds, Mathias Preiner
 *
 * This file is part of the cvc5 project.
 *
 * Copyright (c) 2009-2025 by the authors listed in the file AUTHORS
 * in the top-level source directory and their institutional affiliations.
 * All rights reserved.  See the file COPYING in the top-level source
 * directory for licensing information.
 * ****************************************************************************
 *
 * Implementation of CAD proof checker.
 */

#include "theory/arith/nl/coverings/proof_checker.h"

#include "expr/sequence.h"
#include "theory/rewriter.h"

using namespace cvc5::internal::kind;

namespace cvc5::internal {
namespace theory {
namespace arith {
namespace nl {
namespace coverings {

CoveringsProofRuleChecker::CoveringsProofRuleChecker(NodeManager* nm)
    : ProofRuleChecker(nm)
{
}

void CoveringsProofRuleChecker::registerTo(ProofChecker* pc)
{
  pc->registerChecker(ProofRule::ARITH_COVERINGS_UNIV, this);
  pc->registerChecker(ProofRule::DECOMP, this);
  pc->registerChecker(ProofRule::SGN_INV_INTRO, this);
  pc->registerChecker(ProofRule::SGN_INV_ELIM, this);
  pc->registerChecker(ProofRule::VALIDATE_POLY_MAP, this);
  pc->registerChecker(ProofRule::RAN_EVAL, this);
}

Node CoveringsProofRuleChecker::checkInternal(ProofRule id,
                                              const std::vector<Node>& children,
                                              const std::vector<Node>& args)
{
  NodeManager* nm = nodeManager();
  // TODO: Actually check the proof.
  if (id == ProofRule::ARITH_COVERINGS_UNIV)
  {
    return nm->mkConst<bool>(false);
  }
  // DECOMP, SGN_INV_INTRO, SGN_INV_ELIM, RAN_EVAL and VALIDATE_POLY_MAP do
  // not conclude
  // false, so returning it here would be rejected as a conclusion mismatch
  // as soon as such steps are emitted. TODO: compute their conclusions from
  // the premises and arguments.
  return Node::null();
}

}  // namespace coverings
}  // namespace nl
}  // namespace arith
}  // namespace theory
}  // namespace cvc5::internal
