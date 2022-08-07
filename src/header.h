
#include <stddef.h>

// String slice type (will be useful because cstring have memory allocation
// overhead for substring)
struct StrSlice typedef StrSlice;
struct StrSlice {
    char const *data;
    int size;
};

// Parser+Iterator over format specifiers
struct FmtIter typedef FmtIter;
struct FmtIter {
    va_list args;
    StrSlice fmt_str;
};

// Data for format specifier
struct FmtSpecifier typedef FmtSpecifier;
struct FmtSpecifier {
    StrSlice name;
    int      is_vararg;
};

// Utility for checking the name of a specifier
//   may add more functions, like checking for name and whether its vararg
//   simulteneusly (todo: fix grammar error there)
int fmt_spec_name(FmtSpecifier *spec, char const *name);

// Iterator function
// Usage:
//    FmtSpecifier spec;
//    while(fmt_iter_next(iter, &spec)) {
//        if(fmt_spec_name(&spec, "width")) {
              // parse width
//        }
//    }
int fmt_iter_next(FmtIter *iter, FmtSpecifier *result);

// Prototype for user callback
typedef int (FmtCallback)(StrSlice fmt_name, FmtIter *iter);

// Means of registering a callback for formatting
int fmt_add_format(char const *fmt_name, FmtCallback *callback_function);

// The actual based printf
int fmt(char const *msg, ...);

