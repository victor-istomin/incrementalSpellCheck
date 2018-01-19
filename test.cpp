#include "incrementalSearch.hpp"
#include "getch.h"

#include <iostream>
#include <fstream>
#include <ctime>
#include <cassert>
#include <map>

void unitTests();
void interactive();
void help();

static const std::string s_interactive = "-i";
static const std::string s_unitTests   = "-u";
static const std::string s_help        = "-h";

typedef void (*Callback)();
typedef std::pair<Callback, const char*> CallbackInfo;
typedef std::map<std::string, CallbackInfo> Handlers;

static Handlers s_handlers =
{
    {s_unitTests,   {unitTests,   "unit test"}},
    {s_interactive, {interactive, "interactive"}},
    {s_help,        {help,        "help"}},
};

void help()
{
    std::cout << "Available options: " << std::endl;
    for(const auto& optionHandlerPair : s_handlers)
        std::cout << "  " << optionHandlerPair.first << "\t  : " << optionHandlerPair.second.second << std::endl;
}

int main(int argc, char *argv[])
{
    std::vector<std::string> params;
    for(int i = 1; i < argc; ++i)
        params.push_back(argv[i]);

    if (params.empty())
        params.push_back(s_help);

    for(const std::string& mode : params)
    {
        Callback handler = s_handlers[mode].first;
        if (handler == nullptr)
        {
            help();
            return 1;
        }

        handler();
    }

    return 0;
}

IncrementalSearch load()
{
    std::ifstream articles = std::ifstream("../wikipedia.txt");
    std::vector<std::string> wikipedia;
    wikipedia.reserve(10000);
    while (articles)
    {
        std::string line;
        std::getline(articles, line);

        if (!line.empty())
            wikipedia.emplace_back(std::move(line));
    }

    return IncrementalSearch { wikipedia };
}

void interactive()
{
    static const char ESC       = 0x1b;
    static const char CTRL_C    = 0x03;
    static const char BACKSPACE = 0x08;
    static const char LINUX_DEL = 0x7F;

    IncrementalSearch search = load();

    std::cout << "Loaded" << std::endl;

    std::string substring;

    int key = 0;
    while ((key = getch()) != ESC && key != CTRL_C)
    {
        switch (key)
        {
        case LINUX_DEL:
        case BACKSPACE:
            substring.resize(std::max(0, static_cast<int>(substring.length()) - 1));
            break;

        default:
            if(key >= ' ')  // omit control characters
                substring += (char)key;
            break;
        }

        std::cout << std::endl
                  << " ============== " << std::endl
                  << "Search: '" << substring << "'..." << std::endl;

        clock_t start = clock();
        auto results = search.search(substring);

#ifdef WANT_PROFILING
        for (int i = 0; i < 300; ++i)
        {
            auto results = search.search(substring);
        }
#endif

        clock_t elapsedTime = clock() - start;

        for (const std::string& result : results)
            std::cout << " > " << result << std::endl;

        std::cout << "Corrections: { ";

        for (const SpellCheck::Correction& correction : search.getCorrections(substring))
            std::cout << correction.m_distance << ": " << *correction.m_word << "; ";

        std::cout << "} (" << elapsedTime << " clocks) " << std::endl;
    }
}

void unitTests()
{
    IncrementalSearch search = load();
    const SpellCheck& spellChecker = search.getSpellCheck();

    // insertion
    assert(spellChecker.getSmartDistance("abc", "abc") == 0);
    assert(spellChecker.getSmartDistance("abc", "ab") == 1);
    assert(spellChecker.getSmartDistance("abc", "ac") == 1);
    assert(spellChecker.getSmartDistance("abc", "b") == 2);
    assert(spellChecker.getSmartDistance("abc", "") == 3);

    // deletion
    assert(spellChecker.getSmartDistance("bc", "abc") == 1);
    assert(spellChecker.getSmartDistance("ac", "abc") == 1);
    assert(spellChecker.getSmartDistance("b", "abc") == 2);
    assert(spellChecker.getSmartDistance("", "abc") == 3);
    assert(spellChecker.getSmartDistance("", "") == 0);

    // substitution
    assert(spellChecker.getSmartDistance("abc", "abx") == 1);
    assert(spellChecker.getSmartDistance("abc", "axx") == 2);
    assert(spellChecker.getSmartDistance("abc", "xxx") == 3);

    // transposition
    assert(spellChecker.getSmartDistance("abc", "acb") == 1);
    assert(spellChecker.getSmartDistance("abc", "bac") == 1);
    assert(spellChecker.getSmartDistance("abc", "cba") == 2);

    // special case: Optimal string alignment distance
    assert(spellChecker.getSmartDistance("ca", "abc") == 3);
    assert(spellChecker.getSmartDistance("abc", "ca") == 3);

    // incremental spell check
    assert(spellChecker.getSmartDistance("abcd",  "ab",  true) == 2);      // too short for meaningful incremental search
    assert(spellChecker.getSmartDistance("abcde", "xbc", true) == 3);      // too short common ending for meaningful incremental search

    assert(spellChecker.getSmartDistance("abcd",  "abc", true) == 0);      // 'abc.*' matches 'abcd'
    assert(spellChecker.getSmartDistance("abcde", "abc", true) == 0);      // 'abc.*' matches 'abcde'
    assert(spellChecker.getSmartDistance("abcde", "xbcd", true)  == 1);    // 1 correction xbcd -> abcd, then 'abc.*' matches 'abcde'
    assert(spellChecker.getSmartDistance("abcdefg", "axcde", true) == 1);  // axcde -> abcde, then incremental
    assert(spellChecker.getSmartDistance("abcdefg", "bcd", true) == 1);
    assert(spellChecker.getSmartDistance("abcdefg", "acde", true) == 1);   // acde -> abcde, then incremental

    assert(spellChecker.getSmartDistance("abcdefg", "cde", true) == 4);    // common substring 'cde' located at too different word positions

    assert(spellChecker.getSmartDistance("abcde", "xbcd", true) == 1);     // 1 correction xbcd -> abcd, then incremental match

    assert(spellChecker.getSmartDistance("abcde", "xabc", true) == 1);    // xabc -> abc, then incremental

    SpellCheck other;
    assert(other.getSmartDistance("abcde", "xbcd", true) == 1);     // 1 correction xbcd -> abcd, then incremental match

    const char* rawArray[] = { "one two", "Three" };
    SpellCheck fromRawArray { rawArray };

    std::cout << "unit tests: OK" << std::endl;
}


