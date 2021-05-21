/*
    wxWeaver - A GUI Designer Editor for wxWidgets.
    Copyright (C) 2005 José Antonio Hurtado
    Copyright (C) 2005 Juan Antonio Ortega (as wxFormBuilder)
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
#include "rad/xrcpanel/xrcpanel.h"

#include "codegen/codewriter.h"
#include "codegen/xrccg.h"
#include "model/objectbase.h"
#include "utils/typeconv.h"
#include "utils/exception.h"
#include "rad/appdata.h"
#include "rad/codeeditor/codeeditor.h"
#include "rad/event.h"

#include <wx/fdrepdlg.h>
#include <wx/aui/auibook.h>

#if 0
BEGIN_EVENT_TABLE(XrcPanel, wxPanel)
EVT_WVR_CODE_GENERATION(XrcPanel::OnCodeGeneration)
EVT_WVR_PROJECT_REFRESH(XrcPanel::OnProjectRefresh)
EVT_WVR_PROPERTY_MODIFIED(XrcPanel::OnPropertyModified)
EVT_WVR_OBJECT_CREATED(XrcPanel::OnObjectChange)
EVT_WVR_OBJECT_REMOVED(XrcPanel::OnObjectChange)
EVT_WVR_OBJECT_SELECTED(XrcPanel::OnObjectChange)
EVT_FIND(wxID_ANY, XrcPanel::OnFind)
EVT_FIND_NEXT(wxID_ANY, XrcPanel::OnFind)
END_EVENT_TABLE()
#endif

XrcPanel::XrcPanel(wxWindow* parent, int id)
    : wxPanel(parent, id)
    , m_xrcPanel(new CodeEditor(this, wxID_ANY))
    , m_codeWriter(PTCCodeWriter(new TCCodeWriter(m_xrcPanel->GetTextCtrl())))
{
    AppData()->AddHandler(this->GetEventHandler());
    wxBoxSizer* topSizer = new wxBoxSizer(wxVERTICAL);
    SetSizer(topSizer);

    InitStyledTextCtrl(m_xrcPanel->GetTextCtrl());

    topSizer->Add(m_xrcPanel, 1, wxEXPAND, 0);
    topSizer->Fit(this);
    topSizer->Layout();

    SetAutoLayout(true);

    Bind(wxEVT_FIND, &XrcPanel::OnFind, this);
    Bind(wxEVT_FIND_NEXT, &XrcPanel::OnFind, this);

    Bind(wxEVT_WVR_CODE_GENERATION, &XrcPanel::OnCodeGeneration, this);
    Bind(wxEVT_WVR_PROJECT_REFRESH, &XrcPanel::OnProjectRefresh, this);
    Bind(wxEVT_WVR_PROPERTY_MODIFIED, &XrcPanel::OnPropertyModified, this);
    Bind(wxEVT_WVR_OBJECT_CREATED, &XrcPanel::OnObjectChange, this);
    Bind(wxEVT_WVR_OBJECT_REMOVED, &XrcPanel::OnObjectChange, this);
    Bind(wxEVT_WVR_OBJECT_SELECTED, &XrcPanel::OnObjectChange, this);
}

XrcPanel::~XrcPanel()
{
    AppData()->RemoveHandler(this->GetEventHandler());
}

void XrcPanel::InitStyledTextCtrl(wxStyledTextCtrl* stc)
{
    stc->SetLexer(wxSTC_LEX_XML);
#ifdef __WXGTK__
    // Debe haber un bug en wxGTK ya que la familia wxMODERN no es de ancho fijo.
    wxFont font(8, wxFONTFAMILY_MODERN, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL);
    font.SetFaceName("Monospace");
#else
    wxFont font(10, wxFONTFAMILY_MODERN, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL);
#endif
    bool darkMode = AppData()->IsDarkMode();
    if (darkMode) {
        stc->StyleSetBackground(wxSTC_STYLE_DEFAULT, wxColour(30, 30, 30));
        stc->StyleSetForeground(wxSTC_STYLE_DEFAULT, wxColour(170, 180, 190));
    } else {
        stc->StyleSetBackground(wxSTC_STYLE_DEFAULT,
                                wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOW));
        stc->StyleSetForeground(wxSTC_STYLE_DEFAULT,
                                wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOWTEXT));
    }
    stc->StyleSetFont(wxSTC_STYLE_DEFAULT, font);
    stc->StyleClearAll();
    if (!AppData()->IsDarkMode()) {
        stc->StyleSetForeground(wxSTC_H_DOUBLESTRING, *wxRED);
        stc->StyleSetForeground(wxSTC_H_TAG, wxColour(0, 0, 128));
        stc->StyleSetForeground(wxSTC_H_ATTRIBUTE, wxColour(128, 0, 128));
    } else {
        stc->StyleSetForeground(wxSTC_H_DOUBLESTRING, wxColour(23, 198, 163));
        stc->StyleSetForeground(wxSTC_H_TAG, wxColour(18, 144, 195));
        stc->StyleSetForeground(wxSTC_H_ATTRIBUTE, wxColour(221, 40, 103));
    }
    stc->SetCaretForeground(wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOWTEXT));
    stc->SetCaretWidth(2);
    stc->SetReadOnly(true);

    // TODO: Make this configurable
    stc->SetUseTabs(false);
    stc->SetTabWidth(4);
    stc->SetTabIndents(true);
    stc->SetBackSpaceUnIndents(true);
    stc->SetIndent(4);
}

void XrcPanel::OnFind(wxFindDialogEvent& event)
{
    wxAuiNotebook* notebook = wxDynamicCast(this->GetParent(), wxAuiNotebook);
    if (!notebook)
        return;

    int selection = notebook->GetSelection();
    if (selection < 0)
        return;

    wxString text = notebook->GetPageText(selection);
    if (text == "XRC")
        m_xrcPanel->GetEventHandler()->ProcessEvent(event);
}

void XrcPanel::OnPropertyModified(wxWeaverPropertyEvent& event)
{
    event.SetId(1); // Generate code to the panel only
    OnCodeGeneration(event);
}

void XrcPanel::OnProjectRefresh(wxWeaverEvent& event)
{
    event.SetId(1); // Generate code to the panel only
    OnCodeGeneration(event);
}

void XrcPanel::OnObjectChange(wxWeaverObjectEvent& event)
{
    event.SetId(1); // Generate code to the panel only
    OnCodeGeneration(event);
}

void XrcPanel::OnCodeGeneration(wxWeaverEvent& event)
{
    bool doPanel = IsShown(); // Generate code in the panel if the panel is active

    // Using the previously unused Id field in the event to carry a boolean
    bool panelOnly = (event.GetId());

    // Only generate to panel + panel is not shown = do nothing
    if (panelOnly && !doPanel)
        return;

    // For code preview generate only code relevant to selected form,
    //  otherwise generate full project code.
    //PObjectBase project = AppData()->GetProjectData();
    PObjectBase project;
    if (panelOnly)
        project = AppData()->GetSelectedForm();

    if (!panelOnly || !project)
        project = AppData()->GetProjectData();

    if (!project)
        return;

    if (doPanel) { // Generate code in the panel if the panel is active

        Freeze();

        wxStyledTextCtrl* editor = m_xrcPanel->GetTextCtrl();

        editor->SetReadOnly(false);

        int line = editor->GetFirstVisibleLine() + editor->LinesOnScreen() - 1;
        int xOffset = editor->GetXOffset();

        XrcCodeGenerator codegen;
        codegen.SetWriter(m_codeWriter);
        codegen.GenerateCode(project);
        editor->SetReadOnly(true);
        editor->GotoLine(line);
        editor->SetXOffset(xOffset);
        editor->SetAnchor(0);
        editor->SetCurrentPos(0);

        Thaw();
    }
    if (panelOnly)
        return;

    PProperty pCodeGen = project->GetProperty("code_generation");
    if (!pCodeGen || !TypeConv::FlagSet("XRC", pCodeGen->GetValue()))
        return;

    { // And now in the file.
        try {
            wxString path = AppData()->GetOutputPath(); // Get the output path
            wxString file;
            PProperty pfile = project->GetProperty("file");
            if (pfile)
                file = pfile->GetValue();

            if (file.empty())
                file = "noname";

            wxString filePath;
            filePath << path << file << ".xrc";
            PCodeWriter cw(new FileCodeWriter(filePath));

            XrcCodeGenerator codegen;
            codegen.SetWriter(cw);
            codegen.GenerateCode(project);
            wxLogStatus(_("Code generated on \'%s\'."), path.c_str());
        } catch (wxWeaverException& ex) {
            wxLogError(ex.what());
        }
    }
}
