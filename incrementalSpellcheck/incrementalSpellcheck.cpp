// incrementalSpellcheck.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

#include "incrementalSpellcheck.h"

#include <conio.h>
#include <iostream>

const char* textCorpus[] = 
{
	"list test",
    "first, item",
    "second item!",
    "third &string",
    "fourth...element",
    "...fifth",
    "sixth,item,in,list",
    "seventh string",
	"one more string",
	"and one more item"
};

int main()
{
	static const char ESC       = 0x1b;
	static const char CTRL_C    = 0x03;
    static const char BACKSPACE = 0x08;

    IncrementalSearch search { textCorpus };

    std::string substring;

	for(const auto& next : textCorpus)
		std::cout << "   " << next << std::endl;

    int key = 0;
    while ((key = _getch()) != ESC && key != CTRL_C)
    {
        switch (key)
        {
        case BACKSPACE:
            substring.resize(std::max((size_t)0, substring.length() - 1));
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

