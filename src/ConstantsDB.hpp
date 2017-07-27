// ConstantsDB --- Constants Database
//////////////////////////////////////////////////////////////////////////////

#ifndef CONSTANTS_DB_HPP_
#define CONSTANTS_DB_HPP_

#include <string>
#include <vector>
#include <map>
#include <cctype>
#include <cstdio>
#include <iostream>

#include "MString.hpp"
#include "id_string.hpp"

//////////////////////////////////////////////////////////////////////////////

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

    struct CheckItem
    {
        BOOL        checked;
        ValueType   value;
        ValueType   mask;
        NameType    name;

        CheckItem() : checked(FALSE), value(0), mask(0)
        {
        }
    };
    typedef std::vector<CheckItem> CheckList;

    ConstantsDB()
    {
    }

    TableType GetTable(CategoryType category) const
    {
        ::CharUpperW(&category[0]);

        MapType::const_iterator it = m_Map.find(category);
        if (it == m_Map.end())
            return TableType();

        TableType Table = it->second;
        return Table;
    }

    NameType GetName(CategoryType category, ValueType value) const
    {
        const TableType& Table = GetTable(category);
        TableType::const_iterator it, end = Table.end();
        for (it = Table.begin(); it != end; ++it)
        {
            if (it->value == value)
                return it->name;
        }
        return 0;
    }

    ValueType GetValue(CategoryType category, NameType name) const
    {
        const TableType& Table = GetTable(category);
        TableType::const_iterator it, end = Table.end();
        for (it = Table.begin(); it != end; ++it)
        {
            if (it->name == name)
                return it->value;
        }
        std::printf("%S: not found\n", name.c_str());
        return 0;
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
        m_Map.clear();

        FILE *fp = _wfopen(FileName, L"rb");
        if (fp == NULL)
            return FALSE;

        CategoryType category;
        char buf[MAX_PATH];
        while (fgets(buf, MAX_PATH, fp))
        {
            MStringW line;
            line = MAnsiToWide(buf);

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
                    m_Map[category];
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
            m_Map[category].push_back(entry);
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
            if (!ret.empty())
                ret += L" | ";

            ret += mstr_hex(value);
        }

        if (default_value)
        {
            str3 = _dumpBitField(cat1, default_value, TRUE);
            if (!ret.empty())
                ret += L" | ";
            ret += str3;
        }
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

        if (!str1.empty())
        {
            if (!str2.empty())
                ret = str1 + L" | " + str2;
            else
                ret = str1;
        }
        else
        {
            if (!str2.empty())
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
            if (!ret.empty())
                ret += L" | ";
            ret += str3;
        }
        return ret;
    }

    StringType DumpValue(CategoryType category, ValueType value) const
    {
        StringType ret;

        ::CharUpperW(&category[0]);

        MapType::const_iterator found = m_Map.find(category);
        if (found == m_Map.end())
            return ret;

        const TableType& Table = found->second;
        TableType::const_iterator it, end = Table.end();
        for (it = Table.begin(); it != end; ++it)
        {
            if (value == it->value)
            {
                return it->name;
            }
        }

        return mstr_hex(value);
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

    BOOL GetCheckList(CategoryType category, CheckList& List, ValueType value) const
    {
        ::CharUpperW(&category[0]);

        List.clear();

        MapType::const_iterator found = m_Map.find(category);
        if (found == m_Map.end())
            return FALSE;

        const TableType& Table = found->second;
        TableType::const_iterator it, end = Table.end();
        CheckItem item;
        for (it = Table.begin(); it != end; ++it)
        {
            if (it->value == 0)
                continue;

            item.checked = ((value & it->mask) == it->value);
            item.name = it->name;
            item.value = it->value;
            item.mask = it->mask;
            List.push_back(item);
        }

        return TRUE;
    }

    ValueType ValueFromCheckList(const CheckList& List) const
    {
        ValueType value = 0;
        CheckList::const_iterator it, end = List.end();
        for (it = List.begin(); it != end; ++it)
        {
            if (it->checked)
            {
                value &= ~it->mask;
                value |= it->value;
            }
        }
        return value;
    }

    std::wstring GetCtrlID(WORD id) const
    {
        std::wstring ret;
        if (id == 0xFFFF)
        {
            ret = L"-1";
        }
        else
        {
            ret = DumpValue(L"CTRLID", id);
        }
        return ret;
    }

protected:
    MapType m_Map;

    StringType _dumpBitField(CategoryType category, ValueType& value,
                             BOOL Not = FALSE) const
    {
        StringType ret;

        ::CharUpperW(&category[0]);

        MapType::const_iterator found = m_Map.find(category);
        if (found == m_Map.end())
            return ret;

        const TableType& Table = found->second;
        TableType::const_iterator it, end = Table.end();
        for (it = Table.begin(); it != end; ++it)
        {
            if (it->value == 0)
                continue;

            if ((value & it->mask) == it->value)
            {
                if (!ret.empty())
                    ret += L" | ";

                if (Not)
                    ret += L"NOT ";
                ret += it->name;
                value &= ~it->value;
            }
        }

        return ret;
    }
};

//////////////////////////////////////////////////////////////////////////////

#endif  // ndef CONSTANTS_DB_HPP_

//////////////////////////////////////////////////////////////////////////////
