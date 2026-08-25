// mcdx.cpp --- Original message compiler by katahiromz
// Author: katahiromz
// License: GPL-3 or later
#define NO_CONSTANTS_DB
#define NO_STRSAFE

#ifdef WONVER
    #undef UNICODE
    #undef _UNICODE
#endif

#if defined(_WIN32) && !defined(WONVER)
    #include "MProcessMaker.hpp"
    #include <shellapi.h>   // CommandLineToArgvW
    #include <shlwapi.h>
    #include <io.h>         // _wunlink
    #include <wchar.h>      // _wfopen, _wgetenv
#endif
#include "MString.hpp"
#include "MIdOrString.hpp"
#include "MacroParser.hpp"
#include "MessageRes.hpp"
#include "ResHeader.hpp"
#include "MTextToText.hpp"

#include <cctype>
#include <algorithm>
#include <sys/types.h>
#include <sys/stat.h>
#include <vector>
#include <string>

#ifndef RT_MESSAGETABLE
    #define RT_MESSAGETABLE MAKEINTRESOURCE(11)
#endif

#ifndef _countof
    #define _countof(array) (sizeof(array) / sizeof(array[0]))
#endif

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

// Types
typedef std::pair<LANGID, MIdOrString>           msg_table_key_type;
typedef std::map<msg_table_key_type, MessageRes> msg_tables_type;

//////////////////////////////////////////////////////////////////////////////
// Globals

bool g_wrap_enabled = false;    // wrap enabled flag

// NOTE: These are MString (TCHAR-based): on Windows they are Unicode
//       (UTF-16) strings; on other platforms they are narrow (UTF-8) strings.
MString g_cpp      = TEXT("mcpp");
MString g_windres  = TEXT("windres");
MString g_progname = TEXT("mcdx");

MString g_input_file;    // empty == not specified
MString g_output_file;   // empty == not specified / write to stdout

// Format names ("rc"/"res"/"bin"/"coff") are always plain ASCII, so these
// stay as ordinary narrow strings regardless of platform.
std::string g_inp_format;
std::string g_out_format;

std::vector<MString> g_include_directories;
std::vector<MString> g_definitions;
std::vector<MString> g_undefinitions;

std::string g_strFile = "(anonymous)";
int g_nLineNo = 0;

LANGID   g_langid    = 0;
uint16_t g_wCodePage = CP_UTF8;
int      g_value     = 0;
BOOL     g_in_msg_table = FALSE;
MIdOrString g_table_id((WORD)1);  // Current MESSAGETABLEDX table-id (default: 1)

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
        "mcdx ver.0.9.3\n"
        "Copyright (C) 2018-2026 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>.\n"
        "This program is free software; you may redistribute it under the terms of\n"
        "the GNU General Public License version 3 or (at your option) any later version.\n"
        "This program has absolutely no warranty.\n",
        stdout);
}

//////////////////////////////////////////////////////////////////////////////
// Unicode-aware file helpers.
//
// On Windows these call the wide ("W") CRT/Win32 entry points directly so
// that file paths containing non-ASCII characters work correctly. On other
// platforms MString is already a narrow (UTF-8) std::string and the plain
// POSIX calls are used as-is.

static FILE *T_fopen(const MString& path, const MString& mode)
{
#if defined(_WIN32) && !defined(WONVER)
    return _wfopen(path.c_str(), mode.c_str());
#else
    return fopen(path.c_str(), mode.c_str());
#endif
}

static bool T_remove(const MString& path)
{
#if defined(_WIN32) && !defined(WONVER)
    return !!DeleteFileW(path.c_str());
#else
    return unlink(path.c_str()) == 0;
#endif
}

static bool T_file_exists(const MString& path)
{
#if defined(_WIN32) && !defined(WONVER)
    return !!PathFileExistsW(path.c_str());
#else
    struct stat st;
    return stat(path.c_str(), &st) == 0;
#endif
}

