#include "tiny_parse.hpp"

#include <iostream>

int main() {
  using namespace tiny_parse;

  std::stringstream ss{"ttest str"};

  auto parser = (CharParser<'c'>{} | ++CharParser<'t'>{}) & CharParser<'e'>{};
  ss >> parser;

  std::cout << (parser ? "Successful parse!" : "Parse unsuccessful!") << std::endl;

  if (!ss.eof())
    std::cout << std::endl << "Didn't reach end of file!" << std::endl;

  return 0;
}