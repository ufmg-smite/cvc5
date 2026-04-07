/******************************************************************************
 * Top contributors (to current version):
 *   Alan Prado
 *
 * This file is part of the cvc5 project.
 *
 * Copyright (c) 2009-2025 by the authors listed in the file AUTHORS
 * in the top-level source directory and their institutional affiliations.
 * All rights reserved.  See the file COPYING in the top-level source
 * directory for licensing information.
 * ****************************************************************************
 *
 * TODO(alanctprado)
 */

#include "theory/bv/pb/pb_blast_proof_generator.h"

namespace cvc5::internal {
namespace theory {
namespace bv {
namespace pb{

PbBlastProofGenerator::PbBlastProofGenerator(Env& env) : EnvObj(env)
{}

std::shared_ptr<ProofNode> PbBlastProofGenerator::getProofFor(CVC5_UNUSED Node eq)
{
    return nullptr;
}

}  // namespace pb
}  // namespace bv
}  // namespace theory
}  // namespace cvc5::internal
