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
#include <iomanip>
#include <string>
#include <string_view>

namespace tiny_parse {

/**
 * @brief A consumer is a function that will be called on the result of a
 * successful parse.
 */
using TINY_PARSE_PUBLIC Consumer = std::function<void(const std::string_view&)>;

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
class TINY_PARSE_PUBLIC Parser {
 public:
  virtual ~Parser() = default;

  virtual Result parse(const std::string_view& sv) const = 0;
};

/**
 * @brief The base parser class.
 */
template <class Derived>
class TINY_PARSE_PUBLIC BaseParser : public Parser {
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
  Derived copy() const noexcept {
    return Derived{*static_cast<const Derived*>(this)};
  }

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
  inline Result parse(const std::string_view& sv) const final {
    const auto result = parse_it(sv);

    if (consumer_ && result.success)
      consumer_(sv.substr(0, sv.size() - result.value.size()));

    return result;
  }

  /**
   * @brief The minimum number of parsed characters that constitute a full
   * parse
   */
  virtual size_t min_length() const noexcept = 0;

 protected:
  virtual Result parse_it(const std::string_view& sv) const noexcept = 0;

 private:
  Consumer consumer_;
};

/** @brief Syntactic sugar for calling the parse function. */
template <class Derived>
inline TINY_PARSE_PUBLIC Result operator>>(const std::string_view& sv,
                                           const BaseParser<Derived>& parser) {
  return parser.parse(sv);
}

/** @brief Syntactic sugar for calling the parse function. */
template <class Derived>
inline TINY_PARSE_PUBLIC Result operator>>(const Result& result,
                                           const BaseParser<Derived>& parser) {
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
class TINY_PARSE_PUBLIC Or : public BaseParser<Or<T, S>> {
 public:
  Or(const T& p1, const S& p2) noexcept : parser1_{p1}, parser2_{p2} {}

  constexpr size_t min_length() const noexcept override {
    return std::min(parser1_.min_length(), parser2_.min_length());
  }

 protected:
  constexpr Result parse_it(
      const std::string_view& sv) const noexcept override {
    if (const auto result = sv >> parser1_; result.success) return result;
    return sv >> parser2_;
  }

 private:
  T parser1_;
  S parser2_;
};

/** @brief Syntactic sugar for creating an Or parser. */
template <class T, class S>
constexpr TINY_PARSE_PUBLIC Or<T, S> operator|(const T& p1,
                                               const S& p2) noexcept {
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
class TINY_PARSE_PUBLIC Then : public BaseParser<Then<T, S>> {
 public:
  Then(const T& p1, const S& p2) noexcept : parser1_{p1}, parser2_{p2} {}

  constexpr size_t min_length() const noexcept override {
    return parser1_.min_length() + parser2_.min_length();
  }

 protected:
  constexpr Result parse_it(
      const std::string_view& sv) const noexcept override {
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

/** @brief Syntactic sugar for creating a Then parser. */
template <class T, class S>
constexpr TINY_PARSE_PUBLIC Then<T, S> operator&(const T& p1,
                                                 const S& p2) noexcept {
  return Then<T, S>{p1, p2};
}

/**
 * @brief A parser that optionally matches the given parser.
 *
 * @tparam T The parser to match.
 */
template <class T>
class TINY_PARSE_PUBLIC Optional : public BaseParser<Optional<T>> {
 public:
  explicit Optional(const T& parser) noexcept : parser_{parser} {}

  constexpr size_t min_length() const noexcept override { return 0; }

 protected:
  constexpr Result parse_it(
      const std::string_view& sv) const noexcept override {
    return {parser_.parse(sv).value, true};
  }

 private:
  T parser_;
};

template <class T>
constexpr TINY_PARSE_PUBLIC Optional<T> operator~(const T& parser) noexcept {
  return Optional<T>{parser};
}

/**
 * @brief A parser that matches the given parser zero or more times.
 *
 * @tparam T The parser to match.
 */
template <class T>
class TINY_PARSE_PUBLIC Many : public BaseParser<Many<T>> {
 public:
  explicit Many(const T& parser) noexcept : parser_{parser} {}

  constexpr size_t min_length() const noexcept override { return 0; }

 protected:
  constexpr Result parse_it(
      const std::string_view& sv) const noexcept override {
    auto result = sv >> parser_;
    while (result.success) {
      result = result >> parser_;
    }
    return {result.value, true};
  }

 private:
  T parser_;
};

/** @brief Syntactic sugar for creating parser that matches zero or more
 * characters */
template <class T>
constexpr TINY_PARSE_PUBLIC Many<T> operator*(const T& parser) noexcept {
  return Many<T>{parser};
}

/**
 * @brief A parser that matches the given parser an exact number of times.
 *
 * @tparam T The parser to match.
 */
template <class T>
class TINY_PARSE_PUBLIC Times : public BaseParser<Times<T>> {
 public:
  Times(size_t times, const T& parser) noexcept
      : times_{times}, parser_{parser} {}

  constexpr size_t min_length() const noexcept override {
    return parser_.min_length() * times_;
  }

 protected:
  constexpr Result parse_it(
      const std::string_view& sv) const noexcept override {
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

/** @brief Syntactic sugar for creating parser that matches an exact number of
 * times */
template <class T>
constexpr TINY_PARSE_PUBLIC Times<T> operator*(size_t times,
                                               const T& parser) noexcept {
  return Times<T>{times, parser};
}

/** @brief Syntactic sugar for creating a parser that matches an exact number of
 * times */
template <class T>
constexpr TINY_PARSE_PUBLIC Times<T> operator*(const T& parser,
                                               size_t times) noexcept {
  return Times<T>{times, parser};
}

/**
 * @brief A parser that matches the given parser more than a given number of
 * times.
 *
 * @tparam T The parser to match.
 */
template <class T>
class TINY_PARSE_PUBLIC GreaterThan : public BaseParser<GreaterThan<T>> {
 public:
  GreaterThan(size_t min, const T& parser) noexcept
      : min_{min}, parser_{parser} {}

  constexpr size_t min_length() const noexcept override {
    return (min_ + 1) * parser_.min_length();
  }

 protected:
  constexpr Result parse_it(
      const std::string_view& sv) const noexcept override {
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

/** @brief Syntactic sugar for creating a GreaterThan parser. */
template <class T>
constexpr TINY_PARSE_PUBLIC GreaterThan<T> operator<(size_t minimum,
                                                     const T& parser) noexcept {
  return GreaterThan<T>{minimum, parser};
}

/** @brief Syntactic sugar for creating a GreaterThan parser. */
template <class T>
constexpr TINY_PARSE_PUBLIC GreaterThan<T> operator>(const T& parser,
                                                     size_t minimum) noexcept {
  return GreaterThan<T>{minimum, parser};
}

/** @brief Syntactic sugar for creating parser that matches one or more
 * characters */
template <class T>
constexpr TINY_PARSE_PUBLIC GreaterThan<T> operator+(const T& parser) noexcept {
  return GreaterThan<T>{0, parser};
}

/**
 * @brief A parser that matches the given parser less than a given number of
 * times.
 *
 * @tparam T The parser to match.
 */
template <class T>
class TINY_PARSE_PUBLIC LessThan : public BaseParser<LessThan<T>> {
 public:
  LessThan(size_t max, const T& parser) noexcept : max_{max}, parser_{parser} {}
  constexpr size_t min_length() const noexcept override { return 0; }

 protected:
  constexpr Result parse_it(
      const std::string_view& sv) const noexcept override {
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

/** @brief Syntactic sugar for creating a LessThan parser. */
template <class T>
constexpr TINY_PARSE_PUBLIC LessThan<T> operator<(const T& parser,
                                                  size_t maximum) noexcept {
  return LessThan<T>{maximum, parser};
}

/** @brief Syntactic sugar for creating a LessThan parser. */
template <class T>
constexpr TINY_PARSE_PUBLIC LessThan<T> operator>(size_t maximum,
                                                  const T& parser) noexcept {
  return LessThan<T>{maximum, parser};
}

}  // namespace tiny_parse
