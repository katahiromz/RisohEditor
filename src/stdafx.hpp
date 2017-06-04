#pragma once

#include "RisohEditor.hpp"

std::wstring str_vkey(WORD w);

#include "id_string.hpp"
#include "Text.hpp"
#include "ByteStream.hpp"

#include "Samples.hpp"

#include "AccelRes.hpp"
#include "IconRes.hpp"
#include "MenuRes.hpp"
#include "MessageRes.hpp"
#include "StringRes.hpp"
#include "DialogRes.hpp"
#include "VersionRes.hpp"

#include "ConstantsDB.hpp"
#include "PackedDIB.hpp"
#include "Png.hpp"
#include "Res.hpp"

#include "MFile.hpp"
#include "MProcessMaker.hpp"

#include "MTestMenuDlg.hpp"
#include "MTestDialog.hpp"

#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "uuid.lib")
