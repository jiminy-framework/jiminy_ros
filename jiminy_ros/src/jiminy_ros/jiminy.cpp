// MIT License
//
// Copyright (c) 2026 Jiminy Framework
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#include <algorithm>
#include <stdexcept>
#include <unordered_set>

#include "jiminy_ros/jiminy.hpp"

using namespace jiminy;

Jiminy::Jiminy(const std::string &description,
               const std::map<std::string, Fact> &facts,
               const std::map<std::string, Norm> &norms,
               const std::map<std::string, Contrary> &contraries,
               const std::map<std::string, Priority> &priorities)
    : description_(description), facts_(facts), norms_(norms),
      contraries_(contraries), priorities_(priorities) {}

/*********************************************************
 * Getters
 *********************************************************/
Fact Jiminy::get_fact(const std::string &id) const {
  auto it = this->facts_.find(id);
  if (it == this->facts_.end()) {
    throw std::out_of_range("Fact with id " + id + " not found.");
  }
  return it->second;
}

Norm Jiminy::get_norm(const std::string &id) const {
  auto it = this->norms_.find(id);
  if (it == this->norms_.end()) {
    throw std::out_of_range("Norm with id " + id + " not found.");
  }
  return it->second;
}

Contrary Jiminy::get_contrary(const std::string &name) const {
  auto it = this->contraries_.find(name);
  if (it == this->contraries_.end()) {
    throw std::out_of_range("Contrary with id " + name + " not found.");
  }
  return it->second;
}

Priority Jiminy::get_priority(const std::string &id) const {
  auto it = this->priorities_.find(id);
  if (it == this->priorities_.end()) {
    throw std::out_of_range("Priority with id " + id + " not found.");
  }
  return it->second;
}

/*********************************************************
 * Argument generation
 *********************************************************/
std::map<std::string, Norm>
Jiminy::generate_arguments(const std::map<std::string, Fact> &facts) {

  bool changed = true;
  std::map<std::string, Fact> context = facts;
  std::map<std::string, Norm> args;

  while (changed) {
    changed = false;

    for (const auto &[norm_id, norm] : this->norms_) {
      if (norm.type == NormType::CONSTITUTIVE) {

        if (this->all_body_facts_present(norm.body, context)) {
          // Add conclusion fact if not already present
          if (context.count(norm.conclusion) == 0) {
            context.emplace(
                norm.conclusion,
                Fact{norm.conclusion, "Derived from norm " + norm.id});
            changed = true;
          }

          this->add_argument_if_not_present(args, norm);
        }
      }
    }
  }

  // Add regulative and permissive norms whose body facts are present
  for (const auto &[norm_id, norm] : this->norms_) {
    if (norm.type != NormType::CONSTITUTIVE) {

      if (this->all_body_facts_present(norm.body, context)) {
        args.emplace(norm.conclusion, norm);
      }
    }
  }

  return args;
}

/*********************************************************
 * Argumentation semantics
 *********************************************************/
std::pair<std::vector<Norm>, std::vector<Norm>>
Jiminy::compute_extension(const std::map<std::string, Norm> &arguments,
                          Semantics semantics) {
  switch (semantics) {
  case Semantics::GROUNDED:
    return this->compute_grounded_extension(arguments);
  case Semantics::PREFERRED:
    return this->compute_preferred_extension(arguments);
  case Semantics::STABLE:
    return this->compute_stable_extension(arguments);
  case Semantics::PRIORITY:
    return this->compute_priority_extension_B2(arguments);
  default:
    return {{}, {}};
  }
}

