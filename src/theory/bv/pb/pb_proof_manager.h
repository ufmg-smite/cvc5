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
 * PB proof manager for generating proofs for the complete PB-blasting
 * procedure. This includes:
 * - Recovering PB-blasting proof steps;
 * - Generating cutting-plane proofs of unsatisfiability.
 * - TODO(alanctprado): what else?
 */
#include "cvc5_private.h"

#ifndef CVC5__THEORY__BV__PB__PB_PROOF_MANAGER_H
#define CVC5__THEORY__BV__PB__PB_PROOF_MANAGER_H

#include "smt/env_obj.h"
#include "theory/bv/pb/pb_blast_proof_generator.h"
#include "theory/bv/pb/pb_proof_rules.h"

namespace cvc5::internal {

namespace theory {
namespace bv {
namespace pb {

class PbProofManager : protected EnvObj
{
 public:
  PbProofManager(Env& env, PbBlastProofGenerator* pbbpg);
  ~PbProofManager() {};

  void addPbProof(std::vector<std::string>
                      pb_proof);  // TODO(alanctprado): do not copy proof

 private:
  PbBlastProofGenerator* d_pbbpg;
  CDProof* d_cdp;
  PbProofRules* d_pbpr;
  std::vector<Node> parseProofLines(std::vector<std::string> proofLines);
};

}  // namespace pb
}  // namespace bv
}  // namespace theory
}  // namespace cvc5::internal

#endif /* CVC5__THEORY__BV__PB__PB_PROOF_MANAGER_H */
