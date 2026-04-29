#include <gtest/gtest.h>

#include <algorithm>
#include <map>
#include <string>
#include <vector>

#include "mini_jiminy/mini_jiminy.hpp"

using namespace mini_jiminy;

//==============================================================================
// Helper functions to build test scenarios
//==============================================================================
namespace {

Fact make_fact(const std::string &id, const std::string &desc = "") {
  return Fact{id, desc.empty() ? "Fact " + id : desc};
}

Norm make_norm(const std::string &id, const std::vector<std::string> &body,
               const std::string &conclusion, NormType type,
               const std::string &stakeholder = "S",
               const std::string &desc = "") {
  return Norm{id,   body,        conclusion,
              type, stakeholder, desc.empty() ? "Norm " + id : desc};
}

Contrary make_contrary(const std::string &id,
                       const std::vector<std::string> &contraries,
                       const std::string &desc = "") {
  return Contrary{id, contraries, desc.empty() ? "Contrary " + id : desc};
}

Priority make_priority(const std::string &id, int value,
                       const std::string &desc = "") {
  return Priority{id, value, desc.empty() ? "Priority " + id : desc};
}

bool has_conclusion(const std::vector<Norm> &norms,
                    const std::string &conclusion) {
  return std::any_of(norms.begin(), norms.end(), [&conclusion](const Norm &n) {
    return n.conclusion == conclusion;
  });
}

} // namespace

//==============================================================================
// Test Fixture
//==============================================================================
class JiminyTest : public ::testing::Test {
protected:
  void SetUp() override {}

  void TearDown() override {}

  // Create a simple Jiminy instance with basic facts and norms
  std::unique_ptr<Jiminy> create_simple_jiminy() {
    std::map<std::string, Fact> facts = {{"w1", make_fact("w1")},
                                         {"w2", make_fact("w2")}};

    std::map<std::string, Norm> norms = {
        {"n1", make_norm("n1", {"w1"}, "i1", NormType::CONSTITUTIVE)},
        {"n2", make_norm("n2", {"i1"}, "d1", NormType::REGULATIVE)}};

    std::map<std::string, Contrary> contraries;
    std::map<std::string, Priority> priorities;

    return std::make_unique<Jiminy>("Test scenario", facts, norms, contraries,
                                    priorities);
  }
};

//==============================================================================
// Constructor and Getter Tests
//==============================================================================
TEST_F(JiminyTest, ConstructorInitializesCorrectly) {
  auto jiminy = create_simple_jiminy();

  EXPECT_EQ(jiminy->get_description(), "Test scenario");
  EXPECT_EQ(jiminy->get_facts().size(), 2);
  EXPECT_EQ(jiminy->get_norms().size(), 2);
  EXPECT_TRUE(jiminy->get_contraries().empty());
  EXPECT_TRUE(jiminy->get_priorities().empty());
}

TEST_F(JiminyTest, GetFactReturnsCorrectFact) {
  auto jiminy = create_simple_jiminy();

  Fact fact = jiminy->get_fact("w1");
  EXPECT_EQ(fact.id, "w1");
}

TEST_F(JiminyTest, GetFactThrowsForNonExistentFact) {
  auto jiminy = create_simple_jiminy();

  EXPECT_THROW(jiminy->get_fact("nonexistent"), std::out_of_range);
}

TEST_F(JiminyTest, GetNormReturnsCorrectNorm) {
  auto jiminy = create_simple_jiminy();

  Norm norm = jiminy->get_norm("n1");
  EXPECT_EQ(norm.id, "n1");
  EXPECT_EQ(norm.conclusion, "i1");
  EXPECT_EQ(norm.type, NormType::CONSTITUTIVE);
}

TEST_F(JiminyTest, GetNormThrowsForNonExistentNorm) {
  auto jiminy = create_simple_jiminy();

  EXPECT_THROW(jiminy->get_norm("nonexistent"), std::out_of_range);
}

TEST_F(JiminyTest, GetContraryThrowsForNonExistent) {
  auto jiminy = create_simple_jiminy();

  EXPECT_THROW(jiminy->get_contrary("nonexistent"), std::out_of_range);
}

TEST_F(JiminyTest, GetPriorityThrowsForNonExistent) {
  auto jiminy = create_simple_jiminy();

  EXPECT_THROW(jiminy->get_priority("nonexistent"), std::out_of_range);
}

