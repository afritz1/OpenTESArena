#include "fnmatch.h"
#include "../debug/Debug.h"

// Only use this on the implementation side because it's not part of the official interface.
#undef FNM_SUCCESS
#define FNM_SUCCESS 0
#undef FNM_FAILURE
#define FNM_FAILURE (-1)

int fnmatch(const char *pattern, const char *str, int flags)
{
	if ((pattern == nullptr) || (str == nullptr))
	{
		DebugLogError("'pattern' or 'str' was null.");
		return FNM_FAILURE;
	}

	if (flags != 0)
	{
		DebugLogError("'flags' not supported in fnmatch() implementation.");
		return FNM_FAILURE;
	}

	static_cast<void>(flags);

	// Iterator through pattern string.
	const char *patIter = pattern;

	// Iterator through match string.
	const char *strIter = str;

	// Value of currently selected pattern string character.
	char curPatChar;

	while ((curPatChar = *patIter++) != '\0')
	{
		switch (curPatChar)
		{
		case '?':
			if (*strIter == '\0')
			{
				return FNM_NOMATCH;
			}

			break;

		case '\\':
			curPatChar = *patIter++;
			if (*strIter != curPatChar)
			{
				return FNM_NOMATCH;
			}

			break;

		case '*':
		{
			for (curPatChar = *patIter++; (curPatChar == '?') || (curPatChar == '*'); curPatChar = *patIter++, strIter++)
			{
				if ((curPatChar == '?') && (*strIter == '\0'))
				{
					return FNM_NOMATCH;
				}
			}

			if (curPatChar == '\0')
			{
				return FNM_SUCCESS;
			}

			const char maybeEscapedChar = (curPatChar == '\\') ? *patIter : curPatChar;

			for (patIter--; *strIter != '\0'; strIter++)
			{
				if ((curPatChar == '[') || (*strIter == maybeEscapedChar))
				{
					return FNM_SUCCESS;
				}
			}

			return FNM_NOMATCH;
		}

		case '[':
		{
			if (*strIter == '\0')
			{
				return FNM_NOMATCH;
			}

			// Nonzero if the sense of the character class is inverted.
			const bool inverted = (*patIter == '!') || (*patIter == '^');
			if (inverted)
			{
				patIter++;
			}

			curPatChar = *patIter++;
			while (true)
			{
				char cStart = curPatChar;
				char cEnd = curPatChar;

				if (curPatChar == '\\')
				{
					cEnd = *patIter++;
					cStart = cEnd;
				}

				if (curPatChar == '\0')
				{
					// [ (unterminated) loses.
					return FNM_NOMATCH;
				}

				curPatChar = *patIter++;

				if ((curPatChar == '-') && (*patIter != ']'))
				{
					cEnd = *patIter++;
					if (cEnd == '\\')
					{
						cEnd = *patIter++;
					}

					if (cEnd == '\0')
					{
						return FNM_NOMATCH;
					}

					curPatChar = *patIter++;
				}

				if ((*strIter >= cStart) && (*strIter <= cEnd))
				{
					goto matchedLeftBracket;
				}

				if (curPatChar == ']')
				{
					break;
				}
			}

			if (!inverted)
			{
				return FNM_NOMATCH;
			}

			break;

		matchedLeftBracket:
			// Skip the rest of the [...] that already matched.
			while (curPatChar != ']')
			{
				if (curPatChar == '\0')
				{
					// [... (unterminated) loses.
					return FNM_NOMATCH;
				}

				curPatChar = *patIter++;
				if (curPatChar == '\\')
				{
					// 1003.2d11 is unclear if this is right.
					patIter++;
				}
			}

			if (inverted)
			{
				return FNM_NOMATCH;
			}

			break;
		}

		default:
			if (curPatChar != *strIter)
			{
				return FNM_NOMATCH;
			}
		}

		strIter++;
	}

	return (*strIter == '\0') ? FNM_SUCCESS : FNM_NOMATCH;
}
