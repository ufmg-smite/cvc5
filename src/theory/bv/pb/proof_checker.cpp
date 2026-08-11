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
 * Implementation of the pseudo-Boolean / cutting-planes proof checker.
 */

#include "theory/bv/pb/proof_checker.h"

namespace cvc5::internal {
namespace theory {
namespace bv {
namespace pb {

PbProofRuleChecker::PbProofRuleChecker(NodeManager* nm) : ProofRuleChecker(nm)
{
}

void PbProofRuleChecker::registerTo(ProofChecker* pc)
{
  pc->registerChecker(ProofRule::CUTTING_PLANES_REFUTATION, this);
  pc->registerChecker(ProofRule::CUTTING_PLANES_AXIOM, this);
  pc->registerChecker(ProofRule::CUTTING_PLANES_ADDITION, this);
  pc->registerChecker(ProofRule::CUTTING_PLANES_MULTIPLICATION, this);
  pc->registerChecker(ProofRule::CUTTING_PLANES_DIVISION, this);
  pc->registerChecker(ProofRule::CUTTING_PLANES_SATURATION, this);
  pc->registerChecker(ProofRule::MACRO_CUTTING_PLANES_RESOLUTION, this);
  pc->registerChecker(ProofRule::MACRO_PB_BLAST_STEP, this);
}

Node PbProofRuleChecker::checkInternal(ProofRule id,
                                       const std::vector<Node>& children,
                                       const std::vector<Node>& args)
{
  // First-cut implementation: every rule is registered, but we trust the
  // stated conclusion (args[0]) rather than recomputing it from the children
  // and remaining arithmetic args. Full semantic checking is a follow-up.
  switch (id)
  {
    case ProofRule::CUTTING_PLANES_REFUTATION:
      // Two shapes:
      //   coarse: children: (), args: parsed VeriPB step Nodes
      //   fine:   children: (C), args: (C) where C is the unsat PB constraint
      // Both conclude `false`.
      return nodeManager()->mkConst(false);
    case ProofRule::CUTTING_PLANES_AXIOM:
      // children: (), args: (conclusion, literal)
      Assert(children.empty());
      Assert(args.size() == 2);
      return args[0];
    case ProofRule::CUTTING_PLANES_ADDITION:
      // children: (C1, C2), args: (conclusion)
      Assert(children.size() == 2);
      Assert(args.size() == 1);
      return args[0];
    case ProofRule::CUTTING_PLANES_MULTIPLICATION:
      // children: (C), args: (conclusion, k)
      Assert(children.size() == 1);
      Assert(args.size() == 2);
      return args[0];
    case ProofRule::CUTTING_PLANES_DIVISION:
      // children: (C), args: (conclusion, d)
      Assert(children.size() == 1);
      Assert(args.size() == 2);
      return args[0];
    case ProofRule::CUTTING_PLANES_SATURATION:
      // children: (C), args: (conclusion)
      Assert(children.size() == 1);
      Assert(args.size() == 1);
      return args[0];
    case ProofRule::MACRO_CUTTING_PLANES_RESOLUTION:
      // children: (C1, ..., Cn), args: (conclusion, polarities-SEXPR,
      // pivots-SEXPR) -- shape mirrors CHAIN_M_RESOLUTION.
      Assert(!children.empty());
      Assert(args.size() == 3);
      return args[0];
    case ProofRule::MACRO_PB_BLAST_STEP:
      // children: (F), args: (C). Trusted derivation of PB constraint C from
      // BV fact F via the PB blaster.
      Assert(children.size() == 1);
      Assert(args.size() == 1);
      return args[0];
    default: return Node::null();
  }
}

}  // namespace pb
}  // namespace bv
}  // namespace theory
}  // namespace cvc5::internal
