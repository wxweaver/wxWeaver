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
#include "codegen/codewriter.h"
#include "codegen/cppcg.h"
#include "gui/aui/tabart.h"
#include "gui/dialogs/preferences.h"
#include "gui/panels/codeeditor/codeeditor.h"
#include "gui/panels/codeeditor/plugins/cpp.h"
#include "gui/bitmaps.h"
#include "rtti/objectbase.h"
#include "utils/exception.h"
#include "utils/typeconv.h"
#include "appdata.h"
#include "event.h"
#include "settings.h"

#include <wx/fdrepdlg.h>
#include <wx/stc/stc.h>
#include <wx/aui/auibook.h>

CppPanel::CppPanel(wxWindow* parent, int id)
    : wxPanel(parent, id)
    , m_notebook(new wxAuiNotebook(this, wxID_ANY, wxDefaultPosition,
                                   wxDefaultSize, wxAUI_NB_TOP))
    , m_editorCpp(new CodeEditor(m_notebook, wxID_ANY))
    , m_editorH(new CodeEditor(m_notebook, wxID_ANY))
    , m_codeWriterH(PTCCodeWriter(new TCCodeWriter(m_editorH->GetTextCtrl())))
    , m_codeWriterCpp(PTCCodeWriter(new TCCodeWriter(m_editorCpp->GetTextCtrl())))
{
    AppData()->AddHandler(this->GetEventHandler());
    wxBoxSizer* topSizer = new wxBoxSizer(wxVERTICAL);
    SetSizer(topSizer);

    InitStyledTextCtrl(m_editorCpp->GetTextCtrl());
    InitStyledTextCtrl(m_editorH->GetTextCtrl());

#if wxVERSION_NUMBER < 3140
    m_notebook->SetArtProvider(new AuiTabArt());
#endif
    m_notebook->AddPage(m_editorCpp, "cpp", false, 0);
    m_notebook->AddPage(m_editorH, "h", false, 1);
    m_notebook->SetPageBitmap(0, AppBitmaps::GetBitmap("cpp", 16));
    m_notebook->SetPageBitmap(1, AppBitmaps::GetBitmap("h", 16));

    topSizer->Add(m_notebook, 1, wxEXPAND, 0);
    topSizer->Fit(this);
    topSizer->Layout();

    SetAutoLayout(true);

    Bind(wxEVT_FIND, &CppPanel::OnFind, this);
    Bind(wxEVT_FIND_NEXT, &CppPanel::OnFind, this);

    Bind(wxEVT_WVR_CODE_GENERATION, &CppPanel::OnCodeGeneration, this);
    Bind(wxEVT_WVR_PROJECT_REFRESH, &CppPanel::OnProjectRefresh, this);
    Bind(wxEVT_WVR_PROPERTY_MODIFIED, &CppPanel::OnPropertyModified, this);
    Bind(wxEVT_WVR_OBJECT_CREATED, &CppPanel::OnObjectChange, this);
    Bind(wxEVT_WVR_OBJECT_REMOVED, &CppPanel::OnObjectChange, this);
    Bind(wxEVT_WVR_OBJECT_SELECTED, &CppPanel::OnObjectChange, this);
    Bind(wxEVT_WVR_EVENT_HANDLER_MODIFIED, &CppPanel::OnEventHandlerModified, this);
}

CppPanel::~CppPanel()
{
    AppData()->RemoveHandler(this->GetEventHandler());
}

void CppPanel::InitStyledTextCtrl(wxStyledTextCtrl* stc)
{
    stc->SetLexer(wxSTC_LEX_CPP);
    stc->SetKeyWords(
        0,
        "#define #else #elseif #endif #error #if #ifdef #ifndef #include #pragma "
        "#warning nullptr alignas alignof and and_eq asm auto bitand bitor break "
        "case catch class co_return co_wait co_yield compl concept const const "
        "const_cast consteval constexpr constinit continue decltype default "
        "delete do dynamic_cast else elseif enum explicit export export extern "
        "for friend goto if inline mutable namespace new noexcept not not_eq "
        "nullptr operator or or_eq private protected public register "
        "reinterpret_cast requires return static static_assert static_cast "
        "switch template then this thread_local throw try typedef typeid typename "
        "union using virtual void volatile while xor xor_eq");

    stc->SetKeyWords(
        1,
        "bool char char16_t char32_t char8_t double false float int long short"
        "struct true unsigned wchar_t");
}

void CppPanel::OnFind(wxFindDialogEvent& event)
{
    wxAuiNotebook* languageBook = wxDynamicCast(this->GetParent(), wxAuiNotebook);
    if (!languageBook)
        return;

    int languageSelection = languageBook->GetSelection();
    if (languageSelection < 0)
        return;

    wxString languageText = languageBook->GetPageText(languageSelection);
    if (languageText != "C++")
        return;

    wxAuiNotebook* notebook = wxDynamicCast(m_editorCpp->GetParent(), wxAuiNotebook);
    if (!notebook)
        return;

    int selection = notebook->GetSelection();
    if (selection < 0)
        return;

    wxString text = notebook->GetPageText(selection);
    if (text == "cpp")
        m_editorCpp->GetEventHandler()->ProcessEvent(event);
    else if (text == "h")
        m_editorH->GetEventHandler()->ProcessEvent(event);
}

