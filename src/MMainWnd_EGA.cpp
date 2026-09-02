// MMainWnd_EGA.cpp --- RisohEditor
//////////////////////////////////////////////////////////////////////////////
// RisohEditor --- Another free Win32 resource editor
// Copyright (C) 2017-2025 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
// License: GPL-3 or later

#include "MMainWnd.hpp"
#include <memory>
#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include "RisohEditor.hpp"
#include "MString.hpp"
#include "MIdOrString.hpp"
#include "ConstantsDB.hpp"
#include "Utils.h"
#include "MString.hpp"
#include "resource.h"

MMainWnd *s_pMainWnd = NULL;
INT g_bNoGuiMode = 0; // No-GUI mode

MIdOrString g_RES_select_type = BAD_TYPE;
MIdOrString g_RES_select_name = BAD_NAME;
LANGID g_RES_select_lang = BAD_LANG;

IMPLEMENT_DYNAMIC(MEgaDlg)

////////////////////////////////////////////////////////////////////////////

bool MMainWnd::DoResLoad(const MStringW& filename, const MStringW& options)
{
	bool bOK;
	++g_bNoGuiMode;
	BOOL bAutoLoadNearbyResH = g_settings.bAutoLoadNearbyResH;
	g_settings.bAutoLoadNearbyResH = options.find(L"(no-load-res-h)") == options.npos;
	{
		bOK = !!DoLoadFile(m_hwnd, filename.c_str(), 0, TRUE);
	}
	g_settings.bAutoLoadNearbyResH = bAutoLoadNearbyResH;
	--g_bNoGuiMode;
	return bOK;
}

bool MMainWnd::DoResSave(const MStringW& filename, const MStringW& options)
{
	bool bOK;
	++g_bNoGuiMode;
	BOOL bUseIDC_STATIC = g_settings.bUseIDC_STATIC;
	BOOL bAskUpdateResH = g_settings.bAskUpdateResH;
	BOOL bCompressByUPX = g_settings.bCompressByUPX;
	BOOL bSepFilesByLang = g_settings.bSepFilesByLang;
	BOOL bStoreToResFolder = g_settings.bStoreToResFolder;
	BOOL bSelectableByMacro = g_settings.bSelectableByMacro;
	BOOL bRedundantComments = g_settings.bRedundantComments;
	BOOL bWrapManifest = g_settings.bWrapManifest;
	BOOL bUseBeginEnd = g_settings.bUseBeginEnd;
	UINT nOldCodePageForRC = g_settings.nCodePageForRC;
	BOOL bAddBomToRC = g_settings.bAddBomToRC;
	BOOL bBackup = g_settings.bBackup;
	BOOL bUseMSMSGTABLE = g_settings.bUseMSMSGTABLE;
	g_settings.bUseIDC_STATIC = options.find(L"(idc-static)") != options.npos;
	g_settings.bAskUpdateResH = FALSE;
	g_settings.bCompressByUPX = options.find(L"(compress)") != options.npos;
	g_settings.bSepFilesByLang = options.find(L"(sep-lang)") != options.npos;
	g_settings.bStoreToResFolder = options.find(L"(no-res-folder)") == options.npos;
	g_settings.bSelectableByMacro = options.find(L"(lang-macro)") != options.npos;
	g_settings.bRedundantComments = options.find(L"(less-comments)") == options.npos;
	g_settings.bWrapManifest = options.find(L"(wrap-manifest)") != options.npos;
	g_settings.bUseBeginEnd = options.find(L"(begin-end)") != options.npos;
	g_settings.nCodePageForRC = (options.find(L"(utf-16)") != options.npos) ? _CP_UTF16 : CP_UTF8;
	g_settings.bAddBomToRC = options.find(L"(bom)") != options.npos;
	g_settings.bBackup = options.find(L"(backup)") != options.npos;
	g_settings.bUseMSMSGTABLE = options.find(L"(ms-msgtbl)") != options.npos;
	{
		bOK = !!DoSaveFile(m_hwnd, filename.c_str());
	}
	g_settings.bUseIDC_STATIC = bUseIDC_STATIC;
	g_settings.bAskUpdateResH = bAskUpdateResH;
	g_settings.bCompressByUPX = bCompressByUPX;
	g_settings.bSepFilesByLang = bSepFilesByLang;
	g_settings.bStoreToResFolder = bStoreToResFolder;
	g_settings.bSelectableByMacro = bSelectableByMacro;
	g_settings.bRedundantComments = bRedundantComments;
	g_settings.bWrapManifest = bWrapManifest;
	g_settings.bUseBeginEnd = bUseBeginEnd;
	g_settings.nCodePageForRC = nOldCodePageForRC;
	g_settings.bAddBomToRC = bAddBomToRC;
	g_settings.bBackup = bBackup;
	g_settings.bUseMSMSGTABLE = bUseMSMSGTABLE;

	--g_bNoGuiMode;
	return bOK;
}

EGA::arg_t EGA_FN EGA_RES_load(const EGA::args_t& args)
{
	return s_pMainWnd->RES_load(args);
}

EGA::arg_t EGA_FN EGA_RES_save(const EGA::args_t& args)
{
	return s_pMainWnd->RES_save(args);
}

EGA::arg_t EGA_FN EGA_RES_search(const EGA::args_t& args)
{
	return s_pMainWnd->RES_search(args);
}

EGA::arg_t EGA_FN EGA_RES_delete(const EGA::args_t& args)
{
	return s_pMainWnd->RES_delete(args);
}

EGA::arg_t EGA_FN EGA_RES_clone_by_name(const EGA::args_t& args)
{
	return s_pMainWnd->RES_clone_by_name(args);
}

