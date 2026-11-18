// =============================================================================
// ParseUtil.hpp -- tiny text helpers shared by Parser and SimParser
// (C character handling only: no STL, no std::string)
// =============================================================================
#ifndef PARSEUTIL_HPP
#define PARSEUTIL_HPP

#include <cstdio>
#include <cstdlib>
#include <cstdarg>

const int MAX_LINE = 1024;   // longest accepted input line
const int MAX_TOK  = 128;    // most words accepted on one line

inline bool isBlank(char c) { return c == ' ' || c == '\t' || c == '\r' || c == '\n'; }

// Cuts the line at '#' (comment), then splits it IN PLACE into words:
// tok[i] points into 'line'. Returns the word count, or -1 if there are more than maxTok.
inline int tokenize(char* line, char* tok[], int maxTok) {
    for (char* p = line; *p; p++)
        if (*p == '#') { *p = '\0'; break; }
    int n = 0;
    char* p = line;
    while (*p) {
        while (*p && isBlank(*p)) p++;
        if (*p == '\0') break;
        if (n == maxTok) return -1;
        tok[n++] = p;
        while (*p && !isBlank(*p)) p++;
        if (*p) *p++ = '\0';
    }
    return n;
}

// Case-insensitive equality of two C strings (ASCII letters only)
inline bool eqNoCase(const char* a, const char* b) {
    while (*a && *b) {
        char x = *a, y = *b;
        if (x >= 'a' && x <= 'z') x = x - 'a' + 'A';
        if (y >= 'a' && y <= 'z') y = y - 'a' + 'A';
        if (x != y) return false;
        a++; b++;
    }
    return *a == *b;
}

// Parses a whole string as a non-negative decimal integer (digits only, no sign, no junk).
inline bool parseNonNegInt(const char* s, int& out) {
    if (*s == '\0') return false;
    long v = 0;
    for (const char* p = s; *p; p++) {
        if (*p < '0' || *p > '9') return false;
        v = v * 10 + (*p - '0');
        if (v > 1000000000L) return false;   // keeps it inside int
    }
    out = (int)v;
    return true;
}

// Error message in the form  file:line: error: text   (printed to stderr)
inline void parseError(const char* file, int line, const char* fmt, ...) {
    fprintf(stderr, "%s:%d: error: ", file, line);
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
    fprintf(stderr, "\n");
}

#endif