// ConstantsDB.hpp --- Constants Database
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

#ifndef CONSTANTS_DB_HPP_
#define CONSTANTS_DB_HPP_

#pragma once

#include <string>
#include <vector>
#include <map>
#include <cctype>
#include <cstdio>
#include <iostream>

#include "MString.hpp"
#include "MIdOrString.hpp"

//////////////////////////////////////////////////////////////////////////////

enum IDTYPE_
{
    IDTYPE_UNKNOWN      = 0,  // Unknown.ID
    IDTYPE_CURSOR       = 1,  // Cursor.ID
    IDTYPE_BITMAP       = 2,  // Bitmap.ID
    IDTYPE_MENU         = 3,  // Menu.ID
    IDTYPE_DIALOG       = 4,  // Dialog.ID
    IDTYPE_STRING       = 5,  // String.ID
    IDTYPE_ACCEL        = 6,  // Accel.ID
    IDTYPE_ICON         = 7,  // Icon.ID
    IDTYPE_ANICURSOR    = 8,  // AniCursor.ID
    IDTYPE_ANIICON      = 9,  // AniIcon.ID
    IDTYPE_HTML         = 10, // Html.ID
    IDTYPE_HELP         = 11, // Help.ID
    IDTYPE_COMMAND      = 12, // Command.ID
    IDTYPE_CONTROL      = 13, // Control.ID
    IDTYPE_RESOURCE     = 14, // Resource.ID
    IDTYPE_MESSAGE      = 15, // Message.ID
    IDTYPE_WINDOW       = 16, // Window.ID
    IDTYPE_NEWCOMMAND   = 17, // New.Command.ID
    IDTYPE_PROMPT       = 18, // Prompt.ID
    IDTYPE_RCDATA       = 19  // RCData.ID
};

class ConstantsDB
{
public:
    typedef std::wstring StringType;
    typedef StringType NameType;
    typedef StringType CategoryType;
    typedef DWORD ValueType;
    typedef std::vector<ValueType> ValuesType;

    struct EntryType
    {
        NameType    name;
        ValueType   value;
        ValueType   mask;
        bool        selected;

        EntryType(NameType name_, ValueType value_)
            : name(name_), value(value_), mask(value_), selected(false)
        {
        }
        EntryType(NameType name_, ValueType value_, ValueType mask_)
            : name(name_), value(value_), mask(mask_), selected(false)
        {
        }
    };
    typedef std::vector<EntryType> TableType;
    typedef std::map<CategoryType, TableType> MapType;
    MapType m_map;

    bool IsEntityIDType(IDTYPE_ nIDTYPE_) const
    {
        switch (nIDTYPE_)
        {
        case IDTYPE_CURSOR:
        case IDTYPE_BITMAP:
        case IDTYPE_MENU:
        case IDTYPE_DIALOG:
        case IDTYPE_ACCEL:
        case IDTYPE_ICON:
        case IDTYPE_ANICURSOR:
        case IDTYPE_ANIICON:
        case IDTYPE_HTML:
        case IDTYPE_RESOURCE:
        case IDTYPE_RCDATA:
            return true;
        }
        return false;
    }

    IDTYPE_ IDTypeFromResType(const MIdOrString& type) const
    {
        if (type == RT_CURSOR)
        {
            return IDTYPE_UNKNOWN;
        }
        if (type == RT_BITMAP)
        {
            return IDTYPE_BITMAP;
        }
        if (type == RT_ICON)
        {
            return IDTYPE_UNKNOWN;
        }
        if (type == RT_MENU)
        {
            return IDTYPE_MENU;
        }
        if (type == RT_DIALOG)
        {
            return IDTYPE_DIALOG;
        }
        if (type == RT_STRING)
        {
            return IDTYPE_STRING;
        }
        if (type == RT_ACCELERATOR)
        {
            return IDTYPE_ACCEL;
        }
        if (type == RT_GROUP_CURSOR)
        {
            return IDTYPE_CURSOR;
        }
        if (type == RT_GROUP_ICON)
        {
            return IDTYPE_ICON;
        }
        if (type == RT_VERSION)
        {
            return IDTYPE_UNKNOWN;
        }
        if (type == RT_DLGINCLUDE)
        {
            return IDTYPE_UNKNOWN;
        }
        if (type == RT_ANICURSOR)
        {
            return IDTYPE_ANICURSOR;
        }
        if (type == RT_ANIICON)
        {
            return IDTYPE_ANIICON;
        }
        if (type == RT_HTML)
        {
            return IDTYPE_HTML;
        }
        if (type == RT_MANIFEST)
        {
            return IDTYPE_UNKNOWN;
        }
        if (type == RT_MESSAGETABLE)
        {
            return IDTYPE_MESSAGE;
        }
        if (type == RT_RCDATA)
        {
            return IDTYPE_RCDATA;
        }
        return IDTYPE_RESOURCE;
    }