EGA::arg_t EGA_FN EGA_RES_clone_by_lang(const EGA::args_t& args)
{
	return s_pMainWnd->RES_clone_by_lang(args);
}

EGA::arg_t EGA_FN EGA_RES_unload_resh(const EGA::args_t& args)
{
	return s_pMainWnd->RES_unload_resh(args);
}

EGA::arg_t EGA_FN EGA_RES_select(const EGA::args_t& args)
{
	return s_pMainWnd->RES_select(args);
}

EGA::arg_t EGA_FN EGA_RES_get_binary(const EGA::args_t& args)
{
	return s_pMainWnd->RES_get_binary(args);
}

EGA::arg_t EGA_FN EGA_RES_set_binary(const EGA::args_t& args)
{
	return s_pMainWnd->RES_set_binary(args);
}

EGA::arg_t EGA_FN EGA_RES_get_text(const EGA::args_t& args)
{
	return s_pMainWnd->RES_get_text(args[0], args[1], args[2]);
}

EGA::arg_t EGA_FN EGA_RES_set_text(const EGA::args_t& args)
{
	return s_pMainWnd->RES_set_text(args[0], args[1], args[2], args[3]);
}

EGA::arg_t EGA_FN EGA_RES_const(const EGA::args_t& args)
{
	return s_pMainWnd->RES_const(args);
}

EGA::arg_t EGA_FN EGA_RES_str_get(const EGA::args_t& args)
{
	if (args.size() == 1)
		return s_pMainWnd->RES_str_get(args[0]);
	else
		return s_pMainWnd->RES_str_get(args[0], args[1]);
}

EGA::arg_t EGA_FN EGA_RES_str_set(const EGA::args_t& args)
{
	if (args.size() == 2)
		return s_pMainWnd->RES_str_set(args[0], args[1]);
	else
		return s_pMainWnd->RES_str_set(args[0], args[1], args[2]);
}

EGA::arg_t EGA_FN EGA_RES_extract(const EGA::args_t& args)
{
	return s_pMainWnd->RES_extract(args);
}

MIdOrString EGA_get_id_or_str(const arg_t& arg0)
{
	MIdOrString ret;

	if (arg0->get_type() == AST_INT)
	{
		ret = (WORD)EGA_get_int(arg0);
	}
	else
	{
		std::string str = EGA_get_str(arg0);
		MAnsiToWide wide(CP_UTF8, str);
		ret = wide.c_str();
	}

	return ret;
}

EGA::arg_t EGA_set_id_or_str(const MIdOrString& id)
{
	if (id.is_str())
	{
		MWideToAnsi ansi(CP_UTF8, id.m_str);
		return EGA::make_arg<AstStr>(ansi.str());
	}
	else
	{
		return EGA::make_arg<AstInt>(id.m_id);
	}
}

EGA::arg_t MMainWnd::RES_load(const EGA::args_t& args)
{
	using namespace EGA;
	arg_t arg0, arg1;

	if (args.size() >= 1)
		arg0 = EGA_eval_arg(args[0], true);
	if (args.size() >= 2)
		arg1 = EGA_eval_arg(args[1], false);

	auto utf8_filename = EGA_get_str(arg0);
	if (!EgaBridge::FileSecurity0(utf8_filename, "RES_load"))
	{
		EgaBridge::HitSecurity();
		return make_arg<AstInt>(0);
	}

	MAnsiToWide str0(CP_UTF8, utf8_filename);
	MStringW filename = str0.c_str();
	MStringW options;
	if (args.size() >= 2)
	{
		MAnsiToWide str1(CP_UTF8, EGA_get_str(arg1));
		options = str1.c_str();
	}

	// NOTE: ret must NOT be captured by reference. RunOnUIThread can time
	// out (500ms) and return false while the task is still sitting in
	// the UI queue; when the UI thread eventually runs it, it must not
	// write into a stack frame that RES_load has already unwound past
	// (via the EGA_control_break thrown below). Keeping ret on the heap
	// via shared_ptr, captured by value, keeps the write target valid
	// for as long as the queued task might outlive this function.
	auto ret = std::make_shared<bool>(false);
	bool completed = EgaBridge::RunOnUIThread([this, ret, filename, options](void*)
	{
		*ret = DoResLoad(filename, options);
	});
	if (!completed)
		throw EGA_control_break(0);

	return make_arg<AstInt>(*ret);
}

EGA::arg_t MMainWnd::RES_save(const EGA::args_t& args)
{
	using namespace EGA;
	arg_t arg0, arg1;

	if (args.size() >= 1)
		arg0 = EGA_eval_arg(args[0], true);
	if (args.size() >= 2)
		arg1 = EGA_eval_arg(args[1], false);

	auto utf8_filename = EGA_get_str(arg0);
	if (!EgaBridge::FileSecurity1(utf8_filename, "RES_save"))
	{
		EgaBridge::HitSecurity();
		return make_arg<AstInt>(0);
	}

	MAnsiToWide str0(CP_UTF8, utf8_filename.c_str());
	MStringW filename = str0.c_str();
	MStringW options;
	if (args.size() >= 2)
	{
		MAnsiToWide str1(CP_UTF8, EGA_get_str(arg1));
		options = str1.c_str();
	}

	// See the comment in RES_load: ret must live on the heap (not the
	// stack) because the queued UI task may still run after this
	// function has thrown EGA_control_break and unwound.
	auto ret = std::make_shared<bool>(false);
	bool completed = EgaBridge::RunOnUIThread([this, ret, filename, options](void*)
	{
		*ret = DoResSave(filename, options);
	});
	if (!completed)
		throw EGA_control_break(0);

	return make_arg<AstInt>(*ret);
}

