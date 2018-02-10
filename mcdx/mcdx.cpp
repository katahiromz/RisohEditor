#include "MProcessMaker.hpp"
#include "MString.hpp"
#include "MacroParser.hpp"

#ifndef _countof
    #define _countof(array)     (sizeof(array) / sizeof(array[0]))
#endif

void show_help(void)
{
    printf("mcdx --- Message Compiler Dirty Extension by katahiromz\n");
    printf("Copyright (C) 2018 Katayama Hirofumi MZ. License: GPLv3.\n");
    printf("\n");
    printf("Usage: mcdx [option(s)] [input-file] [output-file]\n");
    printf("Options:\n");
    printf("  -i --input=<file>             Name input file\n");
    printf("  -o --output=<file>            Name output file\n");
    printf("  -J --input-format=<format>    Specify input format\n");
    printf("  -O --output-format=<format>   Specify output format\n");
    printf("  -I --include-dir=<dir>        Include directory when preprocessing rc file\n");
    printf("  -D --define=<sym>[=<val>]     Define SYM when preprocessing rc file\n");
    printf("  -U --undefine <sym>           Undefine SYM when preprocessing rc file\n");
    printf("FORMAT is one of rc, res or bin, and is deduced from the file name\n");
    printf("No input-file is stdin, default rc.  No output-file is stdout, default res.\n");
    printf("Report bugs to <katayama.hirofumi.mz@gmail.com>\n");
}

void show_version(void)
{
    printf("mcdx ver.0.0\n");
}

WCHAR g_szCppExe[MAX_PATH] = L"";

wchar_t *g_input_file = NULL;
wchar_t *g_output_file = NULL;
const wchar_t *g_inp_format = L"rc";
const wchar_t *g_out_format = L"res";

std::vector<MStringW> g_include_directories;
std::vector<MStringW> g_definitions;
std::vector<MStringW> g_undefinitions;

BOOL check_cpp_exe(VOID);
{
    WCHAR szPath[MAX_PATH], *pch;
    GetModuleFileNameW(NULL, szPath, _countof(szPath));
    pch = wcsrchr(szPath, L'\\');
    lstrcpyW(pch, L"\\cpp.exe");
    if (::GetFileAttributesW(szPath) == INVALID_FILE_ATTRIBUTES)
    {
        lstrcpyW(pch, L"\\data\\bin\\cpp.exe");
        if (::GetFileAttributesW(szPath) == INVALID_FILE_ATTRIBUTES)
        {
            lstrcpyW(pch, L"\\..\\data\\bin\\cpp.exe");
            if (::GetFileAttributesW(szPath) == INVALID_FILE_ATTRIBUTES)
            {
                lstrcpyW(pch, L"\\..\\..\\data\\bin\\cpp.exe");
                if (::GetFileAttributesW(szPath) == INVALID_FILE_ATTRIBUTES)
                {
                    lstrcpyW(pch, L"\\..\\..\\..\\data\\bin\\cpp.exe");
                    if (::GetFileAttributesW(szPath) == INVALID_FILE_ATTRIBUTES)
                    {
                        return FALSE;
                    }
                }
            }
        }
    }
    lstrcpynW(g_szCppExe, szPath, MAX_PATH);
    return TRUE;
}

int eat_output(const std::string& strOutput)
{
    std::vector<std::string> lines;
    mstr_split(lines, strOutput, "\n");

    for (size_t i = 0; i < lines.size(); ++i)
    {
        if (lines[i][0] == '#')
        {
            lines.erase(lines.begin() + i);
            --i;
        }
        else
        {
            mstr_trim(lines[i]);
        }
    }

    BOOL bInBrace = FALSE;
    LANGID langid = 0;
    for (size_t i = 0; i < lines.size(); ++i)
    {
        std::string& line = lines[i];
        if (memcmp("LANGUAGE", &line[0], 8) == 0)
        {
            char *ptr = &line[8];
            while (std::isspace(*ptr))
            {
                ++ptr;
            }
            char *ptr0 = ptr;
            while (std::isalnum(*ptr))
            {
                ++ptr;
            }
            char *ptr1 = ptr;
            while (std::isspace(*ptr))
            {
                ++ptr;
            }
            char *ptr2 = ptr;
            while (std::isalnum(*ptr))
            {
                ++ptr;
            }
            *ptr1 = 0;
            WORD wPrimaryLang = strtoul(ptr0, NULL, 0);
            WORD wSubLang = strtoul(ptr2, NULL, 0);
            langid = MAKELANGID(wPrimaryLang, wSubLang);
            continue;
        }
        else if (memcmp("MESSAGETABLEDX", &line[0], 14) == 0)
        {
            char *ptr = &line[14];
            while (std::isspace(*ptr))
            {
                ++ptr;
            }
            if (*ptr == '{')
            {
                bInBrace = TRUE;
                continue;
            }
            if (memcmp(ptr, "BEGIN", 5) == 0)
            {
                bInBrace = TRUE;
                continue;
            }
        }
        else if (line[0] == '\"')
        {
            if (bInBrace)
            {
                
            }
            else
            {
                fprintf(stderr, "ERROR: Invalid statement\n");
                return 8;
            }
        }
        else if (memcmp("END", &line[0], 3) == 0 || line[0] == '}')
        {
            bInBrace = FALSE;
            continue;
        }
    }
}

