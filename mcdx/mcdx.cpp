// mcdx.hpp --- Message Compiler by katahiromz
//////////////////////////////////////////////////////////////////////////////
// RisohEditor --- Another free Win32 resource editor
// Copyright (C) 2017-2018 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
// 
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//////////////////////////////////////////////////////////////////////////////

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
    EXITCODE_NOT_FOUND_CPP,
    EXITCODE_NOT_SUPPORTED_YET
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
    printf("FORMAT is one of rc, res, bin or coff, and is deduced from the file name\n");
    printf("Report bugs to <katayama.hirofumi.mz@gmail.com>\n");
}

void show_version(void)
{
    printf("mcdx ver.0.7\n");
    printf("Copyright (C) 2018 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>.\n");
    printf("This program is free software; you may redistribute it under the terms of\n");
    printf("the GNU General Public License version 3 or (at your option) any later version.\n");
    printf("This program has absolutely no warranty.\n");
}

////////////////////////////////////////////////////////////////////////////

WCHAR g_szCppExe[MAX_PATH] = L"";
WCHAR g_szWindResExe[MAX_PATH] = L"";

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
int g_value = 0;

typedef std::map<LANGID, MessageRes> msg_tables_type;
msg_tables_type g_msg_tables;

int syntax_error(void)
{
    fprintf(stderr, "%s (%d): ERROR: Syntax error\n", g_strFile.c_str(), g_nLineNo);
    return EXITCODE_SYNTAX_ERROR;
}

