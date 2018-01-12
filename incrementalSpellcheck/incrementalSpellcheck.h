#pragma once
#include <vector>
#include <string>
#include <algorithm>
#include <iterator>
#include <unordered_set>

class IncrementalSearch
{
public:
    typedef std::vector<std::string> Strings;

    template <typename StringsArray>
    explicit IncrementalSearch(const StringsArray& text)
    {
        m_text.reserve(std::size(text));
        std::copy(std::begin(text), std::end(text), std::back_inserter(m_text));
    }

    Strings search(const std::string& substring, size_t maxCount = 10)
    {
        Strings results;
        std::unordered_set<Strings::const_pointer> alreadyAdded;

        results.reserve(maxCount);

        // exact match
        for (const std::string& text : m_text)
        {
            if (isStartsWith(text, substring))
            {
                results.push_back(text);
                alreadyAdded.insert(&text);
            }
        }

        // starts with
        for (const std::string& text : m_text)
        {
            if (alreadyAdded.find(&text) == alreadyAdded.end() && isContains(text, substring))
            {
                results.push_back(text);
                alreadyAdded.insert(&text);
            }
        }

        if (results.size() > maxCount)
            results.resize(maxCount);

        return results;
    }

private:
    Strings m_text;

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

class SpellCheck
{
public:
    template <typename StringsArray>
    explicit SpellCheck(const StringsArray& text)
    {
        Strings textVector;
        textVector.reserve(std::size(text));

        for (const std::string& sentence : textVector)
        {
            tokenize(sentence, std::back_inserter(m_text));
        }
    }

private:
    typedef std::vector<std::string> Strings;

    Strings m_text;

    void tokenize(const std::string& string, Strings::iterator inserterIt);
};
