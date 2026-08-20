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
 * cvc5 cutting-planes proof steps (CUTTING_PLANES_* and the macro
 * MACRO_CUTTING_PLANES_RESOLUTION).
 *
 * First-cut scope: 'rup' and the 'pol' (reverse polish notation) operations
 * embedded in VeriPB. Other VeriPB step kinds are skipped with a trace.
 */
#include "cvc5_private.h"

#ifndef CVC5__THEORY__BV__PB__PB_PROOF_TRANSLATOR_H
#define CVC5__THEORY__BV__PB__PB_PROOF_TRANSLATOR_H

#include <functional>
#include <unordered_map>

#include "proof/proof.h"
#include "smt/env_obj.h"

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
   * Register an input-formula constraint with its 1-based VeriPB id.
   * Subsequent rup/pol steps may reference this id.
   */
  void registerInputConstraint(size_t veriPbId, Node constraint);

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
   * Translate a 'rup' step into a MACRO_CUTTING_PLANES_RESOLUTION proof
   * step. The hint ids in child 1 are resolved to previously-recorded
   * conclusions and supplied as children. The polarities and pivots
   * s-expressions are emitted empty for now; computing them is a TODO that
   * mirrors proof_tracer.cpp's mark_var scan, adapted to PB literals.
   *
   * Returns the conclusion Node of the emitted step.
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
};

}  // namespace pb
}  // namespace bv
}  // namespace theory
}  // namespace cvc5::internal

#endif /* CVC5__THEORY__BV__PB__PB_PROOF_TRANSLATOR_H */
