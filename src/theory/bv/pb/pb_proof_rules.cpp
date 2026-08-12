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
 * PB proof rules
 */

#include "theory/bv/pb/pb_proof_rules.h"

#include <stack>

#include "util/string.h"

namespace cvc5::internal {
namespace theory {
namespace bv {
namespace pb {

PbProofRules::PbProofRules(Env& env, CDProof* cdp) : EnvObj(env), d_cdp(cdp)
{
  initializeRules();
}

void PbProofRules::initializeRules()
{
  rules = {
      {"del",
       [this](std::istringstream& iss) { return deleteConstraints2(iss); }},
      {"d", [this](std::istringstream& iss) { return deleteConstraints(iss); }},
      {"a", [this](std::istringstream& iss) { return assumption(iss); }},
      {"u",
       [this](std::istringstream& iss) { return reverseUnitPropagation(iss); }},
      {"rup",
       [this](std::istringstream& iss) { return reverseUnitPropagation(iss); }},
      {"e", [this](std::istringstream& iss) { return constraintEquals(iss); }},
      {"i", [this](std::istringstream& iss) { return constraintImplies(iss); }},
      {"ia",
       [this](std::istringstream& iss) { return syntacticImpliesAdd(iss); }},
      {"j",
       [this](std::istringstream& iss) {
         return constraintImpliesGetImplied(iss);
       }},
      {"v", [this](std::istringstream& iss) { return solution(iss); }},
      {"ov", [this](std::istringstream& iss) { return originalSolution(iss); }},
      {"o", [this](std::istringstream& iss) { return objectiveBound(iss); }},
      {"c", [this](std::istringstream& iss) { return isContradiction(iss); }},
      {"p",
       [this](std::istringstream& iss) { return reversePolishNotation(iss); }},
      {"pol",
       [this](std::istringstream& iss) { return reversePolishNotation(iss); }},
      {"f", [this](std::istringstream& iss) { return loadFormula(iss); }},
      {"l", [this](std::istringstream& iss) { return loadAxiom(iss); }},
      {"core", [this](std::istringstream& iss) { return markCore(iss); }},
      {"#", [this](std::istringstream& iss) { return setLevel(iss); }},
      {"w", [this](std::istringstream& iss) { return wipeLevel(iss); }},
      {"red", [this](std::istringstream& iss) { return redundancy(iss); }},
      {"output",
       [this](std::istringstream& iss) { return outputSection(iss); }},
      {"conclusion",
       [this](std::istringstream& iss) { return conclusionSection(iss); }},
      {"end", [this](std::istringstream& iss) { return endProof(iss); }}};
}

Node PbProofRules::parseLine(const std::string& line)
{
  std::istringstream iss(line);
  std::string ruleId;
  iss >> ruleId;
  auto it = rules.find(ruleId);
  if (it == rules.end())
  {
    Unreachable() << "\nPbProofRules::parseLine: failed parsing line:\n"
                  << line << "\n";
  }
  return it->second(iss);
}

Node PbProofRules::assumption(std::istringstream& iss)
{
  Trace("bv-pb-proof") << "PbProofRules::assumption\n";
  NodeManager* nm = nodeManager();
  Node constraint = parseOpbFormat(iss);

  // RoundingSat's logAssumption emits no trailing separator, but we tolerate a
  // stray ';' defensively. Any other leftover token is a parsing error.
  std::string token;
  while (iss >> token)
  {
    if (token != ";")
      Unreachable() << "\nPbProofRules::assumption: unexpected token '" << token
                    << "'\n";
  }
  return nm->mkNode(Kind::PB_PROOF_ASSUMPTION, constraint);
}

Node PbProofRules::constraintEquals(CVC5_UNUSED std::istringstream& iss)
{
  Unimplemented();
}

Node PbProofRules::constraintImplies(CVC5_UNUSED std::istringstream& iss)
{
  Unimplemented();
}

Node PbProofRules::constraintImpliesGetImplied(
    CVC5_UNUSED std::istringstream& iss)
{
  Unimplemented();
}

Node PbProofRules::deleteConstraints(CVC5_UNUSED std::istringstream& iss)
{
  Unimplemented();
}

Node PbProofRules::deleteConstraints2(std::istringstream& iss)
{
  Trace("bv-pb-proof") << "PbProofRules::deleteConstraints2\n";
  NodeManager* nm = nodeManager();

  std::string sub;
  iss >> sub;
  if (sub != "id")
  {
    // 'del spec <ineq>' (VeriPB 3.0 augmented, DRAT-style) not yet supported;
    // RoundingSat does not emit it.
    Unimplemented() << "\nPbProofRules::deleteConstraints2: unsupported "
                    << "'del " << sub << "' form\n";
  }

  std::vector<Node> ids;
  std::string token;
  while (iss >> token)
  {
    if (token == ";") continue;  // defensive; RoundingSat emits no terminator
    ids.push_back(nm->mkBoundVar(token, nm->stringType()));
  }
  Node id_list = nm->mkNode(Kind::SEXPR, ids);
  return nm->mkNode(Kind::PB_PROOF_DELETE_BY_ID, id_list);
}

Node PbProofRules::isContradiction(std::istringstream& iss)
{
  Trace("bv-pb-proof") << "PbProofRules::isContradiction\n";
  std::string constraint_id;
  iss >> constraint_id;

  if (!iss.eof())
  {
    std::string remaining;
    iss >> remaining;
    if (!iss.eof() || remaining != "0") Unreachable() << "\nParsing error\n";
  }

  NodeManager* nm = nodeManager();
  Node constraint = nm->mkBoundVar(constraint_id, nm->stringType());
  return nm->mkNode(Kind::PB_PROOF_CONTRADICTION, constraint);
}

Node PbProofRules::loadAxiom(std::istringstream& iss)
{
  Trace("bv-pb-proof") << "PbProofRules::loadAxiom\n";
  size_t axiom_id;
  iss >> axiom_id;

  if (!iss.eof()) Unreachable() << "\nParsing error\n";

  NodeManager* nm = nodeManager();
  return nm->mkConstInt(axiom_id);
}

Node PbProofRules::loadFormula(std::istringstream& iss)
{
  Trace("bv-pb-proof") << "PbProofRules::loadFormula\n";
  size_t num_constraints;
  iss >> num_constraints;

  if (!iss.eof()) Unreachable() << "\nParsing error\n";

  NodeManager* nm = nodeManager();
  return nm->mkNode(Kind::PB_PROOF_LOAD_FORMULA,
                    nm->mkConstInt(Rational(num_constraints)));
}

Node PbProofRules::markCore(CVC5_UNUSED std::istringstream& iss)
{
  Unimplemented();
}

Node PbProofRules::objectiveBound(CVC5_UNUSED std::istringstream& iss)
{
  Unimplemented();
}

Node PbProofRules::originalSolution(CVC5_UNUSED std::istringstream& iss)
{
  Unimplemented();
}

Node PbProofRules::reversePolishNotation(std::istringstream& iss)
{
  Trace("bv-pb-proof") << "PbProofRules::reversePolishNotation\n";
  Node pol_constraint = parsePolishNotation(iss);
  NodeManager* nm = nodeManager();
  return nm->mkNode(Kind::PB_PROOF_REVERSE_POLISH_NOTATION, pol_constraint);
}

Node PbProofRules::reverseUnitPropagation(std::istringstream& iss)
{
  Trace("bv-pb-proof") << "PbProofRules::reverseUnitPropagation\n";
  NodeManager* nm = nodeManager();
  Node rup_constraint = parseOpbFormat(iss);

  std::vector<Node> hint_ids;
  std::string token;
  while (iss >> token)
  {
    // Tolerate stray ';' (VeriPB 2.0) and ':' (VeriPB 3.0 hint delimiter).
    if (token == ";" || token == ":") continue;
    hint_ids.push_back(nm->mkBoundVar(token, nm->stringType()));
  }
  Node hints = nm->mkNode(Kind::SEXPR, hint_ids);
  return nm->mkNode(Kind::PB_PROOF_REVERSE_UNIT_PROPAGATION,
                    rup_constraint,
                    hints);
}

Node PbProofRules::setLevel(CVC5_UNUSED std::istringstream& iss)
{
  Unimplemented();
}

Node PbProofRules::solution(CVC5_UNUSED std::istringstream& iss)
{
  Unimplemented();
}

Node PbProofRules::syntacticImpliesAdd(std::istringstream& iss)
{
  Trace("bv-pb-proof") << "PbProofRules::syntacticImpliesAdd\n";
  NodeManager* nm = nodeManager();
  Node constraint = parseOpbFormat(iss);

  std::vector<Node> hint_ids;
  std::string token;
  while (iss >> token)
  {
    // Tolerate stray ';' (VeriPB 2.0) and ':' (VeriPB 3.0 hint delimiter).
    if (token == ";" || token == ":") continue;
    hint_ids.push_back(nm->mkBoundVar(token, nm->stringType()));
  }
  Node hints = nm->mkNode(Kind::SEXPR, hint_ids);
  return nm->mkNode(Kind::PB_PROOF_SYNTACTIC_IMPLIES_ADD, constraint, hints);
}

Node PbProofRules::wipeLevel(CVC5_UNUSED std::istringstream& iss)
{
  Unimplemented();
}

Node PbProofRules::redundancy(std::istringstream& iss)
{
  Trace("bv-pb-proof") << "PbProofRules::redundancy\n";
  NodeManager* nm = nodeManager();
  Node constraint = parseOpbFormat(iss);

  // Witness tokens up to the ';' terminator: each token is a piece of the
  // substitution (e.g. 'xV', '->', '1' or 'xU'). RoundingSat may emit an empty
  // witness ('addWitness({})' -> just '; '), in which case the SEXPR is empty.
  std::vector<Node> witness;
  std::string token;
  while (iss >> token)
  {
    if (token == ";") break;
    witness.push_back(nm->mkBoundVar(token, nm->stringType()));
  }
  Node witness_sexpr = nm->mkNode(Kind::SEXPR, witness);

  // Optional subproof body: everything after the ';' (typically a block
  // 'begin\n ... \nend' that the line-gluer in PbProofManager::parseProofLines
  // has already collapsed into this single string). Stored verbatim; consumers
  // that need structured access to inner steps can re-parse it.
  std::ostringstream rest;
  rest << iss.rdbuf();
  std::string subproof_text = rest.str();
  size_t start = subproof_text.find_first_not_of(" \t\n\r");
  subproof_text =
      (start == std::string::npos) ? std::string() : subproof_text.substr(start);
  Node subproof = nm->mkConst(String(subproof_text));

  return nm->mkNode(Kind::PB_PROOF_REDUNDANCY,
                    constraint,
                    witness_sexpr,
                    subproof);
}

Node PbProofRules::outputSection(std::istringstream& iss)
{
  Trace("bv-pb-proof") << "PbProofRules::outputSection\n";
  NodeManager* nm = nodeManager();
  std::vector<Node> tokens;
  std::string token;
  while (iss >> token)
  {
    if (token == ";") continue;
    tokens.push_back(nm->mkBoundVar(token, nm->stringType()));
  }
  Node content = nm->mkNode(Kind::SEXPR, tokens);
  return nm->mkNode(Kind::PB_PROOF_OUTPUT, content);
}

Node PbProofRules::conclusionSection(std::istringstream& iss)
{
  Trace("bv-pb-proof") << "PbProofRules::conclusionSection\n";
  NodeManager* nm = nodeManager();
  std::vector<Node> tokens;
  std::string token;
  while (iss >> token)
  {
    // Skip ';' (statement terminator) and ':' (UNSAT/SAT payload separator).
    if (token == ";" || token == ":") continue;
    tokens.push_back(nm->mkBoundVar(token, nm->stringType()));
  }
  Node content = nm->mkNode(Kind::SEXPR, tokens);
  return nm->mkNode(Kind::PB_PROOF_CONCLUSION, content);
}

Node PbProofRules::endProof(std::istringstream& iss)
{
  Trace("bv-pb-proof") << "PbProofRules::endProof\n";
  NodeManager* nm = nodeManager();
  std::vector<Node> tokens;
  std::string token;
  while (iss >> token)
  {
    tokens.push_back(nm->mkBoundVar(token, nm->stringType()));
  }
  Node content = nm->mkNode(Kind::SEXPR, tokens);
  return nm->mkNode(Kind::PB_PROOF_END, content);
}

// Based on http://www.cril.univ-artois.fr/PB12/format.pdf
Node PbProofRules::parseOpbFormat(std::istringstream& iss)
{
  Trace("bv-pb-proof") << "PbProofRules::parseOpbFormat\n";
  NodeManager* nm = nodeManager();
  std::vector<std::string> sum;
  std::string coefficient;
  std::string variable;
  std::string rhs;
  Kind relational_operator;

  iss >> coefficient;
  while (coefficient[0] == '+' || coefficient[0] == '-')
  {
    sum.push_back(coefficient);
    iss >> variable;
    sum.push_back(variable);
    iss >> coefficient;
  }

  if (coefficient[0] == '=')
  {
    relational_operator = Kind::EQUAL;
    if (coefficient == "=")
      iss >> rhs;
    else
      rhs = coefficient.substr(1);
    if (rhs.back() == ';') rhs.pop_back();
  }

  else
  {
    relational_operator = Kind::GEQ;
    if (coefficient == ">=")
      iss >> rhs;
    else
      rhs = coefficient.substr(2);
    if (rhs.back() == ';') rhs.pop_back();
  }

  std::vector<Node> sum_nodes;
  for (size_t i = 0; i < sum.size(); i += 2)
  {
    Node coefficient_node = nm->mkConstInt(Rational(Integer(sum[i])));
    Node variable_node = nm->mkBoundVar(sum[i + 1], nm->integerType());
    sum_nodes.push_back(
        nm->mkNode(Kind::MULT, coefficient_node, variable_node));
  }

  /* TODO(alanctprado)
   *
   * How to deal with cases where the left hand side is empty? For instance,
   *
   *     u >= 0 ;
   *
   * This seems to arise in other scenarios in VeriPB's README, but there is no
   * further explanation.
   *
   *     >= 1 ;
   *     >= 3 ;
   *     3 x1 -2 x2 >= 4 ;
   *
   */
  Node lhs_node;
  if (sum_nodes.size() == 0)
    lhs_node = nm->mkConstInt(Rational(0));
  else
    lhs_node = nm->mkNode(Kind::ADD, sum_nodes);

  Node rhs_node = nm->mkConstInt(Rational(Integer(rhs)));

  return nm->mkNode(relational_operator, lhs_node, rhs_node);
}

// Based on
// https://github.com/StephanGocht/VeriPB?tab=readme-ov-file#reverse-polish-notation
Node PbProofRules::parsePolishNotation(std::istringstream& iss)
{
  Trace("bv-pb-proof") << "PbProofRules::parsePolishNotation\n";
  NodeManager* nm = nodeManager();
  std::string formula;
  std::string token;
  std::stack<Node> stack;

  auto popOne = [&stack]() -> Node {
    Node n = stack.top();
    stack.pop();
    return n;
  };
  auto popTwo = [&stack]() -> std::pair<Node, Node> {
    Node rhs = stack.top();
    stack.pop();
    Node lhs = stack.top();
    stack.pop();
    return {lhs, rhs};
  };

  while (iss >> token)
  {
    if (token == "+")
      stack.push(polishAddition(popTwo()));
    else if (token == "*")
      stack.push(polishMultiplication(popTwo()));
    else if (token == "d")
      stack.push(polishDivision(popTwo()));
    else if (token == "s")
      stack.push(polishSaturation(popOne()));
    else if (token == "w")
      stack.push(polishWeakening(popTwo()));
    else
      stack.push(nm->mkConst(String(token)));
  }

  if (stack.size() == 2) stack.pop();  // Remove final 0
  if (stack.size() != 1) Unreachable() << "\nFailed parsing RPN.\n";
  return stack.top();
}

Node PbProofRules::polishAddition(std::pair<Node, Node> operands)
{
  NodeManager* nm = nodeManager();
  auto [lhs, rhs] = operands;
  Node lhs_constraint = polishConstraint(lhs);
  Node rhs_constraint = polishConstraint(rhs);
  return nm->mkNode(Kind::ADD, lhs_constraint, rhs_constraint);
}

Node PbProofRules::polishDivision(std::pair<Node, Node> operands)
{
  NodeManager* nm = nodeManager();
  auto [lhs, rhs] = operands;
  Node constraint = polishConstraint(lhs);
  if (rhs.getKind() != Kind::CONST_STRING) Unreachable();
  Node factor =
      nm->mkConstInt(Rational(Integer(rhs.getConst<String>().toString())));
  return nm->mkNode(Kind::DIVISION, constraint, factor);
}

Node PbProofRules::polishMultiplication(std::pair<Node, Node> operands)
{
  NodeManager* nm = nodeManager();
  auto [lhs, rhs] = operands;
  Node constraint = polishConstraint(lhs);
  if (rhs.getKind() != Kind::CONST_STRING) Unreachable();
  Node factor =
      nm->mkConstInt(Rational(Integer(rhs.getConst<String>().toString())));
  return nm->mkNode(Kind::MULT, constraint, factor);
}

Node PbProofRules::polishSaturation(Node operand) { return operand; }

Node PbProofRules::polishWeakening(std::pair<Node, Node> operands)
{
  return operands.second;
}

Node PbProofRules::polishConstraint(Node node)
{
  NodeManager* nm = nodeManager();
  // case 1: already processed
  if (node.getKind() != Kind::CONST_STRING) return node;
  std::string content = node.getConst<String>().toString();
  // case 2: literal axiom
  if (content[0] == 'x')
  {
    Node variable = nm->mkBoundVar(content, nm->integerType());
    Node lhs = nm->mkNode(Kind::MULT, nm->mkConstInt(Rational(1)), variable);
    Node rhs = nm->mkConstInt(Rational(0));
    return nm->mkNode(Kind::GEQ, lhs, rhs);
  }
  if (content[0] == '~')
  {
    Node variable = nm->mkBoundVar(content.substr(1), nm->integerType());
    Node lhs = nm->mkNode(Kind::MULT, nm->mkConstInt(Rational(-1)), variable);
    Node rhs = nm->mkConstInt(Rational(-1));
    return nm->mkNode(Kind::GEQ, lhs, rhs);
  }
  // case 3: constraint id
  return nm->mkBoundVar(content, nm->stringType());
}

}  // namespace pb
}  // namespace bv
}  // namespace theory
}  // namespace cvc5::internal
