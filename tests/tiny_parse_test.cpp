#include <tiny_parse/built_in.hpp>
#include <tiny_parse/tiny_parse.hpp>

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest.h>

#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>

TEST_SUITE_BEGIN("tiny_parse");

TEST_CASE("CharP") {
  using namespace tiny_parse;
  using namespace tiny_parse::built_in;

  const auto parser = CharP<'a'>{};
  CHECK(parser.min_length() == 1);
  CHECK(parser.parse("a") == Result{"", true});
  CHECK(parser.parse("b") == Result{"b", false});
  CHECK(parser.parse("") == Result{"", false});
}

TEST_CASE("RangeP") {
  using namespace tiny_parse;
  using namespace tiny_parse::built_in;

  const auto parser = RangeP<'0', '9'>{};
  CHECK(parser.min_length() == 1);
  CHECK(parser.parse("0") == Result{"", true});
  CHECK(parser.parse("9") == Result{"", true});
  CHECK(parser.parse("a") == Result{"a", false});
  CHECK(parser.parse(".") == Result{".", false});
  CHECK(parser.parse("") == Result{"", false});
}

TEST_CASE("AnyP") {
  using namespace tiny_parse;
  using namespace tiny_parse::built_in;

  const auto parser = AnyP{};
  CHECK(parser.min_length() == 1);
  CHECK(parser.parse("a") == Result{"", true});
  CHECK(parser.parse("9") == Result{"", true});
  CHECK(parser.parse("") == Result{"", false});
}

template <bool throws = false>
struct Consumer {
  explicit Consumer(std::string expected) : expected_{std::move(expected)} {}
  ~Consumer() = default;

  Consumer(const Consumer&) = delete;
  Consumer& operator=(const Consumer&) = delete;

  void consume(std::string_view sv) {
    called_ = true;
    CHECK(sv == expected_);
    if constexpr (throws) throw std::runtime_error{"error"};
  }

  [[nodiscard]] bool was_called() const noexcept { return called_; }

 private:
  bool called_{false};
  const std::string expected_;
};

TEST_CASE("Consumer") {
  namespace tp = tiny_parse;
  namespace tpi = tiny_parse::built_in;

  SUBCASE("Validity") {
    Consumer consumer{"a"};
    const auto callback = [&](std::string_view sv) { consumer.consume(sv); };
    auto parser = tpi::CharP<'a'>{}.consumer(callback);

    SUBCASE("Valid case") {
      const auto res = parser.parse("a");
      CHECK(consumer.was_called());
      CHECK(res == tp::Result{"", true});
    }

    SUBCASE("Invalid case") {
      const auto res = parser.parse("b");
      CHECK(!consumer.was_called());
      CHECK(res == tp::Result{"b", false});
    }
  }

  SUBCASE("Consumer throws") {
    Consumer<true> consumer{"a"};
    const auto callback = [&](std::string_view sv) { consumer.consume(sv); };
    auto throwing_parser = tpi::CharP<'a'>{}.consumer(callback);

    SUBCASE("simple parser") {
      auto parser = throwing_parser;
      SUBCASE("Valid case") {
        CHECK_THROWS_AS({ [[maybe_unused]] const auto res = parser.parse("a"); },
                        std::runtime_error);
        CHECK(consumer.was_called());
      }

      SUBCASE("Invalid case") {
        const auto res = parser.parse("b");
        CHECK(!consumer.was_called());
        CHECK(res == tp::Result{"b", false});
      }
    }

    SUBCASE("parser with a then") {
      SUBCASE("throwing first") {
        auto parser = throwing_parser & tpi::CharP<'b'>{};

        SUBCASE("Valid case") {
          CHECK_THROWS_AS({ [[maybe_unused]] const auto res = parser.parse("ab"); },
                          std::runtime_error);
          CHECK(consumer.was_called());
        }

        SUBCASE("Invalid case") {
          const auto res = parser.parse("b");
          CHECK(!consumer.was_called());
          CHECK(res == tp::Result{"b", false});
        }
      }

      SUBCASE("throwing second") {
        auto parser = tpi::CharP<'b'>{} & throwing_parser;

        SUBCASE("Valid case") {
          CHECK_THROWS_AS({ [[maybe_unused]] const auto res = parser.parse("ba"); },
                          std::runtime_error);
          CHECK(consumer.was_called());
        }

        SUBCASE("Invalid case") {
          const auto res = parser.parse("a");
          CHECK(!consumer.was_called());
          CHECK(res == tp::Result{"a", false});
        }
      }
    }
  }
}

TEST_CASE("Or") {
  using namespace tiny_parse;
  using namespace tiny_parse::built_in;

  const auto parser = CharP<'a'>{} | CharP<'b'>{};
  CHECK(parser.min_length() == 1);
  CHECK(parser.parse("a") == Result{"", true});
  CHECK(parser.parse("b") == Result{"", true});
  CHECK(parser.parse("c") == Result{"c", false});
  CHECK(parser.parse("") == Result{"", false});
}

