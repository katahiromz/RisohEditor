// VersionRes --- Version Resources
//////////////////////////////////////////////////////////////////////////////

#ifndef VERSION_RES_HPP_
#define VERSION_RES_HPP_

#include <windows.h>
#include <cassert>
#include <vector>
#include <map>

#include "ByteStream.hpp"
#include "Text.hpp"
#include "id_string.hpp"

//////////////////////////////////////////////////////////////////////////////

typedef struct VarInfoHead
{
    WORD    wLength;
    WORD    wValueLength;
    WORD    wType;
} VarInfoHead;

struct Var
{
    VarInfoHead             head;
    std::wstring            key;
    std::vector<BYTE>       value;
    std::vector<BYTE>       children;
    std::vector<Var>        vars;
};
typedef std::vector<Var> Vars;

//////////////////////////////////////////////////////////////////////////////

class VersionRes
{
public:
    VersionRes() { }

    BOOL VarsFromStream(Vars& vars, const ByteStream& stream)
    {
        Var var;

        stream.ReadDwordAlignment();

        DWORD pos0 = stream.pos();

        if (!stream.ReadRaw(var.head) || !stream.ReadSz(var.key))
            return FALSE;

        DWORD pos1 = pos0 + var.head.wLength;
        stream.ReadDwordAlignment();

        if (var.head.wValueLength)
        {
            DWORD dwSize = var.head.wValueLength;
            if (var.head.wType == 1)
                dwSize *= 2;
            var.value.resize(dwSize);
            if (!stream.ReadData(&var.value[0], dwSize))
                return FALSE;
        }
        stream.ReadDwordAlignment();

        while (stream.pos() < pos1)
        {
            VarsFromStream(var.vars, stream);
        }

        vars.push_back(var);

        return TRUE;
    }

    BOOL LoadFromData(const std::vector<BYTE>& data)
    {
        ZeroMemory(&m_fixed, sizeof(m_fixed));

        ByteStream stream(data);
        if (!VarsFromStream(m_vars, stream))
            return FALSE;

        if (m_vars.size() != 1)
            return FALSE;

        Var& var = m_vars[0];
        if (var.key != L"VS_VERSION_INFO")
            return FALSE;

        if (var.value.size() == sizeof(VS_FIXEDFILEINFO))
        {
            CopyMemory(&m_fixed, &var.value[0], var.value.size());
        }

        return TRUE;
    }

    std::wstring DumpValue(WORD wType, const Var& value, int depth = 0) const
    {
        std::wstring ret = std::wstring(depth * 4, L' ');
        ret += L"VALUE ";
        ret += str_quote(value.key);

        if (value.value.size() >= 2)
        {
            if (wType == 0)
            {
                LPWORD pw = LPWORD(&value.value[0]);
                WCHAR buf[MAX_PATH];
                for (size_t i = 0; i < value.value.size(); i += 2)
                {
                    wsprintfW(buf, L", 0x%04X", *pw++);
                    ret += buf;
                }
            }
            else
            {
                WCHAR *pch = (WCHAR *)(&value.value[0]);
                std::wstring str(pch, value.value.size() / 2);
                ret += L", ";
                ret += str_quote(str);
            }
        }
        else
        {
            ret += L", \"\"";
        }

        ret += L"\r\n";
        return ret;
    }

    std::wstring DumpBlock(const Var& var, int depth = 0) const
    {
        std::wstring ret;

        ret += std::wstring(depth * 4, L' ');
        ret += L"BLOCK \"";
        ret += var.key;
        ret += L"\"\r\n";
        ret += std::wstring(depth * 4, L' ');
        ret += L"{\r\n";

        Vars::const_iterator it, end = var.vars.end();
        for (it = var.vars.begin(); it != end; ++it)
        {
            if (var.key == L"StringFileInfo")
            {
                ret += DumpBlock(*it, depth + 1);
            }
            else
            {
                ret += DumpValue(it->head.wType, *it, depth + 1);
            }
        }

        ret += std::wstring(depth * 4, L' ');
        ret += L"}\r\n";

        return ret;
    }

    std::wstring Dump(const ID_OR_STRING& name) const
    {
        std::wstring ret;
        WCHAR line[MAX_PATH];

        ret += name.wstr();
        ret += L" VERSIONINFO\r\n";

        wsprintfW(line, L"FILEVERSION %u,%u,%u,%u\r\n",
            HIWORD(m_fixed.dwFileVersionMS),
            LOWORD(m_fixed.dwFileVersionMS),
            HIWORD(m_fixed.dwFileVersionLS),
            LOWORD(m_fixed.dwFileVersionLS));
        ret += line;

        wsprintfW(line, L"PRODUCTVERSION %u,%u,%u,%u\r\n",
            HIWORD(m_fixed.dwProductVersionMS),
            LOWORD(m_fixed.dwProductVersionMS),
            HIWORD(m_fixed.dwProductVersionLS),
            LOWORD(m_fixed.dwProductVersionLS));
        ret += line;

        wsprintfW(line, L"FILEOS 0x%04lX\r\n", m_fixed.dwFileOS);
        ret += line;

        wsprintfW(line, L"FILETYPE 0x%X\r\n", m_fixed.dwFileType);
        ret += line;

        ret += L"{\r\n";

        const std::vector<Var>& vars = m_vars[0].vars;
        std::vector<Var>::const_iterator it, end = vars.end();
        for (it = vars.begin(); it != end; ++it)
        {
            ret += DumpBlock(*it, 1);
        }
        ret += L"}\r\n";

        return ret;
    }

protected:
    std::vector<Var>        m_vars;
    VS_FIXEDFILEINFO        m_fixed;
};

//////////////////////////////////////////////////////////////////////////////

#endif  // ndef VERSION_RES_HPP_

//////////////////////////////////////////////////////////////////////////////
