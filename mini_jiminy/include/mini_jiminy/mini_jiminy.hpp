// MIT License
//
// Copyright (c) 2026 Miguel Ángel González Santamarta
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

#ifndef MINI_JIMINY__MINI_JIMINY_HPP_
#define MINI_JIMINY__MINI_JIMINY_HPP_

#include <map>
#include <string>
#include <unordered_set>
#include <vector>

namespace mini_jiminy {

/**
 * @enum NormType
 * @brief Enumeration for different types of norms.
 */
enum class NormType {
  CONSTITUTIVE,
  REGULATIVE,
  PERMISSIVE,
};

/**
 * @enum Semantics
 * @brief Enumeration for different argumentation semantics.
 */
enum class Semantics {
  GROUNDED,
  PREFERRED,
  STABLE,
  PRIORITY,
};

/**
 * @struct Fact
 * @brief Structure representing a fact with an identifier and description.
 */
struct Fact {
  std::string id;
  std::string description;
};

/**
 * @struct Norm
 * @brief Structure representing a norm with its components.
 */
struct Norm {
  std::string id;
  std::vector<std::string> body;
  std::string conclusion;
  NormType type;
  std::string stakeholder;
  std::string description;
};

/**
 * @struct Contrary
 * @brief Structure representing contraries for a given identifier.
 */
struct Contrary {
  std::string id;
  std::vector<std::string> contraries;
  std::string description;
};

/**
 * @struct Priority
 * @brief Structure representing priority levels for norms or facts.
 */
struct Priority {
  std::string id;
  int value;
  std::string description;
};

/**
 * @struct MetaPriority
 * @brief Structure representing dynamic priority rules based on context facts.
 */
struct MetaPriority {
  std::string if_condition; // fact that triggers this rule
  std::string stakeholder;
  int value;
  std::string description;
};

/**
 * @class Jiminy
 * @brief Main class for managing facts, norms, contraries, and priorities.
 */
class Jiminy {
public:
  /**
   * @brief Constructor for the Jiminy class.
   * @param description The description of the scenario.
   * @param facts A map of fact identifiers to Fact structures.
   * @param norms A map of norm identifiers to Norm structures.
   * @param contraries A map of contrary identifiers to Contrary structures.
   * @param priorities A map of priority identifiers to Priority structures.
   */
  Jiminy(const std::string &description,
         const std::map<std::string, Fact> &facts,
         const std::map<std::string, Norm> &norms,
         const std::map<std::string, Contrary> &contraries,
         const std::map<std::string, Priority> &priorities);

  /**
   * @brief Retrieve the description of the Jiminy instance.
   * @return A string containing the description.
   */
  std::string get_description() const { return this->description_; }

  /**
   * @brief Retrieve all facts managed by Jiminy.
   * @return A const reference to a map of fact identifiers to Fact structures.
   */
  const std::map<std::string, Fact> &get_facts() const { return this->facts_; }

  /**
   * @brief Retrieve a fact by its identifier.
   * @param id The identifier of the fact.
   * @return The Fact structure corresponding to the given identifier.
   */
  Fact get_fact(const std::string &id) const;

  /**
   * @brief Retrieve all norms managed by Jiminy.
   * @return A const reference to a map of norm identifiers to Norm structures.
   */
  const std::map<std::string, Norm> &get_norms() const { return this->norms_; }

  /**
   * @brief Retrieve a norm by its identifier.
   * @param id The identifier of the norm.
   * @return The Norm structure corresponding to the given identifier.
   */
  Norm get_norm(const std::string &id) const;

  /**
   * @brief Retrieve all contraries managed by Jiminy.
   * @return A const reference to a map of contrary identifiers to Contrary
   * structures.
   */
  const std::map<std::string, Contrary> &get_contraries() const {
    return this->contraries_;
  }

  /**
   * @brief Retrieve a contrary by its identifier.
   * @param name The identifier of the contrary.
   * @return The Contrary structure corresponding to the given identifier.
   */
  Contrary get_contrary(const std::string &name) const;

  /**
   * @brief Retrieve all priorities managed by Jiminy.
   * @return A const reference to a map of priority identifiers to Priority
   * structures.
   */
  const std::map<std::string, Priority> &get_priorities() const {
    return this->priorities_;
  }