    ConstantsDB()
    {
    }

    TableType GetTable(CategoryType category) const
    {
        ::CharUpperW(&category[0]);

        MapType::const_iterator it = m_map.find(category);
        if (it == m_map.end())
            return TableType();

        TableType table = it->second;
        return table;
    }

    TableType GetTableByPrefix(CategoryType category, NameType prefix) const
    {
        ::CharUpperW(&category[0]);

        TableType table1;
        MapType::const_iterator found = m_map.find(category);
        if (found == m_map.end())
            return table1;

        const TableType& table2 = found->second;
        TableType::const_iterator it, end = table2.end();
        for (it = table2.begin(); it != end; ++it)
        {
            if (it->name.find(prefix) == 0)
            {
                table1.push_back(*it);
            }
        }
        return table1;
    }

    ValueType GetResIDValue(NameType name) const
    {
        ValueType value = GetValue(L"RESOURCE.ID", name);
        if (!value)
        {
            value = GetValue(L"CTRLID", name);
        }
        return value;
    }
    ValueType GetCtrlIDValue(NameType name) const
    {
        return GetValue(L"CTRLID", name);
    }

    bool HasCtrlID(NameType name) const
    {
        TableType table = GetTable(L"CTRLID");
        for (auto& table_entry : table)
        {
            if (table_entry.name == name)
                return true;
        }
        return false;
    }
    bool HasResID(NameType name) const
    {
        TableType table = GetTable(L"RESOURCE.ID");
        for (auto& table_entry : table)
        {
            if (table_entry.name == name)
                return true;
        }
        table = GetTable(L"CTRLID");
        for (auto& table_entry : table)
        {
            if (table_entry.name == name)
                return true;
        }
        return false;
    }

    bool AreMacroIDShown() const
    {
        return !GetValue(L"HIDE.ID", L"HIDE.ID");
    }

    void ShowMacroID(bool bShown = true)
    {
        TableType& table = m_map[L"HIDE.ID"];
        table.clear();
        EntryType entry(L"HIDE.ID", !bShown);
        table.push_back(entry);
    }

    bool DoesUseIDC_STATIC() const
    {
        return !!GetValue(L"USE.IDC_STATIC", L"USE.IDC_STATIC");
    }

    void UseIDC_STATIC(bool bUse = true)
    {
        TableType& table = m_map[L"USE.IDC_STATIC"];
        table.clear();
        EntryType entry(L"USE.IDC_STATIC", bUse);
        table.push_back(entry);
    }

    void AddIDC_STATIC()
    {
        const NameType name = L"IDC_STATIC";
        TableType table = GetTable(L"RESOURCE.ID");
        auto end = table.end();
        for (auto it = table.begin(); it != end; ++it)
        {
            if (it->name == name)
            {
                table.erase(it);
                break;
            }
        }
        EntryType entry(name, -1);
        table.push_back(entry);
        m_map[L"RESOURCE.ID"] = table;
    }

