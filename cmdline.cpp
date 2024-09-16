#include "cmdline.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <tuple>
#include <vector>

enum Options
{
    WC_BYTES,
    WC_CHARS,
    WC_WORDS,
    WC_LINES,
    WC_HELP,
    WC_VERSION,
    WC_OPTION_COUNT,
};

struct OptionString
{
    std::string shortOption;
    std::string longOption;

    OptionString(const std::string &shortOption, const std::string &longOption)
        : shortOption{shortOption}, longOption{longOption}
    {
    }
};

static const std::array<OptionString, WC_OPTION_COUNT> wcOptions = {
    OptionString("-c", "--bytes"),
    OptionString("-m", "--chars"),
    OptionString("-w", "--words"),
    OptionString("-l", "--lines"),
    OptionString("-h", "--help"),
    OptionString("-v", "--version")
};

static const std::string shortOptions = "cmwlhv";

static constexpr const char *wcVersion            = "0.0.1";
static constexpr const char *wcUsage              = "Usage: wc [OPTION]... [FILE]...";
static constexpr const char *wcBytesOptionHelpMsg = "  -c, --bytes    prints the byte counts";
static constexpr const char *wcCharsOptionHelpMsg = "  -m, --chars    prints the character counts";
static constexpr const char *wcWordsOptionHelpMsg = "  -w, --words    prints the word counts";
static constexpr const char *wcLinesOptionHelpMsg = "  -l, --lines    prints the newline counts";
static constexpr const char *wcVrsnOptionHelpMsg  = "  -v, --version  prints the version information";
static constexpr const char *wcHelpOptionHelpMsg  = "  -h, --help     prints this help and exits";

class CmdLine::Impl
{
private:
    std::vector<std::string> m_inputFiles;
    int m_argumentCount;
    std::array<bool, WC_OPTION_COUNT> m_options;
    struct FileStat
    {
        size_t bytes = 0;
        size_t chars = 0;
        size_t words = 0;
        size_t lines = 0;
    } m_totalStat;
    std::string m_invalidOption;

    void printHelp() const
    {
        std::cout << wcUsage << '\n'
                  << wcBytesOptionHelpMsg << '\n'
                  << wcCharsOptionHelpMsg << '\n'
                  << wcWordsOptionHelpMsg << '\n'
                  << wcLinesOptionHelpMsg << '\n'
                  << wcVrsnOptionHelpMsg  << '\n'
                  << wcHelpOptionHelpMsg  << '\n';
    }

    void printVersion() const
    {
        std::cout << "wc - version " << wcVersion << '\n';
    }

    void printInvalidOption() const
    {
        std::cout << "wc - invalid option provided : " << m_invalidOption << '\n'
                  << "Try 'wc --help' to get usage.\n";
    }

    void extractOptions(char** commandOptions)
    {
        for (int i = 0; i < m_argumentCount; ++i)
        {
            std::string optionStr{commandOptions[i]};
            if (optionStr.starts_with("-"))
            {
                extractSingleOption(optionStr);
            }
            else
            {
                m_inputFiles.push_back(optionStr);
            }
        }
    }

    void extractSingleOption(const std::string& cmdLineArgument)
    {
        bool validOption = false;
        size_t argLen = cmdLineArgument.length();
        if (!cmdLineArgument.starts_with("--"))
        {
            for (size_t i = 1; i < argLen; ++i)
            {
                size_t pos = shortOptions.find(cmdLineArgument[i]);
                if (pos != std::string::npos)
                {
                    m_options[pos] = true;
                    validOption = true;
                }
                else
                {
                    m_invalidOption = cmdLineArgument;
                    break;
                }
            }
        }
        if (!validOption && m_invalidOption.empty())
        {
            for (int i = 0; i < WC_OPTION_COUNT; ++i)
            {
                if (cmdLineArgument == wcOptions[i].longOption)
                {
                    m_options[i] = true;
                    m_invalidOption.clear();
                    break;
                }
                m_invalidOption = cmdLineArgument;
            }
        }
    }

    std::tuple<size_t, size_t> getCharAndWordCounts(const std::string &line)
    {
        size_t charCount = 0, wordCount = 0;
        bool wordFlag = false;
        for (const char &c : line)
        {
            ++charCount;
            if (std::isspace(c))
            {
                wordFlag = false;
            }
            else if (!wordFlag)
            {
                ++wordCount;
                wordFlag = true;
            }
        }
        ++charCount; // for newline
        return std::make_tuple(charCount, wordCount);
    }

    std::string formatOutput(const FileStat &fileStat)
    {
        std::ostringstream oss;
        if (std::all_of(m_options.begin(), m_options.end(), [&](bool b) { return !b; }))
        {
            oss << fileStat.lines << " " << fileStat.words << " " << fileStat.chars;
        }
        if (m_options[WC_LINES])
        {
            oss << fileStat.lines << " ";
        }
        if (m_options[WC_WORDS])
        {
            oss << fileStat.words << " ";
        }
        if (m_options[WC_CHARS])
        {
            oss << fileStat.chars << " ";
        }
        if (m_options[WC_BYTES])
        {
            oss << fileStat.chars << " ";
        }
        return oss.str();
    }

    void process(std::istream &in, const char *filename = nullptr)
    {
        FileStat fileStat;
        std::string line;
        while (std::getline(in, line))
        {
            ++fileStat.lines;
            auto charsAndWords = getCharAndWordCounts(line);
            fileStat.chars += std::get<0>(charsAndWords);
            fileStat.words += std::get<1>(charsAndWords);
        }
        m_totalStat.chars += fileStat.chars;
        m_totalStat.words += fileStat.words;
        m_totalStat.lines += fileStat.lines;
        std::string file = filename ? ": " + std::string(filename) : "";
        std::cout << formatOutput(fileStat) << file << '\n';
    }

    void process(const std::string &file)
    {
        std::ifstream fileStream(file);
        try
        {
            process(fileStream, file.c_str());
        }
        catch (const std::exception &e)
        {
            std::cerr << "wc: " << file << ": " << e.what() << '\n';
        }
    }

    void processInputs()
    {
        size_t fileCount = m_inputFiles.size();
        if (fileCount == 0)
        {
            process(std::cin);
        }
        else
        {
            for (const std::string &file : m_inputFiles)
            {
                process(file);
            }
        }
        if (fileCount > 1)
        {
            std::cout << formatOutput(m_totalStat) << " : total\n";
        }
    }

public:
    Impl(int argumentCount, char **commandOptions)
        : m_argumentCount{argumentCount}
    {
        m_options.fill(false);
        extractOptions(commandOptions);
    }

    void process()
    {
        if (m_options[WC_HELP])
        {
            printHelp();
            return;
        }
        if (m_options[WC_VERSION])
        {
            printVersion();
            return;
        }
        if (!m_invalidOption.empty())
        {
            printInvalidOption();
            std::exit(EXIT_FAILURE);
        }
        processInputs();
    }
};

CmdLine::CmdLine(int argumentCount, char **commandOptions)
{
    m_impl = new Impl(argumentCount, commandOptions);
}

CmdLine::~CmdLine()
{
    delete m_impl;
}

void CmdLine::process()
{
    m_impl->process();
}
