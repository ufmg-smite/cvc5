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
#include "util/rational.h"
#include "util/real_algebraic_number.h"

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
  pc->registerChecker(ProofRule::COVER, this);
  pc->registerChecker(ProofRule::SGN_INV_INTRO, this);
  pc->registerChecker(ProofRule::SGN_INV_ELIM, this);
  pc->registerChecker(ProofRule::VALIDATE_POLY_MAP, this);
  pc->registerChecker(ProofRule::RAN_EVAL, this);
}

// TODO: Check side condition
Node CoveringsProofRuleChecker::checkCover(const std::vector<Node>& args)
{
  NodeManager* nm = nodeManager();
  if (args.size() != 2 || args[1].getKind() != Kind::SEXPR)
  {
    return Node::null();
  }
  const Node& var = args[0];
  if (!var.getType().isRealOrInt())
  {
    return Node::null();
  }
  std::vector<Node> disjs;
  for (const Node& iv : args[1])
  {
    if (iv.getKind() != Kind::SEXPR || iv.getNumChildren() != 4)
    {
      return Node::null();
    }

    if (iv[0] == iv[2])
    {
      disjs.push_back(nm->mkNode(Kind::EQUAL, var, iv[0]));
    }
    else
    {
      std::vector<Node> conjs;
      if (iv[0].getKind() != Kind::MINUS_INFINITY)
      {
        bool lowerOpen = iv[1] == nm->mkConst<bool>(true);
        Node conj =
          lowerOpen ?
            nm->mkNode(Kind::GT, var, iv[0]) :
            nm->mkNode(Kind::GEQ, var, iv[0]);
        conjs.push_back(conj);
      }
      if (iv[2].getKind() != Kind::PLUS_INFINITY)
      {
        bool upperOpen = iv[3] == nm->mkConst<bool>(true);
        Node conj =
          upperOpen ?
            nm->mkNode(Kind::LT, var, iv[2]) :
            nm->mkNode(Kind::LEQ, var, iv[2]);
        conjs.push_back(conj);
      }
      if (conjs.empty())
      {
        return nm->mkConst<bool>(true);
      }
      disjs.push_back(nm->mkAnd(conjs));
    }
  }

  return nm->mkOr(disjs);
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
  if (id == ProofRule::COVER)
  {
    // return nm->mkConst<bool>(false);
    return checkCover(args);
  }
  // SGN_INV_INTRO, SGN_INV_ELIM, RAN_EVAL and VALIDATE_POLY_MAP do not
  // conclude false, so returning it here would be rejected as a conclusion
  // mismatch as soon as such steps are emitted. TODO: compute their
  // conclusions from the premises and arguments.
  return Node::null();
}

}  // namespace coverings
}  // namespace nl
}  // namespace arith
}  // namespace theory
}  // namespace cvc5::internal
