/******************************************************************************
 * Top contributors (to current version):
 *   Alan Prado, Haniel Barbosa
 *
 * This file is part of the cvc5 project.
 *
 * Copyright (c) 2009-2024 by the authors listed in the file AUTHORS
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
template <class T>
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

}  // namespace pb
}  // namespace bv
}  // namespace theory
}  // namespace cvc5::internal

#endif  // CVC5__THEORY__BV__PB__PB_BLAST_STRATEGIES_TEMPLATE_H