int just_do_it(void)
{
    MStringW strCommandLine;
    strCommandLine += L"\"";
    strCommandLine += g_szCppExe;
    strCommandLine += L"\" -P";

    for (size_t i = 0; i < g_definitions.size(); ++i)
    {
        strCommandLine += L' ';
        strCommandLine += g_definitions[i];
    }

    strCommandLine += L' ';
    strCommandLine += g_input_file;

    MProcessMaker maker;
    maker.SetShowWindow(SW_HIDE);
    maker.SetCreationFlags(CREATE_NEW_CONSOLE);

    MFile hInputWrite, hOutputRead;
    if (maker.PrepareForRedirect(&hInputWrite, &hOutputRead) &&
        maker.CreateProcessDx(NULL, g_input_file.c_str()))
    {
        std::string strOutput;
        pmaker.ReadAll(strOutput, hOutputRead);

        if (pmaker.GetExitCode() == 0)
        {
            return eat_output(strOutput);
        }

        fputs(strOutput.c_str(), stdout);
    }
    return -1;
}

extern "C"
int wmain(int argc, wchar_t **wargv)
{
    g_definitions.push_back(L"-DRC_INVOKED");
    g_definitions.push_back(L"-DMC_INVOKED");

    static const wchar_t *include_dir_equal = L"--include-dir=";
    size_t include_dir_equal_len = lstrlenW(include_dir_equal);
    size_t include_dir_equal_size = include_dir_equal_len * sizeof(wchar_t);

    static const wchar_t *define_equal = L"--define=";
    size_t define_equal_len = lstrlenW(define_equal);
    size_t define_equal_size = define_equal_len * sizeof(wchar_t);

    // parse command line
    for (int i = 1; i < argc; ++i)
    {
        // show help?
        if (lstrcmpiW(wargv[i], L"--help") == 0 ||
            lstrcmpiW(wargv[i], L"/?") == 0)
        {
            show_help();
            return 0;
        }
        // show version?
        if (lstrcmpiW(wargv[i], L"--version") == 0)
        {
            show_version();
            return 0;
        }
        // input file?
        if (lstrcmpiW(wargv[i], L"--input_file") == 0 ||
            lstrcmpW(wargv[i], L"-i") == 0)
        {
            ++i;
            if (i < argc)
            {
                if (g_input_file)
                {
                    fprintf(stderr, "ERROR: Too many input files\n");
                    return 1;
                }
                else
                {
                    g_input_file = wargv[i];
                    continue;
                }
            }
            else
            {
                fprintf(stderr, "ERROR: -i or --input_file requires an argument\n");
                return 1;
            }
        }
        // output file?
        if (lstrcmpiW(wargv[i], L"--output") == 0 ||
            lstrcmpW(wargv[i], L"-o") == 0)
        {
            ++i;
            if (i < argc)
            {
                if (g_output_file)
                {
                    fprintf(stderr, "ERROR: Too many output files\n");
                    return 1;
                }
                else
                {
                    g_output_file = wargv[i];
                    continue;
                }
            }
            else
            {
                fprintf(stderr, "ERROR: -o or --output requires an argument\n");
                return 1;
            }
        }
        // input format?
        if (lstrcmpiW(wargv[i], L"--input-format") == 0 ||
            lstrcmpW(wargv[i], L"-J") == 0)
        {
            ++i;
            if (i < argc)
            {
                g_inp_format = wargv[i];
                continue;
            }
            else
            {
                fprintf(stderr, "ERROR: -J or --input-format requires an argument\n");
                return 1;
            }
        }
        // output format?
        if (lstrcmpiW(wargv[i], L"--output-format") == 0 ||
            lstrcmpW(wargv[i], L"-O") == 0)
        {
            ++i;
            if (i < argc)
            {
                g_out_format = wargv[i];
                continue;
            }
            else
            {
                fprintf(stderr, "ERROR: -O or --output-format requires an argument\n");
                return 1;
            }
        }
        // include directory?
        if (lstrcmpW(wargv[i], L"-I") == 0)
        {
            ++i;
            if (i < argc)
            {
                g_include_directories.push_back(wargv[i]);
                continue;
            }
            else
            {
                fprintf(stderr, "ERROR: -I requires an argument\n");
                return 1;
            }
        }
        if (memcmp(wargv[i], include_dir, include_dir_equal_size) == 0)
        {
            g_include_directories.push_back(&wargv[i][include_dir_equal_len]);
            continue;
        }
        // definition?
        if (lstrcmpW(wargv[i], L"-D") == 0)
        {
            ++i;
            if (i < argc)
            {
                g_definitions.push_back(wargv[i]);
                continue;
            }
            else
            {
                fprintf(stderr, "ERROR: -D requires an argument\n");
                return 1;
            }
        }
        if (memcmp(wargv[i], L"-D", 2) == 0)
        {
            g_definitions.push_back(&wargv[i][2]);
            continue;
        }
        // undefine?
        if (lstrcmpW(wargv[i], L"-U") == 0 ||
            lstrcmpiW(wargv[i], L"-undefine") == 0)
        {
            ++i;
            if (i < argc)
            {
                g_undefinitions.push_back(wargv[i]);
                continue;
            }
            else
            {
                fprintf(stderr, "ERROR: -U requires an argument\n");
                return 1;
            }
        }
        if (memcmp(wargv[i], define_equal, define_equal_size) == 0)
        {
            g_include_directories.push_back(&wargv[i][define_equal_len]);
            continue;
        }

        // otherwise...
        if (g_input_file == NULL)
        {
            g_input_file = wargv[i];
        }
        else if (g_output_file == NULL)
        {
            g_output_file = wargv[i];
        }
        else
        {
            fprintf(stderr, "ERROR: Too many arguments\n");
            return 1;
        }
    }

    if (!check_cpp_exe())
    {
        printf("ERROR: Unable to find cpp.exe\n");
        return 2;
    }

    return just_do_it();
}
