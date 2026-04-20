#ifndef FLASKCPP_STR_COMMON_H
#define FLASKCPP_STR_COMMON_H

#include <string>
#include <vector>

static inline std::string stringLower(const std::string& src)
{
    std::string ret = "";
    for (int i=0;i<src.length();i++) {
        int assic = src[i];
        if (assic < 91 && assic > 64) {
            ret += (char)(assic + 32);
        }
        else ret += src[i];
    }
    return ret;
}

static inline bool stringEndsWith(const std::string& src, const std::string& suffix) {
    size_t str_len = src.length();
    size_t suffix_len = suffix.length();
    if (suffix_len > str_len) return false;
    return (src.find(suffix, str_len - suffix_len) == (str_len - suffix_len));
}

static inline bool stringStartsWith(const std::string& str_, const std::string& prefix)
{
    size_t str_len = str_.length();
    size_t prefix_len = prefix.length();
    if (prefix_len > str_len) return false;
    return str_.find(prefix) == 0;
}

static inline std::vector<std::string> stringSplit(const std::string& str_, const std::string& delimiter) {
    if (str_.empty()) return {""};
    std::vector<std::string> tokens;
    size_t pos = 0;
    std::string::size_type prev_pos = 0;
    while ((pos = str_.find(delimiter, prev_pos)) != std::string::npos) {
        tokens.push_back(str_.substr(prev_pos, pos - prev_pos));
        prev_pos = pos + delimiter.length();
    }
    if (prev_pos < str_.length()) tokens.push_back(str_.substr(prev_pos));
    if (stringEndsWith(str_, delimiter)) tokens.push_back("");
    return tokens;
}

#endif