    StringType GetNameOfResID(IDTYPE_ nIDTYPE_, ValueType value) const
    {
        if (!AreMacroIDShown())
        {
            if (nIDTYPE_ == IDTYPE_CONTROL)
                return mstr_dec_short((SHORT)value);
            else if (nIDTYPE_ == IDTYPE_MESSAGE)
                return mstr_hex(value);
            else
                return mstr_dec_word((WORD)value);
        }

        TableType table = GetTable(L"RESOURCE.ID.PREFIX");
        if (nIDTYPE_ < 0 || nIDTYPE_ >= INT(table.size()))
        {
            return mstr_dec_word((WORD)value);
        }

        StringType prefix = table[nIDTYPE_].name;
        if (prefix.empty())
        {
            if (nIDTYPE_ == IDTYPE_CONTROL)
                return mstr_dec_short((SHORT)value);
            else if (nIDTYPE_ == IDTYPE_MESSAGE)
                return mstr_hex(value);
            else
                return mstr_dec_word((WORD)value);
        }

        table = GetTableByPrefix(L"RESOURCE.ID", prefix);
        auto end = table.end();
        for (auto it = table.begin(); it != end; ++it)
        {
            if (it->value == value)
                return it->name;
        }

        if (nIDTYPE_ == IDTYPE_CONTROL)
        {
            if (value == -1 || value == 0xFFFF)
            {
                if (DoesUseIDC_STATIC() && AreMacroIDShown())
                    return L"IDC_STATIC";
                return L"-1";
            }

            return GetCtrlOrCmdName(value);
        }

        if (nIDTYPE_ != IDTYPE_RESOURCE && IsEntityIDType(nIDTYPE_))
        {
            return GetNameOfResID(IDTYPE_RESOURCE, value);
        }

        if (nIDTYPE_ == IDTYPE_HELP)
        {
            return mstr_dec_dword(value);
        }

        if (nIDTYPE_ == IDTYPE_MESSAGE)
        {
            return mstr_hex(value);
        }

        if (nIDTYPE_ == IDTYPE_COMMAND)
        {
            return GetCtrlOrCmdName(value);
        }

        return mstr_dec_word(WORD(value));
    }

    StringType GetNameOfIDTypeValue(IDTYPE_ nIDTYPE_, ValueType value) const
    {
        TableType table = GetTable(L"RESOURCE.ID.PREFIX");
        StringType prefix = table[nIDTYPE_].name;

        table = GetTableByPrefix(L"RESOURCE.ID", prefix);
        {
            auto end = table.end();
            for (auto it = table.begin(); it != end; ++it)
            {
                if (it->value == value)
                    return it->name;
            }
        }
        return L"";
    }

    StringType GetCtrlOrCmdName(ValueType value) const
    {
        StringType ret = DumpValue(L"CTRLID", value);
        if (ret.empty() || ret[0] == L'-' || mchr_is_digit(ret[0]))
        {
            StringType str;
            str = GetNameOfIDTypeValue(IDTYPE_COMMAND, value);
            if (str.size())
                return str;
            str = GetNameOfIDTypeValue(IDTYPE_CONTROL, value);
            if (str.size())
                return str;
            str = GetNameOfIDTypeValue(IDTYPE_NEWCOMMAND, value);
            if (str.size())
                return str;
        }
        return ret;
    }

    StringType GetNameOfResID(IDTYPE_ nIDTYPE_1, IDTYPE_ nIDTYPE_2, ValueType value) const
    {
        StringType ret = GetNameOfResID(nIDTYPE_1, value);
        if (mchr_is_digit(ret[0]) || ret[0] == L'-')
            ret = GetNameOfResID(nIDTYPE_2, value);
        return ret;
    }

    NameType GetName(CategoryType category, ValueType value) const
    {
        const TableType& table = GetTable(category);
        TableType::const_iterator it, end = table.end();
        for (it = table.begin(); it != end; ++it)
        {
            if (it->value == value)
                return it->name;
        }
        return NameType();
    }

    ValueType GetValue(CategoryType category, NameType name) const
    {
        const TableType& table = GetTable(category);
        TableType::const_iterator it, end = table.end();
        for (it = table.begin(); it != end; ++it)
        {
            if (it->name == name)
                return it->value;
        }
        return (ValueType)mstr_parse_int(name.c_str());
    }

