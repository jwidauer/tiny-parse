#include <tiny_parse/built_in.hpp>
#include <tiny_parse/tiny_parse.hpp>

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <iostream>

#include "doctest.h"

TEST_SUITE_BEGIN("tiny_parse");

TEST_CASE("CharP") {
  using namespace tiny_parse;
  using namespace tiny_parse::built_in;
  using Result = Parser::Result;

  auto parser = CharP<'a'>{};
  CHECK(parser.min_length() == 1);
  CHECK(parser.parse("a") == Result{"", true});
  CHECK(parser.parse("b") == Result{"b", false});
  CHECK(parser.parse("") == Result{"", false});
}

TEST_CASE("RangeP") {
  using namespace tiny_parse;
  using namespace tiny_parse::built_in;
  using Result = Parser::Result;

  auto parser = RangeP<'0', '9'>{};
  CHECK(parser.min_length() == 1);
  CHECK(parser.parse("0") == Result{"", true});
  CHECK(parser.parse("9") == Result{"", true});
  CHECK(parser.parse("a") == Result{"a", false});
  CHECK(parser.parse("") == Result{"", false});
}

TEST_CASE("AnyP") {
  using namespace tiny_parse;
  using namespace tiny_parse::built_in;
  using Result = Parser::Result;

  auto parser = AnyP{};
  CHECK(parser.min_length() == 1);
  CHECK(parser.parse("a") == Result{"", true});
  CHECK(parser.parse("9") == Result{"", true});
  CHECK(parser.parse("") == Result{"", false});
}

TEST_CASE("Then") {
  using namespace tiny_parse;
  using namespace tiny_parse::built_in;
  using Result = Parser::Result;

  auto parser = CharP<'a'>{} & CharP<'b'>{};
  CHECK(parser.min_length() == 2);
  CHECK(parser.parse("ab") == Result{"", true});
  CHECK(parser.parse("a") == Result{"a", false});
  CHECK(parser.parse("b") == Result{"b", false});
  CHECK(parser.parse("") == Result{"", false});
}

TEST_CASE("Or") {
  using namespace tiny_parse;
  using namespace tiny_parse::built_in;
  using Result = Parser::Result;

  auto parser = CharP<'a'>{} | CharP<'b'>{};
  CHECK(parser.min_length() == 1);
  CHECK(parser.parse("a") == Result{"", true});
  CHECK(parser.parse("b") == Result{"", true});
  CHECK(parser.parse("c") == Result{"c", false});
  CHECK(parser.parse("") == Result{"", false});
}

TEST_CASE("Optional") {
  using namespace tiny_parse;
  using namespace tiny_parse::built_in;
  using Result = Parser::Result;

  auto parser = ~CharP<'a'>{};
  CHECK(parser.min_length() == 0);
  CHECK(parser.parse("aa") == Result{"a", true});
  CHECK(parser.parse("a") == Result{"", true});
  CHECK(parser.parse("") == Result{"", true});
}

// TEST_CASE("More") {
//   using namespace tiny_parse;
//   using namespace tiny_parse::built_in;

//   auto parser = CharP<'a'>{} * 3;
//   CHECK(parser.min_length() == 3);
//   CHECK(parser.parse("aaa") == "");
//   CHECK(parser.parse("aa") == "aa");
//   CHECK(parser.parse("a") == "a");
//   CHECK(parser.parse("") == "");
// }

TEST_CASE("GreaterThan") {
  using namespace tiny_parse;
  using namespace tiny_parse::built_in;
  using Result = Parser::Result;

  auto perform_checks = [](const auto& parser) {
    CHECK(parser.min_length() == 2);
    CHECK(parser.parse("aaaab") == Result{"b", true});
    CHECK(parser.parse("aaaa") == Result{"", true});
    CHECK(parser.parse("aaa") == Result{"", true});
    CHECK(parser.parse("aa") == Result{"aa", false});
    CHECK(parser.parse("a") == Result{"a", false});
    CHECK(parser.parse("") == Result{"", false});
  };

  SUBCASE("operator<") {
    auto parser = 2 < CharP<'a'>{};
    perform_checks(parser);
  }

  SUBCASE("operator>") {
    auto parser = CharP<'a'>{} > 2;
    perform_checks(parser);
  }
}

TEST_CASE("LessThan") {
  using namespace tiny_parse;
  using namespace tiny_parse::built_in;
  using Result = Parser::Result;

  auto perform_checks = [](const auto& parser) {
    CHECK(parser.min_length() == 0);
    CHECK(parser.parse("baaaa") == Result{"baaaa", false});
    CHECK(parser.parse("aaaa") == Result{"aa", true});
    CHECK(parser.parse("aaa") == Result{"a", true});
    CHECK(parser.parse("aa") == Result{"", true});
    CHECK(parser.parse("a") == Result{"", true});
    CHECK(parser.parse("") == Result{"", false});
  };

  SUBCASE("operator<") {
    auto parser = CharP<'a'>{} < 3;
    perform_checks(parser);
  }

  SUBCASE("operator>") {
    auto parser = 3 > CharP<'a'>{};
    perform_checks(parser);
  }
}

TEST_SUITE_END();
