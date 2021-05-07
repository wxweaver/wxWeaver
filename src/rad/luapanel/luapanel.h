/*
    wxWeaver - A GUI Designer Editor for wxWidgets.
    Copyright (C) 2005 José Antonio Hurtado (as wxFormBuilder)
    Copyright (C) 2021 Andrea Zanellato <redtid3@gmail.com>

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License
    as published by the Free Software Foundation; either version 2
    of the License, or (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#ifndef __LUA_PANEL__
#define __LUA_PANEL__

#include "../../utils/wxfbdefs.h"

#include <wx/panel.h>

class CodeEditor;

class wxStyledTextCtrl;

class wxFindDialogEvent;

class wxWeaverEvent;
class wxWeaverPropertyEvent;
class wxWeaverObjectEvent;
class wxWeaverEventHandlerEvent;

class LuaPanel : public wxPanel
{
private:
	CodeEditor* m_luaPanel;
	PTCCodeWriter m_luaCW;

    void InitStyledTextCtrl( wxStyledTextCtrl* stc );

public:
	LuaPanel( wxWindow *parent, int id );
	~LuaPanel() override;

	void OnPropertyModified( wxWeaverPropertyEvent& event );
	void OnProjectRefresh( wxWeaverEvent& event );
	void OnCodeGeneration( wxWeaverEvent& event );
	void OnObjectChange( wxWeaverObjectEvent& event );
	void OnEventHandlerModified( wxWeaverEventHandlerEvent& event );

	void OnFind( wxFindDialogEvent& event );

	DECLARE_EVENT_TABLE()
};

#endif //__LUA_PANEL__
