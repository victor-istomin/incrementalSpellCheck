## Incremental search and spellchecker in C++

This is C++ header-only implementation of incremental search within a list of captions (menu, articles, etc) with misprint corrections support. 

## Contents

 * [incrementalSearch.h](incrementalSearch.h) - search implementation
 * [spellCheck.h](spellCheck.h) - spelling checker using [Damerau–Levenshtein distance](https://en.wikipedia.org/wiki/Damerau%E2%80%93Levenshtein_distance)
 * [test.cpp](test.cpp) - a kind of tests and usage example.
 * [msvc-test](msvc-test) - test solution for MSVC 2015 and higher
 * [linux-test](linux-test) - Linux Makefile
 * getch.cpp, getch.h - a stub for similar getch() on windows and Linux
 * [list of Wikipedia core articles](https://github.com/victor-istomin/incrementalSpellcheck/blob/master/incrementalSpellcheck/wikipedia.txt) is used as text to perform search in
