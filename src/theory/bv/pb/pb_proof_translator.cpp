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

#include <algorithm>
#include <unordered_map>
#include <utility>
#include <vector>

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

/** Accumulate a GEQ/EQUAL constraint into coefficient-by-variable plus rhs. */
void parseLinear(Node constraint,
                 std::unordered_map<Node, Integer>& terms,
                 Integer& rhs)
{
  Assert(constraint.getKind() == Kind::GEQ
         || constraint.getKind() == Kind::EQUAL)
      << "not a PB constraint: " << constraint;
  for (const Node& term : linearTerms(constraint[0]))
  {
    Assert(term.getKind() == Kind::MULT && term[0].isConst()
           && term.getNumChildren() == 2)
        << "malformed PB summand: " << term;
    terms[term[1]] =
        terms[term[1]] + term[0].getConst<Rational>().getNumerator();
  }
  rhs = constraint[1].getConst<Rational>().getNumerator();
}

/** Degree of the literal-normalized form: rhs minus the negative coeffs. */
Integer literalDegree(const std::unordered_map<Node, Integer>& terms,
                      const Integer& rhs)
{
  Integer degree = rhs;
  for (const auto& [variable, coefficient] : terms)
  {
    if (coefficient.sgn() < 0) degree = degree - coefficient;
  }
  return degree;
}

