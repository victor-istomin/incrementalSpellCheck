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
        
        std::string item;
        for (const auto& textItem : text)
        {
            item = textItem;
            m_text.push_back(item);

            std::transform(item.begin(), item.end(), item.begin(), [](char c) { return tolower(c); });
            m_textLowercase.push_back(item);
        }
    }

    Strings search(std::string substring, size_t maxCount = 10)
    {
        Strings results;
        std::unordered_set<std::string> alreadyAdded;

        std::transform(substring.begin(), substring.end(), substring.begin(), [](char c) { return tolower(c); });

        results.reserve(maxCount);

        // starts with
        for (size_t i = 0; i < m_textLowercase.size(); ++i)
        {
            const std::string& lowercaseItem = m_textLowercase[i];
            if (isStartsWith(lowercaseItem, substring))
            {
                results.push_back(m_text[i]);
                alreadyAdded.insert(lowercaseItem);
            }
        }

        // contains
        for (size_t i = 0; i < m_textLowercase.size(); ++i)
        {
            const std::string& lowercaseItem = m_textLowercase[i];
            if (isContains(lowercaseItem, substring) && alreadyAdded.find(lowercaseItem) == alreadyAdded.end())
            {
                results.push_back(m_text[i]);
                alreadyAdded.insert(lowercaseItem);
            }
        }

        // with corrections
        auto     corrections  = getCorrections(substring);
        unsigned minMisprints = corrections.empty() ? 0 : corrections.front().m_distance;

        for (const SpellCheck::Correction& correction : corrections)
        {
            if (correction.m_distance != minMisprints)
                break;   // use most attractive corrections only

            const std::string& corrected = *correction.m_word;

            for (size_t i = 0; i < m_textLowercase.size(); ++i)
            {
                const std::string& lowercaseItem = m_textLowercase[i];
                if (isContains(lowercaseItem, corrected) && alreadyAdded.find(lowercaseItem) == alreadyAdded.end())
                {
                    results.push_back(m_text[i]);
                    alreadyAdded.insert(lowercaseItem);
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
    Strings    m_textLowercase;
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