  /**
   * @brief Retrieve a priority by its identifier.
   * @param id The identifier of the priority.
   * @return The Priority structure corresponding to the given identifier.
   */
  Priority get_priority(const std::string &id) const;

  /**
   * @brief Generate arguments based on the provided facts.
   * @param facts A map of fact identifiers to Fact structures.
   * @return A map of norm identifiers to Norm structures representing the
   * generated arguments.
   */
  std::map<std::string, Norm>
  generate_arguments(const std::map<std::string, Fact> &facts);

  /**
   * @brief Compute the extension of arguments based on the specified semantics.
   * @param arguments A map of norm identifiers to Norm structures representing
   * the arguments.
   * @param semantics The semantics to be used for computing the extension.
   * @return A pair of vectors containing accepted and rejected Norm structures.
   */
  std::pair<std::vector<Norm>, std::vector<Norm>>
  compute_extension(const std::map<std::string, Norm> &arguments,
                    Semantics semantics);

  /**
   * @brief Compute the grounded extension of arguments.
   * @param arguments A map of norm identifiers to Norm structures representing
   * the arguments.
   * @return A pair of vectors containing accepted and rejected Norm structures.
   */
  std::pair<std::vector<Norm>, std::vector<Norm>>
  compute_grounded_extension(const std::map<std::string, Norm> &arguments);

  /**
   * @brief Compute the preferred extension of arguments.
   * @param arguments A map of norm identifiers to Norm structures representing
   * the arguments.
   * @return A pair of vectors containing accepted and rejected Norm structures.
   */
  std::pair<std::vector<Norm>, std::vector<Norm>>
  compute_preferred_extension(const std::map<std::string, Norm> &arguments);

  /**
   * @brief Compute the stable extension of arguments.
   * @param arguments A map of norm identifiers to Norm structures representing
   * the arguments.
   * @return A pair of vectors containing accepted and rejected Norm structures.
   */
  std::pair<std::vector<Norm>, std::vector<Norm>>
  compute_stable_extension(const std::map<std::string, Norm> &arguments);

  /**
   * @brief Compute the priority-based extension using the B2 method.
   * @param arguments A map of norm identifiers to Norm structures representing
   * the arguments.
   * @return A pair of vectors containing accepted and rejected Norm structures.
   */
  std::pair<std::vector<Norm>, std::vector<Norm>>
  compute_priority_extension_B2(const std::map<std::string, Norm> &arguments);

private:
  /**
   * @brief Check if all body facts of a norm are present in the given context.
   * @param body A vector of fact identifiers representing the body of the norm.
   * @param context A map of fact identifiers to Fact structures representing
   * the context.
   * @return True if all body facts are present in the context, false otherwise.
   */
  bool all_body_facts_present(const std::vector<std::string> &body,
                              const std::map<std::string, Fact> &context) const;

  /**
   * @brief Add a norm to the arguments map if it is not already present.
   * @param args A map of norm identifiers to Norm structures representing the
   * arguments.
   * @param norm The Norm structure to be added.
   */
  void add_argument_if_not_present(std::map<std::string, Norm> &args,
                                   const Norm &norm);

  /**
   * @brief Retrieve the contrary identifiers for a given conclusion.
   * @param conclusion The conclusion for which to find contraries.
   * @return A vector of contrary identifiers.
   */
  std::vector<std::string>
  get_contrary_ids(const std::string &conclusion) const;

  /**
   * @brief Generate the powerset of a set of arguments.
   * @param arguments A map of norm identifiers to Norm structures representing
   * the arguments.
   * @return A vector of unordered sets, each representing a subset of
   * arguments.
   */
  std::vector<std::unordered_set<std::string>>
  powerset(const std::map<std::string, Norm> &arguments) const;

  /**
   * @brief Check if one norm attacks another.
   * @param attacker The attacking Norm structure.
   * @param target The target Norm structure.
   * @return True if the attacker attacks the target, false otherwise.
   */
  bool attacks(const Norm &attacker, const Norm &target) const;