/** rhs of the signed form with literal-normalized degree `degree`. */
Integer signedRhs(const std::unordered_map<Node, Integer>& terms,
                  const Integer& degree)
{
  Integer rhs = degree;
  for (const auto& [variable, coefficient] : terms)
  {
    if (coefficient.sgn() < 0) rhs = rhs + coefficient;
  }
  return rhs;
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

void PbProofTranslator::setVariableIndices(
    std::unordered_map<Node, uint64_t> indices)
{
  d_varIndex = std::move(indices);
}

uint64_t PbProofTranslator::variableIndex(const Node& variable) const
{
  auto it = d_varIndex.find(variable);
  if (it == d_varIndex.end())
  {
    Unreachable() << "PbProofTranslator: variable " << variable
                  << " has no backend index";
  }
  return it->second;
}

Node PbProofTranslator::mkPbConstraint(
    const std::unordered_map<Node, Integer>& terms, const Integer& rhs)
{
  std::vector<std::pair<uint64_t, Node>> order;
  for (const auto& [variable, coefficient] : terms)
  {
    if (coefficient.sgn() == 0) continue;
    order.emplace_back(variableIndex(variable), variable);
  }
  std::sort(order.begin(), order.end());
  NodeManager* nm = nodeManager();
  std::vector<Node> summands;
  for (const auto& [index, variable] : order)
  {
    summands.push_back(nm->mkNode(
        Kind::MULT, nm->mkConstInt(Rational(terms.at(variable))), variable));
  }
  Node lhs = summands.empty()      ? nm->mkConstInt(Rational(0))
             : summands.size() == 1 ? summands[0]
                                    : nm->mkNode(Kind::ADD, summands);
  return nm->mkNode(Kind::GEQ, lhs, nm->mkConstInt(Rational(rhs)));
}

void PbProofTranslator::registerInputConstraint(size_t veriPbId,
                                                Node constraint,
                                                Node fact,
                                                bool negated)
{
  Assert(!negated || constraint.getKind() == Kind::EQUAL);
  std::unordered_map<Node, Integer> terms;
  Integer rhs;
  parseLinear(constraint, terms, rhs);
  if (negated)
  {
    for (auto& entry : terms) entry.second = -entry.second;
    rhs = -rhs;
  }
  // TODO: prove each new variable's domain bounds at the PB-blasting level.
  for (const auto& [variable, coefficient] : terms)
  {
    d_boundedVars.insert(variable);
  }
  Node canonical = mkPbConstraint(terms, rhs);
  d_cdp->addStep(
      canonical, ProofRule::MACRO_PB_BLAST_STEP, {fact}, {canonical});
  d_coreById[veriPbId] = canonical;
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
  NodeManager* nm = nodeManager();
  Node conclusion = rupNode[0];
  Node negated = conclusion.notNode();
  std::vector<Node> premises;
  std::unordered_map<Node, size_t> premiseIndex;
  std::vector<Node> hintArgs;
  for (const Node& hint : rupNode[1])
  {
    Assert(hint.getKind() == Kind::CONST_INTEGER);
    Node premise = lookup(hint);
    if (premise.isNull())
    {
      Trace("bv-pb-proof-translate")
          << "  hint " << hint << " of rup step " << veriPbId
          << " names no known constraint\n";
      Unreachable();
      continue;
    }
    auto [it, fresh] = premiseIndex.emplace(premise, premises.size());
    if (fresh) premises.push_back(premise);
    hintArgs.push_back(nm->mkConstInt(
        Rational(Integer(static_cast<unsigned long>(it->second)))));
  }
  CDProof localcdp(d_env);
  // Proofs link by ProofNode identity, so reuse d_cdp's premise nodes.
  for (const Node& premise : premises)
  {
    localcdp.addProof(d_cdp->getProofFor(premise));
  }
  premises.push_back(negated);

  Node falseNode = nm->mkConst(false);
  localcdp.addStep(falseNode,
                   ProofRule::CUTTING_PLANES_RUP,
                   premises,
                   {conclusion, nm->mkNode(Kind::SEXPR, hintArgs)});
  Node doubleNegated = negated.notNode();
  localcdp.addStep(doubleNegated, ProofRule::SCOPE, {falseNode}, {negated});
  localcdp.addStep(conclusion, ProofRule::NOT_NOT_ELIM, {doubleNegated}, {});
  localcdp.addProofTo(conclusion, d_cdp);
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

Node PbProofTranslator::weakenVariable(Node constraint, Node variable)
{
  std::unordered_map<Node, Integer> terms;
  Integer rhs;
  parseLinear(constraint, terms, rhs);
  auto it = terms.find(variable);
  if (it == terms.end() || it->second.sgn() == 0) return constraint;
  Integer coefficient = it->second;
  bool positive = coefficient.sgn() > 0;
  // Cancel the variable against its domain bound, scaled to the coefficient:
  // for a > 0 add a * (-v >= -1), for a < 0 add |a| * (v >= 0).
  Node addend = mkDomainBound(variable, positive);
  if (coefficient.abs() != Integer(1))
  {
    std::unordered_map<Node, Integer> scaled{{variable, -coefficient}};
    Node scaledBound =
        mkPbConstraint(scaled, positive ? -coefficient : Integer(0));
    d_cdp->addStep(
        scaledBound,
        ProofRule::CUTTING_PLANES_MULTIPLICATION,
        {addend},
        {scaledBound, nodeManager()->mkConstInt(Rational(coefficient.abs()))});
    addend = scaledBound;
  }
  terms[variable] = Integer(0);
  Node conclusion = mkPbConstraint(terms, positive ? rhs - coefficient : rhs);
  d_cdp->addStep(conclusion,
                 ProofRule::CUTTING_PLANES_ADDITION,
                 {constraint, addend},
                 {conclusion});
  return conclusion;
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
    if (looked.isNull())
    {
      Unreachable() << "PbProofTranslator::translatePolExpr: id " << expr
                    << " names no known constraint";
    }
    return looked;
  }
  // Leaf: the literal axiom of a `x`/`~x` token, i.e. a domain bound.
  if (k == Kind::GEQ)
  {
    Assert(expr[0].getKind() == Kind::MULT && expr[0][0].isConst());
    return mkDomainBound(expr[0][1],
                         expr[0][0].getConst<Rational>().sgn() < 0);
  }
  if (k == Kind::PB_PROOF_POL_ADD)
  {
    Node lhs = translatePolExpr(expr[0]);
    Node rhs = translatePolExpr(expr[1]);
    std::unordered_map<Node, Integer> terms;
    Integer lhsRhs, rhsRhs;
    parseLinear(lhs, terms, lhsRhs);
    parseLinear(rhs, terms, rhsRhs);
    Node conclusion = mkPbConstraint(terms, lhsRhs + rhsRhs);
    // Adding a trivial constraint changes nothing; a step from a constraint
    // to itself would be cyclic.
    if (conclusion == lhs || conclusion == rhs) return conclusion;
    d_cdp->addStep(conclusion,
                   ProofRule::CUTTING_PLANES_ADDITION,
                   {lhs, rhs},
                   {conclusion});
    return conclusion;
  }

  if (k == Kind::PB_PROOF_POL_MUL)
  {
    Node operand = translatePolExpr(expr[0]);
    Integer factor = expr[1].getConst<Rational>().getNumerator();
    std::unordered_map<Node, Integer> terms;
    Integer rhs;
    parseLinear(operand, terms, rhs);
    for (auto& entry : terms) entry.second = entry.second * factor;
    Node conclusion = mkPbConstraint(terms, rhs * factor);
    if (conclusion == operand) return operand;
    d_cdp->addStep(conclusion,
                   ProofRule::CUTTING_PLANES_MULTIPLICATION,
                   {operand},
                   {conclusion, expr[1]});
    return conclusion;
  }

  if (k == Kind::PB_PROOF_POL_DIV)
  {
    Node operand = translatePolExpr(expr[0]);
    Integer divisor = expr[1].getConst<Rational>().getNumerator();
    std::unordered_map<Node, Integer> terms;
    Integer rhs;
    parseLinear(operand, terms, rhs);
    // Ceiling division acts on the literal-normalized form: all coefficients
    // positive over possibly-negated literals, degree = rhs shifted by the
    // negative coefficients.
    Integer degree = literalDegree(terms, rhs).ceilingDivideQuotient(divisor);
    for (auto& entry : terms)
    {
      Integer& c = entry.second;
      if (c.sgn() > 0)
      {
        c = c.ceilingDivideQuotient(divisor);
      }
      else if (c.sgn() < 0)
      {
        c = -((-c).ceilingDivideQuotient(divisor));
      }
    }
    Node conclusion = mkPbConstraint(terms, signedRhs(terms, degree));
    if (conclusion == operand) return operand;
    d_cdp->addStep(conclusion,
                   ProofRule::CUTTING_PLANES_DIVISION,
                   {operand},
                   {conclusion, expr[1]});
    return conclusion;
  }

  if (k == Kind::PB_PROOF_POL_SAT)
  {
    Node operand = translatePolExpr(expr[0]);
    std::unordered_map<Node, Integer> terms;
    Integer rhs;
    parseLinear(operand, terms, rhs);
    // Saturation caps each literal-form coefficient at the degree.
    Integer degree = literalDegree(terms, rhs);
    if (degree.sgn() < 0) degree = Integer(0);
    for (auto& entry : terms)
    {
      Integer& c = entry.second;
      if (c > degree)
      {
        c = degree;
      }
      else if (c < -degree)
      {
        c = -degree;
      }
    }
    Node conclusion = mkPbConstraint(terms, signedRhs(terms, degree));
    if (conclusion == operand) return operand;
    d_cdp->addStep(conclusion,
                   ProofRule::CUTTING_PLANES_SATURATION,
                   {operand},
                   {conclusion});
    return conclusion;
  }

  if (k == Kind::PB_PROOF_POL_WEAKEN)
  {
    Node operand = translatePolExpr(expr[0]);
    return weakenVariable(operand, expr[1]);
  }

  Unreachable()
      << "PbProofTranslator::translatePolExpr: unhandled expression kind "
      << k;
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