EGA::arg_t MMainWnd::RES_unload_resh(const EGA::args_t& args)
{
	using namespace EGA;

	bool completed = EgaBridge::RunOnUIThread([this](void*)
	{
		UnloadResourceH(m_hwnd);
		DoSetFileModified(TRUE);
	});
	if (!completed)
		throw EGA_control_break(0);

	return make_arg<AstInt>(1);
}

EGA::arg_t MMainWnd::RES_search(const EGA::args_t& args)
{
	using namespace EGA;
	arg_t arg0, arg1, arg2;

	if (args.size() >= 1)
		arg0 = EGA_eval_arg(args[0], false);
	if (args.size() >= 2)
		arg1 = EGA_eval_arg(args[1], false);
	if (args.size() >= 3)
		arg2 = EGA_eval_arg(args[2], false);

	MIdOrString type = BAD_TYPE, name = BAD_NAME;
	LANGID lang = BAD_LANG;

	if (arg0)
		type = EGA_get_id_or_str(arg0);
	if (arg1)
		name = EGA_get_id_or_str(arg1);
	if (arg2)
		lang = (WORD)EGA_get_int(arg2);

	struct FoundItem
	{
		MIdOrString type;
		MIdOrString name;
		LANGID      lang;
	};
	// See the comment in RES_load: output must be captured by value via
	// shared_ptr, not by reference, since a queued UI task can still run
	// after this function has thrown EGA_control_break and unwound.
	auto items = std::make_shared<std::vector<FoundItem>>();

	bool completed = EgaBridge::RunOnUIThread([this, type, name, lang, items](void*)
	{
		EntrySet found;
		g_res.search(found, ET_LANG, type, name, lang);

		items->reserve(found.size());
		for (auto& item : found)
		{
			items->push_back({ item->m_type, item->m_name, item->m_lang });
		}
	});
	if (!completed)
		throw EGA_control_break(0);

	auto array = make_arg<AstContainer>(AST_ARRAY, 0, "RES_LIST");
	for (auto& item : *items)
	{
		auto child = make_arg<AstContainer>(AST_ARRAY, 0, "RES");
		child->add(EGA_set_id_or_str(item.type));
		child->add(EGA_set_id_or_str(item.name));
		child->add(EGA::make_arg<AstInt>(item.lang));
		array->add(child);
	}
	return array;
}

EGA::arg_t MMainWnd::RES_delete(const EGA::args_t& args)
{
    using namespace EGA;
    arg_t arg0, arg1, arg2;

    if (args.size() >= 1)
        arg0 = EGA_eval_arg(args[0], false);
    if (args.size() >= 2)
        arg1 = EGA_eval_arg(args[1], false);
    if (args.size() >= 3)
        arg2 = EGA_eval_arg(args[2], false);

    MIdOrString type = BAD_TYPE, name = BAD_NAME;
    LANGID lang = BAD_LANG;

    if (arg0)
        type = EGA_get_id_or_str(arg0);
    if (arg1)
        name = EGA_get_id_or_str(arg1);
    if (arg2)
        lang = (WORD)EGA_get_int(arg2);

    // See the comment in RES_load: ret must be captured by value via
    // shared_ptr, not by reference, since a queued UI task can still run
    // after this function has thrown EGA_control_break and unwound.
    auto ret = std::make_shared<bool>(false);

    bool completed = EgaBridge::RunOnUIThread([this, type, name, lang, ret](void*)
    {
        EntrySet found;
        *ret = g_res.search(found, ET_ANY, type, name, lang);
        // Use delete_entry() (not a bare mark_invalid()) for each matched
        // entry. delete_entry() recursively marks a parent invalid too
        // once it has become childless, so deleting an ET_LANG entry here
        // also cascades to remove its now-childless ET_NAME parent, and
        // that ET_NAME becoming empty in turn cascades to remove a
        // now-childless ET_TYPE grandparent. A plain mark_invalid() loop
        // followed by a single delete_invalid() call only catches one
        // level of "just became childless" per call (delete_invalid()'s
        // search_invalid() makes just one pass over the set), so an
        // ET_LANG -> ET_NAME -> ET_TYPE cascade would leave the emptied
        // ET_NAME/ET_TYPE nodes stranded in the treeview.
        for (auto entry : found)
            g_res.delete_entry(entry);
        g_res.delete_invalid();

        if (*ret)
        {
            DoSetFileModified(TRUE);
            if (g_res.empty())
                HidePreview(STV_RESETTEXTANDMODIFIED);
        }
    });
    if (!completed)
        throw EGA_control_break(0);

    return make_arg<AstInt>(*ret);
}

EGA::arg_t MMainWnd::RES_clone_by_name(const EGA::args_t& args)
{
	using namespace EGA;
	arg_t arg0, arg1, arg2;

	if (args.size() >= 1)
		arg0 = EGA_eval_arg(args[0], false);
	if (args.size() >= 2)
		arg1 = EGA_eval_arg(args[1], false);
	if (args.size() >= 3)
		arg2 = EGA_eval_arg(args[2], false);

	MIdOrString type = BAD_TYPE, src_name = BAD_NAME, dest_name;
	LANGID lang = BAD_LANG;

	if (arg0)
		type = EGA_get_id_or_str(arg0);
	if (arg1)
		src_name = EGA_get_id_or_str(arg1);
	if (arg2)
		dest_name = EGA_get_id_or_str(arg2);

	// See the comment in RES_load: ret must be captured by value via
	// shared_ptr, not by reference, since a queued UI task can still run
	// after this function has thrown EGA_control_break and unwound.
	auto ret = std::make_shared<bool>(false);
	bool completed = EgaBridge::RunOnUIThread([this, type, src_name, dest_name, lang, ret](void*)
	{
		EntrySet found;
		g_res.search(found, ET_LANG, type, src_name, lang);

		if (type == RT_GROUP_ICON)     // group icon
		{
			for (auto e : found)
			{
				g_res.copy_group_icon(e, dest_name, e->m_lang);
			}
		}
		else if (type == RT_GROUP_CURSOR)  // group cursor
		{
			for (auto e : found)
			{
				g_res.copy_group_cursor(e, dest_name, e->m_lang);
			}
		}
		else    // otherwise
		{
			for (auto e : found)
			{
				g_res.add_lang_entry(e->m_type, dest_name, e->m_lang, e->m_data);
			}
		}

		g_res.delete_invalid();

		if (!found.empty())
			DoSetFileModified(TRUE);

		*ret = !found.empty();
	});
	if (!completed)
		throw EGA_control_break(0);

	return make_arg<AstInt>(*ret);
}

