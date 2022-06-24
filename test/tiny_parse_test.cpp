#include <tiny_parse/tiny_parse.hpp>

#include <iostream>

int main() {
  using namespace tiny_parse;

  std::string_view sv{"ttest str"};

  auto parser = (CharP<'c'>{} | ++CharP<'t'>{}) & CharP<'e'>{};
  parser.consumer([](const auto& sv) { std::cout << sv << std::endl; });
  const auto result = parser.parse(sv);

  if (result.size() < sv.size())
    std::cout << "Parsed " << (sv.size() - result.size()) << " characters."
              << std::endl;

  if (!result.empty()) std::cout << "Not a full parse!" << std::endl;
  return 0;
}