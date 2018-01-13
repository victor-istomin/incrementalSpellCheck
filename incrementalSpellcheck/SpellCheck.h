#pragma once
#include <algorithm>
#include <string>
#include <list>

class SpellCheck
{
public:
    template <typename StringsArray>
    explicit SpellCheck(const StringsArray& text)
    {
        Strings textVector;
        textVector.reserve(std::size(text));
        std::copy(std::begin(text), std::end(text), std::back_inserter(textVector));

        for (const std::string& sentence : textVector)
            tokenize(sentence, m_tokens);

        std::sort(m_tokens.begin(), m_tokens.end());
        m_tokens.erase(std::unique(m_tokens.begin(), m_tokens.end()), m_tokens.end());
    }

    struct Correction
    {
        unsigned           m_distance;
        const std::string* m_word;
    };

    typedef std::list<Correction> Corrections;

    Corrections getCorrections(const std::string& word, unsigned maxCount, bool isIncremental = false)
    {
        Corrections corrections;

        std::string incrementalWord;
        
        for (const auto& correctWord : m_tokens)
        {
            unsigned distance = 0;
            if (isIncremental)
            {
                // ignore insertions past the end of word, assume user will type them later
                incrementalWord = correctWord;
                if(incrementalWord.size() > word.size())
                    incrementalWord.resize(word.size());

                distance = levenshteinDistance(incrementalWord, word);
            }
            else
            {
                distance = levenshteinDistance(correctWord, word);
            }

            if (corrections.size() < maxCount || corrections.back().m_distance > distance)
            {
                // insert item, keep correction sorted
                auto position = std::find_if(corrections.begin(), corrections.end(), [distance](const Correction& c) { return c.m_distance > distance; });
                corrections.insert(position, Correction { distance, &correctWord });

                if(corrections.size() > maxCount)
                    corrections.resize(maxCount);
            }
        }

        return corrections;
    }

private:
    typedef std::vector<std::string> Strings;

    Strings m_tokens;

    void tokenize(const std::string& input, Strings& insertInto)
    {
        std::string nextToken;
        nextToken.reserve(input.size());

        for (size_t i = 0; i < input.size(); ++i)
        {
            int nextChar = input[i];
            if (isalnum(nextChar))
            {
                nextToken += tolower(nextChar);
            }
            else if (!nextToken.empty())
            {
                insertInto.push_back(nextToken);
                nextToken.clear();
            }
        }

        if (!nextToken.empty())
        {
            insertInto.push_back(nextToken);
            nextToken.clear();
        }
    }

    unsigned levenshteinDistance(const std::string& source, const std::string& target)
    {
        // actually, Damerau–Levenshtein distance
        // TODO: faster algorithm https://en.wikibooks.org/wiki/Algorithm_Implementation/Strings/Levenshtein_distance#C++

        size_t width  = source.length() + 1;
        size_t height = target.length() + 1;

        enum COST
        {
            DELETION = 1,
            INSERTION = 1,
            SUBSTITUTION = 1,
            TRANSPOSITION = 1,
        };

        std::vector<unsigned> distanceMatrix(width * height, 0);
        for (unsigned int i = 1; i < width; ++i)
            distanceMatrix[i] = i;

        for (unsigned int j = 1; j < height; ++j)
            distanceMatrix[width * j] = j;

        for (size_t i = 1, im = 0; i < height; ++i, ++im)
        {
            for (size_t j = 1, jn = 0; j < width; ++j, ++jn)
            {
                if (source[jn] == target[im])
                {
                    distanceMatrix[(i * width) + j] = distanceMatrix[((i - 1) * width) + (j - 1)];
                }
                else
                {
                    unsigned nextCost = std::min(
                    {
                        distanceMatrix[(i - 1) * width + j]       + DELETION,    // a deletion
                        distanceMatrix[i * width + (j - 1)]       + INSERTION,   // an insertion
                        distanceMatrix[(i - 1) * width + (j - 1)] + SUBSTITUTION // a substitution
                    });

                    // a transposition
                    if (i > 1 && j > 1 && source[jn] == target[im - 1] && source[jn - 1] == target[im])
                    {
                        nextCost = std::min(nextCost, distanceMatrix[((i - 2) * width) + (j - 2)] + TRANSPOSITION);
                    }

                    distanceMatrix[(i * width) + j] = nextCost;
                }
            }
        }

        int distance = distanceMatrix[width * height - 1];
        return distance;
    }

};


