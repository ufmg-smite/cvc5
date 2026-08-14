/******************************************************************************
 * This file is part of the cvc5 project.
 *
 * Copyright (c) 2009-2026 by the authors listed in the file AUTHORS
 * in the top-level source directory and their institutional affiliations.
 * All rights reserved.  See the file COPYING in the top-level source
 * directory for licensing information.
 * ****************************************************************************
 *
 * The module for final processing proof nodes.
 */

#include "cvc5_private.h"

#ifndef CVC5__SMT__PROOF_INTERPOLATION_H
#define CVC5__SMT__PROOF_INTERPOLATION_H

#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "expr/node.h"
#include "proof/proof_node.h"

namespace cvc5::internal {
namespace smt {

void partition(const std::vector<Node>& aTerms,
               const std::vector<Node>& assertions,
               std::unordered_set<Node>& aAssertions,
               std::unordered_set<Node>& bAssertions,
               std::unordered_set<Node>& aSymbols,
               std::unordered_set<Node>& bSymbols);

Node getItp(std::shared_ptr<ProofNode> p,
            std::unordered_set<Node>& aAssertions,
            std::unordered_set<Node>& bAssertions,
            std::unordered_set<Node>& aSymbols,
            std::unordered_set<Node>& bSymbols,
            NodeManager* nm,
            std::unordered_map<ProofNode*, Node>& cache);

}  // namespace smt
}  // namespace cvc5::internal



#endif /* CVC5__SMT__PROOF_INTERPOLATION_H */