/******************************************************************************
 * This file is part of the cvc5 project.
 *
 * Copyright (c) 2009-2026 by the authors listed in the file AUTHORS
 * in the top-level source directory and their institutional affiliations.
 * All rights reserved.  See the file COPYING in the top-level source
 * directory for licensing information.
 * ****************************************************************************
 *
 * The solver for interpolation queries.
 */

#include "smt/proof_interpolation.h"

#include <iostream>
#include <vector>

#include "expr/node_algorithm.h"
#include "base/output.h"
#include "proof/proof_generator.h"


namespace cvc5::internal {
namespace smt {

static bool isLocalA(Node pivot,
                     const std::unordered_set<Node>& symbolsA,
                     const std::unordered_set<Node>& symbolsB)
{
  std::unordered_set<Node> pivotSymbols;
  expr::getSymbols(pivot, pivotSymbols);

  if (pivotSymbols.empty())
  {
    return false;
  }

  for (const Node& s : pivotSymbols)
  {
    if (symbolsB.find(s) == symbolsB.end())
    {
      return true;
    }
  }

  return false;
}

static bool isSharedLiteral(Node literal, std::unordered_set<Node>& bSymbols)
{
  std::unordered_set<Node> litSymbols;
  expr::getSymbols(literal, litSymbols);

  if (litSymbols.empty()) return false;

  for (const Node& n : litSymbols)
  {
    if (bSymbols.find(n) == bSymbols.end()) return false;
  }
  return true;
}

static Node shared(Node clause,
                   std::unordered_set<Node>& bSymbols,
                   NodeManager* nm)
{
  std::vector<Node> sharedVar;

  if (clause.getKind() == Kind::OR)
  {
    for (const Node& literal : clause)
    {
      if (isSharedLiteral(literal, bSymbols)) sharedVar.push_back(literal);
    }
  }
  else
  {
    if (isSharedLiteral(clause, bSymbols)) sharedVar.push_back(clause);
  }

  return nm->mkOr(sharedVar);
}

void partition(const std::vector<Node>& aTerms,
               const std::vector<Node>& assertions,
               std::unordered_set<Node>& aAssertions,
               std::unordered_set<Node>& bAssertions,
               std::unordered_set<Node>& aSymbols,
               std::unordered_set<Node>& bSymbols)
{

  std::unordered_set<Node> aSet(aTerms.begin(), aTerms.end());

  //divide assertions between A and B
  for (const Node& n : assertions)
  {
    bool inA = aSet.find(n) != aSet.end();
  
    std::unordered_set<Node>& target = inA ? aAssertions : bAssertions;

    target.insert(n);

    if (n.getKind() == Kind::AND)
    {
      for (const Node& child : n)
      {
        target.insert(child);
      }
    }
  }

  for (const Node& a : aAssertions)
  {
    expr::getSymbols(a, aSymbols);
  }
  for (const Node& b : bAssertions)
  {
    expr::getSymbols(b, bSymbols);
  }
}

//Workaround - temporario
static ProofNode* findRegisteredLeaf(ProofNode* p,
                                     const Env::LeafGenMap& leafGen)
{
  if (p->getRule() == ProofRule::ASSUME)
  {
    return nullptr;
  }

  if (leafGen.find(p->getResult()) != leafGen.end())
  {
    return p;
  }

  for (const std::shared_ptr<ProofNode>& c : p->getChildren())
  {
    ProofNode* found = findRegisteredLeaf(c.get(), leafGen);
    if (found != nullptr)
    {
      return found;
    }
  }
  return nullptr;
}

Node getItp(std::shared_ptr<ProofNode> p,
            std::unordered_set<Node>& aAssertions,
            std::unordered_set<Node>& bAssertions,
            std::unordered_set<Node>& aSymbols,
            std::unordered_set<Node>& bSymbols,
            NodeManager* nm,
            std::unordered_map<ProofNode*, Node>& cache,
            const Env::LeafGenMap& leafGen)
{
  auto it = cache.find(p.get());
  if (it != cache.end())
  {
    return it->second;
  }

  Node result;

  switch (p->getRule())
  {
    case ProofRule::SCOPE:
      result = getItp(p->getChildren()[0], aAssertions, bAssertions, aSymbols, bSymbols, nm, cache, leafGen);
      break;
    case ProofRule::ASSUME:
    {
      Node f = p->getResult();
      if (aAssertions.find(f) != aAssertions.end())
      {
        result = shared(f, bSymbols, nm);
      }
      else if (bAssertions.find(f) != bAssertions.end())
      {
        result = nm->mkConst(true);
      }
      break;
    }
    case ProofRule::CONTRA:
    {
      const auto& children = p->getChildren();

      Node itp0 = getItp(
          children[0], aAssertions, bAssertions, aSymbols, bSymbols, nm, cache, leafGen);

      Node itp1 = getItp(
          children[1], aAssertions, bAssertions, aSymbols, bSymbols, nm, cache, leafGen);

      Node pivot = children[0]->getResult();

      bool pivotLocalA = isLocalA(pivot, aSymbols, bSymbols);

      if (pivotLocalA)
      {
        result = nm->mkNode(Kind::OR, itp0, itp1);
      }
      else
      {
        result = nm->mkNode(Kind::AND, itp0, itp1);
      }
      break;
    }
    case ProofRule::CHAIN_RESOLUTION:
    case ProofRule::CHAIN_M_RESOLUTION:
    {
      const auto& children = p->getChildren();
      const auto& args = p->getArguments();

      Node itp0 = getItp(
          children[0], aAssertions, bAssertions, aSymbols, bSymbols, nm, cache, leafGen);

      for (size_t i = 1; i < children.size(); i++)
      {
        Node child_itp = getItp(children[i],
                                aAssertions,
                                bAssertions,
                                aSymbols,
                                bSymbols,
                                nm,
                                cache, leafGen);
        Node pivot;

        if (p->getRule() == ProofRule::CHAIN_RESOLUTION)
        {
          pivot = args[1][i - 1];
        }
        else if (p->getRule() == ProofRule::CHAIN_M_RESOLUTION)
        {
          pivot = args[2][i - 1];
        }

        bool pivotLocalA = isLocalA(pivot, aSymbols, bSymbols);

        if (pivotLocalA)
        {
          itp0 = nm->mkNode(Kind::OR, itp0, child_itp);
        }
        else
        {
          itp0 = nm->mkNode(Kind::AND, itp0, child_itp);
        }
      }
      result = itp0;
      break;
    }
    case ProofRule::REORDERING:
    case ProofRule::FACTORING:
    {
      result = getItp(p->getChildren()[0],
                      aAssertions,
                      bAssertions,
                      aSymbols,
                      bSymbols,
                      nm,
                      cache,
                      leafGen);
      break;
    }
    default:
    {
      ProofNode* leaf = findRegisteredLeaf(p.get(), leafGen);
      if (leaf != nullptr)
      {
        //Found a theory lemma:
        Node conc = leaf->getResult();
        ProofGenerator* gen = leafGen.find(conc)->second;
        result = gen->getPartialInterpolant(conc, aSymbols, bSymbols, nm);
        Trace("itp") << "getItp: rule: " << p->getRule()
                          << " - lemma: " << conc << " -> " << result
                          << std::endl;
      }
      else
      {
        //Not implemented yet :(
        Trace("itp") << "regra sem registro: "
                          << p->getRule() << " conclusao: " << p->getResult()
                          << std::endl;
        result = nm->mkConst(true);
      }
      break;
    }

  }

  cache[p.get()] = result;
  return result;
}

}  // namespace smt
}  // namespace cvc5::internal