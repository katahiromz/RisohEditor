// MessageRes --- Message Table Resources
//////////////////////////////////////////////////////////////////////////////
// RisohEditor --- Win32API resource editor
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

#include <windows.h>
#include <cassert>
#include <vector>
#include <map>

#include "MByteStreamEx.hpp"
#include "MString.hpp"

//////////////////////////////////////////////////////////////////////////////

struct MESSAGE_RESOURCE_ENTRY_HEADER
{
    USHORT  Length;
    USHORT  Flags;
};

//////////////////////////////////////////////////////////////////////////////

class MessageRes
{
public:
    typedef std::map<ULONG, std::wstring> map_type;

    MessageRes() { }

    BOOL LoadFromStream(const MByteStreamEx& stream)
    {
        m_map.clear();
        if (stream.size() < 4)
            return FALSE;

        ULONG NumberOfBlocks;
        if (!stream.ReadRaw(NumberOfBlocks) || NumberOfBlocks == 0)
            return FALSE;

        std::vector<MESSAGE_RESOURCE_BLOCK> Blocks(NumberOfBlocks);
        DWORD SizeOfBlocks = sizeof(MESSAGE_RESOURCE_BLOCK) * NumberOfBlocks;
        if (!stream.ReadData(&Blocks[0], SizeOfBlocks))
            return FALSE;

        for (DWORD i = 0; i < NumberOfBlocks; ++i)
        {
            const MESSAGE_RESOURCE_BLOCK& Block = Blocks[i];
            DWORD Offset = Block.OffsetToEntries;
            stream.pos(Offset);
            for (DWORD k = Block.LowId; k <= Block.HighId; ++k)
            {
                MESSAGE_RESOURCE_ENTRY_HEADER header;
                if (!stream.ReadRaw(header))
                    return FALSE;

                std::vector<BYTE> data(header.Length);
                if (header.Length)
                {
                    if (!stream.ReadData(&data[0], header.Length))
                        return FALSE;
                }

                if (header.Flags == MESSAGE_RESOURCE_UNICODE)
                {
                    size_t len = data.size() / sizeof(wchar_t);
                    if (len)
                    {
                        std::wstring str((const wchar_t *)&data[0], len);
                        m_map[k] = str;
                    }
                    else
                    {
                        m_map[k].clear();
                    }
                }
                else
                {
                    if (data.size())
                    {
                        MStringA str((const char *)&data[0], data.size());
                        MStringW wstr = MAnsiToWide(CP_ACP, str).c_str();
                        m_map[k] = wstr;
                    }
                    else
                    {
                        m_map[k].clear();
                    }
                }
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
                    header.Length = (WORD)(wstr.size() * sizeof(WCHAR));

                    if (mstr_is_ascii(wstr))
                    {
                        MWideToAnsi astr(CP_ACP, wstr);

                        header.Flags = 0;
                        if (!stream.WriteRaw(header))
                            return FALSE;

                        size_t size = astr.size() * sizeof(char);
                        if (!stream.WriteData(&astr[0], size))
                            return FALSE;
                    }
                    else
                    {
                        header.Flags = MESSAGE_RESOURCE_UNICODE;
                        if (!stream.WriteRaw(header))
                            return FALSE;

                        size_t size = wstr.size() * sizeof(WCHAR);
                        if (!stream.WriteData(&wstr[0], size))
                            return FALSE;
                    }
                }
            }
        }

        return TRUE;
    }

    std::wstring Dump() const
    {
        std::wstring ret;
        WCHAR sz[64];

        ret += L"MESSAGETABLE\r\n";
        ret += L"{\r\n";

        map_type::const_iterator it, end = m_map.end();
        for (it = m_map.begin(); it != end; ++it)
        {
            wsprintfW(sz, L"%u", it->first);
            ret += L"    ";
            ret += sz;
            ret += L", \"";
            ret += mstr_escape(it->second);
            ret += L"\"\r\n";
        }

        ret += L"}\r\n";
        return ret;
    }

protected:
    map_type    m_map;

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
                offset += sizeof(USHORT) * 2;
                offset += m_map[k].size() * sizeof(WCHAR);
            }
        }
        return TRUE;
    }
};

//////////////////////////////////////////////////////////////////////////////

#endif  // ndef MESSAGE_RES_HPP_
