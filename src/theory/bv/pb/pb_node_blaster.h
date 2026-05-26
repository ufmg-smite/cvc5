/******************************************************************************
 * Top contributors (to current version):
 *   Alan Prado, Haniel Barbosa
 *
 * This file is part of the cvc5 project.
 *
 * Copyright (c) 2009-2026 by the authors listed in the file AUTHORS
 * in the top-level source directory and their institutional affiliations.
 * All rights reserved.  See the file COPYING in the top-level source
 * directory for licensing information.
 * ****************************************************************************
 *
 * Implementation of a simple PB-blaster. Implements the bare minimum to PB-
 * blast bit-vector atoms and terms.
 */

#include "cvc5_private.h"

#ifndef CVC5__THEORY__BV__PB__PB_NODE_BLASTER_H
#define CVC5__THEORY__BV__PB__PB_NODE_BLASTER_H

#include "theory/bv/pb/pb_blaster.h"
#include "theory/theory.h"

namespace cvc5::internal {
namespace theory {
namespace bv {
namespace pb {

class PseudoBooleanBlaster : public TPseudoBooleanBlaster<Node>,
                             protected EnvObj
{
 public:
  PseudoBooleanBlaster(Env& env, TheoryState* state);
  ~PseudoBooleanBlaster() = default;

  /** PB-blast atom 'node'. */
  void blastAtom(Node atom) override;

  /**
   * PB-blast term 'node', returns the variables for this term and the
   * constraints for it and for all its subterms.
   */
  Node blastTerm(Node term) override;

  /** Create a new variable not yet used in the solver. */
  Node newVariable(unsigned numBits = 1) override;

 private:
  /** Counts variables used so far. */
  unsigned d_varCounter;
};

}  // namespace pb
}  // namespace bv
}  // namespace theory
}  // namespace cvc5::internal

#endif  // CVC5__THEORY__BV__PB__PB_NODE_BLASTER_H
