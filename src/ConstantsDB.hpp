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

#define IDTYPE_CURSOR       0   // Cursor.ID
#define IDTYPE_BITMAP       1   // Bitmap.ID
#define IDTYPE_MENU         2   // Menu.ID
#define IDTYPE_DIALOG       3   // Dialog.ID
#define IDTYPE_STRING       4   // String.ID
#define IDTYPE_ACCEL        5   // Accel.ID
#define IDTYPE_ICON         6   // Icon.ID
#define IDTYPE_ANICURSOR    7   // AniCursor.ID
#define IDTYPE_ANIICON      8   // AniIcon.ID
#define IDTYPE_HTML         9   // Html.ID
#define IDTYPE_HELP         10  // Help.ID
#define IDTYPE_COMMAND      11  // Command.ID
#define IDTYPE_CONTROL      12  // Control.ID
#define IDTYPE_RESOURCE     13  // Resource.ID
#define IDTYPE_MESSAGE      14  // Message.ID
#define IDTYPE_INVALID      -1

class ConstantsDB
{
public:
    typedef std::wstring StringType;
    typedef StringType NameType;
    typedef StringType CategoryType;
    typedef DWORD ValueType;

    struct EntryType
    {
        NameType    name;
        ValueType   value;
        ValueType   mask;
        BOOL        selected;

