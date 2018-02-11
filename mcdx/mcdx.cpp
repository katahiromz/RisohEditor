////////////////////////////////////////////////////////////////////////////

#define _CRT_SECURE_NO_WARNINGS

#include "MProcessMaker.hpp"
#include "MString.hpp"
#include "MacroParser.hpp"
#include "MessageRes0.hpp"
#include "ResHeader.hpp"
#include <cctype>

////////////////////////////////////////////////////////////////////////////

#ifndef _countof
    #define _countof(array)     (sizeof(array) / sizeof(array[0]))
#endif

enum EXITCODE
{
    EXITCODE_SUCCESS = 0,
    EXITCODE_INVALID_ARGUMENT,
    EXITCODE_FAIL_TO_PREPROCESS,
    EXITCODE_SYNTAX_ERROR,
    EXITCODE_CANNOT_OPEN,
    EXITCODE_CANNOT_WRITE,
    EXITCODE_INVALID_DATA,
    EXITCODE_NOT_FOUND_CPP
};

////////////////////////////////////////////////////////////////////////////

void show_help(void)
{
    printf("mcdx --- Message Compiler Dirty Extension by katahiromz\n");
    printf("Copyright (C) 2018 Katayama Hirofumi MZ. License: GPLv3.\n");
    printf("\n");
    printf("Usage: mcdx [option(s)] [input-file] [output-file]\n");
    printf("Options:\n");
    printf("  -i --input=<file>            Name input file\n");
    printf("  -o --output=<file>           Name output file\n");
    printf("  -J --input-format=<format>   Specify input format\n");
    printf("  -O --output-format=<format>  Specify output format\n");
    printf("  -I --include-dir=<dir>       Include directory when preprocessing rc file\n");
    printf("  -D --define=<sym>[=<val>]    Define SYM when preprocessing rc file\n");
    printf("  -U --undefine <sym>          Undefine SYM when preprocessing rc file\n");
    printf("  -c --codepage=<codepage>     Specify default codepage\n");
    printf("  -l --language=<val>          Set language when reading rc file\n");
    printf("FORMAT is one of rc, res or bin, and is deduced from the file name\n");
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
const wchar_t *g_inp_format = NULL;
const wchar_t *g_out_format = NULL;

std::vector<MStringW> g_include_directories;
std::vector<MStringW> g_definitions;
std::vector<MStringW> g_undefinitions;

std::string g_strFile = "(anonymous)";
int g_nLineNo = 0;

LANGID g_langid = 0;
WORD g_wCodePage = CP_UTF8;

typedef std::map<LANGID, MessageRes> msg_tables_type;
msg_tables_type g_msg_tables;

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

bool do_pragma_line(char*& ptr)
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

    g_nLineNo = strtol(ptr1, NULL, 0) - 1;

    std::string file = ptr3;
    mstr_unquote(file);
    g_strFile = file;

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
    while (*ptr && *ptr != ',' && *ptr != '\"')
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
            fprintf(stderr, "%s (%d): ERROR: Syntax error\n", g_strFile.c_str(), g_nLineNo);
            return EXITCODE_SYNTAX_ERROR;
        }
    }
    else
    {
        fprintf(stderr, "%s (%d): ERROR: Syntax error\n", g_strFile.c_str(), g_nLineNo);
        return EXITCODE_SYNTAX_ERROR;
    }

    // get string value
    while (*ptr1 == ',' || std::isspace(*ptr1))
    {
        ++ptr1;
    }
    str = ptr1;
    mstr_unquote(str);

    MStringW wstr(MAnsiToWide(g_wCodePage, str.c_str()).c_str());
    g_msg_tables[g_langid].m_map[(DWORD)value] = wstr;

    return EXITCODE_SUCCESS;
}

