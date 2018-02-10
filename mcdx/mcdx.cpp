////////////////////////////////////////////////////////////////////////////

#define _CRT_SECURE_NO_WARNINGS

#include "MProcessMaker.hpp"
#include "MString.hpp"
#include "MacroParser.hpp"
#include "MessageRes0.hpp"
#include <cctype>

////////////////////////////////////////////////////////////////////////////

#ifndef _countof
    #define _countof(array)     (sizeof(array) / sizeof(array[0]))
#endif

////////////////////////////////////////////////////////////////////////////

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

////////////////////////////////////////////////////////////////////////////

WCHAR g_szBinDir[MAX_PATH] = L"";
WCHAR g_szCppExe[MAX_PATH] = L"";

wchar_t *g_input_file = NULL;
wchar_t *g_output_file = NULL;
const wchar_t *g_inp_format = L"rc";
const wchar_t *g_out_format = L"res";

std::vector<MStringW> g_include_directories;
std::vector<MStringW> g_definitions;
std::vector<MStringW> g_undefinitions;

WORD g_wCodePage = CP_UTF8;

MessageRes g_msg_res;

BOOL check_cpp_exe(VOID)
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
    pch = wcsrchr(szPath, L'\\');
    *pch = 0;
    lstrcpynW(g_szBinDir, szPath, MAX_PATH);
    return TRUE;
}

bool do_pragma_line(char*& ptr, std::string& strFile, int& nLineNo)
{
    // # line "file"
    char *ptr1 = ptr;
    while (std::isdigit(*ptr))
    {
        ++ptr;
    }
    char *ptr2 = ptr;
    while (std::isspace(*ptr))
    {
        ++ptr;
    }
    char *ptr3 = ptr;
    while (*ptr)
    {
        ++ptr;
    }
    *ptr2 = 0;

    nLineNo = strtol(ptr1, NULL, 0);

    std::string file = ptr3;
    mstr_unquote(file);
    strFile = file;

    return true;
}

LANGID do_language(char*& ptr)
{
    // LANGUAGE (primary), (sublang)
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
    WORD wPrimaryLang = (WORD)strtoul(ptr0, NULL, 0);
    WORD wSubLang = (WORD)strtoul(ptr2, NULL, 0);
    return MAKELANGID(wPrimaryLang, wSubLang);
}

int do_entry(char*& ptr)
{
    // get number string
    char *ptr0 = ptr;
    while (*ptr && *ptr != ',')
    {
        ++ptr;
    }
    char *ptr1 = ptr;
    MStringA str(ptr0, ptr1);

    // parse
    using namespace MacroParser;
    StringScanner scanner(str);
    TokenStream stream(scanner);
    stream.read_tokens();
    Parser parser(stream);

    int value = 0;
    if (parser.parse())
    {
        if (eval_ast(parser.ast(), value))
        {
            ;
        }
        else
        {
            fprintf(stderr, "ERROR: Parse error\n");
            return 9;
        }
    }
    else
    {
        fprintf(stderr, "ERROR: Syntax error\n");
        return 8;
    }

    // get string value
    while (*ptr1 == ',' || std::isspace(*ptr1))
    {
        ++ptr1;
    }
    str = ptr1;
    mstr_unquote(str);

    MStringW wstr(MAnsiToWide(g_wCodePage, str.c_str()).c_str());
    g_msg_res.m_map[(DWORD)value] = wstr;

    return 0;
}

