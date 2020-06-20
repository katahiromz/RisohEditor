// IconRes.hpp --- Icon Resources
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

#pragma once

#ifndef _INC_WINDOWS
    #include <windows.h>
#endif
#include <cassert>
#include "MByteStreamEx.hpp"

//////////////////////////////////////////////////////////////////////////////

// the common header of RT_CURSOR, RT_ICON, icon/cursor files
typedef struct ICONDIR
{
    WORD           idReserved;  // Reserved (must be 0)
    WORD           idType;      // RES_ICON or RES_CURSOR
    WORD           idCount;     // How many images?
} ICONDIR, *LPICONDIR;

// the header of RT_CURSOR
typedef struct LOCALHEADER {
    WORD xHotSpot;
    WORD yHotSpot;
} LOCALHEADER;

// the entry of icon/cursor files (after ICONDIR)
typedef struct ICONDIRENTRY
{
    BYTE        bWidth;         // Width, in pixels, of the image
    BYTE        bHeight;        // Height, in pixels, of the image
    BYTE        bColorCount;    // Number of colors in image (0 if >= 8bpp)
    BYTE        bReserved;      // Reserved (must be 0)
    union
    {
        WORD    xHotSpot;       // Hot Spot X
        WORD    wPlanes;        // Color Planes
    };
    union
    {
        WORD    yHotSpot;       // Hot Spot Y
        WORD    wBitCount;      // Bits per pixel
    };
    DWORD       dwBytesInRes;   // How many bytes in this resource?
    DWORD       dwImageOffset;  // Where in the file is this image?
} ICONDIRENTRY, *LPICONDIRENTRY;

// the entry of group icon resource (RT_GROUP_ICON)
#include <pshpack2.h>
typedef struct GRPICONDIRENTRY
{
    BYTE   bWidth;              // Width, in pixels, of the image
    BYTE   bHeight;             // Height, in pixels, of the image
    BYTE   bColorCount;         // Number of colors in image (0 if >=8bpp)
    BYTE   bReserved;           // Reserved
    WORD   wPlanes;             // Color Planes
    WORD   wBitCount;           // Bits per pixel
    DWORD  dwBytesInRes;        // how many bytes in this resource?
    WORD   nID;                 // the ID
} GRPICONDIRENTRY, *LPGRPICONDIRENTRY;
#include <poppack.h>

// the entry of group cursor resource (RT_GROUP_CURSOR)
#include <pshpack2.h>
typedef struct GRPCURSORDIRENTRY
{
    WORD    wWidth;             // Width, in pixels, of the image
    WORD    wHeight;            // Height, in pixels, of the image
    WORD    wPlanes;            // Must be 1
    WORD    wBitCount;          // Bits per pixel
    DWORD   dwBytesInRes;       // how many bytes in this resource?
    WORD    nID;                // the ID
} GRPCURSORDIRENTRY, *LPGRPCURSORDIRENTRY;
#include <poppack.h>

//////////////////////////////////////////////////////////////////////////////

class IconFile
{
public:
    typedef ICONDIRENTRY            EntryType;
    typedef std::vector<EntryType>  EntryListType;
    typedef std::vector<BYTE>       DataType;
    typedef std::vector<DataType>   DataListType;
    typedef GRPICONDIRENTRY         ResourceEntryType;

    IconFile()
    {
        memset(&m_dir, 0, sizeof(m_dir));
    }

    void clear()
    {
        memset(&m_dir, 0, sizeof(m_dir));
        m_entries.clear();
        m_images.clear();
    }

    bool IsIconDirOK() const
    {
        return m_dir.idReserved == 0 && m_dir.idType == RES_ICON &&
               m_dir.idCount > 0;
    }

    WORD type() const
    {
        return m_dir.idType;
    }

    int SizeOfIconGroup() const
    {
        return sizeof(ICONDIR) + sizeof(ResourceEntryType) * GetImageCount();
    }

    WORD GetImageCount() const
    {
        return m_dir.idCount;
    }

    DataType& GetImage(int index)
    {
        assert(0 <= index && index < GetImageCount());
        return m_images[index];
    }
    const DataType& GetImage(int index) const
    {
        assert(0 <= index && index < GetImageCount());
        return m_images[index];
    }

    BYTE *GetImagePtr(int index)
    {
        return &(GetImage(index)[0]);
    }
    const BYTE *GetImagePtr(int index) const
    {
        return &(GetImage(index)[0]);
    }

    DWORD GetImageSize(int index) const
    {
        return DWORD(m_images[index].size());
    }

    bool LoadFromStream(const MByteStreamEx& stream);

    bool LoadFromFile(LPCTSTR pszFileName)
    {
        MByteStreamEx stream;
        if (!stream.LoadFromFile(pszFileName))
            return false;

        return LoadFromStream(stream);
    }

    bool SaveToStream(MByteStreamEx& stream);

    bool SaveToFile(LPCTSTR pszFileName)
    {
        MByteStreamEx stream;
        if (!SaveToStream(stream))
            return false;
        return stream.SaveToFile(pszFileName);
    }

    DataType GetIconGroup(int nBaseID) const;

protected:
    ICONDIR                     m_dir;
    EntryListType               m_entries;
    DataListType                m_images;
}; // class IconFile

//////////////////////////////////////////////////////////////////////////////

class CursorFile
{
public:
    typedef ICONDIRENTRY            EntryType;
    typedef std::vector<EntryType>  EntryListType;
    typedef std::vector<BYTE>       DataType;
    typedef std::vector<DataType>   DataListType;
    typedef GRPCURSORDIRENTRY       ResourceEntryType;

    CursorFile()
    {
        memset(&m_dir, 0, sizeof(m_dir));
    }

    void clear()
    {
        memset(&m_dir, 0, sizeof(m_dir));
        m_entries.clear();
        m_images.clear();
    }

    bool IsIconDirOK() const
    {
        return m_dir.idReserved == 0 && m_dir.idType == RES_CURSOR &&
               m_dir.idCount > 0;
    }

    WORD type() const
    {
        return m_dir.idType;
    }

    int SizeOfCursorGroup() const
    {
        return sizeof(ICONDIR) + sizeof(ResourceEntryType) * GetImageCount();
    }

    WORD GetImageCount() const
    {
        return m_dir.idCount;
    }

    DataType& GetImage(int index)
    {
        assert(0 <= index && index < GetImageCount());
        return m_images[index];
    }
    const DataType& GetImage(int index) const
    {
        assert(0 <= index && index < GetImageCount());
        return m_images[index];
    }

    BYTE *GetImagePtr(int index)
    {
        return &(GetImage(index)[0]);
    }
    const BYTE *GetImagePtr(int index) const
    {
        return &(GetImage(index)[0]);
    }

    DWORD GetImageSize(int index) const
    {
        return DWORD(m_images[index].size());
    }

    bool LoadFromStream(const MByteStreamEx& stream);

    bool LoadFromFile(LPCTSTR pszFileName)
    {
        MByteStreamEx stream;
        if (!stream.LoadFromFile(pszFileName))
            return false;

        return LoadFromStream(stream);
    }

    bool SaveToStream(MByteStreamEx& stream);

    bool SaveToFile(LPCTSTR pszFileName)
    {
        MByteStreamEx stream;
        if (!SaveToStream(stream))
            return false;
        return stream.SaveToFile(pszFileName);
    }

    DataType GetCursorGroup(int nBaseID) const;

protected:
    ICONDIR                     m_dir;
    EntryListType               m_entries;
    DataListType                m_images;
}; // class CursorFile
