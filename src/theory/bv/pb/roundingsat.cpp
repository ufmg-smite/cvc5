/******************************************************************************
 * Top contributors (to current version):
 *   Alan Prado, Pedro Saccomani
 *
 * This file is part of the cvc5 project.
 *
 * Copyright (c) 2009-2026 by the authors listed in the file AUTHORS
 * in the top-level source directory and their institutional affiliations.
 * All rights reserved.  See the file COPYING in the top-level source
 * directory for licensing information.
 * ****************************************************************************
 *
 * Wrapper for the RoundingSat PB Solver.
 *
 * Implementation of the RoundingSat PB solver for cvc5 (bit-vectors), built on
 * the in-process `rs::api::PbSolver` library.
 */

#ifdef CVC5_USE_ROUNDINGSAT

#include "theory/bv/pb/roundingsat.h"

#include <utility>

#include "api/PbSolver.hpp"
#include "base/check.h"
#include "util/rational.h"

namespace cvc5::internal {
namespace theory {
namespace bv {
namespace pb {

RoundingSatSolver::RoundingSatSolver(Env& env,
                                     StatisticsRegistry& registry,
                                     const std::string& name,
                                     bool logProofs)
    : EnvObj(env),
      d_solver(std::make_unique<rs::api::PbSolver>(logProofs)),
      d_logProofs(logProofs),
      d_statistics(registry, name)
{
}

RoundingSatSolver::~RoundingSatSolver() = default;

void RoundingSatSolver::addVariable(const Node variable)
{
  Trace("bv-pb-roundingsat")
      << "RoundingSatSolver::addVariable " << variable << "\n";
  Assert(variable.isVar());
  if (d_nodeToVar.count(variable)) return;
  RsVar v = d_solver->addVariable();
  d_nodeToVar.emplace(variable, v);
  d_varToNode.emplace(v, variable);
  d_nameToVar.emplace(variable.toString(), v);
  // TODO: ++d_statistics.d_numVariables;
}

RoundingSatSolver::RsVar RoundingSatSolver::toRsVar(const Node& node)
{
  auto it = d_nodeToVar.find(node);
  if (it != d_nodeToVar.end()) return it->second;
  addVariable(node);
  return d_nodeToVar.at(node);
}

Kind RoundingSatSolver::parseConstraint(
    const Node& constraint,
    std::vector<std::pair<Integer, RsVar>>& terms,
    Integer& rhs)
{
  const Node& linear_form = constraint[0];
  auto pushTerm = [&](const Node& term) {
    Assert(term.getNumChildren() == 2);
    Assert(term[0].isConst());
    Assert(term[1].isVar());
    const Rational& coeff = term[0].getConst<Rational>();
    Assert(coeff.isIntegral());
    terms.emplace_back(coeff.getNumerator(), toRsVar(term[1]));
  };

  if (linear_form.getKind() == Kind::PB_TERM)
  {
    pushTerm(linear_form);
  }
  else if (linear_form.getKind() == Kind::PB_SUM)
  {
    for (const Node& term : linear_form)
    {
      pushTerm(term);
    }
  }
  else
  {
    Unreachable();
  }

  const Rational& rhsRat = constraint[1].getConst<Rational>();
  Assert(rhsRat.isIntegral());
  rhs = rhsRat.getNumerator();

  return constraint.getKind();
}

void RoundingSatSolver::addConstraint(const Node constraint)
{
  Trace("bv-pb-roundingsat")
      << "RoundingSatSolver::addConstraint " << constraint << "\n";
  if (!d_constraintSet.emplace(constraint).second) return;

  std::vector<std::pair<Integer, RsVar>> terms;
  Integer rhs;
  Kind k = parseConstraint(constraint, terms, rhs);

  // Coefficients are passed as decimal strings to preserve arbitrary precision
  // (bit-vector place values exceed int64_t for wide bit-vectors). All PB
  // variables appear as positive literals.
  std::vector<std::pair<std::string, rs::api::Lit>> lhs;
  lhs.reserve(terms.size());
  for (const auto& [coeff, var] : terms)
  {
    lhs.emplace_back(coeff.toString(), var);
  }

  rs::api::PbRelOp op = (k == Kind::EQUAL) ? rs::api::PbRelOp::EQUAL
                                           : rs::api::PbRelOp::GEQ;
  d_solver->addConstraint(lhs, rhs.toString(), op);
  // TODO: ++d_statistics.d_numConstraints;
}

void RoundingSatSolver::addGuardedGeq(
    const std::vector<std::pair<Integer, RsVar>>& terms,
    const Integer& rhs,
    RsVar selector)
{
  // L = sum of the negative coefficients = minimum value of the left-hand side.
  Integer lower(0);
  for (const auto& [coeff, var] : terms)
  {
    if (coeff.sgn() < 0) lower += coeff;
  }
  // Guarded constraint: terms + (L - rhs) * selector >= L.
  //   selector = 1 -> sum >= rhs (the original constraint)
  //   selector = 0 -> sum >= L   (always true)
  Integer selCoeff = lower - rhs;

  std::vector<std::pair<std::string, rs::api::Lit>> lhs;
  lhs.reserve(terms.size() + 1);
  for (const auto& [coeff, var] : terms)
  {
    lhs.emplace_back(coeff.toString(), var);
  }
  // When selCoeff is zero the selector cannot toggle the (already trivially
  // satisfied) constraint, so the term is omitted.
  if (selCoeff.sgn() != 0)
  {
    lhs.emplace_back(selCoeff.toString(), selector);
  }
  d_solver->addConstraint(lhs, lower.toString(), rs::api::PbRelOp::GEQ);
}

void RoundingSatSolver::addConstraint(const Node constraint,
                                      const Node selector)
{
  Trace("bv-pb-roundingsat") << "RoundingSatSolver::addConstraint " << constraint
                             << " guarded by " << selector << "\n";
  if (!d_constraintSet.emplace(constraint).second) return;

  RsVar sel = toRsVar(selector);
  std::vector<std::pair<Integer, RsVar>> terms;
  Integer rhs;
  Kind k = parseConstraint(constraint, terms, rhs);

  addGuardedGeq(terms, rhs, sel);
  if (k == Kind::EQUAL)
  {
    // The other direction: -sum >= -rhs, also guarded by the selector.
    std::vector<std::pair<Integer, RsVar>> negated;
    negated.reserve(terms.size());
    for (const auto& [coeff, var] : terms)
    {
      negated.emplace_back(-coeff, var);
    }
    addGuardedGeq(negated, -rhs, sel);
  }
  // TODO: ++d_statistics.d_numConstraints;
}

void RoundingSatSolver::reset()
{
  d_constraintSet.clear();
  d_nodeToVar.clear();
  d_varToNode.clear();
  d_nameToVar.clear();
  d_solver->reset();
}

std::vector<std::string> RoundingSatSolver::getProof()
{
  return d_solver->getProof();
}

PbSolveState RoundingSatSolver::solve() { return solve(std::vector<Node>{}); }

PbSolveState RoundingSatSolver::solve(const std::vector<Node>& assumptions)
{
  std::vector<rs::api::Lit> lits;
  lits.reserve(assumptions.size());
  for (const Node& assumption : assumptions)
  {
    lits.push_back(toRsVar(assumption));  // positive literal
  }
  switch (d_solver->solve(lits))
  {
    case rs::api::PbSolveState::SAT: return PB_SAT;
    case rs::api::PbSolveState::UNSAT: return PB_UNSAT;
    default: return PB_UNKNOWN;
  }
}

std::vector<Node> RoundingSatSolver::getUnsatCore()
{
  std::vector<Node> core;
  for (rs::api::Lit lit : d_solver->getUnsatCore())
  {
    RsVar v = lit < 0 ? -lit : lit;
    auto it = d_varToNode.find(v);
    Assert(it != d_varToNode.end());
    core.push_back(it->second);
  }
  return core;
}

PbValue RoundingSatSolver::modelValue(const VariableId variable)
{
  auto it = d_nameToVar.find(variable);
  Assert(it != d_nameToVar.end());
  return d_solver->modelValue(it->second) ? PB_TRUE : PB_FALSE;
}

RoundingSatSolver::Statistics::Statistics(
    CVC5_UNUSED StatisticsRegistry& registry,
    CVC5_UNUSED const std::string& prefix)
{
}

}  // namespace pb
}  // namespace bv
}  // namespace theory
}  // namespace cvc5::internal

#endif  // CVC5_USE_ROUNDINGSAT
