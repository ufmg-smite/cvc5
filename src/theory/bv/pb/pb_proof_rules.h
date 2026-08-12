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
#include "cvc5_private.h"

#ifndef CVC5__THEORY__BV__PB__PB_PROOF_RULES_H
#define CVC5__THEORY__BV__PB__PB_PROOF_RULES_H

#include "proof/proof.h"
#include "smt/env_obj.h"

namespace cvc5::internal {

namespace theory {
namespace bv {
namespace pb {

class PbProofRules : protected EnvObj
{
 public:
  PbProofRules(Env& env, CDProof* cdp);
  ~PbProofRules() {};

  Node parseLine(const std::string& line);

 private:
  CVC5_UNUSED_FIELD CDProof* d_cdp;  // TODO(alanctprado): used once proofs land

  /** Rules */
  Node assumption(std::istringstream&);
  Node conclusionSection(std::istringstream&);
  Node constraintEquals(std::istringstream&);
  Node constraintImplies(std::istringstream&);
  Node constraintImpliesGetImplied(std::istringstream&);
  Node deleteConstraints(std::istringstream&);
  Node deleteConstraints2(std::istringstream&);
  Node endProof(std::istringstream&);
  Node isContradiction(std::istringstream&);
  Node loadAxiom(std::istringstream&);
  Node loadFormula(std::istringstream&);
  Node markCore(std::istringstream&);
  Node objectiveBound(std::istringstream&);
  Node originalSolution(std::istringstream&);
  Node outputSection(std::istringstream&);
  Node redundancy(std::istringstream&);
  Node reversePolishNotation(std::istringstream&);
  Node reverseUnitPropagation(std::istringstream&);
  Node setLevel(std::istringstream&);
  Node solution(std::istringstream&);
  Node syntacticImpliesAdd(std::istringstream&);
  Node wipeLevel(std::istringstream&);

  /** Internal */
  void initializeRules();
  Node parseOpbFormat(std::istringstream&);

  /**
   * Parse a VeriPB 'pol' step into a derivation expression: a tree of
   * PB_PROOF_POL_* operations whose leaves are constraint ids or literal
   * axioms. Leaves are left unresolved; only PbProofTranslator holds the
   * core/derived databases needed to turn an id into a constraint.
   */
  Node parsePolishNotation(std::istringstream&);
  Node polishAddition(std::pair<Node, Node>);
  Node polishDivision(std::pair<Node, Node>);
  Node polishMultiplication(std::pair<Node, Node>);
  Node polishSaturation(Node);
  Node polishWeakening(std::pair<Node, Node>);
  Node polishConstraint(Node);
  /** `op` names the caller for diagnostics. */
  Node polishConstant(Node, const char* op);

  /**
   * The Node for PB variable `name`, an integer-typed variable ranging over
   * {0, 1}. NodeManager::mkBoundVar mints a fresh node per call, so interning
   * here is what makes two occurrences of the same variable compare equal.
   */
  Node mkVariable(const std::string& name);

  std::unordered_map<std::string, std::function<Node(std::istringstream&)>>
      rules;

  std::unordered_map<std::string, Node> d_variables;
};

}  // namespace pb
}  // namespace bv
}  // namespace theory
}  // namespace cvc5::internal

#endif /* CVC5__THEORY__BV__PB__PB_PROOF_RULES_H */