int eat_output(const std::string& strOutput)
{
    g_msg_tables.clear();

    std::vector<std::string> lines;
    mstr_split(lines, strOutput, "\n");

    for (size_t i = 0; i < lines.size(); ++i)
    {
        mstr_trim(lines[i]);
    }

    INT nMode = 0;
    for (size_t i = 0; i < lines.size(); ++i, ++g_nLineNo)
    {
        std::string& line = lines[i];
        if (line[0] == '#')
        {
            // directive
            char *ptr = &line[1];
            while (std::isspace(*ptr))
            {
                ++ptr;
            }
            if (std::isdigit(*ptr))
            {
                do_pragma_line(ptr);
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
                else
                {
                    fprintf(stderr, "%s (%d): WARNING: Unknown pragma\n", g_strFile.c_str(), g_nLineNo);
                }
            }
        }
        else if (memcmp("LANGUAGE", &line[0], 8) == 0)
        {
            // LANGUAGE (primary), (sublang)
            char *ptr = &line[8];
            g_langid = do_language(ptr);
            continue;
        }
        else
        {
            // otherwise
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
                else if (*ptr == '}')
                {
                    fprintf(stderr, "%s (%d): ERROR: Syntax error\n", g_strFile.c_str(), g_nLineNo);
                    return EXITCODE_SYNTAX_ERROR;
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
                if (nMode != 2)
                {
                    if (*ptr && !std::isdigit(*ptr))
                    {
                        fprintf(stderr, "%s (%d): ERROR: Syntax error\n", g_strFile.c_str(), g_nLineNo);
                        return EXITCODE_SYNTAX_ERROR;
                    }
                }
            }
            if (nMode == 2) // in MESSAGETABLEDX { ... }
            {
                if (*ptr == '{')
                {
                    fprintf(stderr, "%s (%d): ERROR: Syntax error\n", g_strFile.c_str(), g_nLineNo);
                    return EXITCODE_SYNTAX_ERROR;
                }
                else if (*ptr == '}' || memcmp(ptr, "END", 3) == 0)
                {
                    nMode = 0;
                    continue;
                }
                if (std::isdigit(*ptr))
                {
                    if (int ret = do_entry(ptr))
                        return ret;
                }
                else if (*ptr)
                {
                    fprintf(stderr, "%s (%d): ERROR: Syntax error\n", g_strFile.c_str(), g_nLineNo);
                    return EXITCODE_SYNTAX_ERROR;
                }
            }
        }
    }
    if (nMode != 0)
    {
        fprintf(stderr, "%s (%d): ERROR: Syntax error\n", g_strFile.c_str(), g_nLineNo);
        return EXITCODE_SYNTAX_ERROR;
    }

    return EXITCODE_SUCCESS;
}

int save_rc(void)
{
    FILE *fp;

    if (g_output_file)
    {
        fp = _wfopen(g_output_file, L"wb");
        if (!fp)
        {
            fprintf(stderr, "ERROR: Unable to open output file.\n");
            return EXITCODE_CANNOT_OPEN;
        }
    }
    else
    {
        fp = stdout;
    }

    msg_tables_type::iterator it, end = g_msg_tables.end();
    for (it = g_msg_tables.begin(); it != end; ++it)
    {
        fprintf(fp, "LANGUAGE 0x%02X, 0x%02X\r\n",
                PRIMARYLANGID(it->first), SUBLANGID(it->first));

        std::wstring wstr = it->second.Dump();
        std::string str = MWideToAnsi(CP_ACP, wstr.c_str()).c_str();

        fprintf(fp, "%s\r\n", str.c_str());
    }

    if (g_output_file)
        fclose(fp);

    if (ferror(fp))
    {
        DeleteFile(g_output_file);
        fprintf(stderr, "ERROR: Unable to write output file.\n");
        return EXITCODE_CANNOT_OPEN;
    }

    return EXITCODE_SUCCESS;
}

