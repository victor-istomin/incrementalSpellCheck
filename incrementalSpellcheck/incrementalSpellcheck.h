#pragma once
#include <vector>
#include <string>
#include <algorithm>
#include <iterator>
#include <unordered_set>
#include <cctype>
#include "SpellCheck.h"


class IncrementalSearch
{
public:
    typedef std::vector<std::string> Strings;

    template <typename StringsArray>
    explicit IncrementalSearch(const StringsArray& text)
        : m_spellCheck(text)
    {
        m_text.reserve(std::size(text));
        std::copy(std::begin(text), std::end(text), std::back_inserter(m_text));
    }

    Strings search(const std::string& substring, size_t maxCount = 10)
    {
        Strings results;
        std::unordered_set<std::string> alreadyAdded;

        results.reserve(maxCount);

        // starts with
        for (const std::string& text : m_text)
        {
            if (isStartsWith(text, substring))
            {
                results.push_back(text);
                alreadyAdded.insert(text);
            }
        }

        // contains
        for (const std::string& text : m_text)
        {
            if (isContains(text, substring) && alreadyAdded.find(text) == alreadyAdded.end())
            {
                results.push_back(text);
                alreadyAdded.insert(text);
            }
        }

        // with corrections
        auto corrections = getCorrections(substring);
        unsigned minMisprints = corrections.empty() ? 0 : corrections.front().m_distance;

        for (const SpellCheck::Correction& correction : corrections)
        {
            if (correction.m_distance != minMisprints)
                break;   // use most attractive corrections only

            const std::string& corrected = *correction.m_word;

            for (const std::string& text : m_text)
            {
                if (isContains(text, corrected) && alreadyAdded.find(text) == alreadyAdded.end())
                {
                    results.push_back(text);
                    alreadyAdded.insert(text);
                }
            }
        }

        if (results.size() > maxCount)
            results.resize(maxCount);

        return results;
    }

    SpellCheck::Corrections getCorrections(const std::string& word)
    {
        // TODO 
        return m_spellCheck.getCorrections(word, 5, true);
    }

private:
    Strings    m_text;
    SpellCheck m_spellCheck;

    bool isStartsWith(const std::string& string, const std::string& substr)
    {
        return string.length() >= substr.length() 
            && 0 == strncmp(string.data(), substr.data(), substr.length());
    }

    bool isContains(const std::string& string, const std::string& substr)
    {
        return std::string::npos != string.find(substr);
    }
};


