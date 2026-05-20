// mcdx.cpp --- Message Compiler by katahiromz
//////////////////////////////////////////////////////////////////////////////
// RisohEditor --- Another free Win32 resource editor
// Copyright (C) 2022-2026 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
// License: GPL-3 or later

#define NO_CONSTANTS_DB
#define NO_STRSAFE

#if defined(_WIN32) && !defined(WONVER)
    #include "MProcessMaker.hpp"
#endif
#include "MString.hpp"
#include "MIdOrString.hpp"
#include "MacroParser.hpp"
#include "MessageRes.hpp"
#include "ResHeader.hpp"
#include "getoptwin.h"

#include <cctype>
#include <sys/types.h>
#include <sys/stat.h>

#ifndef RT_MESSAGETABLE
    #define RT_MESSAGETABLE MAKEINTRESOURCE(11)
#endif

#ifndef _countof
    #define _countof(array) (sizeof(array) / sizeof(array[0]))
#endif

bool g_wrap_enabled = false;

//////////////////////////////////////////////////////////////////////////////

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

//////////////////////////////////////////////////////////////////////////////
// Globals

const char *g_cpp      = "mcpp";
const char *g_windres  = "windres";
const char *g_progname = "mcdx";

char *g_input_file  = NULL;
char *g_output_file = NULL;
const char *g_inp_format = NULL;
const char *g_out_format = NULL;

std::vector<MStringA> g_include_directories;
std::vector<MStringA> g_definitions;
std::vector<MStringA> g_undefinitions;

std::string g_strFile = "(anonymous)";
int g_nLineNo = 0;

LANGID   g_langid    = 0;
uint16_t g_wCodePage = CP_UTF8;
int      g_value     = 0;
BOOL     g_in_msg_table = FALSE;
MIdOrString g_table_id((WORD)1);  // Current MESSAGETABLEDX table-id (default: 1)

typedef std::pair<LANGID, MIdOrString>           msg_table_key_type;
typedef std::map<msg_table_key_type, MessageRes> msg_tables_type;
msg_tables_type g_msg_tables;

char g_lang_english[] = "LANG=en_US";

//////////////////////////////////////////////////////////////////////////////
// Help / version

void show_help(void)
{
    fputs(
        "mcdx --- Message Compiler Dirty Extension by katahiromz\n"
        "Copyright (C) 2018-2026 Katayama Hirofumi MZ. License: GPLv3.\n"
        "\n"
        "Usage: mcdx [option(s)] [input-file] [output-file]\n"
        "Options:\n"
        "  -i --input=<file>            Name input file\n"
        "  -o --output=<file>           Name output file\n"
        "  -J --input-format=<format>   Specify input format\n"
        "  -O --output-format=<format>  Specify output format\n"
        "  -I --include-dir=<dir>       Include directory when preprocessing rc file\n"
        "  -D --define=<sym>[=<val>]    Define SYM when preprocessing rc file\n"
        "  -U --undefine <sym>          Undefine SYM when preprocessing rc file\n"
        "  -c --codepage=<codepage>     Specify default codepage\n"
        "  -l --language=<val>          Set language when reading rc file\n"
        "  --preprocessor=<path>        Set preprocessor path\n"
        "  --windres=<path>             Set windres path\n"
        "FORMAT is one of rc, res, bin or coff, and is deduced from the file name\n"
        "Report bugs to <katayama.hirofumi.mz@gmail.com>\n",
        stdout);
}

void show_version(void)
{
    fputs(
        "mcdx ver.0.9.0\n"
        "Copyright (C) 2018-2026 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>.\n"
        "This program is free software; you may redistribute it under the terms of\n"
        "the GNU General Public License version 3 or (at your option) any later version.\n"
        "This program has absolutely no warranty.\n",
        stdout);
}

//////////////////////////////////////////////////////////////////////////////
// Temporary file management

struct DeleteTempFiles
{
    std::vector<std::string> m_files;

    void push_back(const std::string& file) { m_files.push_back(file); }

    ~DeleteTempFiles()
    {
        for (auto& f : m_files)
            _unlink(f.c_str());
    }
};
DeleteTempFiles g_delete_temp_files;

inline bool file_exists(const char *file)
{
    struct stat st;
    return stat(file, &st) == 0;
}

