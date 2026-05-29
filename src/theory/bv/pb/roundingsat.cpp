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

void RoundingSatSolver::addConstraint(const Node constraint)
{
  Trace("bv-pb-roundingsat")
      << "RoundingSatSolver::addConstraint " << constraint << "\n";
  if (!d_constraintSet.emplace(constraint).second) return;

  // Build the left-hand side as (coefficient, literal) pairs. Coefficients are
  // passed as decimal strings to preserve arbitrary precision (bit-vector
  // place values exceed int64_t for wide bit-vectors). All PB variables appear
  // as positive literals.
  std::vector<std::pair<std::string, rs::api::Lit>> lhs;

  const Node& linear_form = constraint[0];
  auto pushTerm = [&](const Node& term) {
    Assert(term.getNumChildren() == 2);
    Assert(term[0].isConst());
    Assert(term[1].isVar());
    const Rational& coeff = term[0].getConst<Rational>();
    Assert(coeff.isIntegral());
    lhs.emplace_back(coeff.getNumerator().toString(), toRsVar(term[1]));
  };

  if (linear_form.getKind() == Kind::MULT)
  {
    pushTerm(linear_form);
  }
  else if (linear_form.getKind() == Kind::ADD)
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
  std::string rhs = rhsRat.getNumerator().toString();

  rs::api::PbRelOp op = (constraint.getKind() == Kind::EQUAL)
                            ? rs::api::PbRelOp::EQUAL
                            : rs::api::PbRelOp::GEQ;

  d_solver->addConstraint(lhs, rhs, op);
  // TODO: ++d_statistics.d_numConstraints;
}

void RoundingSatSolver::reset()
{
  d_constraintSet.clear();
  d_nodeToVar.clear();
  d_nameToVar.clear();
  d_solver->reset();
}

std::vector<std::string> RoundingSatSolver::getProof()
{
  return d_solver->getProof();
}

PbSolveState RoundingSatSolver::solve()
{
  switch (d_solver->solve())
  {
    case rs::api::PbSolveState::SAT: return PB_SAT;
    case rs::api::PbSolveState::UNSAT: return PB_UNSAT;
    default: return PB_UNKNOWN;
  }
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