EGA::arg_t MMainWnd::RES_clone_by_lang(const EGA::args_t& args)
{
	using namespace EGA;
	arg_t arg0, arg1, arg2, arg3;

	if (args.size() >= 1)
		arg0 = EGA_eval_arg(args[0], false);
	if (args.size() >= 2)
		arg1 = EGA_eval_arg(args[1], false);
	if (args.size() >= 3)
		arg2 = EGA_eval_arg(args[2], false);
	if (args.size() >= 4)
		arg3 = EGA_eval_arg(args[3], false);

	MIdOrString type = BAD_TYPE, name = BAD_NAME;
	LANGID src_lang = BAD_LANG, dest_lang = BAD_LANG;

	if (arg0)
		type = EGA_get_id_or_str(arg0);
	if (arg1)
		name = EGA_get_id_or_str(arg1);
	if (arg2)
		src_lang = (WORD)EGA_get_int(arg2);
	if (arg3)
		dest_lang = (WORD)EGA_get_int(arg3);

	// See the comment in RES_load: ret must be captured by value via
	// shared_ptr, not by reference, since a queued UI task can still run
	// after this function has thrown EGA_control_break and unwound.
	auto ret = std::make_shared<bool>(false);
	bool completed = EgaBridge::RunOnUIThread([this, type, name, src_lang, dest_lang, ret](void*)
	{
		EntrySet found2;
		g_res.search(found2, ET_LANG, type, name, src_lang);

		for (auto& entry : found2)
		{
			if (entry->m_type == RT_GROUP_ICON)     // group icon
			{
				// search the group icons
				EntrySet found;
				g_res.search(found, ET_LANG, RT_GROUP_ICON, name, src_lang);

				// copy them
				for (auto e : found)
				{
					g_res.copy_group_icon(e, e->m_name, dest_lang);
				}
			}
			else if (entry->m_type == RT_GROUP_CURSOR)
			{
				// search the group cursors
				EntrySet found;
				g_res.search(found, ET_LANG, RT_GROUP_CURSOR, name, src_lang);

				// copy them
				for (auto e : found)
				{
					g_res.copy_group_cursor(e, e->m_name, dest_lang);
				}
			}
			else if (entry->m_et == ET_STRING)
			{
				// search the strings
				EntrySet found;
				g_res.search(found, ET_LANG, RT_STRING, BAD_NAME, src_lang);

				// copy them
				for (auto e : found)
				{
					g_res.add_lang_entry(e->m_type, e->m_name, dest_lang, e->m_data);
				}
			}
			else
			{
				// search the entries
				EntrySet found;
				g_res.search(found, ET_LANG, entry->m_type, entry->m_name, entry->m_lang);

				// copy them
				for (auto e : found)
				{
					g_res.add_lang_entry(e->m_type, e->m_name, dest_lang, e->m_data);
				}
			}
		}

		g_res.delete_invalid();

		if (!found2.empty())
			DoSetFileModified(TRUE);

		*ret = !found2.empty();
	});
	if (!completed)
		throw EGA_control_break(0);

	return make_arg<AstInt>(*ret);
}

EGA::arg_t MMainWnd::RES_const(const EGA::args_t& args)
{
	using namespace EGA;
	arg_t arg0 = EGA_eval_arg(args[0], true);
	std::string name = EGA_get_str(arg0);
	MAnsiToWide a2w(CP_ACP, name);
	std::wstring wname = a2w.c_str();

	// See the comment in RES_load: value must be captured by value via
	// shared_ptr, not by reference, since a queued UI task can still run
	// after this function has thrown EGA_control_break and unwound.
	auto value = std::make_shared<INT>(0);
	bool completed = EgaBridge::RunOnUIThread([this, name, wname, value](void*)
	{
		ConstantsDB::ValueType dbValue;
		BOOL bOK = g_db.GetValueOfName(wname.c_str(), dbValue);
		if (!bOK)
		{
			for (auto& pair : g_settings.id_map)
			{
				if (name == pair.first)
				{
					*value = strtol(pair.second.c_str(), NULL, 0);
					bOK = TRUE;
					break;
				}
			}
		}
		else
		{
			*value = dbValue;
		}
	});
	if (!completed)
		throw EGA_control_break(0);

	return make_arg<AstInt>(*value);
}

EGA::arg_t MMainWnd::RES_set_binary(const EGA::args_t& args)
{
	using namespace EGA;
	arg_t arg0, arg1, arg2, arg3;

	arg0 = EGA_eval_arg(args[0], true);
	arg1 = EGA_eval_arg(args[1], true);
	arg2 = EGA_eval_arg(args[2], true);
	arg3 = EGA_eval_arg(args[3], true);

	MIdOrString type, name;
	std::string contents;
	LANGID lang;

	type = EGA_get_id_or_str(arg0);
	name = EGA_get_id_or_str(arg1);
	lang = EGA_get_int(arg2);
	contents = EGA_get_str(arg3);
	if (type.empty() || name.empty() || lang == BAD_LANG || contents.empty())
		return make_arg<AstInt>(0);

	EntryBase::data_type data(contents.begin(), contents.end());

	// See the comment in RES_load: ret must be captured by value via
	// shared_ptr, not by reference, since a queued UI task can still run
	// after this function has thrown EGA_control_break and unwound.
	auto ret = std::make_shared<int>(0);

	bool completed = EgaBridge::RunOnUIThread([this, type, name, lang, data, ret](void*)
	{
		if (g_res.add_lang_entry(type, name, lang, data))
			*ret = 1;

		DoSetFileModified(TRUE);
	});
	if (!completed)
		throw EGA_control_break(0);

	return make_arg<AstInt>(*ret);
}

