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

#include "proof/proof.h"
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

void PbProofManager::convertProof(std::vector<Node> veriPbProof){
  for (const auto& child : veriPbProof)
  {
    
    
  }
 Unimplemented();
}
void PbProofManager::addPbProof(std::vector<std::string> proofLines, bool veriPbFormat)
{
  NodeManager* nm = nodeManager();

  if (proofLines[0] == "pseudo-Boolean proof version 1.0"
      || proofLines[0] == "pseudo-Boolean proof version 2.0")
  {
    proofLines.erase(proofLines.begin());
    std::vector<Node> proof_steps = parseProofLines(proofLines);
  }
  else
  {
    Unreachable() << "\nPbProofManager::addPbProof: cvc5 currently supports"
                  << " only pseudo-Boolean proof versions 1.0 and 2.0";
  }

  Node expected = nm->mkConst(false);
  std::vector<Node> children;

  // d_proof->addStep(expected,
  //                  ProofRule::CUTTING_PLANES_REFUTATION,
  //                  children,
  //                  cutting_plane_steps);

  // The step above generates the following error:
  //
  // Fatal failure within cvc5::internal::ProofNodeManager*
  // cvc5::internal::CDProof::getManager() const at
  // /home/alan/logic/cvc5/src/proof/proof.cpp:454 Check failure

  // pnm != nullptr
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
static bool opensSubproofBlock(const std::string& line)
{
  size_t e = line.find_last_not_of(" \t\r");
  if (e == std::string::npos || e + 1 < 5) return false;
  if (line.compare(e - 4, 5, "begin") != 0) return false;
  return e == 4
         || std::isspace(static_cast<unsigned char>(line[e - 5]));
}

// True if 'line' begins with the token "end" (so 'end', 'end -1', etc.).
// Closes a subproof block opened by a prior 'begin'.
static bool closesSubproofBlock(const std::string& line)
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
