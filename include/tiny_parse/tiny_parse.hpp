#pragma once

#include <algorithm>
#include <functional>
#include <iomanip>
#include <string>
#include <string_view>

namespace tiny_parse {

/**
 * @brief A consumer is a function that will be called on the result of a
 * successful parse.
 */
using Consumer = std::function<void(std::string_view)>;

/**
 * @brief The result of a parse.
 *
 */
struct Result {
  /** @brief The remaining string after the parse. */
  std::string_view value;
  /** @brief Whether the parse was successful. */
  bool success;

  /**
   * @brief Implicit conversion to bool.
   *
   * @return true if the parse was successful.
   * @return false if the parse was not successful.
   */
  explicit operator bool() const noexcept { return success; }
  bool operator==(const Result& other) const noexcept {
    return value == other.value && success == other.success;
  }
  friend std::ostream& operator<<(std::ostream& os, const Result& result);
};

/** @brief The string conversion for a Result. */
inline std::ostream& operator<<(std::ostream& os, const Result& result) {
  using namespace std::literals;

  return os << "{"sv << std::quoted(result.value) << ", "sv
            << (result.success ? "true"sv : "false"sv) << "}"sv;
}

/**
 * @brief Abstract base class for parsers.
 *
 * This is mainly used to allow for runtime polymorphism, without the need to
 * specify all template parameters.
 */
class Parser {
 public:
  virtual ~Parser() = default;

  [[nodiscard]] virtual Result parse(const std::string_view& sv) const = 0;
};

/**
 * @brief The base parser class.
 */
template <class Derived>
class BaseParser : public Parser {
 public:
  BaseParser() = default;
  ~BaseParser() override = default;

  /**
   * @brief Create a copy of this parser.
   *
   * Useful for creating parsers from the built-in parsers.
   *
   * @return Derived A copy of this parser.
   */
  Derived copy() const noexcept { return Derived{*static_cast<const Derived*>(this)}; }

  /**
   * @brief Set the consumer of the parsed string.
   *
   * @param consumer The consumer to invoke on a successful parse.
   */
  Derived& consumer(const Consumer& consumer) noexcept {
    consumer_ = consumer;
    return *static_cast<Derived*>(this);
  }

  /**
   * @brief Parse the given string and apply the consumer on a full parse
   *
   * @param sv The string to parse
   * @return Result The result of the parse.
   */
  [[nodiscard]] inline Result parse(const std::string_view& sv) const final {
    const auto result = parse_it(sv);

    if (consumer_ && result.success) consumer_(sv.substr(0, sv.size() - result.value.size()));

    return result;
  }

  /**
   * @brief The minimum number of parsed characters that constitute a full
   * parse
   */
  [[nodiscard]] virtual size_t min_length() const noexcept = 0;

 protected:
  [[nodiscard]] virtual Result parse_it(const std::string_view& sv) const = 0;

 private:
  Consumer consumer_;
};

/** @relates BaseParser @brief Syntactic sugar for calling the parse function. */
template <class Derived>
inline Result operator>>(const std::string_view& sv, const BaseParser<Derived>& parser) {
  return parser.parse(sv);
}

/** @relates BaseParser @brief Syntactic sugar for calling the parse function. */
template <class Derived>
inline Result operator>>(const Result& result, const BaseParser<Derived>& parser) {
  return parser.parse(result.value);
}

/**
 * @brief A parser that matches one parser or the other.
 *
 * First tries the first parser and if it fails, tries the second parser.
 * If both fail, the result is a unsuccessful parse.
 *
 * @tparam T The first parser that will be tried.
 * @tparam S The second parser that will be tried.
 */
template <class T, class S>
class Or : public BaseParser<Or<T, S>> {
 public:
  Or(const T& p1, const S& p2) noexcept : parser1_{p1}, parser2_{p2} {}

  [[nodiscard]] size_t min_length() const noexcept override {
    return std::min(parser1_.min_length(), parser2_.min_length());
  }

 protected:
  [[nodiscard]] Result parse_it(const std::string_view& sv) const override {
    if (const auto result = sv >> parser1_; result.success) return result;
    return sv >> parser2_;
  }

 private:
  T parser1_;
  S parser2_;
};

/** @relates Or @brief Syntactic sugar for creating an Or parser. */
template <class T, class S>
constexpr Or<T, S> operator|(const T& p1, const S& p2) noexcept {
  return Or<T, S>{p1, p2};
}

/**
 * @brief A parser that matches a sequence of parsers.
 *
 * Only tries the second parser if the first one succeeds.
 *
 * @tparam T The first parser that will be tried.
 * @tparam S The second parser that will be tried.
 */
template <class T, class S>
class Then : public BaseParser<Then<T, S>> {
 public:
  Then(const T& p1, const S& p2) noexcept : parser1_{p1}, parser2_{p2} {}

  [[nodiscard]] size_t min_length() const noexcept override {
    return parser1_.min_length() + parser2_.min_length();
  }

