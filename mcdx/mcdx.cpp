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
#define _SCL_SECURE_NO_WARNINGS

#define NO_CONSTANTS_DB

#if defined(_WIN32) && !defined(WONVER)
    #include "MProcessMaker.hpp"
#endif
#include "MString.hpp"
#include "MacroParser.hpp"
#include "MessageRes.hpp"

#include "ResHeader.hpp"
#include "getoptwin.h"
#include <cctype>
#include <sys/types.h>
#include <sys/stat.h>

#ifndef RT_MESSAGETABLE
    #define RT_MESSAGETABLE     MAKEINTRESOURCE(11)
#endif

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
    EXITCODE_NOT_FOUND_WINDRES,
    EXITCODE_NOT_SUPPORTED_YET,
    EXITCODE_CANT_MAKE_TEMP
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
    printf("  --preprocessor=<path>        Set preprocessor path\n");
    printf("  --windres=<path>             Set windres path\n");
    printf("FORMAT is one of rc, res, bin or coff, and is deduced from the file name\n");
    printf("Report bugs to <katayama.hirofumi.mz@gmail.com>\n");
}

void show_version(void)
{
    printf("mcdx ver.0.8.2\n");
    printf("Copyright (C) 2018 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>.\n");
    printf("This program is free software; you may redistribute it under the terms of\n");
    printf("the GNU General Public License version 3 or (at your option) any later version.\n");
    printf("This program has absolutely no warranty.\n");
}

////////////////////////////////////////////////////////////////////////////
// file_exists and tmpfilenam

inline bool file_exists(const char *file)
{
    struct stat st;
    return stat(file, &st) == 0;
}

struct DeleteTempFiles
{
    std::vector<std::string> m_files;
    DeleteTempFiles()
    {
    }
    void push_back(const std::string& file)
    {
        m_files.push_back(file);
    }
    ~DeleteTempFiles()
    {
        for (size_t i = 0; i < m_files.size(); ++i)
        {
            _unlink(m_files[i].c_str());
        }
        m_files.clear();
    }
};
DeleteTempFiles g_delete_temp_files;

FILE *tmpfilenam(char *pathname)
{
    FILE *fp;
    int i, k;
    const int retry_count = 64;
    char tmp[MAX_PATH], name[16], file[MAX_PATH];
    char *ptr = getenv("TMP");

    if (!ptr)
        ptr = getenv("TEMP");

    if (ptr)
    {
        strcpy(tmp, ptr);
        mstr_trim_right(tmp, "/\\");
    }
    else
    {
        strcpy(tmp, ".");
    }

    k = 0;
    do
    {
        do
        {
            for (i = 0; i < 8; ++i)
            {
                name[i] = (char)('A' + rand() % ('Z' - 'A' + 1));
            }
            name[i++] = '.';
            name[i++] = 't';
            name[i++] = 'm';
            name[i++] = 'p';
            name[i] = 0;

            strcpy(file, tmp);
            #ifdef _WIN32
                strcat(file, "\\");
            #else
                strcat(file, "/");
            #endif
            strcat(file, name);
            ++k;
        } while (file_exists(file) && k < retry_count);

        fp = fopen(file, "w+b");
    } while (fp == NULL && k < retry_count);

    if (fp)
    {
        g_delete_temp_files.push_back(file);
        strcpy(pathname, file);
    }
    else
    {
        pathname[0] = 0;
    }

    return fp;
}

////////////////////////////////////////////////////////////////////////////

const char *g_cpp = "cpp";
const char *g_windres = "windres";

char *g_input_file = NULL;
char *g_output_file = NULL;
const char *g_inp_format = NULL;
const char *g_out_format = NULL;

std::vector<MStringA> g_include_directories;
std::vector<MStringA> g_definitions;
std::vector<MStringA> g_undefinitions;

std::string g_strFile = "(anonymous)";
int g_nLineNo = 0;

LANGID g_langid = 0;
uint16_t g_wCodePage = CP_UTF8;
int g_value = 0;

typedef std::map<LANGID, MessageRes> msg_tables_type;
msg_tables_type g_msg_tables;

int syntax_error(void)
{
    fprintf(stderr, "%s (%d): ERROR: Syntax error\n", g_strFile.c_str(), g_nLineNo);
    return EXITCODE_SYNTAX_ERROR;
}