//==============================================================================
// Argument Generation Tests
//==============================================================================
TEST_F(JiminyTest, ArgumentClosureGeneratesInstitutionalFacts) {
  // Test that constitutive norms derive new facts
  std::map<std::string, Fact> facts = {{"w1", make_fact("w1")}};

  std::map<std::string, Norm> norms = {
      {"n1", make_norm("n1", {"w1"}, "i1", NormType::CONSTITUTIVE)},
      {"n2", make_norm("n2", {"i1"}, "d1", NormType::REGULATIVE)}};

  Jiminy jiminy("", facts, norms, {}, {});

  std::map<std::string, Fact> context = {{"w1", make_fact("w1")}};
  auto args = jiminy.generate_arguments(context);

  // Should have both i1 (derived from constitutive) and d1 (regulative)
  EXPECT_TRUE(args.count("i1") > 0);
  EXPECT_TRUE(args.count("d1") > 0);
}

TEST_F(JiminyTest, ArgumentGenerationWithMultipleBodyFacts) {
  std::map<std::string, Fact> facts = {{"w1", make_fact("w1")},
                                       {"w2", make_fact("w2")},
                                       {"w3", make_fact("w3")}};

  std::map<std::string, Norm> norms = {
      {"n1", make_norm("n1", {"w1", "w2"}, "i1", NormType::CONSTITUTIVE)},
      {"n2", make_norm("n2", {"w1"}, "i2", NormType::CONSTITUTIVE)}};

  Jiminy jiminy("", facts, norms, {}, {});

  // Only w1 present - should only generate i2
  std::map<std::string, Fact> context1 = {{"w1", make_fact("w1")}};
  auto args1 = jiminy.generate_arguments(context1);
  EXPECT_TRUE(args1.count("i2") > 0);
  EXPECT_FALSE(args1.count("i1") > 0);

  // w1 and w2 present - should generate both
  std::map<std::string, Fact> context2 = {{"w1", make_fact("w1")},
                                          {"w2", make_fact("w2")}};
  auto args2 = jiminy.generate_arguments(context2);
  EXPECT_TRUE(args2.count("i1") > 0);
  EXPECT_TRUE(args2.count("i2") > 0);
}

TEST_F(JiminyTest, ArgumentGenerationChainedNorms) {
  // Test chained constitutive norms: w1 -> i1 -> i2 -> d1
  std::map<std::string, Fact> facts = {{"w1", make_fact("w1")}};

  std::map<std::string, Norm> norms = {
      {"n1", make_norm("n1", {"w1"}, "i1", NormType::CONSTITUTIVE)},
      {"n2", make_norm("n2", {"i1"}, "i2", NormType::CONSTITUTIVE)},
      {"n3", make_norm("n3", {"i2"}, "d1", NormType::REGULATIVE)}};

  Jiminy jiminy("", facts, norms, {}, {});

  std::map<std::string, Fact> context = {{"w1", make_fact("w1")}};
  auto args = jiminy.generate_arguments(context);

  EXPECT_TRUE(args.count("i1") > 0);
  EXPECT_TRUE(args.count("i2") > 0);
  EXPECT_TRUE(args.count("d1") > 0);
}

TEST_F(JiminyTest, PermissiveNormsIncluded) {
  std::map<std::string, Fact> facts = {{"w1", make_fact("w1")}};

  std::map<std::string, Norm> norms = {
      {"n1", make_norm("n1", {"w1"}, "p1", NormType::PERMISSIVE)}};

  Jiminy jiminy("", facts, norms, {}, {});

  std::map<std::string, Fact> context = {{"w1", make_fact("w1")}};
  auto args = jiminy.generate_arguments(context);

  EXPECT_TRUE(args.count("p1") > 0);
}

TEST_F(JiminyTest, EmptyContextGeneratesNoArguments) {
  auto jiminy = create_simple_jiminy();

  std::map<std::string, Fact> context;
  auto args = jiminy->generate_arguments(context);

  EXPECT_TRUE(args.empty());
}

//==============================================================================
// Grounded Semantics Tests
//==============================================================================
TEST_F(JiminyTest, GroundedSelectsUnattackedArgument) {
  // A attacks B, but A has no attackers → A is IN
  std::map<std::string, Fact> facts = {{"w", make_fact("w")}};

  std::map<std::string, Norm> norms = {
      {"nA", make_norm("nA", {"w"}, "A", NormType::REGULATIVE)},
      {"nB", make_norm("nB", {"w"}, "B", NormType::REGULATIVE)}};

  std::map<std::string, Contrary> contraries = {
      {"A", make_contrary("A", {"B"})}};

  Jiminy jiminy("", facts, norms, contraries, {});

  std::map<std::string, Fact> context = {{"w", make_fact("w")}};
  auto args = jiminy.generate_arguments(context);
  auto [accepted, rejected] =
      jiminy.compute_extension(args, Semantics::GROUNDED);

  EXPECT_TRUE(has_conclusion(accepted, "A"));
  EXPECT_TRUE(has_conclusion(rejected, "B"));
}

