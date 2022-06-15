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

#include <sstream>

namespace tiny_parse {

class TINY_PARSE_PUBLIC Parser {
 public:
  virtual ~Parser() = default;
  virtual std::stringstream& parse(std::stringstream& ss) = 0;
  operator bool() { return matched_; }

 protected:
  bool& matched() { return matched_; }

 private:
  bool matched_ = false;
};

template <char C>
class TINY_PARSE_PUBLIC CharParser : public Parser {
 public:
  std::stringstream& parse(std::stringstream& ss) override {
    const auto c = ss.peek();
    matched() = c && (c == C);
    if (matched()) ss.get();
    return ss;
  }
};

template <class T, class S>
class TINY_PARSE_PUBLIC Or : public Parser {
 public:
  Or(const T& p1, const S& p2) : parser1_{p1}, parser2_{p2} {}
  std::stringstream& parse(std::stringstream& ss) override {
    ss >> parser1_;
    if (!parser1_) ss >> parser2_;

    matched() = parser1_ || parser2_;
    return ss;
  }

 private:
  T parser1_;
  S parser2_;
};

template <class T, class S>
class TINY_PARSE_PUBLIC Then : public Parser {
 public:
  Then(const T& p1, const S& p2) : parser1_{p1}, parser2_{p2} {}
  std::stringstream& parse(std::stringstream& ss) override {
    ss >> parser1_ >> parser2_;
    matched() = parser1_ && parser2_;
    return ss;
  }

 private:
  T parser1_;
  S parser2_;
};

template <class T>
class TINY_PARSE_PUBLIC More : public Parser {
 public:
  More(const T& parser) : parser_{parser} {}
  std::stringstream& parse(std::stringstream& ss) override {
    ss >> parser_;
    matched() = parser_;
    while (parser_) {
      ss >> parser_;
      matched() |= parser_;
    }

    return ss;
  }

 private:
  T parser_;
};

std::stringstream& operator>>(std::stringstream& ss, Parser& parser) {
  return parser.parse(ss);
}

template <class T, class S>
Or<T, S> operator|(const T& p1, const S& p2) {
  return Or<T, S>{p1, p2};
}

template <class T, class S>
Then<T, S> operator&(const T& p1, const S& p2) {
  return Then<T, S>{p1, p2};
}

template <class T>
More<T> operator++(const T& parser) {
  return More<T>{parser};
}

}  // namespace tiny_parse
