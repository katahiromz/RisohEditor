// MessageRes.hpp --- Message Table Resources
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

#ifndef MESSAGE_RES_HPP_
#define MESSAGE_RES_HPP_

#include "MByteStreamEx.hpp"
#include "MString.hpp"
#include "MTextToText.hpp"
#include "ConstantsDB.hpp"
#include <map>

//////////////////////////////////////////////////////////////////////////////

// These structures are defined in <winnt.h>:
//     typedef struct _MESSAGE_RESOURCE_BLOCK {
//         DWORD    LowId;
//         DWORD    HighId;
//         DWORD    OffsetToEntries;        // from this structure
//     } MESSAGE_RESOURCE_BLOCK, *PMESSAGE_RESOURCE_BLOCK;
//     typedef struct _MESSAGE_RESOURCE_DATA {
//         DWORD                    NumberOfBlocks;
//         MESSAGE_RESOURCE_BLOCK   Blocks[1];
//     } MESSAGE_RESOURCE_DATA, *PMESSAGE_RESOURCE_DATA;
//     typedef struct _MESSAGE_RESOURCE_ENTRY {
//         WORD   Length;
//         WORD   Flags;        // 0 for ANSI, 1 for Unicode
//         BYTE   Text[1];
//     } MESSAGE_RESOURCE_ENTRY, *PMESSAGE_RESOURCE_ENTRY;

typedef struct _MESSAGE_RESOURCE_DATA_HEADER {
    DWORD                    NumberOfBlocks;
} MESSAGE_RESOURCE_DATA_HEADER, *PMESSAGE_RESOURCE_DATA_HEADER;

typedef struct _MESSAGE_RESOURCE_ENTRY_HEADER {
    WORD   Length;
    WORD   Flags;        // 0 for ANSI, 1 for Unicode
} MESSAGE_RESOURCE_ENTRY_HEADER, *PMESSAGE_RESOURCE_ENTRY_HEADER;

//////////////////////////////////////////////////////////////////////////////

class MessageRes
{
public:
    typedef std::wstring string_type;
    typedef std::map<ULONG, string_type> map_type;
    map_type    m_map;

    MessageRes()
    {
    }

    void clear()
    {
        m_map.clear();
    }

    BOOL LoadFromStream(const MByteStreamEx& stream, WORD wName)
    {
        m_map.clear();
        if (stream.size() < sizeof(MESSAGE_RESOURCE_DATA))
            return FALSE;

        MESSAGE_RESOURCE_DATA_HEADER data;
        if (!stream.ReadRaw(data) || data.NumberOfBlocks == 0)
            return FALSE;

        std::vector<MESSAGE_RESOURCE_BLOCK> blocks(data.NumberOfBlocks);
        DWORD dwSizeOfBlocks = sizeof(MESSAGE_RESOURCE_BLOCK) * data.NumberOfBlocks;
        if (!stream.ReadData(&blocks[0], dwSizeOfBlocks))
            return FALSE;

        for (DWORD i = 0; i < data.NumberOfBlocks; ++i)
        {
            const MESSAGE_RESOURCE_BLOCK& block = blocks[i];

            DWORD dwOffset = block.OffsetToEntries;
            stream.pos(dwOffset);

            for (DWORD dwID = block.LowId; dwID <= block.HighId; ++dwID)
            {
                size_t pos = stream.pos();

                MESSAGE_RESOURCE_ENTRY_HEADER entry_head;
                if (!stream.ReadRaw(entry_head))
                    return FALSE;

                std::wstring wstr = (const WCHAR *)&stream[stream.pos()];
                if (entry_head.Flags & MESSAGE_RESOURCE_UNICODE)
                {
                    size_t len = (entry_head.Length - sizeof(entry_head)) / sizeof(wchar_t);
                    std::wstring str;
                    str.resize(len);
                    if (!stream.ReadData(&str[0], len * sizeof(wchar_t)))
                    {
                        return FALSE;
                    }
                    str.resize(std::wcslen(str.c_str()));
                    m_map[dwID] = str;
                }
                else
                {
                    size_t len = entry_head.Length - sizeof(entry_head);
                    std::string str;
                    str.resize(len);
                    if (!stream.ReadData(&str[0], len * sizeof(char)))
                    {
                        return FALSE;
                    }
                    str.resize(std::strlen(str.c_str()));
                    m_map[dwID] = MAnsiToWide(CP_ACP, str.c_str()).c_str();
                }

                stream.pos(pos + entry_head.Length);
            }
        }

        return TRUE;
    }

