## Incremental search and spellchecker in C++

This is C++ header-only implementation of incremental search within a list of captions (menu, articles, etc) with misprint corrections support. 

## Contents

 * [incrementalSpellcheck.h](https://github.com/victor-istomin/incrementalSpellcheck/blob/master/incrementalSpellcheck/incrementalSpellcheck.h) - search implementation
 * [spellCheck.h](https://github.com/victor-istomin/incrementalSpellcheck/blob/master/incrementalSpellcheck/SpellCheck.h) - spelling checker using [Damerau–Levenshtein distance](https://en.wikipedia.org/wiki/Damerau%E2%80%93Levenshtein_distance)
 * [incrementalSpellcheck.cpp](https://github.com/victor-istomin/incrementalSpellcheck/blob/master/incrementalSpellcheck/incrementalSpellcheck.cpp)  -a kind of tests and usage example.
 * [list of Wikipedia core articles](https://github.com/victor-istomin/incrementalSpellcheck/blob/master/incrementalSpellcheck/wikipedia.txt) is used as text to perform search in