TEST_F(JiminyTest, GroundedMutualAttackResultsInEmptyExtension) {
  // A ↔ B → grounded picks none
  std::map<std::string, Fact> facts = {{"w", make_fact("w")}};

  std::map<std::string, Norm> norms = {
      {"nA", make_norm("nA", {"w"}, "A", NormType::REGULATIVE)},
      {"nB", make_norm("nB", {"w"}, "B", NormType::REGULATIVE)}};

  std::map<std::string, Contrary> contraries = {
      {"A", make_contrary("A", {"B"})}, {"B", make_contrary("B", {"A"})}};

  Jiminy jiminy("", facts, norms, contraries, {});

  std::map<std::string, Fact> context = {{"w", make_fact("w")}};
  auto args = jiminy.generate_arguments(context);
  auto [accepted, rejected] =
      jiminy.compute_extension(args, Semantics::GROUNDED);

  EXPECT_TRUE(accepted.empty());
  EXPECT_EQ(rejected.size(), 2);
}

TEST_F(JiminyTest, GroundedNoConflictsAcceptsAll) {
  std::map<std::string, Fact> facts = {{"w", make_fact("w")}};

  std::map<std::string, Norm> norms = {
      {"nA", make_norm("nA", {"w"}, "A", NormType::REGULATIVE)},
      {"nB", make_norm("nB", {"w"}, "B", NormType::REGULATIVE)}};

  Jiminy jiminy("", facts, norms, {}, {});

  std::map<std::string, Fact> context = {{"w", make_fact("w")}};
  auto args = jiminy.generate_arguments(context);
  auto [accepted, rejected] =
      jiminy.compute_extension(args, Semantics::GROUNDED);

  EXPECT_EQ(accepted.size(), 2);
  EXPECT_TRUE(rejected.empty());
}

//==============================================================================
// Preferred Semantics Tests
//==============================================================================
TEST_F(JiminyTest, PreferredSelectsMaximalConflictFreeSet) {
  // A attacks B; B does not attack A → Preferred chooses {A}
  std::map<std::string, Fact> facts = {{"w", make_fact("w")}};

  std::map<std::string, Norm> norms = {
      {"nA", make_norm("nA", {"w"}, "A", NormType::REGULATIVE)},
      {"nB", make_norm("nB", {"w"}, "B", NormType::REGULATIVE)}};

  std::map<std::string, Contrary> contraries = {
      {"A", make_contrary("A", {"B"})}};

  Jiminy jiminy("", facts, norms, contraries, {});

  std::map<std::string, Fact> context = {{"w", make_fact("w")}};
  auto args = jiminy.generate_arguments(context);
  auto [accepted, rejected] =
      jiminy.compute_extension(args, Semantics::PREFERRED);

  EXPECT_TRUE(has_conclusion(accepted, "A"));
}

TEST_F(JiminyTest, PreferredMutualAttackReturnsAtLeastOne) {
  // A ↔ B → preferred extensions are {A} and {B}
  std::map<std::string, Fact> facts = {{"w", make_fact("w")}};

  std::map<std::string, Norm> norms = {
      {"nA", make_norm("nA", {"w"}, "A", NormType::REGULATIVE)},
      {"nB", make_norm("nB", {"w"}, "B", NormType::REGULATIVE)}};

  std::map<std::string, Contrary> contraries = {
      {"A", make_contrary("A", {"B"})}, {"B", make_contrary("B", {"A"})}};

  Jiminy jiminy("", facts, norms, contraries, {});

  std::map<std::string, Fact> context = {{"w", make_fact("w")}};
  auto args = jiminy.generate_arguments(context);
  auto [accepted, rejected] =
      jiminy.compute_extension(args, Semantics::PREFERRED);

  // Implementation returns union, so at least 1 argument accepted
  EXPECT_GE(accepted.size(), 1);
}

