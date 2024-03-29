#include <algorithm>
#include <charconv>
#include <cstdint>
#include <functional>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <system_error>
#include <tiny_parse/built_in.hpp>
#include <tiny_parse/tiny_parse.hpp>
#include <vector>

class Validator {
 public:
  Validator() = default;
  ~Validator() = default;

  void validate_byte(std::string_view sv) {
    uint8_t byte{};
    auto [ptr, ec]{std::from_chars(sv.data(), sv.data() + sv.size(), byte)};
    if (ec != std::errc())
      throw std::invalid_argument{"Value \"" + std::string(sv) + "\" not in range [0, 255]"};

    ip_.push_back(byte);
  }

 private:
  std::vector<uint8_t> ip_;
  std::optional<uint8_t> ip_prefix_;
  std::optional<uint16_t> port;
};

int main() {
  using namespace tiny_parse;

  Validator validator;

  // Define what constitutes a digit
  auto byte = built_in::whole_number;
  byte.consumer(std::bind(&Validator::validate_byte, &validator, std::placeholders::_1));

  auto dot = built_in::CharP<'.'>{};
  auto ip_parser = byte & dot & byte & dot & byte & dot & byte;

  std::string_view ip{"192.168.1.1"};
  const auto result = ip >> ip_parser;

  if (!result)
    std::cout << "Invalid IP address!!" << std::endl;
  else
    std::cout << "Valid IP address!" << std::endl;

  std::cout << "Result is: " << result.value << std::endl;

  return 0;
}