const char *g_progname = "mcdx";

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
                uint16_t wCodePage = 0;
                if (std::isdigit(*ptr))
                {
                    wCodePage = uint16_t(strtol(ptr, NULL, 0));
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

int eat_output(const std::string& output)
{
    g_msg_tables.clear();

    std::vector<std::string> lines;
    mstr_split(lines, output, "\n");

    for (size_t i = 0; i < lines.size(); ++i)
    {
        mstr_trim(lines[i]);
    }

    // parse lines
    int nMode = 0;
    uint8_t bPrimLang = 0, bSubLang = 0;
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
                bPrimLang = (uint8_t)strtoul(ptr0, NULL, 0);
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
                bSubLang = (uint8_t)strtoul(ptr, NULL, 0);
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
            if (memcmp("MESSAGETABLEDX", ptr, 14) == 0 &&
                (std::isspace(ptr[14]) || ptr[14] == 0 || ptr[14] == '{'))  // }
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

int save_rc(const char *output_file)
{
    FILE *fp;
    if (output_file)
    {
        fp = fopen(output_file, "wb");
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

    fprintf(fp, "#pragma code_page(65001) /* UTF-8 */\r\n\r\n");

    msg_tables_type::iterator it, end = g_msg_tables.end();
    for (it = g_msg_tables.begin(); it != end; ++it)
    {
        fprintf(fp, "#ifdef MCDX_INVOKED\r\n");
        fprintf(fp, "LANGUAGE 0x%02X, 0x%02X\r\n",
                PRIMARYLANGID(it->first), SUBLANGID(it->first));

        MStringW wstr = it->second.Dump();
        MStringA str = MWideToAnsi(CP_UTF8, wstr.c_str()).c_str();

        fputs(str.c_str(), fp);
        fprintf(fp, "#endif\r\n\r\n");
    }

    if (output_file)
        fclose(fp);

    if (ferror(fp))
    {
        if (output_file)
            _unlink(output_file);
        fprintf(stderr, "ERROR: Unable to write output file.\n");
        return EXITCODE_CANNOT_OPEN;
    }

    return EXITCODE_SUCCESS;
}

int save_res(const char *output_file)
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
        fp = fopen(output_file, "wb");
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
            _unlink(output_file);
        fprintf(stderr, "ERROR: Unable to write output file.\n");
        return EXITCODE_CANNOT_OPEN;
    }

    return EXITCODE_SUCCESS;
}

int save_coff(const char *output_file)
{
    char temp_file[MAX_PATH];

    if (FILE *fp = tmpfilenam(temp_file))
    {
        fclose(fp);
    }
    else
    {
        fprintf(stderr, "ERROR: Unable to create temporary file.\n");
        return EXITCODE_CANT_MAKE_TEMP;
    }
    
    if (int ret = save_res(temp_file))
    {
        return ret;
    }

    MStringA command_line;
    command_line += g_windres;
    command_line += " \"";
    command_line += temp_file;
    if (output_file)
    {
        command_line += "\" \"";
        command_line += output_file;
        command_line += "\"";
    }
    else
    {
        command_line += "\" -O coff";
    }

    // create a process
#if defined(_WIN32) && !defined(WONVER)
    MProcessMaker maker;
    maker.SetShowWindow(SW_HIDE);
    maker.SetCreationFlags(CREATE_NEW_CONSOLE);
    MFile hInputWrite, hOutputRead;
    SetEnvironmentVariableA("LANG", "en_US");
    if (maker.PrepareForRedirect(&hInputWrite, &hOutputRead) &&
        maker.CreateProcessDx(NULL, command_line.c_str()))
    {
        std::string output;
        maker.ReadAll(output, hOutputRead);

        if (maker.GetExitCode() == 0)
        {
            return EXITCODE_SUCCESS;
        }
        else if (output.find(": no resources") != std::string::npos)
        {
            output.clear();
            return EXITCODE_SUCCESS;
        }

        fputs(output.c_str(), stderr);
    }
#else
    putenv("LANG=en_US");
    if (FILE *fp = popen(command_line.c_str(), "r"))
    {
        std::string output;

        char buf[256];
        for (;;)
        {
            size_t count = fread(buf, 1, _countof(buf), fp);
            if (!count)
                break;
            output.append(buf, count);
        }
        if (pclose(fp) == 0)
        {
            return EXITCODE_SUCCESS;
        }
        else if (output.find(": no resources") != std::string::npos)
        {
            output.clear();
            return EXITCODE_SUCCESS;
        }

        fputs(output.c_str(), stderr);
    }
#endif

    fprintf(stderr, "ERROR: Failed to create process\n");
    return EXITCODE_FAIL_TO_PREPROCESS;
}

int save_bin(const char *output_file)
{
    MessageRes msg_res = g_msg_tables.begin()->second;

    MByteStreamEx stream;
    msg_res.SaveToStream(stream);

    FILE *fp;
    if (output_file)
    {
        fp = fopen(output_file, "wb");
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
            _unlink(output_file);
        fprintf(stderr, "ERROR: Unable to write output file.\n");
        return EXITCODE_CANNOT_OPEN;
    }

    return EXITCODE_SUCCESS;
}

int load_rc(const char *input_file)
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
    MString command_line;
    command_line += g_cpp;
    command_line += ' ';
    for (size_t i = 0; i < g_definitions.size(); ++i)
    {
        command_line += " -D";
        command_line += g_definitions[i];
    }
    command_line += " \"";
    command_line += input_file;
    command_line += "\"";

    g_strFile = input_file;
    g_nLineNo = 1;

    // create a process
#if defined(_WIN32) && !defined(WONVER)
    MProcessMaker maker;
    maker.SetShowWindow(SW_HIDE);
    maker.SetCreationFlags(CREATE_NEW_CONSOLE);
    MFile hInputWrite, hOutputRead;
    if (maker.PrepareForRedirect(&hInputWrite, &hOutputRead) &&
        maker.CreateProcessDx(NULL, command_line.c_str()))
    {
        std::string output;
        maker.ReadAll(output, hOutputRead);

        if (maker.GetExitCode() == 0)
        {
            // eat the output
            if (int ret = eat_output(output))
                return ret;

            return EXITCODE_SUCCESS;
        }

        fputs(output.c_str(), stderr);
    }
#else
    putenv("LANG=en_US");
    if (FILE *fp = popen(command_line.c_str(), "r"))
    {
        std::string output;

        char buf[256];
        for (;;)
        {
            size_t count = fread(buf, 1, _countof(buf), fp);
            if (!count)
                break;
            output.append(buf, count);
        }
        if (pclose(fp) == 0)
        {
            // eat the output
            if (int ret = eat_output(output))
                return ret;

            return EXITCODE_SUCCESS;
        }

        fputs(output.c_str(), stderr);
    }
#endif

    fprintf(stderr, "ERROR: Failed to preprocess\n");
    return EXITCODE_FAIL_TO_PREPROCESS;
}

