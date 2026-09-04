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
 * Translator from VeriPB proof step Nodes (as produced by PbProofRules) into
 * cvc5 cutting-planes proof steps (CUTTING_PLANES_*).
 *
 * First-cut scope: 'rup' and the 'pol' (reverse polish notation) operations
 * embedded in VeriPB. Other VeriPB step kinds are skipped with a trace.
 */
#include "cvc5_private.h"

#ifndef CVC5__THEORY__BV__PB__PB_PROOF_TRANSLATOR_H
#define CVC5__THEORY__BV__PB__PB_PROOF_TRANSLATOR_H

#include <cstdint>
#include <functional>
#include <unordered_map>
#include <unordered_set>

#include "proof/proof.h"
#include "smt/env_obj.h"
#include "util/integer.h"

namespace cvc5::internal {
namespace theory {
namespace bv {
namespace pb {

/**
 * Walks the VeriPB step Nodes produced by PbProofRules and emits the
 * corresponding cutting-planes ProofRule steps into a CDProof.
 */
class PbProofTranslator : protected EnvObj
{
 public:
  PbProofTranslator(Env& env, CDProof* cdp);
  ~PbProofTranslator() = default;

  /**
   * Register an input-formula constraint with its 1-based VeriPB id and the
   * BV fact it was blasted from. The constraint is stored in canonical GEQ
   * form, concluded from `fact` by a MACRO_PB_BLAST_STEP; an EQUAL constraint
   * takes two consecutive ids, with `negated` selecting the flipped second
   * direction. Each new variable's {0,1} domain bounds are also concluded
   * from `fact`, so later steps can use them as premises directly.
   */
  void registerInputConstraint(size_t veriPbId,
                               Node constraint,
                               Node fact,
                               bool negated);

  /**
   * Bind each PB variable to its backend index, the canonical term order used
   * by mkPbConstraint. Must be set before registering constraints.
   */
  void setVariableIndices(std::unordered_map<Node, uint64_t> indices);

  /**
   * Translate one VeriPB step Node into one or more cutting-planes proof
   * steps added to the underlying CDProof.
   * Currently handles PB_PROOF_REVERSE_UNIT_PROPAGATION and
   * PB_PROOF_REVERSE_POLISH_NOTATION; other step kinds are skipped.
   */
  void translateStep(Node veriPbStep, size_t veriPbId);

 private:
  /** Populate d_translators with one entry per supported VeriPB step Kind. */
  void initializeTranslators();

  /**
   * Translate a 'rup' step into a CUTTING_PLANES_RUP proof step. The hint ids
   * in child 1 resolve to previously-recorded conclusions, which become the
   * premises together with the negated derived constraint; the hints are
   * re-expressed as indices into that premise list and passed as an argument.
   * RUP is a primitive of the calculus, so the step is emitted as-is rather
   * than elaborated into cutting-planes derivations. Since the rule refutes
   * its premises, a SCOPE discharging the negated constraint and a
   * NOT_NOT_ELIM recover the derived constraint itself.
   *
   * Returns the derived constraint, the conclusion recorded for this id.
   */
  Node translateRup(Node rupNode, size_t veriPbId);

  /**
   * Translate a 'pol' step. Walks the reverse-polish-notation expression
   * tree bottom-up, emitting a CUTTING_PLANES_* primitive step per internal
   * node and registering each step's conclusion. Returns the top-level
   * conclusion.
   */
  Node translatePol(Node polNode, size_t veriPbId);

  /** Recursive RPN walker used by translatePol. */
  Node translatePolExpr(Node expr);

  /** The backend index of `variable`; asserts it is bound. */
  uint64_t variableIndex(const Node& variable) const;

  /**
   * The canonical Node for the PB constraint `sum(terms) >= rhs`: zero
   * coefficients dropped, terms sorted by backend variable index, and the same
   * MULT/ADD/GEQ shape parseOpbFormat produces.
   */
  Node mkPbConstraint(const std::unordered_map<Node, Integer>& terms,
                      const Integer& rhs);

  /**
   * Expand a weakening on `variable` into AXIOM/MULTIPLICATION/ADDITION steps
   * that cancel it from `constraint`. Returns the weakened conclusion.
   */
  Node weakenVariable(Node constraint, Node variable);

  /**
   * Translate a 'conclusion' footer. In decision mode RoundingSat always
   * emits `conclusion UNSAT : <id>`; we look up <id> and refute the
   * contradicting PB constraint it names, so that `false` sits on top of the
   * fine-grained per-step DAG.
   */
  Node translateConclusion(Node conclNode, size_t veriPbId);

  /**
   * Derive `false` from a contradicting PB constraint by cancelling each of
   * its variables against that variable's {0,1} domain bound, leaving a
   * ground inequality that rewrites to false.
   */
  void refuteContradicting(Node constraint);

  /** The upper (x <= 1) or lower (x >= 0) domain bound of a PB variable. */
  Node mkDomainBound(Node variable, bool upper);

  /**
   * Translate a `del id <ids>` step (PB_PROOF_DELETE_BY_ID). Iterates the
   * SEXPR child of ids and removes each one from whichever database (core or
   * derived) holds it. Matches VeriPB augmented-format semantics: for each id,
   * dispatch to `deld` if it's in the derived set, `delc` if in the core set.
   * Returns a null Node (no conclusion to record).
   */
  Node translateDeleteById(Node delNode, size_t veriPbId);

  /**
   * Look up the conclusion Node previously registered for a VeriPB constraint
   * id (passed as a CONST_INTEGER produced by the PbProofRules parser).
   * Checks the core database first, then derived. Returns a null Node if not
   * found.
   */
  Node lookup(Node idNode) const;

  /** Remove `id` from the derived set; assert it is in fact derived. */
  void eraseDerived(size_t veriPbId);
  /** Remove `id` from the core set; assert it is in fact core (unchecked). */
  void eraseCore(size_t veriPbId);
  /**
   * Remove `id` from whichever set holds it. Implements VeriPB augmented
   * `del id <ids>`: dispatches to eraseDerived or eraseCore per id.
   */
  void eraseAny(size_t veriPbId);

  CDProof* d_cdp;

  /**
   * The VeriPB *core* set C (Section 3.4 of the SAT 2025 VeriPB spec). Holds
   * Maps the numeric VeriPB id to the conclusion Node.
   */
  std::unordered_map<size_t, Node> d_coreById;

  /**
   * The VeriPB *derived* set D.
   * Maps the numeric VeriPB id to the conclusion Node.
   */
  std::unordered_map<size_t, Node> d_derivedById;

  /**
   * Maps each supported VeriPB step Kind to the handler that translates it
   * and returns its conclusion Node. Mirrors PbProofRules::rules: a flat
   * dispatch table that scales with the number of step kinds without an
   * if-else chain in translateStep. Handlers must return a non-null Node on
   * success; returning a null Node signals "no conclusion recorded".
   */
  std::unordered_map<Kind, std::function<Node(Node, size_t)>> d_translators;

  /** Backend index per PB variable; the canonical term order. */
  std::unordered_map<Node, uint64_t> d_varIndex;

  /** Variables whose domain bounds were proved during input registration. */
  std::unordered_set<Node> d_boundedVars;
};

}  // namespace pb
}  // namespace bv
}  // namespace theory
}  // namespace cvc5::internal

#endif /* CVC5__THEORY__BV__PB__PB_PROOF_TRANSLATOR_H */
