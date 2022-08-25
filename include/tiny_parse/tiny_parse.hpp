#pragma once

#if defined _WIN32 || defined __CYGWIN__
#ifdef BUILDING_TINY_PARSE
#define TINY_PARSE_PUBLIC __declspec(dllexport)
#else
#define TINY_PARSE_PUBLIC __declspec(dllimport)
#endif
#else
#ifdef BUILDING_TINY_PARSE
#define TINY_PARSE_PUBLIC __attribute__((visibility("default")))
#else
#define TINY_PARSE_PUBLIC
#endif
#endif

#include <algorithm>
#include <functional>
#include <string>
#include <string_view>

namespace tiny_parse {

using TINY_PARSE_PUBLIC Consumer = std::function<void(const std::string_view&)>;

class TINY_PARSE_PUBLIC Parser {
 public:
  Parser() = default;
  virtual ~Parser() = default;
  explicit Parser(const Consumer& consumer) : consumer_{consumer} {}

  struct Result {
    std::string_view value;
    bool success;

    explicit operator bool() const { return success; }
    bool operator==(const Result& other) const {
      return value == other.value && success == other.success;
    }
    friend std::ostream& operator<<(std::ostream& os, const Result& result);
  };

  /**
   * @brief Set the consumer of the parsed string.
   *
   * @param consumer The consumer to invoce on a successful parse.
   */
  void consumer(const Consumer& consumer) { consumer_ = consumer; }

  /**
   * @brief Parse the given string and apply the consumer on a full parse
   *
   * @param sv The string to parse
   * @return Parser::Result The unparsed rest of the input
   */
  inline Result parse(const std::string_view& sv) const {
    const auto result = parse_it(sv);

    if (consumer_ && result.success)
      consumer_(sv.substr(0, sv.size() - result.value.size()));

    return result;
  }

  /**
   * @brief The minimum number of parsed characters that constitute a full
   * parse
   */
  virtual size_t min_length() const = 0;

 protected:
  virtual Result parse_it(const std::string_view& sv) const = 0;

 private:
  Consumer consumer_;
};

inline TINY_PARSE_PUBLIC Parser::Result operator>>(const std::string_view& sv,
                                                   const Parser& parser) {
  return parser.parse(sv);
}

std::ostream& operator<<(std::ostream& os, const Parser::Result& result) {
  const std::string s = "{\"" + std::string{result.value} + "\", " +
                        (result.success ? "true" : "false") + "}";
  return os << s;
}

inline TINY_PARSE_PUBLIC Parser::Result operator>>(const Parser::Result& result,
                                                   const Parser& parser) {
  return parser.parse(result.value);
}

template <class T, class S>
class TINY_PARSE_PUBLIC Or : public Parser {
 public:
  Or(const T& p1, const S& p2) : parser1_{p1}, parser2_{p2} {}

  constexpr size_t min_length() const override {
    return std::min(parser1_.min_length(), parser2_.min_length());
  }

 protected:
  constexpr Parser::Result parse_it(const std::string_view& sv) const override {
    if (const auto result = sv >> parser1_; result.success) return result;
    return sv >> parser2_;
  }

 private:
  T parser1_;
  S parser2_;
};

template <class T, class S>
constexpr TINY_PARSE_PUBLIC Or<T, S> operator|(const T& p1, const S& p2) {
  return Or<T, S>{p1, p2};
}

template <class T, class S>
class TINY_PARSE_PUBLIC Then : public Parser {
 public:
  Then(const T& p1, const S& p2) : parser1_{p1}, parser2_{p2} {}

  constexpr size_t min_length() const override {
    return parser1_.min_length() + parser2_.min_length();
  }

 protected:
  constexpr Parser::Result parse_it(const std::string_view& sv) const override {
    auto result = sv >> parser1_;

    if (!result.success) return {sv, false};
    result = result >> parser2_;

    if (!result.success) return {sv, false};
    return result;
  }

 private:
  T parser1_;
  S parser2_;
};

template <class T, class S>
constexpr TINY_PARSE_PUBLIC Then<T, S> operator&(const T& p1, const S& p2) {
  return Then<T, S>{p1, p2};
}

template <class T>
class TINY_PARSE_PUBLIC More : public Parser {
 public:
  explicit More(const T& parser) : parser_{parser} {}

  constexpr size_t min_length() const override { return 0; }

 protected:
  constexpr Parser::Result parse_it(const std::string_view& sv) const override {
    auto result = sv >> parser_;
    while (result.success) {
      result = result >> parser_;
    }

    return result;
  }

 private:
  template <class U>
  friend class GreaterThan;

  T parser_;
};

template <class T>
constexpr TINY_PARSE_PUBLIC More<T> operator++(const T& parser) {
  return More<T>{parser};
}

/**
 * @brief A parser that optionally matches the given parser.
 *
 * @tparam T The parser to match
 */
template <class T>
class TINY_PARSE_PUBLIC Optional : public Parser {
 public:
  explicit Optional(const T& parser) : parser_{parser} {}

  constexpr size_t min_length() const override { return 0; }

 protected:
  constexpr Parser::Result parse_it(const std::string_view& sv) const override {
    return {parser_.parse(sv).value, true};
  }

 private:
  T parser_;
};

template <class T>
constexpr TINY_PARSE_PUBLIC Optional<T> operator~(const T& parser) {
  return Optional<T>{parser};
}

template <class T>
class TINY_PARSE_PUBLIC GreaterThan : public Parser {
 public:
  GreaterThan(size_t min, const T& parser) : min_{min}, parser_{parser} {}

  constexpr size_t min_length() const override {
    return min_ * parser_.min_length();
  }

 protected:
  constexpr Parser::Result parse_it(const std::string_view& sv) const override {
    size_t i = 0;
    auto result = sv >> parser_;
    while (result.success) {
      ++i;
      result = result >> parser_;
    }
    return (min_ < i) ? Parser::Result{result.value, true}
                      : Parser::Result{sv, false};
  }

 private:
  const size_t min_;
  T parser_;
};

template <class T>
constexpr TINY_PARSE_PUBLIC GreaterThan<T> operator<(size_t minimum,
                                                     const T& parser) {
  return GreaterThan<T>{minimum, parser};
}

template <class T>
constexpr TINY_PARSE_PUBLIC GreaterThan<T> operator>(const T& parser,
                                                     size_t minimum) {
  return GreaterThan<T>{minimum, parser};
}

template <class T>
class TINY_PARSE_PUBLIC LessThan : public Parser {
 public:
  LessThan(size_t max, const T& parser) : max_{max}, parser_{parser} {}
  constexpr size_t min_length() const override { return 0; }

 protected:
  constexpr Parser::Result parse_it(const std::string_view& sv) const override {
    auto result = sv >> parser_;
    auto success = result.success;
    // Start at 2 because we already ran the parser once and want to stop at
    // max_ - 1
    size_t i = 2;
    for (; result.success && i < max_; ++i) {
      result = result >> parser_;
      success |= result.success;
    }

    return {result.value, success};
  }

 private:
  const size_t max_;
  T parser_;
};

template <class T>
constexpr TINY_PARSE_PUBLIC LessThan<T> operator<(const T& parser,
                                                  size_t maximum) {
  return LessThan<T>{maximum, parser};
}

template <class T>
constexpr TINY_PARSE_PUBLIC LessThan<T> operator>(size_t maximum,
                                                  const T& parser) {
  return LessThan<T>{maximum, parser};
}

}  // namespace tiny_parse
