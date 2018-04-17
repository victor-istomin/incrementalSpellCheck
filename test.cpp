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
void profileOsa();
void profileOsaIncremental();

static const std::string s_help = "-h";

typedef void (*Callback)();
typedef std::pair<Callback, const char*> CallbackInfo;
typedef std::map<std::string, CallbackInfo> Handlers;

static Handlers s_handlers =
{
    { "-u",       { &unitTests,             "unit test" } },
    { "-i",       { &interactive,           "interactive" } },
    { s_help,     { &help,                  "help" } },
    { "-p_osa",   { &profileOsa,            "profile Optimal String Alignment code" } },
    { "-p_osa_i", { &profileOsaIncremental, "profile Optimal String Alignment incremental code" } },
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

    return std::move(IncrementalSearch { wikipedia });
}

#undef max

void interactive()
{
    static const char ESC       = 0x1b;
    static const char CTRL_C    = 0x03;
    static const char BACKSPACE = 0x08;
    static const char LINUX_DEL = 0x7F;

    unitTests();

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

        std::cout << "} (" << static_cast<int>((double)elapsedTime/CLOCKS_PER_SEC * 1000) << "ms)" << std::endl;
    }
}

void unitTests()
{
    IncrementalSearch search = load();

    // insertion
    assert(SpellCheck::getSmartDistance("abc", "abc") == 0);
    assert(SpellCheck::getSmartDistance("abc", "ab") == 1);
    assert(SpellCheck::getSmartDistance("abc", "ac") == 1);
    assert(SpellCheck::getSmartDistance("abc", "b") == 2);
    assert(SpellCheck::getSmartDistance("abc", "") == 3);

    // deletion
    assert(SpellCheck::getSmartDistance("bc", "abc") == 1);
    assert(SpellCheck::getSmartDistance("ac", "abc") == 1);
    assert(SpellCheck::getSmartDistance("b", "abc") == 2);
    assert(SpellCheck::getSmartDistance("", "abc") == 3);
    assert(SpellCheck::getSmartDistance("", "") == 0);

    // substitution
    assert(SpellCheck::getSmartDistance("abc", "abx") == 1);
    assert(SpellCheck::getSmartDistance("abc", "axx") == 2);
    assert(SpellCheck::getSmartDistance("abc", "xxx") == 3);

    // transposition
    assert(SpellCheck::getSmartDistance("abc", "acb") == 1);
    assert(SpellCheck::getSmartDistance("abc", "bac") == 1);
    assert(SpellCheck::getSmartDistance("abc", "cba") == 2);

    // special case: Optimal string alignment distance
    assert(SpellCheck::getSmartDistance("ca", "abc") == 3);
    assert(SpellCheck::getSmartDistance("abc", "ca") == 3);

    // incremental spell check
    assert(SpellCheck::getSmartDistance("abcd",  "ab",  true) == 2);      // too short for meaningful incremental search
    assert(SpellCheck::getSmartDistance("abcde", "xbc", true) == 1);      // xbc -> abc -> abc.* (incremental match)

    assert(SpellCheck::getSmartDistance("abcd",  "abc", true) == 0);      // 'abc.*' matches 'abcd'
    assert(SpellCheck::getSmartDistance("abcde", "abc", true) == 0);      // 'abc.*' matches 'abcde'
    assert(SpellCheck::getSmartDistance("abcde", "xbcd", true)  == 1);    // 1 correction xbcd -> abcd, then 'abc.*' matches 'abcde'
    assert(SpellCheck::getSmartDistance("abcdefg", "axcde", true) == 1);  // axcde -> abcde, then incremental
    assert(SpellCheck::getSmartDistance("abcdefg", "bcd", true) == 1);
    assert(SpellCheck::getSmartDistance("abcdefg", "acde", true) == 1);   // acde -> abcde, then incremental

    assert(SpellCheck::getSmartDistance("abcdefg", "cde", true) == 2);    // 'cde' -> 'acbcd' (2) -> 'abcde.*'

    assert(SpellCheck::getSmartDistance("abcde", std::string("xbcd"), true) == 1);     // 1 correction xbcd -> abcd, then incremental match

    assert(SpellCheck::getSmartDistance("abcde", "xabc", true) == 1);    // xabc -> abc, then incremental
    assert(SpellCheck::getSmartDistance("abcde", "bac", true) == 1);     // bac -> abc, then incremental

    assert(SpellCheck::getSmartDistance("abcdefgh", "abdc", true) == 1); // transposition at end of string, then incremental match

    assert(SpellCheck::getSmartDistance("abcde", "xbcd", true) == 1);     // 1 correction xbcd -> abcd, then incremental match

    assert(SpellCheck::getSmartDistance("1234567890qwertyuiopasdfghjklzxcvbnm", "____1234567890zxcvbnm____", true) == 27);  // long strings

    const char* rawArray[] = { "one two", "Three" };
    SpellCheck fromRawArray { rawArray };

    std::cout << "unit tests: OK" << std::endl;
}


