// Logger.cpp — Explicit template instantiations
//
// Because EventLogger is a class template defined entirely in Logger.hpp,
// the compiler needs to see the full definition at every point of use.
// However, to reduce compile times and ensure the linker has exactly one
// copy of the most commonly-used instantiation, we provide an explicit
// instantiation here for std::string.
//
// If additional instantiations are needed in the future (e.g.
// EventLogger<int>), add them below.

#include "Logger.hpp"
#include <string>

// Explicit template instantiation for common types
// This ensures the linker can find the template implementations
template class EventLogger<std::string>;