 protected:
  [[nodiscard]] Result parse_it(const std::string_view& sv) const override {
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

/** @relates Then @brief Syntactic sugar for creating a Then parser. */
template <class T, class S>
constexpr Then<T, S> operator&(const T& p1, const S& p2) noexcept {
  return Then<T, S>{p1, p2};
}

/**
 * @brief A parser that optionally matches the given parser.
 *
 * @tparam T The parser to match.
 */
template <class T>
class Optional : public BaseParser<Optional<T>> {
 public:
  explicit Optional(const T& parser) noexcept : parser_{parser} {}

  [[nodiscard]] size_t min_length() const noexcept override { return 0; }

 protected:
  [[nodiscard]] Result parse_it(const std::string_view& sv) const override {
    return {parser_.parse(sv).value, true};
  }

 private:
  T parser_;
};

/** @relates Optional @brief Syntactic sugar for creating an Optional parser. */
template <class T>
constexpr Optional<T> operator~(const T& parser) noexcept {
  return Optional<T>{parser};
}

/**
 * @brief A parser that matches the given parser zero or more times.
 *
 * @tparam T The parser to match.
 */
template <class T>
class Many : public BaseParser<Many<T>> {
 public:
  explicit Many(const T& parser) noexcept : parser_{parser} {}

  [[nodiscard]] size_t min_length() const noexcept override { return 0; }

 protected:
  [[nodiscard]] Result parse_it(const std::string_view& sv) const override {
    auto result = sv >> parser_;
    while (result.success) {
      result = result >> parser_;
    }
    return {result.value, true};
  }

 private:
  T parser_;
};

/** @relates Many @brief Syntactic sugar for creating parser that matches zero or more
 * characters */
template <class T>
constexpr Many<T> operator*(const T& parser) noexcept {
  return Many<T>{parser};
}

/**
 * @brief A parser that matches the given parser an exact number of times.
 *
 * @tparam T The parser to match.
 */
template <class T>
class Times : public BaseParser<Times<T>> {
 public:
  Times(size_t times, const T& parser) noexcept : times_{times}, parser_{parser} {}

  [[nodiscard]] size_t min_length() const noexcept override {
    return parser_.min_length() * times_;
  }

 protected:
  [[nodiscard]] Result parse_it(const std::string_view& sv) const override {
    size_t i = 1;
    auto result = sv >> parser_;
    for (; result.success && i < times_; ++i) {
      result = result >> parser_;
    }

    return (i == times_ && result.success) ? result : Result{sv, false};
  }

 private:
  const size_t times_;
  T parser_;
};

/** @relates Times @brief Syntactic sugar for creating parser that matches an exact number of
 * times */
template <class T>
constexpr Times<T> operator*(size_t times, const T& parser) noexcept {
  return Times<T>{times, parser};
}

/** @brief Syntactic sugar for creating a parser that matches an exact number of
 * times */
template <class T>
constexpr Times<T> operator*(const T& parser, size_t times) noexcept {
  return Times<T>{times, parser};
}

/**
 * @brief A parser that matches the given parser more than a given number of
 * times.
 *
 * @tparam T The parser to match.
 */
template <class T>
class GreaterThan : public BaseParser<GreaterThan<T>> {
 public:
  GreaterThan(size_t min, const T& parser) noexcept : min_{min}, parser_{parser} {}

  [[nodiscard]] size_t min_length() const noexcept override {
    return (min_ + 1) * parser_.min_length();
  }

 protected:
  [[nodiscard]] Result parse_it(const std::string_view& sv) const override {
    size_t i = 0;
    auto result = sv >> parser_;
    while (result.success) {
      ++i;
      result = result >> parser_;
    }
    return (min_ < i) ? Result{result.value, true} : Result{sv, false};
  }

 private:
  const size_t min_;
  T parser_;
};

/** @relates GreaterThan @brief Syntactic sugar for creating a GreaterThan parser. */
template <class T>
constexpr GreaterThan<T> operator<(size_t minimum, const T& parser) noexcept {
  return GreaterThan<T>{minimum, parser};
}

/** @relates GreaterThan @brief Syntactic sugar for creating a GreaterThan parser. */
template <class T>
constexpr GreaterThan<T> operator>(const T& parser, size_t minimum) noexcept {
  return GreaterThan<T>{minimum, parser};
}

/** @relates GreaterThan @brief Syntactic sugar for creating parser that matches one or more
 * characters */
template <class T>
constexpr GreaterThan<T> operator+(const T& parser) noexcept {
  return GreaterThan<T>{0, parser};
}

/**
 * @brief A parser that matches the given parser less than a given number of
 * times.
 *
 * @tparam T The parser to match.
 */
template <class T>
class LessThan : public BaseParser<LessThan<T>> {
 public:
  LessThan(size_t max, const T& parser) noexcept : max_{max}, parser_{parser} {}
  [[nodiscard]] size_t min_length() const noexcept override { return 0; }

 protected:
  [[nodiscard]] Result parse_it(const std::string_view& sv) const override {
    auto result = sv >> parser_;
    auto success = result.success;
    // Start at 2 because we already ran the parser once and want to stop at
    // max_ - 1
    for (size_t i = 2; result.success && i < max_; ++i) {
      result = result >> parser_;
      success |= result.success;
    }

    return {result.value, success};
  }

 private:
  const size_t max_;
  T parser_;
};

/** @relates LessThan @brief Syntactic sugar for creating a LessThan parser. */
template <class T>
constexpr LessThan<T> operator<(const T& parser, size_t maximum) noexcept {
  return LessThan<T>{maximum, parser};
}

/** @relates LessThan @brief Syntactic sugar for creating a LessThan parser. */
template <class T>
constexpr LessThan<T> operator>(size_t maximum, const T& parser) noexcept {
  return LessThan<T>{maximum, parser};
}

}  // namespace tiny_parse