EGA::arg_t MMainWnd::RES_get_binary(const EGA::args_t& args)
{
	using namespace EGA;
	arg_t arg0, arg1, arg2;

	if (args.size() >= 1)
		arg0 = EGA_eval_arg(args[0], false);
	if (args.size() >= 2)
		arg1 = EGA_eval_arg(args[1], false);
	if (args.size() >= 3)
		arg2 = EGA_eval_arg(args[2], false);

	MIdOrString type = BAD_TYPE, name = BAD_NAME;
	LANGID lang = BAD_LANG;

	if (arg0)
		type = EGA_get_id_or_str(arg0);
	if (arg1)
		name = EGA_get_id_or_str(arg1);
	if (arg2)
		lang = (WORD)EGA_get_int(arg2);

	// See the comment in RES_load: ret must be captured by value via
	// shared_ptr, not by reference, since a queued UI task can still run
	// after this function has thrown EGA_control_break and unwound.
	auto ret = std::make_shared<std::string>();
	bool completed = EgaBridge::RunOnUIThread([this, type, name, lang, ret](void*)
	{
		EntrySet found;
		g_res.search(found, ET_LANG, type, name, lang);

		if (found.size())
		{
			for (auto e : found)
			{
				ret->resize(e->size());
				memcpy(&(*ret)[0], &e->m_data[0], e->size());
				break;
			}
		}
	});
	if (!completed)
		throw EGA_control_break(0);

	return make_arg<AstStr>(*ret);
}

EGA::arg_t MMainWnd::RES_get_text(EGA::arg_t arg0, EGA::arg_t arg1, EGA::arg_t arg2)
{
	using namespace EGA;

	arg0 = EGA_eval_arg(arg0, true);
	arg1 = EGA_eval_arg(arg1, true);
	arg2 = EGA_eval_arg(arg2, true);

	MIdOrString type = EGA_get_id_or_str(arg0);
	MIdOrString name = EGA_get_id_or_str(arg1);
	LANGID lang = static_cast<LANGID>(EGA_get_int(arg2));

	// See the comment in RES_load: ret must be captured by value via
	// shared_ptr, not by reference, since a queued UI task can still run
	// after this function has thrown EGA_control_break and unwound.
	auto ret = std::make_shared<std::wstring>();
	bool completed = EgaBridge::RunOnUIThread([this, type, name, lang, ret](void*)
	{
		auto bHideID = g_settings.bHideID; // Save old value
		g_settings.bHideID = TRUE;

		auto entry = g_res.find(ET_LANG, type, name, lang);
		if (entry)
		{
			ResToText res2text;
			*ret = res2text.DumpEntry(*entry);
		}

		g_settings.bHideID = bHideID;
	});
	if (!completed)
		throw EGA_control_break(0);

	MWideToAnsi ansi(CP_UTF8, *ret);
	return make_arg<AstStr>(ansi.str());
}

EGA::arg_t MMainWnd::RES_set_text(EGA::arg_t arg0, EGA::arg_t arg1, EGA::arg_t arg2, EGA::arg_t arg3)
{
	using namespace EGA;

	arg0 = EGA_eval_arg(arg0, true);
	arg1 = EGA_eval_arg(arg1, true);
	arg2 = EGA_eval_arg(arg2, true);
	arg3 = EGA_eval_arg(arg3, true);

	MIdOrString type = EGA_get_id_or_str(arg0);
	MIdOrString name = EGA_get_id_or_str(arg1);
	LANGID lang = static_cast<LANGID>(EGA_get_int(arg2));
	auto ansi = EGA_get_str(arg3);
	MAnsiToWide wide(CP_UTF8, ansi);

	std::wstring wtext = wide.str();

	// See the comment in RES_load: ret must be captured by value via
	// shared_ptr, not by reference, since a queued UI task can still run
	// after this function has thrown EGA_control_break and unwound.
	auto ret = std::make_shared<BOOL>(FALSE);
	bool completed = EgaBridge::RunOnUIThread([this, type, name, lang, wtext, ret](void*)
	{
		MStringA strOutput;
		++g_bNoGuiMode;
		*ret = CompileParts(strOutput, type, name, lang, wtext, FALSE);
		--g_bNoGuiMode;

		DoSetFileModified(TRUE);
	});
	if (!completed)
		throw EGA_control_break(0);

	return make_arg<AstInt>(*ret);
}

