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
#include "gui/panels/codeeditor/plugins/cpp.h"

#include "appdata.h"
#include "gui/aui/tabart.h"
#include "gui/bitmaps.h"
#include "gui/panels/codeeditor/codeeditor.h"
#include "event.h"
#include "utils/typeconv.h"
#include "utils/exception.h"
#include "model/objectbase.h"
#include "codegen/codewriter.h"
#include "codegen/cppcg.h"

#include <wx/fdrepdlg.h>
#include <wx/stc/stc.h>
#include <wx/aui/auibook.h>

#if 0
BEGIN_EVENT_TABLE(CppPanel, wxPanel)
EVT_WVR_CODE_GENERATION(CppPanel::OnCodeGeneration)
EVT_WVR_PROJECT_REFRESH(CppPanel::OnProjectRefresh)
EVT_WVR_PROPERTY_MODIFIED(CppPanel::OnPropertyModified)
EVT_WVR_OBJECT_CREATED(CppPanel::OnObjectChange)
EVT_WVR_OBJECT_REMOVED(CppPanel::OnObjectChange)
EVT_WVR_OBJECT_SELECTED(CppPanel::OnObjectChange)
EVT_WVR_EVENT_HANDLER_MODIFIED(CppPanel::OnEventHandlerModified)

EVT_FIND(wxID_ANY, CppPanel::OnFind)
EVT_FIND_NEXT(wxID_ANY, CppPanel::OnFind)
END_EVENT_TABLE()
#endif

CppPanel::CppPanel(wxWindow* parent, int id)
    : wxPanel(parent, id)
    , m_notebook(new wxAuiNotebook(this, wxID_ANY, wxDefaultPosition,
                                   wxDefaultSize, wxAUI_NB_TOP))
    , m_cppPanel(new CodeEditor(m_notebook, wxID_ANY))
    , m_hPanel(new CodeEditor(m_notebook, wxID_ANY))
    , m_hCW(PTCCodeWriter(new TCCodeWriter(m_hPanel->GetTextCtrl())))
    , m_cppCW(PTCCodeWriter(new TCCodeWriter(m_cppPanel->GetTextCtrl())))
{
    AppData()->AddHandler(this->GetEventHandler());
    wxBoxSizer* topSizer = new wxBoxSizer(wxVERTICAL);
    SetSizer(topSizer);

    InitStyledTextCtrl(m_cppPanel->GetTextCtrl());
    InitStyledTextCtrl(m_hPanel->GetTextCtrl());

#if wxVERSION_NUMBER < 3140
    m_notebook->SetArtProvider(new AuiTabArt());
#endif
    m_notebook->AddPage(m_cppPanel, "cpp", false, 0);
    m_notebook->AddPage(m_hPanel, "h", false, 1);
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

    wxAuiNotebook* notebook = wxDynamicCast(m_cppPanel->GetParent(), wxAuiNotebook);
    if (!notebook)
        return;

    int selection = notebook->GetSelection();
    if (selection < 0)
        return;

    wxString text = notebook->GetPageText(selection);
    if (text == "cpp")
        m_cppPanel->GetEventHandler()->ProcessEvent(event);
    else if (text == "h")
        m_hPanel->GetEventHandler()->ProcessEvent(event);
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
        doFile = TypeConv::FlagSet("C++", pCodeGen->GetValue()) && !panelOnly;

    if (!(doPanel || doFile))
        return;

    size_t firstID = 1000; // Get First ID from Project File
    PProperty pFirstID = project->GetProperty("first_id");
    if (pFirstID)
        firstID = pFirstID->GetValueAsInteger();

    wxString file; // Get the file name
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

        codegen.SetHeaderWriter(m_hCW);
        codegen.SetSourceWriter(m_cppCW);

        Freeze();

        wxStyledTextCtrl* cppEditor = m_cppPanel->GetTextCtrl();
        wxStyledTextCtrl* hEditor = m_hPanel->GetTextCtrl();
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

        // Determin if Microsoft BOM should be used
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
