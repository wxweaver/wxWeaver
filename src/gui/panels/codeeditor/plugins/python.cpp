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
#include "codegen/codewriter.h"
#include "codegen/pythoncg.h"
#include "gui/panels/codeeditor/codeeditor.h"
#include "gui/panels/codeeditor/plugins/python.h"
#include "rtti/objectbase.h"
#include "utils/exception.h"
#include "utils/typeconv.h"
#include "appdata.h"
#include "event.h"

#include <wx/fdrepdlg.h>
#include <wx/stc/stc.h>

PythonPanel::PythonPanel(wxWindow* parent, int id)
    : wxPanel(parent, id)
    , m_editor(new CodeEditor(this, wxID_ANY))
    , m_codeWriter(PTCCodeWriter(new TCCodeWriter(m_editor->GetTextCtrl())))
{
    AppData()->AddHandler(this->GetEventHandler());
    wxBoxSizer* topSizer = new wxBoxSizer(wxVERTICAL);
    SetSizer(topSizer);

    InitStyledTextCtrl(m_editor->GetTextCtrl());

    topSizer->Add(m_editor, 1, wxEXPAND, 0);
    topSizer->Fit(this);
    topSizer->Layout();

    SetAutoLayout(true);

    Bind(wxEVT_FIND, &PythonPanel::OnFind, this);
    Bind(wxEVT_FIND_NEXT, &PythonPanel::OnFind, this);

    Bind(wxEVT_WVR_CODE_GENERATION, &PythonPanel::OnCodeGeneration, this);
    Bind(wxEVT_WVR_PROJECT_REFRESH, &PythonPanel::OnProjectRefresh, this);
    Bind(wxEVT_WVR_PROPERTY_MODIFIED, &PythonPanel::OnPropertyModified, this);
    Bind(wxEVT_WVR_OBJECT_CREATED, &PythonPanel::OnObjectChange, this);
    Bind(wxEVT_WVR_OBJECT_REMOVED, &PythonPanel::OnObjectChange, this);
    Bind(wxEVT_WVR_OBJECT_SELECTED, &PythonPanel::OnObjectChange, this);
    Bind(wxEVT_WVR_EVENT_HANDLER_MODIFIED, &PythonPanel::OnEventHandlerModified, this);
}

PythonPanel::~PythonPanel()
{
    AppData()->RemoveHandler(this->GetEventHandler());
}

void PythonPanel::InitStyledTextCtrl(wxStyledTextCtrl* stc)
{
    stc->SetLexer(wxSTC_LEX_PYTHON);
    stc->SetKeyWords(0, "and assert break class continue def del elif else \
                        except exec finally for from global if import in \
                        is lambda not or pass print raise return try while");
}

void PythonPanel::OnFind(wxFindDialogEvent& event)
{
    m_editor->GetEventHandler()->ProcessEvent(event);
}

void PythonPanel::OnPropertyModified(wxWeaverPropertyEvent& event)
{
    event.SetId(1); // Generate code to the panel only
    OnCodeGeneration(event);
}

void PythonPanel::OnProjectRefresh(wxWeaverEvent& event)
{
    event.SetId(1); // Generate code to the panel only
    OnCodeGeneration(event);
}

void PythonPanel::OnObjectChange(wxWeaverObjectEvent& event)
{
    event.SetId(1); // Generate code to the panel only
    OnCodeGeneration(event);
}

void PythonPanel::OnEventHandlerModified(wxWeaverEventHandlerEvent& event)
{
    event.SetId(1); // Generate code to the panel only
    OnCodeGeneration(event);
}

void PythonPanel::OnCodeGeneration(wxWeaverEvent& event)
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
            while (project->GetChildCount() > 1)
                if (project->GetChild(i) != objectToGenerate)
                    project->RemoveChild(i);
                else
                    i++;
        }
    }
    if (!project || !objectToGenerate)
        return;

    // Get Python properties from the project
    // If Python generation is not enabled, do not generate the file
    bool doFile = false;
    PProperty pCodeGen = project->GetProperty("code_generation");
    if (pCodeGen)
        doFile = TypeConv::FlagSet("Python", pCodeGen->GetValueAsString()) && !panelOnly;

    if (!(doPanel || doFile))
        return;

    size_t firstID = 1000; // Get First ID from Project File
    PProperty pFirstID = project->GetProperty("first_id");
    if (pFirstID) {
        firstID = pFirstID->GetValueAsInteger();
    }

    // Get the file name
    wxString file;
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
    bool useSpaces = false;
    PProperty pUseSpaces = project->GetProperty("indent_with_spaces");
    if (pUseSpaces)
        useSpaces = (pUseSpaces->GetValueAsInteger() ? true : false);

    m_codeWriter->SetIndentWithSpaces(useSpaces);

    wxString imagePathWrapperFunctionName;
    PProperty pImagePathWrapperFunctionName
        = project->GetProperty("image_path_wrapper_function_name");

    if (pImagePathWrapperFunctionName)
        imagePathWrapperFunctionName = pImagePathWrapperFunctionName->GetValueAsString();

    if (doPanel) { // Generate code in the panel
        PythonCodeGenerator codegen;
        codegen.UseRelativePath(useRelativePath, path);
        codegen.SetImagePathWrapperFunctionName(imagePathWrapperFunctionName);

        if (pFirstID)
            codegen.SetFirstID(firstID);

        codegen.SetSourceWriter(m_codeWriter);

        Freeze();

        wxStyledTextCtrl* pythonEditor = m_editor->GetTextCtrl();
        pythonEditor->SetReadOnly(false);
        int pythonLine
            = pythonEditor->GetFirstVisibleLine() + pythonEditor->LinesOnScreen() - 1;

        int pythonXOffset = pythonEditor->GetXOffset();

        codegen.GenerateCode(project);

        pythonEditor->SetReadOnly(true);
        pythonEditor->GotoLine(pythonLine);
        pythonEditor->SetXOffset(pythonXOffset);
        pythonEditor->SetAnchor(0);
        pythonEditor->SetCurrentPos(0);

        Thaw();
    }
    if (doFile) { // Generate code in the file
        try {
            PythonCodeGenerator codegen;
            codegen.UseRelativePath(useRelativePath, path);
            codegen.SetImagePathWrapperFunctionName(imagePathWrapperFunctionName);

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

            PCodeWriter python_cw(
                new FileCodeWriter(path + file + ".py", useMicrosoftBOM, useUtf8));

            python_cw->SetIndentWithSpaces(useSpaces);

            codegen.SetSourceWriter(python_cw);
            codegen.GenerateCode(project);
            wxLogStatus("Code generated on \'%s\'.", path.c_str());
        } catch (wxWeaverException& ex) {
            wxLogError(ex.what());
        }
    }
}