void profileSpellCheck(bool isIncremental)
{
    int doNotOptimize = 0;

    static const std::string words[] =
    {
        "abcdefghig",         "1_abcdefghig",         "2_abcdefghig",         "3_abcdefghig",         "4_defghig",
        "bace",               "1_bace",               "2_bace",               "3_bace",               "4_e",
        "abace",              "1_abace",              "2_abace",              "3_abace",              "4_ce",
        "qwertyui",           "1_qwertyui",           "2_qwertyui",           "3_qwertyui",           "4_rtyui",
        "zxcvbnm",            "1_zxcvbnm",            "2_zxcvbnm",            "3_zxcvbnm",            "4_vbnm",
        "poiuytrewq",         "1_poiuytrewq",         "2_poiuytrewq",         "3_poiuytrewq",         "4_uytrewq",
        "qazwsx",             "1_qazwsx",             "2_qazwsx",             "3_qazwsx",             "4_wsx",
        "qazwsxedc",          "1_qazwsxedc",          "2_qazwsxedc",          "3_qazwsxedc",          "4_wsxedc",
        "",                   "1_",                   "2_",                   "3_",                   "4_",
        "abcdefghiabcdefghi", "1_abcdefghiabcdefghi", "2_abcdefghiabcdefghi", "3_abcdefghiabcdefghi", "4_defghiabcdefghi",
        "defgh",              "1_defgh",              "2_defgh",              "3_defgh",              "4_gh",
        "___defghi",          "1____defghi",          "2____defghi",          "3____defghi",          "4_defghi",
        "_b_d_f_h_g",         "1__b_d_f_h_g",         "2__b_d_f_h_g",         "3__b_d_f_h_g",         "4_d_f_h_g",
        "a_c_e_g_i_",         "1_a_c_e_g_i_",         "2_a_c_e_g_i_",         "3_a_c_e_g_i_",         "4__e_g_i_",
        "acegi",              "1_acegi",              "2_acegi",              "3_acegi",              "4_gi",
        "bacdfeghgi",         "1_bacdfeghgi",         "2_bacdfeghgi",         "3_bacdfeghgi",         "4_dfeghgi",
        "gihgfedcba",         "1_gihgfedcba",         "2_gihgfedcba",         "3_gihgfedcba",         "4_gfedcba",
    };

    clock_t start = clock();

    for (int i = 0; i < 10000; ++i)
    {
        for (const std::string& s1 : words)
            for (const std::string& s2 : words)
                doNotOptimize += SpellCheck::getSmartDistance(s1, s2, isIncremental);
    }

    clock_t end = clock() - start;

    std::cout << doNotOptimize << ": " << end;
}

void profileOsa()
{
    profileSpellCheck(false);
}

void profileOsaIncremental()
{
    profileSpellCheck(true);
}

