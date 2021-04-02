#pragma once

// https://stackoverflow.com/a/217605
// https://stackoverflow.com/a/3418285

#include <string>
#include <algorithm> 
#include <cctype>
#include <locale>

/// trim from start (in place)
static inline void ltrim(std::string &s) {
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch) {
        return !std::isspace(ch);
    }));
}

/// trim from end (in place)
static inline void rtrim(std::string &s) {
    s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch) {
        return !std::isspace(ch);
    }).base(), s.end());
}

/// trim from both ends (in place)
static inline void trim(std::string &s) {
    ltrim(s);
    rtrim(s);
}

/// checks if a string begins with a prefix
static inline bool beginsWith(const std::string& s, const std::string& pref){ return s.rfind(pref, 0) == 0; }

/// replaces all occurences of `from` with `to` in `str`
static inline void replaceAll(std::string& str, const std::string& from, const std::string& to){
    if(from.empty()) return;
    size_t start_pos = 0;
    while((start_pos = str.find(from, start_pos)) != std::string::npos){
        str.replace(start_pos, from.length(), to);
        start_pos += to.length();
    }
}
