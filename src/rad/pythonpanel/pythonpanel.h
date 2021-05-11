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

/**
@file
@author Michal Bližňák - michal.bliznak@gmail.com
@note
*/

#ifndef __PYTHON_PANEL__
#define __PYTHON_PANEL__

#include "utils/defs.h"

#include <wx/panel.h>

class CodeEditor;

class wxStyledTextCtrl;

class wxFindDialogEvent;

class wxWeaverEvent;
class wxWeaverPropertyEvent;
class wxWeaverObjectEvent;
class wxWeaverEventHandlerEvent;

class PythonPanel : public wxPanel
{
private:
	CodeEditor* m_pythonPanel;
	PTCCodeWriter m_pythonCW;

    void InitStyledTextCtrl( wxStyledTextCtrl* stc );

public:
	PythonPanel( wxWindow *parent, int id );
	~PythonPanel() override;
	PythonPanel(const PythonPanel&) = delete;
	PythonPanel& operator=(const PythonPanel&) = delete;
	PythonPanel(PythonPanel&&) = delete;
	PythonPanel& operator=(PythonPanel&&) = delete;

	void OnPropertyModified( wxWeaverPropertyEvent& event );
	void OnProjectRefresh( wxWeaverEvent& event );
	void OnCodeGeneration( wxWeaverEvent& event );
	void OnObjectChange( wxWeaverObjectEvent& event );
	void OnEventHandlerModified( wxWeaverEventHandlerEvent& event );

	void OnFind( wxFindDialogEvent& event );

	DECLARE_EVENT_TABLE()
};

#endif //__PYTHON_PANEL__
