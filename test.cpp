#include "incrementalSearch.hpp"
#include "getch.h"

#include <iostream>
#include <fstream>
#include <ctime>

int main()
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

    static const char ESC       = 0x1b;
    static const char CTRL_C    = 0x03;
    static const char BACKSPACE = 0x08;
    static const char LINUX_DEL = 0x7F;

    IncrementalSearch search { wikipedia };

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

    return 0;
}