std::pair<std::vector<Norm>, std::vector<Norm>>
Jiminy::compute_grounded_extension(
    const std::map<std::string, Norm> &arguments) {
  std::vector<Norm> arg_list;
  arg_list.reserve(arguments.size());
  for (const auto &[key, norm] : arguments) {
    arg_list.push_back(norm);
  }

  std::unordered_set<std::string> E;
  bool changed = true;

  while (changed) {
    changed = false;

    for (const auto &A : arg_list) {
      // Find attackers
      auto attacker_keys = this->get_attacker_keys(A, arguments);

      // No attackers → accept
      if (attacker_keys.empty()) {
        if (E.count(A.conclusion) == 0) {
          E.insert(A.conclusion);
          changed = true;
        }
        continue;
      }

      // Otherwise must be defended by current E
      bool defended = true;
      for (const auto &att_key : attacker_keys) {
        const Norm &B = arguments.at(att_key);
        bool has_defender = false;
        for (const auto &[k, C] : arguments) {
          if (E.count(C.conclusion) && this->attacks(C, B)) {
            has_defender = true;
            break;
          }
        }
        if (!has_defender) {
          defended = false;
          break;
        }
      }

      if (defended && E.count(A.conclusion) == 0) {
        E.insert(A.conclusion);
        changed = true;
      }
    }
  }

  return this->build_extension(E, arguments);
}

std::pair<std::vector<Norm>, std::vector<Norm>>
Jiminy::compute_preferred_extension(
    const std::map<std::string, Norm> &arguments) {
  auto pset = this->powerset(arguments);
  std::vector<std::unordered_set<std::string>> admissible_sets;
  for (const auto &S : pset) {
    if (this->is_admissible(S, arguments)) {
      admissible_sets.push_back(S);
    }
  }

  // select maximal
  std::vector<std::unordered_set<std::string>> preferred_sets;
  for (const auto &S : admissible_sets) {
    bool is_maximal = true;
    for (const auto &T : admissible_sets) {
      if (S != T && std::includes(T.begin(), T.end(), S.begin(), S.end()) &&
          T.size() > S.size()) {
        is_maximal = false;
        break;
      }
    }
    if (is_maximal) {
      preferred_sets.push_back(S);
    }
  }

  if (preferred_sets.empty()) {
    return {{}, this->get_all_norms(arguments)};
  }

  // union
  std::unordered_set<std::string> accepted;
  for (const auto &S : preferred_sets) {
    for (const auto &s : S) {
      accepted.insert(s);
    }
  }

  return this->build_extension(accepted, arguments);
}

std::pair<std::vector<Norm>, std::vector<Norm>>
Jiminy::compute_stable_extension(const std::map<std::string, Norm> &arguments) {
  if (this->has_symmetric_cycle(arguments)) {
    return {{}, this->get_all_norms(arguments)};
  }

  auto pset = this->powerset(arguments);
  std::vector<std::unordered_set<std::string>> stable_sets;
  for (const auto &S : pset) {
    if (this->is_conflict_free(S, arguments) &&
        this->attacks_all_outside(S, arguments)) {
      stable_sets.push_back(S);
    }
  }

  if (stable_sets.empty()) {
    return {{}, this->get_all_norms(arguments)};
  }

  // pick first
  const auto &first = stable_sets[0];
  return this->build_extension(first, arguments);
}