static MString T_getenv(const MString& name)
{
#if defined(_WIN32) && !defined(WONVER)
    WCHAR text[512];
    if (!GetEnvironmentVariableW(name.c_str(), text, _countof(text)))
        return L"";
    return text;
#else
    const char *p = getenv(name.c_str());
    return p ? MString(p) : MString();
#endif
}

// Convert an MString to a narrow (UTF-8) std::string, e.g. for use in
// fprintf(stderr, "%s", ...). On non-Unicode builds this is a no-op copy.
static std::string to_narrow(const MString& s)
{
#if defined(_WIN32) && !defined(WONVER)
    return MWideToAnsi(CP_UTF8, s.c_str()).c_str();
#else
    return s;
#endif
}

//////////////////////////////////////////////////////////////////////////////
// Temporary file management

struct AutoDeleteFile
{
    MString m_file;
    AutoDeleteFile(const MString& file = MString()) : m_file(file) { }
    ~AutoDeleteFile()
    {
        if (!m_file.empty())
            T_remove(m_file);
    }
};

static FILE *tmpfilenam(MString& pathname)
{
#if defined(_WIN32) && !defined(WONVER)
    TCHAR path[MAX_PATH];
    DWORD len = GetTempPath(_countof(path), path);
    if (len == 0 || len >= _countof(path))
    {
        pathname.clear();
        return NULL;
    }

    TCHAR file[MAX_PATH];
    if (GetTempFileName(path, TEXT("mcd"), 0, file) == 0)
    {
        pathname.clear();
        return NULL;
    }

    FILE *fp = T_fopen(file, TEXT("wb"));
    if (!fp)
    {
        ::DeleteFile(file);
        pathname.clear();
        return NULL;
    }

    pathname = file;
    return fp;
#else
    MString tmp = T_getenv(TEXT("TMP"));
    if (tmp.empty())
        tmp = T_getenv(TEXT("TEMP"));
    if (tmp.empty())
        tmp = TEXT(".");
    mstr_trim_right(tmp, TEXT("/\\"));

    MString file = tmp;
    file += TEXT("/");
    file += TEXT("mcdx.tmp");

    FILE *fp = T_fopen(file, TEXT("wb"));
    if (!fp)
    {
        pathname.clear();
        return NULL;
    }

    pathname = file;
    return fp;
#endif
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
static bool read_file_contents(const MString& path, std::string& out)
{
    FILE *fp = T_fopen(path, TEXT("rb"));
    if (!fp)
        return false;

    // Pre-size the buffer from the file size when available so the string
    // doesn't have to repeatedly grow/reallocate/copy while reading (this
    // matters for the potentially large .res/.bin inputs).
    out.clear();
    if (fseek(fp, 0, SEEK_END) == 0)
    {
        long size = ftell(fp);
        if (size > 0)
            out.reserve((size_t)size);
        fseek(fp, 0, SEEK_SET);
    }

    // Heap-allocated (not a stack array): keeps the frame small regardless
    // of the caller's stack depth or thread stack size.
    std::vector<char> buf(65536);
    for (;;)
    {
        size_t len = fread(buf.data(), 1, buf.size(), fp);
        if (!len) break;
        out.append(buf.data(), len);
    }
    fclose(fp);
    return true;
}

// Open an output file (or return stdout when path is empty).
// Writes error message and returns NULL on failure.
static FILE *open_output_file(const MString& path)
{
    if (path.empty())
        return stdout;
    FILE *fp = T_fopen(path, TEXT("wb"));
    if (!fp)
        fprintf(stderr, "ERROR: Unable to open output file.\n");
    return fp;
}

// Finish writing: close file, check ferror, unlink on error.
static int finish_output_file(FILE *fp, const MString& path)
{
    bool err = ferror(fp) != 0;
    if (!path.empty())
        fclose(fp);
    if (err)
    {
        if (!path.empty())
            T_remove(path);
        fprintf(stderr, "ERROR: Unable to write output file.\n");
        return EXITCODE_CANNOT_OPEN;
    }
    return EXITCODE_SUCCESS;
}

//////////////////////////////////////////////////////////////////////////////
// Process helper (abstracts Win32 vs POSIX)

#if defined(_WIN32) && !defined(WONVER)

// Quote one argument according to the Windows command-line parsing rules.
// In particular, backslashes immediately preceding a '"' or the closing
// '"' have to be doubled.
static void append_process_arg(MString& command_line, const MString& arg)
{
    command_line += TEXT("\"");

    for (size_t i = 0; i < arg.size(); ++i)
    {
        MString::value_type ch = arg[i];

        if (ch == TEXT('\\'))
        {
            size_t j = i;
            while (j < arg.size() && arg[j] == TEXT('\\'))
                ++j;

            size_t count = j - i;

            if (j == arg.size())
            {
                // Backslashes before the closing quote must be doubled.
                command_line.append(count * 2, TEXT('\\'));
            }
            else if (arg[j] == TEXT('"'))
            {
                // Backslashes before a quote must be doubled,
                // and the quote itself must be escaped.
                command_line.append(count * 2 + 1, TEXT('\\'));
            }
            else
            {
                command_line.append(count, TEXT('\\'));
            }

            i = j - 1;
        }
        else if (ch == TEXT('"'))
        {
            command_line += TEXT("\\\"");
        }
        else
        {
            command_line += ch;
        }
    }

    command_line += TEXT("\"");
}

#endif

// Run `command_line`, capture stdout+stderr into `output`.
// Returns true when the process exits with code 0.
static bool run_process(
    const MString& application,
    const MString& command_line,
    std::string& output)
{
#if defined(_WIN32) && !defined(WONVER)
    MProcessMaker maker;

    maker.SetCreationFlags(CREATE_NO_WINDOW);

    MFile hInputWrite, hOutputRead;
    if (!maker.PrepareForRedirect(&hInputWrite, &hOutputRead) ||
        !maker.CreateProcessDx(application.c_str(), command_line.c_str()))
    {
        return false;
    }

    maker.ReadAll(output, hOutputRead);
    return maker.GetExitCode() == 0;
#else
    putenv(g_lang_english);
    MString command_line_2 = application;
    command_line_2 += ' ';
    command_line_2 += command_line;
    FILE *fp = popen(command_line_2.c_str(), "r");
    if (!fp)
        return false;

    // A larger buffer means far fewer read()/append() round-trips for the
    // (often multi-megabyte) preprocessor output than the previous 256-byte
    // chunks did. Heap-allocated for the same reason as read_file_contents().
    std::vector<char> buf(65536);
    for (;;)
    {
        size_t count = fread(buf.data(), 1, buf.size(), fp);
        if (!count) break;
        output.append(buf.data(), count);
    }
    return pclose(fp) == 0;
#endif
}

//////////////////////////////////////////////////////////////////////////////
// Parsing helpers
//
// NOTE: Everything below this point works on the *contents* produced by the
// mcpp preprocessor (RC source / message table text). That text is always
// narrow, code-page-tagged data (its encoding is controlled at the RC level
// via `#pragma code_page(...)` and the `-c`/`--codepage` option), which is
// unrelated to the Unicode-ness of file *paths*. It is therefore left as
// plain std::string / char*, exactly as before.

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

// Fast path for the (overwhelmingly common) case where a message-table id
// is a bare integer literal such as "1", "0x20", or "-5", with no
// surrounding whitespace other than what's trimmed here. Avoids building a
// StringScanner/TokenStream/Parser and heap-allocated AST nodes -- which
// dominates do_mode_2()'s cost on large message tables -- for entries that
// don't need real expression evaluation. Returns false (leaving 'value'
// untouched) for anything that isn't a single plain literal, so callers can
// fall back to the full MacroParser for real expressions such as
// "WM_USER+1" or "(1<<2)".
static bool try_parse_plain_int(const char *str, size_t len, int& value)
{
    size_t i = 0;
    while (i < len && mchr_is_space(str[i]))
        ++i;

    size_t start = i;
    if (i < len && (str[i] == '+' || str[i] == '-'))
        ++i;

    size_t digits_start = i;
    if (i + 1 < len && str[i] == '0' && (str[i + 1] == 'x' || str[i + 1] == 'X'))
    {
        i += 2;
        size_t hex_start = i;
        while (i < len && mchr_is_xdigit(str[i]))
            ++i;
        if (i == hex_start)
            return false;
    }
    else
    {
        while (i < len && mchr_is_digit(str[i]))
            ++i;
        if (i == digits_start)
            return false;
    }

    size_t end = i;
    while (i < len && mchr_is_space(str[i]))
        ++i;
    if (i != len)
        return false; // trailing junk: not a plain literal, needs the real parser

    std::string token(str + start, end - start);
    value = mstr_parse_int(token.c_str(), true, 0);
    return true;
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

    if (!try_parse_plain_int(ptr0, ptr - ptr0, g_value))
    {
        // Not a bare literal (e.g. a macro/operator expression): fall back
        // to the full expression parser.
        MStringA str(ptr0, ptr);

        using namespace MacroParser;
        StringScanner scanner(str);
        TokenStream ts(scanner);
        ts.read_tokens();
        Parser parser(ts);
        if (!parser.parse() || !eval_int(parser.ast(), g_value))
            return syntax_error();
    }

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
    // Preprocessed message-table output is easily tens of thousands of
    // lines; reserving avoids repeated vector growth/copy while splitting.
    lines.reserve(std::count(output.begin(), output.end(), '\n') + 1);
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
                                g_table_id = MIdOrString(MAnsiToWide(g_wCodePage, token.c_str()).c_str());
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
                            char *p2 = mstr_skip_space(ptr);
                            if (memcmp("MESSAGETABLEDX", p2, 14) == 0 &&
                                (mchr_is_space(p2[14]) || p2[14] == 0 || p2[14] == '{'))
                            {
                                int val = 1;
                                bool ok = try_parse_plain_int(ptr0, ptr - ptr0, val);
                                if (!ok)
                                {
                                    MStringA token(ptr0, ptr);
                                    using namespace MacroParser;
                                    StringScanner scanner(token);
                                    TokenStream ts(scanner);
                                    ts.read_tokens();
                                    Parser parser(ts);
                                    ok = parser.parse() && eval_int(parser.ast(), val);
                                }
                                if (ok)
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

int save_rc(const MString& output_file)
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

int save_res(const MString& output_file)
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

int save_coff(const MString& output_file)
{
    MString temp_file;
    if (FILE *fp = tmpfilenam(temp_file))
        fclose(fp);
    else
    {
        fprintf(stderr, "ERROR: Unable to create temporary file.\n");
        return EXITCODE_CANT_MAKE_TEMP;
    }

    AutoDeleteFile auto_delete_0(temp_file);

    if (int ret = save_res(temp_file))
        return ret;

    MString command_line;
    append_process_arg(command_line, temp_file);
    if (!output_file.empty())
    {
        command_line += TEXT(" ");
        append_process_arg(command_line, output_file);
    }
    else
    {
        command_line += TEXT(" -O coff");
    }

#if defined(_WIN32) && !defined(WONVER)
    SetEnvironmentVariable(TEXT("LANG"), TEXT("en_US"));
#endif

    std::string output;
    if (run_process(g_windres, command_line, output))
        return EXITCODE_SUCCESS;

    if (output.find(": no resources") != std::string::npos)
        return EXITCODE_SUCCESS;

    fputs(output.c_str(), stderr);
    fprintf(stderr, "ERROR: Failed to create process\n");
    return EXITCODE_FAIL_TO_PREPROCESS;
}

int save_bin(const MString& output_file)
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

bool IsUTF16File(const MString& input_file)
{
    FILE *fp = T_fopen(input_file, TEXT("rb"));
    if (!fp)
        return false;

    char ab[2];
    bool result = fread(ab, 1, 2, fp) == 2 &&
                  (memcmp(ab, "\xFF\xFE", 2) == 0 || (ab[0] && !ab[1]));
    fclose(fp);
    return result;
}

int load_rc(const MString& input_file)
{
    // Apply undefinitions
    for (const auto& undef : g_undefinitions)
    {
        size_t ulen = undef.size();
        for (size_t k = 0; k < g_definitions.size(); ++k)
        {
            if (g_definitions[k].find(undef) == 0)
            {
                MString::value_type c = g_definitions[k].c_str()[ulen];
                if (c == 0 || c == TEXT('='))
                {
                    g_definitions.erase(g_definitions.begin() + k);
                    --k;
                }
            }
        }
    }

    // Build preprocessor command line
    MString command_line;
    for (const auto& def : g_definitions)
    {
        command_line += TEXT(" -D");
        append_process_arg(command_line, def);
    }
    for (const auto& inc : g_include_directories)
    {
        command_line += TEXT(" -I");
        append_process_arg(command_line, inc);
    }
    command_line += TEXT(" ");
    append_process_arg(command_line, input_file);

    g_strFile  = to_narrow(input_file);
    g_nLineNo  = 1;

    std::string output;
    if (run_process(g_cpp, command_line, output))
        return eat_output(output);

    fputs(output.c_str(), stderr);
    fprintf(stderr, "ERROR: Failed to preprocess\n");
    return EXITCODE_FAIL_TO_PREPROCESS;
}

int load_bin(const MString& input_file)
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

int load_res(const MString& input_file)
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

const char *get_format(const MString& file_path)
{
    const MString::value_type *pch = mstrrchr(file_path.c_str(), TEXT('.'));
    if (!pch)
        return "rc";

    MString ext(pch);
    if (ext == TEXT(".rc"))   return "rc";
    if (ext == TEXT(".res"))  return "res";
    if (ext == TEXT(".bin"))  return "bin";
    if (ext == TEXT(".o") || ext == TEXT(".obj") || ext == TEXT(".coff"))
        return "coff";
    return "rc";
}

int just_do_it(void)
{
    // Load
    if (g_inp_format == "rc")
    {
        if (int r = load_rc(g_input_file))
            return r;
    }
    else if (g_inp_format == "res")
    {
        if (int r = load_res(g_input_file))
            return r;
    }
    else if (g_inp_format == "bin")
    {
        if (int r = load_bin(g_input_file))
            return r;
    }
    else if (g_inp_format == "coff")
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
    if (g_out_format == "rc")
        return save_rc(g_output_file);
    else if (g_out_format == "res")
        return save_res(g_output_file);
    else if (g_out_format == "bin")
        return save_bin(g_output_file);
    else if (g_out_format == "coff")
        return save_coff(g_output_file);
    else
    {
        fprintf(stderr, "ERROR: invalid output format\n");
        return EXITCODE_INVALID_ARGUMENT;
    }
}

//////////////////////////////////////////////////////////////////////////////
// Command-line option table (replaces getopt_long; no external dependency)

struct OptionSpec
{
    const MString::value_type *name;   // long option name, e.g. "input"
    char code;                         // short option letter
    bool has_arg;
};

static const OptionSpec s_options[] =
{
    { TEXT("help"),           'h', false },
    { TEXT("version"),        'V', false },
    { TEXT("input"),          'i', true  },
    { TEXT("output"),         'o', true  },
    { TEXT("input-format"),   'J', true  },
    { TEXT("output-format"),  'O', true  },
    { TEXT("include-dir"),    'I', true  },
    { TEXT("define"),         'D', true  },
    { TEXT("undefine"),       'U', true  },
    { TEXT("codepage"),       'c', true  },
    { TEXT("language"),       'l', true  },
    { TEXT("preprocessor"),   'p', true  },
    { TEXT("windres"),        'w', true  },
};

// Applies one parsed option. Returns -1 to continue parsing, or an
// EXITCODE_* value (>= 0) when parsing should stop immediately.
static int apply_option(char code, const MString *value)
{
    switch (code)
    {
    case 'h':
        show_help();
        return EXITCODE_SUCCESS;
    case 'V':
        show_version();
        return EXITCODE_SUCCESS;
    case 'i':
        if (!g_input_file.empty())
        {
            fprintf(stderr, "ERROR: Too many input files\n");
            return EXITCODE_INVALID_ARGUMENT;
        }
        g_input_file = *value;
        return -1;
    case 'o':
        if (!g_output_file.empty())
        {
            fprintf(stderr, "ERROR: Too many output files\n");
            return EXITCODE_INVALID_ARGUMENT;
        }
        g_output_file = *value;
        return -1;
    case 'J':
        g_inp_format = to_narrow(*value);
        return -1;
    case 'O':
        g_out_format = to_narrow(*value);
        return -1;
    case 'I':
        g_include_directories.push_back(*value);
        return -1;
    case 'D':
        g_definitions.push_back(*value);
        return -1;
    case 'U':
        g_undefinitions.push_back(*value);
        return -1;
    case 'c':
        g_wCodePage = (uint16_t)mstr_parse_int(value->c_str(), false, 0);
        return -1;
    case 'l':
        {
            uint16_t w    = (uint16_t)mstr_parse_int(value->c_str(), false, 0);
            uint8_t  bPrim = LOBYTE(w);
            uint8_t  bSub  = HIBYTE(w);
            g_langid = MAKELANGID(bPrim, bSub);
        }
        return -1;
    case 'p':
        g_cpp = *value;
        return -1;
    case 'w':
        g_windres = *value;
        return -1;
    default:
        fprintf(stderr, "ERROR: Unknown option\n");
        return EXITCODE_INVALID_ARGUMENT;
    }
}

// Returns true/false via 'known' for whether c is a recognized short option,
// and (when known) whether it takes an argument.
static bool short_option_lookup(char c, bool& needs_arg)
{
    switch (c)
    {
    case 'h': case 'V':
        needs_arg = false;
        return true;
    case 'i': case 'o': case 'J': case 'O': case 'I':
    case 'D': case 'U': case 'c': case 'l': case 'p': case 'w':
        needs_arg = true;
        return true;
    default:
        needs_arg = false;
        return false;
    }
}

//////////////////////////////////////////////////////////////////////////////
// Entry point (platform-neutral; works on an array of MString arguments)

int mcdx_main(int argc, MString *argv)
{
    g_definitions.push_back(TEXT("RC_INVOKED"));
    g_definitions.push_back(TEXT("MCDX_INVOKED"));

#ifdef __CYGWIN__
    extern char __declspec(dllimport) *__progname;
    g_progname = __progname;
#else
    if (argc > 0)
        g_progname = argv[0];
#endif

    std::vector<MString> positional;
    bool no_more_opts = false;

    for (int i = 1; i < argc; ++i)
    {
        const MString& arg = argv[i];

        if (!no_more_opts && arg == TEXT("--"))
        {
            no_more_opts = true;
            continue;
        }

        if (!no_more_opts && arg.size() >= 2 && arg[0] == TEXT('-') && arg[1] == TEXT('-'))
        {
            MString body = arg.substr(2);
            MString name = body;
            MString value;
            bool has_value = false;

            size_t eq = body.find(TEXT('='));
            if (eq != MString::npos)
            {
                name  = body.substr(0, eq);
                value = body.substr(eq + 1);
                has_value = true;
            }

            const OptionSpec *spec = NULL;
            for (size_t k = 0; k < _countof(s_options); ++k)
            {
                if (name == s_options[k].name)
                {
                    spec = &s_options[k];
                    break;
                }
            }
            if (!spec)
            {
                fprintf(stderr, "ERROR: Unknown option: --%s\n", to_narrow(name).c_str());
                return EXITCODE_INVALID_ARGUMENT;
            }

            if (spec->has_arg && !has_value)
            {
                if (i + 1 >= argc)
                {
                    fprintf(stderr, "ERROR: Option --%s requires an argument\n", to_narrow(name).c_str());
                    return EXITCODE_INVALID_ARGUMENT;
                }
                value = argv[++i];
                has_value = true;
            }

            int ret = apply_option(spec->code, has_value ? &value : NULL);
            if (ret >= 0)
                return ret;
            continue;
        }

        if (!no_more_opts && arg.size() >= 2 && arg[0] == TEXT('-'))
        {
            char c = (char)arg[1];
            bool needs_arg = false;
            if (!short_option_lookup(c, needs_arg))
            {
                fprintf(stderr, "ERROR: Unknown option: -%c\n", c);
                return EXITCODE_INVALID_ARGUMENT;
            }

            MString value;
            bool has_value = false;
            if (needs_arg)
            {
                if (arg.size() > 2)
                {
                    value = arg.substr(2);
                    has_value = true;
                }
                else
                {
                    if (i + 1 >= argc)
                    {
                        fprintf(stderr, "ERROR: Option -%c requires an argument\n", c);
                        return EXITCODE_INVALID_ARGUMENT;
                    }
                    value = argv[++i];
                    has_value = true;
                }
            }

            int ret = apply_option(c, has_value ? &value : NULL);
            if (ret >= 0)
                return ret;
            continue;
        }

        positional.push_back(arg);
    }

    // Positional arguments
    for (size_t k = 0; k < positional.size(); ++k)
    {
        if (g_input_file.empty())
            g_input_file = positional[k];
        else if (g_output_file.empty())
            g_output_file = positional[k];
        else
        {
            fprintf(stderr, "ERROR: Too many arguments\n");
            return EXITCODE_INVALID_ARGUMENT;
        }
    }

    AutoDeleteFile auto_delete_1;

    // Read stdin into temp file when no input file given
    if (g_input_file.empty())
    {
        MString temp_file;
        FILE *fp = tmpfilenam(temp_file);
        if (!fp)
        {
            fprintf(stderr, "ERROR: Unable to create temporary file\n");
            return EXITCODE_CANT_MAKE_TEMP;
        }
        auto_delete_1.m_file = temp_file;
        char buf[512];
        while (fgets(buf, _countof(buf), stdin))
            fputs(buf, fp);
        fclose(fp);
        g_input_file = temp_file;
    }

    if (g_inp_format.empty())
        g_inp_format = get_format(g_input_file);

    if (g_out_format.empty())
        g_out_format = !g_output_file.empty() ? get_format(g_output_file) : "rc";

    return just_do_it();
}

#if defined(_WIN32) && !defined(WONVER)

// Windows entry point: obtains the *original* Unicode command line via the
// Win32 API (CommandLineToArgvW) instead of relying on a CRT-provided
// wmain()/argv, and instead of any getopt-style shim. This is what gives
// mcdx correct Unicode support for file paths and arguments on Windows.
int main()
{
    int argc = 0;
    LPWSTR *wargv = ::CommandLineToArgvW(::GetCommandLineW(), &argc);
    if (!wargv)
    {
        fprintf(stderr, "ERROR: Unable to parse command line.\n");
        return EXITCODE_INVALID_ARGUMENT;
    }

    std::vector<MString> args(argc);
    for (int i = 0; i < argc; ++i)
        args[i] = wargv[i];
    ::LocalFree(wargv);

    return mcdx_main(argc, args.empty() ? NULL : &args[0]);
}

#else

int main(int argc, char **argv)
{
    std::vector<MString> args(argc);
    for (int i = 0; i < argc; ++i)
        args[i] = argv[i];

    return mcdx_main(argc, args.empty() ? NULL : &args[0]);
}

#endif

//////////////////////////////////////////////////////////////////////////////
