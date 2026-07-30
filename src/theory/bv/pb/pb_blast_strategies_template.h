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
 * Implementation of pseudo-boolean blasting functions for various operators.
 */

#include "cvc5_private.h"

#ifndef CVC5__THEORY__BV__PB__PB_BLAST_STRATEGIES_TEMPLATE_H
#define CVC5__THEORY__BV__PB__PB_BLAST_STRATEGIES_TEMPLATE_H

#include <cmath>

#include "theory/bv/pb/pb_blast_utils.h"
#include "theory/bv/theory_bv_utils.h"
#include "util/bitvector.h"

namespace cvc5::internal {
namespace theory {
namespace bv {
namespace pb {

/**
 * Default Atom pb-blasting strategies:
 *
 * @param atom  the atom to be pb-blasted
 * @param pbb   the pseudo-boolean blaster
 */

/** Fallback method for unimplemented atom strategies */
template <class T>
T UndefinedAtomPbStrategy(T atom, CVC5_UNUSED TPseudoBooleanBlaster<T>* pbb)
{
  Trace("bv-pb") << "Undefined PB-blasting strategy for atom of kind: "
                 << atom.getKind() << "\n";
  Unreachable();
}

/** TODO(alanctprado): consider adding bit-level equalities? */
/*template <class T>
T DefaultEqPb(T atom, TPseudoBooleanBlaster<T>* pbb)
{
  Assert(atom.getKind() == Kind::EQUAL);
  Trace("bv-pb") << "theory::bv::pb::DefaultEqPb " << atom << "\n";

  T lhs = pbb->blastTerm(atom[0]);
  T rhs = pbb->blastTerm(atom[1]);
  Assert(lhs[0].getNumChildren() == rhs[0].getNumChildren());

  NodeManager* nm = pbb->getNodeManager();
  std::vector<T> coefficients = bvToUnsigned(lhs[0].getNumChildren(), nm);
  for (const T& c : bvToUnsigned(rhs[0].getNumChildren(), nm, -1))
    coefficients.push_back(c);

  std::vector<T> variables;
  for (const T& v : lhs[0]) variables.push_back(v);
  for (const T& v : rhs[0]) variables.push_back(v);

  T atom_constraint =
      mkConstraintNode(Kind::EQUAL, variables, coefficients, pbb->d_ZERO, nm);
  Trace("bv-pb") << "theory::bv::pb::DefaultEqPb resulted in constraint "
                 << atom_constraint << "\n";

  std::unordered_set<T> constraints;
  constraints.insert(atom_constraint);
  for (const T& c : lhs[1]) constraints.insert(c);
  for (const T& c : rhs[1]) constraints.insert(c);
  return mkAtomNode(constraints, nm);
}*/

template <class T>
T DefaultEqPb(T atom, TPseudoBooleanBlaster<T>* pbb)
{
  Assert(atom.getKind() == Kind::EQUAL);
  Trace("bv-pb") << "theory::bv::pb::DefaultEqPb " << atom << "\n";

  T lhs = pbb->blastTerm(atom[0]);
  T rhs = pbb->blastTerm(atom[1]);
  Assert(lhs[0].getNumChildren() == rhs[0].getNumChildren());

  NodeManager* nm = pbb->getNodeManager();
  std::vector<T> atom_constraints;
  for (unsigned i = 0; i < lhs[0].getNumChildren(); i++)
  {
    std::vector<Node> unit_constraint = {lhs[0][i], rhs[0][i]};
    atom_constraints.push_back(
          mkConstraintNode(Kind::EQUAL, unit_constraint, {1, -1}, 0, nm));
  }

  Trace("bv-pb") << "theory::bv::pb::DefaultEqPb resulted in constraints";
  for (const T& c : atom_constraints) Trace("bv-pb") << " " << c;
  Trace("bv-pb") << "\n";

  std::unordered_set<T> constraints;
  for (const T& c : atom_constraints) constraints.emplace(c);
  for (const T& c : lhs[1]) constraints.insert(c);
  for (const T& c : rhs[1]) constraints.insert(c);
  return mkAtomNode(constraints, nm);
}

template <class T>
T DefaultUltPb(T atom, TPseudoBooleanBlaster<T>* pbb)
{
  Assert(atom.getKind() == Kind::BITVECTOR_ULT);
  Trace("bv-pb") << "theory::bv::pb::DefaultUltPb " << atom << "\n";

  T lhs = pbb->blastTerm(atom[0]);
  T rhs = pbb->blastTerm(atom[1]);
  Assert(lhs[0].getNumChildren() == rhs[0].getNumChildren());

  NodeManager* nm = pbb->getNodeManager();
  std::vector<T> coefficients = bvToUnsigned(rhs[0].getNumChildren(), nm);
  for (const T& c : bvToUnsigned(lhs[0].getNumChildren(), nm, -1))
    coefficients.push_back(c);

  std::vector<T> variables;
  for (const T& v : rhs[0]) variables.push_back(v);
  for (const T& v : lhs[0]) variables.push_back(v);

  T atom_constraint =
      mkConstraintNode(Kind::GEQ, variables, coefficients, pbb->d_ONE, nm);
  Trace("bv-pb") << "theory::bv::pb::DefaultUltPb resulted in constraint "
                 << atom_constraint << "\n";

  std::unordered_set<T> constraints;
  constraints.emplace(atom_constraint);
  for (const T& c : lhs[1]) constraints.insert(c);
  for (const T& c : rhs[1]) constraints.insert(c);
  return mkAtomNode(constraints, nm);
}

template <class T>
T DefaultUgePb(T atom, TPseudoBooleanBlaster<T>* pbb)
{
  Assert(atom.getKind() == Kind::BITVECTOR_UGE);
  Trace("bv-pb") << "theory::bv::pb::DefaultUgePb " << atom << "\n";

  T lhs = pbb->blastTerm(atom[0]);
  T rhs = pbb->blastTerm(atom[1]);
  Assert(lhs[0].getNumChildren() == rhs[0].getNumChildren());

  NodeManager* nm = pbb->getNodeManager();
  std::vector<T> coefficients = bvToUnsigned(lhs[0].getNumChildren(), nm);
  for (const T& c : bvToUnsigned(rhs[0].getNumChildren(), nm, -1))
    coefficients.push_back(c);

  std::vector<T> variables;
  for (const T& v : lhs[0]) variables.push_back(v);
  for (const T& v : rhs[0]) variables.push_back(v);

  T atom_constraint =
      mkConstraintNode(Kind::GEQ, variables, coefficients, pbb->d_ZERO, nm);
  Trace("bv-pb") << "theory::bv::pb::DefaultUgePb resulted in constraint "
                 << atom_constraint << "\n";

  std::unordered_set<T> constraints;
  constraints.emplace(atom_constraint);
  for (const T& c : lhs[1]) constraints.insert(c);
  for (const T& c : rhs[1]) constraints.insert(c);
  return mkAtomNode(constraints, nm);
}

template <class T>
T DefaultUlePb(T atom, TPseudoBooleanBlaster<T>* pbb)
{
  Assert(atom.getKind() == Kind::BITVECTOR_ULE);
  Trace("bv-pb") << "theory::bv::pb::DefaultUlePb " << atom << "\n    "
                 << "is equivalent to DefaultUgePb with the sides swapped\n";
  T swapped_atom =
      pbb->getNodeManager()->mkNode(Kind::BITVECTOR_UGE, atom[1], atom[0]);
  return DefaultUgePb(swapped_atom, pbb);
}

template <class T>
T DefaultUgtPb(T atom, TPseudoBooleanBlaster<T>* pbb)
{
  Assert(atom.getKind() == Kind::BITVECTOR_UGT);
  Trace("bv-pb") << "theory::bv::pb::DefaultUgtPb " << atom << "\n    "
                 << "is equivalent to DefaultUltPb with the sides swapped\n";
  T swapped_atom =
      pbb->getNodeManager()->mkNode(Kind::BITVECTOR_ULT, atom[1], atom[0]);
  return DefaultUltPb(swapped_atom, pbb);
}

template <class T>
T DefaultSltPb(T atom, TPseudoBooleanBlaster<T>* pbb)
{
  Assert(atom.getKind() == Kind::BITVECTOR_SLT);
  Trace("bv-pb") << "theory::bv::pb::DefaultSltPb " << atom << "\n";

  T lhs = pbb->blastTerm(atom[0]);
  T rhs = pbb->blastTerm(atom[1]);
  Assert(lhs[0].getNumChildren() == rhs[0].getNumChildren());

  NodeManager* nm = pbb->getNodeManager();
  std::vector<T> coefficients = bvToSigned(rhs[0].getNumChildren(), nm);
  for (const T& c : bvToSigned(lhs[0].getNumChildren(), nm, -1))
    coefficients.push_back(c);

  std::vector<T> variables;
  for (const T& v : rhs[0]) variables.push_back(v);
  for (const T& v : lhs[0]) variables.push_back(v);

  T atom_constraint =
      mkConstraintNode(Kind::GEQ, variables, coefficients, pbb->d_ONE, nm);
  Trace("bv-pb") << "theory::bv::pb::DefaultSltPb resulted in constraint "
                 << atom_constraint << "\n";

  std::unordered_set<T> constraints;
  constraints.emplace(atom_constraint);
  for (const T& c : lhs[1]) constraints.insert(c);
  for (const T& c : rhs[1]) constraints.insert(c);
  return mkAtomNode(constraints, nm);
}

template <class T>
T DefaultSgePb(T atom, TPseudoBooleanBlaster<T>* pbb)
{
  Assert(atom.getKind() == Kind::BITVECTOR_SGE);
  Trace("bv-pb") << "theory::bv::pb::DefaultSgePb " << atom << "\n";

  T lhs = pbb->blastTerm(atom[0]);
  T rhs = pbb->blastTerm(atom[1]);
  Assert(lhs[0].getNumChildren() == rhs[0].getNumChildren());

  NodeManager* nm = pbb->getNodeManager();
  std::vector<T> coefficients = bvToSigned(lhs[0].getNumChildren(), nm);
  for (const T& c : bvToSigned(rhs[0].getNumChildren(), nm, -1))
    coefficients.push_back(c);

  std::vector<T> variables;
  for (const T& v : lhs[0]) variables.push_back(v);
  for (const T& v : rhs[0]) variables.push_back(v);

  T atom_constraint =
      mkConstraintNode(Kind::GEQ, variables, coefficients, pbb->d_ZERO, nm);
  Trace("bv-pb") << "theory::bv::pb::DefaultSgePb resulted in constraint "
                 << atom_constraint << "\n";

  std::unordered_set<T> constraints;
  constraints.emplace(atom_constraint);
  for (const T& c : lhs[1]) constraints.insert(c);
  for (const T& c : rhs[1]) constraints.insert(c);
  return mkAtomNode(constraints, nm);
}

template <class T>
T DefaultSlePb(T atom, TPseudoBooleanBlaster<T>* pbb)
{
  Assert(atom.getKind() == Kind::BITVECTOR_SLE);
  Trace("bv-pb") << "theory::bv::pb::DefaultSlePb " << atom << "\n    "
                 << "is equivalent to DefaultSgePb with the sides swapped\n";
  T swapped_atom =
      pbb->getNodeManager()->mkNode(Kind::BITVECTOR_SGE, atom[1], atom[0]);
  return DefaultSgePb(swapped_atom, pbb);
}

template <class T>
T DefaultSgtPb(T atom, TPseudoBooleanBlaster<T>* pbb)
{
  Assert(atom.getKind() == Kind::BITVECTOR_SGT);
  Trace("bv-pb") << "theory::bv::pb::DefaultSgtPb " << atom << "\n    "
                 << "is equivalent to DefaultSltPb with the sides swapped\n";
  T swapped_atom =
      pbb->getNodeManager()->mkNode(Kind::BITVECTOR_SLT, atom[1], atom[0]);
  return DefaultSltPb(swapped_atom, pbb);
}

/**
 * Negated Atom pb-blasting strategies:
 *
 * @param atom  the atom to be pb-blasted
 * @param pbb   the pseudo-boolean blaster
 */

/**
 * Negated Bit-Vector Equality
 *
 * (x != y) is equivalent to
 *
 * r = xor(x, y)
 * \sum_i r_i >= 1
 */
template <class T>
T NegatedEqPb(T atom, TPseudoBooleanBlaster<T>* pbb)
{
  Trace("bv-pb") << "theory::bv::pb::NegatedEqPb " << atom << "\n";
  Assert(atom.getKind() == Kind::EQUAL);
  NodeManager* nm = pbb->getNodeManager();

  T xor_node =
      pbb->getNodeManager()->mkNode(Kind::BITVECTOR_XOR, atom[0], atom[1]);
  T blasted_xor = pbb->blastTerm(xor_node);

  Assert(utils::getSize(atom[0]) == utils::getSize(atom[1]));
  Assert(blasted_xor[0].getNumChildren() == utils::getSize(atom[0]));

  std::vector<T> variables;
  for (const T& v : blasted_xor[0]) variables.push_back(v);
  T atom_constraint = mkConstraintNode(
      Kind::GEQ, variables, std::vector<int>(variables.size(), 1), 1, nm);
  Trace("bv-pb") << "theory::bv::pb::NegatedEqPb resulted in constraint "
                 << atom_constraint << "\n";

  std::unordered_set<T> constraints;
  constraints.emplace(atom_constraint);
  for (const T& c : blasted_xor[1]) constraints.emplace(c);
  return mkAtomNode(constraints, nm);
}

template <class T>
T NegatedUltPb(T atom, TPseudoBooleanBlaster<T>* pbb)
{
  Assert(atom.getKind() == Kind::BITVECTOR_ULT);
  Trace("bv-pb") << "theory::bv::pb::NegatedUltPb " << atom << "\n    "
                 << "is equivalent to DefaultUgePb\n";
  T equivalent_atom =
      pbb->getNodeManager()->mkNode(Kind::BITVECTOR_UGE, atom[0], atom[1]);
  return DefaultUgePb(equivalent_atom, pbb);
}

template <class T>
T NegatedUlePb(T atom, TPseudoBooleanBlaster<T>* pbb)
{
  Assert(atom.getKind() == Kind::BITVECTOR_ULE);
  Trace("bv-pb") << "theory::bv::pb::NegatedUlePb " << atom << "\n    "
                 << "is equivalent to DefaultUgtPb\n";
  T equivalent_atom =
      pbb->getNodeManager()->mkNode(Kind::BITVECTOR_UGT, atom[0], atom[1]);
  return DefaultUgtPb(equivalent_atom, pbb);
}

template <class T>
T NegatedUgtPb(T atom, TPseudoBooleanBlaster<T>* pbb)
{
  Assert(atom.getKind() == Kind::BITVECTOR_UGT);
  Trace("bv-pb") << "theory::bv::pb::NegatedUgtPb " << atom << "\n    "
                 << "is equivalent to DefaultUlePb\n";
  T equivalent_atom =
      pbb->getNodeManager()->mkNode(Kind::BITVECTOR_ULE, atom[0], atom[1]);
  return DefaultUlePb(equivalent_atom, pbb);
}

template <class T>
T NegatedUgePb(T atom, TPseudoBooleanBlaster<T>* pbb)
{
  Assert(atom.getKind() == Kind::BITVECTOR_UGE);
  Trace("bv-pb") << "theory::bv::pb::NegatedUgePb " << atom << "\n    "
                 << "is equivalent to DefaultUltPb\n";
  T equivalent_atom =
      pbb->getNodeManager()->mkNode(Kind::BITVECTOR_ULT, atom[0], atom[1]);
  return DefaultUltPb(equivalent_atom, pbb);
}

template <class T>
T NegatedSltPb(T atom, TPseudoBooleanBlaster<T>* pbb)
{
  Assert(atom.getKind() == Kind::BITVECTOR_SLT);
  Trace("bv-pb") << "theory::bv::pb::NegatedSltPb " << atom << "\n    "
                 << "is equivalent to DefaultSgePb\n";
  T equivalent_atom =
      pbb->getNodeManager()->mkNode(Kind::BITVECTOR_SGE, atom[0], atom[1]);
  return DefaultSgePb(equivalent_atom, pbb);
}

template <class T>
T NegatedSlePb(T atom, TPseudoBooleanBlaster<T>* pbb)
{
  Assert(atom.getKind() == Kind::BITVECTOR_SLE);
  Trace("bv-pb") << "theory::bv::pb::NegatedSlePb " << atom << "\n    "
                 << "is equivalent to DefaultSgtPb\n";
  T equivalent_atom =
      pbb->getNodeManager()->mkNode(Kind::BITVECTOR_SGT, atom[0], atom[1]);
  return DefaultSgtPb(equivalent_atom, pbb);
}

template <class T>
T NegatedSgtPb(T atom, TPseudoBooleanBlaster<T>* pbb)
{
  Assert(atom.getKind() == Kind::BITVECTOR_SGT);
  Trace("bv-pb") << "theory::bv::pb::NegatedSgtPb " << atom << "\n    "
                 << "is equivalent to DefaultSlePb\n";
  T equivalent_atom =
      pbb->getNodeManager()->mkNode(Kind::BITVECTOR_SLE, atom[0], atom[1]);
  return DefaultSlePb(equivalent_atom, pbb);
}

template <class T>
T NegatedSgePb(T atom, TPseudoBooleanBlaster<T>* pbb)
{
  Assert(atom.getKind() == Kind::BITVECTOR_SGE);
  Trace("bv-pb") << "theory::bv::pb::NegatedSgePb " << atom << "\n    "
                 << "is equivalent to DefaultSltPb\n";
  T equivalent_atom =
      pbb->getNodeManager()->mkNode(Kind::BITVECTOR_SLT, atom[0], atom[1]);
  return DefaultSltPb(equivalent_atom, pbb);
}

/*
 * Default Term PB-Blasting strategies
 *
 * @param node the term to be bitblasted
 * @param sp [output parameter] pair representing the variables and constraints
 *                              generated by the blasting process
 * @param pbb the bitblaster in which the clauses are added
 */

template <class T>
T UndefinedTermPbStrategy(T node, CVC5_UNUSED TPseudoBooleanBlaster<T>* pbb)
{
  Trace("bv-pb") << "Undefined PB-blasting strategy for term of kind: "
                 << node.getKind() << "\n";
  Unreachable();
}

template <class T>
T DefaultVarPb(T term, TPseudoBooleanBlaster<T>* pbb)
{
  Trace("bv-pb") << "theory::bv::pb::DefaultVarPb blasting " << term;
  T variables = pbb->newVariable(utils::getSize(term));
  Trace("bv-pb") << " with bits " << variables << "\n";
  return mkTermNode(variables, std::vector<T>(), pbb->getNodeManager());
}

/** TODO: consider adding word-level constraints? */
/** TODO: Change `term` type to T */
template <class T>
T DefaultConstPb(Node term, TPseudoBooleanBlaster<T>* pbb)
{
  Trace("bv-pb") << "theory::bv::pb::DefaultConstPb blasting " << term << " ";
  Assert(term.getKind() == Kind::CONST_BITVECTOR);

  NodeManager* nm = pbb->getNodeManager();
  unsigned size = utils::getSize(term);
  T variables = pbb->newVariable(size);
  Trace("bv-pb") << "with bits " << variables << "\n";

  std::vector<T> constraints;
  for (unsigned i = 0; i < size; i++)
  {
    Integer bit_value = term.getConst<BitVector>().extract(i, i).getValue();
    T rhs = bit_value == Integer(0) ? pbb->d_ZERO : pbb->d_ONE;
    constraints.push_back(
        mkConstraintNode(Kind::EQUAL, {variables[i]}, {pbb->d_ONE}, rhs, nm));
  }
  return mkTermNode(variables, constraints, nm);
}

template <class T>
T DefaultXorPb(T term, TPseudoBooleanBlaster<T>* pbb)
{
  Trace("bv-pb") << "theory::bv::pb::DefaultXorPb blasting " << term;
  Assert(term.getKind() == Kind::BITVECTOR_XOR);
  if (term.getNumChildren() < 2) Unreachable();

  NodeManager* nm = pbb->getNodeManager();
  unsigned num_bits = utils::getSize(term);
  T variables = pbb->newVariable(num_bits);
  Trace("bv-pb") << " with bits " << variables << "\n";

  T lhs = pbb->blastTerm(term[0]);
  Assert(lhs[0].getNumChildren() == num_bits);

  T rhs;
  if (term.getNumChildren() > 2)
  {
    std::vector<T> rhs_nodes;
    for (unsigned i = 1; i < term.getNumChildren(); i++)
    {
      rhs_nodes.push_back(term[i]);
    }
    T rhs_xor = nm->mkNode(Kind::BITVECTOR_XOR, rhs_nodes);
    rhs = pbb->blastTerm(rhs_xor);
  }
  else
  {
    rhs = pbb->blastTerm(term[1]);
  }

  Assert(lhs[0].getNumChildren() == rhs[0].getNumChildren());

  std::unordered_set<T> constraints;
  for (unsigned i = 0; i < num_bits; i++)
  {
    for (const T& c : mkPbXor(lhs[0][i], rhs[0][i], variables[i], nm))
      constraints.emplace(c);
  }

  for (const T& c : lhs[1]) constraints.insert(c);
  for (const T& c : rhs[1]) constraints.insert(c);

  T blasted_term = mkTermNode(variables, constraints, nm);
  Assert(blasted_term[0].getNumChildren() == num_bits);
  Trace("bv-pb") << "theory::bv::pb::DefaultXorPb done\n";
  return blasted_term;
}

template <class T>
T DefaultXnorPb(T term, TPseudoBooleanBlaster<T>* pbb)
{
  Trace("bv-pb") << "theory::bv::pb::DefaultXnorPb blasting " << term;
  Assert(term.getKind() == Kind::BITVECTOR_XNOR);
  if (term.getNumChildren() < 2) Unreachable();

  NodeManager* nm = pbb->getNodeManager();
  std::vector<T> children;
  for (unsigned i = 1; i < term.getNumChildren(); i++)
  {
    children.push_back(term[i]);
  }

  T rewritten_node = nm->mkNode(Kind::BITVECTOR_NOT,
                                nm->mkNode(Kind::BITVECTOR_XOR, children));
  return pbb->blastTerm(rewritten_node);
}

template <class T>
T DefaultAddPb(T term, TPseudoBooleanBlaster<T>* pbb)
{
  Trace("bv-pb") << "theory::bv::pb::DefaultAddPb blasting " << term;
  Assert(term.getKind() == Kind::BITVECTOR_ADD);

  NodeManager* nm = pbb->getNodeManager();
  unsigned num_bits = utils::getSize(term);
  T term_vars = pbb->newVariable(num_bits);
  Trace("bv-pb") << " with bits " << term_vars << "\n";

  std::vector<Node> variables, coefficients;
  std::unordered_set<Node> constraints;

  std::vector<Node> aux = bvToUnsigned(num_bits, nm);
  for (unsigned i = 0; i < term.getNumChildren(); i++)
  {
    T blasted = pbb->blastTerm(term[i]);
    Assert(blasted[0].getNumChildren() == num_bits);
    for (const T& v : blasted[0]) variables.push_back(v);
    std::copy(aux.begin(), aux.end(), std::back_inserter(coefficients));
    for (const T& c : blasted[1]) constraints.insert(c);
  }

  /** extra_bits used to store possible overflow */
  int extra_bits = ceilLog2(term.getNumChildren());
  T extra_vars = pbb->newVariable(extra_bits);
  for (const T& v : term_vars) variables.push_back(v);
  for (const T& v : extra_vars) variables.push_back(v);

  aux = bvToUnsigned(num_bits + extra_bits, nm, -1);
  std::move(aux.begin(), aux.end(), std::back_inserter(coefficients));
  constraints.insert(
      mkConstraintNode(Kind::EQUAL, variables, coefficients, pbb->d_ZERO, nm));

  T blasted_term = mkTermNode(term_vars, constraints, nm);
  Assert(blasted_term[0].getNumChildren() == utils::getSize(term));
  Trace("bv-pb") << "theory::bv::pb::DefaultAddPb done\n";
  return blasted_term;
}

template <class T>
T DefaultAndPb(T term, TPseudoBooleanBlaster<T>* pbb)
{
  Trace("bv-pb") << "theory::bv::pb::DefaultAndPb blasting " << term;
  Assert(term.getKind() == Kind::BITVECTOR_AND);

  NodeManager* nm = pbb->getNodeManager();
  unsigned num_bits = utils::getSize(term);
  T result_vars = pbb->newVariable(num_bits);
  Trace("bv-pb") << " with bits " << result_vars << "\n";

  std::vector<std::vector<Node>> variables(num_bits);
  std::vector<std::vector<int>> coefficients(num_bits);
  std::unordered_set<Node> constraints;

  unsigned n = term.getNumChildren();
  for (unsigned j = 0; j < n; j++)
  {
    T blasted = pbb->blastTerm(term[j]);
    Assert(blasted[0].getNumChildren() == num_bits);
    for (const T& c : blasted[1]) constraints.insert(c);

    for (unsigned i = 0; i < num_bits; i++)
    {
      std::vector<Node> unit_constraint = {blasted[0][i], result_vars[i]};
      constraints.insert(
          mkConstraintNode(Kind::GEQ, unit_constraint, {1, -1}, 0, nm));
      variables[i].push_back(blasted[0][i]);
      coefficients[i].push_back(-1);
    }
  }

  for (unsigned i = 0; i < num_bits; i++)
  {
    variables[i].push_back(result_vars[i]);
    coefficients[i].push_back(1);
    constraints.insert(
        mkConstraintNode(Kind::GEQ, variables[i], coefficients[i], 1 - n, nm));
  }

  T blasted_term = mkTermNode(result_vars, constraints, nm);
  Assert(blasted_term[0].getNumChildren() == utils::getSize(term));
  Trace("bv-pb") << "theory::bv::pb::DefaultAndPb done\n";
  return blasted_term;
}

template <class T>
T DefaultOrPb(T term, TPseudoBooleanBlaster<T>* pbb)
{
  Trace("bv-pb") << "theory::bv::pb::DefaultOrPb blasting " << term;
  Assert(term.getKind() == Kind::BITVECTOR_OR);

  NodeManager* nm = pbb->getNodeManager();
  unsigned num_bits = utils::getSize(term);
  T result_vars = pbb->newVariable(num_bits);
  Trace("bv-pb") << " with bits " << result_vars << "\n";

  std::vector<std::vector<Node>> variables(num_bits);
  std::vector<std::vector<int>> coefficients(num_bits);
  std::unordered_set<Node> constraints;

  unsigned n = term.getNumChildren();
  for (unsigned j = 0; j < n; j++)
  {
    T blasted = pbb->blastTerm(term[j]);
    Assert(blasted[0].getNumChildren() == num_bits);
    for (const T& c : blasted[1]) constraints.insert(c);

    for (unsigned i = 0; i < num_bits; i++)
    {
      std::vector<Node> unit_constraint = {blasted[0][i], result_vars[i]};
      constraints.insert(
          mkConstraintNode(Kind::GEQ, unit_constraint, {-1, 1}, 0, nm));
      variables[i].push_back(blasted[0][i]);
      coefficients[i].push_back(1);
    }
  }

  for (unsigned i = 0; i < num_bits; i++)
  {
    variables[i].push_back(result_vars[i]);
    coefficients[i].push_back(-1);
    constraints.insert(
        mkConstraintNode(Kind::GEQ, variables[i], coefficients[i], 0, nm));
  }

  T blasted_term = mkTermNode(result_vars, constraints, nm);
  Assert(blasted_term[0].getNumChildren() == utils::getSize(term));
  Trace("bv-pb") << "theory::bv::pb::DefaultOrPb done\n";
  return blasted_term;
}

template <class T>
T DefaultNotPb(T term, TPseudoBooleanBlaster<T>* pbb)
{
  Trace("bv-pb") << "theory::bv::pb::DefaultNotPb blasting " << term;
  Assert(term.getKind() == Kind::BITVECTOR_NOT);
  Assert(term.getNumChildren() == 1);

  NodeManager* nm = pbb->getNodeManager();
  unsigned num_bits = utils::getSize(term);
  T result_vars = pbb->newVariable(num_bits);
  Trace("bv-pb") << " with bits " << result_vars << "\n";

  std::unordered_set<Node> constraints;

  T blasted = pbb->blastTerm(term[0]);
  Assert(blasted[0].getNumChildren() == num_bits);
  for (const T& c : blasted[1]) constraints.insert(c);

  for (unsigned i = 0; i < num_bits; i++)
  {
    std::vector<Node> unit_constraint = {blasted[0][i], result_vars[i]};
    constraints.insert(
        mkConstraintNode(Kind::EQUAL, unit_constraint, {1, 1}, 1, nm));
  }

  T blasted_term = mkTermNode(result_vars, constraints, nm);
  Assert(blasted_term[0].getNumChildren() == utils::getSize(term));
  Trace("bv-pb") << "theory::bv::pb::DefaultNotPb done\n";
  return blasted_term;
}

template <class T>
T DefaultMultPb(T term, TPseudoBooleanBlaster<T>* pbb)
{
  Trace("bv-pb") << "theory::bv::pb::DefaultMultPb blasting " << term;
  Assert(term.getKind() == Kind::BITVECTOR_MULT);
  if (term.getNumChildren() != 2) Unreachable();

  NodeManager* nm = pbb->getNodeManager();
  unsigned num_bits = utils::getSize(term);

  T term_vars = pbb->newVariable(num_bits);
  T tableau = pbb->newVariable(num_bits * num_bits);
  Trace("bv-pb") << " with bits " << term_vars << "\n";

  T lhs = pbb->blastTerm(term[0]);
  T rhs = pbb->blastTerm(term[1]);
  Assert(lhs[0].getNumChildren() == rhs[0].getNumChildren());
  Assert(num_bits == rhs[0].getNumChildren());

  std::unordered_set<Node> constraints;
  for (unsigned i = 0; i < num_bits; i++)
  {
    for (unsigned j = 0; j < num_bits; j++)
    {
      std::vector<Node> and_constraint = {
          lhs[0][i], rhs[0][j], tableau[i * num_bits + j]};
      constraints.insert(
          mkConstraintNode(Kind::GEQ, and_constraint, {1, 1, -2}, 0, nm));
      constraints.insert(
          mkConstraintNode(Kind::GEQ, and_constraint, {-1, -1, 1}, -1, nm));
    }
  }

  std::vector<Node> variables;
  std::vector<Node> coefficients;
  for (unsigned i = 0; i < num_bits; i++)
  {
    for (unsigned j = 0; j < num_bits; j++)
    {
      variables.push_back(tableau[i * num_bits + j]);
      coefficients.push_back(
          nm->mkConstInt(Rational(Integer(1).multiplyByPow2(i + j))));
    }
  }

  T extra_vars = pbb->newVariable(num_bits);
  Trace("bv-pb-mult") << term_vars << "\n";
  Trace("bv-pb-mult") << extra_vars << "\n";
  for (const T& v : term_vars) variables.push_back(v);
  for (const T& v : extra_vars) variables.push_back(v);
  for (const T& c : bvToUnsigned(2 * num_bits, nm, -1))
  {
    coefficients.push_back(c);
  }

  Trace("bv-pb-mult") << variables << "\n";
  Trace("bv-pb-mult") << coefficients << "\n";
  // Trace("bv-pb-mult") << mkLongConstraintNode(Kind::EQUAL, variables,
  // coefficients, 0, nm) << "\n";

  for (const T& c : lhs[1]) constraints.insert(c);
  for (const T& c : rhs[1]) constraints.insert(c);

  constraints.insert(
      mkConstraintNode(Kind::EQUAL, variables, coefficients, pbb->d_ZERO, nm));

  T blasted_term = mkTermNode(term_vars, constraints, nm);
  Assert(blasted_term[0].getNumChildren() == utils::getSize(term));
  Trace("bv-pb") << "theory::bv::pb::DefaultMultPb done\n";
  return blasted_term;
}

template <class T>
T DefaultConcatPb(T term, TPseudoBooleanBlaster<T>* pbb)
{
  Trace("bv-pb") << "theory::bv::pb::DefaultConcatPb blasting " << term;
  Assert(term.getKind() == Kind::BITVECTOR_CONCAT);

  NodeManager* nm = pbb->getNodeManager();
  unsigned num_bits = utils::getSize(term);
  T result_vars = pbb->newVariable(num_bits);
  Trace("bv-pb") << " with bits " << result_vars << "\n";
  std::unordered_set<Node> constraints;

  unsigned result_index = 0;
  for (unsigned i = 0; i < term.getNumChildren(); i++)
  {
    Assert(result_index < num_bits);
    T blasted_subterm = pbb->blastTerm(term[term.getNumChildren() - i - 1]);
    for (const T& c : blasted_subterm[1]) constraints.insert(c);
    for (unsigned j = 0; j < blasted_subterm[0].getNumChildren(); j++)
    {
      std::vector<Node> vars = {blasted_subterm[0][j],
                                result_vars[result_index]};
      constraints.insert(mkConstraintNode(Kind::EQUAL, vars, {1, -1}, 0, nm));
      result_index++;
    }
  }
  Assert(result_index == num_bits);

  T blasted_term = mkTermNode(result_vars, constraints, nm);
  Assert(blasted_term[0].getNumChildren() == num_bits);
  Trace("bv-pb") << "theory::bv::pb::DefaultConcatPb done\n";
  return blasted_term;
}

template <class T>
T DefaultExtractPb(T term, TPseudoBooleanBlaster<T>* pbb)
{
  Trace("bv-pb") << "theory::bv::pb::DefaultExtractPb blasting " << term;
  Assert(term.getKind() == Kind::BITVECTOR_EXTRACT);
  Assert(term.getNumChildren() == 1);

  uint32_t high = utils::getExtractHigh(term);
  uint32_t low = utils::getExtractLow(term);
  uint32_t num_bits = high - low + 1;
  Assert(num_bits == utils::getSize(term));

  NodeManager* nm = pbb->getNodeManager();
  // TODO(alanctprado): instead of creating new variables, return the
  // corresponding variables from 'blasted'.
  T result_vars = pbb->newVariable(num_bits);
  Trace("bv-pb") << " with bits " << result_vars << "\n";

  T blasted = pbb->blastTerm(term[0]);
  std::unordered_set<Node> constraints;
  for (const T& c : blasted[1]) constraints.insert(c);

  for (uint32_t i = low; i <= high; i++)
  {
    std::vector<Node> vars = {blasted[0][i], result_vars[i - low]};
    constraints.insert(mkConstraintNode(Kind::EQUAL, vars, {1, -1}, 0, nm));
  }

  T blasted_term = mkTermNode(result_vars, constraints, nm);
  Assert(blasted_term[0].getNumChildren() == num_bits);
  Trace("bv-pb") << "theory::bv::pb::DefaultExtractPb done\n";
  return blasted_term;
}

template <class T>
T DefaultNegPb(T term, TPseudoBooleanBlaster<T>* pbb)
{
  Trace("bv-pb") << "theory::bv::pb::DefaultNegPb blasting " << term;
  Assert(term.getKind() == Kind::BITVECTOR_NEG);

  NodeManager* nm = pbb->getNodeManager();
  unsigned num_bits = utils::getSize(term);
  T result_vars = pbb->newVariable(num_bits);
  Trace("bv-pb") << " with bits " << result_vars << "\n";

  T blasted = pbb->blastTerm(term[0]);
  std::unordered_set<Node> constraints;
  for (const T& c : blasted[1]) constraints.insert(c);

  std::vector<Node> variables;
  std::vector<Node> coefficients;
  std::vector<Node> aux_coefficients = bvToUnsigned(num_bits, nm);
  for (unsigned i = 0; i < num_bits; i++)
  {
    variables.push_back(result_vars[i]);
    variables.push_back(blasted[0][i]);
    coefficients.push_back(aux_coefficients[i]);
    coefficients.push_back(aux_coefficients[i]);
  }

  T rhs = nm->mkConstInt(Rational(Integer(1).multiplyByPow2(num_bits)));

  constraints.insert(
      mkConstraintNode(Kind::EQUAL, variables, coefficients, rhs, nm));

  T blasted_term = mkTermNode(result_vars, constraints, nm);
  Assert(blasted_term[0].getNumChildren() == utils::getSize(term));
  Trace("bv-pb") << "theory::bv::pb::DefaultNegPb done\n";
  return blasted_term;
}

template <class T>
T DefaultSubPb(T term, TPseudoBooleanBlaster<T>* pbb)
{
  Trace("bv-pb") << "theory::bv::pb::DefaultSubPb blasting " << term;
  Assert(term.getKind() == Kind::BITVECTOR_SUB);
  if (term.getNumChildren() != 2) Unreachable();

  NodeManager* nm = pbb->getNodeManager();
  T rewritten_node = nm->mkNode(
      Kind::BITVECTOR_ADD, term[0], nm->mkNode(Kind::BITVECTOR_NEG, term[1]));
  return pbb->blastTerm(rewritten_node);
}

template <class T>
T DefaultShlPb(T term, TPseudoBooleanBlaster<T>* pbb)
{
  Trace("bv-pb") << "theory::bv::pb::DefaultShlPb blasting " << term;
  Assert(term.getKind() == Kind::BITVECTOR_SHL);

  NodeManager* nm = pbb->getNodeManager();
  unsigned num_bits = utils::getSize(term);
  T result_vars = pbb->newVariable(num_bits);
  Trace("bv-pb") << " with bits " << result_vars << "\n";

  T a = pbb->blastTerm(term[0]);
  T b = pbb->blastTerm(term[1]);

  unsigned log2_size = ceilLog2(num_bits);
  std::unordered_set<Node> constraints;
  T prev_z = a[0];
  // we only need to look at the bits bellow log2_size
  for (unsigned j = 0; j < log2_size; j++)
  {
    unsigned threshold = pow(2, j);
    T z = pbb->newVariable(num_bits);

    for (unsigned i = 0; i < num_bits; i++)
    {
      constraints.insert(
          mkConstraintNode(Kind::GEQ,
                           std::vector<Node>{z[i], prev_z[i], b[0][j]},
                           {1, -1, 1},
                           0,
                           nm));
      constraints.insert(
          mkConstraintNode(Kind::GEQ,
                           std::vector<Node>{z[i], prev_z[i], b[0][j]},
                           {-1, 1, 1},
                           0,
                           nm));
      if (i < threshold)
      {
        constraints.insert(mkConstraintNode(
            Kind::GEQ, std::vector<Node>{z[i], b[0][j]}, {-1, -1}, -1, nm));
      }
      else
      {
        constraints.insert(mkConstraintNode(
            Kind::GEQ,
            std::vector<Node>{z[i], prev_z[i - threshold], b[0][j]},
            {1, -1, -1},
            -1,
            nm));
        constraints.insert(mkConstraintNode(
            Kind::GEQ,
            std::vector<Node>{z[i], prev_z[i - threshold], b[0][j]},
            {-1, 1, -1},
            -1,
            nm));
      }
    }

    prev_z = z;
  }

  // -r - y >= -1
  for (unsigned i = 0; i < num_bits; i++)
  {
    for (unsigned j = log2_size; j < num_bits; j++)
    {
      std::vector<Node> unit_constraint = {result_vars[i], b[0][j]};
      constraints.insert(
          mkConstraintNode(Kind::GEQ, unit_constraint, {-1, -1}, -1, nm));
    }
  }

  std::vector<T> variables;
  std::vector<int> coefficients;
  for (unsigned j = log2_size; j < num_bits; j++)
  {
    variables.push_back(b[0][j]);
    coefficients.push_back(1);
  }
  coefficients.push_back(1);
  coefficients.push_back(-1);

  for (unsigned i = 0; i < num_bits; i++)
  {
    // -r + z >= 0
    std::vector<Node> unit_constraint = {result_vars[i], prev_z[i]};
    constraints.insert(
        mkConstraintNode(Kind::GEQ, unit_constraint, {-1, 1}, 0, nm));

    // r + sum(y) - z >= 0
    variables.push_back(result_vars[i]);
    variables.push_back(prev_z[i]);

    constraints.insert(
        mkConstraintNode(Kind::GEQ, variables, coefficients, 0, nm));

    variables.pop_back();
    variables.pop_back();
  }

  for (const T& c : a[1]) constraints.insert(c);
  for (const T& c : b[1]) constraints.insert(c);

  T blasted_term = mkTermNode(result_vars, constraints, nm);
  Assert(blasted_term[0].getNumChildren() == utils::getSize(term));
  Trace("bv-pb") << "theory::bv::pb::DefaultShlPb done\n";
  return blasted_term;
}

template <class T>
T DefaultLshrPb(T term, TPseudoBooleanBlaster<T>* pbb)
{
  Trace("bv-pb") << "theory::bv::pb::DefaultLshrPb blasting " << term;
  Assert(term.getKind() == Kind::BITVECTOR_LSHR);

  NodeManager* nm = pbb->getNodeManager();
  unsigned num_bits = utils::getSize(term);
  T result_vars = pbb->newVariable(num_bits);
  Trace("bv-pb") << " with bits " << result_vars << "\n";

  T a = pbb->blastTerm(term[0]);
  T b = pbb->blastTerm(term[1]);

  unsigned log2_size = ceilLog2(num_bits);
  std::unordered_set<Node> constraints;
  T prev_z = a[0];
  // we only need to look at the bits bellow log2_size
  for (unsigned j = 0; j < log2_size; j++)
  {
    unsigned threshold = pow(2, j);
    T z = pbb->newVariable(num_bits);

    for (unsigned i = 0; i < num_bits; i++)
    {
      constraints.insert(
          mkConstraintNode(Kind::GEQ,
                           std::vector<Node>{z[i], prev_z[i], b[0][j]},
                           {1, -1, 1},
                           0,
                           nm));
      constraints.insert(
          mkConstraintNode(Kind::GEQ,
                           std::vector<Node>{z[i], prev_z[i], b[0][j]},
                           {-1, 1, 1},
                           0,
                           nm));
      if (i + threshold >= num_bits)
      {
        constraints.insert(mkConstraintNode(
            Kind::GEQ, std::vector<Node>{z[i], b[0][j]}, {-1, -1}, -1, nm));
      }
      else
      {
        constraints.insert(mkConstraintNode(
            Kind::GEQ,
            std::vector<Node>{z[i], prev_z[i + threshold], b[0][j]},
            {1, -1, -1},
            -1,
            nm));
        constraints.insert(mkConstraintNode(
            Kind::GEQ,
            std::vector<Node>{z[i], prev_z[i + threshold], b[0][j]},
            {-1, 1, -1},
            -1,
            nm));
      }
    }

    prev_z = z;
  }

  // -r - y >= -1
  for (unsigned i = 0; i < num_bits; i++)
  {
    for (unsigned j = log2_size; j < num_bits; j++)
    {
      std::vector<Node> unit_constraint = {result_vars[i], b[0][j]};
      constraints.insert(
          mkConstraintNode(Kind::GEQ, unit_constraint, {-1, -1}, -1, nm));
    }
  }

  std::vector<T> variables;
  std::vector<int> coefficients;
  for (unsigned j = log2_size; j < num_bits; j++)
  {
    variables.push_back(b[0][j]);
    coefficients.push_back(1);
  }
  coefficients.push_back(1);
  coefficients.push_back(-1);

  for (unsigned i = 0; i < num_bits; i++)
  {
    // -r + z >= 0
    std::vector<Node> unit_constraint = {result_vars[i], prev_z[i]};
    constraints.insert(
        mkConstraintNode(Kind::GEQ, unit_constraint, {-1, 1}, 0, nm));

    // r + sum(y) - z >= 0
    variables.push_back(result_vars[i]);
    variables.push_back(prev_z[i]);

    constraints.insert(
        mkConstraintNode(Kind::GEQ, variables, coefficients, 0, nm));

    variables.pop_back();
    variables.pop_back();
  }

  for (const T& c : a[1]) constraints.insert(c);
  for (const T& c : b[1]) constraints.insert(c);

  T blasted_term = mkTermNode(result_vars, constraints, nm);
  Assert(blasted_term[0].getNumChildren() == utils::getSize(term));
  Trace("bv-pb") << "theory::bv::pb::DefaultLshrPb done\n";
  return blasted_term;
}

template <class T>
T DefaultAshrPb(T term, TPseudoBooleanBlaster<T>* pbb)
{
  Trace("bv-pb") << "theory::bv::pb::DefaultAshrPb blasting " << term;
  Assert(term.getKind() == Kind::BITVECTOR_ASHR);

  NodeManager* nm = pbb->getNodeManager();
  unsigned num_bits = utils::getSize(term);
  T result_vars = pbb->newVariable(num_bits);
  Trace("bv-pb") << " with bits " << result_vars << "\n";

  T a = pbb->blastTerm(term[0]);
  T b = pbb->blastTerm(term[1]);

  unsigned log2_size = ceilLog2(num_bits);
  std::unordered_set<Node> constraints;
  T prev_z = a[0];
  T sign_bit = a[0][num_bits - 1];
  // we only need to look at the bits bellow log2_size
  for (unsigned j = 0; j < log2_size; j++)
  {
    unsigned threshold = pow(2, j);
    T z = pbb->newVariable(num_bits);

    for (unsigned i = 0; i < num_bits; i++)
    {
      constraints.insert(
          mkConstraintNode(Kind::GEQ,
                           std::vector<Node>{z[i], prev_z[i], b[0][j]},
                           {1, -1, 1},
                           0,
                           nm));
      constraints.insert(
          mkConstraintNode(Kind::GEQ,
                           std::vector<Node>{z[i], prev_z[i], b[0][j]},
                           {-1, 1, 1},
                           0,
                           nm));
      if (i + threshold >= num_bits)
      {
        constraints.insert(
            mkConstraintNode(Kind::GEQ,
                             std::vector<Node>{z[i], sign_bit, b[0][j]},
                             {-1, 1, -1},
                             -1,
                             nm));
        constraints.insert(
            mkConstraintNode(Kind::GEQ,
                             std::vector<Node>{z[i], sign_bit, b[0][j]},
                             {1, -1, -1},
                             -1,
                             nm));
      }
      else
      {
        constraints.insert(mkConstraintNode(
            Kind::GEQ,
            std::vector<Node>{z[i], prev_z[i + threshold], b[0][j]},
            {1, -1, -1},
            -1,
            nm));
        constraints.insert(mkConstraintNode(
            Kind::GEQ,
            std::vector<Node>{z[i], prev_z[i + threshold], b[0][j]},
            {-1, 1, -1},
            -1,
            nm));
      }
    }

    prev_z = z;
  }

  // -r + sign_bit - y >= -1
  // r - sign_bit - y >= -1
  for (unsigned i = 0; i < num_bits; i++)
  {
    for (unsigned j = log2_size; j < num_bits; j++)
    {
      std::vector<Node> unit_constraint = {result_vars[i], sign_bit, b[0][j]};
      constraints.insert(
          mkConstraintNode(Kind::GEQ, unit_constraint, {-1, 1, -1}, -1, nm));
      constraints.insert(
          mkConstraintNode(Kind::GEQ, unit_constraint, {1, -1, -1}, -1, nm));
    }
  }

  std::vector<T> variables;
  std::vector<int> coefficients;
  for (unsigned j = log2_size; j < num_bits; j++)
  {
    variables.push_back(b[0][j]);
    coefficients.push_back(1);
  }
  coefficients.push_back(1);
  coefficients.push_back(-1);

  for (unsigned i = 0; i < num_bits; i++)
  {
    // -r + z >= 0
    std::vector<Node> unit_constraint = {result_vars[i], prev_z[i]};
    constraints.insert(
        mkConstraintNode(Kind::GEQ, unit_constraint, {-1, 1}, 0, nm));

    // r + sum(y) - z >= 0
    variables.push_back(result_vars[i]);
    variables.push_back(prev_z[i]);

    constraints.insert(
        mkConstraintNode(Kind::GEQ, variables, coefficients, 0, nm));

    variables.pop_back();
    variables.pop_back();
  }

  for (const T& c : a[1]) constraints.insert(c);
  for (const T& c : b[1]) constraints.insert(c);

  T blasted_term = mkTermNode(result_vars, constraints, nm);
  Assert(blasted_term[0].getNumChildren() == utils::getSize(term));
  Trace("bv-pb") << "theory::bv::pb::DefaultAshrPb done\n";
  return blasted_term;
}

template <class T>
T DefaultUdivPb(T term, TPseudoBooleanBlaster<T>* pbb)
{
  Trace("bv-pb") << "theory::bv::pb::DefaultUdivPb blasting " << term;
  Assert(term.getKind() == Kind::BITVECTOR_UDIV);

  NodeManager* nm = pbb->getNodeManager();
  unsigned num_bits = utils::getSize(term);
  T result_vars = pbb->newVariable(num_bits);
  Trace("bv-pb") << " with bits " << result_vars << "\n";

  T a = pbb->blastTerm(term[0]);
  T b = pbb->blastTerm(term[1]);

  T quot = pbb->newVariable(num_bits);
  T rem = pbb->newVariable(num_bits);
  std::unordered_set<Node> constraints;
  // a = b*quot + rem
  T tableau = pbb->newVariable(num_bits * num_bits);

  for (unsigned i = 0; i < num_bits; i++)
  {
    for (unsigned j = 0; j < num_bits; j++)
    {
      std::vector<Node> and_constraint = {
          b[0][i], quot[j], tableau[i * num_bits + j]};
      constraints.insert(
          mkConstraintNode(Kind::GEQ, and_constraint, {1, 1, -2}, 0, nm));
      constraints.insert(
          mkConstraintNode(Kind::GEQ, and_constraint, {-1, -1, 1}, -1, nm));
    }
  }

  std::vector<Node> variables;
  std::vector<Node> coefficients;
  for (unsigned i = 0; i < num_bits; i++)
  {
    for (unsigned j = 0; j < num_bits; j++)
    {
      variables.push_back(tableau[i * num_bits + j]);
      coefficients.push_back(
          nm->mkConstInt(Rational(Integer(1).multiplyByPow2(i + j))));
    }
  }

  for (const T& v : rem) variables.push_back(v);
  for (const T& c : bvToUnsigned(num_bits, nm))
  {
    coefficients.push_back(c);
  }

  for (const T& v : a[0]) variables.push_back(v);
  for (const T& c : bvToUnsigned(num_bits, nm, -1))
  {
    coefficients.push_back(c);
  }

  // sum(2^i*t) + sum(2^i*rem) - sum(2^i*a) = 0
  constraints.insert(
      mkConstraintNode(Kind::EQUAL, variables, coefficients, pbb->d_ZERO, nm));

  // rem < b -> sum(2^i*b) - sum(2^i*rem) >= 1
  std::vector<Node> ult_variables;
  std::vector<Node> ult_coefficients;
  for (const T& v : b[0]) ult_variables.push_back(v);
  for (const T& v : rem) ult_variables.push_back(v);
  for (const T& c : bvToUnsigned(num_bits, nm))
  {
    ult_coefficients.push_back(c);
  }
  for (const T& c : bvToUnsigned(num_bits, nm, -1))
  {
    ult_coefficients.push_back(c);
  }

  constraints.insert(mkConstraintNode(
      Kind::GEQ, ult_variables, ult_coefficients, pbb->d_ONE, nm));

  // bitwise OR reduction of b
  T cond = pbb->newVariable(1);
  for (unsigned i = 0; i < num_bits; i++)
  {
    std::vector<Node> unit_constraint = {cond[0], b[0][i]};
    constraints.insert(
        mkConstraintNode(Kind::GEQ, unit_constraint, {1, -1}, 0, nm));
  }
  std::vector<Node> disjunction_vars;
  std::vector<int> disjunction_coef;
  for (unsigned i = 0; i < num_bits; i++)
  {
    disjunction_vars.push_back(b[0][i]);
    disjunction_coef.push_back(1);
  }
  disjunction_vars.push_back(cond[0]);
  disjunction_coef.push_back(-1);

  constraints.insert(
      mkConstraintNode(Kind::GEQ, disjunction_vars, disjunction_coef, 0, nm));

  // result_vars <-> cond ? quo : 11..11
  for (unsigned i = 0; i < num_bits; i++)
  {
    // -cond + quot - r >= -1
    constraints.insert(
        mkConstraintNode(Kind::GEQ,
                         std::vector<Node>{cond[0], quot[i], result_vars[i]},
                         {-1, 1, -1},
                         -1,
                         nm));
    // -cond - quot + r >= -1
    constraints.insert(
        mkConstraintNode(Kind::GEQ,
                         std::vector<Node>{cond[0], quot[i], result_vars[i]},
                         {-1, -1, 1},
                         -1,
                         nm));
    // cond + r >= 1
    constraints.insert(mkConstraintNode(
        Kind::GEQ, std::vector<Node>{cond[0], result_vars[i]}, {1, 1}, 1, nm));
  }

  for (const T& c : a[1]) constraints.insert(c);
  for (const T& c : b[1]) constraints.insert(c);

  T blasted_term = mkTermNode(result_vars, constraints, nm);
  Assert(blasted_term[0].getNumChildren() == utils::getSize(term));
  Trace("bv-pb") << "theory::bv::pb::DefaultUdivPb done\n";
  return blasted_term;
}

template <class T>
T DefaultUremPb(T term, TPseudoBooleanBlaster<T>* pbb)
{
  Trace("bv-pb") << "theory::bv::pb::DefaultUremPb blasting " << term;
  Assert(term.getKind() == Kind::BITVECTOR_UREM);

  NodeManager* nm = pbb->getNodeManager();
  unsigned num_bits = utils::getSize(term);
  T result_vars = pbb->newVariable(num_bits);
  Trace("bv-pb") << " with bits " << result_vars << "\n";

  T a = pbb->blastTerm(term[0]);
  T b = pbb->blastTerm(term[1]);

  T quot = pbb->newVariable(num_bits);
  T rem = pbb->newVariable(num_bits);
  std::unordered_set<Node> constraints;
  // a = b*quot + rem
  T tableau = pbb->newVariable(num_bits * num_bits);

  for (unsigned i = 0; i < num_bits; i++)
  {
    for (unsigned j = 0; j < num_bits; j++)
    {
      std::vector<Node> and_constraint = {
          b[0][i], quot[j], tableau[i * num_bits + j]};
      constraints.insert(
          mkConstraintNode(Kind::GEQ, and_constraint, {1, 1, -2}, 0, nm));
      constraints.insert(
          mkConstraintNode(Kind::GEQ, and_constraint, {-1, -1, 1}, -1, nm));
    }
  }

  std::vector<Node> variables;
  std::vector<Node> coefficients;
  for (unsigned i = 0; i < num_bits; i++)
  {
    for (unsigned j = 0; j < num_bits; j++)
    {
      variables.push_back(tableau[i * num_bits + j]);
      coefficients.push_back(
          nm->mkConstInt(Rational(Integer(1).multiplyByPow2(i + j))));
    }
  }

  for (const T& v : rem) variables.push_back(v);
  for (const T& c : bvToUnsigned(num_bits, nm))
  {
    coefficients.push_back(c);
  }

  for (const T& v : a[0]) variables.push_back(v);
  for (const T& c : bvToUnsigned(num_bits, nm, -1))
  {
    coefficients.push_back(c);
  }

  // sum(2^i*t) + sum(2^i*rem) - sum(2^i*a) = 0
  constraints.insert(
      mkConstraintNode(Kind::EQUAL, variables, coefficients, pbb->d_ZERO, nm));

  // rem < b -> sum(2^i*b) - sum(2^i*rem) >= 1
  std::vector<Node> ult_variables;
  std::vector<Node> ult_coefficients;
  for (const T& v : b[0]) ult_variables.push_back(v);
  for (const T& v : rem) ult_variables.push_back(v);
  for (const T& c : bvToUnsigned(num_bits, nm))
  {
    ult_coefficients.push_back(c);
  }
  for (const T& c : bvToUnsigned(num_bits, nm, -1))
  {
    ult_coefficients.push_back(c);
  }

  constraints.insert(mkConstraintNode(
      Kind::GEQ, ult_variables, ult_coefficients, pbb->d_ONE, nm));

  // bitwise OR reduction of b
  T cond = pbb->newVariable(1);
  for (unsigned i = 0; i < num_bits; i++)
  {
    std::vector<Node> unit_constraint = {cond[0], b[0][i]};
    constraints.insert(
        mkConstraintNode(Kind::GEQ, unit_constraint, {1, -1}, 0, nm));
  }
  std::vector<Node> disjunction_vars;
  std::vector<int> disjunction_coef;
  for (unsigned i = 0; i < num_bits; i++)
  {
    disjunction_vars.push_back(b[0][i]);
    disjunction_coef.push_back(1);
  }
  disjunction_vars.push_back(cond[0]);
  disjunction_coef.push_back(-1);

  constraints.insert(
      mkConstraintNode(Kind::GEQ, disjunction_vars, disjunction_coef, 0, nm));

  // result_vars <-> cond ? rem : a
  for (unsigned i = 0; i < num_bits; i++)
  {
    // -cond + rem - r >= -1
    constraints.insert(
        mkConstraintNode(Kind::GEQ,
                         std::vector<Node>{cond[0], rem[i], result_vars[i]},
                         {-1, 1, -1},
                         -1,
                         nm));
    // -cond - rem + r >= -1
    constraints.insert(
        mkConstraintNode(Kind::GEQ,
                         std::vector<Node>{cond[0], rem[i], result_vars[i]},
                         {-1, -1, 1},
                         -1,
                         nm));
    // cond + a - r >= 0
    constraints.insert(
        mkConstraintNode(Kind::GEQ,
                         std::vector<Node>{cond[0], a[0][i], result_vars[i]},
                         {1, 1, -1},
                         0,
                         nm));
    // cond - a + r >= 0
    constraints.insert(
        mkConstraintNode(Kind::GEQ,
                         std::vector<Node>{cond[0], a[0][i], result_vars[i]},
                         {1, -1, 1},
                         0,
                         nm));
  }

  for (const T& c : a[1]) constraints.insert(c);
  for (const T& c : b[1]) constraints.insert(c);

  T blasted_term = mkTermNode(result_vars, constraints, nm);
  Assert(blasted_term[0].getNumChildren() == utils::getSize(term));
  Trace("bv-pb") << "theory::bv::pb::DefaultUremPb done\n";
  return blasted_term;
}

template <class T>
T DefaultSdivPb(T term, TPseudoBooleanBlaster<T>* pbb)
{
  Trace("bv-pb") << "theory::bv::pb::DefaultSdivPb blasting " << term;
  Assert(term.getKind() == Kind::BITVECTOR_SDIV);

  NodeManager* nm = pbb->getNodeManager();
  unsigned num_bits = utils::getSize(term);
  T result_vars = pbb->newVariable(num_bits);
  Trace("bv-pb") << " with bits " << result_vars << "\n";

  T a = pbb->blastTerm(term[0]);
  T b = pbb->blastTerm(term[1]);

  T rem_p = pbb->newVariable(num_bits);
  T rem_n = pbb->newVariable(num_bits);
  T a_sign_bit = a[0][num_bits - 1];
  std::unordered_set<Node> constraints;

  // 0 <= rem_p < b <-> sum(2^i*b) - sum(2^i*rem_p) >= 1 && sum(2^i*rem_p) >= 0
  std::vector<Node> ult_variables_p;
  std::vector<Node> ult_coefficients_p;
  std::vector<Node> aux_coefficients_b = bvToSigned(num_bits, nm);
  std::vector<Node> aux_coefficients_rem_p = bvToSigned(num_bits, nm, -1);
  for (unsigned i = 0; i < num_bits; i++)
  {
    ult_variables_p.push_back(b[0][i]);
    ult_variables_p.push_back(rem_p[i]);
    ult_coefficients_p.push_back(aux_coefficients_b[i]);
    ult_coefficients_p.push_back(aux_coefficients_rem_p[i]);
  }

  constraints.insert(mkConstraintNode(
      Kind::GEQ, ult_variables_p, ult_coefficients_p, pbb->d_ONE, nm));

  std::vector<Node> rem_p_vars;
  for (const T& v : rem_p) rem_p_vars.push_back(v);

  constraints.insert(mkConstraintNode(
      Kind::GEQ, rem_p_vars, bvToSigned(num_bits, nm), pbb->d_ZERO, nm));

  // -b < rem_n <= 0 <-> sum(2^i*b) + sum(2^i*rem_p) >= 1 && sum(2^i*rem_p) <= 0
  std::vector<Node> ult_variables_n;
  std::vector<Node> ult_coefficients_n;
  std::vector<Node> aux_coefficients = bvToSigned(num_bits, nm);
  for (unsigned i = 0; i < num_bits; i++)
  {
    ult_variables_n.push_back(b[0][i]);
    ult_variables_n.push_back(rem_n[i]);
    ult_coefficients_n.push_back(aux_coefficients[i]);
    ult_coefficients_n.push_back(aux_coefficients[i]);
  }

  constraints.insert(mkConstraintNode(
      Kind::GEQ, ult_variables_n, ult_coefficients_n, pbb->d_ONE, nm));

  std::vector<Node> rem_n_vars;
  for (const T& v : rem_n) rem_n_vars.push_back(v);

  constraints.insert(mkConstraintNode(
      Kind::GEQ, rem_n_vars, bvToSigned(num_bits, nm, -1), pbb->d_ZERO, nm));

  // rem <-> a_sign_bit ? rem_n : rem_p
  T rem = pbb->newVariable(num_bits);
  for (unsigned i = 0; i < num_bits; i++)
  {
    for (const T& c : mkPbIte(a_sign_bit, rem_n[i], rem_p[i], rem[i], nm))
      constraints.emplace(c);
  }

  // a = b*quot + rem
  unsigned num_bits_quot = num_bits + 1;
  T quot = pbb->newVariable(num_bits_quot);

  T tableau = pbb->newVariable(num_bits * num_bits_quot);

  for (unsigned i = 0; i < num_bits; i++)
  {
    for (unsigned j = 0; j < num_bits_quot; j++)
    {
      std::vector<Node> and_constraint = {
          b[0][i], quot[j], tableau[i * num_bits_quot + j]};
      constraints.insert(
          mkConstraintNode(Kind::GEQ, and_constraint, {1, 1, -2}, 0, nm));
      constraints.insert(
          mkConstraintNode(Kind::GEQ, and_constraint, {-1, -1, 1}, -1, nm));
    }
  }

  std::vector<Node> variables;
  std::vector<Node> coefficients;
  for (unsigned i = 0; i < num_bits; i++)
  {
    for (unsigned j = 0; j < num_bits_quot; j++)
    {
      variables.push_back(tableau[i * num_bits_quot + j]);
      if (j == num_bits)
      {
        coefficients.push_back(
            nm->mkConstInt(Rational(Integer(-1).multiplyByPow2(i + j))));
      }
      else
      {
        coefficients.push_back(
            nm->mkConstInt(Rational(Integer(1).multiplyByPow2(i + j))));
      }
    }
  }

  for (const T& v : rem) variables.push_back(v);
  for (const T& c : bvToSigned(num_bits, nm))
  {
    coefficients.push_back(c);
  }

  for (const T& v : a[0]) variables.push_back(v);
  for (const T& c : bvToSigned(num_bits, nm, -1))
  {
    coefficients.push_back(c);
  }

  // sum(2^i*t) + sum(2^i*rem) - sum(2^i*a) = 0
  constraints.insert(
      mkConstraintNode(Kind::EQUAL, variables, coefficients, pbb->d_ZERO, nm));

  // bitwise OR reduction of b
  T cond = pbb->newVariable(1);
  for (unsigned i = 0; i < num_bits; i++)
  {
    std::vector<Node> unit_constraint = {cond[0], b[0][i]};
    constraints.insert(
        mkConstraintNode(Kind::GEQ, unit_constraint, {1, -1}, 0, nm));
  }
  std::vector<Node> disjunction_vars;
  std::vector<int> disjunction_coef;
  for (unsigned i = 0; i < num_bits; i++)
  {
    disjunction_vars.push_back(b[0][i]);
    disjunction_coef.push_back(1);
  }
  disjunction_vars.push_back(cond[0]);
  disjunction_coef.push_back(-1);

  constraints.insert(
      mkConstraintNode(Kind::GEQ, disjunction_vars, disjunction_coef, 0, nm));

  // result_vars <-> cond ? quot : 11..11
  for (unsigned i = 0; i < num_bits; i++)
  {
    for (const T& c : mkPbIte(cond[0], quot[i], pbb->d_ONE, result_vars[i], nm))
      constraints.emplace(c);
  }

  for (const T& c : a[1]) constraints.insert(c);
  for (const T& c : b[1]) constraints.insert(c);

  T blasted_term = mkTermNode(result_vars, constraints, nm);
  Assert(blasted_term[0].getNumChildren() == utils::getSize(term));
  Trace("bv-pb") << "theory::bv::pb::DefaultSdivPb done\n";
  return blasted_term;
}

template <class T>
T DefaultItePb(T term, TPseudoBooleanBlaster<T>* pbb)
{
  Trace("bv-pb") << "theory::bv::pb::DefaultItePb blasting " << term;
  Assert(term.getKind() == Kind::BITVECTOR_ITE);

  NodeManager* nm = pbb->getNodeManager();
  unsigned num_bits = utils::getSize(term);
  T result_vars = pbb->newVariable(num_bits);
  Trace("bv-pb") << " with bits " << result_vars << "\n";

  T cond = pbb->blastTerm(term[0]);
  T thenpart = pbb->blastTerm(term[1]);
  T elsepart = pbb->blastTerm(term[2]);
  Assert(cond[0].getNumChildren() == 1);
  Assert(thenpart[0].getNumChildren() == elsepart[0].getNumChildren());
  Assert(num_bits == thenpart[0].getNumChildren());

  std::unordered_set<Node> constraints;
  for (unsigned i = 0; i < num_bits; i++)
  {
    for (const T& c : mkPbIte(cond[0][0], thenpart[0][i], elsepart[0][i], result_vars[i], nm))
      constraints.emplace(c);
  }

  for (const T& c : cond[1]) constraints.insert(c);
  for (const T& c : thenpart[1]) constraints.insert(c);
  for (const T& c : elsepart[1]) constraints.insert(c);

  T blasted_term = mkTermNode(result_vars, constraints, nm);
  Assert(blasted_term[0].getNumChildren() == utils::getSize(term));
  Trace("bv-pb") << "theory::bv::pb::DefaultItePb done\n";
  return blasted_term;
}

template <class T>
T DefaultCompPb(T term, TPseudoBooleanBlaster<T>* pbb)
{
  Trace("bv-pb") << "theory::bv::pb::DefaultCompPb blasting " << term;
  Assert(term.getKind() == Kind::BITVECTOR_COMP);

  NodeManager* nm = pbb->getNodeManager();
  unsigned num_bits = utils::getSize(term);
  T result_var = pbb->newVariable(num_bits);
  Trace("bv-pb") << " with bits " << result_var << "\n";

  T a = pbb->blastTerm(term[0]);
  T b = pbb->blastTerm(term[1]);

  unsigned blasted_size = a[0].getNumChildren();
  Assert(a[0].getNumChildren() == b[0].getNumChildren());
  std::unordered_set<T> constraints;
  T a_xor_b = pbb->newVariable(blasted_size);
  for (unsigned i = 0; i < blasted_size; i++)
  {
    for (const T& c : mkPbXor(a[0][i], b[0][i], a_xor_b[i], nm))
      constraints.emplace(c);
  }

  // NOR with bits from a_xor_b
  for (unsigned i = 0; i < blasted_size; i++)
  {
    std::vector<Node> unit_constraint = {result_var[0], a_xor_b[i]};
    constraints.insert(
        mkConstraintNode(Kind::GEQ, unit_constraint, {-1, -1}, -1, nm));
  }
  std::vector<Node> nor_variables;
  std::vector<int> nor_coefficients;
  for (unsigned i = 0; i < blasted_size; i++)
  {
    nor_variables.push_back(a_xor_b[i]);
    nor_coefficients.push_back(1);
  }
  nor_variables.push_back(result_var[0]);
  nor_coefficients.push_back(1);

  constraints.insert(
      mkConstraintNode(Kind::GEQ, nor_variables, nor_coefficients, 1, nm));

  for (const T& c : a[1]) constraints.insert(c);
  for (const T& c : b[1]) constraints.insert(c);

  T blasted_term = mkTermNode(result_var, constraints, nm);
  Assert(blasted_term[0].getNumChildren() == utils::getSize(term));
  Trace("bv-pb") << "theory::bv::pb::DefaultCompPb done\n";
  return blasted_term;
}

template <class T>
T DefaultSignExtendPb(T term, TPseudoBooleanBlaster<T>* pbb)
{
  Trace("bv-pb") << "theory::bv::pb::DefaultSignExtendPb blasting " << term;
  Assert(term.getKind() == Kind::BITVECTOR_SIGN_EXTEND);

  NodeManager* nm = pbb->getNodeManager();
  unsigned num_bits = utils::getSize(term);
  T result_vars = pbb->newVariable(num_bits);
  Trace("bv-pb") << " with bits " << result_vars << "\n";

  std::unordered_set<T> constraints;

  T blasted = pbb->blastTerm(term[0]);
  for (const T& c : blasted[1]) constraints.insert(c);

  unsigned blasted_size = blasted[0].getNumChildren();
  unsigned amount = utils::getSignExtendAmount(term);
  Assert(blasted_size + amount == num_bits);
  T sign_bit = blasted[0][blasted_size - 1];

  for (unsigned i = 0; i < blasted_size; i++)
  {
    std::vector<Node> vars = {blasted[0][i], result_vars[i]};
    constraints.insert(mkConstraintNode(Kind::EQUAL, vars, {1, -1}, 0, nm));
  }

  for (unsigned i = 0; i < amount; i++)
  {
    std::vector<Node> vars = {sign_bit, result_vars[blasted_size + i]};
    constraints.insert(mkConstraintNode(Kind::EQUAL, vars, {1, -1}, 0, nm));
  }

  T blasted_term = mkTermNode(result_vars, constraints, nm);
  Assert(blasted_term[0].getNumChildren() == utils::getSize(term));
  Trace("bv-pb") << "theory::bv::pb::DefaultSignExtendPb done\n";
  return blasted_term;
}

template <class T>
T DefaultUltbvPb(T term, TPseudoBooleanBlaster<T>* pbb)
{
  Trace("bv-pb") << "theory::bv::pb::DefaultUltbvPb blasting " << term;
  Assert(term.getKind() == Kind::BITVECTOR_ULTBV);

  NodeManager* nm = pbb->getNodeManager();
  unsigned num_bits = utils::getSize(term);
  T result_var = pbb->newVariable(num_bits);
  Trace("bv-pb") << " with bits " << result_var << "\n";

  T a = pbb->blastTerm(term[0]);
  T b = pbb->blastTerm(term[1]);

  unsigned blasted_size = a[0].getNumChildren();
  Assert(a[0].getNumChildren() == b[0].getNumChildren());
  T subtraction_vars = pbb->newVariable(blasted_size);
  std::unordered_set<T> constraints;

  // NUM(a) - NUM(b) - NUM(subtraction_vars concat result_var) = 0
  std::vector<Node> variables;
  std::vector<Node> coefficients;
  std::vector<Node> aux_coefficients_a = bvToUnsigned(blasted_size, nm);
  std::vector<Node> aux_coefficients_b = bvToUnsigned(blasted_size, nm, -1);
  for (unsigned i = 0; i < blasted_size; i++)
  {
    variables.push_back(a[0][i]);
    variables.push_back(b[0][i]);
    coefficients.push_back(aux_coefficients_a[i]);
    coefficients.push_back(aux_coefficients_b[i]);
  }
  for (const T& v : subtraction_vars) variables.push_back(v);
  variables.push_back(result_var[0]);
  for (const T& c : bvToSigned(blasted_size + 1, nm, -1))
  {
    coefficients.push_back(c);
  }

  constraints.insert(
      mkConstraintNode(Kind::EQUAL, variables, coefficients, pbb->d_ZERO, nm));

  for (const T& c : a[1]) constraints.insert(c);
  for (const T& c : b[1]) constraints.insert(c);

  T blasted_term = mkTermNode(result_var, constraints, nm);
  Assert(blasted_term[0].getNumChildren() == utils::getSize(term));
  Trace("bv-pb") << "theory::bv::pb::DefaultUltbvPb done\n";
  return blasted_term;
}

template <class T>
T DefaultSltbvPb(T term, TPseudoBooleanBlaster<T>* pbb)
{
  Trace("bv-pb") << "theory::bv::pb::DefaultSltbvPb blasting " << term;
  Assert(term.getKind() == Kind::BITVECTOR_SLTBV);

  NodeManager* nm = pbb->getNodeManager();
  unsigned num_bits = utils::getSize(term);
  T result_var = pbb->newVariable(num_bits);
  Trace("bv-pb") << " with bits " << result_var << "\n";

  T a = pbb->blastTerm(term[0]);
  T b = pbb->blastTerm(term[1]);

  unsigned blasted_size = a[0].getNumChildren();
  Assert(a[0].getNumChildren() == b[0].getNumChildren());
  T subtraction_vars = pbb->newVariable(blasted_size);
  std::unordered_set<T> constraints;

  // NUM(a) - NUM(b) - NUM(subtraction_vars concat result_var) = 0
  std::vector<Node> variables;
  std::vector<Node> coefficients;
  std::vector<Node> aux_coefficients_a = bvToSigned(blasted_size, nm);
  std::vector<Node> aux_coefficients_b = bvToSigned(blasted_size, nm, -1);
  for (unsigned i = 0; i < blasted_size; i++)
  {
    variables.push_back(a[0][i]);
    variables.push_back(b[0][i]);
    coefficients.push_back(aux_coefficients_a[i]);
    coefficients.push_back(aux_coefficients_b[i]);
  }
  for (const T& v : subtraction_vars) variables.push_back(v);
  variables.push_back(result_var[0]);
  for (const T& c : bvToSigned(blasted_size + 1, nm, -1))
  {
    coefficients.push_back(c);
  }

  constraints.insert(
      mkConstraintNode(Kind::EQUAL, variables, coefficients, pbb->d_ZERO, nm));

  for (const T& c : a[1]) constraints.insert(c);
  for (const T& c : b[1]) constraints.insert(c);

  T blasted_term = mkTermNode(result_var, constraints, nm);
  Assert(blasted_term[0].getNumChildren() == utils::getSize(term));
  Trace("bv-pb") << "theory::bv::pb::DefaultSltbvPb done\n";
  return blasted_term;
}

}  // namespace pb
}  // namespace bv
}  // namespace theory
}  // namespace cvc5::internal

#endif  // CVC5__THEORY__BV__PB__PB_BLAST_STRATEGIES_TEMPLATE_H