int eat_output(const std::string& strOutput)
{
    g_msg_res.clear();

    std::vector<std::string> lines;
    mstr_split(lines, strOutput, "\n");

    for (size_t i = 0; i < lines.size(); ++i)
    {
        mstr_trim(lines[i]);
    }

    INT nMode = 0;
    LANGID langid = 0;
    std::string strFile = "(anonymous)";
    int nLineNo = 1;
    WORD g_wCodePage = CP_UTF8;
    for (size_t i = 0; i < lines.size(); ++i, ++nLineNo)
    {
        std::string& line = lines[i];
        if (line[0] == '#')
        {
            char *ptr = &line[1];
            while (std::isspace(*ptr))
            {
                ++ptr;
            }
            if (std::isdigit(*ptr))
            {
                do_pragma_line(ptr, strFile, nLineNo);
            }
            else if (memcmp(ptr, "pragma", 6) == 0)
            {
                // #pragma
                char *ptr1 = ptr + 6;
                while (std::isspace(*ptr))
                {
                    ++ptr;
                }
                char *ptr2 = ptr;
                if (memcmp(ptr, "code_page(", 10) == 0) // ')'
                {
                    // #pragma code_page(...)
                    ptr += 10;
                    g_wCodePage = WORD(strtol(ptr, NULL, 0));
                }
            }
        }
        else if (memcmp("LANGUAGE", &line[0], 8) == 0)
        {
            // LANGUAGE (primary), (sublang)
            char *ptr = &line[8];
            langid = do_language(ptr);
            continue;
        }
        else 
        {
            char *ptr = &line[0];
            if (nMode == 0) // out of MESSAGETABLEDX { ... }
            {
                if (memcmp("MESSAGETABLEDX", ptr, 14) == 0)
                {
                    nMode = 1;
                    ptr += 14;
                    while (std::isspace(*ptr))
                    {
                        ++ptr;
                    }
                }
            }
            if (nMode == 1) // after MESSAGETABLEDX
            {
                if (*ptr == '{')
                {
                    nMode = 2;
                    ++ptr;
                }
                else if (memcmp(ptr, "BEGIN", 5) == 0)
                {
                    nMode = 2;
                    ptr += 5;
                }
                while (std::isspace(*ptr))
                {
                    ++ptr;
                }
            }
            if (nMode == 2) // in MESSAGETABLEDX { ... }
            {
                if (*ptr == '}' || memcmp(ptr, "END", 3) == 0)
                {
                    nMode = 0;
                    continue;
                }
                if (std::isdigit(*ptr))
                {
                    if (int ret = do_entry(ptr))
                        return ret;
                }
                else
                {
					;
                }
            }
        }
    }
    return 0;
}

int do_emit(void)
{
    std::wstring wstr = g_msg_res.Dump();
	std::string str = MWideToAnsi(CP_ACP, wstr.c_str()).c_str();
    printf("%s\n", str.c_str());
    return 0;
}

int just_do_it(void)
{
    // build up command line
    MStringW strCommandLine;
    strCommandLine += L"\"";
    strCommandLine += g_szCppExe;
    strCommandLine += L"\" ";
    for (size_t i = 0; i < g_definitions.size(); ++i)
    {
        strCommandLine += L" ";
        strCommandLine += g_definitions[i];
    }
    strCommandLine += L" \"";
    strCommandLine += g_input_file;
    strCommandLine += L"\"";

    printf("%s\n", 
    MWideToAnsi(CP_ACP, strCommandLine.c_str()).c_str());

    // create a process
    MProcessMaker maker;
    maker.SetShowWindow(SW_HIDE);
    maker.SetCreationFlags(CREATE_NEW_CONSOLE);
    maker.SetCurrentDirectoryW(g_szBinDir);
    MFile hInputWrite, hOutputRead;
    if (maker.PrepareForRedirect(&hInputWrite, &hOutputRead) &&
        maker.CreateProcessDx(NULL, strCommandLine.c_str()))
    {
        std::string strOutput;
        maker.ReadAll(strOutput, hOutputRead);

        if (maker.GetExitCode() == 0)
        {
            // eat the output
            if (int ret = eat_output(strOutput))
                return ret;

            return do_emit();
        }

        fputs(strOutput.c_str(), stdout);
    }
    DWORD dwError = GetLastError();
    printf("%ld, %ld\n", dwError, GetFileAttributesW(g_input_file));
    return -1;
}

int main(int argc, char **argv)
{
    LPWSTR *wargv = CommandLineToArgvW(GetCommandLineW(), &argc);

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
        if (memcmp(wargv[i], include_dir_equal, include_dir_equal_size) == 0)
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
