#pragma once
#include <vector>
#include <string>
#include <algorithm>
#include <cctype>

#include "spellCheck.hpp"


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
        Strings resultsStart;
        Strings resultsContain;
        Strings resultsCorrected;

        std::transform(substring.begin(), substring.end(), substring.begin(), [](char c) { return tolower(c); });

        resultsStart.reserve(maxCount);
        resultsContain.reserve(maxCount);
        resultsCorrected.reserve(maxCount);

        auto     corrections  = getCorrections(substring);
        unsigned minMisprints = corrections.empty() ? 0 : corrections.front().m_distance;
        auto worstCorrections = std::find_if(corrections.begin(), corrections.end(), [minMisprints](const SpellCheck::Correction& c) { return c.m_distance > minMisprints; });
        corrections.resize(std::distance(corrections.begin(), worstCorrections));

        for (size_t i = 0; i < m_textLowercase.size() && resultsStart.size() < maxCount; ++i)
        {
            const std::string& lowercaseText = m_textLowercase[i];
            if (isStartsWith(lowercaseText, substring))
            {
                resultsStart.push_back(m_text[i]);
            }
            else if (resultsContain.size() < maxCount && isContains(lowercaseText, substring))
            {
                resultsContain.push_back(m_text[i]);
            }
            else if (resultsContain.size() < maxCount && resultsCorrected.size() < maxCount)
            {
                for (const SpellCheck::Correction& correction : corrections)
                {
                    const std::string& corrected = *correction.m_word;
                    if (isContains(lowercaseText, corrected))
                    {
                        resultsCorrected.push_back(m_text[i]);
                        break;
                    }
                }
            }
        }

        Strings results;
        results.reserve(resultsStart.size() + resultsContain.size() + resultsCorrected.size());
        std::copy(resultsStart.begin(), resultsStart.end(), std::back_inserter(results));
        std::copy(resultsContain.begin(), resultsContain.end(), std::back_inserter(results));
        std::copy(resultsCorrected.begin(), resultsCorrected.end(), std::back_inserter(results));

        if (results.size() > maxCount)
            results.resize(maxCount);

        return results;
    }

    SpellCheck::Corrections getCorrections(const std::string& word)
    {
        static const unsigned MAX_COUNT = 5;
        return m_spellCheck.getCorrections(word, MAX_COUNT, true);
    }

    const SpellCheck& getSpellCheck() const { return m_spellCheck; }

    IncrementalSearch(IncrementalSearch&& right)
        : m_text(std::move(right.m_text))
        , m_textLowercase(std::move(right.m_textLowercase))
        , m_spellCheck(std::move(right.m_spellCheck))
    {}

private:
    Strings    m_text;
    Strings    m_textLowercase;
    SpellCheck m_spellCheck;

    IncrementalSearch(const IncrementalSearch&)            = delete;
    IncrementalSearch& operator=(const IncrementalSearch&) = delete;

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


