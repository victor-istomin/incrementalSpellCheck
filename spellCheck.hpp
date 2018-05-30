#pragma once
#include <algorithm>
#include <numeric>
#include <string>
#include <cstring>
#include <list>
#include <array>
#include <vector>
#include <cassert>

#ifdef max
#undef max
#define HAD_MAX_DEFINE
#endif

#define DUMP
#if defined DUMP
#include <iostream>
#include <iomanip>

template <typename Int>
Int getItem(Int i)   { return i; }

template <>
char getItem(char c) { return c != '\0' ? c : '-'; }

template <typename Item>
void dump(Item* buffer, size_t width, size_t height, const char* rowLabels = nullptr, const char* columnLabels = nullptr)
{
	if(columnLabels)
	{
		std::cout << "    ";

        for(size_t i = 0; i < width; ++i)
			std::cout << std::setw(4) << (i > 0 ? columnLabels[i - 1] : ' ');

		std::cout << std::endl;
	}

    for(size_t row = 0; row < height; ++row)
    {
		if(rowLabels)
			std::cout << std::setw(4) << (row > 0 ? rowLabels[row - 1] : ' ');
		else
			std::cout << "    ";

		for(size_t column = 0; column < width; ++column)
        {
            std::cout << std::setw(4) << getItem(buffer[row * width + column]);
        }

        std::cout << std::endl;
    }
}


#endif


class SpellCheck
{
public:
    struct NoCaseConversion
    {
        char operator()(char c) const { return c; }
    };

    // with vocabulary
    template <typename StringsArray, typename CaseConvertor = NoCaseConversion>
    explicit SpellCheck(const StringsArray& text, CaseConvertor changeCase = NoCaseConversion())
    {
        for (const auto& sentence : text)
            tokenize(sentence, m_tokens, changeCase);

        // remove duplicates
        std::sort(m_tokens.begin(), m_tokens.end());
        m_tokens.erase(std::unique(m_tokens.begin(), m_tokens.end()), m_tokens.end());
    }

    struct Correction
    {
        unsigned           m_distance;
        const std::string* m_word;
    };

    typedef std::list<Correction> Corrections;

