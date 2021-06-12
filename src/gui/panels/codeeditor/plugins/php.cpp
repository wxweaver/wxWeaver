/*
    wxWeaver - A GUI Designer Editor for wxWidgets.
    Copyright (C) 2005 José Antonio Hurtado
    Copyright (C) 2005 Juan Antonio Ortega (as wxFormBuilder)
    Copyright (C) 2011-2021 Jefferson González <jgmdev@gmail.com>
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
#include "codegen/phpcg.h"
#include "gui/panels/codeeditor/codeeditor.h"
#include "gui/panels/codeeditor/plugins/php.h"
#include "model/objectbase.h"
#include "utils/exception.h"
#include "utils/typeconv.h"
#include "appdata.h"
#include "event.h"

#include <wx/fdrepdlg.h>
#include <wx/stc/stc.h>

PHPPanel::PHPPanel(wxWindow* parent, int id)
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

    Bind(wxEVT_FIND, &PHPPanel::OnFind, this);
    Bind(wxEVT_FIND_NEXT, &PHPPanel::OnFind, this);

    Bind(wxEVT_WVR_CODE_GENERATION, &PHPPanel::OnCodeGeneration, this);
    Bind(wxEVT_WVR_PROJECT_REFRESH, &PHPPanel::OnProjectRefresh, this);
    Bind(wxEVT_WVR_PROPERTY_MODIFIED, &PHPPanel::OnPropertyModified, this);
    Bind(wxEVT_WVR_OBJECT_CREATED, &PHPPanel::OnObjectChange, this);
    Bind(wxEVT_WVR_OBJECT_REMOVED, &PHPPanel::OnObjectChange, this);
    Bind(wxEVT_WVR_OBJECT_SELECTED, &PHPPanel::OnObjectChange, this);
    Bind(wxEVT_WVR_EVENT_HANDLER_MODIFIED, &PHPPanel::OnEventHandlerModified, this);
}

PHPPanel::~PHPPanel()
{
    AppData()->RemoveHandler(this->GetEventHandler());
}

void PHPPanel::InitStyledTextCtrl(wxStyledTextCtrl* stc)
{
    stc->SetLexer(wxSTC_LEX_CPP);
    stc->SetKeyWords(
        0,
        "php abstract and array as break case catch cfunction \
        class clone const continue declare default do \
        else elseif enddeclare endfor endforeach \
        endif endswitch endwhile extends final for foreach function \
        global goto if implements interface instanceof \
        namespace new old_function or private protected public \
        static switch throw try use var while xor __class__ __dir__ \
        __file__ __line__ __function__ __method__ __namespace__ \
        die echo empty eval exit include include_once isset list require \
        require_once return print unset null");
}

void PHPPanel::OnFind(wxFindDialogEvent& event)
{
    m_editor->GetEventHandler()->ProcessEvent(event);
}

void PHPPanel::OnPropertyModified(wxWeaverPropertyEvent& event)
{
    event.SetId(1); // Generate code to the panel only
    OnCodeGeneration(event);
}

void PHPPanel::OnProjectRefresh(wxWeaverEvent& event)
{
    event.SetId(1); // Generate code to the panel only
    OnCodeGeneration(event);
}

void PHPPanel::OnObjectChange(wxWeaverObjectEvent& event)
{
    event.SetId(1); // Generate code to the panel only
    OnCodeGeneration(event);
}

void PHPPanel::OnEventHandlerModified(wxWeaverEventHandlerEvent& event)
{
    event.SetId(1); // Generate code to the panel only
    OnCodeGeneration(event);
}

void PHPPanel::OnCodeGeneration(wxWeaverEvent& event)
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

    // Get PHP properties from the project
    // If PHP generation is not enabled, do not generate the file
    bool doFile = false;
    PProperty pCodeGen = project->GetProperty("code_generation");
    if (pCodeGen)
        doFile = TypeConv::FlagSet("PHP", pCodeGen->GetValue()) && !panelOnly;

    if (!(doPanel || doFile))
        return;

    // Get First ID from Project File
    size_t firstID = 1000;
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
        PHPCodeGenerator codegen;
        codegen.UseRelativePath(useRelativePath, path);

        if (pFirstID)
            codegen.SetFirstID(firstID);

        codegen.SetSourceWriter(m_codeWriter);

        Freeze();

        wxStyledTextCtrl* phpEditor = m_editor->GetTextCtrl();
        phpEditor->SetReadOnly(false);
        int phpLine = phpEditor->GetFirstVisibleLine() + phpEditor->LinesOnScreen() - 1;
        int phpXOffset = phpEditor->GetXOffset();

        codegen.GenerateCode(project);

        phpEditor->SetReadOnly(true);
        phpEditor->GotoLine(phpLine);
        phpEditor->SetXOffset(phpXOffset);
        phpEditor->SetAnchor(0);
        phpEditor->SetCurrentPos(0);

        Thaw();
    }
    if (doFile) { // Generate code in the file
        try {
            PHPCodeGenerator codegen;
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

            PCodeWriter php_cw(new FileCodeWriter(path + file + ".php", useMicrosoftBOM, useUtf8));

            codegen.SetSourceWriter(php_cw);
            codegen.GenerateCode(project);
            wxLogStatus("Code generated on \'%s\'.", path.c_str());
        } catch (wxWeaverException& ex) {
            wxLogError(ex.what());
        }
    }
}