TEST_CASE("Then") {
  using namespace tiny_parse;
  using namespace tiny_parse::built_in;

  const auto parser = CharP<'a'>{} & CharP<'b'>{};
  CHECK(parser.min_length() == 2);
  CHECK(parser.parse("ab") == Result{"", true});
  CHECK(parser.parse("a") == Result{"a", false});
  CHECK(parser.parse("b") == Result{"b", false});
  CHECK(parser.parse("") == Result{"", false});
}

TEST_CASE("Optional") {
  using namespace tiny_parse;
  using namespace tiny_parse::built_in;

  const auto parser = ~CharP<'a'>{};
  CHECK(parser.min_length() == 0);
  CHECK(parser.parse("aa") == Result{"a", true});
  CHECK(parser.parse("a") == Result{"", true});
  CHECK(parser.parse("") == Result{"", true});
}

TEST_CASE("Many") {
  using namespace tiny_parse;
  using namespace tiny_parse::built_in;

  SUBCASE("operator*") {
    const auto parser = *CharP<'a'>{};
    CHECK(parser.min_length() == 0);
    CHECK(parser.parse("aaaab") == Result{"b", true});
    CHECK(parser.parse("aaaa") == Result{"", true});
    CHECK(parser.parse("aaa") == Result{"", true});
    CHECK(parser.parse("aa") == Result{"", true});
    CHECK(parser.parse("a") == Result{"", true});
    CHECK(parser.parse("") == Result{"", true});
  }
}

TEST_CASE("Times") {
  using namespace tiny_parse;
  using namespace tiny_parse::built_in;

  auto perform_checks = [](auto parser) {
    CHECK(parser.min_length() == 3);
    CHECK(parser.parse("aaba") == Result{"aaba", false});
    CHECK(parser.parse("aaaa") == Result{"a", true});
    CHECK(parser.parse("aaa") == Result{"", true});
    CHECK(parser.parse("aa") == Result{"aa", false});
    CHECK(parser.parse("a") == Result{"a", false});
    CHECK(parser.parse("") == Result{"", false});
  };

  SUBCASE("<parser> * 3") { perform_checks(CharP<'a'>{} * 3); }
  SUBCASE("3 * <parser>") { perform_checks(3 * CharP<'a'>{}); }
}

TEST_CASE("GreaterThan") {
  using namespace tiny_parse;
  using namespace tiny_parse::built_in;

  auto perform_checks = [](const auto& parser) {
    CHECK(parser.min_length() == 3);
    CHECK(parser.parse("aaaab") == Result{"b", true});
    CHECK(parser.parse("aaaa") == Result{"", true});
    CHECK(parser.parse("aaa") == Result{"", true});
    CHECK(parser.parse("aa") == Result{"aa", false});
    CHECK(parser.parse("a") == Result{"a", false});
    CHECK(parser.parse("") == Result{"", false});
  };

  SUBCASE("operator<") {
    const auto parser = 2 < CharP<'a'>{};
    perform_checks(parser);
  }

  SUBCASE("operator>") {
    const auto parser = CharP<'a'>{} > 2;
    perform_checks(parser);
  }

  SUBCASE("operator+") {
    const auto parser = +CharP<'a'>{};
    CHECK(parser.min_length() == 1);
    CHECK(parser.parse("aaaab") == Result{"b", true});
    CHECK(parser.parse("aaaa") == Result{"", true});
    CHECK(parser.parse("aaa") == Result{"", true});
    CHECK(parser.parse("aa") == Result{"", true});
    CHECK(parser.parse("a") == Result{"", true});
    CHECK(parser.parse("") == Result{"", false});
  }
}

TEST_CASE("LessThan") {
  using namespace tiny_parse;
  using namespace tiny_parse::built_in;

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
    const auto parser = CharP<'a'>{} < 3;
    perform_checks(parser);
  }

  SUBCASE("operator>") {
    const auto parser = 3 > CharP<'a'>{};
    perform_checks(parser);
  }
}

TEST_CASE("Result") {
  using namespace tiny_parse;
  using namespace tiny_parse::built_in;

  SUBCASE("operator bool") {
    SUBCASE("true") { CHECK(Result{"abc", true}); }

    SUBCASE("false") { CHECK_FALSE(Result{"abc", false}); }
  }

  SUBCASE("operator<<") {
    std::stringstream ss;

    SUBCASE("true") {
      ss << Result{"abc", true};
      CHECK(ss.str() == "{\"abc\", true}");
    }
    SUBCASE("false") {
      ss << Result{"abc", false};
      CHECK(ss.str() == "{\"abc\", false}");
    }
  }

  SUBCASE("operator==") {
    const Result result{"abc", true};
    SUBCASE("true") { CHECK(result == result); }

    SUBCASE("false") {
      CHECK_FALSE(result == Result{"abc", false});
      CHECK_FALSE(result == Result{"abcd", true});
      CHECK_FALSE(result == Result{"abcd", false});
    }
  }
}

TEST_SUITE_END();
