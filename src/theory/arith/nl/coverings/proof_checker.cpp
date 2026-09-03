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
#include "theory/arith/arith_utilities.h"
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
  pc->registerChecker(ProofRule::VALIDATE_INTERVALS, this);
  pc->registerChecker(ProofRule::RAN_EVAL, this);
}

Node CoveringsProofRuleChecker::checkValidateIntervals(const std::vector<Node>& args)
{
  NodeManager* nm = nodeManager();
  if (args.size() != 3 || args[0].getKind() != Kind::SEXPR
      || args[1].getKind() != Kind::SEXPR || args[2].getKind() != Kind::SEXPR)
  {
    return Node::null();
  }
  // TODO: check the side condition (root map complete, each (l, r) a gap
  // between consecutive roots of p or an infinite side)
  std::vector<Node> conj;
  for (const Node& e : args[0])
  {
    if (e.getKind() != Kind::SEXPR || e.getNumChildren() != 3)
    {
      return Node::null();
    }
    conj.push_back(mkNoRoots(nm, e[0], e[1], e[2]));
  }
  return nm->mkAnd(conj);
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
    if (iv.getKind() != Kind::SEXPR || iv.getNumChildren() != 2)
    {
      return Node::null();
    }

    if (iv[0] == iv[1])
    {
      disjs.push_back(nm->mkNode(Kind::EQUAL, var, iv[0]));
    }
    else
    {
      std::vector<Node> conjs;
      if (iv[0].getKind() != Kind::MINUS_INFINITY)
      {
        Node conj = nm->mkNode(Kind::GT, var, iv[0]);
        conjs.push_back(conj);
      }
      if (iv[1].getKind() != Kind::PLUS_INFINITY)
      {
        Node conj = nm->mkNode(Kind::LT, var, iv[1]);
        conjs.push_back(conj);
      }
      if (conjs.empty())
      {
        return nm->mkConst(true);
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
    return nm->mkConst(false);
  }
  if (id == ProofRule::COVER)
  {
    return checkCover(args);
  }
  if (id == ProofRule::VALIDATE_INTERVALS)
  {
    return checkValidateIntervals(args);
  }
  // SGN_INV_INTRO, SGN_INV_ELIM and RAN_EVAL do not
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
