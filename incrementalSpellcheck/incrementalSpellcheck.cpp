// incrementalSpellcheck.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

#include "incrementalSpellcheck.h"

#include <conio.h>
#include <iostream>

const char* textCorpus[] = 
{
    "first item",
    "second item",
    "third string",
    "fourth element",
    "fifth",
};

int main()
{
    static const char ESC       = 0x1b;
    static const char BACKSPACE = 0x08;

    IncrementalSearch search { textCorpus };

    std::string substring;

    int key = 0;
    while ((key = _getch()) != ESC)
    {
        switch (key)
        {
        case BACKSPACE:
            substring.resize(std::max(0u, substring.length() - 1));
            break;

        default:
            substring += (char)key;
            break;
        }

        std::cout << std::endl
                  << " ============== " << std::endl 
                  << substring << "..." << std::endl;

        for (const std::string& result : search.search(substring))
            std::cout << " > " << result << std::endl;

    }

    return 0;
}