FILE *tmpfilenam(char *pathname)
{
    char tmp[MAX_PATH];
    const char *ptr = getenv("TMP");
    if (!ptr) ptr = getenv("TEMP");
    if (ptr)
    {
        strcpy(tmp, ptr);
        mstr_trim_right(tmp, "/\\");
    }
    else
    {
        strcpy(tmp, ".");
    }

    FILE *fp = NULL;
    char file[MAX_PATH];
    const int retry_count = 64;

    for (int k = 0; k < retry_count && !fp; ++k)
    {
        char name[13];
        for (int i = 0; i < 8; ++i)
            name[i] = (char)('A' + rand() % ('Z' - 'A' + 1));
        strcpy(name + 8, ".tmp");

        strcpy(file, tmp);
#ifdef _WIN32
        strcat(file, "\\");
#else
        strcat(file, "/");
#endif
        strcat(file, name);

        if (!file_exists(file))
            fp = fopen(file, "w+b");
    }

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

//////////////////////////////////////////////////////////////////////////////
// Error reporting

int syntax_error(void)
{
    fprintf(stderr, "%s (%d): ERROR: Syntax error\n", g_strFile.c_str(), g_nLineNo);
    assert(0);
    return EXITCODE_SYNTAX_ERROR;
}

//////////////////////////////////////////////////////////////////////////////
// File I/O helpers

// Read entire contents of a binary file into a string.
// Returns true on success.
static bool read_file_contents(const char *path, std::string& out)
{
    FILE *fp = fopen(path, "rb");
    if (!fp)
        return false;

    char buf[256];
    for (;;)
    {
        size_t len = fread(buf, 1, sizeof(buf), fp);
        if (!len) break;
        out.append(buf, len);
    }
    fclose(fp);
    return true;
}

// Open an output file (or return stdout when path is NULL).
// Writes error message and returns NULL on failure.
static FILE *open_output_file(const char *path)
{
    if (!path)
        return stdout;
    FILE *fp = fopen(path, "wb");
    if (!fp)
        fprintf(stderr, "ERROR: Unable to open output file.\n");
    return fp;
}

// Finish writing: close file, check ferror, unlink on error.
static int finish_output_file(FILE *fp, const char *path)
{
    bool err = ferror(fp) != 0;
    if (path)
        fclose(fp);
    if (err)
    {
        if (path)
            _unlink(path);
        fprintf(stderr, "ERROR: Unable to write output file.\n");
        return EXITCODE_CANNOT_OPEN;
    }
    return EXITCODE_SUCCESS;
}

//////////////////////////////////////////////////////////////////////////////
// Process helper (abstracts Win32 vs POSIX)

// Run `command_line`, capture stdout+stderr into `output`.
// Returns true when the process exits with code 0.
static bool run_process(const std::string& command_line, std::string& output)
{
#if defined(_WIN32) && !defined(WONVER)
    MProcessMaker maker;
    maker.SetShowWindow(SW_HIDE);
    maker.SetCreationFlags(CREATE_NEW_CONSOLE);
    MFile hInputWrite, hOutputRead;
    if (!maker.PrepareForRedirect(&hInputWrite, &hOutputRead) ||
        !maker.CreateProcessDx(NULL, command_line.c_str()))
    {
        return false;
    }
    maker.ReadAll(output, hOutputRead);
    return maker.GetExitCode() == 0;
#else
    putenv(g_lang_english);
    FILE *fp = popen(command_line.c_str(), "r");
    if (!fp)
        return false;

    char buf[256];
    for (;;)
    {
        size_t count = fread(buf, 1, sizeof(buf), fp);
        if (!count) break;
        output.append(buf, count);
    }
    return pclose(fp) == 0;
#endif
}

//////////////////////////////////////////////////////////////////////////////
// Parsing helpers

bool do_directive_line(char*& ptr)
{
    char *ptr1 = ptr;
    while (mchr_is_digit(*ptr))
        ++ptr;
    char *ptr2 = ptr;
    ptr = mstr_skip_space(ptr);
    char *ptr3 = ptr;
    while (*ptr)
        ++ptr;
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
    else if (memcmp(ptr, "BEGIN", 5) == 0 &&
             (ptr[5] == 0 || mchr_is_space(ptr[5])))
    {
        nMode = 2;
        ptr += 5;
    }
    ptr = mstr_skip_space(ptr);
    if (nMode != 2 && *ptr && !mchr_is_digit(*ptr))
        return syntax_error();

    return EXITCODE_SUCCESS;
}

int do_mode_2(char*& ptr, int& nMode, bool& do_retry)
{
    ptr = mstr_skip_space(ptr);
    if (*ptr == '{')
    {
        return syntax_error();
    }
    if (*ptr == '}')
    {
        ++ptr;
        nMode = 0;
        g_in_msg_table = FALSE;
        do_retry = true;
        return EXITCODE_SUCCESS;
    }
    if (memcmp(ptr, "END", 3) == 0 && (ptr[3] == 0 || mchr_is_space(ptr[3])))
    {
        ptr += 3;
        nMode = 0;
        g_in_msg_table = FALSE;
        do_retry = true;
        return EXITCODE_SUCCESS;
    }

    if (!*ptr)
        return EXITCODE_SUCCESS;

    // parse integer expression up to ',' or '"'
    char *ptr0 = ptr;
    while (*ptr && *ptr != ',' && *ptr != '"')
        ++ptr;
    MStringA str(ptr0, ptr);

    using namespace MacroParser;
    StringScanner scanner(str);
    TokenStream ts(scanner);
    ts.read_tokens();
    Parser parser(ts);
    if (!parser.parse() || !eval_int(parser.ast(), g_value))
        return syntax_error();

    if (*ptr == ',' || *ptr == '"')
    {
        if (*ptr == ',') ++ptr;
        nMode = 3;
        do_retry = true;
    }
    else if (*ptr == 0)
    {
        nMode = 3;
    }
    else
    {
        return syntax_error();
    }
    return EXITCODE_SUCCESS;
}

int do_mode_3(char*& ptr, int& nMode, bool& do_retry)
{
    if (!g_in_msg_table)
        return EXITCODE_SUCCESS;
    ptr = mstr_skip_space(ptr);
    if (*ptr == ',') ++ptr;
    ptr = mstr_skip_space(ptr);

    if (*ptr == '"')
    {
        MStringA str = ptr;
        mstr_unquote(str);
        MStringW wstr(MAnsiToWide(g_wCodePage, str.c_str()).c_str());
        g_msg_tables[{g_langid, g_table_id}].m_map[(DWORD)g_value] = wstr;

        const char *ptr0 = ptr;
        guts_quote(str, ptr0);
        ptr = const_cast<char *>(ptr0);

        nMode = 2;
        do_retry = true;
        return EXITCODE_SUCCESS;
    }

    if (*ptr != 0)
        return syntax_error();

    return EXITCODE_SUCCESS;
}

int do_directive(char*& ptr)
{
    ++ptr;
    ptr = mstr_skip_space(ptr);

    if (mchr_is_digit(*ptr))
    {
        do_directive_line(ptr);
        return EXITCODE_SUCCESS;
    }

    if (memcmp(ptr, "pragma", 6) != 0)
        return EXITCODE_SUCCESS;

    ptr += 6;
    ptr = mstr_skip_space(ptr);
    char *pragma_start = ptr;

    if (memcmp(ptr, "pack", 4) == 0)
    {
        // #pragma pack - ignored
    }
    else if (memcmp(ptr, "code_page", 9) == 0)
    {
        ptr += 9;
        ptr = mstr_skip_space(ptr);
        if (*ptr == '(')
        {
            ++ptr;
            ptr = mstr_skip_space(ptr);
            uint16_t wCodePage = 0;
            if (mchr_is_digit(*ptr))
                wCodePage = uint16_t(strtol(ptr, NULL, 0));
            while (mchr_is_alnum(*ptr))
                ++ptr;
            ptr = mstr_skip_space(ptr);
            if (*ptr == ')')
            {
                ++ptr;
                g_wCodePage = wCodePage;
            }
            else
            {
                fprintf(stderr, "%s (%d): WARNING: Invalid pragma: %s\n",
                        g_strFile.c_str(), g_nLineNo, pragma_start);
            }
        }
        else
        {
            fprintf(stderr, "%s (%d): WARNING: Invalid pragma: %s\n",
                    g_strFile.c_str(), g_nLineNo, pragma_start);
        }
    }
    else
    {
        fprintf(stderr, "%s (%d): WARNING: Unknown pragma: %s\n",
                g_strFile.c_str(), g_nLineNo, pragma_start);
    }

    return EXITCODE_SUCCESS;
}

//////////////////////////////////////////////////////////////////////////////
// Main parser

int eat_output(const std::string& output)
{
    g_msg_tables.clear();

    std::vector<std::string> lines;
    mstr_split(lines, output, "\n");
    for (auto& l : lines)
        mstr_trim(l);

    int nMode = 0;
    uint8_t bPrimLang = 0, bSubLang = 0;

    for (size_t i = 0; i < lines.size(); ++i, ++g_nLineNo)
    {
        std::string& line = lines[i];
        if (line.empty())
            continue;

        char *ptr = &line[0];

        if (*ptr == '#')
        {
            if (int ret = do_directive(ptr))
                return ret;
            continue;
        }

        if (memcmp("LANGUAGE", ptr, 8) == 0 &&
            (ptr[8] == 0 || mchr_is_space(ptr[8])))
        {
            ptr += 8;
            nMode = -1;
        }

        // Inner retry loop replaces goto
        bool do_retry;
        do
        {
            do_retry = false;

            if (nMode == -1 && *ptr)
            {
                ptr = mstr_skip_space(ptr);
                if (mchr_is_digit(*ptr))
                    nMode = -2;
            }
            if (nMode == -2 && *ptr)
            {
                ptr = mstr_skip_space(ptr);
                char *ptr0 = ptr;
                while (mchr_is_alnum(*ptr))
                    ++ptr;
                if (mchr_is_digit(*ptr0))
                {
                    bPrimLang = (uint8_t)strtoul(ptr0, NULL, 0);
                    nMode = -3;
                }
                else if (*ptr)
                {
                    return syntax_error();
                }
            }
            if (nMode == -3 && *ptr)
            {
                ptr = mstr_skip_space(ptr);
                if (*ptr == ',') { ++ptr; nMode = -4; }
            }
            if (nMode == -4 && *ptr)
            {
                ptr = mstr_skip_space(ptr);
                if (mchr_is_digit(*ptr))
                {
                    bSubLang = (uint8_t)strtoul(ptr, NULL, 0);
                    g_langid = MAKELANGID(bPrimLang, bSubLang);
                    nMode = 0;
                    break;
                }
                else if (*ptr)
                {
                    return syntax_error();
                }
            }
            if (nMode == 0 && *ptr)
            {
                ptr = mstr_skip_space(ptr);
                g_table_id = MIdOrString((WORD)1); // reset to default for each new block

                // Check whether line starts directly with MESSAGETABLEDX or
                // has an optional table-id (integer / string literal / macro) before it.
                bool found = (memcmp("MESSAGETABLEDX", ptr, 14) == 0 &&
                              (mchr_is_space(ptr[14]) || ptr[14] == 0 ||
                               ptr[14] == '{'));
                if (!found)
                {
                    char *ptr_save = ptr;

                    // Case 1: quoted string literal  "MyTable" MESSAGETABLEDX
                    if (*ptr == '"')
                    {
                        std::string token;
                        const char *p2 = ptr;
                        if (guts_quote(token, p2))
                        {
                            p2 = mstr_skip_space(p2);
                            if (memcmp("MESSAGETABLEDX", p2, 14) == 0 &&
                                (mchr_is_space(p2[14]) || p2[14] == 0 || p2[14] == '{'))
                            {
                                g_table_id = MIdOrString(token.c_str());
                                ptr  = const_cast<char *>(p2);
                                found = true;
                            }
                        }
                    }

                    // Case 2: integer literal / macro expression  2 MESSAGETABLEDX
                    if (!found)
                    {
                        char *ptr0 = ptr;
                        while (*ptr && !mchr_is_space(*ptr))
                            ++ptr;
                        if (ptr != ptr0)
                        {
                            MStringA token(ptr0, ptr);
                            char *p2 = mstr_skip_space(ptr);
                            if (memcmp("MESSAGETABLEDX", p2, 14) == 0 &&
                                (mchr_is_space(p2[14]) || p2[14] == 0 || p2[14] == '{'))
                            {
                                using namespace MacroParser;
                                StringScanner scanner(token);
                                TokenStream ts(scanner);
                                ts.read_tokens();
                                Parser parser(ts);
                                int val = 1;
                                if (parser.parse() && eval_int(parser.ast(), val))
                                {
                                    g_table_id = MIdOrString((WORD)(uint16_t)val);
                                    ptr   = p2;
                                    found = true;
                                }
                            }
                        }
                    }

                    if (!found)
                        ptr = ptr_save;
                }

                if (found)
                {
                    nMode = 1;
                    g_in_msg_table = TRUE;
                    ptr += 14;
                    ptr = mstr_skip_space(ptr);
                }
            }
            if (nMode == 1 && *ptr)
            {
                if (int ret = do_mode_1(ptr, nMode, do_retry))
                    return ret;
                continue; // do_retry is checked at top of do{}while
            }
            if (nMode == 2 && *ptr)
            {
                if (int ret = do_mode_2(ptr, nMode, do_retry))
                    return ret;
            }
            if (nMode == 3 && *ptr)
            {
                if (int ret = do_mode_3(ptr, nMode, do_retry))
                    return ret;
            }
        } while (do_retry);
    }

    if (nMode != 0)
        return syntax_error();

    return EXITCODE_SUCCESS;
}

//////////////////////////////////////////////////////////////////////////////
// Save functions

int save_rc(const char *output_file)
{
    FILE *fp = open_output_file(output_file);
    if (!fp)
        return EXITCODE_CANNOT_OPEN;

    fprintf(fp, "#pragma code_page(65001) /* UTF-8 */\r\n\r\n");

    for (auto& kv : g_msg_tables)
    {
        LANGID            langid   = kv.first.first;
        const MIdOrString table_id = kv.first.second;

        fprintf(fp, "#ifdef MCDX_INVOKED\r\n");
        fprintf(fp, "LANGUAGE 0x%02X, 0x%02X\r\n",
                PRIMARYLANGID(langid), SUBLANGID(langid));

        MStringW wstr = kv.second.Dump(table_id);
        MStringA str  = MWideToAnsi(CP_UTF8, wstr.c_str()).c_str();
        fputs(str.c_str(), fp);
        fprintf(fp, "#endif\r\n\r\n");
    }

    return finish_output_file(fp, output_file);
}

int save_res(const char *output_file)
{
    MByteStreamEx bs;
    ResHeader header;
    if (!header.WriteTo(bs))
        return EXITCODE_INVALID_DATA;

    for (auto& kv : g_msg_tables)
    {
        LANGID            langid   = kv.first.first;
        const MIdOrString table_id = kv.first.second;

        MByteStreamEx stream;
        kv.second.SaveToStream(stream);

        header.DataSize   = DWORD(stream.size());
        header.HeaderSize = header.GetHeaderSize(RT_MESSAGETABLE, table_id);
        if (header.HeaderSize == 0 || header.HeaderSize >= 0x10000)
            return FALSE;

        header.type           = RT_MESSAGETABLE;
        header.name           = table_id;
        header.DataVersion    = 0;
        header.MemoryFlags    = MEMORYFLAG_DISCARDABLE | MEMORYFLAG_PURE |
                                MEMORYFLAG_MOVEABLE;
        header.LanguageId     = langid;
        header.Version        = 0;
        header.Characteristics = 0;

        if (!header.WriteTo(bs))
            return FALSE;
        if (!bs.WriteData(&stream[0], stream.size()))
            return FALSE;

        bs.WriteDwordAlignment();
    }

    FILE *fp = open_output_file(output_file);
    if (!fp)
        return EXITCODE_CANNOT_OPEN;

    fwrite(&bs[0], bs.size(), 1, fp);
    return finish_output_file(fp, output_file);
}

int save_coff(const char *output_file)
{
    char temp_file[MAX_PATH];
    if (FILE *fp = tmpfilenam(temp_file))
        fclose(fp);
    else
    {
        fprintf(stderr, "ERROR: Unable to create temporary file.\n");
        return EXITCODE_CANT_MAKE_TEMP;
    }

    if (int ret = save_res(temp_file))
        return ret;

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

#if defined(_WIN32) && !defined(WONVER)
    SetEnvironmentVariableA("LANG", "en_US");
#endif

    std::string output;
    if (run_process(command_line, output))
        return EXITCODE_SUCCESS;

    if (output.find(": no resources") != std::string::npos)
        return EXITCODE_SUCCESS;

    fputs(output.c_str(), stderr);
    fprintf(stderr, "ERROR: Failed to create process\n");
    return EXITCODE_FAIL_TO_PREPROCESS;
}

int save_bin(const char *output_file)
{
    MessageRes msg_res = g_msg_tables.begin()->second;

    MByteStreamEx stream;
    msg_res.SaveToStream(stream);

    FILE *fp = open_output_file(output_file);
    if (!fp)
        return EXITCODE_CANNOT_OPEN;

    fwrite(&stream[0], stream.size(), 1, fp);
    return finish_output_file(fp, output_file);
}

//////////////////////////////////////////////////////////////////////////////
// Load functions

bool IsUTF16File(const char *input_file)
{
    FILE *fp = fopen(input_file, "rb");
    if (!fp)
        return false;

    char ab[2];
    bool result = fread(ab, 1, 2, fp) == 2 &&
                  (memcmp(ab, "\xFF\xFE", 2) == 0 || (ab[0] && !ab[1]));
    fclose(fp);
    return result;
}

int load_rc(const char *input_file)
{
    // Apply undefinitions
    for (const auto& undef : g_undefinitions)
    {
        size_t ulen = undef.size();
        for (size_t k = 0; k < g_definitions.size(); ++k)
        {
            if (g_definitions[k].find(undef) == 0)
            {
                char c = g_definitions[k].c_str()[ulen];
                if (c == 0 || c == '=')
                {
                    g_definitions.erase(g_definitions.begin() + k);
                    --k;
                }
            }
        }
    }

    // Build preprocessor command line
    MString command_line = g_cpp;
    command_line += ' ';
    for (const auto& def : g_definitions)
    {
        command_line += " -D";
        command_line += def;
    }
    for (const auto& inc : g_include_directories)
    {
        command_line += " -I\"";
        command_line += inc;
        command_line += "\"";
    }
    command_line += " \"";
    command_line += input_file;
    command_line += "\"";

    g_strFile  = input_file;
    g_nLineNo  = 1;

    std::string output;
    if (run_process(command_line, output))
        return eat_output(output);

    fputs(output.c_str(), stderr);
    fprintf(stderr, "ERROR: Failed to preprocess\n");
    return EXITCODE_FAIL_TO_PREPROCESS;
}

int load_bin(const char *input_file)
{
    std::string contents;
    if (!read_file_contents(input_file, contents))
    {
        fprintf(stderr, "ERROR: Unable to open input file.\n");
        return EXITCODE_CANNOT_OPEN;
    }

    MByteStreamEx stream(&contents[0], contents.size());
    // bin files carry raw MESSAGETABLE data without a resource header, so the
    // table-id defaults to 1.
    if (!g_msg_tables[{g_langid, 1}].LoadFromStream(stream))
    {
        fprintf(stderr, "ERROR: Invalid data.\n");
        return EXITCODE_INVALID_DATA;
    }
    return EXITCODE_SUCCESS;
}

int load_res(const char *input_file)
{
    std::string contents;
    if (!read_file_contents(input_file, contents))
    {
        fprintf(stderr, "ERROR: Unable to open input file.\n");
        return EXITCODE_CANNOT_OPEN;
    }

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
            break;

        // header.name is an MIdOrString holding either a WORD id or a string name.
        // Use it as-is for the composite key so round-trip fidelity is preserved.
        const MIdOrString& table_id = header.name;
        if (!g_msg_tables[{header.LanguageId, table_id}].LoadFromStream(bs))
        {
            fprintf(stderr, "ERROR: Data is broken, invalid, or not supported.\n");
            return EXITCODE_INVALID_DATA;
        }
        stream.ReadDwordAlignment();
    }
    return EXITCODE_SUCCESS;
}

