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
 * Wrapper for the Exact PB Solver.
 *
 * Implementation of the Exact PB solver for cvc5 (bit-vectors).
 */

#ifdef CVC5_USE_EXACT

#include "theory/bv/pb/exact.h"

// #include "base/check.h"
// #include "options/main_options.h"
// #include "options/proof_options.h"
// #include "prop/theory_proxy.h"
// #include "util/resource_manager.h"
// #include "util/statistics_registry.h"
#include "util/rational.h"

namespace cvc5::internal {
namespace theory {
namespace bv {
namespace pb {

ExactSolver::ExactSolver(Env& env,
                         StatisticsRegistry& registry,
                         const std::string& name,
                         bool logProofs)
    : EnvObj(env),
      d_solver(new Exact()),
      d_logProofs(logProofs),
      d_statistics(registry, name)
{
  if (logProofs) Unimplemented();
  init();  // TODO: Remove this? Make a private constructor?
}

void ExactSolver::init()
{
  d_solver->setOption("verbosity", "0");
  if (d_logProofs)
  { /** TODO */
  }
}

void ExactSolver::addVariable(const Node variable)
{
  Trace("bv-pb-exact") << "ExactSolver::addVariable " << variable << "\n";
  Assert(variable.isVar());
  if (d_variableSet.count(variable)) return;
  // d_solver->addVariable(variable.getName().substr(1), 0, 1);
  d_solver->addVariable(variable.getName(), 0, 1);
  d_variableSet.emplace(variable);
  // TODO: ++d_statistics.d_numVariables;
}

void ExactSolver::addConstraint(const Node constraint)
{
  Trace("bv-pb-exact") << "ExactSolver::addConstraint " << constraint << "\n";
  if (d_constraintSet.count(constraint)) return;

  std::vector<std::string> variables, coefficients;

  Node linear_form = constraint[0];

  if (linear_form.getKind() == Kind::MULT)
  {
    Assert(linear_form.getNumChildren() == 2);
    Assert(linear_form[0].isConst());
    Assert(linear_form[1].isVar());
    coefficients.push_back(linear_form[0].getConst<Rational>().toString());
    if (!d_variableSet.count(linear_form[1])) addVariable(linear_form[1]);
    variables.push_back(linear_form[1].toString());
  }

  else if (linear_form.getKind() == Kind::ADD)
  {
    for (const Node& term : linear_form)
    {
      Assert(term.getNumChildren() == 2);
      Assert(term[0].isConst());
      Assert(term[1].isVar());
      coefficients.push_back(term[0].getConst<Rational>().toString());
      if (!d_variableSet.count(term[1])) addVariable(term[1]);
      variables.push_back(term[1].toString());
    }
  }

  else
    Unreachable();

  bool use_ub = constraint.getKind() == Kind::EQUAL;
  std::string rhs = constraint[1].getConst<Rational>().toString();
  d_solver->addConstraint(coefficients, variables, 1, rhs, use_ub, rhs);

  // TODO: ++d_statistics.d_numConstraints;
  d_constraintSet.emplace(constraint);
}

PbSolveState ExactSolver::solve()
{
  SolveState s = d_solver->runFull(0);
  switch (s)
  {
    case SolveState::SAT: return PB_SAT;
    case SolveState::UNSAT: return PB_UNSAT;
    default: return PB_UNKNOWN;
  }
}

void ExactSolver::reset()
{
  // Exact has no in-place reset, so recreate the underlying instance to clear
  // all variables and constraints and re-apply the initial options.
  d_solver.reset(new Exact());
  d_variableSet.clear();
  d_constraintSet.clear();
  init();
}

PbValue ExactSolver::modelValue(CVC5_UNUSED const VariableId variable)
{
  // TODO(alanctprado): extract the satisfying assignment from Exact.
  Unimplemented();
}

std::vector<std::string> ExactSolver::getProof()
{
  // TODO(alanctprado): proof logging is not yet supported by the Exact
  // back-end (see the logProofs guard in the constructor).
  Unimplemented();
}

ExactSolver::Statistics::Statistics(CVC5_UNUSED StatisticsRegistry& registry,
                                    CVC5_UNUSED const std::string& prefix)
{
}

}  // namespace pb
}  // namespace bv
}  // namespace theory
}  // namespace cvc5::internal

#endif  // CVC5_USE_EXACT