BOOL check_cpp_exe(VOID)
{
    WCHAR szPath[MAX_PATH + 64], *pch;

    SearchPathW(NULL, L"cpp.exe", NULL, _countof(szPath), szPath, &pch);
    if (::GetFileAttributesW(szPath) != INVALID_FILE_ATTRIBUTES)
    {
        lstrcpynW(g_szCppExe, szPath, MAX_PATH);
        return TRUE;
    }

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

BOOL check_windres_exe(VOID)
{
    WCHAR szPath[MAX_PATH + 64], *pch;

    SearchPathW(NULL, L"windres.exe", NULL, _countof(szPath), szPath, &pch);
    if (::GetFileAttributesW(szPath) != INVALID_FILE_ATTRIBUTES)
    {
        lstrcpynW(g_szWindResExe, szPath, MAX_PATH);
        return TRUE;
    }

    GetModuleFileNameW(NULL, szPath, _countof(szPath));
    pch = wcsrchr(szPath, L'\\');
    lstrcpyW(pch, L"\\windres.exe");
    if (::GetFileAttributesW(szPath) == INVALID_FILE_ATTRIBUTES)
    {
        lstrcpyW(pch, L"\\data\\bin\\windres.exe");
        if (::GetFileAttributesW(szPath) == INVALID_FILE_ATTRIBUTES)
        {
            lstrcpyW(pch, L"\\..\\data\\bin\\windres.exe");
            if (::GetFileAttributesW(szPath) == INVALID_FILE_ATTRIBUTES)
            {
                lstrcpyW(pch, L"\\..\\..\\data\\bin\\windres.exe");
                if (::GetFileAttributesW(szPath) == INVALID_FILE_ATTRIBUTES)
                {
                    lstrcpyW(pch, L"\\..\\..\\..\\data\\bin\\windres.exe");
                    if (::GetFileAttributesW(szPath) == INVALID_FILE_ATTRIBUTES)
                    {
                        return FALSE;
                    }
                }
            }
        }
    }
    lstrcpynW(g_szWindResExe, szPath, MAX_PATH);
    return TRUE;
}

bool do_directive_line(char*& ptr)
{
    // # line "file"
    char *ptr1 = ptr;
    while (std::isdigit(*ptr))
    {
        ++ptr;
    }
    char *ptr2 = ptr;
    ptr = mstr_skip_space(ptr);
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

int do_mode_1(char*& ptr, int& nMode, bool& do_retry)
{
    ptr = mstr_skip_space(ptr);
    if (*ptr == '{')
    {
        nMode = 2;
        ++ptr;
    }
    else if (*ptr == '}')
    {
        return syntax_error();
    }
    else if (memcmp(ptr, "BEGIN", 5) == 0)
    {
        nMode = 2;
        ptr += 5;
    }
    ptr = mstr_skip_space(ptr);
    if (nMode != 2)
    {
        if (*ptr && !std::isdigit(*ptr))
        {
            return syntax_error();
        }
    }
    return EXITCODE_SUCCESS;
}

int do_mode_2(char*& ptr, int& nMode, bool& do_retry)
{
    ptr = mstr_skip_space(ptr);
    if (*ptr == '{')
    {
        return syntax_error();
    }
    else if (*ptr == '}')
    {
        ++ptr;
        nMode = 0;
        do_retry = true;
        return EXITCODE_SUCCESS;
    }
    else if (memcmp(ptr, "END", 3) == 0 &&
             (std::isspace(ptr[3]) || ptr[3] == 0))
    {
        ptr += 3;
        nMode = 0;
        do_retry = true;
        return EXITCODE_SUCCESS;
    }
    if (*ptr)
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
        if (parser.parse())
        {
            if (!eval_ast(parser.ast(), g_value))
            {
                return syntax_error();
            }
        }
        else
        {
            return syntax_error();
        }

        if (*ptr == ',')
        {
            ++ptr;
            nMode = 3;
            do_retry = true;
            return EXITCODE_SUCCESS;
        }
        else if (*ptr == '\"')
        {
            nMode = 3;
            do_retry = true;
            return EXITCODE_SUCCESS;
        }
        else if (*ptr == 0)
        {
            nMode = 3;
        }
        else
        {
            return syntax_error();
        }
    }

    return EXITCODE_SUCCESS;
}

int do_mode_3(char*& ptr, int& nMode, bool& do_retry)
{
    ptr = mstr_skip_space(ptr);
    if (*ptr == ',')
    {
        ++ptr;
    }
    ptr = mstr_skip_space(ptr);
    if (*ptr == '"')
    {
        MStringA str = ptr;
        mstr_unquote(str);

        MStringW wstr(MAnsiToWide(g_wCodePage, str.c_str()).c_str());
        g_msg_tables[g_langid].m_map[(DWORD)g_value] = wstr;

        const char *ptr0 = ptr;
        guts_quote(str, ptr0);
        ptr = const_cast<char *>(ptr0);

        nMode = 2;
        do_retry = true;
        return EXITCODE_SUCCESS;
    }

    if (*ptr != 0)
    {
        return syntax_error();
    }

    return EXITCODE_SUCCESS;
}

int do_directive(char*& ptr)
{
    ++ptr;
    ptr = mstr_skip_space(ptr);
    if (std::isdigit(*ptr))
    {
        do_directive_line(ptr);
    }
    else if (memcmp(ptr, "pragma", 6) == 0)
    {
        // #pragma
        ptr += 6;
        char *ptr1 = ptr;
        ptr = mstr_skip_space(ptr);
        char *ptr2 = ptr;
        if (memcmp(ptr, "pack", 4) == 0)
        {
            // #pragma pack...
        }
        else if (memcmp(ptr, "code_page", 9) == 0)
        {
            ptr += 9;
            ptr = mstr_skip_space(ptr);
            if (*ptr == '(')
            {
                ++ptr;
                ptr = mstr_skip_space(ptr);
                // #pragma code_page(...)
                WORD wCodePage = 0;
                if (std::isdigit(*ptr))
                {
                    wCodePage = WORD(strtol(ptr, NULL, 0));
                }
                while (std::isalnum(*ptr))
                {
                    ++ptr;
                }
                ptr = mstr_skip_space(ptr);
                if (*ptr == ')')
                {
                    ++ptr;
                    g_wCodePage = wCodePage;
                }
                else
                {
                    fprintf(stderr, "%s (%d): WARNING: Invalid pragma: %s\n", g_strFile.c_str(), g_nLineNo, ptr2);
                }
            }
            else
            {
                fprintf(stderr, "%s (%d): WARNING: Invalid pragma: %s\n", g_strFile.c_str(), g_nLineNo, ptr2);
            }
        }
        else
        {
            fprintf(stderr, "%s (%d): WARNING: Unknown pragma: %s\n", g_strFile.c_str(), g_nLineNo, ptr2);
        }
    }

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

    // parse lines
    INT nMode = 0;
    BYTE bPrimLang = 0, bSubLang = 0;
    for (size_t i = 0; i < lines.size(); ++i, ++g_nLineNo)
    {
        std::string& line = lines[i];
        char *ptr = &line[0];
        if (*ptr == '#')
        {
            if (int ret = do_directive(ptr))
                return ret;

            continue;
        }
        else if (memcmp("LANGUAGE", ptr, 8) == 0)
        {
            // LANGUAGE (primary), (sublang)
            ptr = &line[8];
            nMode = -1;
        }
retry:
        if (nMode == -1 && *ptr)    // after LANGUAGE
        {
            ptr = mstr_skip_space(ptr);
            if (std::isdigit(*ptr))
            {
                nMode = -2;
            }
        }
        if (nMode == -2 && *ptr)    // expect PRIMARYLANGID
        {
            ptr = mstr_skip_space(ptr);
            char *ptr0 = ptr;
            while (std::isalnum(*ptr))
            {
                ++ptr;
            }
            if (std::isdigit(*ptr0))
            {
                bPrimLang = (BYTE)strtoul(ptr0, NULL, 0);
                nMode = -3;
            }
            else if (*ptr)
            {
                return syntax_error();
            }
        }
        if (nMode == -3 && *ptr)    // expect comma
        {
            ptr = mstr_skip_space(ptr);
            if (*ptr == ',')
            {
                ++ptr;
                nMode = -4;
            }
        }
        if (nMode == -4 && *ptr)    // expect SUBLANGID
        {
            ptr = mstr_skip_space(ptr);
            if (std::isdigit(*ptr))
            {
                bSubLang = (BYTE)strtoul(ptr, NULL, 0);
                g_langid = MAKELANGID(bPrimLang, bSubLang);
                nMode = 0;
            }
            else if (*ptr)
            {
                return syntax_error();
            }
        }
        if (nMode == 0 && *ptr) // out of MESSAGETABLEDX { ... }
        {
            ptr = mstr_skip_space(ptr);
            if (memcmp("MESSAGETABLEDX", ptr, 14) == 0)
            {
                nMode = 1;
                ptr += 14;
                ptr = mstr_skip_space(ptr);
            }
        }
        if (nMode == 1 && *ptr) // after MESSAGETABLEDX
        {
            bool do_retry = false;
            if (int ret = do_mode_1(ptr, nMode, do_retry))
                return ret;
            if (do_retry)
                goto retry;
        }
        if (nMode == 2 && *ptr) // in MESSAGETABLEDX { ... }
        {
            bool do_retry = false;
            if (int ret = do_mode_2(ptr, nMode, do_retry))
                return ret;
            if (do_retry)
                goto retry;
        }
        if (nMode == 3 && *ptr)
        {
            bool do_retry = false;
            if (int ret = do_mode_3(ptr, nMode, do_retry))
                return ret;
            if (do_retry)
                goto retry;
        }
    }
    if (nMode != 0)
    {
        return syntax_error();
    }

    return EXITCODE_SUCCESS;
}

int save_rc(wchar_t *output_file)
{
    FILE *fp;
    if (output_file)
    {
        fp = _wfopen(output_file, L"wb");
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
        fprintf(fp, "#ifdef MCDX_INVOKED\r\n");
        fprintf(fp, "LANGUAGE 0x%02X, 0x%02X\r\n",
                PRIMARYLANGID(it->first), SUBLANGID(it->first));

        std::wstring wstr = it->second.Dump();
        std::string str = MWideToAnsi(CP_ACP, wstr.c_str()).c_str();

        fputs(str.c_str(), fp);
        fprintf(fp, "#endif\r\n");
    }

    if (output_file)
        fclose(fp);

    if (ferror(fp))
    {
        if (output_file)
            DeleteFile(output_file);
        fprintf(stderr, "ERROR: Unable to write output file.\n");
        return EXITCODE_CANNOT_OPEN;
    }

    return EXITCODE_SUCCESS;
}

int save_res(wchar_t *output_file)
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

        header.DataSize = DWORD(stream.size());
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
    if (output_file)
    {
        fp = _wfopen(output_file, L"wb");
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

    if (output_file)
        fclose(fp);

    if (ferror(fp))
    {
        if (output_file)
            DeleteFile(output_file);
        fprintf(stderr, "ERROR: Unable to write output file.\n");
        return EXITCODE_CANNOT_OPEN;
    }

    return EXITCODE_SUCCESS;
}

int save_coff(wchar_t *output_file)
{
    TCHAR szTempPath[MAX_PATH], szTempFile[MAX_PATH];
    GetTempPath(_countof(szTempPath), szTempPath);
    GetTempFileName(szTempPath, TEXT("res"), 0, szTempFile);

    if (int ret = save_res(szTempFile))
    {
        DeleteFile(szTempFile);
        return ret;
    }

    MStringW strCommandLine;
    strCommandLine += L"\"";
    strCommandLine += g_szWindResExe;
    strCommandLine += L"\" \"";
    strCommandLine += szTempFile;
    if (output_file)
    {
        strCommandLine += L"\" \"";
        strCommandLine += output_file;
        strCommandLine += L"\"";
    }
    else
    {
        strCommandLine += L"\" -O coff";
    }

    // create a process
    MProcessMaker maker;
    maker.SetShowWindow(SW_HIDE);
    maker.SetCreationFlags(CREATE_NEW_CONSOLE);
    MFile hInputWrite, hOutputRead;
    SetEnvironmentVariableW(L"LANG", L"en_US");
    if (maker.PrepareForRedirect(&hInputWrite, &hOutputRead) &&
        maker.CreateProcessDx(NULL, strCommandLine.c_str()))
    {
        std::string strOutput;
        maker.ReadAll(strOutput, hOutputRead);

        if (maker.GetExitCode() == 0)
        {
            DeleteFile(szTempFile);
            return EXITCODE_SUCCESS;
        }
        else if (strOutput.find(": no resources") != std::string::npos)
        {
            strOutput.clear();
            DeleteFile(szTempFile);
            return EXITCODE_SUCCESS;
        }

        fputs(strOutput.c_str(), stderr);
    }
    fprintf(stderr, "ERROR: Failed to create process\n");
    DeleteFile(szTempFile);
    return EXITCODE_FAIL_TO_PREPROCESS;
}

int save_bin(wchar_t *output_file)
{
    MessageRes msg_res = g_msg_tables.begin()->second;

    MByteStreamEx stream;
    msg_res.SaveToStream(stream);

    FILE *fp;
    if (output_file)
    {
        fp = _wfopen(output_file, L"wb");
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

    if (output_file)
        fclose(fp);

    if (ferror(fp))
    {
        if (output_file)
            DeleteFile(output_file);
        fprintf(stderr, "ERROR: Unable to write output file.\n");
        return EXITCODE_CANNOT_OPEN;
    }

    return EXITCODE_SUCCESS;
}

int load_rc(wchar_t *input_file)
{
    // definitions minus undefinitions
    for (size_t i = 0; i < g_undefinitions.size(); ++i)
    {
        for (size_t k = 0; k < g_definitions.size(); ++k)
        {
            if (g_definitions[k].find(g_undefinitions[i]) == 0)
            {
                size_t len = g_undefinitions[i].size();
                if (g_definitions[k].c_str()[len] == 0 ||
                    g_definitions[k].c_str()[len] == L'=')
                {
                    g_definitions.erase(g_definitions.begin() + k);
                    --k;
                }
            }
        }
    }

    // build up command line
    MStringW strCommandLine;
    strCommandLine += L"\"";
    strCommandLine += g_szCppExe;
    strCommandLine += L"\" ";
    for (size_t i = 0; i < g_definitions.size(); ++i)
    {
        strCommandLine += L" -D";
        strCommandLine += g_definitions[i];
    }
    strCommandLine += L" \"";
    strCommandLine += input_file;
    strCommandLine += L"\"";

    g_strFile = MWideToAnsi(CP_ACP, input_file).c_str();
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

int load_bin(wchar_t *input_file)
{
    MFile file(input_file);
    if (!file)
    {
        fprintf(stderr, "ERROR: Unable to open input file.\n");
        return EXITCODE_CANNOT_OPEN;
    }
    char buf[256];
    DWORD dwSize;
    std::string strContents;
    while (file.ReadFile(buf, sizeof(buf), &dwSize) && dwSize)
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

int load_res(wchar_t *input_file)
{
    MFile file(input_file);
    if (!file)
    {
        fprintf(stderr, "ERROR: Unable to open input file.\n");
        return EXITCODE_CANNOT_OPEN;
    }

    char buf[512];
    DWORD dwSize;
    std::string strContents;
    while (file.ReadFile(buf, sizeof(buf), &dwSize) && dwSize)
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
        if (int ret = load_rc(g_input_file))
            return ret;
    }
    else if (lstrcmpiW(g_inp_format, L"res") == 0)
    {
        if (int ret = load_res(g_input_file))
            return ret;
    }
    else if (lstrcmpiW(g_inp_format, L"bin") == 0)
    {
        if (int ret = load_bin(g_input_file))
            return ret;
    }
    else if (lstrcmpiW(g_inp_format, L"coff") == 0)
    {
        fprintf(stderr, "ERROR: COFF input format is not supported yet.\n");
        return EXITCODE_NOT_SUPPORTED_YET;
    }
    else
    {
        fprintf(stderr, "ERROR: invalid input format\n");
        return EXITCODE_INVALID_ARGUMENT;
    }

    if (lstrcmpiW(g_out_format, L"rc") == 0)
    {
        return save_rc(g_output_file);
    }
    else if (lstrcmpiW(g_out_format, L"res") == 0)
    {
        return save_res(g_output_file);
    }
    else if (lstrcmpiW(g_out_format, L"bin") == 0)
    {
        return save_bin(g_output_file);
    }
    else if (lstrcmpiW(g_out_format, L"coff") == 0)
    {
        return save_coff(g_output_file);
    }
    else
    {
        fprintf(stderr, "ERROR: invalid output format\n");
        return EXITCODE_INVALID_ARGUMENT;
    }
}

const wchar_t *get_format(const wchar_t *file_path)
{
    LPCWSTR pch = wcsrchr(file_path, L'.');
    if (pch == NULL)
    {
        return L"rc";
    }
    else if (lstrcmpiW(pch, L".rc") == 0)
    {
        return L"rc";
    }
    else if (lstrcmpiW(pch, L".res") == 0)
    {
        return L"res";
    }
    else if (lstrcmpiW(pch, L".bin") == 0)
    {
        return L"bin";
    }
    else if (lstrcmpiW(pch, L".o") == 0 ||
             lstrcmpiW(pch, L".obj") == 0 ||
             lstrcmpiW(pch, L".coff") == 0)
    {
        return L"coff";
    }
    else
    {
        return L"rc";
    }
}

//////////////////////////////////////////////////////////////////////////////

int main(int argc, char **argv)
{
    LPWSTR *wargv = CommandLineToArgvW(GetCommandLineW(), &argc);

    g_definitions.push_back(L"RC_INVOKED");
    g_definitions.push_back(L"MCDX_INVOKED");

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
        if (memcmp(wargv[i], L"-D", 2 * sizeof(wchar_t)) == 0)
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
        if (memcmp(wargv[i], L"-U", 2 * sizeof(wchar_t)) == 0)
        {
            g_undefinitions.push_back(&wargv[i][2]);
            continue;
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
        // invalid argument?
        if (wargv[i][0] == L'-' || wargv[i][0] == L'/')
        {
            MStringA str = MWideToAnsi(CP_ACP, wargv[i]).c_str();
            fprintf(stderr, "ERROR: Invalid argument --- '%s'\n", str.c_str());
            show_help();
            return EXITCODE_INVALID_ARGUMENT;
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
        g_inp_format = get_format(g_input_file);
    }

    if (g_out_format == NULL)
    {
        if (g_output_file)
        {
            g_out_format = get_format(g_output_file);
        }
        else
        {
            g_out_format = L"rc";
        }
    }

    if (!check_cpp_exe())
    {
        fprintf(stderr, "ERROR: Unable to find cpp.exe\n");
        return EXITCODE_NOT_FOUND_CPP;
    }

    if (!check_windres_exe())
    {
        fprintf(stderr, "ERROR: Unable to find windres.exe\n");
        return EXITCODE_NOT_FOUND_CPP;
    }

    int ret = just_do_it();

    if (s_szTempFile[0])
        DeleteFile(s_szTempFile);

    return ret;
}

//////////////////////////////////////////////////////////////////////////////
