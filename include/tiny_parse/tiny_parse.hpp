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
#include <string_view>

namespace tiny_parse {

using TINY_PARSE_PUBLIC Consumer = std::function<void(const std::string_view&)>;

class TINY_PARSE_PUBLIC Parser {
 public:
  Parser() : consumer_{} {};
  explicit Parser(const Consumer& consumer) : consumer_{consumer} {}
  virtual ~Parser() = default;

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
   * @return std::string_view The unparsed rest of the input
   */
  inline std::string_view parse(const std::string_view& sv) const {
    const auto result = parse_it(sv);
    const auto nr_parsed = sv.size() - result.size();

    if (consumer_ && min_length() <= nr_parsed && nr_parsed != 0)
      consumer_(sv.substr(0, nr_parsed));

    return result;
  }

  /**
   * @brief The minimum number of parsed characters that constitute a full
   * parse
   */
  virtual size_t min_length() const = 0;

 protected:
  virtual std::string_view parse_it(const std::string_view& sv) const = 0;

 private:
  Consumer consumer_;
};

inline TINY_PARSE_PUBLIC std::string_view operator>>(const std::string_view& sv,
                                                     const Parser& parser) {
  return parser.parse(sv);
}

template <class T, class S>
class TINY_PARSE_PUBLIC Or : public Parser {
 public:
  Or(const T& p1, const S& p2) : parser1_{p1}, parser2_{p2} {}

  constexpr size_t min_length() const override {
    return std::min(parser1_.min_length(), parser2_.min_length());
  }

 protected:
  constexpr std::string_view parse_it(
      const std::string_view& sv) const override {
    const auto len = sv.size();
    auto tmp = parser1_.parse(sv);
    if (tmp.size() == len) tmp = parser2_.parse(sv);

    return tmp;
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
  constexpr std::string_view parse_it(
      const std::string_view& sv) const override {
    auto tmp = parser1_.parse(sv);
    const auto len = tmp.size();

    if (len == sv.size()) return tmp;
    tmp = parser2_.parse(tmp);

    if (tmp.size() == len) return sv;
    return tmp;
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
  constexpr std::string_view parse_it(
      const std::string_view& sv) const override {
    auto old_len = sv.size();
    auto result = parser_.parse(sv);
    while (!result.empty() && result.size() < old_len) {
      old_len = result.size();
      result = parser_.parse(result);
    }

    return result;
  }

 private:
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
  constexpr std::string_view parse_it(
      const std::string_view& sv) const override {
    return parser_.parse(sv);
  }

 private:
  T parser_;
};

template <class T>
constexpr TINY_PARSE_PUBLIC Optional<T> operator~(const T& parser) {
  return Optional<T>{parser};
}

template <size_t min, size_t max, typename T>
class TINY_PARSE_PUBLIC Repeat : public Parser {
 public:
  explicit Repeat(const T& parser) : parser_{parser} {}

  constexpr size_t min_length() const override {
    return min * parser_.min_length();
  }

 protected:
  constexpr std::string_view parse_it(
      const std::string_view& sv) const override {
    auto result = sv;

    for (size_t i = 0; i < min; ++i) {
      result = result >> parser_;
      if (result.empty()) return result;
    }

    for (size_t i = min; i < max; ++i) {
      const auto tmp = result >> parser_;
      if (tmp.empty()) return tmp;
      result = tmp;
    }

    return result;
  }

 private:
  T parser_;
};

template <size_t min, size_t max, typename T>
constexpr TINY_PARSE_PUBLIC Repeat<min, max, T> repeat(const T& parser) {
  return Repeat<min, max, T>{parser};
}

template <size_t min, size_t max, class T>
constexpr TINY_PARSE_PUBLIC Repeat<min, max, T> operator*(const T& parser) {
  return Repeat<min, max, T>{parser};
}

}  // namespace tiny_parse