    ValuesType GetValues(CategoryType category, NameType name) const
    {
        ValuesType ret;
        const TableType& table = GetTable(category);
        TableType::const_iterator it, end = table.end();
        for (it = table.begin(); it != end; ++it)
        {
            if (it->name == name)
            {
                ret.push_back(it->value);
            }
        }
        return ret;
    }

    bool LoadFromFile(LPCWSTR FileName)
    {
        using namespace std;
        m_map.clear();

        FILE *fp;
        _wfopen_s(&fp, FileName, L"rb");
        if (fp == NULL)
            return false;

        CategoryType category;
        char buf[MAX_PATH];
        while (fgets(buf, MAX_PATH, fp))
        {
            MStringW line;
            line = MAnsiToWide(CP_ACP, buf);

            mstr_replace_all(line, L" |", L"|");
            mstr_replace_all(line, L"| ", L"|");

            mstr_trim(line);
            if (line.empty())
                continue;

            // "[category]"
            if (line[0] == L'[')
            {
                if (line[line.size() - 1] == L']')
                {
                    category = line.substr(1, line.size() - 2);
                    ::CharUpperW(&category[0]);
                    m_map[category];
                }
                continue;
            }

            // "[name], value, mask"
            static const wchar_t *s_delim = L" ,\r\n";
            wchar_t *context;
            WCHAR *pch0, *pch1, *pch2;
            pch0 = &line[0];
            if (*pch0 == L',')
            {
                pch0 = &line[0];
                *pch0 = 0;
                pch1 = wcstok_s(pch0 + 1, s_delim, &context);
            }
            else
            {
                pch0 = wcstok_s(&line[0], s_delim, &context);
                if (pch0 == NULL)
                    continue;
                pch1 = wcstok_s(NULL, s_delim, &context);
            }
            if (pch1 == NULL)
                continue;
            pch2 = wcstok_s(NULL, s_delim, &context);

            NameType name = pch0;
            mstr_trim(name);
            //if (name.empty())
            //    continue;

            StringType value_str = pch1;
            mstr_trim(value_str);
            if (value_str.empty())
                continue;

            StringType mask_str;
            if (pch2 == NULL)
            {
                mask_str = value_str;
            }
            else
            {
                mask_str = pch2;
            }
            mstr_trim(mask_str);

            ValueType value;
            if (iswdigit(value_str[0]))
            {
                value = mstr_parse_int(value_str.c_str(), false);
            }
            else
            {
                value = ParseBitField(category, value_str);
            }

            ValueType mask;
            if (iswdigit(mask_str[0]))
            {
                mask = mstr_parse_int(mask_str.c_str(), false);
            }
            else
            {
                mask = ParseBitField(category, mask_str);
            }

            EntryType entry(name, value, mask);
            m_map[category].push_back(entry);
        }

        fclose(fp);
        return true;
    }

    StringType DumpBitField(CategoryType cat1, ValueType& value,
                            ValueType default_value = 0) const
    {
        StringType ret, str1, str3;

        ValueType def = default_value;
        default_value &= ~value;
        value &= ~def;

        str1 = _dumpBitField(cat1, value);

        if (!str1.empty())
        {
            ret = str1;
        }
        else
        {
            ret = L"0";
        }

        if (value)
        {
            if (!ret.empty() && ret != L"0")
                ret += L" | ";

            ret += mstr_hex(value);
        }

        if (default_value)
        {
            str3 = _dumpBitField(cat1, default_value, true);
            if (ret == L"0")
                ret.clear();
            else if (!ret.empty() && !str3.empty())
                ret += L" | ";
            ret += str3;
        }

        return ret;
    }

    StringType DumpBitFieldOrZero(CategoryType cat1, ValueType& value,
                                  ValueType default_value = 0) const
    {
        StringType ret = DumpBitField(cat1, value, default_value);
        if (ret.empty())
            ret = L"0";
        return ret;
    }