int load_bin(const char *input_file)
{
    FILE *fp = fopen(input_file, "rb");
    if (!fp)
    {
        fprintf(stderr, "ERROR: Unable to open input file.\n");
        return EXITCODE_CANNOT_OPEN;
    }

    std::string contents;
    char buf[256];
    for (;;)
    {
        size_t len = fread(buf, 1, 256, fp);
        if (!len)
            break;
        contents.append(buf, len);
    }
    fclose(fp);

    MByteStreamEx stream(&contents[0], contents.size());
    if (!g_msg_tables[g_langid].LoadFromStream(stream, 1))
    {
        fprintf(stderr, "ERROR: Invalid data.\n");
        return EXITCODE_INVALID_DATA;
    }

    return EXITCODE_SUCCESS;
}

int load_res(const char *input_file)
{
    FILE *fp = fopen(input_file, "rb");
    if (!fp)
    {
        fprintf(stderr, "ERROR: Unable to open input file.\n");
        return EXITCODE_CANNOT_OPEN;
    }

    std::string contents;
    char buf[256];
    for (;;)
    {
        size_t len = fread(buf, 1, 256, fp);
        if (!len)
            break;
        contents.append(buf, len);
    }
    fclose(fp);

    MByteStreamEx stream(&contents[0], contents.size());
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
        if (!g_msg_tables[header.LanguageId].LoadFromStream(bs, 1))
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
    if (strcmp(g_inp_format, "rc") == 0)
    {
        if (int ret = load_rc(g_input_file))
            return ret;
    }
    else if (strcmp(g_inp_format, "res") == 0)
    {
        if (int ret = load_res(g_input_file))
            return ret;
    }
    else if (strcmp(g_inp_format, "bin") == 0)
    {
        if (int ret = load_bin(g_input_file))
            return ret;
    }
    else if (strcmp(g_inp_format, "coff") == 0)
    {
        fprintf(stderr, "ERROR: COFF input format is not supported yet.\n");
        return EXITCODE_NOT_SUPPORTED_YET;
    }
    else
    {
        fprintf(stderr, "ERROR: invalid input format\n");
        return EXITCODE_INVALID_ARGUMENT;
    }

    if (strcmp(g_out_format, "rc") == 0)
    {
        return save_rc(g_output_file);
    }
    else if (strcmp(g_out_format, "res") == 0)
    {
        return save_res(g_output_file);
    }
    else if (strcmp(g_out_format, "bin") == 0)
    {
        return save_bin(g_output_file);
    }
    else if (strcmp(g_out_format, "coff") == 0)
    {
        return save_coff(g_output_file);
    }
    else
    {
        fprintf(stderr, "ERROR: invalid output format\n");
        return EXITCODE_INVALID_ARGUMENT;
    }
}

