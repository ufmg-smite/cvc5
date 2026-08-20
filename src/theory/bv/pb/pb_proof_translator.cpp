/******************************************************************************
 * This file is part of the cvc5 project.
 *
 * Copyright (c) 2009-2026 by the authors listed in the file AUTHORS
 * in the top-level source directory and their institutional affiliations.
 * All rights reserved.  See the file COPYING in the top-level source
 * directory for licensing information.
 * ****************************************************************************
 *
 * Implementation of the VeriPB-to-cutting-planes proof translator.
 */

#include "theory/bv/pb/pb_proof_translator.h"

#include "expr/node_manager.h"
#include "proof/proof_checker.h"
#include "proof/proof_node_manager.h"
#include "proof/proof_step_buffer.h"
#include "theory/arith/arith_proof_utilities.h"
#include "util/rational.h"

namespace cvc5::internal {
namespace theory {
namespace bv {
namespace pb {

namespace {
/** The (MULT c v) summands of a PB constraint's left-hand side. */
std::vector<Node> linearTerms(Node lhs)
{
  if (lhs.getKind() == Kind::MULT) return {lhs};
  if (lhs.getKind() == Kind::ADD) return {lhs.begin(), lhs.end()};
  return {};
}
}  // namespace

PbProofTranslator::PbProofTranslator(Env& env, CDProof* cdp)
    : EnvObj(env), d_cdp(cdp)
{
  initializeTranslators();
}

void PbProofTranslator::initializeTranslators()
{
  d_translators = {
      {Kind::PB_PROOF_REVERSE_UNIT_PROPAGATION,
       [this](Node step, size_t id) { return translateRup(step, id); }},
      {Kind::PB_PROOF_REVERSE_POLISH_NOTATION,
       [this](Node step, size_t id) { return translatePol(step, id); }},
      {Kind::PB_PROOF_CONCLUSION,
       [this](Node step, size_t id) { return translateConclusion(step, id); }},
      {Kind::PB_PROOF_DELETE_BY_ID,
       [this](Node step, size_t id) { return translateDeleteById(step, id); }},
  };
}

void PbProofTranslator::registerInputConstraint(size_t veriPbId,
                                                Node constraint)
{
  d_coreById[veriPbId] = constraint;
}

void PbProofTranslator::translateStep(Node veriPbStep, size_t veriPbId)
{
  auto it = d_translators.find(veriPbStep.getKind());
  if (it == d_translators.end())
  {
    Trace("bv-pb-proof-translate-unsupported")
        << "PbProofTranslator::translateStep: skipping unsupported step kind "
        << veriPbStep.getKind() << " with conclusion " << veriPbStep[0] << std::endl;
    return;
  }
  Node conclusion = it->second(veriPbStep, veriPbId);
  if (!conclusion.isNull())
  {
    d_derivedById[veriPbId] = conclusion;
  }
}

void PbProofTranslator::eraseDerived(size_t veriPbId)
{
  Assert(d_derivedById.find(veriPbId) != d_derivedById.end())
      << "deld: id " << veriPbId << " not in derived set";
  d_derivedById.erase(veriPbId);
}

void PbProofTranslator::eraseCore(size_t veriPbId)
{
  Assert(d_coreById.find(veriPbId) != d_coreById.end())
      << "delc: id " << veriPbId << " not in core set";
  d_coreById.erase(veriPbId);
}

void PbProofTranslator::eraseAny(size_t veriPbId)
{
  if (d_derivedById.find(veriPbId) != d_derivedById.end())
  {
    eraseDerived(veriPbId);
    return;
  }
  if (d_coreById.find(veriPbId) != d_coreById.end())
  {
    eraseCore(veriPbId);
    return;
  }
  Trace("bv-pb-proof-translate")
      << "PbProofTranslator::eraseAny: id " << veriPbId
      << " not present in either set; ignoring\n";
}

Node PbProofTranslator::translateRup(Node rupNode, size_t veriPbId)
{
  Trace("bv-pb-proof-translate")
      << "PbProofTranslator::translateRup id=" << veriPbId << "\n";
  Assert(rupNode.getNumChildren() == 2);
  Node conclusion = rupNode[0];
  Node hints = rupNode[1];

  // Resolve each hint id to the previously-recorded conclusion. Hints whose
  // ids are unknown are skipped silently; they may reference constraints not
  // yet translated, in which case the resulting proof step will be sound but
  // unfaithful to the hint chain. Real pivot computation will need every
  // hint resolved.
  std::vector<Node> children;
  for (const Node& hintBoundVar : hints)
  {
    Node child = lookup(hintBoundVar);
    if (!child.isNull()) children.push_back(child);
  }

  // Args: (conclusion, polarities-SEXPR, pivots-SEXPR). The polarity and
  // pivot lists are emitted empty for now.
  //
  // TODO: compute pivots by walking the hint chain with a
  // mark_var-style scan adapted to PB literals (cf. proof_tracer.cpp:225):
  // for each consecutive (running, next-antecedent) pair, the pivot is the
  // variable whose coefficient has opposite sign in the two constraints.
  NodeManager* nm = nodeManager();
  std::vector<Node> empty;
  Node polarities = nm->mkNode(Kind::SEXPR, empty);
  Node pivots = nm->mkNode(Kind::SEXPR, empty);
  std::vector<Node> args{conclusion, polarities, pivots};

  d_cdp->addStep(conclusion,
                 ProofRule::MACRO_CUTTING_PLANES_RESOLUTION,
                 children,
                 args);
  return conclusion;
}

  
Node PbProofTranslator::translatePol(Node polNode, size_t veriPbId)
{
  Trace("bv-pb-proof-translate")
      << "PbProofTranslator::translatePol id=" << veriPbId << "\n";
  Assert(polNode.getNumChildren() == 1);
  return translatePolExpr(polNode[0]);
}

Node PbProofTranslator::translateConclusion(Node conclNode,
                                            CVC5_UNUSED size_t veriPbId)
{
  Trace("bv-pb-proof-translate")
      << "PbProofTranslator::translateConclusion " << conclNode << "\n";
  Assert(conclNode.getNumChildren() == 1
         && conclNode[0].getKind() == Kind::CONST_INTEGER);
  Node unsatConstraint = lookup(conclNode[0]);
  if (unsatConstraint.isNull())
  {
    Trace("bv-pb-proof-translate")
        << "  unsat constraint id " << conclNode[0] << " unknown; skipping\n";
    return Node::null();
  }
  refuteContradicting(unsatConstraint);
  return Node::null();
}

Node PbProofTranslator::mkDomainBound(Node variable, bool upper)
{
  // Shaped like the literal axioms PbProofRules emits for the tokens `~x` and
  // `x`: (>= (* -1 x) -1) and (>= (* 1 x) 0).
  NodeManager* nm = nodeManager();
  Node coefficient = nm->mkConstInt(Rational(upper ? -1 : 1));
  Node lhs = nm->mkNode(Kind::MULT, coefficient, variable);
  Node rhs = nm->mkConstInt(Rational(upper ? -1 : 0));
  return nm->mkNode(Kind::GEQ, lhs, rhs);
}

void PbProofTranslator::refuteContradicting(Node constraint)
{
  NodeManager* nm = nodeManager();
  if (constraint.getKind() != Kind::GEQ)
  {
    Trace("bv-pb-proof-translate")
        << "  unsat constraint " << constraint << " is not a GEQ; skipping\n";
    return;
  }
  std::vector<Node> children{constraint};
  std::vector<Node> coefficients{nm->mkConstInt(Rational(-1))};
  for (const Node& term : linearTerms(constraint[0]))
  {
    if (term.getKind() != Kind::MULT || !term[0].isConst())
    {
      Trace("bv-pb-proof-translate")
          << "  unexpected summand " << term << " in " << constraint
          << "; skipping\n";
      return;
    }
    Rational coefficient = term[0].getConst<Rational>();
    if (coefficient.sgn() == 0) continue;
    bool positive = coefficient.sgn() > 0;
    Node bound = mkDomainBound(term[1], positive);
    children.push_back(bound);
    coefficients.push_back(
        nm->mkConstInt(positive ? -coefficient : coefficient));
  }

  ProofStepBuffer steps(d_cdp->getManager()->getChecker());
  Node ground = constraint;
  if (children.size() > 1)
  {
    std::vector<Node> coefficientsUse =
        arith::getMacroSumUbCoeff(nm, children, coefficients);
    ground = steps.tryStep(
        ProofRule::MACRO_ARITH_SCALE_SUM_UB, children, coefficientsUse);
    if (ground.isNull())
    {
      Trace("bv-pb-proof-translate")
          << "  could not cancel the variables of " << constraint
          << " against their domain bounds; skipping\n";
      return;
    }
  }
  Node falseNode = nm->mkConst(false);
  if (steps
          .tryStep(ProofRule::MACRO_SR_PRED_TRANSFORM, {ground}, {falseNode})
          .isNull())
  {
    Trace("bv-pb-proof-translate")
        << "  " << ground << " does not rewrite to false; skipping\n";
    return;
  }
  d_cdp->addSteps(steps);
}

Node PbProofTranslator::translatePolExpr(Node expr)
{
  Kind k = expr.getKind();
  // Leaf: a constraint id
  if (k == Kind::CONST_INTEGER)
  {
    Node looked = lookup(expr);
    return looked.isNull() ? expr : looked;
  }
  // Leaf: a literal axiom
  if (k == Kind::GEQ)
  {
    // The literal arg is the variable token inside the LHS MULT. Best-effort
    // for now: pass the whole conclusion as both args[0] (conclusion) and
    // args[1] (literal). The trusted-stub checker treats args[0] as the
    // conclusion; full conclusion-vs-literal split is a follow-up.
    std::vector<Node> args{expr, expr};
    d_cdp->addStep(expr, ProofRule::CUTTING_PLANES_AXIOM, {}, args);
    return expr;
  }
  if (k == Kind::PB_PROOF_POL_ADD)
  {
    Node lhs = translatePolExpr(expr[0]);
    Node rhs = translatePolExpr(expr[1]);
    // TODO: compute the true sum-of-constraints conclusion.
    // Placeholder: pass the structural ADD node so the trusted-stub checker
    // returns it from args[0].
    Node conclusion = expr;
    std::vector<Node> children{lhs, rhs};
    std::vector<Node> args{conclusion};
    d_cdp->addStep(conclusion,
                   ProofRule::CUTTING_PLANES_ADDITION,
                   children,
                   args);
    return conclusion;
  }

  if (k == Kind::PB_PROOF_POL_MUL)
  {
    Node operand = translatePolExpr(expr[0]);
    Node factor = expr[1];
    // TODO: compute the scaled conclusion.
    Node conclusion = expr;
    std::vector<Node> children{operand};
    std::vector<Node> args{conclusion, factor};
    d_cdp->addStep(conclusion,
                   ProofRule::CUTTING_PLANES_MULTIPLICATION,
                   children,
                   args);
    return conclusion;
  }

  if (k == Kind::PB_PROOF_POL_DIV)
  {
    Node operand = translatePolExpr(expr[0]);
    Node divisor = expr[1];
    // TODO: compute the ceiling-divided conclusion.
    Node conclusion = expr;
    std::vector<Node> children{operand};
    std::vector<Node> args{conclusion, divisor};
    d_cdp->addStep(conclusion,
                   ProofRule::CUTTING_PLANES_DIVISION,
                   children,
                   args);
    return conclusion;
  }

  if (k == Kind::PB_PROOF_POL_SAT)
  {
    Node operand = translatePolExpr(expr[0]);
    // TODO: compute the saturated conclusion.
    Node conclusion = expr;
    std::vector<Node> children{operand};
    std::vector<Node> args{conclusion};
    d_cdp->addStep(conclusion,
                   ProofRule::CUTTING_PLANES_SATURATION,
                   children,
                   args);
    return conclusion;
  }

  // TODO: expand into AXIOM/MULTIPLICATION/ADDITION once the operand's
  // constraint is available: weakening away v adds the axiom on ~v, scaled by
  // v's coefficient (on v instead, if that coefficient is negative).
  if (k == Kind::PB_PROOF_POL_WEAKEN)
  {
    translatePolExpr(expr[0]);
    return expr;
  }

  Trace("bv-pb-proof-translate")
      << "PbProofTranslator::translatePolExpr: unhandled expression kind "
      << k << "\n";
  return expr;
}

Node PbProofTranslator::lookup(Node idNode) const
{
  if (idNode.getKind() != Kind::CONST_INTEGER) return Node::null();
  size_t veriPbId =
      idNode.getConst<Rational>().getNumerator().getUnsignedLong();
  auto itCore = d_coreById.find(veriPbId);
  if (itCore != d_coreById.end()) return itCore->second;
  auto itDer = d_derivedById.find(veriPbId);
  if (itDer != d_derivedById.end()) return itDer->second;
  return Node::null();
}

Node PbProofTranslator::translateDeleteById(Node delNode,
                                            CVC5_UNUSED size_t veriPbId)
{
  Trace("bv-pb-proof-translate")
      << "PbProofTranslator::translateDeleteById " << delNode << "\n";
  Assert(delNode.getNumChildren() == 1
         && delNode[0].getKind() == Kind::SEXPR);
  Node ids = delNode[0];
  for (const Node& idTok : ids)
  {
    Assert(idTok.getKind() == Kind::CONST_INTEGER)
        << "PbProofTranslator::translateDeleteById: non-integer id token "
        << idTok;
    size_t id = idTok.getConst<Rational>().getNumerator().getUnsignedLong();
    eraseAny(id);
  }
  return Node::null();
}

}  // namespace pb
}  // namespace bv
}  // namespace theory
}  // namespace cvc5::internal