EGA::arg_t MMainWnd::RES_str_get(EGA::arg_t arg0)
{
	using namespace EGA;

	arg0 = EGA_eval_arg(arg0, true);
	LANGID lang = static_cast<LANGID>(EGA_get_int(arg0));

	// See the comment in RES_load: output must be captured by value via
	// shared_ptr, not by reference, since a queued UI task can still run
	// after this function has thrown EGA_control_break and unwound.
	auto ok = std::make_shared<bool>(false);
	auto strMap = std::make_shared<std::map<WORD, std::wstring>>();
	bool completed = EgaBridge::RunOnUIThread([this, lang, ok, strMap](void*)
	{
		EntrySet found;
		g_res.search(found, ET_LANG, RT_STRING, BAD_NAME, lang);
		if (found.empty())
			return;

		// found --> str_res
		StringRes str_res;
		for (auto e : found)
		{
			MByteStreamEx stream(e->m_data);
			if (!str_res.LoadFromStream(stream, e->m_name.m_id))
				return;
		}

		*strMap = str_res.map();
		*ok = true;
	});
	if (!completed)
		throw EGA_control_break(0);

	if (!*ok)
		return make_arg<AstContainer>(); // error

	auto array1 = make_arg<AstContainer>();
	for (auto& pair : *strMap)
	{
		MWideToAnsi ansi(CP_UTF8, pair.second);

		auto array2 = make_arg<AstContainer>();
		array2->add(make_arg<AstInt>(pair.first));
		array2->add(make_arg<AstStr>(ansi.str()));

		array1->add(array2);
	}

	return array1;
}

EGA::arg_t MMainWnd::RES_str_get(EGA::arg_t arg0, EGA::arg_t arg1)
{
	using namespace EGA;

	arg0 = EGA_eval_arg(arg0, true);
	arg1 = EGA_eval_arg(arg1, true);
	LANGID lang = static_cast<LANGID>(EGA_get_int(arg0));
	int index = EGA_get_int(arg1);
	if (index < 0)
		return make_arg<AstStr>();

	// See the comment in RES_load: output must be captured by value via
	// shared_ptr, not by reference, since a queued UI task can still run
	// after this function has thrown EGA_control_break and unwound.
	auto ok = std::make_shared<bool>(false);
	auto value = std::make_shared<std::wstring>();
	bool completed = EgaBridge::RunOnUIThread([this, lang, index, ok, value](void*)
	{
		EntrySet found;
		g_res.search(found, ET_LANG, RT_STRING, BAD_NAME, lang);
		if (found.empty())
			return;

		// found --> str_res
		StringRes str_res;
		for (auto e : found)
		{
			MByteStreamEx stream(e->m_data);
			if (!str_res.LoadFromStream(stream, e->m_name.m_id))
				return;
		}

		for (auto& pair : str_res.map())
		{
			if (pair.first == static_cast<WORD>(index))
			{
				*value = pair.second;
				*ok = true;
				return;
			}
		}
	});
	if (!completed)
		throw EGA_control_break(0);

	if (!*ok)
		return make_arg<AstStr>(); // error

	MWideToAnsi ansi(CP_UTF8, *value);
	return make_arg<AstStr>(ansi.str());
}

EGA::arg_t MMainWnd::RES_str_set(EGA::arg_t arg0, EGA::arg_t arg1)
{
	arg0 = EGA_eval_arg(arg0, true);
	arg1 = EGA_eval_arg(arg1, true);

	LANGID lang = static_cast<LANGID>(EGA_get_int(arg0));
	auto array1 = EGA_get_array(arg1);

	StringRes str_res;
	for (size_t i = 0; i < array1->size(); ++i)
	{
		auto ary2 = (*array1)[i];
		auto& array2 = *EGA_get_array(ary2);
		if (array2.size() != 2)
			throw EGA_index_out_of_range(array2.get_lineno());

		WORD str_id = static_cast<WORD>(EGA_get_int(array2[0]));
		auto str = EGA_get_str(array2[1]);

		MAnsiToWide wide(CP_UTF8, str);
		str_res.map()[str_id] = wide.c_str();
	}

	// See the comment in RES_load: str_res and ok must be captured by
	// value (str_res via copy, ok via shared_ptr), not by reference,
	// since a queued UI task can still run after this function has
	// thrown EGA_control_break and unwound.
	auto ok = std::make_shared<bool>(true);
	bool completed = EgaBridge::RunOnUIThread([this, str_res, lang, ok](void*) mutable
	{
		EntrySet found;
		g_res.search(found, ET_ANY, RT_STRING, BAD_NAME, lang);
		for (auto entry : found)
		{
			entry->mark_invalid();
		}

		std::set<WORD> names;
		for (auto& pair : str_res.map())
		{
			auto str_id = pair.first;
			WORD name = str_res.NameFromId(str_id);
			if (names.count(name) > 0)
				continue;
			names.insert(name);

			if (str_res.HasAnyValues(name))
			{
				MByteStreamEx stream;
				str_res.SaveToStream(stream, name);

				if (!g_res.add_lang_entry(RT_STRING, name, lang, stream.data()))
				{
					*ok = false;
					return;
				}
			}
		}

		DoSetFileModified(TRUE);

		g_res.delete_invalid();
		if (g_res.empty())
			HidePreview(STV_RESETTEXTANDMODIFIED);
	});
	if (!completed)
		throw EGA_control_break(0);

	return make_arg<AstInt>(*ok ? 1 : 0);
}

