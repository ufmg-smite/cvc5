/******************************************************************************
 * Top contributors (to current version):
 *   Pedro Saccomani
 *
 * This file is part of the cvc5 project.
 *
 * Copyright (c) 2009-2026 by the authors listed in the file AUTHORS
 * in the top-level source directory and their institutional affiliations.
 * All rights reserved.  See the file COPYING in the top-level source
 * directory for licensing information.
 * ****************************************************************************
 *
 * A CNF converter that mirrors prop::CnfStream, but stores the generated
 * clauses instead of streaming them to a SAT solver.
 */
#include "theory/bv/pb/clause_cnf_stream.h"

#include <unordered_set>

#include "base/check.h"
#include "base/output.h"
#include "expr/node.h"
#include "smt/env.h"
#include "theory/theory.h"
#include "util/resource_manager.h"

namespace cvc5::internal {
namespace theory {
namespace bv {
namespace pb {

using prop::SatClause;
using prop::SatLiteral;
using prop::SatVariable;
using prop::undefSatVariable;

ClauseCnfStream::ClauseCnfStream(Env& env,
                                 context::Context* c,
                                 FormulaLitPolicy flpol,
                                 std::string name)
    : EnvObj(env),
      d_nextVar(0),
      d_trueVar(undefSatVariable),
      d_falseVar(undefSatVariable),
      d_booleanVariables(c),
      d_notifyFormulas(c),
      d_nodeToLiteralMap(c),
      d_literalToNodeMap(c),
      d_flitPolicy(flpol),
      d_name(name),
      d_removable(false),
      d_stats(statisticsRegistry(), name)
{
}

const std::vector<SatClause>& ClauseCnfStream::getClauses() const
{
  return d_clauses;
}

std::size_t ClauseCnfStream::getNumClauses() const { return d_clauses.size(); }

void ClauseCnfStream::clearClauses() { d_clauses.clear(); }

SatVariable ClauseCnfStream::newVar() { return d_nextVar++; }

SatVariable ClauseCnfStream::trueVar()
{
  if (d_trueVar == undefSatVariable)
  {
    d_trueVar = newVar();
    // Pin the variable to true so that the stored clause set stays sound.
    SatClause unit{SatLiteral(d_trueVar)};
    d_clauses.push_back(unit);
  }
  return d_trueVar;
}

SatVariable ClauseCnfStream::falseVar()
{
  if (d_falseVar == undefSatVariable)
  {
    d_falseVar = newVar();
    // Pin the variable to false so that the stored clause set stays sound.
    SatClause unit{~SatLiteral(d_falseVar)};
    d_clauses.push_back(unit);
  }
  return d_falseVar;
}

bool ClauseCnfStream::assertClause(TNode node, SatClause& c)
{
  Trace("cnf") << "Storing clause " << c << " node = " << node << "\n";

  // Filter out duplicate literals, matching prop::CnfStream. This keeps the
  // stored clauses in the same normalized form that the SAT solver would see.
  std::unordered_set<uint64_t> cache;
  SatClause cl;
  for (const auto& lit : c)
  {
    if (cache.insert(lit.toInt()).second)
    {
      cl.push_back(lit);
    }
  }

  d_clauses.push_back(cl);
  return true;
}

bool ClauseCnfStream::assertClause(TNode node, SatLiteral a)
{
  SatClause clause(1);
  clause[0] = a;
  return assertClause(node, clause);
}

bool ClauseCnfStream::assertClause(TNode node, SatLiteral a, SatLiteral b)
{
  SatClause clause(2);
  clause[0] = a;
  clause[1] = b;
  return assertClause(node, clause);
}

bool ClauseCnfStream::assertClause(TNode node,
                                   SatLiteral a,
                                   SatLiteral b,
                                   SatLiteral c)
{
  SatClause clause(3);
  clause[0] = a;
  clause[1] = b;
  clause[2] = c;
  return assertClause(node, clause);
}

bool ClauseCnfStream::hasLiteral(TNode n) const
{
  NodeToLiteralMap::const_iterator find = d_nodeToLiteralMap.find(n);
  return find != d_nodeToLiteralMap.end();
}

void ClauseCnfStream::ensureMappingForLiteral(TNode n)
{
  SatLiteral lit = getLiteral(n);
  if (!d_literalToNodeMap.contains(lit))
  {
    // Store backward-mappings
    d_literalToNodeMap.insert(lit, n);
    d_literalToNodeMap.insert(~lit, n.notNode());
  }
}

void ClauseCnfStream::ensureLiteral(TNode n)
{
  AlwaysAssertArgument(
      hasLiteral(n) || n.getType().isBoolean(),
      n,
      "ClauseCnfStream::ensureLiteral() requires a node of Boolean type.\n"
      "got node: %s\n"
      "its type: %s\n",
      n.toString().c_str(),
      n.getType().toString().c_str());
  Trace("cnf") << "ensureLiteral(" << n << ")\n";
  TimerStat::CodeTimer codeTimer(d_stats.d_cnfConversionTime, true);
  if (hasLiteral(n))
  {
    ensureMappingForLiteral(n);
    return;
  }
  // remove top level negation
  n = n.getKind() == Kind::NOT ? n[0] : n;
  if (d_env.theoryOf(n) == theory::THEORY_BOOL && !n.isVar())
  {
    // If we were called with something other than a theory atom (or Boolean
    // variable), we get a SatLiteral that is definitionally equal to it. These
    // are not removable.
    d_removable = false;

    SatLiteral lit = toCNF(n, false);

    // Store backward-mappings. These may already exist.
    d_literalToNodeMap.insert_safe(lit, n);
    d_literalToNodeMap.insert_safe(~lit, n.notNode());
  }
  else
  {
    // We have a theory atom or variable.
    convertAtom(n);
  }
}

SatLiteral ClauseCnfStream::newLiteral(TNode node, bool isTheoryAtom)
{
  Trace("cnf") << d_name << "::newLiteral(" << node << ", " << isTheoryAtom
               << ")\n"
               << push;
  Assert(node.getKind() != Kind::NOT);

  // Get the literal for this node
  SatLiteral lit;
  if (!hasLiteral(node))
  {
    Trace("cnf") << d_name << "::newLiteral: node not registered\n";
    // If no literal, we'll make one
    if (node.getKind() == Kind::CONST_BOOLEAN)
    {
      Trace("cnf") << d_name << "::newLiteral: boolean const\n";
      if (node.getConst<bool>())
      {
        lit = SatLiteral(trueVar());
      }
      else
      {
        lit = SatLiteral(falseVar());
      }
    }
    else
    {
      Trace("cnf") << d_name << "::newLiteral: new var\n";
      lit = SatLiteral(newVar());
      d_stats.d_numAtoms++;
    }
    d_nodeToLiteralMap.insert(node, lit);
    d_nodeToLiteralMap.insert(node.notNode(), ~lit);
  }
  else
  {
    Trace("cnf") << d_name << "::newLiteral: node already registered\n";
    lit = getLiteral(node);
  }

  // If it's a theory literal, need to store it for back queries
  if (isTheoryAtom || d_flitPolicy == FormulaLitPolicy::TRACK
      || d_flitPolicy == FormulaLitPolicy::TRACK_AND_NOTIFY_VAR)
  {
    d_literalToNodeMap.insert_safe(lit, node);
    d_literalToNodeMap.insert_safe(~lit, node.notNode());
  }

  Trace("cnf") << "newLiteral(" << node << ") => " << lit << "\n" << pop;
  return lit;
}

TNode ClauseCnfStream::getNode(const SatLiteral& literal)
{
  Assert(d_literalToNodeMap.find(literal) != d_literalToNodeMap.end());
  Trace("cnf") << "getNode(" << literal << ")\n";
  Trace("cnf") << "getNode(" << literal << ") => "
               << d_literalToNodeMap[literal] << "\n";
  return d_literalToNodeMap[literal];
}

const ClauseCnfStream::NodeToLiteralMap&
ClauseCnfStream::getTranslationCache() const
{
  return d_nodeToLiteralMap;
}

const ClauseCnfStream::LiteralToNodeMap& ClauseCnfStream::getNodeCache() const
{
  return d_literalToNodeMap;
}

void ClauseCnfStream::getBooleanVariables(
    std::vector<TNode>& outputVariables) const
{
  outputVariables.insert(outputVariables.end(),
                         d_booleanVariables.begin(),
                         d_booleanVariables.end());
}

bool ClauseCnfStream::isNotifyFormula(TNode node) const
{
  return d_notifyFormulas.find(node) != d_notifyFormulas.end();
}

SatLiteral ClauseCnfStream::convertAtom(TNode node)
{
  Trace("cnf") << "convertAtom(" << node << ")\n";

  Assert(!hasLiteral(node)) << "atom already mapped!";

  bool theoryLiteral = false;

  // Is this a variable add it to the list. We distinguish whether a Boolean
  // variable has been marked as a "Boolean term skolem". These variables are
  // introduced by the term formula removal pass (term_formula_removal.h) and
  // maintained by Env (smt/env.h). We treat such variables as theory atoms
  // since they may occur in term positions.
  bool isInternalBoolVar = false;
  if (node.isVar())
  {
    isInternalBoolVar = !d_env.isBooleanTermSkolem(node);
  }
  if (isInternalBoolVar)
  {
    d_booleanVariables.push_back(node);
    // if TRACK_AND_NOTIFY_VAR, we track Boolean variables as theory literals.
    if (d_flitPolicy == FormulaLitPolicy::TRACK_AND_NOTIFY_VAR)
    {
      theoryLiteral = true;
    }
  }
  else
  {
    theoryLiteral = true;
  }

  // Make a new literal (variables are not considered theory literals)
  SatLiteral lit = newLiteral(node, theoryLiteral);
  // Return the resulting literal
  return lit;
}

SatLiteral ClauseCnfStream::getLiteral(TNode node)
{
  Assert(!node.isNull()) << "ClauseCnfStream: can't getLiteral() of null node";

  Assert(d_nodeToLiteralMap.contains(node))
      << "Literal not in the CNF Cache: " << node << "\n";

  SatLiteral literal = d_nodeToLiteralMap[node];
  Trace("cnf") << "ClauseCnfStream::getLiteral(" << node << ") => " << literal
               << "\n";
  return literal;
}

void ClauseCnfStream::handleXor(TNode xorNode)
{
  Assert(!hasLiteral(xorNode)) << "Atom already mapped!";
  Assert(xorNode.getKind() == Kind::XOR) << "Expecting an XOR expression!";
  Assert(xorNode.getNumChildren() == 2) << "Expecting exactly 2 children!";
  Assert(!d_removable) << "Removable clauses can not contain Boolean structure";
  Trace("cnf") << "ClauseCnfStream::handleXor(" << xorNode << ")\n";

  SatLiteral a = getLiteral(xorNode[0]);
  SatLiteral b = getLiteral(xorNode[1]);

  SatLiteral xorLit = newLiteral(xorNode);

  assertClause(xorNode.negate(), a, b, ~xorLit);
  assertClause(xorNode.negate(), ~a, ~b, ~xorLit);
  assertClause(xorNode, a, ~b, xorLit);
  assertClause(xorNode, ~a, b, xorLit);
}

void ClauseCnfStream::handleOr(TNode orNode)
{
  Assert(!hasLiteral(orNode)) << "Atom already mapped!";
  Assert(orNode.getKind() == Kind::OR) << "Expecting an OR expression!";
  Assert(orNode.getNumChildren() > 1) << "Expecting more then 1 child!";
  Assert(!d_removable) << "Removable clauses can not contain Boolean structure";
  Trace("cnf") << "ClauseCnfStream::handleOr(" << orNode << ")\n";

  // Number of children
  size_t numChildren = orNode.getNumChildren();

  // Get the literal for this node
  SatLiteral orLit = newLiteral(orNode);

  // Transform all the children first
  SatClause clause(numChildren + 1);
  for (size_t i = 0; i < numChildren; ++i)
  {
    clause[i] = getLiteral(orNode[i]);

    // lit <- (a_1 | a_2 | a_3 | ... | a_n)
    // lit | ~(a_1 | a_2 | a_3 | ... | a_n)
    // (lit | ~a_1) & (lit | ~a_2) & (lit & ~a_3) & ... & (lit & ~a_n)
    assertClause(orNode, orLit, ~clause[i]);
  }

  // lit -> (a_1 | a_2 | a_3 | ... | a_n)
  // ~lit | a_1 | a_2 | a_3 | ... | a_n
  clause[numChildren] = ~orLit;
  assertClause(orNode.negate(), clause);
}

void ClauseCnfStream::handleAnd(TNode andNode)
{
  Assert(!hasLiteral(andNode)) << "Atom already mapped!";
  Assert(andNode.getKind() == Kind::AND) << "Expecting an AND expression!";
  Assert(andNode.getNumChildren() > 1) << "Expecting more than 1 child!";
  Assert(!d_removable) << "Removable clauses can not contain Boolean structure";
  Trace("cnf") << "handleAnd(" << andNode << ")\n";

  // Number of children
  size_t numChildren = andNode.getNumChildren();

  // Get the literal for this node
  SatLiteral andLit = newLiteral(andNode);

  // Transform all the children first (remembering the negation)
  SatClause clause(numChildren + 1);
  for (size_t i = 0; i < numChildren; ++i)
  {
    clause[i] = ~getLiteral(andNode[i]);

    // lit -> (a_1 & a_2 & a_3 & ... & a_n)
    // ~lit | (a_1 & a_2 & a_3 & ... & a_n)
    // (~lit | a_1) & (~lit | a_2) & ... & (~lit | a_n)
    assertClause(andNode.negate(), ~andLit, ~clause[i]);
  }

  // lit <- (a_1 & a_2 & a_3 & ... a_n)
  // lit | ~(a_1 & a_2 & a_3 & ... & a_n)
  // lit | ~a_1 | ~a_2 | ~a_3 | ... | ~a_n
  clause[numChildren] = andLit;
  assertClause(andNode, clause);
}

void ClauseCnfStream::handleImplies(TNode impliesNode)
{
  Assert(!hasLiteral(impliesNode)) << "Atom already mapped!";
  Assert(impliesNode.getKind() == Kind::IMPLIES)
      << "Expecting an IMPLIES expression!";
  Assert(impliesNode.getNumChildren() == 2) << "Expecting exactly 2 children!";
  Assert(!d_removable) << "Removable clauses can not contain Boolean structure";
  Trace("cnf") << "handleImplies(" << impliesNode << ")\n";

  // Convert the children to cnf
  SatLiteral a = getLiteral(impliesNode[0]);
  SatLiteral b = getLiteral(impliesNode[1]);

  SatLiteral impliesLit = newLiteral(impliesNode);

  // lit -> (a->b)
  // ~lit | ~ a | b
  assertClause(impliesNode.negate(), ~impliesLit, ~a, b);

  // (a->b) -> lit
  // ~(~a | b) | lit
  // (a | l) & (~b | l)
  assertClause(impliesNode, a, impliesLit);
  assertClause(impliesNode, ~b, impliesLit);
}

void ClauseCnfStream::handleIff(TNode iffNode)
{
  Assert(!hasLiteral(iffNode)) << "Atom already mapped!";
  Assert(iffNode.getKind() == Kind::EQUAL) << "Expecting an EQUAL expression!";
  Assert(iffNode.getNumChildren() == 2) << "Expecting exactly 2 children!";
  Assert(!d_removable) << "Removable clauses can not contain Boolean structure";
  Trace("cnf") << "handleIff(" << iffNode << ")\n";

  // Convert the children to CNF
  SatLiteral a = getLiteral(iffNode[0]);
  SatLiteral b = getLiteral(iffNode[1]);

  // Get the now literal
  SatLiteral iffLit = newLiteral(iffNode);

  // lit -> ((a-> b) & (b->a))
  // ~lit | ((~a | b) & (~b | a))
  // (~a | b | ~lit) & (~b | a | ~lit)
  assertClause(iffNode.negate(), ~a, b, ~iffLit);
  assertClause(iffNode.negate(), a, ~b, ~iffLit);

  // (a<->b) -> lit
  // ~((a & b) | (~a & ~b)) | lit
  // (~(a & b)) & (~(~a & ~b)) | lit
  // ((~a | ~b) & (a | b)) | lit
  // (~a | ~b | lit) & (a | b | lit)
  assertClause(iffNode, ~a, ~b, iffLit);
  assertClause(iffNode, a, b, iffLit);
}

void ClauseCnfStream::handleIte(TNode iteNode)
{
  Assert(!hasLiteral(iteNode)) << "Atom already mapped!";
  Assert(iteNode.getKind() == Kind::ITE);
  Assert(iteNode.getNumChildren() == 3);
  Assert(!d_removable) << "Removable clauses can not contain Boolean structure";
  Trace("cnf") << "handleIte(" << iteNode[0] << " " << iteNode[1] << " "
               << iteNode[2] << ")\n";

  SatLiteral condLit = getLiteral(iteNode[0]);
  SatLiteral thenLit = getLiteral(iteNode[1]);
  SatLiteral elseLit = getLiteral(iteNode[2]);

  SatLiteral iteLit = newLiteral(iteNode);

  // If ITE is true then one of the branches is true and the condition
  // implies which one
  // lit -> (ite b t e)
  // lit -> (t | e) & (b -> t) & (!b -> e)
  // lit -> (t | e) & (!b | t) & (b | e)
  // (!lit | t | e) & (!lit | !b | t) & (!lit | b | e)
  assertClause(iteNode.negate(), ~iteLit, thenLit, elseLit);
  assertClause(iteNode.negate(), ~iteLit, ~condLit, thenLit);
  assertClause(iteNode.negate(), ~iteLit, condLit, elseLit);

  // If ITE is false then one of the branches is false and the condition
  // implies which one
  // !lit -> !(ite b t e)
  // !lit -> (!t | !e) & (b -> !t) & (!b -> !e)
  // !lit -> (!t | !e) & (!b | !t) & (b | !e)
  // (lit | !t | !e) & (lit | !b | !t) & (lit | b | !e)
  assertClause(iteNode, iteLit, ~thenLit, ~elseLit);
  assertClause(iteNode, iteLit, ~condLit, ~thenLit);
  assertClause(iteNode, iteLit, condLit, ~elseLit);
}

SatLiteral ClauseCnfStream::toCNF(TNode node, bool negated)
{
  Trace("cnf") << "toCNF(" << node
               << ", negated = " << (negated ? "true" : "false") << ")\n";

  TNode cur;
  SatLiteral nodeLit;
  std::vector<TNode> visit;
  std::unordered_map<TNode, bool> cache;

  visit.push_back(node);
  while (!visit.empty())
  {
    cur = visit.back();
    Assert(cur.getType().isBoolean());

    if (hasLiteral(cur))
    {
      visit.pop_back();
      continue;
    }

    const auto& it = cache.find(cur);
    if (it == cache.end())
    {
      cache.emplace(cur, false);
      Kind k = cur.getKind();
      // Only traverse Boolean nodes
      if (k == Kind::NOT || k == Kind::XOR || k == Kind::ITE
          || k == Kind::IMPLIES || k == Kind::OR || k == Kind::AND
          || (k == Kind::EQUAL && cur[0].getType().isBoolean()))
      {
        // Preserve the order of the recursive version
        for (size_t i = 0, size = cur.getNumChildren(); i < size; ++i)
        {
          visit.push_back(cur[size - 1 - i]);
        }
      }
      continue;
    }
    else if (!it->second)
    {
      it->second = true;
      Kind k = cur.getKind();
      switch (k)
      {
        case Kind::NOT: Assert(hasLiteral(cur[0])); break;
        case Kind::XOR: handleXor(cur); break;
        case Kind::ITE: handleIte(cur); break;
        case Kind::IMPLIES: handleImplies(cur); break;
        case Kind::OR: handleOr(cur); break;
        case Kind::AND: handleAnd(cur); break;
        default:
          if (k == Kind::EQUAL && cur[0].getType().isBoolean())
          {
            handleIff(cur);
          }
          else
          {
            convertAtom(cur);
          }
          break;
      }
    }
    visit.pop_back();
  }

  nodeLit = getLiteral(node);
  Trace("cnf") << "toCNF(): resulting literal: "
               << (!negated ? nodeLit : ~nodeLit) << "\n";
  return negated ? ~nodeLit : nodeLit;
}

void ClauseCnfStream::convertAndAssertAnd(TNode node, bool negated)
{
  Assert(node.getKind() == Kind::AND);
  Trace("cnf") << "ClauseCnfStream::convertAndAssertAnd(" << node
               << ", negated = " << (negated ? "true" : "false") << ")\n";
  if (!negated)
  {
    // If the node is a conjunction, we handle each conjunct separately
    for (TNode::const_iterator conjunct = node.begin(), node_end = node.end();
         conjunct != node_end;
         ++conjunct)
    {
      convertAndAssert(*conjunct, false);
    }
  }
  else
  {
    // If the node is a disjunction, we construct a clause and assert it
    int nChildren = node.getNumChildren();
    SatClause clause(nChildren);
    TNode::const_iterator disjunct = node.begin();
    for (int i = 0; i < nChildren; ++disjunct, ++i)
    {
      Assert(disjunct != node.end());
      clause[i] = toCNF(*disjunct, true);
    }
    Assert(disjunct == node.end());
    assertClause(node.negate(), clause);
  }
}

void ClauseCnfStream::convertAndAssertOr(TNode node, bool negated)
{
  Assert(node.getKind() == Kind::OR);
  Trace("cnf") << "ClauseCnfStream::convertAndAssertOr(" << node
               << ", negated = " << (negated ? "true" : "false") << ")\n";
  if (!negated)
  {
    // If the node is a disjunction, we construct a clause and assert it
    int nChildren = node.getNumChildren();
    SatClause clause(nChildren);
    TNode::const_iterator disjunct = node.begin();
    for (int i = 0; i < nChildren; ++disjunct, ++i)
    {
      Assert(disjunct != node.end());
      clause[i] = toCNF(*disjunct, false);
    }
    Assert(disjunct == node.end());
    assertClause(node, clause);
  }
  else
  {
    // If the node is a conjunction, we handle each conjunct separately
    for (TNode::const_iterator conjunct = node.begin(), node_end = node.end();
         conjunct != node_end;
         ++conjunct)
    {
      convertAndAssert(*conjunct, true);
    }
  }
}

void ClauseCnfStream::convertAndAssertXor(TNode node, bool negated)
{
  Assert(node.getKind() == Kind::XOR);
  Trace("cnf") << "ClauseCnfStream::convertAndAssertXor(" << node
               << ", negated = " << (negated ? "true" : "false") << ")\n";
  if (!negated)
  {
    // p XOR q
    SatLiteral p = toCNF(node[0], false);
    SatLiteral q = toCNF(node[1], false);
    // Construct the clauses (p => !q) and (!q => p)
    SatClause clause1(2);
    clause1[0] = ~p;
    clause1[1] = ~q;
    assertClause(node, clause1);
    SatClause clause2(2);
    clause2[0] = p;
    clause2[1] = q;
    assertClause(node, clause2);
  }
  else
  {
    // !(p XOR q) is the same as p <=> q
    SatLiteral p = toCNF(node[0], false);
    SatLiteral q = toCNF(node[1], false);
    // Construct the clauses (p => q) and (q => p)
    SatClause clause1(2);
    clause1[0] = ~p;
    clause1[1] = q;
    assertClause(node.negate(), clause1);
    SatClause clause2(2);
    clause2[0] = p;
    clause2[1] = ~q;
    assertClause(node.negate(), clause2);
  }
}

void ClauseCnfStream::convertAndAssertIff(TNode node, bool negated)
{
  Assert(node.getKind() == Kind::EQUAL);
  Trace("cnf") << "ClauseCnfStream::convertAndAssertIff(" << node
               << ", negated = " << (negated ? "true" : "false") << ")\n";
  if (!negated)
  {
    // p <=> q
    SatLiteral p = toCNF(node[0], false);
    SatLiteral q = toCNF(node[1], false);
    // Construct the clauses (p => q) and (q => p)
    SatClause clause1(2);
    clause1[0] = ~p;
    clause1[1] = q;
    assertClause(node, clause1);
    SatClause clause2(2);
    clause2[0] = p;
    clause2[1] = ~q;
    assertClause(node, clause2);
  }
  else
  {
    // !(p <=> q) is the same as p XOR q
    SatLiteral p = toCNF(node[0], false);
    SatLiteral q = toCNF(node[1], false);
    // Construct the clauses (p => !q) and (!q => p)
    SatClause clause1(2);
    clause1[0] = ~p;
    clause1[1] = ~q;
    assertClause(node.negate(), clause1);
    SatClause clause2(2);
    clause2[0] = p;
    clause2[1] = q;
    assertClause(node.negate(), clause2);
  }
}

void ClauseCnfStream::convertAndAssertImplies(TNode node, bool negated)
{
  Assert(node.getKind() == Kind::IMPLIES);
  Trace("cnf") << "ClauseCnfStream::convertAndAssertImplies(" << node
               << ", negated = " << (negated ? "true" : "false") << ")\n";
  if (!negated)
  {
    // p => q
    SatLiteral p = toCNF(node[0], false);
    SatLiteral q = toCNF(node[1], false);
    // Construct the clause ~p || q
    SatClause clause(2);
    clause[0] = ~p;
    clause[1] = q;
    assertClause(node, clause);
  }
  else
  {  // Construct the
    // !(p => q) is the same as (p && ~q)
    convertAndAssert(node[0], false);
    convertAndAssert(node[1], true);
  }
}

void ClauseCnfStream::convertAndAssertIte(TNode node, bool negated)
{
  Assert(node.getKind() == Kind::ITE);
  Trace("cnf") << "ClauseCnfStream::convertAndAssertIte(" << node
               << ", negated = " << (negated ? "true" : "false") << ")\n";
  // ITE(p, q, r)
  SatLiteral p = toCNF(node[0], false);
  SatLiteral q = toCNF(node[1], negated);
  SatLiteral r = toCNF(node[2], negated);
  // Construct the clauses:
  // (p => q) and (!p => r)
  //
  // Note that below q and r can be used directly because whether they are
  // negated has been push to the literal definitions above
  Node nnode = node;
  if (negated)
  {
    nnode = node.negate();
  }
  SatClause clause1(2);
  clause1[0] = ~p;
  clause1[1] = q;
  assertClause(nnode, clause1);
  SatClause clause2(2);
  clause2[0] = p;
  clause2[1] = r;
  assertClause(nnode, clause2);
}

// At the top level we must ensure that all clauses that are asserted are not
// unit, except for the direct assertions. This allows us to remove the clauses
// later when they are not needed anymore (lemmas for example).
void ClauseCnfStream::convertAndAssert(TNode node, bool removable, bool negated)
{
  Trace("cnf") << "convertAndAssert(" << node
               << ", negated = " << (negated ? "true" : "false")
               << ", removable = " << (removable ? "true" : "false") << ")\n";
  d_removable = removable;
  TimerStat::CodeTimer codeTimer(d_stats.d_cnfConversionTime, true);
  convertAndAssert(node, negated);
}

void ClauseCnfStream::convertAndAssert(TNode node, bool negated)
{
  Trace("cnf") << "convertAndAssert(" << node
               << ", negated = " << (negated ? "true" : "false") << ")\n";

  resourceManager()->spendResource(Resource::CnfStep);

  switch (node.getKind())
  {
    case Kind::AND: convertAndAssertAnd(node, negated); break;
    case Kind::OR: convertAndAssertOr(node, negated); break;
    case Kind::XOR: convertAndAssertXor(node, negated); break;
    case Kind::IMPLIES: convertAndAssertImplies(node, negated); break;
    case Kind::ITE: convertAndAssertIte(node, negated); break;
    case Kind::NOT: convertAndAssert(node[0], !negated); break;
    case Kind::EQUAL:
      if (node[0].getType().isBoolean())
      {
        convertAndAssertIff(node, negated);
        break;
      }
      CVC5_FALLTHROUGH;
    default:
    {
      Node nnode = node;
      if (negated)
      {
        nnode = node.negate();
      }
      // Atoms
      assertClause(nnode, toCNF(node, negated));
    }
    break;
  }
}

ClauseCnfStream::Statistics::Statistics(StatisticsRegistry& sr,
                                        const std::string& name)
    : d_cnfConversionTime(
          sr.registerTimer(name + "::ClauseCnfStream::cnfConversionTime")),
      d_numAtoms(sr.registerInt(name + "::ClauseCnfStream::numAtoms"))
{
}

}  // namespace pb
}  // namespace bv
}  // namespace theory
}  // namespace cvc5::internal
