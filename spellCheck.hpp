#pragma once
#include <algorithm>
#include <iterator>
#include <numeric>
#include <string>
#include <list>
#include <array>
#include <cstring>

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

    Corrections getCorrections(const std::string& word, unsigned maxCount, bool isIncremental = false) const
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

                distance = damerauLevenshteinDistance(incrementalWord, word);
            }
            else
            {
                distance = damerauLevenshteinDistance(correctWord, word);
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
    enum COST
    {
        DELETION = 1,
        INSERTION = 1,
        SUBSTITUTION = 1,
        TRANSPOSITION = 1,
    };

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

    class Buffer
    {
        static const size_t STATIC_SIZE = 16 * 16;

        std::array<unsigned, STATIC_SIZE> m_static;
        std::vector<unsigned>             m_dynamic;
        unsigned*                         m_actual;

        Buffer(const Buffer&)            = delete;
        Buffer(const Buffer&&)           = delete;
        Buffer& operator=(const Buffer&) = delete;

    public:
        explicit Buffer(size_t size)
            : m_static()
            , m_dynamic()
            , m_actual(m_static.data())
        {
            if (size > STATIC_SIZE)
            {
                m_dynamic.resize(size);
                m_actual = m_dynamic.data();
            }
        }

        unsigned&       operator[](size_t index)       { return m_actual[index]; }
        const unsigned& operator[](size_t index) const { return m_actual[index]; }
    };

    unsigned damerauLevenshteinDistance(const std::string& source, const std::string& target) const
    {
        size_t width  = source.length() + 1;
        size_t height = target.length() + 1;

        Buffer distanceMatrix(width * height);
        std::iota(&distanceMatrix[0], &distanceMatrix[width], 0);  // 0,1,2,...,width

        for (size_t i = 1, im = 0; i < height; ++i, ++im)
        {
            distanceMatrix[i * width] = (unsigned)i;
            for (size_t j = 1, jn = 0; j < width; ++j, ++jn)
            {
                size_t thisRow = i * width;
                size_t prevRow = (i - 1) * width;

                if (source[jn] == target[im])
                {
                    distanceMatrix[thisRow + j] = distanceMatrix[prevRow + (j - 1)];
                }
                else
                {
                    unsigned nextCost = std::min(
                    {
                        distanceMatrix[prevRow + (j - 1)] + SUBSTITUTION, // a substitution
                        distanceMatrix[prevRow + j]       + DELETION,     // a deletion
                        distanceMatrix[thisRow + (j - 1)] + INSERTION     // an insertion
                    });

                    // a transposition
                    if (i > 1 && j > 1 && source[jn] == target[im - 1] && source[jn - 1] == target[im])
                    {
                        nextCost = std::min(nextCost, distanceMatrix[((i - 2) * width) + (j - 2)] + TRANSPOSITION);
                    }

                    distanceMatrix[thisRow + j] = nextCost;
                }
            }
        }

        int distance = distanceMatrix[width * height - 1];
        return distance;
    }

};


