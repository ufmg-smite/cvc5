/******************************************************************************
 * Top contributors (to current version):
 *   Gereon Kremer, Daniel Larraz, Andrew Reynolds
 *
 * This file is part of the cvc5 project.
 *
 * Copyright (c) 2009-2025 by the authors listed in the file AUTHORS
 * in the top-level source directory and their institutional affiliations.
 * All rights reserved.  See the file COPYING in the top-level source
 * directory for licensing information.
 * ****************************************************************************
 *
 * Implementation of coverings proof generator.
 */

#include "theory/arith/nl/coverings/proof_generator.h"

#ifdef CVC5_POLY_IMP

#include "proof/lazy_tree_proof_generator.h"
#include "proof/proof_node_algorithm.h"
#include "theory/arith/arith_poly_norm.h"
#include "theory/arith/arith_utilities.h"
#include "theory/arith/nl/poly_conversion.h"
#include "util/indexed_root_predicate.h"

using namespace cvc5::internal::kind;

namespace cvc5::internal {
namespace theory {
namespace arith {
namespace nl {
namespace coverings {


poly::Polynomial orientLikeConstraint(const Node& n, const poly::Polynomial& p)
{
  bool negated = n.getKind() == Kind::NOT;
  Kind k = (negated ? n[0] : n).getKind();
  Assert(k == Kind::EQUAL || k == Kind::LT || k == Kind::LEQ || k == Kind::GT
         || k == Kind::GEQ);
  // mirrors normalize_kind in poly_conversion.cpp
  bool flipped = (k == Kind::GT || k == Kind::GEQ) != negated;
  return flipped ? -p : p;
}

RootMap buildRootMap(
    const std::vector<std::pair<poly::Polynomial, poly::Value>>& polyRoots)
{
  RootMap rootMap;

  for (const auto& pr : polyRoots)
  {
    rootMap.d_roots.push_back(pr.second);
  }
  std::sort(rootMap.d_roots.begin(), rootMap.d_roots.end());
  rootMap.d_roots.erase(std::unique(rootMap.d_roots.begin(), rootMap.d_roots.end()),
                      rootMap.d_roots.end());

  for (const auto& pr : polyRoots)
  {
    auto rit = std::lower_bound(
        rootMap.d_roots.begin(), rootMap.d_roots.end(), pr.second);
    Assert(rit != rootMap.d_roots.end() && *rit == pr.second);
    std::size_t id = std::distance(rootMap.d_roots.begin(), rit);

    auto mit = std::find_if(
        rootMap.d_members.begin(), rootMap.d_members.end(), [&pr](const auto& m) {
          return m.first == pr.first;
        });
    if (mit == rootMap.d_members.end())
    {
      rootMap.d_members.emplace_back(pr.first, std::vector<std::size_t>{id});
    }
    else if (std::find(mit->second.begin(), mit->second.end(), id)
             == mit->second.end())
    {
      mit->second.push_back(id);
    }
  }
  for (auto& m : rootMap.d_members)
  {
    std::sort(m.second.begin(), m.second.end());
  }

  return rootMap;
}

std::vector<size_t> RootMap::polyRootIndices(const poly::Polynomial& p)
{
  for (const auto& [q, ids]: d_members)
  {
    if (q == p)
    {
      return ids;
    }
  }
  return {};
}

size_t RootMap::rootIndex(const poly::Value& r)
{
  for (size_t i = 0; i < d_roots.size(); ++i)
  {
    if (d_roots[i] == r)
    {
      return i;
    }
  }
  Assert(false);
  return 0;
}

std::ostream& operator<<(std::ostream& os, const RootMap& map)
{
  os << "RootMap:" << std::endl;
  os << "  roots (canonical, ascending):" << std::endl;
  for (std::size_t i = 0; i < map.d_roots.size(); ++i)
  {
    os << "    [" << i << "] " << map.d_roots[i] << std::endl;
  }
  os << "  members:" << std::endl;
  for (const auto& m : map.d_members)
  {
    os << "    " << m.first << " -> {";
    for (std::size_t j = 0; j < m.second.size(); ++j)
    {
      os << (j == 0 ? "" : ", ") << m.second[j];
    }
    os << "}" << std::endl;
  }
  return os;
}

namespace {

/**
 * Retrieves the root indices of the sign-invariant region of v.
 *
 * We assume that roots holds a sorted list of roots from one polynomial.
 * If v is equal to one of these roots, we return (id,id) where id is the index
 * of this root within roots. Otherwise, we return the id of the largest root
 * below v and the id of the smallest root above v. To make sure a smaller root
 * and a larger root always exist, we implicitly extend the roots by -infty and
 * infty.
 *
 * ATTENTION: if we return id, the corresponding root is:
 *   - id = 0: -infty
 *   - 0 < id <= roots.size(): roots[id-1]
 *   - id = roots.size() + 1: infty
 */
std::pair<std::size_t, std::size_t> getRootIDs(
    const std::vector<poly::Value>& roots, const poly::Value& v)
{
  for (std::size_t i = 0; i < roots.size(); ++i)
  {
    if (roots[i] == v)
    {
      return {i + 1, i + 1};
    }
    if (roots[i] > v)
    {
      return {i, i + 1};
    }
  }
  return {roots.size(), roots.size() + 1};
}

/**
 * Constructs an IndexedRootExpression:
 *   var ~rel~ root_k(poly)
 * where root_k(poly) is "the k'th root of the polynomial".
 *
 * @param var The variable that is bounded
 * @param rel The relation for this constraint
 * @param zero A node representing Rational(0)
 * @param k The index of the root (starting with 1)
 * @param poly The polynomial whose root shall be considered
 * @param vm A variable mapper from cvc5 to libpoly variables
 */
Node mkIRP(NodeManager* nm,
           const Node& var,
           Kind rel,
           const Node& zero,
           std::size_t k,
           const poly::Polynomial& poly,
           VariableMapper& vm)
{
  auto op = nm->mkConst<IndexedRootPredicate>(IndexedRootPredicate(k));
  return nm->mkNode(Kind::INDEXED_ROOT_PREDICATE,
                    op,
                    nm->mkNode(rel, var, zero),
                    as_cvc_polynomial(nm, poly, vm));
}

}  // namespace

CoveringsProofGenerator::CoveringsProofGenerator(Env& env,
                                                 context::Context* ctx)
    : EnvObj(env),
      d_proofs(env, ctx),
      d_current(nullptr),
      d_cdp(new CDProof(env, ctx)),
      d_ctx(ctx)
{
  d_false = nodeManager()->mkConst(false);
  d_zero = nodeManager()->mkConstReal(Rational(0));
}

void CoveringsProofGenerator::startNewProof(bool isUniv)
{
  // roots collected for a previous check must not leak into this proof
  d_polysRoots.clear();
  d_intervals.clear();
  d_rootMap = RootMap();
  if (!isUniv)
  {
    d_current = d_proofs.allocateProof();
    return;
  }
  d_cdp = new CDProof(d_env, d_ctx);
}
void CoveringsProofGenerator::startRecursive() { d_current->openChild(); }
void CoveringsProofGenerator::endRecursive(size_t intervalId)
{
  d_current->setCurrentTrust(
      intervalId, TrustId::ARITH_NL_COVERING_RECURSIVE, {}, {d_false}, d_false);
  d_current->closeChild();
}
void CoveringsProofGenerator::startScope()
{
  d_current->openChild();
  d_current->getCurrent().d_rule = ProofRule::SCOPE;
}
void CoveringsProofGenerator::endScope(const std::vector<Node>& args)
{
  d_current->setCurrent(0, ProofRule::SCOPE, {}, args, d_false);
  d_current->closeChild();
}

ProofGenerator* CoveringsProofGenerator::getProofGenerator() const
{
  return d_current;
}

CDProof* CoveringsProofGenerator::getUnivProofGenerator() const
{
  return d_cdp;
}

void CoveringsProofGenerator::initializeRootMap()
{
  d_rootMap = buildRootMap(d_polysRoots);
}

void CoveringsProofGenerator::addUnivRoots(
    const std::vector<poly::Value>& roots, poly::Polynomial poly)
{
  for (const auto& root: roots)
  {
    d_polysRoots.emplace_back(poly, root);
  }
}

void CoveringsProofGenerator::addPointPiece(
    const poly::Value& v,
    const poly::Polynomial& p,
    const Node& origin)
{
  for (const auto& pInterval: d_intervals)
  {
    const poly::Interval& interval = pInterval.d_interval;
    if (poly::is_point(interval) && poly::get_lower(interval) == v)
    {
      return;
    }
  }
  d_intervals.emplace_back(poly::Interval(v), p, origin, Node::null());
}

void CoveringsProofGenerator::addIntervals(
    const std::vector<CACInterval>& intervals,
    const std::map<Node, poly::Polynomial>& constraintPolys)
{
  for (const auto& cac_interval: intervals)
  {
    const poly::Interval& interval = cac_interval.d_interval;
    Assert(cac_interval.d_origins.size() == 1);
    const Node& originalConstraint = cac_interval.d_origins[0];
    const poly::Polynomial& polynomial = constraintPolys.at(originalConstraint);

    if (poly::is_point(interval))
    {
      addPointPiece(poly::get_lower(interval), polynomial, originalConstraint);
      continue;
    }
    poly::Value lower = poly::get_lower(interval);
    poly::Value upper = poly::get_upper(interval);
    if (!poly::get_lower_open(interval))
    {
      addPointPiece(lower, polynomial, originalConstraint);
    }

    int lower_idx = poly::is_minus_infinity(lower) ? -1 : d_rootMap.rootIndex(lower);
    int upper_idx = poly::is_plus_infinity(upper) ?
      d_rootMap.d_roots.size() :
      d_rootMap.rootIndex(upper);
    for (const auto& root_idx: d_rootMap.polyRootIndices(polynomial))
    {
      if (int(root_idx) >= upper_idx)
      {
        break;
      }
      if (int(root_idx) > lower_idx)
      {
        poly::Value pRoot = d_rootMap.d_roots[root_idx];
        poly::Interval open_interval = poly::Interval(lower, pRoot);
        d_intervals.emplace_back(open_interval, polynomial, originalConstraint, Node::null());
        addPointPiece(pRoot, polynomial, originalConstraint);
        lower_idx = root_idx;
        lower = d_rootMap.d_roots[root_idx];
      }
    }
    auto open_interval = poly::Interval(lower, upper);
    d_intervals.emplace_back(open_interval, polynomial, originalConstraint, Node::null());

    if (!poly::get_upper_open(interval))
    {
      addPointPiece(upper, polynomial, originalConstraint);
    }
  }
}

Node CoveringsProofGenerator::addCoverStep(const Node& var)
{
  NodeManager* nm = nodeManager();
  std::vector<Node> intsData;
  std::vector<Node> disjs;
  bool hasFullInterval = false;
  for (const auto& pInterval: d_intervals)
  {
    const poly::Interval& interval = pInterval.d_interval;
    Node lower = value_to_node(get_lower(interval), var);
    Node upper = value_to_node(get_upper(interval), var);
    intsData.push_back(nm->mkNode(Kind::SEXPR, {lower, upper}));

    if (lower == upper)
    {
      disjs.push_back(nm->mkNode(Kind::EQUAL, var, lower));
    }
    else
    {
      std::vector<Node> conjs;
      if (lower.getKind() != Kind::MINUS_INFINITY)
      {
        Node conj = nm->mkNode(Kind::GT, var, lower);
        conjs.push_back(conj);
      }
      if (upper.getKind() != Kind::PLUS_INFINITY)
      {
        Node conj = nm->mkNode(Kind::LT, var, upper);
        conjs.push_back(conj);
      }
      if (conjs.empty())
      {
        hasFullInterval = true;
      }
      disjs.push_back(nm->mkAnd(conjs));
    }
  }

  Node coverConc = hasFullInterval ? nm->mkConst(true) : nm->mkOr(disjs);
  if (!hasFullInterval)
  {
    Node intsDataNode = nm->mkNode(Kind::SEXPR, intsData);
    std::vector<Node> coverArgs{var, intsDataNode};
    d_cdp->addStep(coverConc, ProofRule::COVER, {}, coverArgs);
  }
  return coverConc;
}

void CoveringsProofGenerator::addValidateIntervalsStep(
    const Node& var,
    VariableMapper& vm)
{
  NodeManager* nm = nodeManager();
  std::vector<Node> intervalsData;
  std::vector<Node> concConjs;
  for (auto& pInterval: d_intervals)
  {
    const poly::Interval& interval = pInterval.d_interval;
    const poly::Polynomial& p = pInterval.d_poly;
    Node cvc_p = nl::as_cvc_polynomial_no_pow(nm, p, vm);
    if (is_point(interval))
    {
      Node r = value_to_node(get_upper(interval), var);
      concConjs.push_back(mkIsRoot(nm, cvc_p, r));
      intervalsData.push_back(nm->mkNode(Kind::SEXPR, {cvc_p, r}));
      pInterval.d_fact = concConjs.back();
      continue;
    }
    Node l = value_to_node(get_lower(interval), var);
    Node r = value_to_node(get_upper(interval), var);

    intervalsData.push_back(nm->mkNode(Kind::SEXPR, {cvc_p, l, r}));
    concConjs.push_back(mkSgnInv(nm, cvc_p, l, r));
    pInterval.d_fact = concConjs.back();
  }

  std::vector<Node> rootsData;
  for (const poly::Value& r : d_rootMap.d_roots)
  {
    rootsData.push_back(value_to_node(r, var));
  }

  std::vector<Node> membersData;
  for (const auto& [p, ids] : d_rootMap.d_members)
  {
    Node cvc_p = nl::as_cvc_polynomial_no_pow(nm, p, vm);
    std::vector<Node> idsData;
    for (size_t id : ids)
    {
      idsData.push_back(nm->mkConstInt(Rational(id)));
    }
    membersData.push_back(
        nm->mkNode(Kind::SEXPR, cvc_p, nm->mkNode(Kind::SEXPR, idsData)));
  }

  std::vector<Node> args{nm->mkNode(Kind::SEXPR, intervalsData),
                         nm->mkNode(Kind::SEXPR, rootsData),
                         nm->mkNode(Kind::SEXPR, membersData)};
  Node conc = nm->mkAnd(concConjs);
  d_cdp->addStep(conc, ProofRule::VALIDATE_INTERVALS, {}, args);
  if (concConjs.size() > 1)
  {
    for (size_t i = 0, n = concConjs.size(); i < n; ++i)
    {
        d_cdp->addStep(concConjs[i], ProofRule::AND_ELIM, {conc},
                       {nm->mkConstInt(Rational(i))});
    }
  }
}

void CoveringsProofGenerator::addElimSteps(
    const Node& var,
    VariableMapper& vm)
{
  NodeManager* nm = nodeManager();
  for (auto& pInterval : d_intervals)
  {
    Assert(!pInterval.d_fact.isNull())
        << "addValidateIntervalsStep must run before addElimSteps";
    const poly::Interval& interval = pInterval.d_interval;
    if (poly::is_point(interval))
    {
      Node cvc_p = pInterval.d_fact[0];
      Node cvc_r = pInterval.d_fact[1];
      std::vector<Node> args {var, cvc_r, cvc_p};
      Node lit = addNormalizedLiteral(pInterval.d_origin, cvc_p);
      std::vector<Node> premises {pInterval.d_fact, lit};
      auto conc = nm->mkNode(Kind::EQUAL, var, cvc_r).notNode();
      d_cdp->addStep(conc, ProofRule::RAN_EVAL, premises, args);
      pInterval.d_elim = conc;
      continue;
    }
    const poly::Value& l = poly::get_lower(interval);
    const poly::Value& r = poly::get_upper(interval);
    // a rational strictly inside (l, r); handles infinite endpoints
    poly::Value s = poly::value_between(l, true, r, true);

    Node lower = value_to_node(l, var);
    Node upper = value_to_node(r, var);
    Node sample = value_to_node(s, var);
    Node cvc_p = nl::as_cvc_polynomial_no_pow(nm, pInterval.d_poly, vm);
    // (p ~ 0), derived from the original constraint
    Node lit = addNormalizedLiteral(pInterval.d_origin, cvc_p);

    // (not piece); for the full line the piece is trivially true, so the
    // conclusion is false
    Node piece = mkOpenPiece(nm, var, lower, upper);
    Node conc = piece.isNull() ? d_false : piece.notNode();

    std::vector<Node> args{var, cvc_p, sample, lower, upper};
    d_cdp->addStep(
        conc, ProofRule::SGN_INV_ELIM, {pInterval.d_fact, lit}, args);
    pInterval.d_elim = conc;
  }
}

Node CoveringsProofGenerator::addNormalizedLiteral(const Node& origin,
                                                   const Node& q)
{
  NodeManager* nm = nodeManager();
  bool negated = origin.getKind() == Kind::NOT;
  Node atom = negated ? origin[0] : origin;
  Assert(atom.getNumChildren() == 2);
  Node canon = nm->mkNode(atom.getKind(), q, mkZero(q.getType()));
  Node lit = negated ? canon.notNode() : canon;
  if (lit == origin)
  {
    return lit;
  }
  // (= (* cx (- a b)) (* cy (- q 0))), by polynomial normalization
  Rational cx, cy;
  bool isRel = PolyNorm::isArithPolyNormRel(atom, canon, cx, cy);
  AlwaysAssert(isRel) << "Constraint " << origin
                      << " is not a scaling of its canonical literal " << lit;
  Node premise = PolyNorm::getArithPolyNormRelPremise(atom, canon, cx, cy);
  d_cdp->addStep(premise, ProofRule::ARITH_POLY_NORM, {}, {premise});
  // (= (k a b) (k q 0))
  Node equiv = atom.eqNode(canon);
  d_cdp->addStep(equiv, ProofRule::ARITH_POLY_NORM_REL, {premise}, {equiv});
  if (negated)
  {
    // (= (not (k a b)) (not (k q 0)))
    equiv = expr::proveCong(d_env, d_cdp, origin, {equiv});
    Assert(equiv == origin.eqNode(lit));
  }
  d_cdp->addStep(lit, ProofRule::EQ_RESOLVE, {origin, equiv}, {});
  return lit;
}

void CoveringsProofGenerator::addResolutionStep(const Node& coverConc)
{
  NodeManager* nm = nodeManager();
  if (!coverConc.isConst())
  {
    std::vector<Node> resChildren{coverConc};
    std::vector<Node> pols;
    std::vector<Node> pivots;
    for (const auto& pInterval : d_intervals)
    {
      Assert(pInterval.d_elim.getKind() == Kind::NOT);
      resChildren.push_back(pInterval.d_elim);
      pols.push_back(nm->mkConst(true));
      pivots.push_back(pInterval.d_elim[0]);
    }
    std::vector<Node> resArgs{nm->mkNode(Kind::SEXPR, pols),
                              nm->mkNode(Kind::SEXPR, pivots)};
    d_cdp->addStep(d_false, ProofRule::CHAIN_RESOLUTION, resChildren, resArgs);
  }
}

void CoveringsProofGenerator::closeUnivProof(
    std::vector<Node> constraints,
    VariableMapper& vm)
{
  Assert(vm.mVarCVCpoly.size() == 1 && vm.mVarpolyCVC.size() == 1);
  Assert(!d_intervals.empty());
  Node var = vm.mVarCVCpoly.begin()->first;
  NodeManager* nm = nodeManager();
  Node mis = nm->mkAnd(constraints);

  Node coverConc = addCoverStep(var);
  addValidateIntervalsStep(var, vm);
  addElimSteps(var, vm);
  addResolutionStep(coverConc);
  d_cdp->addStep(mis.notNode(), ProofRule::SCOPE, {d_false}, constraints);

  // std::vector<Node> prem;
  // std::vector<Node> args{var};
  // for (const auto& pr : d_polysRoots)
  // {
  //   Node poly = as_cvc_polynomial_no_pow(nodeManager(), pr.first, vm);
  //   Node val = value_to_node(pr.second, var);
  //   args.push_back(nodeManager()->mkNode(Kind::SEXPR, poly, val));
  // }
  // for (auto& pInterval : d_intervals)
  // {
  //   prem.push_back(pInterval.d_elim);
  // }
  // prem.push_back(coverConc);
  // prem.push_back(validateIntervalsConc);
  // prem.insert(prem.end(), constraints.begin(), constraints.end());
  // d_cdp->addStep(d_false, ProofRule::ARITH_COVERINGS_UNIV, prem, args);
}
void CoveringsProofGenerator::addDirect(Node var,
                                        VariableMapper& vm,
                                        const poly::Polynomial& poly,
                                        const poly::Assignment& a,
                                        const poly::Interval& interval,
                                        Node constraint,
                                        size_t intervalId)
{
  if (is_minus_infinity(get_lower(interval))
      && is_plus_infinity(get_upper(interval)))
  {
    // "Full conflict", constraint excludes (-inf,inf)
    d_current->openChild();
    d_current->setCurrentTrust(intervalId,
                               TrustId::ARITH_NL_COVERING_DIRECT,
                               {constraint},
                               {d_false},
                               d_false);
    d_current->closeChild();
    return;
  }
  std::vector<Node> res;
  auto roots = poly::isolate_real_roots(poly, a);
  if (get_lower(interval) == get_upper(interval))
  {
    // Excludes a single point only
    auto ids = getRootIDs(roots, get_lower(interval));
    Assert(ids.first == ids.second);
    res.emplace_back(mkIRP(nodeManager(),
                           var,
                           Kind::EQUAL,
                           mkZero(var.getType()),
                           ids.first,
                           poly,
                           vm));
  }
  else
  {
    // Excludes an open interval
    if (!is_minus_infinity(get_lower(interval)))
    {
      // Interval has lower bound that is not -inf
      auto ids = getRootIDs(roots, get_lower(interval));
      Assert(ids.first == ids.second);
      Kind rel = poly::get_lower_open(interval) ? Kind::GT : Kind::GEQ;
      res.emplace_back(
          mkIRP(nodeManager(), var, rel, d_zero, ids.first, poly, vm));
    }
    if (!is_plus_infinity(get_upper(interval)))
    {
      // Interval has upper bound that is not inf
      auto ids = getRootIDs(roots, get_upper(interval));
      Assert(ids.first == ids.second);
      Kind rel = poly::get_upper_open(interval) ? Kind::LT : Kind::LEQ;
      res.emplace_back(
          mkIRP(nodeManager(), var, rel, d_zero, ids.first, poly, vm));
    }
  }
  // Add to proof manager
  startScope();
  d_current->openChild();
  d_current->setCurrentTrust(intervalId,
                             TrustId::ARITH_NL_COVERING_DIRECT,
                             {constraint},
                             {d_false},
                             d_false);
  d_current->closeChild();
  endScope(res);
}

std::vector<Node> CoveringsProofGenerator::constructCell(
    Node var,
    const CACInterval& i,
    const poly::Assignment& a,
    const poly::Value& s,
    VariableMapper& vm)
{
  if (is_minus_infinity(get_lower(i.d_interval))
      && is_plus_infinity(get_upper(i.d_interval)))
  {
    // "Full conflict", constraint excludes (-inf,inf)
    return {};
  }

  std::vector<Node> res;

  // Just use bounds for all polynomials
  for (const auto& poly : i.d_mainPolys)
  {
    auto roots = poly::isolate_real_roots(poly, a);
    auto ids = getRootIDs(roots, s);
    if (ids.first == ids.second)
    {
      // Excludes a single point only
      res.emplace_back(
          mkIRP(nodeManager(), var, Kind::EQUAL, d_zero, ids.first, poly, vm));
    }
    else
    {
      // Excludes an open interval
      if (ids.first > 0)
      {
        // Interval has lower bound that is not -inf
        res.emplace_back(
            mkIRP(nodeManager(), var, Kind::GT, d_zero, ids.first, poly, vm));
      }
      if (ids.second <= roots.size())
      {
        // Interval has upper bound that is not inf
        res.emplace_back(
            mkIRP(nodeManager(), var, Kind::LT, d_zero, ids.second, poly, vm));
      }
    }
  }

  return res;
}

std::ostream& operator<<(std::ostream& os, const CoveringsProofGenerator& proof)
{
  return os << *proof.d_current;
}

}  // namespace coverings
}  // namespace nl
}  // namespace arith
}  // namespace theory
}  // namespace cvc5::internal

#endif
