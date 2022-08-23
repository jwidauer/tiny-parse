#pragma once

#include "tiny_parse.hpp"

namespace tiny_parse::built_in {

/**
 * @brief A parser that matches a single character.
 *
 * @tparam C The character to match.
 */
template <char C>
class TINY_PARSE_PUBLIC CharP : public Parser {
 public:
  constexpr size_t min_length() const override { return 1; }

 protected:
  constexpr std::string_view parse_it(
      const std::string_view& sv) const override {
    if (!sv.empty() && sv.front() == C) return sv.substr(1);
    return sv;
  }
};

/**
 * @brief A parser that matches any character in a given range.
 *
 * @tparam lower Lower bound of the range.
 * @tparam upper Upper bound of the range.
 */
template <char lower, char upper>
class TINY_PARSE_PUBLIC RangeP : public Parser {
 public:
  constexpr size_t min_length() const override { return 1; }

 protected:
  constexpr std::string_view parse_it(
      const std::string_view& sv) const override {
    if (!sv.empty() && sv.front() >= lower && sv.front() <= upper)
      return sv.substr(1);
    return sv;
  }
};

/**
 * @brief A parser that matches any single character.
 */
class TINY_PARSE_PUBLIC AnyP : public Parser {
 public:
  constexpr size_t min_length() const override { return 1; }

 protected:
  constexpr std::string_view parse_it(
      const std::string_view& sv) const override {
    if (!sv.empty()) return sv.substr(1);
    return sv;
  }
};

const auto digit = RangeP<'0', '9'>{};

const auto whole_number = ++digit;

const auto integer = ~CharP<'-'>{} & whole_number;

const auto decimal = integer & CharP<'.'>{} & whole_number;

const auto number = integer | decimal;

const auto lower_case_character = RangeP<'a', 'z'>{};

const auto upper_case_character = RangeP<'A', 'Z'>{};

const auto letter = lower_case_character | upper_case_character;

}  // namespace tiny_parse::built_in