//==============================================================================
// Stable Semantics Tests
//==============================================================================
TEST_F(JiminyTest, StableAcceptsOneAndRejectsAttacked) {
  // A attacks B → stable = {A}
  std::map<std::string, Fact> facts = {{"w", make_fact("w")}};

  std::map<std::string, Norm> norms = {
      {"nA", make_norm("nA", {"w"}, "A", NormType::REGULATIVE)},
      {"nB", make_norm("nB", {"w"}, "B", NormType::REGULATIVE)}};

  std::map<std::string, Contrary> contraries = {
      {"A", make_contrary("A", {"B"})}};

  Jiminy jiminy("", facts, norms, contraries, {});

  std::map<std::string, Fact> context = {{"w", make_fact("w")}};
  auto args = jiminy.generate_arguments(context);
  auto [accepted, rejected] = jiminy.compute_extension(args, Semantics::STABLE);

  EXPECT_TRUE(has_conclusion(accepted, "A"));
  EXPECT_FALSE(has_conclusion(accepted, "B"));
}

TEST_F(JiminyTest, StableNoExtensionWhenSymmetricCycleExists) {
  // A ↔ B has no stable extension
  std::map<std::string, Fact> facts = {{"w", make_fact("w")}};

  std::map<std::string, Norm> norms = {
      {"nA", make_norm("nA", {"w"}, "A", NormType::REGULATIVE)},
      {"nB", make_norm("nB", {"w"}, "B", NormType::REGULATIVE)}};

  std::map<std::string, Contrary> contraries = {
      {"A", make_contrary("A", {"B"})}, {"B", make_contrary("B", {"A"})}};

  Jiminy jiminy("", facts, norms, contraries, {});

  std::map<std::string, Fact> context = {{"w", make_fact("w")}};
  auto args = jiminy.generate_arguments(context);
  auto [accepted, rejected] = jiminy.compute_extension(args, Semantics::STABLE);

  EXPECT_TRUE(accepted.empty());
  EXPECT_EQ(rejected.size(), 2);
}

TEST_F(JiminyTest, StableAllowsAsymmetricAttacks) {
  std::map<std::string, Fact> facts = {{"w", make_fact("w")}};

  std::map<std::string, Norm> norms = {
      {"nA", make_norm("nA", {"w"}, "A", NormType::REGULATIVE)},
      {"nB", make_norm("nB", {"w"}, "B", NormType::REGULATIVE)}};

  // A attacks B, B does not attack A
  std::map<std::string, Contrary> contraries = {
      {"A", make_contrary("A", {"B"})}};

  Jiminy jiminy("", facts, norms, contraries, {});

  std::map<std::string, Fact> context = {{"w", make_fact("w")}};
  auto args = jiminy.generate_arguments(context);
  auto [accepted, rejected] = jiminy.compute_extension(args, Semantics::STABLE);

  EXPECT_TRUE(has_conclusion(accepted, "A"));
  EXPECT_FALSE(has_conclusion(accepted, "B"));
}

TEST_F(JiminyTest, StableAcyclicFrameworkAcceptsAll) {
  std::map<std::string, Fact> facts = {{"w", make_fact("w")}};

  std::map<std::string, Norm> norms = {
      {"nA", make_norm("nA", {"w"}, "A", NormType::REGULATIVE)},
      {"nB", make_norm("nB", {"w"}, "B", NormType::REGULATIVE)}};

  // No contraries
  Jiminy jiminy("", facts, norms, {}, {});

  std::map<std::string, Fact> context = {{"w", make_fact("w")}};
  auto args = jiminy.generate_arguments(context);
  auto [accepted, rejected] = jiminy.compute_extension(args, Semantics::STABLE);

  EXPECT_EQ(accepted.size(), 2);
  EXPECT_TRUE(rejected.empty());
}

//==============================================================================
// Priority Semantics Tests
//==============================================================================
TEST_F(JiminyTest, PriorityPrefersHigherPriority) {
  std::map<std::string, Fact> facts = {{"w", make_fact("w")}};

  std::map<std::string, Norm> norms = {
      {"nA", make_norm("nA", {"w"}, "a", NormType::REGULATIVE)},
      {"nB", make_norm("nB", {"w"}, "b", NormType::REGULATIVE)}};

  std::map<std::string, Contrary> contraries = {
      {"a", make_contrary("a", {"b"})}, {"b", make_contrary("b", {"a"})}};

  std::map<std::string, Priority> priorities = {{"a", make_priority("a", 5)},
                                                {"b", make_priority("b", 1)}};

  Jiminy jiminy("", facts, norms, contraries, priorities);

  std::map<std::string, Fact> context = {{"w", make_fact("w")}};
  auto args = jiminy.generate_arguments(context);
  auto [accepted, rejected] =
      jiminy.compute_extension(args, Semantics::PRIORITY);

  EXPECT_TRUE(has_conclusion(accepted, "a"));
  EXPECT_FALSE(has_conclusion(accepted, "b"));
  EXPECT_TRUE(has_conclusion(rejected, "b"));
}