std::pair<std::vector<Norm>, std::vector<Norm>>
Jiminy::compute_priority_extension_B2(
    const std::map<std::string, Norm> &arguments) {

  // 1. Group arguments by conclusion
  std::map<std::string, std::vector<Norm>> by_conclusion;
  for (const auto &[key, norm] : arguments) {
    by_conclusion[norm.conclusion].push_back(norm);
  }

  // 2. First pass: local priority comparison
  std::unordered_set<std::string> winners, losers;

  for (const auto &[conclusion, args] : by_conclusion) {
    auto contrs = this->get_contrary_ids(conclusion);
    std::vector<std::string> contrs_in_by_conclusion;
    for (const auto &c : contrs) {
      if (by_conclusion.count(c))
        contrs_in_by_conclusion.push_back(c);
    }

    if (contrs_in_by_conclusion.empty()) {
      winners.insert(conclusion);
      continue;
    }

    std::vector<std::string> candidates = {conclusion};
    candidates.insert(candidates.end(), contrs_in_by_conclusion.begin(),
                      contrs_in_by_conclusion.end());

    std::string best = this->find_highest_priority(candidates);

    if (conclusion == best) {
      winners.insert(conclusion);
    } else {
      losers.insert(conclusion);
    }
  }

  // 3 (B1). Clean contraries coming from losers
  std::map<std::string, std::unordered_set<std::string>> cleaned_contraries;
  for (const auto &[conclusion, _] : by_conclusion) {
    auto orig = this->get_contrary_ids(conclusion);
    std::unordered_set<std::string> clean;
    for (const auto &c : orig) {
      if (losers.count(c) == 0)
        clean.insert(c);
    }
    cleaned_contraries[conclusion] = clean;
  }

  // 4 (B2). REEVALUATE LOSERS (revival)
  std::unordered_set<std::string> revived;

  for (const auto &conclusion : losers) {
    if (!this->is_defeated_by(conclusion, winners, cleaned_contraries)) {
      revived.insert(conclusion);
    }
  }

  winners.insert(revived.begin(), revived.end());
  for (const auto &r : revived) {
    losers.erase(r);
  }

  // 5. Final defeat propagation among winners
  std::unordered_set<std::string> final_conclusions;

  for (const auto &conclusion : winners) {
    if (!this->is_defeated_by(conclusion, winners, cleaned_contraries)) {
      final_conclusions.insert(conclusion);
    }
  }

  // 6. Build argument list from accepted conclusions
  return this->build_extension(final_conclusions, arguments);
}

/*********************************************************
 * Helper functions
 *********************************************************/
bool Jiminy::all_body_facts_present(
    const std::vector<std::string> &body,
    const std::map<std::string, Fact> &context) const {
  for (const std::string &body_fact : body) {
    if (context.count(body_fact) == 0) {
      return false;
    }
  }
  return true;
}

void Jiminy::add_argument_if_not_present(std::map<std::string, Norm> &args,
                                         const Norm &norm) {
  // Use conclusion as key for consistency with how arguments are used elsewhere
  if (args.count(norm.conclusion)) {
    return;
  }
  args[norm.conclusion] = norm;
}

bool Jiminy::attacks(const Norm &attacker, const Norm &target) const {
  auto contrs = get_contrary_ids(attacker.conclusion);
  return !contrs.empty() && std::find(contrs.begin(), contrs.end(),
                                      target.conclusion) != contrs.end();
}

std::vector<std::unordered_set<std::string>>
Jiminy::powerset(const std::map<std::string, Norm> &arguments) const {
  // Pre-extract keys for efficient bit manipulation
  std::vector<std::string> keys;
  keys.reserve(arguments.size());
  for (const auto &[k, v] : arguments) {
    keys.push_back(k);
  }
  const int n = keys.size();
  const int total = 1 << n;
  std::vector<std::unordered_set<std::string>> result;
  result.reserve(total);
  for (int i = 0; i < total; ++i) {
    std::unordered_set<std::string> subset;
    subset.reserve(__builtin_popcount(i));
    for (int j = 0; j < n; ++j) {
      if (i & (1 << j)) {
        subset.insert(keys[j]);
      }
    }
    result.push_back(std::move(subset));
  }
  return result;
}

bool Jiminy::is_admissible(const std::unordered_set<std::string> &arguments_ids,
                           const std::map<std::string, Norm> &arguments) const {
  if (!this->is_conflict_free(arguments_ids, arguments)) {
    return false;
  }
  for (const auto &conclusion : arguments_ids) {
    // attackers of conclusion
    auto attackers =
        this->get_attacker_keys(arguments.at(conclusion), arguments);
    for (const auto &att_key : attackers) {
      bool defended = false;
      for (const auto &def_key : arguments_ids) {
        if (this->attacks(arguments.at(def_key), arguments.at(att_key))) {
          defended = true;
          break;
        }
      }
      if (!defended) {
        return false;
      }
    }
  }
  return true;
}

