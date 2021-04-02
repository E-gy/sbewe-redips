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

/// https://tools.ietf.org/html/rfc3986#section-5.2.4
static inline void removeDotSegments(std::string& s){
	for(std::size_t p = 0; p < s.length();){
		const auto inpb = s.substr(p);
		//A
		if(beginsWith(inpb, "./")) s.erase(p, 1);
		else if(beginsWith(inpb, "../")) s.erase(p, 2);
		//B
		else if(beginsWith(inpb, "/.") && (p+2 >= s.length() || s[p+2] == '/')) s.erase(p, 2);
		//C
		else if(beginsWith(inpb, "/..") && (p+3 >= s.length() || s[p+3] == '/')){
			s.erase(p, 3);
			if(p > 0){
				auto pseg = s.rfind('/', p-1);
				if(pseg != s.npos){
					s.erase(pseg, p-pseg);
					p = pseg;
				}
			}
		}
		//D
		else if(inpb == ".") s.erase(p, 2);
		else if(inpb == "..") s.erase(p, 3);
		else p = s.find('/', p+1);
	}
	if(s.length() == 0) s = "/"; //always keep a leading slash
}

static inline void decodeUnreservedPercent(std::string& s){
	for(std::size_t p = s.find('%'); p < s.length(); p = s.find('%', p+1)) if(p+2 < s.length()){
		std::size_t nom = 0;
		unsigned char c = std::stoul(s.substr(p+1, 2), &nom, 16);
		if(nom == 2 && (::isalnum(c) || (c == '-' || c == '_' || c == '.' || c == '~') || c > 0x7F)) s.replace(p, 3, {char(c)});
	}
}

static inline void normalizeURIPath(std::string& s){
	replaceAll(s, "//", "/");
	removeDotSegments(s);
	decodeUnreservedPercent(s);
}