TEST_F(JiminyTest, PriorityWithEqualPrioritiesUsesLexicographic) {
  std::map<std::string, Fact> facts = {{"w", make_fact("w")}};

  std::map<std::string, Norm> norms = {
      {"nA", make_norm("nA", {"w"}, "a", NormType::REGULATIVE)},
      {"nB", make_norm("nB", {"w"}, "b", NormType::REGULATIVE)}};

  std::map<std::string, Contrary> contraries = {
      {"a", make_contrary("a", {"b"})}, {"b", make_contrary("b", {"a"})}};

  // Same priority - lexicographic order should prefer "a"
  std::map<std::string, Priority> priorities = {{"a", make_priority("a", 5)},
                                                {"b", make_priority("b", 5)}};

  Jiminy jiminy("", facts, norms, contraries, priorities);

  std::map<std::string, Fact> context = {{"w", make_fact("w")}};
  auto args = jiminy.generate_arguments(context);
  auto [accepted, rejected] =
      jiminy.compute_extension(args, Semantics::PRIORITY);

  EXPECT_TRUE(has_conclusion(accepted, "a"));
}

TEST_F(JiminyTest, PriorityWithNoPrioritiesDefaultsToZero) {
  std::map<std::string, Fact> facts = {{"w", make_fact("w")}};

  std::map<std::string, Norm> norms = {
      {"nA", make_norm("nA", {"w"}, "a", NormType::REGULATIVE)},
      {"nB", make_norm("nB", {"w"}, "b", NormType::REGULATIVE)}};

  std::map<std::string, Contrary> contraries = {
      {"a", make_contrary("a", {"b"})}, {"b", make_contrary("b", {"a"})}};

  // No priorities defined
  Jiminy jiminy("", facts, norms, contraries, {});

  std::map<std::string, Fact> context = {{"w", make_fact("w")}};
  auto args = jiminy.generate_arguments(context);
  auto [accepted, rejected] =
      jiminy.compute_extension(args, Semantics::PRIORITY);

  // At least one should be accepted (lexicographic order)
  EXPECT_GE(accepted.size(), 1);
}

TEST_F(JiminyTest, PriorityWithNoConflictsAcceptsAll) {
  std::map<std::string, Fact> facts = {{"w", make_fact("w")}};

  std::map<std::string, Norm> norms = {
      {"nA", make_norm("nA", {"w"}, "a", NormType::REGULATIVE)},
      {"nB", make_norm("nB", {"w"}, "b", NormType::REGULATIVE)}};

  std::map<std::string, Priority> priorities = {{"a", make_priority("a", 5)},
                                                {"b", make_priority("b", 1)}};

  Jiminy jiminy("", facts, norms, {}, priorities);

  std::map<std::string, Fact> context = {{"w", make_fact("w")}};
  auto args = jiminy.generate_arguments(context);
  auto [accepted, rejected] =
      jiminy.compute_extension(args, Semantics::PRIORITY);

  EXPECT_EQ(accepted.size(), 2);
  EXPECT_TRUE(rejected.empty());
}

//==============================================================================
// Edge Cases and Complex Scenarios
//==============================================================================
TEST_F(JiminyTest, EmptyNormsReturnsEmpty) {
  std::map<std::string, Fact> facts = {{"w", make_fact("w")}};

  Jiminy jiminy("", facts, {}, {}, {});

  std::map<std::string, Fact> context = {{"w", make_fact("w")}};
  auto args = jiminy.generate_arguments(context);

  EXPECT_TRUE(args.empty());
}

