#ifndef DEBUG_HPP
#define DEBUG_HPP
// This boolean is set to true if the '-d' command line option is specified.
// This should probably not be referenced directly, instead, use the DEBUG macro below.

extern bool debugOn;

// SIMP_DEBUG macro - This macro should be used by code to emit debug information.
// In the '-d' option is specified on the command line, and if this is a
// debug build, then the code specified as the option to the macro will be
// executed.  Otherwise it will not be.
#ifndef DEBUG
#define SIMP_DEBUG(X)
#else // DEBUG
#define SIMP_DEBUG(X) do { if (debugOn) { X; } } while (0)
#endif // DEBUG

#endif // DEBUG_HPP