EGA::arg_t MMainWnd::RES_str_set(EGA::arg_t arg0, EGA::arg_t arg1, EGA::arg_t arg2)
{
	arg0 = EGA_eval_arg(arg0, true);
	arg1 = EGA_eval_arg(arg1, true);
	arg2 = EGA_eval_arg(arg2, true);

	LANGID lang = static_cast<LANGID>(EGA_get_int(arg0));
	WORD str_id = static_cast<WORD>(EGA_get_int(arg1));

	std::string str = EGA_get_str(arg2);
	MAnsiToWide wide(CP_UTF8, str);
	std::wstring wtext = wide.c_str();

	// See the comment in RES_load: output must be captured by value
	// (wtext as a std::wstring copy, result via shared_ptr), not by
	// reference, since a queued UI task can still run after this
	// function has thrown EGA_control_break and unwound.
	auto result = std::make_shared<int>(0);

	bool completed = EgaBridge::RunOnUIThread([this, lang, str_id, wtext, result](void*)
	{
		EntrySet found;
		g_res.search(found, ET_LANG, RT_STRING, BAD_NAME, lang);

		// found --> str_res
		StringRes str_res;
		for (auto e : found)
		{
			MByteStreamEx stream(e->m_data);
			if (!str_res.LoadFromStream(stream, e->m_name.m_id))
				return;
		}

		str_res.map()[str_id] = wtext;

		WORD name = str_res.NameFromId(str_id);

		if (str_res.HasAnyValues(name))
		{
			MByteStreamEx stream;
			str_res.SaveToStream(stream, name);

			if (!g_res.add_lang_entry(RT_STRING, name, lang, stream.data()))
				return;
		}
		else
		{
			EntrySet found2;
			g_res.search(found2, ET_ANY, RT_STRING, name, lang);
			for (auto entry : found2)
				entry->mark_invalid();
		}

		DoSetFileModified(TRUE);

		g_res.delete_invalid();
		if (g_res.empty())
			HidePreview(STV_RESETTEXTANDMODIFIED);

		*result = 1; // success
	});
	if (!completed)
		throw EGA_control_break(0);

	return make_arg<AstInt>(*result);
}

EGA::arg_t MMainWnd::RES_select(const EGA::args_t& args)
{
	using namespace EGA;
	arg_t arg0, arg1, arg2;

	if (args.size() >= 1)
		arg0 = EGA_eval_arg(args[0], false);
	if (args.size() >= 2)
		arg1 = EGA_eval_arg(args[1], false);
	if (args.size() >= 3)
		arg2 = EGA_eval_arg(args[2], false);

	MIdOrString type = BAD_TYPE, name = BAD_NAME;
	LANGID lang = BAD_LANG;

	if (arg0)
		type = EGA_get_id_or_str(arg0);
	if (arg1)
		name = EGA_get_id_or_str(arg1);
	if (arg2)
		lang = (WORD)EGA_get_int(arg2);

	// See the comment in RES_load: found_any must be captured by value
	// via shared_ptr, not by reference, since a queued UI task can
	// still run after this function has thrown EGA_control_break and
	// unwound.
	auto found_any = std::make_shared<bool>(false);
	bool completed = EgaBridge::RunOnUIThread([this, type, name, lang, found_any](void*)
	{
		EntrySet found;
		g_res.search(found, ET_LANG, type, name, lang);

		*found_any = !found.empty();
		if (*found_any)
		{
			g_RES_select_type = type;
			g_RES_select_name = name;
			g_RES_select_lang = lang;
		}
	});
	if (!completed)
		throw EGA_control_break(0);

	return make_arg<AstInt>(*found_any);
}

void MMainWnd::OnStartEgaConsole(HWND hwnd, PCWSTR file)
{
	// compile if necessary
	if (!CompileIfNecessary(TRUE))
		return;

	if (s_hwndEga && ::IsWindow(s_hwndEga)) // Already open?
	{
		::ShowWindow(s_hwndEga, SW_SHOWNORMAL);
		::SetForegroundWindow(s_hwndEga);
		auto pDialog = dynamic_cast<MEgaDlg*>(MDialogBase::GetUserData(s_hwndEga));
		assert(pDialog);
		if (pDialog && file && file[0])
			pDialog->ExecuteEgaFile(file);
		return;
	}

	auto pDialog = dynamic_cast<MEgaDlg*>(MEgaDlg::CreateInstanceDx());
	assert(pDialog);
	pDialog->CreateDialogDx(hwnd);
	::ShowWindow(*pDialog, SW_SHOWNORMAL);
	if (file && file[0])
		pDialog->ExecuteEgaFile(file);
}

void MMainWnd::OnAskEgaProgramAndExecute(HWND hwnd)
{
	// compile if necessary
	if (!CompileIfNecessary(TRUE))
		return;

	WCHAR szDir[MAX_PATH];
	GetModuleFileNameW(NULL, szDir, ARRAYSIZE(szDir));
	PathRemoveFileSpecW(szDir);

	// Find EGA folder and build a filter
	WCHAR szFile[MAX_PATH] = L"";
	StringCbCopyW(szFile, sizeof(szFile), szDir);
	PathAppendW(szFile, L"EGA");
	if (!PathIsDirectoryW(szFile))
	{
		StringCbCopyW(szFile, sizeof(szFile), szDir);
		PathAppendW(szFile, L"..\\EGA-samples");
		if (!PathIsDirectoryW(szFile))
		{
			StringCbCopyW(szFile, sizeof(szFile), szDir);
			PathAppendW(szFile, L"..\\..\\EGA-samples");
			if (!PathIsDirectoryW(szFile))
			{
				StringCbCopyW(szFile, sizeof(szFile), szDir);
				PathAppendW(szFile, L"..\\..\\..\\EGA-samples");
				if (!PathIsDirectoryW(szFile))
				{
					return;
				}
			}
		}
	}
	PathAppendW(szFile, L"*.ega");

	// Ask EGA program
	OPENFILENAMEW ofn = { OPENFILENAME_SIZE_VERSION_400W, hwnd };
	ofn.hwndOwner = hwnd;
	ofn.lpstrFilter = MakeFilterDx(LoadStringDx(IDS_EGAFILTER));
	ofn.lpstrFile = szFile;
	ofn.nMaxFile = ARRAYSIZE(szFile);
	ofn.lpstrTitle = LoadStringDx(IDS_LOADEGAPROGRAM);
	ofn.lpstrDefExt = L"ega";
	if (GetOpenFileNameW(&ofn))
	{
		// Start EGA console and execute
		OnStartEgaConsole(hwnd, szFile);
	}
}