    StringType DumpBitField(CategoryType cat1, CategoryType cat2,
                            ValueType& value, ValueType default_value = 0) const
    {
        StringType ret, str1, str2, str3;

        ValueType def = default_value;
        default_value &= ~value;
        value &= ~def;

        str1 = _dumpBitField(cat1, value);
        if (!cat2.empty())
            str2 = _dumpBitField(cat2, value);

        if (!str1.empty() && str1 != L"0")
        {
            if (!str2.empty() && str2 != L"0")
                ret = str1 + L" | " + str2;
            else
                ret = str1;
        }
        else
        {
            if (!str2.empty() && str2 != L"0")
                ret = str2;
            else
                ret = L"0";
        }

        if (value)
        {
            if (!ret.empty())
                ret += L" | ";

            ret += mstr_hex(value);
        }

        if (default_value)
        {
            str3 = _dumpBitField(cat1, default_value, true);
            if (ret == L"0")
                ret.clear();
            else if (!ret.empty() && !str3.empty())
                ret += L" | ";
            ret += str3;
        }

        return ret;
    }

    StringType DumpBitFieldOrZero(CategoryType cat1, CategoryType cat2,
                                  ValueType& value, ValueType default_value = 0) const
    {
        StringType ret = DumpBitField(cat1, cat2, value, default_value);
        if (ret.empty())
            ret = L"0";
        return ret;
    }

    StringType DumpValue(CategoryType category, ValueType value) const
    {
        StringType ret;

        ::CharUpperW(&category[0]);

        MapType::const_iterator found = m_map.find(category);
        if (found != m_map.end())
        {
            const TableType& table = found->second;
            TableType::const_iterator it, end = table.end();
            for (it = table.begin(); it != end; ++it)
            {
                if (value == it->value)
                {
                    return it->name;
                }
            }
        }

        return mstr_dec(value);
    }

    ValueType
    ParseBitField(CategoryType category, const StringType& str,
                  ValueType default_value = 0) const
    {
        ::CharUpperW(&category[0]);

        std::vector<StringType> values;
        mstr_split(values, str, L" \t\r\n|+");

        ValueType value = default_value;
        auto end = values.end();
        for (auto it = values.begin(); it != end; ++it)
        {
            mstr_trim(*it);
            if ((*it).empty())
                continue;

            if (iswdigit((*it)[0]))
            {
                value |= mstr_parse_int(it->c_str(), false);
            }
            else
            {
                if ((*it).find(L"NOT ") != StringType::npos)
                {
                    (*it) = (*it).substr(4);
                    mstr_trim(*it);
                    value &= ~GetValue(category, *it);
                }
                else
                {
                    value |= GetValue(category, *it);
                }
            }
        }

        return value;
    }

protected:
    StringType _dumpBitField(CategoryType category, ValueType& value,
                             bool bNot = false) const
    {
        StringType ret;

        ::CharUpperW(&category[0]);

        MapType::const_iterator found = m_map.find(category);
        if (found == m_map.end())
            return ret;

        const TableType& table = found->second;
        for (;;)
        {
            ValueType max_value = 0;
            TableType::const_iterator max_it = table.end();
            TableType::const_iterator it, end = table.end();
            for (it = table.begin(); it != end; ++it)
            {
                if (it->value == 0)
                    continue;

                if ((value & it->mask) == it->value)
                {
                    if (it->value > max_value)
                    {
                        max_value = it->value;
                        max_it = it;
                    }
                }
            }

            if (max_it == end)
                break;  // not found

            if (!ret.empty())
                ret += L" | ";
            if (bNot)
            {
                ret += L"NOT ";
            }

            ret += max_it->name;
            value &= ~max_it->value;
        }

        return ret;
    }
};

#ifdef USE_GLOBALS
    extern ConstantsDB g_db;
#else
    inline ConstantsDB& DB_GetMaster(void)
    {
        static ConstantsDB s_db;
        return s_db;
    }
    #define g_db DB_GetMaster()
#endif

//////////////////////////////////////////////////////////////////////////////

#endif  // ndef CONSTANTS_DB_HPP_

//////////////////////////////////////////////////////////////////////////////
