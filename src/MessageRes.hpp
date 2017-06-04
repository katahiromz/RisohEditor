// MessageRes --- Message Table Resources
//////////////////////////////////////////////////////////////////////////////

#ifndef MESSAGE_RES_HPP_
#define MESSAGE_RES_HPP_

#include <windows.h>
#include <cassert>
#include <vector>
#include <map>

#include "ByteStream.hpp"
#include "Text.hpp"

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

    BOOL LoadFromStream(const ByteStream& stream)
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
                    DWORD len = data.size() / sizeof(wchar_t);
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
                        std::string str(data[0], data.size());
                        std::wstring wstr = AnsiToWide(str);
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

    BOOL SaveToStream(ByteStream& stream)
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
                Block.OffsetToEntries = offsets[i];
                Blocks.push_back(Block);
            }
        }

        DWORD SizeOfBlocks = Blocks.size() * sizeof(MESSAGE_RESOURCE_BLOCK);
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
                    header.Length = wstr.size() * sizeof(WCHAR);

                    if (is_ascii(wstr))
                    {
                        std::string astr = WideToAnsi(wstr);

                        header.Flags = 0;
                        if (!stream.WriteRaw(header))
                            return FALSE;

                        DWORD size = astr.size() * sizeof(char);
                        if (!stream.WriteData(&astr[0], size))
                            return FALSE;
                    }
                    else
                    {
                        header.Flags = MESSAGE_RESOURCE_UNICODE;
                        if (!stream.WriteRaw(header))
                            return FALSE;

                        DWORD size = wstr.size() * sizeof(WCHAR);
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
            ret += str_escape(it->second);
            ret += L"\"\r\n";
        }

        ret += L"}\r\n";
        return ret;
    }

protected:
    map_type    m_map;

    struct ID_RANGE
    {
        ULONG FirstId;
        ULONG LastId;
    };
    typedef ID_RANGE range_type;
    typedef std::vector<range_type> ranges_type;

    BOOL GetRanges(ranges_type& ranges) const
    {
        ID_RANGE range = { 0xFFFFFFFF, 0xFFFFFFFF };

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

    typedef std::vector<DWORD> offsets_type;
    BOOL OffsetsFromRanges(offsets_type& offsets, const ranges_type& ranges)
    {
        DWORD offset = sizeof(ULONG);
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

//////////////////////////////////////////////////////////////////////////////