int save_res(void)
{
    MByteStreamEx bs;
    ResHeader header;
    if (!header.WriteTo(bs))
        return EXITCODE_INVALID_DATA;

    msg_tables_type::iterator it, end = g_msg_tables.end();
    for (it = g_msg_tables.begin(); it != end; ++it)
    {
        MByteStreamEx stream;
        it->second.SaveToStream(stream);

        header.DataSize = stream.size();
        header.HeaderSize = header.GetHeaderSize(RT_MESSAGETABLE, 1);
        if (header.HeaderSize == 0 || header.HeaderSize >= 0x10000)
            return FALSE;

        header.type = RT_MESSAGETABLE;
        header.name = 1;
        header.DataVersion = 0;
        header.MemoryFlags = MEMORYFLAG_DISCARDABLE | MEMORYFLAG_PURE |
                             MEMORYFLAG_MOVEABLE;
        header.LanguageId = it->first;
        header.Version = 0;
        header.Characteristics = 0;

        if (!header.WriteTo(bs))
            return FALSE;

        if (!bs.WriteData(&stream[0], stream.size()))
            return FALSE;

        bs.WriteDwordAlignment();
    }

    FILE *fp;
    if (g_output_file)
    {
        fp = _wfopen(g_output_file, L"wb");
        if (!fp)
        {
            fprintf(stderr, "ERROR: Unable to open output file.\n");
            return EXITCODE_CANNOT_OPEN;
        }
    }
    else
    {
        fp = stdout;
    }

    fwrite(&bs[0], bs.size(), 1, fp);

    if (g_output_file)
        fclose(fp);

    if (ferror(fp))
    {
        DeleteFile(g_output_file);
        fprintf(stderr, "ERROR: Unable to write output file.\n");
        return EXITCODE_CANNOT_OPEN;
    }

    return EXITCODE_SUCCESS;
}

int save_bin(void)
{
    MessageRes msg_res = g_msg_tables.begin()->second;

    MByteStreamEx stream;
    msg_res.SaveToStream(stream);

    FILE *fp;
    if (g_output_file)
    {
        fp = _wfopen(g_output_file, L"wb");
        if (!fp)
        {
            fprintf(stderr, "ERROR: Unable to open output file.\n");
            return EXITCODE_CANNOT_OPEN;
        }
    }
    else
    {
        fp = stdout;
    }

    fwrite(&stream[0], stream.size(), 1, fp);

    if (g_output_file)
        fclose(fp);

    if (ferror(fp))
    {
        DeleteFile(g_output_file);
        fprintf(stderr, "ERROR: Unable to write output file.\n");
        return EXITCODE_CANNOT_OPEN;
    }

    return EXITCODE_SUCCESS;
}

int load_rc(void)
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

    g_strFile = MWideToAnsi(CP_ACP, g_input_file).c_str();
    g_nLineNo = 1;

    // create a process
    MProcessMaker maker;
    maker.SetShowWindow(SW_HIDE);
    maker.SetCreationFlags(CREATE_NEW_CONSOLE);
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

            return EXITCODE_SUCCESS;
        }

        fputs(strOutput.c_str(), stderr);
    }
    fprintf(stderr, "ERROR: Failed to preprocess\n");
    return EXITCODE_FAIL_TO_PREPROCESS;
}

int load_bin(void)
{
    MFile file(g_input_file);
    if (!file)
    {
        fprintf(stderr, "ERROR: Unable to open input file.\n");
		return EXITCODE_CANNOT_OPEN;
    }
    char buf[256];
    DWORD dwSize;
    std::string strContents;
    while (file.ReadFile(buf, sizeof(buf), &dwSize))
    {
        strContents.append(buf, dwSize);
    }
    file.CloseHandle();

    MByteStreamEx stream(&strContents[0], strContents.size());
    if (!g_msg_tables[g_langid].LoadFromStream(stream))
    {
        fprintf(stderr, "ERROR: Invalid data.\n");
        return EXITCODE_INVALID_DATA;
    }

    return EXITCODE_SUCCESS;
}

