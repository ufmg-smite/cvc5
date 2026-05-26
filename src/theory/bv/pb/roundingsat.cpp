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
 * Wrapper for the RoundingSat PB Solver.
 *
 * Implementation of the RoundingSat PB solver for cvc5 (bit-vectors).
 */

#ifdef CVC5_USE_ROUNDINGSAT

#include "theory/bv/pb/roundingsat.h"

// #include "base/check.h"
// #include "options/main_options.h"
// #include "options/proof_options.h"
// #include "prop/theory_proxy.h"
// #include "util/resource_manager.h"
// #include "util/statistics_registry.h"
#include <cstdio>

#include "util/rational.h"

namespace cvc5::internal {
namespace theory {
namespace bv {
namespace pb {

RoundingSatSolver::RoundingSatSolver(std::string solverPath,
                                     Env& env,
                                     StatisticsRegistry& registry,
                                     const std::string& name,
                                     bool logProofs)
    : EnvObj(env),
      d_binPath(solverPath),
      d_logProofs(logProofs),
      d_statistics(registry, name)
{
}

void RoundingSatSolver::addVariable(const Node variable)
{
  Trace("bv-pb-roundingsat")
      << "RoundingSatSolver::addVariable " << variable << "\n";
  Assert(variable.isVar());
  if (d_variableSet.count(variable)) return;
  d_variableSet.emplace(variable);
  // TODO: ++d_statistics.d_numVariables;
}

void RoundingSatSolver::addConstraint(const Node constraint)
{
  Trace("bv-pb-roundingsat")
      << "RoundingSatSolver::addConstraint " << constraint << "\n";
  if (d_constraintSet.count(constraint)) return;

  std::vector<std::string> variables, coefficients;
  std::string result;

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

  for (unsigned i = 0; i < variables.size(); i++)
  {
    if (coefficients[i][0] != '-') result += "+";
    result += coefficients[i] + " ";
    result += variables[i] + " ";
  }

  if (constraint.getKind() == Kind::EQUAL)
    result += "= ";
  else
    result += ">= ";
  result += constraint[1].getConst<Rational>().toString();
  result += ";\n";

  // TODO: ++d_statistics.d_numConstraints;
  d_pboConstraints.push_back(result);
  Trace("bv-pb-roundingsat") << "results in " << result;
}

void RoundingSatSolver::writeInput(std::string path)
{
  std::ofstream file(path);
  file << "* #variable= " << d_variableSet.size()
       << " #constraint= " << d_pboConstraints.size() << "\n";
  for (std::string constraint : d_pboConstraints)
  {
    file << constraint;
  }
  file.close();

  if (TraceIsOn("bv-pb-rs-input"))
  {
    std::ifstream ifile(path);
    Trace("bv-pb-rs-input") << "Contents of the file " << path << " are:\n";
    ifile.seekg(0);
    std::string line;
    while (getline(ifile, line))
    {
      Trace("bv-pb-rs-input") << line << '\n';
    }
    ifile.close();
  }
}

std::string RoundingSatSolver::buildCliCommand(std::string input_path,
                                               std::string output_path,
                                               std::string proof_path)
{
  std::string command = d_binPath;
  command += " --bits-learned=0 --bits-overflow=0 --bits-reduced=0 --lp=0";
  if (d_logProofs)
  {
    command += " --proof-log=" + proof_path;
  }
  command += " " + input_path + " > " + output_path;
  Trace("bv-pb-roundingsat") << "Built command: " << command << "\n";
  return command;
}

void RoundingSatSolver::reset()
{
  d_constraintSet.clear();
  d_variableSet.clear();
  d_pboConstraints.clear();
}

PbSolveState RoundingSatSolver::parseOutput(std::string output_path)
{
  std::ifstream output(output_path);
  std::string line, result;
  Trace("bv-pb-rs-output") << "RoundingSat result:\n";
  while (getline(output, line))
  {
    Trace("bv-pb-rs-output") << line << '\n';
    if (line[0] == 's')
    {
      result = line.substr(2, line.length() - 2);
    }
  }
  output.close();
  if (result == "SATISFIABLE")
  {
    // computeSatisfyingAssignment();
    return PB_SAT;
  }
  if (result == "UNSATISFIABLE") return PB_UNSAT;
  return PB_UNKNOWN;
}

void RoundingSatSolver::parseProof(std::string proof_path)
{
  if (TraceIsOn("bv-pb-proof"))
  {
    d_proofLines.clear();
    std::ifstream proof_file(proof_path + ".proof");
    std::string line;
    while (getline(proof_file, line))
    {
      d_proofLines.push_back(line);
    }
  }
}

std::vector<std::string> RoundingSatSolver::getProof() { return d_proofLines; }

PbSolveState RoundingSatSolver::solve()
{
  std::string input_path = std::string(std::tmpnam(nullptr)) + ".pbo";
  std::string output_path = std::string(std::tmpnam(nullptr)) + "txt";
  std::string proof_path = std::string(std::tmpnam(nullptr));

  writeInput(input_path);
  std::string command = buildCliCommand(input_path, output_path, proof_path);

  std::system(command.c_str());

  if (d_logProofs) parseProof(proof_path);

  return parseOutput(output_path);
}

void RoundingSatSolver::computeSatisfyingAssignment()
{
  Unimplemented();
  // std::string output_file = std::tmpnam(nullptr);
  // output_file += ".txt";
  // std::string command = d_binPath;
  // command += " --bits-learned=0 --bits-overflow=0 --bits-reduced=0 --lp=0";
  // command += " --print-sol=1";
  // command += " " + d_pboPath + " > " + output_file;
  // Trace("bv-pb-debug") << "    The command is: " << command << "\n";

  // std::system(command.c_str());

  // std::fstream output;
  // output.open(output_file);
  // Trace("bv-pb-debug") << "RoundingSat result:\n";
  // std::string line;
  // std::string result;
  // std::string assignment;
  // while (getline (output,line))
  // {
  //   // Trace("bv-pb-debug") << line << '\n';
  //   if (line[0] == 's') result = line.substr(2, line.length() - 2);
  //   if (line[0] == 'v') assignment = line.substr(2, line.length() - 2);
  // }
  // output.close();
  // Assert(result == "SATISFIABLE");
  // Trace("bv-pb-debug") << "Assignment:\n";
  // Trace("bv-pb-debug") << assignment;

  // std::vector<std::string> variables;
  // std::stringstream ss(assignment);
  // std::string var;
  // while (ss >> var) variables.push_back(var);

  // d_assignmentMap.clear();
  // for (const auto& variable : variables) {
  //   int value = (variable[0] == '-') ? 0 : 1;
  //   VariableId variable_id = variable.substr(1 - value);
  //   d_assignmentMap[variable_id] = (value == 0) ? PB_FALSE : PB_TRUE;
  //   Trace("bv-pb-debug") << variable << " = " << d_assignmentMap[variable_id]
  //   << "\n";
  // }
}

PbValue RoundingSatSolver::modelValue(const VariableId variable)
{
  return d_assignmentMap[variable];
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