MStringA MMainWnd::ExtractEntry(EntryBase *entry, PCSTR u8_filename)
{
	if (!entry)
		return "";

	MStringA fname;
	if (u8_filename && u8_filename[0])
	{
		fname = u8_filename;
	}
	else
	{
		ResToText res2text;
		MStringW wide_filename;
		res2text.GetEntryFileNameEx(*entry, wide_filename);
		if (wide_filename.empty())
		{
			if (entry->m_lang)
			{
				wide_filename += std::to_wstring(entry->m_lang);
				wide_filename += L"_";
			}
			wide_filename += entry->m_type.str(true);
			wide_filename += L"_";
			wide_filename += entry->m_name.str(true);
			wide_filename += L".bin";
		}
		MWideToAnsi utf8(CP_UTF8, wide_filename.c_str());
		fname = utf8.c_str();
	}

	if (!EgaBridge::FileSecurity1(fname, "RES_extract"))
	{
		EgaBridge::HitSecurity();
		return "";
	}

	MAnsiToWide wide(CP_UTF8, fname.c_str());
	MStringW wide_fname = wide.c_str();

	BOOL ret = FALSE;
	switch (entry->m_et)
	{
	case ET_TYPE:
	case ET_NAME:
	case ET_STRING:
		ret = !!g_res.extract_bin(wide_fname.c_str(), entry);
		break;

	case ET_LANG:
		if (entry->m_type == RT_ICON || entry->m_type == RT_GROUP_ICON ||
			entry->m_type == RT_ANIICON)
		{
			ret = g_res.extract_icon(wide_fname.c_str(), entry);
		}
		else if (entry->m_type == RT_CURSOR || entry->m_type == RT_GROUP_CURSOR ||
				 entry->m_type == RT_ANICURSOR)
		{
			ret = g_res.extract_cursor(wide_fname.c_str(), entry);
		}
		else
		{
			ret = g_res.extract_bin(wide_fname.c_str(), entry);
		}
	}

	if (!ret)
		return "";

	return fname;
}

EGA::arg_t MMainWnd::RES_extract(const EGA::args_t& args)
{
	using namespace EGA;
	arg_t arg0, arg1, arg2, arg3;

	arg0 = EGA_eval_arg(args[0], true);
	arg1 = EGA_eval_arg(args[1], true);
	arg2 = EGA_eval_arg(args[2], true);
	if (args.size() == 4)
		arg3 = EGA_eval_arg(args[3], true);

	MIdOrString type = BAD_TYPE, name = BAD_NAME;
	LANGID lang = BAD_LANG;

	type = EGA_get_id_or_str(arg0);
	name = EGA_get_id_or_str(arg1);
	lang = (WORD)EGA_get_int(arg2);

	std::string filename;
	if (args.size() == 4)
		filename = EGA_get_str(arg3);

	// See the comment in RES_load: fname must be captured by value via
	// shared_ptr, not by reference, since a queued UI task can still run
	// after this function has thrown EGA_control_break and unwound.
	auto fname = std::make_shared<MStringA>(); // result
	bool completed = EgaBridge::RunOnUIThread([this, type, name, lang, filename, fname](void*)
	{
		auto* entry = g_res.find(ET_LANG, type, name, lang);
		if (!entry)
			return;

		if (filename.empty())
			*fname = ExtractEntry(entry, nullptr);
		else
			*fname = ExtractEntry(entry, filename.c_str());
	});
	if (!completed)
		throw EGA_control_break(0);

	if (fname->size())
		return make_arg<AstStr>(fname->c_str());

	return make_arg<AstInt>(0);
}

////////////////////////////////////////////////////////////////////////////

void EGA_extension(void)
{
	EGA_add_fn("RES_clone_by_lang", 4, 4, EGA_RES_clone_by_lang, "RES_clone_by_lang(type, name, src_lang, dest_lang)");
	EGA_add_fn("RES_clone_by_name", 3, 3, EGA_RES_clone_by_name, "RES_clone_by_name(type, src_name, dest_name)");
	EGA_add_fn("RES_const", 1, 1, EGA_RES_const, "RES_const(name)");
	EGA_add_fn("RES_delete", 0, 3, EGA_RES_delete, "RES_delete([type[, name[, lang]]])");
	EGA_add_fn("RES_extract", 3, 4, EGA_RES_extract, "RES_extract(type, name, lang[, filename])");
	EGA_add_fn("RES_get_binary", 0, 3, EGA_RES_get_binary, "RES_get_binary([type[, name[, lang]]])");
	EGA_add_fn("RES_get_text", 3, 3, EGA_RES_get_text, "RES_get_text(type, name, lang)");
	EGA_add_fn("RES_load", 1, 2, EGA_RES_load, "RES_load(filename[, options])");
	EGA_add_fn("RES_save", 1, 2, EGA_RES_save, "RES_save(filename[, options])");
	EGA_add_fn("RES_search", 0, 3, EGA_RES_search, "RES_search([type[, name[, lang]]])");
	EGA_add_fn("RES_select", 0, 3, EGA_RES_select, "RES_select([type[, name[, lang]]])");
	EGA_add_fn("RES_set_binary", 4, 4, EGA_RES_set_binary, "RES_set_binary(type, name, lang, bin)");
	EGA_add_fn("RES_set_text", 4, 4, EGA_RES_set_text, "RES_set_text(type, name, lang, text)");
	EGA_add_fn("RES_str_get", 1, 2, EGA_RES_str_get, "RES_str_get(lang[, str_id])");
	EGA_add_fn("RES_str_set", 2, 3, EGA_RES_str_set, "RES_str_set(lang, str_id, str) or RES_str_set(lang, ary)");
	EGA_add_fn("RES_unload_resh", 0, 0, EGA_RES_unload_resh, "RES_unload_resh()");
}