    // Get a list of correction suggestions. In case of 'isIncremental', don't count insertions past the end if 'initialWord', 
    // assume that user will type insufficient chars later
    template <typename String>
    Corrections getCorrections(const String& initialWord, unsigned maxCorrections, bool isIncremental = false) const
    {
        Corrections corrections;

        for (const auto& correctWord : m_tokens)
        {
            unsigned distance = getSmartDistance(correctWord, initialWord, isIncremental);

            if (corrections.empty() || corrections.back().m_distance > distance)
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
    template <typename String, typename OtherString>
    static unsigned getSmartDistance(const String& correctWord, const OtherString& initialWord, bool isIncremental = false)
    {
        Buffer<CorrectionType> traceback;
        unsigned distance = optimalStringAlignementDistance(correctWord, initialWord, &traceback);

        static const size_t k_minIncrSearchLen = 3;
        if (isIncremental && getSize(initialWord) >= k_minIncrSearchLen && getSize(correctWord) > getSize(initialWord))
        {
            // ignore insertions past the end of word, assume user will type them later.
            int insertionsPastEnd = 0;
            for(auto it = traceback.begin(); it != traceback.end() && *it == CorrectionType::eINSERTION; ++it)
                ++insertionsPastEnd;

            distance -= insertionsPastEnd;
        }

        return distance;
    }

    template <typename String, typename StringList, typename CaseConvertor = NoCaseConversion>
    static void tokenize(const String& input, StringList& insertInto,  CaseConvertor changeCase = NoCaseConversion())
    {
        using Token = typename StringList::value_type;
        std::string nextToken;
        nextToken.reserve(getSize(input));

        for(size_t i = 0; i < getSize(input); ++i)
        {
            int nextChar = input[i];
            if(isalnum(nextChar))
            {
                nextToken += changeCase(nextChar);
            }
            else if(!nextToken.empty())
            {
                insertInto.push_back(Token(nextToken.c_str()));
                nextToken.clear();
            }
        }

        if(!nextToken.empty())
        {
            insertInto.push_back(Token(nextToken.c_str()));
            nextToken.clear();
        }
    }

private:
    enum COST
    {
        DELETION = 1,
        INSERTION = 1,
        SUBSTITUTION = 1,
        TRANSPOSITION = 1,
    };

    std::vector<std::string> m_tokens;

    // this is simple Buffer implementation. Actually, it's either std::array or std::vector
    // std::array is user in order to speed up computation by avoiding extra heap allocations
    // while std::vector is a fallback in case of large buffer needed
    template <typename T>
    class Buffer
    {
		static const size_t STATIC_SIZE = 16 * 16;

        std::array<T, STATIC_SIZE> m_static;
        std::vector<T>             m_dynamic;
        T*                         m_actual;
        size_t                     m_size;

        Buffer(const Buffer&)            = delete;
        Buffer& operator=(const Buffer&) = delete;

        bool isStatic() const { return m_actual == m_static.data(); }

    public:
        explicit Buffer(size_t size = 0)
            : m_static()
            , m_dynamic()
            , m_actual(m_static.data())
            , m_size(size)
        {
            if (size > STATIC_SIZE)
            {
                m_dynamic.resize(size);
                m_actual = m_dynamic.data();
            }
        }

        Buffer(Buffer&& right)
        {
            *this = std::move(right);
        }

        Buffer& operator=(Buffer&& other)
        {
            if(other.isStatic())
            {
                size_t itemsToMove = other.m_size;

                for(size_t i = 0; i < itemsToMove; ++i)
                    std::swap(m_static[i], other.m_static[i]);

                m_actual = m_static.data();
            }
            else
            {
                std::swap(m_dynamic, other.m_dynamic);
                m_actual = m_dynamic.data();
            }

            m_size = other.m_size;

            // don't care about these members in temporary 'other'
            other.m_actual = nullptr;
            other.m_size = 0;

            return *this;
        }

        T&       operator[](size_t index)       { return m_actual[index]; }
        const T& operator[](size_t index) const { return m_actual[index]; }

		T* begin() { return m_actual; }
		T* end()   { return m_actual + m_size;  }

		void shrink(size_t actualSize)
		{
			assert(actualSize <= m_size);
			if(actualSize <= m_size)
			{
				m_size = actualSize;
			}
		}
    };

    enum class CorrectionType : char
    {
        eNOT_INITIALIZED = 0,
        eNONE = 'n',
        eSUBSTITUTION = 's',
        eDELETION = 'd',
        eINSERTION = 'i',
        eTRANSPOSITION = 't'
    };

    struct Alternative
    {
        CorrectionType bestType = CorrectionType::eNOT_INITIALIZED;
        int bestDistance        = std::numeric_limits<int>::max();

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

    // std::string support. Note: 'auto' return type is needed in order to force "expression SFINAE" to work under MSVC 2015
    template <typename T> static auto getSize(const T& string) -> decltype(string.size())
    { 
        return string.size();
    }

    // CString support. Note: 'auto' return type is needed in order to force "expression SFINAE" to work under MSVC 2015
    template <typename T> static auto getSize(const T& string) -> decltype(static_cast<size_t>(string.GetLength()))
    { 
        return static_cast<size_t>(string.GetLength());
    }

    static size_t getSize(const char* string)
    {
        return strlen(string);
    }

    template <typename String, typename OtherString>
    static unsigned optimalStringAlignementDistance(const String& source, const OtherString& target, Buffer<CorrectionType>* backtrace = nullptr)
    {
        // it's a variation of Damerau-Levenshtein distance with small improvement:
        // https://en.wikipedia.org/wiki/Damerau%E2%80%93Levenshtein_distance#Algorithm
        // LD_distance("CA", "ABC") == 2, while OSA_distance("CA", "ABC") == 3;

        size_t width  = getSize(source) + 1;
        size_t height = getSize(target) + 1;

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

                    // a transposition
                    if(i > 1 && j > 1 && source[jn] == target[im - 1] && source[jn - 1] == target[im])
                    {
                        correction.propose(CorrectionType::eTRANSPOSITION, distanceMatrix[((i - 2) * width) + (j - 2)] + TRANSPOSITION);
                    }

                    // insertion should be proposed before substitution, because both will resolve insufficient character after the end of 'source'

                    correction.propose(CorrectionType::eINSERTION,    distanceMatrix[thisRow + (j - 1)] + INSERTION);
                    correction.propose(CorrectionType::eSUBSTITUTION, distanceMatrix[prevRow + (j - 1)] + SUBSTITUTION);
                    correction.propose(CorrectionType::eDELETION,     distanceMatrix[prevRow + j]       + DELETION);

                    distanceMatrix[thisRow + j] = correction.bestDistance;
                    correctionsMatrix[thisRow + j] = correction.bestType;
                }
            }
        }

        int distance = distanceMatrix[width * height - 1];

#if defined DUMP
        std::cout << "Costs:\n";
        dump(&distanceMatrix[0], width, height, &target[0], &source[0]);

        std::cout << "Operations:\n";
        dump((char*) &correctionsMatrix[0], width, height, &target[0], &source[0]);
#endif

        // in case of incremental match, we need both 'distance' and a list of corrections
        // backtrace will construct corrections list from 'distanceMatrix'
        if(backtrace != nullptr)
        {
            *backtrace = optimalStringAlignmentBacktrace(height, width, correctionsMatrix);

#if defined DUMP
            std::string s;
            std::transform(backtrace->begin(), backtrace->end(), std::back_inserter(s), [](CorrectionType c) {return (char)c;});
			std::reverse(s.begin(), s.end());
			std::cout << "Source:              " << source << std::endl
			          << "Target:              " << target << std::endl
			          << "Optimal corrections: " << s      << std::endl;
#endif
        }

        return distance;
    }

    // backtrace. The idea description: https://web.stanford.edu/class/cs124/lec/med.pdf
    static Buffer<CorrectionType> optimalStringAlignmentBacktrace(size_t height, size_t width, const Buffer<CorrectionType>& correctionsMatrix)
    {
        Buffer<CorrectionType> backtrace(height + width);

        int row    = height - 1;
        int column = width - 1;

        auto itActualEnd = backtrace.begin();

        while(row != 0 || column != 0)
        {
            CorrectionType fixType = correctionsMatrix[column + row * width];

            assert(itActualEnd != backtrace.end());

            switch(fixType)
            {
            case CorrectionType::eNONE:
            case CorrectionType::eSUBSTITUTION:     // moved diagonal by 1 cell
                --column;
                --row;
                break;

            case CorrectionType::eDELETION:         // moved up by 1 cell
                --row;
                break;

            case CorrectionType::eINSERTION:        // moved left
                --column;
                break;

            case CorrectionType::eTRANSPOSITION:    // moved diagonal by 2 cells
                column -= 2;
                row    -= 2;
                break;

            case CorrectionType::eNOT_INITIALIZED:
            default:
                assert(0 && "incorrect fixType");
                break;
            }

			*itActualEnd = fixType;
			++itActualEnd;

        }

		backtrace.shrink(itActualEnd - backtrace.begin());
        return backtrace;
    }

};

#ifdef HAD_MAX_DEFINE
// from windows.h
#define max(a,b)            (((a) > (b)) ? (a) : (b))
#endif

