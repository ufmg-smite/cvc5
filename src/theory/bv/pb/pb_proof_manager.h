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

#include <cstdint>
#include <string>
#include <unordered_map>

#include "proof/proof_generator.h"
#include "smt/env_obj.h"
#include "theory/bv/pb/pb_blast_proof_generator.h"
#include "theory/bv/pb/pb_proof_rules.h"
#include "theory/bv/pb/pb_proof_translator.h"

namespace cvc5::internal {

namespace theory {
namespace bv {
namespace pb {

class PbProofManager : public ProofGenerator, protected EnvObj
{
 public:
  PbProofManager(Env& env, PbBlastProofGenerator* pbbpg);
  ~PbProofManager() {};

  /**
   * Convert the parsed VeriPB step Nodes into cvc5 cutting-planes proof steps,
   * adding them to the underlying CDProof. The input formula constraints
   * (the same Nodes fed to the PB backend, in the order they were added) are
   * registered under VeriPB ids 1..N before translation begins, so that
   * subsequent hint references and constraint-id leaves resolve.
   */
  void convertProof(const std::vector<Node>& veriPbProof,
                    const std::vector<std::pair<Node, Node>>& inputConstraints);
  /**
   * Process a parsed VeriPB proof, with `inputConstraints` carrying each
   * input PB constraint paired with the BV-level fact it was blasted from.
   * The pair lets us emit one TRUST_THEORY_LEMMA per input constraint that
   * ties it back to its BV fact, so the per-step DAG built by convertProof
   * closes against the BV-level conflict assumptions.
   */
  void addPbProof(std::vector<std::string> pb_proof,
                  const std::vector<std::pair<Node, Node>>& inputConstraints,
                  std::unordered_map<std::string, Node> proofVariables);

  /** ProofGenerator: returns the CDProof's proof DAG for `fact`. */
  std::shared_ptr<ProofNode> getProofFor(Node fact) override;
  std::string identify() const override { return "PbProofManager"; }

 private:
  CVC5_UNUSED_FIELD PbBlastProofGenerator*
      d_pbbpg;  // TODO(alanctprado): used once proofs land
  CDProof* d_cdp;
  PbProofRules* d_pbpr;
  /** Backend index per PB variable, parsed from the proof variable names. */
  std::unordered_map<Node, uint64_t> d_varIndex;
  std::vector<Node> parseProofLines(std::vector<std::string> proofLines);
};

}  // namespace pb
}  // namespace bv
}  // namespace theory
}  // namespace cvc5::internal

#endif /* CVC5__THEORY__BV__PB__PB_PROOF_MANAGER_H */