int load_res(void)
{
    MFile file(g_input_file);
    if (!file)
    {
        fprintf(stderr, "ERROR: Unable to open input file.\n");
        return EXITCODE_CANNOT_OPEN;
    }

    char buf[256];
    DWORD dwSize;
    std::string strContents;
    while (file.ReadFile(buf, sizeof(buf), &dwSize))
    {
        strContents.append(buf, dwSize);
    }
    file.CloseHandle();

    MByteStreamEx stream(&strContents[0], strContents.size());
    ResHeader header;
    while (header.ReadFrom(stream))
    {
        if (header.DataSize == 0)
        {
            stream.ReadDwordAlignment();
            continue;
        }

        if (header.DataSize > stream.remainder())
        {
            fprintf(stderr, "ERROR: Data is broken, invalid, or not supported.\n");
            return EXITCODE_INVALID_DATA;
        }

        MByteStreamEx bs(header.DataSize);
        if (!stream.ReadData(&bs[0], header.DataSize))
        {
            break;
        }
        if (!g_msg_tables[header.LanguageId].LoadFromStream(bs))
        {
            fprintf(stderr, "ERROR: Data is broken, invalid, or not supported.\n");
            return EXITCODE_INVALID_DATA;
        }

        stream.ReadDwordAlignment();
    }

    return EXITCODE_SUCCESS;
}