void CppPanel::OnPropertyModified(wxWeaverPropertyEvent& event)
{
    event.SetId(1); // Generate code to the panel only
    OnCodeGeneration(event);
}

void CppPanel::OnProjectRefresh(wxWeaverEvent& event)
{
    event.SetId(1); // Generate code to the panel only
    OnCodeGeneration(event);
}

void CppPanel::OnObjectChange(wxWeaverObjectEvent& event)
{
    event.SetId(1); // Generate code to the panel only
    OnCodeGeneration(event);
}

void CppPanel::OnEventHandlerModified(wxWeaverEventHandlerEvent& event)
{
    event.SetId(1); // Generate code to the panel only
    OnCodeGeneration(event);
}

void CppPanel::OnCodeGeneration(wxWeaverEvent& event)
{
    bool doPanel = IsShown(); // Generate code in the panel if the panel is active

    // Using the previously unused Id field in the event to carry a boolean
    bool panelOnly = (event.GetId());

    // Only generate to panel + panel is not shown = do nothing
    if (panelOnly && !doPanel)
        return;

    // For code preview generate only code relevant to selected form,
    // otherwise generate full project code.

    // Create copy of the original project due to possible temporary modifications
    PObjectBase project = PObjectBase(new ObjectBase(*AppData()->GetProjectData()));
    PObjectBase objectToGenerate;

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

    // Get C++ properties from the project
    // If C++ generation is not enabled, do not generate the file
    bool doFile = false;
    PProperty pCodeGen = project->GetProperty("code_generation");
    if (pCodeGen)
        doFile = TypeConv::FlagSet("C++", pCodeGen->GetValueAsString()) && !panelOnly;

    if (!(doPanel || doFile))
        return;

    size_t firstID = 1000; // Get First ID from Project File
    PProperty pFirstID = project->GetProperty("first_id");
    if (pFirstID)
        firstID = pFirstID->GetValueAsInteger();

    wxString file; // Get the file name
    PProperty pfile = project->GetProperty("file");
    if (pfile)
        file = pfile->GetValueAsString();

    if (file.empty())
        file = "noname";

    // Determine if the path is absolute or relative
    bool useRelativePath = false;
    PProperty pRelPath = project->GetProperty("relative_path");
    if (pRelPath)
        useRelativePath = (pRelPath->GetValueAsInteger() ? true : false);

    wxString path; // Get the output path
    try {
        path = AppData()->GetOutputPath();
    } catch (wxWeaverException& ex) {
        if (doFile) {
            path = wxEmptyString;
            wxLogWarning(ex.what());
            return;
        }
    }
    if (doPanel) { // Generate code in the panel
        CppCodeGenerator codegen;
        codegen.UseRelativePath(useRelativePath, path);

        if (pFirstID)
            codegen.SetFirstID(firstID);

        codegen.SetHeaderWriter(m_codeWriterH);
        codegen.SetSourceWriter(m_codeWriterCpp);

        Freeze();

        wxStyledTextCtrl* cppEditor = m_editorCpp->GetTextCtrl();
        wxStyledTextCtrl* hEditor = m_editorH->GetTextCtrl();
        cppEditor->SetReadOnly(false);
        int cppLine = cppEditor->GetFirstVisibleLine()
            + cppEditor->LinesOnScreen() - 1;

        int cppXOffset = cppEditor->GetXOffset();

        hEditor->SetReadOnly(false);
        int hLine = hEditor->GetFirstVisibleLine() + hEditor->LinesOnScreen() - 1;
        int hXOffset = hEditor->GetXOffset();

        codegen.GenerateCode(project);

        cppEditor->SetReadOnly(true);
        cppEditor->GotoLine(cppLine);
        cppEditor->SetXOffset(cppXOffset);
        cppEditor->SetAnchor(0);
        cppEditor->SetCurrentPos(0);

        hEditor->SetReadOnly(true);
        hEditor->GotoLine(hLine);
        hEditor->SetXOffset(hXOffset);
        hEditor->SetAnchor(0);
        hEditor->SetCurrentPos(0);

        Thaw();
    }
    if (!doFile)
        return;

    try { // Generate code in the file
        CppCodeGenerator codegen;
        codegen.UseRelativePath(useRelativePath, path);

        if (pFirstID)
            codegen.SetFirstID(firstID);

        // Determine if Microsoft BOM should be used
        bool useMicrosoftBOM = false;

        PProperty pUseMicrosoftBOM = project->GetProperty("use_microsoft_bom");
        if (pUseMicrosoftBOM)
            useMicrosoftBOM = pUseMicrosoftBOM->GetValueAsInteger();

        // Determine if Utf8 or Ansi is to be created
        bool useUtf8 = false;
        PProperty pUseUtf8 = project->GetProperty("encoding");
        if (pUseUtf8)
            useUtf8 = (pUseUtf8->GetValueAsString() != "ANSI");

        PCodeWriter h_cw(new FileCodeWriter(
            path + file + ".h", useMicrosoftBOM, useUtf8));

        PCodeWriter cpp_cw(new FileCodeWriter(
            path + file + ".cpp", useMicrosoftBOM, useUtf8));

        codegen.SetHeaderWriter(h_cw);
        codegen.SetSourceWriter(cpp_cw);
        codegen.GenerateCode(project);
        wxLogStatus("Code generated on \'%s\'.", path.c_str());
    } catch (wxWeaverException& ex) {
        wxLogError(ex.what());
    }
}
