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
 * TODO(alanctprado)
 */

#include "theory/bv/pb/pb_proof_manager.h"

#include <utility>

#include "options/bv_options.h"
#include "proof/proof.h"
#include "proof/proof_node.h"
#include "proof/proof_node_manager.h"
#include "proof/trust_id.h"
#include "util/string.h"

namespace cvc5::internal {
namespace theory {
namespace bv {
namespace pb {

PbProofManager::PbProofManager(Env& env, PbBlastProofGenerator* pbbpg)
    : EnvObj(env),
      d_pbbpg(pbbpg),
      d_cdp(new CDProof(env)),
      d_pbpr(new PbProofRules(env, d_cdp))
{
}

// True if the kind would consume a fresh VeriPB constraint id (i.e. produces a
// derivation that later steps may hint by id). loadFormula, deletion, and the
// output/conclusion/end footer markers do not advance the id counter.
static bool advancesVeriPbId(Kind k)
{
  return k == Kind::PB_PROOF_ASSUMPTION
         || k == Kind::PB_PROOF_REVERSE_UNIT_PROPAGATION
         || k == Kind::PB_PROOF_REVERSE_POLISH_NOTATION
         || k == Kind::PB_PROOF_SYNTACTIC_IMPLIES_ADD
         || k == Kind::PB_PROOF_REDUNDANCY || k == Kind::PB_PROOF_LOAD_AXIOM;
}

void PbProofManager::convertProof(
    const std::vector<Node>& veriPbProof,
    const std::vector<std::pair<Node, Node>>& inputConstraints)
{
  PbProofTranslator translator(d_env, d_cdp);
  translator.setVariableIndices(d_varIndex);

  // Input PB constraints take VeriPB ids 1..N, in RoundingSat's addition
  // order: an EQUAL constraint appears twice consecutively, its second id
  // carrying the sign-flipped direction.
  for (size_t i = 0; i < inputConstraints.size(); ++i)
  {
    const Node& pbc = inputConstraints[i].first;
    const Node& fact = inputConstraints[i].second;
    bool negated = i > 0 && inputConstraints[i] == inputConstraints[i - 1];
    translator.registerInputConstraint(i + 1, pbc, fact, negated);
  }

  // Derived steps start at N+1 and only id-advancing kinds consume an id.
  size_t nextId = inputConstraints.size() + 1;
  for (const Node& step : veriPbProof)
  {
    if (step.getKind() == Kind::PB_PROOF_LOAD_FORMULA) continue;
    bool advances = advancesVeriPbId(step.getKind());
    translator.translateStep(step, advances ? nextId : 0);
    if (advances) ++nextId;
  }
}

void PbProofManager::addPbProof(
    std::vector<std::string> proofLines,
    const std::vector<std::pair<Node, Node>>& inputConstraints,
    std::unordered_map<std::string, Node> proofVariables)
{
  // A proof name "x<i>" denotes backend variable i; that index is the
  // canonical term order of the translator's constraints.
  d_varIndex.clear();
  for (const auto& [name, variable] : proofVariables)
  {
    Assert(name.size() > 1 && name[0] == 'x')
        << "unexpected proof variable name: " << name;
    d_varIndex[variable] = std::stoull(name.substr(1));
  }
  d_pbpr->setProofVariables(std::move(proofVariables));
  if (proofLines[0] != "pseudo-Boolean proof version 1.0"
      && proofLines[0] != "pseudo-Boolean proof version 2.0")
  {
    Unreachable() << "\nPbProofManager::addPbProof: cvc5 currently supports"
                  << " only pseudo-Boolean proof versions 1.0 and 2.0";
  }
  proofLines.erase(proofLines.begin());
  std::vector<Node> proofSteps = parseProofLines(proofLines);
  if (options().bv.bvPbProofGranularity == options::BvPbProofGranularity::FINE)
  {
    convertProof(proofSteps, inputConstraints);
    return;
  }
  d_cdp->addStep(
      nodeManager()->mkConst(false), ProofRule::VERIPB_PROOF, {}, proofSteps);
}

std::shared_ptr<ProofNode> PbProofManager::getProofFor(Node fact)
{
  Trace("bv-pb-proof-sfact") << "PbProofManager::getProofFor " << fact << "\n";
  NodeManager* nm = nodeManager();
  Assert(fact.getKind() == Kind::NOT);
  std::shared_ptr<ProofNode> pnFalse = d_cdp->getProof(nm->mkConst(false));
  Assert(pnFalse);
  std::vector<Node> assumptions;
  Node inner = fact[0];
  if (inner.getKind() == Kind::AND)
  {
    for (const Node& a : inner) assumptions.push_back(a);
  }
  else
  {
    assumptions.push_back(inner);
  }
  std::shared_ptr<ProofNode> scoped =
      d_cdp->getManager()->mkScope(pnFalse, assumptions, false);
  Assert(scoped);
  return scoped;
}

void debugPbProofLine(const std::string& line,
                      const std::vector<Node>& steps,
                      bool foo = true)
{
  if (foo)
  {
    Trace("bv-pb-proof") << "Proof step: " << line << "\n";
    Trace("bv-pb-proof") << "Result: " << steps.back() << "\n";
    return;
  }
  if (line[0] == 'c')
  {
    Trace("bv-pb-proof-c") << "Proof step: " << line << "\n";
    Trace("bv-pb-proof-c") << "Result: " << steps.back() << "\n";
  }
  else if (line[0] == 'l')
  {
    Trace("bv-pb-proof-l") << "Proof step: " << line << "\n";
    Trace("bv-pb-proof-l") << "Result: " << steps.back() << "\n";
  }
  else if (line[0] == 'p')
  {
    Trace("bv-pb-proof-p") << "Proof step: " << line << "\n";
    Trace("bv-pb-proof-p") << "Result: " << steps.back() << "\n";
  }
  else if (line[0] == 'u')
  {
    Trace("bv-pb-proof-u") << "Proof step: " << line << "\n";
    Trace("bv-pb-proof-u") << "Result: " << steps.back() << "\n";
  }
}

// True if 'line' ends with the token "begin" (preceded by whitespace or
// nothing). VeriPB uses this to open a subproof block (e.g. inside 'red').
bool opensSubproofBlock(const std::string& line)
{
  size_t e = line.find_last_not_of(" \t\r");
  if (e == std::string::npos || e + 1 < 5) return false;
  if (line.compare(e - 4, 5, "begin") != 0) return false;
  return e == 4 || std::isspace(static_cast<unsigned char>(line[e - 5]));
}

// True if 'line' begins with the token "end" (so 'end', 'end -1', etc.).
// Closes a subproof block opened by a prior 'begin'.
bool closesSubproofBlock(const std::string& line)
{
  std::istringstream iss(line);
  std::string tok;
  iss >> tok;
  return tok == "end";
}

std::vector<Node> PbProofManager::parseProofLines(
    std::vector<std::string> proofLines)
{
  std::vector<Node> cutting_plane_steps;
  for (size_t i = 0; i < proofLines.size(); ++i)
  {
    std::string line = proofLines[i];
    // Glue multi-line subproof blocks (e.g. 'red ... ; begin\n ... \nend')
    // into a single line so the per-line parseLine dispatch still works.
    // 'begin' increments depth, any 'end' (including 'end -1') decrements it.
    if (opensSubproofBlock(line))
    {
      int depth = 1;
      while (depth > 0 && ++i < proofLines.size())
      {
        line += "\n" + proofLines[i];
        if (opensSubproofBlock(proofLines[i])) ++depth;
        if (closesSubproofBlock(proofLines[i])) --depth;
      }
    }
    cutting_plane_steps.push_back(d_pbpr->parseLine(line));
    debugPbProofLine(line, cutting_plane_steps);
  }
  return cutting_plane_steps;
}

}  // namespace pb
}  // namespace bv
}  // namespace theory
}  // namespace cvc5::internal
