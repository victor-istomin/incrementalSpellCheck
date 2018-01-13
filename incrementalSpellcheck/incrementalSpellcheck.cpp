// incrementalSpellcheck.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

#include "incrementalSpellcheck.h"

#include <conio.h>
#include <iostream>
#include <fstream>

int main()
{
    std::ifstream articles = std::ifstream("wikipedia.txt");
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

    IncrementalSearch search { wikipedia };

    std::cout << "Loaded" << std::endl;

    std::string substring;

    int key = 0;
    while ((key = _getch()) != ESC && key != CTRL_C)
    {
        switch (key)
        {
        case BACKSPACE:
            substring.resize(std::max(0, static_cast<int>(substring.length()) - 1));
            break;

        default:
            substring += (char)key;
            break;
        }

        std::cout << std::endl
                  << " ============== " << std::endl 
                  << "Search: '" << substring << "'..." << std::endl;

        for (const std::string& result : search.search(substring))
            std::cout << " > " << result << std::endl;

		std::cout << "Corrections: { ";
        for (const SpellCheck::Correction& correction : search.getCorrections(substring))
            std::cout << correction.m_distance << ": " << *correction.m_word << "; ";
        std::cout << " }" << std::endl;
    }

    return 0;
}

