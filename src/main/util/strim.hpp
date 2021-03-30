#pragma once

// https://stackoverflow.com/a/217605

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
