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
    // with no vocabulary, for distance computation only
    SpellCheck() {}

    // with vocabulary
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

    // Get a list if correction suggestions. In case of 'isIncremental', don't count insertions past the end if 'initialWord', 
    // assume that user will type insufficient chars later
    Corrections getCorrections(const std::string& initialWord, unsigned maxCorrections, bool isIncremental = false) const
    {
        Corrections corrections;

        for (const auto& correctWord : m_tokens)
        {
            unsigned distance = getSmartDistance(correctWord, initialWord, isIncremental);

            if (corrections.size() < maxCorrections || corrections.back().m_distance > distance)
            {
                // insert item, keep correction sorted
                auto position = std::find_if(corrections.begin(), corrections.end(), [distance](const Correction& c) { return c.m_distance > distance; });
                corrections.insert(position, Correction { distance, &correctWord });

                if(corrections.size() > maxCorrections)
                    corrections.resize(maxCorrections);
            }
        }

        return corrections;
    }

    // Get either Optimal String Alignment distance or its 'incremental' version. 
    // Note that parameters order is important in incremental version, 
    // because it's asymmetric ('abc.*' matches 'abcd', but 'abcd.*' does not match 'abc')
    //
    // Bug: deletions will lead to extra cost in incremental distance: 
    //      getSmartDistance("abcdefg", "xabc",  true) == 3 instead of 1
    //
    // See getCorrections() for incremental explanations or optimalStringAlignementDistance() for OSA distance details
    unsigned getSmartDistance(const std::string& correctWord, const std::string& initialWord, bool isIncremental = false) const
    {
        unsigned distance = optimalStringAlignementDistance(correctWord, initialWord);

        static const size_t k_minIncrSearchLen = 3;
        if (isIncremental && initialWord.length() >= k_minIncrSearchLen && correctWord.length() > initialWord.length())
        {
            // ignore insertions past the end of word, assume user will type them later. To achieve this, 
            // decrease distance if vocabulary word contains ending of input word and that ending is located in the similar place

            char wordEnding[k_minIncrSearchLen + 1];
            int wordEndingPos = initialWord.length() - k_minIncrSearchLen;
            std::copy_n(initialWord.c_str() + wordEndingPos, k_minIncrSearchLen + 1, wordEnding);

            static const int k_endingDiffThreshold = 1;
            size_t offset = (size_t) std::max(0, wordEndingPos - k_endingDiffThreshold);
            auto posInCorrectWord = correctWord.find(wordEnding, offset);
            int endingPosDifference = std::abs(wordEndingPos - (int)posInCorrectWord);

            if (posInCorrectWord != std::string::npos && endingPosDifference <= k_endingDiffThreshold)
            {
                int insufficientChars = correctWord.length() - posInCorrectWord - std::size(wordEnding) + 1;
                distance -= insufficientChars;
            }
        }

        return distance;
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

    unsigned optimalStringAlignementDistance(const std::string& source, const std::string& target) const
    {
        // it's a variation of Damerau-Levenshtein distance with small improvement:
        // https://en.wikipedia.org/wiki/Damerau%E2%80%93Levenshtein_distance#Algorithm
        // LD_distance("CA", "ABC") == 2, while OSA_distance("CA", "ABC") == 3;

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