        EntryType(NameType name_, ValueType value_)
            : name(name_), value(value_), mask(value_), selected(FALSE)
        {
        }
        EntryType(NameType name_, ValueType value_, ValueType mask_)
            : name(name_), value(value_), mask(mask_), selected(FALSE)
        {
        }
    };
    typedef std::vector<EntryType> TableType;
    typedef std::map<CategoryType, TableType> MapType;
    MapType m_map;

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
        return GetValue(L"RESOURCE.ID", name);
    }
    ValueType GetCtrlIDValue(NameType name) const
    {
        return GetValue(L"CTRLID", name);
    }

    BOOL HasCtrlID(NameType name) const
    {
        TableType table = GetTable(L"CTRLID");
        TableType::iterator it, end = table.end();
        for (it = table.begin(); it != end; ++it)
        {
            if (it->name == name)
                return TRUE;
        }
        return FALSE;
    }
    BOOL HasResID(NameType name) const
    {
        TableType table = GetTable(L"RESOURCE.ID");
        TableType::iterator it, end = table.end();
        for (it = table.begin(); it != end; ++it)
        {
            if (it->name == name)
                return TRUE;
        }
        return FALSE;
    }

    StringType GetNameOfResID(INT nIDTYPE_, ValueType value) const
    {
        if ((BOOL)GetValue(L"HIDE.ID", L"HIDE.ID"))
        {
            if (nIDTYPE_ == IDTYPE_CONTROL)
                return mstr_dec_short((SHORT)value);
            else if (nIDTYPE_ == IDTYPE_MESSAGE)
                return mstr_hex(value);
            else
                return mstr_dec_word((WORD)value);
        }

        TableType table = GetTable(L"RESOURCE.ID.PREFIX");
        if (nIDTYPE_ >= (INT)table.size())
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
        TableType::iterator it, end = table.end();
        for (it = table.begin(); it != end; ++it)
        {
            if (it->value == value)
                return it->name;
        }

        if (nIDTYPE_ == IDTYPE_CONTROL)
        {
            if (value == -1 || value == 0xFFFF)
                return L"-1";

            return DumpValue(L"CTRLID", value);
        }

        if (nIDTYPE_ != IDTYPE_RESOURCE && nIDTYPE_ != IDTYPE_STRING &&
            nIDTYPE_ != IDTYPE_CONTROL && nIDTYPE_ != IDTYPE_COMMAND &&
            nIDTYPE_ != IDTYPE_HELP && nIDTYPE_ != IDTYPE_MESSAGE)
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

        return mstr_dec_word(WORD(value));
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
        return (ValueType)wcstol(name.c_str(), NULL, 0);
    }

    template <typename T_STR_CONTAINER>
    void split_by_sep(T_STR_CONTAINER& container,
                      const typename T_STR_CONTAINER::value_type& str) const
    {
        const wchar_t *separators = L" \t\r\n|+";
        container.clear();
        size_t i = 0, j = str.find_first_of(separators);
        while (j != T_STR_CONTAINER::value_type::npos)
        {
            container.push_back(str.substr(i, j - i));
            i = j + 1;
            j = str.find_first_of(separators, i);
        }
        container.push_back(str.substr(i));
    }

    BOOL LoadFromFile(LPCWSTR FileName)
    {
        using namespace std;
        m_map.clear();

        FILE *fp = _wfopen(FileName, L"rb");
        if (fp == NULL)
            return FALSE;

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

            // "name, value, mask"
            static const wchar_t *s_delim = L" ,\r\n";
            WCHAR *pch0 = wcstok(&line[0], s_delim);
            if (pch0 == NULL)
                continue;
            WCHAR *pch1 = wcstok(NULL, s_delim);
            if (pch1 == NULL)
                continue;
            WCHAR *pch2 = wcstok(NULL, s_delim);

            NameType name = pch0;
            mstr_trim(name);
            if (name.empty())
                continue;

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
                value = wcstoul(value_str.c_str(), NULL, 0);
            }
            else
            {
                value = ParseBitField(category, value_str);
            }

            ValueType mask;
            if (iswdigit(mask_str[0]))
            {
                mask = wcstoul(mask_str.c_str(), NULL, 0);
            }
            else
            {
                mask = ParseBitField(category, mask_str);
            }

            EntryType entry(name, value, mask);
            m_map[category].push_back(entry);
        }

        fclose(fp);
        return TRUE;
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
            str3 = _dumpBitField(cat1, default_value, TRUE);
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
            str3 = _dumpBitField(cat1, default_value, TRUE);
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
        if (found == m_map.end())
            return ret;

        const TableType& table = found->second;
        TableType::const_iterator it, end = table.end();
        for (it = table.begin(); it != end; ++it)
        {
            if (value == it->value)
            {
                return it->name;
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
        split_by_sep(values, str);

        ValueType value = default_value;
        std::vector<StringType>::iterator it, end = values.end();
        for (it = values.begin(); it != end; ++it)
        {
            mstr_trim(*it);
            if ((*it).empty())
                continue;

            if (iswdigit((*it)[0]))
            {
                value |= wcstoul(it->c_str(), NULL, 0);
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

    INT IDTypeFromRes(const MIdOrString& id) const
    {
        if (id == RT_CURSOR)
        {
            return IDTYPE_RESOURCE;
        }
        if (id == RT_BITMAP)
        {
            return IDTYPE_BITMAP;
        }
        if (id == RT_ICON)
        {
            return IDTYPE_RESOURCE;
        }
        if (id == RT_MENU)
        {
            return IDTYPE_MENU;
        }
        if (id == RT_DIALOG)
        {
            return IDTYPE_DIALOG;
        }
        if (id == RT_STRING)
        {
            return IDTYPE_INVALID;
        }
        if (id == RT_ACCELERATOR)
        {
            return IDTYPE_ACCEL;
        }
        if (id == RT_GROUP_CURSOR)
        {
            return IDTYPE_CURSOR;
        }
        if (id == RT_GROUP_ICON)
        {
            return IDTYPE_ICON;
        }
        if (id == RT_VERSION)
        {
            return IDTYPE_INVALID;
        }
        if (id == RT_DLGINCLUDE)
        {
            return IDTYPE_INVALID;
        }
        if (id == RT_ANICURSOR)
        {
            return IDTYPE_ANICURSOR;
        }
        if (id == RT_ANIICON)
        {
            return IDTYPE_ANIICON;
        }
        if (id == RT_HTML)
        {
            return IDTYPE_HTML;
        }
        if (id == RT_MANIFEST)
        {
            return IDTYPE_INVALID;
        }
        if (id == RT_MESSAGETABLE)
        {
            return IDTYPE_MESSAGE;
        }
        return IDTYPE_RESOURCE;
    }

protected:
    StringType _dumpBitField(CategoryType category, ValueType& value,
                             BOOL bNot = FALSE) const
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

//////////////////////////////////////////////////////////////////////////////

#endif  // ndef CONSTANTS_DB_HPP_

//////////////////////////////////////////////////////////////////////////////