  /**
   * @brief Check if a set of arguments is admissible.
   * @param arguments_ids A set of argument identifiers.
   * @param arguments A map of norm identifiers to Norm structures representing
   * the arguments.
   * @return True if the set is admissible, false otherwise.
   */
  bool is_admissible(const std::unordered_set<std::string> &arguments_ids,
                     const std::map<std::string, Norm> &arguments) const;

  /**
   * @brief Check if a set of arguments is conflict-free.
   * @param arguments_ids A set of argument identifiers.
   * @param arguments A map of norm identifiers to Norm structures representing
   * the arguments.
   * @return True if the set is conflict-free, false otherwise.
   */
  bool is_conflict_free(const std::unordered_set<std::string> &arguments_ids,
                        const std::map<std::string, Norm> &arguments) const;

  /**
   * @brief Check if a conclusion is defeated by any defeaters.
   * @param conclusion The conclusion to check.
   * @param defeaters A set of defeater identifiers.
   * @param contraries_map A map of contrary identifiers to sets of contrary
   * identifiers.
   */
  bool
  is_defeated_by(const std::string &conclusion,
                 const std::unordered_set<std::string> &defeaters,
                 const std::map<std::string, std::unordered_set<std::string>>
                     &contraries_map) const;

  /**
   * @brief Check if a set of arguments attacks all arguments outside the set.
   * @param S A set of argument identifiers.
   * @param arguments A map of norm identifiers to Norm structures representing
   * the arguments.
   * @return True if the set attacks all outside arguments, false otherwise.
   */
  bool attacks_all_outside(const std::unordered_set<std::string> &S,
                           const std::map<std::string, Norm> &arguments) const;

  /**
   * @brief Check if there is a symmetric attack cycle among the arguments.
   * @param arguments A map of norm identifiers to Norm structures representing
   * the arguments.
   * @return True if a symmetric attack cycle exists, false otherwise.
   */
  bool has_symmetric_cycle(const std::map<std::string, Norm> &arguments) const;

  /**
   * @brief Retrieve the priority value for a given identifier.
   * @param id The identifier for which to get the priority value.
   * @return The priority value associated with the identifier.
   */
  int get_priority_value(const std::string &id) const;

  /**
   * @brief Get the keys of norms that attack a given target norm.
   * @param target The target Norm structure.
   * @param arguments A map of norm identifiers to Norm structures representing
   * the arguments.
   * @return A vector of norm identifiers that attack the target norm.
   */
  std::vector<std::string>
  get_attacker_keys(const Norm &target,
                    const std::map<std::string, Norm> &arguments) const;

  /**
   * @brief Retrieve all norms from the given arguments map.
   * @param arguments A map of norm identifiers to Norm structures.
   * @return A vector of all Norm structures in the arguments map.
   */
  std::vector<Norm>
  get_all_norms(const std::map<std::string, Norm> &arguments) const;

  /**
   * @brief Build the extension based on accepted conclusions and arguments.
   * @param accepted_conclusions A set of accepted conclusion identifiers.
   * @param arguments A map of norm identifiers to Norm structures representing
   * the arguments.
   * @return A pair of vectors containing accepted and rejected Norm structures.
   */
  std::pair<std::vector<Norm>, std::vector<Norm>>
  build_extension(const std::unordered_set<std::string> &accepted_conclusions,
                  const std::map<std::string, Norm> &arguments) const;

  /**
   * @brief Find the candidate with the highest priority.
   * @param candidates A vector of candidate identifiers.
   * @return The identifier of the candidate with the highest priority.
   */
  std::string
  find_highest_priority(const std::vector<std::string> &candidates) const;

  /// @brief Description of the Jiminy instance.
  std::string description_;
  /// @brief Map of fact identifiers to Fact structures.
  std::map<std::string, Fact> facts_;
  /// @brief Map of norm identifiers to Norm structures.
  std::map<std::string, Norm> norms_;
  /// @brief Map of contrary identifiers to Contrary structures.
  std::map<std::string, Contrary> contraries_;
  /// @brief Map of priority identifiers to Priority structures.
  std::map<std::string, Priority> priorities_;
};

} // namespace mini_jiminy

#endif // MINI_JIMINY__MINI_JIMINY_HPP
