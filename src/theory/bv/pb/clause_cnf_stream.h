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
 * A CNF converter that mirrors prop::CnfStream, but instead of streaming the
 * generated clauses to a SAT solver it stores them so that a user can later
 * retrieve the full clause set.
 *
 * This class takes a sequence of formulas and produces a stream of clauses
 * that is propositionally equi-satisfiable with the conjunction of the
 * formulas. Unlike prop::CnfStream it does not depend on a SAT solver: fresh
 * SAT variables are allocated internally and the resulting clauses are kept in
 * a container accessible via getClauses(). The node <-> literal mappings are
 * maintained just like in prop::CnfStream and exposed via getTranslationCache()
 * and getNodeCache().
 */

#include "cvc5_private.h"

#ifndef CVC5__THEORY__BV__PB__CLAUSE_CNF_STREAM_H
#define CVC5__THEORY__BV__PB__CLAUSE_CNF_STREAM_H

#include <string>
#include <vector>

#include "context/cdhashset.h"
#include "context/cdinsert_hashmap.h"
#include "context/cdlist.h"
#include "expr/node.h"
#include "prop/sat_solver_types.h"
#include "smt/env_obj.h"
#include "util/statistics_stats.h"

namespace cvc5::internal {

class Env;

namespace theory {
namespace bv {
namespace pb {

/** A policy for how literals for formulas are handled. */
enum class FormulaLitPolicy : uint32_t
{
  // literals for formulas are tracked (added to node map)
  TRACK,
  // literals for Boolean variables are tracked
  TRACK_AND_NOTIFY_VAR,
  // literals for formulas are kept internal (default)
  INTERNAL,
};

/**
 * Mirror of prop::CnfStream that stores the clauses it generates instead of
 * asserting them to a SAT solver.
 *
 * Implements the Tseitin transformation in a single pass. The general idea is
 * to introduce a new literal that will be equivalent to each subexpression in
 * the constructed equi-satisfiable formula, then substitute the new literal for
 * the formula, and so on, recursively.
 */
class ClauseCnfStream : protected EnvObj
{
 public:
  /** Cache of what nodes have been registered to a literal. */
  typedef context::
      CDInsertHashMap<prop::SatLiteral, TNode, prop::SatLiteralHashFunction>
          LiteralToNodeMap;

  /** Cache of what literals have been registered to a node. */
  typedef context::CDInsertHashMap<Node, prop::SatLiteral> NodeToLiteralMap;

  /**
   * Constructs a ClauseCnfStream that performs equisatisfiable CNF
   * transformations and stores the generated clauses. Unlike prop::CnfStream,
   * no SAT solver is required: fresh SAT variables are allocated internally.
   * This does not take ownership of the context.
   *
   * @param env reference to the environment
   * @param c the context that the CNF should respect.
   * @param flpol policy for literals corresponding to formulas (those that are
   * not-theory literals).
   * @param name string identifier to distinguish between different instances.
   */
  ClauseCnfStream(Env& env,
                  context::Context* c,
                  FormulaLitPolicy flpol = FormulaLitPolicy::INTERNAL,
                  std::string name = "");

  /**
   * Convert a given formula to CNF and store the resulting clauses.
   *
   * @param node node to convert and assert
   * @param removable whether the clauses may be considered removable
   * @param negated whether we are asserting the node negated
   */
  void convertAndAssert(TNode node, bool removable, bool negated);

  /** Access the clauses that have been generated so far. */
  const std::vector<prop::SatClause>& getClauses() const;

  /** Number of clauses that have been generated so far. */
  std::size_t getNumClauses() const;

  /** Discard the stored clauses. Does not reset the literal mappings. */
  void clearClauses();

  /**
   * Get the node that is represented by the given SatLiteral.
   * @param literal the literal
   * @return the actual node
   */
  TNode getNode(const prop::SatLiteral& literal);

  /**
   * Returns true iff the node has an assigned literal (it might not be
   * translated).
   * @param node the node
   */
  bool hasLiteral(TNode node) const;

  /**
   * Ensure that the given node will have a designated SAT literal that is
   * definitionally equal to it. Like a "convert-but-don't-assert" version of
   * convertAndAssert().
   */
  void ensureLiteral(TNode n);

  /**
   * Returns the literal that represents the given node in the CNF
   * representation.
   */
  prop::SatLiteral getLiteral(TNode node);

  /** Returns the Boolean variables from the input problem. */
  void getBooleanVariables(std::vector<TNode>& outputVariables) const;

  /**
   * Returns true if node is a formula tracked via
   * FormulaLitPolicy::TRACK_AND_NOTIFY_VAR.
   */
  bool isNotifyFormula(TNode node) const;

  /** Retrieves map from nodes to literals. */
  const NodeToLiteralMap& getTranslationCache() const;