bool Jiminy::is_conflict_free(
    const std::unordered_set<std::string> &arguments_ids,
    const std::map<std::string, Norm> &arguments) const {
  for (const auto &a : arguments_ids) {
    for (const auto &b : arguments_ids) {
      if (a != b && this->attacks(arguments.at(a), arguments.at(b))) {
        return false;
      }
    }
  }
  return true;
}

bool Jiminy::is_defeated_by(
    const std::string &conclusion,
    const std::unordered_set<std::string> &defeaters,
    const std::map<std::string, std::unordered_set<std::string>>
        &contraries_map) const {
  const int pr_conclusion = this->get_priority_value(conclusion);
  auto it = contraries_map.find(conclusion);
  if (it == contraries_map.end()) {
    return false;
  }
  for (const auto &c : it->second) {
    if (defeaters.count(c) && this->get_priority_value(c) > pr_conclusion) {
      return true;
    }
  }
  return false;
}

bool Jiminy::attacks_all_outside(
    const std::unordered_set<std::string> &S,
    const std::map<std::string, Norm> &arguments) const {
  for (const auto &[k, norm] : arguments) {
    if (S.count(k)) {
      continue;
    }
    bool attacked = false;
    for (const auto &s : S) {
      if (this->attacks(arguments.at(s), norm)) {
        attacked = true;
        break;
      }
    }
    if (!attacked) {
      return false;
    }
  }
  return true;
}

bool Jiminy::has_symmetric_cycle(
    const std::map<std::string, Norm> &arguments) const {
  for (const auto &[a, norm_a] : arguments) {
    for (const auto &[b, norm_b] : arguments) {
      if (a != b && this->attacks(norm_a, norm_b) &&
          this->attacks(norm_b, norm_a)) {
        return true;
      }
    }
  }
  return false;
}

std::vector<std::string>
Jiminy::get_contrary_ids(const std::string &conclusion) const {
  auto it = this->contraries_.find(conclusion);
  if (it != this->contraries_.end()) {
    return it->second.contraries;
  }
  return {};
}

int Jiminy::get_priority_value(const std::string &id) const {
  auto it = this->priorities_.find(id);
  return it != this->priorities_.end() ? it->second.value : 0;
}

std::vector<std::string>
Jiminy::get_attacker_keys(const Norm &target,
                          const std::map<std::string, Norm> &arguments) const {
  std::vector<std::string> attackers;
  attackers.reserve(arguments.size());
  for (const auto &[k, norm] : arguments) {
    // Skip self-attacks
    if (k == target.conclusion) {
      continue;
    }
    if (this->attacks(norm, target)) {
      attackers.push_back(k);
    }
  }
  return attackers;
}

std::vector<Norm>
Jiminy::get_all_norms(const std::map<std::string, Norm> &arguments) const {
  std::vector<Norm> norms;
  norms.reserve(arguments.size());
  for (const auto &[k, v] : arguments) {
    norms.push_back(v);
  }
  return norms;
}

std::pair<std::vector<Norm>, std::vector<Norm>> Jiminy::build_extension(
    const std::unordered_set<std::string> &accepted_conclusions,
    const std::map<std::string, Norm> &arguments) const {
  std::vector<Norm> selected, rejected;
  for (const auto &[k, norm] : arguments) {
    if (accepted_conclusions.count(norm.conclusion)) {
      selected.push_back(norm);
    } else {
      rejected.push_back(norm);
    }
  }
  return {selected, rejected};
}

std::string Jiminy::find_highest_priority(
    const std::vector<std::string> &candidates) const {
  int max_prio = -1;
  std::string best;
  for (const auto &cand : candidates) {
    int p = this->get_priority_value(cand);
    if (p > max_prio || (p == max_prio && cand < best)) {
      max_prio = p;
      best = cand;
    }
  }
  return best;
}