//////////////////////////////////////////////////////////////////////////////
// Format dispatch

const char *get_format(const char *file_path)
{
    const char *pch = mstrrchr(file_path, '.');
    if (!pch)             return "rc";
    if (strcmp(pch, ".rc")   == 0) return "rc";
    if (strcmp(pch, ".res")  == 0) return "res";
    if (strcmp(pch, ".bin")  == 0) return "bin";
    if (strcmp(pch, ".o")    == 0 ||
        strcmp(pch, ".obj")  == 0 ||
        strcmp(pch, ".coff") == 0) return "coff";
    return "rc";
}

int just_do_it(void)
{
    // Load
    if (strcmp(g_inp_format, "rc") == 0)
    {
        if (int r = load_rc(g_input_file))  
            return r;
    }
    else if (strcmp(g_inp_format, "res") == 0) 
    { 
        if (int r = load_res(g_input_file)) 
            return r; 
    }
    else if (strcmp(g_inp_format, "bin") == 0) 
    { 
        if (int r = load_bin(g_input_file)) 
            return r; 
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

    // Save
    if (strcmp(g_out_format, "rc") == 0) 
        return save_rc(g_output_file);
    else if (strcmp(g_out_format, "res") == 0)
        return save_res(g_output_file);
    else if (strcmp(g_out_format, "bin") == 0) 
        return save_bin(g_output_file);
    else if (strcmp(g_out_format, "coff") == 0) 
        return save_coff(g_output_file);
    else
    {
        fprintf(stderr, "ERROR: invalid output format\n");
        return EXITCODE_INVALID_ARGUMENT;
    }
}

//////////////////////////////////////////////////////////////////////////////
// Entry point

int main(int argc, char **argv)
{
#ifdef _WIN32
    srand(GetTickCount());
#endif
    static const struct option long_options[] =
    {
        {"help",          required_argument, NULL, 'h'},
        {"version",       no_argument,       NULL, 'V'},
        {"input",         required_argument, NULL, 'i'},
        {"output",        required_argument, NULL, 'o'},
        {"input-format",  required_argument, NULL, 'J'},
        {"output-format", required_argument, NULL, 'O'},
        {"include-dir",   required_argument, NULL, 'I'},
        {"define",        required_argument, NULL, 'D'},
        {"undefine",      required_argument, NULL, 'U'},
        {"codepage",      required_argument, NULL, 'c'},
        {"language",      required_argument, NULL, 'l'},
        {"preprocessor",  required_argument, NULL, 'p'},
        {"windres",       required_argument, NULL, 'w'},
        {0, 0, NULL, 0}
    };

#ifdef __CYGWIN__
    extern char __declspec(dllimport) *__progname;
    g_progname = __progname;
#else
    g_progname = argv[0];
#endif

    g_definitions.push_back("RC_INVOKED");
    g_definitions.push_back("MCDX_INVOKED");

    int c, option_index = 0;
    while ((c = getopt_long(argc, argv, "hVi:o:J:O:I:D:U:c:l:",
                            long_options, &option_index)) != -1)
    {
        switch (c)
        {
        case 'h': show_help();    return EXITCODE_SUCCESS;
        case 'V': show_version(); return EXITCODE_SUCCESS;
        case 'i':
            if (g_input_file)
                { fprintf(stderr, "ERROR: Too many input files\n"); return EXITCODE_INVALID_ARGUMENT; }
            g_input_file = optarg;
            break;
        case 'o':
            if (g_output_file)
                { fprintf(stderr, "ERROR: Too many output files\n"); return EXITCODE_INVALID_ARGUMENT; }
            g_output_file = optarg;
            break;
        case 'J': g_inp_format = optarg; break;
        case 'O': g_out_format = optarg; break;
        case 'I': g_include_directories.push_back(optarg); break;
        case 'D': g_definitions.push_back(optarg); break;
        case 'U': g_undefinitions.push_back(optarg); break;
        case 'c': g_wCodePage = (uint16_t)strtol(optarg, NULL, 0); break;
        case 'l':
            {
                uint16_t w    = (uint16_t)strtol(optarg, NULL, 0);
                uint8_t bPrim = LOBYTE(w);
                uint8_t bSub  = HIBYTE(w);
                g_langid = MAKELANGID(bPrim, bSub);
            }
            break;
        case 'p': g_cpp     = optarg; break;
        case 'w': g_windres = optarg; break;
        default:  assert(0); break;
        }
    }

    // Positional arguments
    while (optind < argc)
    {
        if (!g_input_file)
            g_input_file = argv[optind++];
        else if (!g_output_file)
            g_output_file = argv[optind++];
        else
        {
            fprintf(stderr, "ERROR: Too many arguments\n");
            return EXITCODE_INVALID_ARGUMENT;
        }
    }

    // Read stdin into temp file when no input file given
    if (!g_input_file)
    {
        static char s_szTempFile[MAX_PATH] = "";
        FILE *fp = tmpfilenam(s_szTempFile);
        if (!fp)
        {
            fprintf(stderr, "ERROR: Unable to create temporary file\n");
            return EXITCODE_CANT_MAKE_TEMP;
        }
        char buf[512];
        while (fgets(buf, _countof(buf), stdin))
            fputs(buf, fp);
        fclose(fp);
        g_input_file = s_szTempFile;
    }

    if (!g_inp_format)
        g_inp_format = get_format(g_input_file);

    if (!g_out_format)
        g_out_format = g_output_file ? get_format(g_output_file) : "rc";

    return just_do_it();
}

//////////////////////////////////////////////////////////////////////////////
