export module ClaFi.StdLib;

export import std;

// `std` exports namespace std and nothing else, and **nothing is re-exported into the global
// namespace here.** Every std name is written std:: at the site that uses it, types included, so
// a global spelling that still compiles came from a C header some translation unit pulled in
// rather than from this module. That is what closes the abs hole: a unit that pulls windows.h
// has ::abs(int) in scope from <stdlib.h>, and abs(aFloat) would bind to the integer overload
// and return zero **without failing to compile.** std::abs cannot be reached by accident.