const char *get_format(const char *file_path)
{
    const char *pch = mstrrchr(file_path, '.');
    if (pch == NULL)
    {
        return "rc";
    }
    else if (strcmp(pch, ".rc") == 0)
    {
        return "rc";
    }
    else if (strcmp(pch, ".res") == 0)
    {
        return "res";
    }
    else if (strcmp(pch, ".bin") == 0)
    {
        return "bin";
    }
    else if (strcmp(pch, ".o") == 0 ||
             strcmp(pch, ".obj") == 0 ||
             strcmp(pch, ".coff") == 0)
    {
        return "coff";
    }
    else
    {
        return "rc";
    }
}

//////////////////////////////////////////////////////////////////////////////

int main(int argc, char **argv)
{
    static const struct option long_options[] =
    {
        {"help",            required_argument, NULL, 'h' },
        {"version",         no_argument,       NULL, 'V' },
        {"input",           required_argument, NULL, 'i' },
        {"output",          required_argument, NULL, 'o' },
        {"input-format",    required_argument, NULL, 'J' },
        {"output-format",   required_argument, NULL, 'O' },
        {"include-dir",     required_argument, NULL, 'I' },
        {"define",          required_argument, NULL, 'D' },
        {"undefine",        required_argument, NULL, 'U' },
        {"codepage",        required_argument, NULL, 'c' },
        {"language",        required_argument, NULL, 'l' },
        {"preprocessor",    required_argument, NULL, 'p' },
        {"windres",         required_argument, NULL, 'w' },
        {0,                 0,                 NULL, 0   }
    };

#ifdef __CYGWIN__
    extern char __declspec(dllimport) *__progname;
    g_progname = __progname;
#else
    g_progname = argv[0];
#endif

    g_definitions.push_back("RC_INVOKED");
    g_definitions.push_back("MCDX_INVOKED");

    // parse command line
    while (1)
    {
        int this_option_optind = optind ? optind : 1;
        int option_index = 0;
        int c = getopt_long(argc, argv, "hVi:o:J:O:I:D:U:c:l:",
                            long_options, &option_index);
        if (c == -1)
            break;

        switch (c) {
        case 'h':
            show_help();
            return EXITCODE_SUCCESS;
        case 'V':
            show_version();
            return EXITCODE_SUCCESS;
        case 'i':
            if (g_input_file)
            {
                fprintf(stderr, "ERROR: Too many input files\n");
                return EXITCODE_INVALID_ARGUMENT;
            }
            else
            {
                g_input_file = optarg;
            }
            break;
        case 'o':
            if (g_output_file)
            {
                fprintf(stderr, "ERROR: Too many output files\n");
                return EXITCODE_INVALID_ARGUMENT;
            }
            else
            {
                g_output_file = optarg;
            }
            break;
        case 'J':
            g_inp_format = optarg;
            break;
        case 'O':
            g_out_format = optarg;
            break;
        case 'I':
            g_include_directories.push_back(optarg);
            break;
        case 'D':
            g_definitions.push_back(optarg);
            break;
        case 'U':
            g_undefinitions.push_back(optarg);
            break;
        case 'c':
            g_wCodePage = (uint16_t)strtol(optarg, NULL, 0);
            break;
        case 'l':
            {
                uint16_t w = (uint16_t)strtol(optarg, NULL, 0);
                uint8_t bPrim = LOBYTE(w);
                uint8_t bSub = HIBYTE(w);
                g_langid = MAKELANGID(bPrim, bSub);
            }
            break;
        case 'p':
            g_cpp = optarg;
            break;
        case 'w':
            g_windres = optarg;
            break;
        default:
            assert(0);
            break;
        }
    }

    if (optind < argc)
    {
        while (optind < argc)
        {
            if (g_input_file == NULL)
            {
                g_input_file = argv[optind++];
            }
            else if (g_output_file == NULL)
            {
                g_output_file = argv[optind++];
            }
            else
            {
                fprintf(stderr, "ERROR: Too many arguments\n");
                return EXITCODE_INVALID_ARGUMENT;
            }
        }
    }

    if (g_input_file == NULL)
    {
        static TCHAR s_szTempFile[MAX_PATH] = TEXT("");
        FILE *fp = tmpfilenam(s_szTempFile);
        if (fp == NULL)
        {
            fprintf(stderr, "ERROR: Unable to create temporary file\n");
            return EXITCODE_CANT_MAKE_TEMP;
        }

        char buf[512];
        while (fgets(buf, _countof(buf), stdin) != NULL)
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
            g_out_format = "rc";
        }
    }

    int ret = just_do_it();
    return ret;
}

//////////////////////////////////////////////////////////////////////////////
