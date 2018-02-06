#pragma once
#include <algorithm>
#include <iterator>
#include <numeric>
#include <string>
#include <list>
#include <array>
#include <cstring>
#include <limits>
#include <cassert>

#if defined DUMP
#include <iostream>
#include <iomanip>

template <typename Item>
void dump(Item* buffer, size_t width, size_t height)
{
	for(size_t row = 0; row < height; ++row)
	{
		for(size_t column = 0; column < width; ++column)
			std::cout << std::setw(4) << buffer[row * width + column];

		std::cout << std::endl;
	}
}

#endif


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
    // See getCorrections() for incremental explanations or optimalStringAlignementDistance() for OSA distance details
    unsigned getSmartDistance(const std::string& correctWord, const std::string& initialWord, bool isIncremental = false) const
    {
		std::list<CorrectionType> traceback;
        unsigned distance = optimalStringAlignementDistance(correctWord, initialWord, traceback);

        static const size_t k_minIncrSearchLen = 3;
        if (isIncremental && initialWord.length() >= k_minIncrSearchLen && correctWord.length() > initialWord.length())
        {
            // ignore insertions past the end of word, assume user will type them later. To achieve this, 
            // decrease distance if vocabulary word contains ending of input word and that ending is located in the similar place

			int insertionsPastEnd = 0;
			for(auto it = traceback.rbegin(); it != traceback.rend() && *it == CorrectionType::eINSERTION; ++it)
				++insertionsPastEnd;

			distance -= insertionsPastEnd;
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

	template <typename T>
    class Buffer
    {
        static const size_t STATIC_SIZE = 16 * 16;

        std::array<T, STATIC_SIZE> m_static;
        std::vector<T>             m_dynamic;
        T*                         m_actual;

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

        T&       operator[](size_t index)       { return m_actual[index]; }
        const T& operator[](size_t index) const { return m_actual[index]; }
    };

	enum class CorrectionType : char
	{
		eNOT_INITIALIZED = 0,
		eNONE,
		eSUBSTITUTION,
		eDELETION,
		eINSERTION,
		eTRANSPOSITION
	};


	struct Alternative
	{
		CorrectionType bestType = CorrectionType::eNOT_INITIALIZED;
		int bestDistance = std::numeric_limits<int>::max();

		Alternative() = default;

		void propose(CorrectionType type, int newCost)
		{
			if(bestDistance > newCost)
			{
				bestDistance = newCost;
				bestType = type;
			}
		}
	};

    unsigned optimalStringAlignementDistance(const std::string& source, const std::string& target, std::list<CorrectionType>& traceback) const
    {
        // it's a variation of Damerau-Levenshtein distance with small improvement:
        // https://en.wikipedia.org/wiki/Damerau%E2%80%93Levenshtein_distance#Algorithm
        // LD_distance("CA", "ABC") == 2, while OSA_distance("CA", "ABC") == 3;

        int width  = source.length() + 1;
        int height = target.length() + 1;

        Buffer<unsigned>       distanceMatrix   (width * height);
		Buffer<CorrectionType> correctionsMatrix(width * height);
		
		std::iota(&distanceMatrix[0], &distanceMatrix[width], 0);  // 0,1,2,...,width
		std::fill(&correctionsMatrix[1], &correctionsMatrix[width], CorrectionType::eINSERTION);

        for (size_t i = 1, im = 0; i < height; ++i, ++im)
        {
            distanceMatrix[i * width] = (unsigned)i;
			correctionsMatrix[i * width] = CorrectionType::eDELETION;

            for (size_t j = 1, jn = 0; j < width; ++j, ++jn)
            {
                size_t thisRow = i * width;
                size_t prevRow = (i - 1) * width;

                if (source[jn] == target[im])
                {
                    distanceMatrix[thisRow + j]    = distanceMatrix[prevRow + (j - 1)];
					correctionsMatrix[thisRow + j] = CorrectionType::eNONE;
                }
                else
                {
					Alternative correction;
					correction.propose(CorrectionType::eSUBSTITUTION, distanceMatrix[prevRow + (j - 1)] + SUBSTITUTION);
					correction.propose(CorrectionType::eDELETION,     distanceMatrix[prevRow + j]       + DELETION);
					correction.propose(CorrectionType::eINSERTION,    distanceMatrix[thisRow + (j - 1)] + INSERTION);

                    // a transposition
                    if (i > 1 && j > 1 && source[jn] == target[im - 1] && source[jn - 1] == target[im])
                    {
						correction.propose(CorrectionType::eTRANSPOSITION, distanceMatrix[((i - 2) * width) + (j - 2)] + TRANSPOSITION);
                    }

                    distanceMatrix[thisRow + j] = correction.bestDistance;
					correctionsMatrix[thisRow + j] = correction.bestType;
                }
            }
        }

        int distance = distanceMatrix[width * height - 1];

#if defined DUMP
		std::cout << "Costs:\n";
		dump(&distanceMatrix[0], width, height);

		std::cout << "Operations:\n";
		dump(&correctionsMatrix[0], width, height);
#endif

		// backtrace. The idea description: https://web.stanford.edu/class/cs124/lec/med.pdf
		int row = height - 1;
		int column = width - 1;

		traceback.clear();

		while(row != 0 || column != 0)
		{
			CorrectionType fixType = correctionsMatrix[column + row * width];
			traceback.push_front(fixType);

			switch(fixType)
			{
			case CorrectionType::eNONE:
			case CorrectionType::eSUBSTITUTION:
				// diagonal by 1 cell
				--column;
				--row;
				break;

				break;
			case CorrectionType::eDELETION:
				// up by 1 cell
				--row;
				break;

			case CorrectionType::eINSERTION:
				// left
				--column;
				break;

			case CorrectionType::eTRANSPOSITION:
				// diagonal by 2 cells
				column -= 2;
				row -= 2;
				break;

			case CorrectionType::eNOT_INITIALIZED:
			default:
				assert(0 && "incorrect fixType");
				break;
			}

		}

        return distance;
    }

};


