#ifndef FNMATCH_H
#define FNMATCH_H

// fnmatch return values (additional values are used on the implementation side).
#undef FNM_NOMATCH
#define	FNM_NOMATCH	1 // Failure

// Barebones implementation of GNU fnmatch. Assumes that 'flags' == 0. Matches 'str' against
// the filename pattern 'pattern', returning 0 if it matches or FNM_NOMATCH if it doesn't.
int fnmatch(const char *pattern, const char *str, int flags);

#endif
