#include <tiny_parse/built_in.hpp>
#include <tiny_parse/tiny_parse.hpp>

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"

TEST_SUITE_BEGIN("tiny_parse");

TEST_CASE("CharP") {
  using namespace tiny_parse;
  using namespace tiny_parse::built_in;

  auto parser = CharP<'a'>{};
  CHECK(parser.min_length() == 1);
  CHECK(parser.parse("a") == "");
  CHECK(parser.parse("b") == "b");
  CHECK(parser.parse("") == "");
}

TEST_CASE("RangeP") {
  using namespace tiny_parse;
  using namespace tiny_parse::built_in;

  auto parser = RangeP<'0', '9'>{};
  CHECK(parser.min_length() == 1);
  CHECK(parser.parse("0") == "");
  CHECK(parser.parse("9") == "");
  CHECK(parser.parse("a") == "a");
  CHECK(parser.parse("") == "");
}

TEST_CASE("AnyP") {
  using namespace tiny_parse;
  using namespace tiny_parse::built_in;

  auto parser = AnyP{};
  CHECK(parser.min_length() == 1);
  CHECK(parser.parse("a") == "");
  CHECK(parser.parse("9") == "");
  CHECK(parser.parse("") == "");
}

TEST_CASE("Then") {
  using namespace tiny_parse;
  using namespace tiny_parse::built_in;

  auto parser = CharP<'a'>{} & CharP<'b'>{};
  CHECK(parser.min_length() == 2);
  CHECK(parser.parse("ab") == "");
  CHECK(parser.parse("a") == "a");
  CHECK(parser.parse("b") == "b");
  CHECK(parser.parse("") == "");
}

TEST_CASE("Or") {
  using namespace tiny_parse;
  using namespace tiny_parse::built_in;

  auto parser = CharP<'a'>{} | CharP<'b'>{};
  CHECK(parser.min_length() == 1);
  CHECK(parser.parse("a") == "");
  CHECK(parser.parse("b") == "");
  CHECK(parser.parse("c") == "c");
  CHECK(parser.parse("") == "");
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

TEST_CASE("Repeat") {
  using namespace tiny_parse;
  using namespace tiny_parse::built_in;

  auto parser = repeat<0, 3>(CharP<'a'>{});
  CHECK(parser.min_length() == 0);
  CHECK(parser.parse("aaaa") == "a");
  CHECK(parser.parse("aaa") == "");
  CHECK(parser.parse("aa") == "");
  CHECK(parser.parse("a") == "");
  CHECK(parser.parse("") == "");
  CHECK(parser.parse("b") == "b");
}

TEST_SUITE_END();