int just_do_it(void)
{
    if (lstrcmpiW(g_inp_format, L"rc") == 0)
    {
        if (int ret = load_rc())
            return ret;
    }
    else if (lstrcmpiW(g_inp_format, L"res") == 0)
    {
        if (int ret = load_res())
            return ret;
    }
    else if (lstrcmpiW(g_inp_format, L"bin") == 0)
    {
        if (int ret = load_bin())
            return ret;
    }
    else
    {
        fprintf(stderr, "ERROR: invalid input format\n");
        return EXITCODE_INVALID_ARGUMENT;
    }

    if (lstrcmpiW(g_out_format, L"rc") == 0)
    {
        return save_rc();
    }
    else if (lstrcmpiW(g_out_format, L"res") == 0)
    {
        return save_res();
    }
    else if (lstrcmpiW(g_out_format, L"bin") == 0)
    {
        return save_bin();
    }
    else
    {
        fprintf(stderr, "ERROR: invalid output format\n");
        return EXITCODE_INVALID_ARGUMENT;
    }
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
            return EXITCODE_SUCCESS;
        }
        // show version?
        if (lstrcmpiW(wargv[i], L"--version") == 0)
        {
            show_version();
            return EXITCODE_SUCCESS;
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
                    return EXITCODE_INVALID_ARGUMENT;
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
                return EXITCODE_INVALID_ARGUMENT;
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
                    return EXITCODE_INVALID_ARGUMENT;
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
                return EXITCODE_INVALID_ARGUMENT;
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
                return EXITCODE_INVALID_ARGUMENT;
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
                return EXITCODE_INVALID_ARGUMENT;
            }
        }
        // include directory?
        if (lstrcmpW(wargv[i], L"-I") == 0 ||
            lstrcmpW(wargv[i], L"--include-dir") == 0)
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
                return EXITCODE_INVALID_ARGUMENT;
            }
        }
        if (memcmp(wargv[i], include_dir_equal, include_dir_equal_size) == 0)
        {
            g_include_directories.push_back(&wargv[i][include_dir_equal_len]);
            continue;
        }
        // definition?
        if (lstrcmpW(wargv[i], L"-D") == 0 ||
            lstrcmpW(wargv[i], L"--define") == 0)
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
                return EXITCODE_INVALID_ARGUMENT;
            }
        }
        if (memcmp(wargv[i], L"-D", 2) == 0)
        {
            g_definitions.push_back(&wargv[i][2]);
            continue;
        }
        // undefine?
        if (lstrcmpW(wargv[i], L"-U") == 0 ||
            lstrcmpiW(wargv[i], L"--undefine") == 0)
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
                return EXITCODE_INVALID_ARGUMENT;
            }
        }
        if (memcmp(wargv[i], define_equal, define_equal_size) == 0)
        {
            g_include_directories.push_back(&wargv[i][define_equal_len]);
            continue;
        }
        // codepage?
        if (lstrcmpiW(wargv[i], L"-c") == 0 ||
            lstrcmpiW(wargv[i], L"--codepage") == 0)
        {
            ++i;
            if (i < argc)
            {
                g_wCodePage = (WORD)wcstol(wargv[i], NULL, 0);
                continue;
            }
            else
            {
                fprintf(stderr, "ERROR: -c requires an argument\n");
                return EXITCODE_INVALID_ARGUMENT;
            }
        }
        if (memcmp(wargv[i], L"--codepage=", 11 * sizeof(wchar_t)) == 0)
        {
            g_wCodePage = (WORD)wcstol(&wargv[i][11], NULL, 0);
            continue;
        }
        // language?
        if (lstrcmpiW(wargv[i], L"-l") == 0 ||
            lstrcmpiW(wargv[i], L"--language") == 0)
        {
            ++i;
            if (i < argc)
            {
                WORD w = (WORD)wcstol(wargv[i], NULL, 0);
                BYTE bPrim = LOBYTE(w);
                BYTE bSub = HIBYTE(w);
                g_langid = MAKELANGID(bPrim, bSub);
                continue;
            }
            else
            {
                fprintf(stderr, "ERROR: -c requires an argument\n");
                return EXITCODE_INVALID_ARGUMENT;
            }
        }
        if (memcmp(wargv[i], L"--language=", 11 * sizeof(wchar_t)) == 0)
        {
            WORD w = (WORD)wcstol(&wargv[i][11], NULL, 0);
            BYTE bPrim = LOBYTE(w);
            BYTE bSub = HIBYTE(w);
            g_langid = MAKELANGID(bPrim, bSub);
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
            return EXITCODE_INVALID_ARGUMENT;
        }
    }

    static TCHAR s_szTempFile[MAX_PATH] = TEXT("");
    if (g_input_file == NULL)
    {
        TCHAR szTempPath[MAX_PATH];
        GetTempPath(MAX_PATH, szTempPath);
        GetTempFileName(szTempPath, L"inp", 0, s_szTempFile);

        FILE *fp = _wfopen(s_szTempFile, L"w");
        if (fp == NULL)
        {
            DeleteFile(s_szTempFile);
            fprintf(stderr, "ERROR: Cannot write to temporary file\n");
            return EXITCODE_CANNOT_WRITE;
        }

        char buf[512];
        while (fgets(buf, 512, stdin) != NULL)
        {
            fputs(buf, fp);
        }
        fclose(fp);

        g_input_file = s_szTempFile;
    }

    if (g_inp_format == NULL)
    {
        LPWSTR pch = wcsrchr(g_input_file, L'.');
        if (lstrcmpiW(pch, L".rc") == 0)
        {
            g_inp_format = L"rc";
        }
        else if (lstrcmpiW(pch, L".res") == 0)
        {
            g_inp_format = L"res";
        }
        else if (lstrcmpiW(pch, L".bin") == 0)
        {
            g_inp_format = L"bin";
        }
        else
        {
            g_inp_format = L"rc";
        }
    }

    if (g_out_format == NULL)
    {
        if (g_output_file)
        {
            LPWSTR pch = wcsrchr(g_output_file, L'.');
            if (lstrcmpiW(pch, L".rc") == 0)
            {
                g_out_format = L"rc";
            }
            else if (lstrcmpiW(pch, L".res") == 0)
            {
                g_out_format = L"res";
            }
            else if (lstrcmpiW(pch, L".bin") == 0)
            {
                g_out_format = L"bin";
            }
            else
            {
                g_out_format = L"rc";
            }
        }
        else
        {
            g_out_format = L"rc";
        }
    }

    if (!check_cpp_exe())
    {
        printf("ERROR: Unable to find cpp.exe\n");
        return EXITCODE_NOT_FOUND_CPP;
    }

    int ret = just_do_it();

    if (s_szTempFile[0])
        DeleteFile(s_szTempFile);

    return ret;
}
