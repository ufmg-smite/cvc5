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
 * PB-blasting solver. Supports RoundingSAT and Exact back-ends.
 */

#include "theory/bv/bv_solver_pb.h"

#include <unordered_map>
#include <vector>

#include "options/bv_options.h"
#include "proof/trust_node.h"
#include "theory/bv/pb/exact.h"
#include "theory/bv/pb/roundingsat.h"
#include "theory/bv/theory_bv.h"
#include "theory/theory_model.h"

namespace cvc5::internal {
namespace theory {
namespace bv {
namespace pb {

BVSolverPseudoBoolean::BVSolverPseudoBoolean(Env& env,
                                             TheoryState* s,
                                             TheoryInferenceManager& inferMgr)
    : BVSolver(env, *s, inferMgr),
      d_pbBlaster(new PseudoBooleanBlaster(env, s)),
      d_facts(context()),
      d_assumptions(context()),
      d_isProofProducing(env.isTheoryProofProducing()),
      d_pbbpg(nullptr),
      d_pbpm(nullptr)
{
  initPbSolver();
  if (d_isProofProducing)
  {
    d_pbbpg = new PbBlastProofGenerator(env);
    d_pbpm = new PbProofManager(env, d_pbbpg);
  }
}

/** TODO(alanctprado): Used in BVSolverBitblast. Not sure we need it. */
bool BVSolverPseudoBoolean::needsEqualityEngine(CVC5_UNUSED EeSetupInfo& esi)
{
  // Same as BVSolverBitblast::needsEqualityEngine
  return logicInfo().isSharingEnabled() || options().bv.bvEqEngine;
}

void BVSolverPseudoBoolean::postCheck(Theory::Effort level)
{
  Trace("bv-pb") << "Post check with effort level " << level << "\n";
  if (level != Theory::Effort::EFFORT_FULL) return;  // TODO(alanctprado): why?

  std::vector<Node> blasted_atoms;
  while (!d_facts.empty())
  {
    Node fact = d_facts.front();
    d_facts.pop();
    if (fact.getKind() == Kind::BITVECTOR_EAGER_ATOM) Unhandled();
    d_pbBlaster->blastAtom(fact);
    d_assumptions.push_back(fact);
    blasted_atoms.push_back(fact);
  }

  d_pbSolver->reset();
  Trace("bv-postcheck") << "Post Check\n";

  // When the backend supports unsat cores we guard each fact's constraints with
  // a fresh selector variable, assume all selectors true, and read back the
  // core to build a minimized conflict. Otherwise we add the constraints
  // unconditionally and conflict over all blasted atoms.
  const bool useCores = d_pbSolver->supportsCores();
  std::vector<Node> selectors;
  std::vector<Node> allFacts;
  std::unordered_map<Node, Node> selectorToFact;
  // Pair each input PB constraint with its BV fact, matching RoundingSat's
  // id stream: dedup by Node (RoundingSat does), and push EQUAL twice (split
  // into >= and <=).
  std::vector<std::pair<Node, Node>> inputConstraints;
  std::unordered_set<Node> recordedConstraints;
  auto recordInput = [&](const Node& c, const Node& fact) {
    if (!recordedConstraints.insert(c).second) return;
    inputConstraints.push_back({c, fact});
    if (c.getKind() == Kind::EQUAL) inputConstraints.push_back({c, fact});
  };
  for (const Node& fact : d_assumptions)
  {
    Trace("bv-postcheck") << fact << "\n";
    allFacts.push_back(fact);
    if (useCores)
    {
      Node selector = d_pbBlaster->newVariable(1)[0];
      selectors.push_back(selector);
      selectorToFact.emplace(selector, fact);
      for (const Node& constraint : d_pbBlaster->getAtom(fact))
      {
        d_pbSolver->addConstraint(constraint, selector);
        recordInput(constraint, fact);
      }
    }
    else
    {
      for (const Node& constraint : d_pbBlaster->getAtom(fact))
      {
        d_pbSolver->addConstraint(constraint);
        recordInput(constraint, fact);
      }
    }
  }

  PbSolveState s =
      useCores ? d_pbSolver->solve(selectors) : d_pbSolver->solve();

  if (s == PbSolveState::PB_UNSAT)
  {
    if (d_isProofProducing)
    {
      std::vector<std::string> proof = d_pbSolver->getProof();
      d_pbpm->addPbProof(proof, inputConstraints);
    }
    NodeManager* nm = nodeManager();
    std::vector<Node> conflictFacts;
    if (useCores)
    {
      for (const Node& selector : d_pbSolver->getUnsatCore())
      {
        conflictFacts.push_back(selectorToFact.at(selector));
      }
      // A guarded encoding is always satisfiable with the selectors off, so an
      // unsat result must implicate at least one selector; fall back to all
      // facts only defensively.
      Trace("bv-pb") << "UNSAT core: " << conflictFacts.size() << " of "
                     << allFacts.size() << " facts\n";
      if (conflictFacts.empty()) conflictFacts = allFacts;
    }
    else
    {
      conflictFacts = blasted_atoms;
    }
    Node conflict = nm->mkAnd(conflictFacts);
    if (d_isProofProducing)
    {
      // Attach PbProofManager as the proof generator so the global proof DAG
      // sees the CUTTING_PLANES_REFUTATION step instead of a TRUST_THEORY_LEMMA.
      // Skip conflictExp() because the BV-PB backend has no proof equality
      // engine, which causes conflictExp to drop the PG silently.
      TrustNode tconf = TrustNode::mkTrustConflict(conflict, d_pbpm);
      d_im.trustedConflict(tconf, InferenceId::BV_PB_BLAST_CONFLICT);
    }
    else
    {
      d_im.conflict(conflict, InferenceId::BV_PB_BLAST_CONFLICT);
    }
  }
  else if (s == PbSolveState::PB_SAT)
  {
    Trace("bv-pb") << "SATISFIABLE\n";
    // for (const auto& atom : blasted_atoms) debugSatisfiedAtom(atom);
  }
  else
    Unreachable();
  Trace("bv-pb") << "\n";
}

bool BVSolverPseudoBoolean::preNotifyFact(CVC5_UNUSED TNode atom,
                                          CVC5_UNUSED bool pol,
                                          TNode fact,
                                          CVC5_UNUSED bool isPrereg,
                                          CVC5_UNUSED bool isInternal)
{
  Trace("bv-pb") << "BVSolverPseudoBoolean::preNotifyFact: " << fact << "\n";
  /**
   * Check whether `fact` is an input assertion on user-level 0.
   *
   * TODO(alanctprado):
   * Not really sure if this should be handled. I'm confident we concluded it
   * won't happen. We don't care about other theories?
   */
  Valuation& val = d_state.getValuation();
  if (options().bv.bvAssertInput && val.isFixed(fact)) Unhandled();

  d_facts.push_back(fact);
  /**
   * TODO(alanctprado):
   * Return false to enable equality engine reasoning in Theory, which is
   * available if we are using the equality engine. I don't think it is our
   * case. I wish there was some better documentation
   */
  return true;
}

/** TODO(alanctprado): Used in BVSolverBitblast. Not sure we need it. */
TrustNode BVSolverPseudoBoolean::explain(TNode n)
{
  // Same as BVSolverBitblast::explain
  Trace("bv-pb") << "explain called on " << n << std::endl;
  return d_im.explainLit(n);
}

/** TODO(alanctprado): Used in BVSolverBitblast. Not sure we need it. */
void BVSolverPseudoBoolean::computeRelevantTerms(
    CVC5_UNUSED std::set<Node>& termSet)
{
  // if (options().bv.bitblastMode == options::BitblastMode::EAGER)
  // {
  //   d_bitblaster->computeRelevantTerms(termSet);
  // }
  Unimplemented();
}

bool BVSolverPseudoBoolean::collectModelValues(TheoryModel* m,
                                               const std::set<Node>& termSet)
{
  for (const auto& term : termSet)
  {
    if (!d_pbBlaster->hasTerm(term)) continue;

    Node variables = d_pbBlaster->getTerm(term);
    Node const_value;  // = d_pbSolver->modelValue(variables);
    Assert(const_value.isNull() || const_value.isConst());
    if (!const_value.isNull())
    {
      if (!m->assertEquality(term, const_value, true))
      {
        return false;
      }
    }
  }
  return true;
}

Node BVSolverPseudoBoolean::getValue(TNode node, bool initialize)
{
  if (node.isConst())
  {
    return node;
  }

  if (!d_pbBlaster->hasTerm(node))
  {
    return initialize ? utils::mkConst(nodeManager(), utils::getSize(node), 0u)
                      : Node();
  }

  // std::vector<Node> bits;
  // d_bitblaster->getBBTerm(node, bits);
  // Integer value(0), one(1), zero(0), bit;
  // for (size_t i = 0, size = bits.size(), j = size - 1; i < size; ++i, --j)
  // {
  //   if (d_cnfStream->hasLiteral(bits[j]))
  //   {
  //     prop::SatLiteral lit = d_cnfStream->getLiteral(bits[j]);
  //     prop::SatValue val = d_satSolver->modelValue(lit);
  //     bit = val == prop::SatValue::SAT_VALUE_TRUE ? one : zero;
  //   }
  //   else
  //   {
  //     if (!initialize) return Node();
  //     bit = zero;
  //   }
  //   value = value * 2 + bit;
  // }
  // return utils::mkConst(bits.size(), value);
  Unimplemented();
}

void BVSolverPseudoBoolean::initPbSolver()
{
  // TODO(alanctprado): move guards / creation to a factory class
  switch (options().bv.bvPbSolver)
  {
    case options::BvPbSolver::EXACT:
      Trace("bv-pb") << "Initializing Exact PB Solver...\n";
#ifdef CVC5_USE_EXACT
      d_pbSolver.reset(new ExactSolver(
          d_env, statisticsRegistry(), "theory::bv::BVSolverPseudoBoolean::"));
      Trace("bv-pb") << "Initialization successful.\n";
#endif
      break;
    case options::BvPbSolver::ROUNDINGSAT:
      Trace("bv-pb") << "Initializing RoundingSat PB Solver...\n";
#ifdef CVC5_USE_ROUNDINGSAT
      d_pbSolver.reset(
          new RoundingSatSolver(d_env,
                                statisticsRegistry(),
                                "theory::bv::BVSolverPseudoBoolean::",
                                d_isProofProducing));
      Trace("bv-pb") << "Initialization successful.\n";
#endif
      break;
    default: Unimplemented();
  }
}

std::string BVSolverPseudoBoolean::getTermVariables(TNode term)
{
  std::string result = "[ ";
  TNode variables = d_pbBlaster->getTerm(term)[0];
  for (int i = variables.getNumChildren() - 1; i >= 0; i--)
  {
    result += variables[i].toString();
    result += " ";
  }
  result += "]\n[ ";
  for (int i = variables.getNumChildren() - 1; i >= 0; i--)
  {
    VariableId variable_id = variables[i].toString();
    PbValue value = d_pbSolver->modelValue(variable_id);
    result += (value == PB_FALSE) ? "0" : "1";
  }
  result += " ]";
  return result;
}

void BVSolverPseudoBoolean::debugSatisfiedAtom(TNode atom)
{
  Node lhs, rhs;
  if (atom.getKind() == Kind::NOT)
  {
    lhs = atom[0][0];
    rhs = atom[0][1];
  }
  else
  {
    lhs = atom[0];
    rhs = atom[1];
  }
  Trace("bv-pb-debug") << "\nDebugging atom " << atom << "\n";
  Trace("bv-pb-debug") << "KIND: " << atom.getKind() << "\n";
  Trace("bv-pb-debug") << "LHS: " << lhs << "\n";
  Trace("bv-pb-debug") << "LHS VARS: " << getTermVariables(lhs) << "\n";
  Trace("bv-pb-debug") << "RHS VARS: " << rhs << "\n";
  Trace("bv-pb-debug") << "RHS VARS: " << getTermVariables(rhs) << "\n";
  debugSatisfiedTerm(lhs);
  debugSatisfiedTerm(rhs);
  Trace("bv-pb-debug") << "\n";
}

void BVSolverPseudoBoolean::debugSatisfiedTerm(TNode term)
{
  Trace("bv-pb-debug") << "\n" << term << "\n";
  Trace("bv-pb-debug") << "KIND: " << term.getKind() << "\n";
  Trace("bv-pb-debug") << "RESULT VARS: " << getTermVariables(term) << "\n";
  for (unsigned i = 0; i < term.getNumChildren(); i++)
  {
    Node child = term[i];
    Trace("bv-pb-debug") << "CHILD: " << child << "\n";
    Trace("bv-pb-debug") << "CHILD VARS: " << getTermVariables(child) << "\n";
  }

  for (unsigned i = 0; i < term.getNumChildren(); i++)
  {
    Node child = term[i];
    debugSatisfiedTerm(child);
  }
}

}  // namespace pb
}  // namespace bv
}  // namespace theory
}  // namespace cvc5::internal
