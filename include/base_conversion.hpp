#pragma once

#include <string>
#include <string_view>
#include <stdexcept>
#include <cstdint>
#include <limits>
#include <array>
#include <charconv>
#include <utility>
#include <format>

namespace evqovv {
namespace base_conversion {
namespace details {
enum class base : unsigned char {
    binary,
    octal,
    decimal,
    hexadecimal,
};

inline constexpr auto binary_base = 2;
inline constexpr auto octal_base = 8;
inline constexpr auto decimal_base = 10;
inline constexpr auto hexadecimal_base = 16;

template <bool uppercase = true>
inline constexpr auto decimal_to_hexadecimal_map(int digit) noexcept -> char {
    static constexpr std::array<char, 16> upper_chars{
        '0', '1', '2', '3', '4', '5', '6', '7',
        '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};
    static constexpr std::array<char, 16> lower_chars{
        '0', '1', '2', '3', '4', '5', '6', '7',
        '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'};

    return uppercase ? upper_chars[digit] : lower_chars[digit];
}

inline constexpr auto hexadecimal_to_binary_map(int digit) noexcept
    -> std::string_view {
    static constexpr std::array<std::string_view, 16> map{
        "0000", "0001", "0010", "0011", "0100", "0101", "0110", "0111",
        "1000", "1001", "1010", "1011", "1100", "1101", "1110", "1111",
    };

    if (digit >= '0' && digit <= '9') {
        return map[digit - '0'];
    } else if (digit >= 'A' && digit <= 'F') {
        return map[digit - 'A' + 10];
    } else if (digit >= 'a' && digit <= 'f') {
        return map[digit - 'a' + 10];
    }

    [[unlikely]] std::unreachable();
}

inline constexpr auto hexadecimal_to_decimal_map(int digit) noexcept -> int {
    if (digit <= '9') {
        return digit - '0';
    } else if (digit <= 'F') {
        return digit - 'A' + 10;
    } else {
        return digit - 'a' + 10;
    }
}

inline constexpr auto octal_to_binary_map(int digit) noexcept
    -> std::string_view {
    static constexpr std::array<std::string_view, 8> map{
        "000", "001", "010", "011", "100", "101", "110", "111"};

    if (digit >= '0' && digit <= '7') {
        return map[digit - '0'];
    }

    [[unlikely]] std::unreachable();
}

inline constexpr auto binary_to_hexadecimal_map(std::string_view str) -> char {
    static constexpr std::array<std::pair<std::string_view, char>, 16> map{
        {{"0000", '0'},
         {"0001", '1'},
         {"0010", '2'},
         {"0011", '3'},
         {"0100", '4'},
         {"0101", '5'},
         {"0110", '6'},
         {"0111", '7'},
         {"1000", '8'},
         {"1001", '9'},
         {"1010", 'A'},
         {"1011", 'B'},
         {"1100", 'C'},
         {"1101", 'D'},
         {"1110", 'E'},
         {"1111", 'F'}}};

    for (auto const &[binary, hexadecimal] : map) {
        if (str == binary) {
            return hexadecimal;
        }
    }

    [[unlikely]] std::unreachable();
}

inline constexpr auto binary_to_octal_map(std::string_view str) -> char {
    static constexpr std::array<std::pair<std::string_view, char>, 16> map{
        {{"000", '0'},
         {"001", '1'},
         {"010", '2'},
         {"011", '3'},
         {"100", '4'},
         {"101", '5'},
         {"110", '6'},
         {"111", '7'}}};

    for (auto const &[binary, octal] : map) {
        if (str == binary) {
            return octal;
        }
    }

    [[unlikely]] std::unreachable();
}

inline auto throw_invalid_character_error(char invalid_char) -> void {
    throw std::invalid_argument(
        std::format("base conversion error: invalid character '{}' in string",
                    invalid_char));
}

inline auto throw_overflow_error() -> void {
    throw std::overflow_error("base conversion error: the value represented by "
                              "string exceeds uint64_t limit");
}

inline auto validate_string(std::string_view str) -> void {
    if (str.empty()) [[unlikely]] {
        throw std::invalid_argument("base conversion error: string is empty");
    }
}

inline auto validate_binary_character(char ch) -> void {
    if (ch != '0' && ch != '1') {
        details::throw_invalid_character_error(ch);
    }
}

inline auto validate_octal_character(char ch) -> void {
    if (ch < '0' || ch > '7') {
        details::throw_invalid_character_error(ch);
    }
}

inline auto validate_hexadecimal_character(char ch) -> void {
    if (!((ch >= '0' && ch <= '9') || (ch >= 'A' && ch <= 'F') ||
          (ch >= 'a' && ch <= 'f'))) {
        details::throw_invalid_character_error(ch);
    }
}

inline auto trim_leading_zeros(std::string_view str) noexcept
    -> std::string_view {
    validate_string(str);

    auto const first_non_zero_pos = str.find_first_not_of('0');
    return (first_non_zero_pos == std::string_view::npos)
               ? "0"
               : str.substr(first_non_zero_pos);
}

inline auto to_uint64_t(std::string_view str) -> uint64_t {
    uint64_t result{};

    auto [ptr, ec] =
        std::from_chars(str.data(), str.data() + str.size(), result);
    if (ec == std::errc::invalid_argument) {
        details::throw_invalid_character_error(*ptr);
    }
    if (ec == std::errc::result_out_of_range) {
        details::throw_overflow_error();
    }

    return result;
}

inline auto validate_binary_string(std::string_view str) -> void {
    for (auto &&ch : str) {
        details::validate_binary_character(ch);
    }
}

inline auto validate_multiple(std::size_t multiple) -> void {
    if (multiple == 0) [[unlikely]] {
        throw std::invalid_argument("base conversion error: multiple is zero");
    }
}

inline auto validate_overflow(uint64_t const &value, int digit, int base) {
    if (value > (std::numeric_limits<uint64_t>::max() - digit) / base)
        [[unlikely]] {
        details::throw_overflow_error();
    }
}
} // namespace details

inline auto zero_padding(std::string_view str, std::size_t multiple)
    -> std::string {
    details::validate_string(str);
    details::validate_multiple(multiple);

    std::string result(str);
    auto const padding_num = (multiple - (result.size() % multiple)) % multiple;
    result.insert(0, padding_num, '0');
    return result;
}

inline auto binary_to_octal(std::string_view str) -> std::string {
    details::validate_string(str);
    details::validate_binary_string(str);

    auto padded_str = zero_padding(details::trim_leading_zeros(str), 3);

    std::string result;
    for (decltype(padded_str.size()) i{}; i != padded_str.size(); i += 3) {
        result += details::binary_to_octal_map(
            std::string_view(padded_str).substr(i, 3));
    }

    return std::string(details::trim_leading_zeros(result));
}

inline auto binary_to_decimal(std::string_view str) -> std::string {
    details::validate_string(str);

    uint64_t result{};
    for (auto &&ch : details::trim_leading_zeros(str)) {
        details::validate_binary_character(ch);

        int digit = ch - '0';

        details::validate_overflow(result, digit, details::binary_base);

        result = result * details::binary_base + digit;
    }

    return std::to_string(result);
}

inline auto binary_to_hexadecimal(std::string_view str) -> std::string {
    details::validate_string(str);
    details::validate_binary_string(str);

    auto padded_str = zero_padding(details::trim_leading_zeros(str), 4);

    std::string result;
    for (decltype(padded_str.size()) i{}; i != padded_str.size(); i += 4) {
        result += details::binary_to_hexadecimal_map(
            std::string_view(padded_str).substr(i, 4));
    }

    return std::string(details::trim_leading_zeros(result));
}

inline auto octal_to_binary(std::string_view str) -> std::string {
    details::validate_string(str);

    std::string result;
    for (auto &&ch : details::trim_leading_zeros(str)) {
        details::validate_octal_character(ch);

        result += details::octal_to_binary_map(ch);
    }

    return std::string(details::trim_leading_zeros(result));
}

inline auto octal_to_decimal(std::string_view str) -> std::string {
    details::validate_string(str);

    uint64_t result{};
    for (auto &&ch : details::trim_leading_zeros(str)) {
        details::validate_octal_character(ch);

        int digit = ch - '0';

        details::validate_overflow(result, digit, details::octal_base);

        result = result * details::octal_base + digit;
    }

    return std::to_string(result);
}

inline auto octal_to_hexadecimal(std::string_view str) -> std::string {
    details::validate_string(str);

    return binary_to_hexadecimal(octal_to_binary(str));
}

inline auto decimal_to_binary(std::string_view str) -> std::string {
    details::validate_string(str);

    auto value = details::to_uint64_t(details::trim_leading_zeros(str));

    std::string result;
    do {
        result.insert(result.begin(),
                      (value % details::binary_base) ? '1' : '0');
        value /= details::binary_base;
    } while (value != 0);

    return result;
}

inline auto decimal_to_octal(std::string_view str) -> std::string {
    details::validate_string(str);

    auto value = details::to_uint64_t(details::trim_leading_zeros(str));

    std::string result;
    do {
        result.insert(result.begin(), '0' + (value % details::octal_base));
        value /= details::octal_base;
    } while (value != 0);

    return result;
}

template <bool uppercase = true>
inline auto decimal_to_hexadecimal(std::string_view str) -> std::string {
    details::validate_string(str);

    auto value = details::to_uint64_t(details::trim_leading_zeros(str));

    std::string result;
    do {
        result.insert(result.begin(),
                      details::decimal_to_hexadecimal_map<uppercase>(
                          value % details::hexadecimal_base));
        value /= details::hexadecimal_base;
    } while (value != 0);

    return result;
}

inline auto hexadecimal_to_binary(std::string_view str) -> std::string {
    details::validate_string(str);

    std::string result;
    for (auto &&ch : details::trim_leading_zeros(str)) {
        details::validate_hexadecimal_character(ch);

        result += details::hexadecimal_to_binary_map(ch);
    }

    return std::string(details::trim_leading_zeros(result));
}

inline auto hexadecimal_to_octal(std::string_view str) -> std::string {
    details::validate_string(str);

    return binary_to_octal(hexadecimal_to_binary(str));
}

inline auto hexadecimal_to_decimal(std::string_view str) -> std::string {
    details::validate_string(str);

    uint64_t result{};
    for (auto &&ch : details::trim_leading_zeros(str)) {
        details::validate_hexadecimal_character(ch);

        int digit = details::hexadecimal_to_decimal_map(ch);

        details::validate_overflow(result, digit, details::hexadecimal_base);

        result = result * details::hexadecimal_base + digit;
    }

    return std::to_string(result);
}
} // namespace base_conversion
} // namespace evqovv