TEST_F(JiminyTest, ComplexScenarioWithMixedNormTypes) {
  std::map<std::string, Fact> facts = {{"w1", make_fact("w1")},
                                       {"w2", make_fact("w2")}};

  std::map<std::string, Norm> norms = {
      {"c1", make_norm("c1", {"w1"}, "i1", NormType::CONSTITUTIVE)},
      {"c2", make_norm("c2", {"w2"}, "i2", NormType::CONSTITUTIVE)},
      {"r1", make_norm("r1", {"i1"}, "d1", NormType::REGULATIVE)},
      {"r2", make_norm("r2", {"i2"}, "d2", NormType::REGULATIVE)},
      {"p1", make_norm("p1", {"i1", "i2"}, "p_both", NormType::PERMISSIVE)}};

  std::map<std::string, Contrary> contraries = {
      {"d1", make_contrary("d1", {"d2"})}, {"d2", make_contrary("d2", {"d1"})}};

  std::map<std::string, Priority> priorities = {{"d1", make_priority("d1", 10)},
                                                {"d2", make_priority("d2", 5)}};

  Jiminy jiminy("", facts, norms, contraries, priorities);

  std::map<std::string, Fact> context = {{"w1", make_fact("w1")},
                                         {"w2", make_fact("w2")}};
  auto args = jiminy.generate_arguments(context);
  auto [accepted, rejected] =
      jiminy.compute_extension(args, Semantics::PRIORITY);

  // i1, i2 should be generated (constitutive)
  EXPECT_TRUE(args.count("i1") > 0);
  EXPECT_TRUE(args.count("i2") > 0);

  // d1 should win over d2 due to higher priority
  EXPECT_TRUE(has_conclusion(accepted, "d1"));
  EXPECT_TRUE(has_conclusion(rejected, "d2"));

  // p_both requires both i1 and i2, should be present
  EXPECT_TRUE(args.count("p_both") > 0);
}

TEST_F(JiminyTest, ThreeWayConflictWithPriorities) {
  std::map<std::string, Fact> facts = {{"w", make_fact("w")}};

  std::map<std::string, Norm> norms = {
      {"nA", make_norm("nA", {"w"}, "A", NormType::REGULATIVE)},
      {"nB", make_norm("nB", {"w"}, "B", NormType::REGULATIVE)},
      {"nC", make_norm("nC", {"w"}, "C", NormType::REGULATIVE)}};

  // A beats B, B beats C, C beats A (rock-paper-scissors)
  std::map<std::string, Contrary> contraries = {
      {"A", make_contrary("A", {"B"})},
      {"B", make_contrary("B", {"C"})},
      {"C", make_contrary("C", {"A"})}};

  std::map<std::string, Priority> priorities = {{"A", make_priority("A", 3)},
                                                {"B", make_priority("B", 2)},
                                                {"C", make_priority("C", 1)}};

  Jiminy jiminy("", facts, norms, contraries, priorities);

  std::map<std::string, Fact> context = {{"w", make_fact("w")}};
  auto args = jiminy.generate_arguments(context);

  // Test all semantics produce some result
  auto [grounded_acc, grounded_rej] =
      jiminy.compute_extension(args, Semantics::GROUNDED);
  auto [preferred_acc, preferred_rej] =
      jiminy.compute_extension(args, Semantics::PREFERRED);
  auto [priority_acc, priority_rej] =
      jiminy.compute_extension(args, Semantics::PRIORITY);

  // Total should always equal the number of arguments
  EXPECT_EQ(grounded_acc.size() + grounded_rej.size(), args.size());
  EXPECT_EQ(preferred_acc.size() + preferred_rej.size(), args.size());
  EXPECT_EQ(priority_acc.size() + priority_rej.size(), args.size());
}

//==============================================================================
// Compute Extension Dispatcher Test
//==============================================================================
TEST_F(JiminyTest, ComputeExtensionDispatchesToCorrectSemantics) {
  std::map<std::string, Fact> facts = {{"w", make_fact("w")}};

  std::map<std::string, Norm> norms = {
      {"nA", make_norm("nA", {"w"}, "A", NormType::REGULATIVE)}};

  Jiminy jiminy("", facts, norms, {}, {});

  std::map<std::string, Fact> context = {{"w", make_fact("w")}};
  auto args = jiminy.generate_arguments(context);

  // All semantics should accept the single unattacked argument
  auto [gr_acc, gr_rej] = jiminy.compute_extension(args, Semantics::GROUNDED);
  auto [pr_acc, pr_rej] = jiminy.compute_extension(args, Semantics::PREFERRED);
  auto [st_acc, st_rej] = jiminy.compute_extension(args, Semantics::STABLE);
  auto [pri_acc, pri_rej] = jiminy.compute_extension(args, Semantics::PRIORITY);

  EXPECT_EQ(gr_acc.size(), 1);
  EXPECT_EQ(pr_acc.size(), 1);
  EXPECT_EQ(st_acc.size(), 1);
  EXPECT_EQ(pri_acc.size(), 1);
}

int main(int argc, char **argv) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
