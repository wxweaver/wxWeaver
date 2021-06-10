/*
    wxWeaver - A GUI Designer Editor for wxWidgets.
    Copyright (C) 2005 José Antonio Hurtado
    Copyright (C) 2005 Juan Antonio Ortega
    Copyright (C) 2013 Vratislav Zival (as wxFormBuilder)
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
#include "gui/panels/codeeditor/plugins/lua.h"

#include "appdata.h"
#include "gui/panels/codeeditor/codeeditor.h"
#include "event.h"
#include "utils/typeconv.h"
#include "utils/exception.h"
#include "model/objectbase.h"
#include "codegen/codewriter.h"
#include "codegen/luacg.h"

#include <wx/fdrepdlg.h>
#include <wx/stc/stc.h>

#if 0
BEGIN_EVENT_TABLE(LuaPanel, wxPanel)
EVT_WVR_CODE_GENERATION(LuaPanel::OnCodeGeneration)
EVT_WVR_PROJECT_REFRESH(LuaPanel::OnProjectRefresh)
EVT_WVR_PROPERTY_MODIFIED(LuaPanel::OnPropertyModified)
EVT_WVR_OBJECT_CREATED(LuaPanel::OnObjectChange)
EVT_WVR_OBJECT_REMOVED(LuaPanel::OnObjectChange)
EVT_WVR_OBJECT_SELECTED(LuaPanel::OnObjectChange)
EVT_WVR_EVENT_HANDLER_MODIFIED(LuaPanel::OnEventHandlerModified)
EVT_FIND(wxID_ANY, LuaPanel::OnFind)
EVT_FIND_NEXT(wxID_ANY, LuaPanel::OnFind)
END_EVENT_TABLE()
#endif

LuaPanel::LuaPanel(wxWindow* parent, int id)
    : wxPanel(parent, id)
    , m_luaPanel(new CodeEditor(this, wxID_ANY))
    , m_luaCW(PTCCodeWriter(new TCCodeWriter(m_luaPanel->GetTextCtrl())))
{
    AppData()->AddHandler(this->GetEventHandler());
    wxBoxSizer* topSizer = new wxBoxSizer(wxVERTICAL);
    SetSizer(topSizer);

    InitStyledTextCtrl(m_luaPanel->GetTextCtrl());

    topSizer->Add(m_luaPanel, 1, wxEXPAND, 0);
    topSizer->Fit(this);
    topSizer->Layout();

    SetAutoLayout(true);

    Bind(wxEVT_FIND, &LuaPanel::OnFind, this);
    Bind(wxEVT_FIND_NEXT, &LuaPanel::OnFind, this);

    Bind(wxEVT_WVR_CODE_GENERATION, &LuaPanel::OnCodeGeneration, this);
    Bind(wxEVT_WVR_PROJECT_REFRESH, &LuaPanel::OnProjectRefresh, this);
    Bind(wxEVT_WVR_PROPERTY_MODIFIED, &LuaPanel::OnPropertyModified, this);
    Bind(wxEVT_WVR_OBJECT_CREATED, &LuaPanel::OnObjectChange, this);
    Bind(wxEVT_WVR_OBJECT_REMOVED, &LuaPanel::OnObjectChange, this);
    Bind(wxEVT_WVR_OBJECT_SELECTED, &LuaPanel::OnObjectChange, this);
    Bind(wxEVT_WVR_EVENT_HANDLER_MODIFIED, &LuaPanel::OnEventHandlerModified, this);
}

LuaPanel::~LuaPanel()
{
    //delete m_icons;
    AppData()->RemoveHandler(this->GetEventHandler());
}

void LuaPanel::InitStyledTextCtrl(wxStyledTextCtrl* stc)
{
    stc->SetLexer(wxSTC_LEX_LUA);
    stc->SetKeyWords(
        0,
        "and assert break class continue def del elif else "
        "except exec finally for from global if import in "
        "is lambda not or pass print raise return try while");
#ifdef __WXGTK__
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
    stc->StyleSetBold(wxSTC_C_WORD, true);
    if (!darkMode) {
        stc->StyleSetForeground(wxSTC_C_WORD, wxColour(0, 0, 128));
        stc->StyleSetForeground(wxSTC_C_STRING, wxColour(0, 128, 0));
        stc->StyleSetForeground(wxSTC_C_STRINGEOL, wxColour(0, 128, 0));
        stc->StyleSetForeground(wxSTC_C_PREPROCESSOR, wxColour(0, 0, 80));
        stc->StyleSetForeground(wxSTC_C_COMMENT, wxColour(0, 128, 0));
        stc->StyleSetForeground(wxSTC_C_COMMENTLINE, wxColour(0, 128, 0));
        stc->StyleSetForeground(wxSTC_C_COMMENTDOC, wxColour(0, 128, 0));
        stc->StyleSetForeground(wxSTC_C_COMMENTLINEDOC, wxColour(0, 128, 0));
        stc->StyleSetForeground(wxSTC_C_NUMBER, wxColour(0, 0, 128));
        stc->SetSelBackground(true, wxSystemSettings::GetColour(wxSYS_COLOUR_HIGHLIGHT));
        stc->SetSelForeground(true, wxSystemSettings::GetColour(wxSYS_COLOUR_HIGHLIGHTTEXT));
    } else {
        stc->StyleSetForeground(wxSTC_C_WORD, wxColour(200, 120, 230));
        stc->StyleSetForeground(wxSTC_C_WORD2, wxColour(235, 100, 115));
        stc->StyleSetForeground(wxSTC_C_GLOBALCLASS, wxColour(235, 100, 115));
        //stc->StyleSetForeground(wxSTC_C_IDENTIFIER, wxColour(90, 180, 250));
        stc->StyleSetForeground(wxSTC_C_CHARACTER, wxColour(150, 200, 120));
        stc->StyleSetForeground(wxSTC_C_STRING, wxColour(150, 200, 120));
        stc->StyleSetForeground(wxSTC_C_STRINGEOL, wxColour(150, 200, 120));
        stc->StyleSetForeground(wxSTC_C_PREPROCESSOR, wxColour(200, 120, 230));
        stc->StyleSetForeground(wxSTC_C_PREPROCESSORCOMMENT, wxColour(90, 100, 120));
        stc->StyleSetForeground(wxSTC_C_COMMENT, wxColour(90, 100, 120));
        stc->StyleSetForeground(wxSTC_C_COMMENTLINE, wxColour(90, 100, 120));
        stc->StyleSetForeground(wxSTC_C_COMMENTDOC, wxColour(90, 100, 120));
        stc->StyleSetForeground(wxSTC_C_COMMENTLINEDOC, wxColour(90, 100, 120));
        stc->StyleSetForeground(wxSTC_C_NUMBER, wxColour(220, 160, 100));
        stc->SetSelBackground(true, wxColour(45, 50, 60));
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

void LuaPanel::OnFind(wxFindDialogEvent& event)
{
    m_luaPanel->GetEventHandler()->ProcessEvent(event);
}

void LuaPanel::OnPropertyModified(wxWeaverPropertyEvent& event)
{
    event.SetId(1); // Generate code to the panel only
    OnCodeGeneration(event);
}

void LuaPanel::OnProjectRefresh(wxWeaverEvent& event)
{
    event.SetId(1); // Generate code to the panel only
    OnCodeGeneration(event);
}

void LuaPanel::OnObjectChange(wxWeaverObjectEvent& event)
{
    event.SetId(1); // Generate code to the panel only
    OnCodeGeneration(event);
}

void LuaPanel::OnEventHandlerModified(wxWeaverEventHandlerEvent& event)
{
    event.SetId(1); // Generate code to the panel only
    OnCodeGeneration(event);
}

void LuaPanel::OnCodeGeneration(wxWeaverEvent& event)
{
    PObjectBase objectToGenerate;

    // Generate code in the panel if the panel is active
    bool doPanel = IsShown();

    // Using the previously unused Id field in the event to carry a boolean
    bool panelOnly = (event.GetId());

    // Only generate to panel + panel is not shown = do nothing
    if (panelOnly && !doPanel)
        return;

    // For code preview generate only code relevant to selected form,
    // otherwise generate full project code.
    // Create copy of the original project due to possible temporary modifications
    PObjectBase project = PObjectBase(new ObjectBase(*AppData()->GetProjectData()));

    if (panelOnly)
        objectToGenerate = AppData()->GetSelectedForm();

    if (!panelOnly || !objectToGenerate)
        objectToGenerate = project;

    // If only one project item should be generated then remove the rest items
    // from the temporary project
    if (doPanel && panelOnly && (objectToGenerate != project)) {
        if (project->GetChildCount() > 0) {
            size_t i = 0;
            while (project->GetChildCount() > 1) {
                if (project->GetChild(i) != objectToGenerate)
                    project->RemoveChild(i);
                else
                    i++;
            }
        }
    }
    if (!project || !objectToGenerate)
        return;

    // Get Lua properties from the project
    // If Lua generation is not enabled, do not generate the file
    bool doFile = false;
    PProperty pCodeGen = project->GetProperty("code_generation");
    if (pCodeGen)
        doFile = TypeConv::FlagSet("Lua", pCodeGen->GetValue()) && !panelOnly;

    if (!(doPanel || doFile))
        return;

    // Get First ID from Project File
    size_t firstID = 1000;
    PProperty pFirstID = project->GetProperty("first_id");
    if (pFirstID)
        firstID = pFirstID->GetValueAsInteger();

    // Get the file name
    wxString file;
    PProperty pfile = project->GetProperty("file");
    if (pfile)
        file = pfile->GetValue();

    if (file.empty())
        file = "noname";

    // Determine if the path is absolute or relative
    bool useRelativePath = false;
    PProperty pRelPath = project->GetProperty("relative_path");
    if (pRelPath)
        useRelativePath = (pRelPath->GetValueAsInteger() ? true : false);

    // Get the output path
    wxString path;
    try {
        path = AppData()->GetOutputPath();
    } catch (wxWeaverException& ex) {
        if (doFile) {
            path = wxEmptyString;
            wxLogWarning(ex.what());
            return;
        }
    }
    // Generate code in the panel
    if (doPanel) {
        LuaCodeGenerator codegen;
        codegen.UseRelativePath(useRelativePath, path);

        if (pFirstID)
            codegen.SetFirstID(firstID);

        codegen.SetSourceWriter(m_luaCW);

        Freeze();

        wxStyledTextCtrl* luaEditor = m_luaPanel->GetTextCtrl();
        luaEditor->SetReadOnly(false);
        int luaLine = luaEditor->GetFirstVisibleLine() + luaEditor->LinesOnScreen() - 1;
        int luaXOffset = luaEditor->GetXOffset();

        codegen.GenerateCode(project);

        luaEditor->SetReadOnly(true);
        luaEditor->GotoLine(luaLine);
        luaEditor->SetXOffset(luaXOffset);
        luaEditor->SetAnchor(0);
        luaEditor->SetCurrentPos(0);

        Thaw();
    }

    // Generate code in the file
    if (doFile) {
        try {
            LuaCodeGenerator codegen;
            codegen.UseRelativePath(useRelativePath, path);

            if (pFirstID)
                codegen.SetFirstID(firstID);

            // Determin if Microsoft BOM should be used
            bool useMicrosoftBOM = false;
            PProperty pUseMicrosoftBOM = project->GetProperty("use_microsoft_bom");

            if (pUseMicrosoftBOM)
                useMicrosoftBOM = (pUseMicrosoftBOM->GetValueAsInteger());

            // Determine if Utf8 or Ansi is to be created
            bool useUtf8 = false;
            PProperty pUseUtf8 = project->GetProperty("encoding");

            if (pUseUtf8)
                useUtf8 = (pUseUtf8->GetValueAsString() != "ANSI");

            PCodeWriter lua_cw(new FileCodeWriter(path + file + ".lua", useMicrosoftBOM, useUtf8));
            codegen.SetSourceWriter(lua_cw);
            codegen.GenerateCode(project);
            wxLogStatus("Code generated on \'%s\'.", path.c_str());
        } catch (wxWeaverException& ex) {
            wxLogError(ex.what());
        }
    }
}