  /** Retrieves map from literals to nodes. */
  const LiteralToNodeMap& getNodeCache() const;

 protected:
  /**
   * Same as the public convertAndAssert, except that it uses the saved
   * d_removable flag. It calls the dedicated converter for the possible formula
   * kinds.
   */
  void convertAndAssert(TNode node, bool negated);
  /** Specific converters for each formula kind. */
  void convertAndAssertAnd(TNode node, bool negated);
  void convertAndAssertOr(TNode node, bool negated);
  void convertAndAssertXor(TNode node, bool negated);
  void convertAndAssertIff(TNode node, bool negated);
  void convertAndAssertImplies(TNode node, bool negated);
  void convertAndAssertIte(TNode node, bool negated);

  /**
   * Transforms the node into CNF recursively and yields a literal
   * definitionally equal to it, populating the node <-> literal caches.
   *
   * @param node the formula to transform
   * @param negated whether the literal is negated
   * @return the literal representing the root of the formula
   */
  prop::SatLiteral toCNF(TNode node, bool negated = false);

  /**
   * Specific clausifiers that clausify a formula based on the given formula
   * kind and introduce a literal definitionally equal to it.
   */
  void handleXor(TNode node);
  void handleImplies(TNode node);
  void handleIff(TNode node);
  void handleIte(TNode node);
  void handleAnd(TNode node);
  void handleOr(TNode node);

  /** Stores the literal of the given node in d_literalToNodeMap.
   *
   * Note that n must already have a literal associated to it in
   * d_nodeToLiteralMap.
   */
  void ensureMappingForLiteral(TNode n);

  /**
   * Stores the given clause.
   * @param node the node giving rise to this clause
   * @param clause the clause to store
   * @return whether the clause was stored.
   */
  bool assertClause(TNode node, prop::SatClause& clause);

  /** Stores the unit clause. */
  bool assertClause(TNode node, prop::SatLiteral a);

  /** Stores the binary clause. */
  bool assertClause(TNode node, prop::SatLiteral a, prop::SatLiteral b);

  /** Stores the ternary clause. */
  bool assertClause(TNode node,
                    prop::SatLiteral a,
                    prop::SatLiteral b,
                    prop::SatLiteral c);

  /**
   * Allocates a new variable to represent the node and inserts the necessary
   * data into the mapping tables.
   * @param node a formula
   * @param isTheoryAtom is this a theory atom.
   * @return the literal corresponding to the formula
   */
  prop::SatLiteral newLiteral(TNode node, bool isTheoryAtom = false);

  /**
   * Constructs a new literal for an atom and returns it. Calls newLiteral().
   *
   * @param node the node to convert; there should be no Boolean structure in
   * this expression. Assumed to not be in the translation cache.
   */
  prop::SatLiteral convertAtom(TNode node);

  /** Allocates a fresh SAT variable (replaces SatSolver::newVar). */
  prop::SatVariable newVar();
  /** The variable that stands for constant true (allocated lazily). */
  prop::SatVariable trueVar();
  /** The variable that stands for constant false (allocated lazily). */
  prop::SatVariable falseVar();

  /** The clauses that have been generated. */
  std::vector<prop::SatClause> d_clauses;

  /** The next fresh SAT variable to hand out. */
  prop::SatVariable d_nextVar;

  /** The designated constant-true variable, or undefSatVariable if unused. */
  prop::SatVariable d_trueVar;

  /** The designated constant-false variable, or undefSatVariable if unused. */
  prop::SatVariable d_falseVar;

  /** Boolean variables that we translated */
  context::CDList<TNode> d_booleanVariables;

  /** Formulas that we translated that we are notifying */
  context::CDHashSet<Node> d_notifyFormulas;

  /** Map from nodes to literals */
  NodeToLiteralMap d_nodeToLiteralMap;

  /** Map from literals to nodes */
  LiteralToNodeMap d_literalToNodeMap;

  /**
   * True if the lit-to-Node map should be kept for all lits, not just theory
   * lits.
   */
  const FormulaLitPolicy d_flitPolicy;

  /** The name of this CNF stream */
  std::string d_name;

  /**
   * Are we generating a removable clause (true) or a permanent clause (false).
   * This is set at the beginning of convertAndAssert so that it doesn't need to
   * be passed on over the stack. Only pure clauses can be generated as
   * removable.
   */
  bool d_removable;

 private:
  struct Statistics
  {
    Statistics(StatisticsRegistry& sr, const std::string& name);
    TimerStat d_cnfConversionTime;
    /** Number of atoms */
    IntStat d_numAtoms;
  };
  /** Statistics */
  Statistics d_stats;

}; /* class ClauseCnfStream */

}  // namespace pb
}  // namespace bv
}  // namespace theory
}  // namespace cvc5::internal

#endif /* CVC5__THEORY__BV__PB__CLAUSE_CNF_STREAM_H */
