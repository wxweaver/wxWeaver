/*
    wxWeaver - A GUI Designer Editor for wxWidgets.
    Copyright (C) 2005 José Antonio Hurtado
    Copyright (C) 2005 Juan Antonio Ortega
    Copyright (C) 2009 Michal Bližňák (as wxFormBuilder)
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
#pragma once

#include "utils/defs.h"
#include <wx/panel.h>

class CodeEditor;

class wxStyledTextCtrl;
class wxFindDialogEvent;

class wxWeaverEvent;
class wxWeaverPropertyEvent;
class wxWeaverObjectEvent;
class wxWeaverEventHandlerEvent;

class PythonPanel : public wxPanel {
public:
    PythonPanel(wxWindow* parent, int id);
    PythonPanel(const PythonPanel&) = delete;
    ~PythonPanel() override;

    PythonPanel& operator=(const PythonPanel&) = delete;
    PythonPanel(PythonPanel&&) = delete;
    PythonPanel& operator=(PythonPanel&&) = delete;

    void OnPropertyModified(wxWeaverPropertyEvent& event);
    void OnProjectRefresh(wxWeaverEvent& event);
    void OnCodeGeneration(wxWeaverEvent& event);
    void OnObjectChange(wxWeaverObjectEvent& event);
    void OnEventHandlerModified(wxWeaverEventHandlerEvent& event);

    void OnFind(wxFindDialogEvent& event);

private:
    void InitStyledTextCtrl(wxStyledTextCtrl* stc);

    CodeEditor* m_pythonPanel;
    PTCCodeWriter m_pythonCW;
};