    BOOL SaveToStream(MByteStreamEx& stream)
    {
        if (m_map.empty())
            return TRUE;

        ranges_type ranges;
        if (!GetRanges(ranges))
            return FALSE;

        offsets_type offsets;
        if (!OffsetsFromRanges(offsets, ranges))
            return FALSE;

        ULONG NumberOfBlocks = DWORD(ranges.size());
        if (!stream.WriteRaw(NumberOfBlocks))
            return FALSE;

        std::vector<MESSAGE_RESOURCE_BLOCK> Blocks;
        {
            size_t i = 0;
            ranges_type::iterator it, end = ranges.end();
            for (it = ranges.begin(); it != end; ++it, ++i)
            {
                MESSAGE_RESOURCE_BLOCK Block;
                Block.LowId = it->FirstId;
                Block.HighId = it->LastId;
                Block.OffsetToEntries = (DWORD)offsets[i];
                Blocks.push_back(Block);
            }
        }

        size_t SizeOfBlocks = Blocks.size() * sizeof(MESSAGE_RESOURCE_BLOCK);
        if (!stream.WriteData(&Blocks[0], SizeOfBlocks))
            return FALSE;

        {
            ranges_type::iterator it, end = ranges.end();
            for (it = ranges.begin(); it != end; ++it)
            {
                for (DWORD k = it->FirstId; k <= it->LastId; ++k)
                {
                    std::wstring& wstr = m_map[k];

                    MESSAGE_RESOURCE_ENTRY_HEADER header;
                    header.Length = (WORD)(sizeof(header) + (wstr.size() + 1) * sizeof(WCHAR));
                    header.Flags = MESSAGE_RESOURCE_UNICODE;
                    if (!stream.WriteRaw(header))
                        return FALSE;

                    header.Flags = MESSAGE_RESOURCE_UNICODE;
                    if (!stream.WriteRaw(header))
                        return FALSE;

                    size_t size = (wstr.size() + 1) * sizeof(WCHAR);
                    if (!stream.WriteData(&wstr[0], size))
                        return FALSE;
                }
            }
        }

        return TRUE;
    }

    string_type Dump(const ConstantsDB& db, WORD wName) const
    {
        std::wstring ret;

        ret += L"MESSAGETABLEDX\r\n";
        ret += L"{\r\n";

        map_type::const_iterator it, end = m_map.end();
        for (it = m_map.begin(); it != end; ++it)
        {
            ret += L"    ";
            if (0)
            {
                ret += mstr_hex(it->first);
            }
            else
            {
                ret += db.GetNameOfResID(IDTYPE_MESSAGE, it->first);
            }
            ret += L", \"";
            ret += mstr_escape(it->second);
            ret += L"\"\r\n";
        }

        ret += L"}\r\n";
        return ret;
    }

    string_type Dump(const ConstantsDB& db) const
    {
        return Dump(db, 1);
    }

protected:
    struct RANGE_OF_ID
    {
        ULONG FirstId;
        ULONG LastId;
    };
    typedef RANGE_OF_ID range_type;
    typedef std::vector<range_type> ranges_type;

    BOOL GetRanges(ranges_type& ranges) const
    {
        RANGE_OF_ID range = { 0xFFFFFFFF, 0xFFFFFFFF };

        map_type::const_iterator it, end = m_map.end();
        for (it = m_map.begin(); it != end; ++it)
        {
            if (range.LastId == 0xFFFFFFFF)
            {
                range.FirstId = range.LastId = it->first;
            }
            else if (range.LastId + 1 == it->first)
            {
                ++range.LastId;
            }
            else
            {
                ranges.push_back(range);
                range.FirstId = range.LastId = it->first;
            }
        }
        ranges.push_back(range);

        return TRUE;
    }

    typedef std::vector<size_t> offsets_type;
    BOOL OffsetsFromRanges(offsets_type& offsets, const ranges_type& ranges)
    {
        size_t offset = sizeof(ULONG);
        offset += sizeof(MESSAGE_RESOURCE_BLOCK) * ranges.size();

        ranges_type::const_iterator it, end = ranges.end();
        for (it = ranges.begin(); it != end; ++it)
        {
            offsets.push_back(offset);
            for (DWORD k = it->FirstId; k <= it->LastId; ++k)
            {
                offset += sizeof(MESSAGE_RESOURCE_ENTRY_HEADER);
                offset += (m_map[k].size() + 1) * sizeof(WCHAR);
            }
        }
        return TRUE;
    }
};

//////////////////////////////////////////////////////////////////////////////

#endif  // ndef MESSAGE_RES_HPP_
