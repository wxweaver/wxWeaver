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
#include "rad/xrcpreview/xrcpreview.h"

#include "codegen/codewriter.h"
#include "codegen/xrccg.h"
#include "model/objectbase.h"
#include "utils/annoyingdialog.h"
#include "utils/typeconv.h"
#include "utils/exception.h"

#include <wx/fs_mem.h>
#include <wx/wizard.h>
#include <wx/xrc/xmlres.h>

#define MENU_DELETE 109

#if 0
BEGIN_EVENT_TABLE(XrcPreviewPopupMenu, wxMenu)
EVT_MENU(wxID_ANY, XrcPreviewPopupMenu::OnMenuEvent)
END_EVENT_TABLE()
#endif

class XrcPreviewPopupMenu : public wxMenu {
public:
    XrcPreviewPopupMenu(wxWindow* window)
        : m_window(window)
    {
        Append(MENU_DELETE, "Close Preview");
        Bind(wxEVT_MENU, &XrcPreviewPopupMenu::OnMenuEvent, this);
    }

    void OnMenuEvent(wxCommandEvent& event)
    {
        int id = event.GetId();
        switch (id) {
        case MENU_DELETE:
            m_window->Close();
            break;
        default:
            break;
        }
    }

private:
    wxWindow* m_window;
};

#if 0
BEGIN_EVENT_TABLE(XRCPreviewEvtHandler, wxEvtHandler)
EVT_KEY_UP(XRCPreviewEvtHandler::OnKeyUp)
EVT_RIGHT_DOWN(XRCPreviewEvtHandler::OnRightDown)
EVT_CLOSE(XRCPreviewEvtHandler::OnClose)
END_EVENT_TABLE()
#endif

class XRCPreviewEvtHandler : public wxEvtHandler {
public:
    XRCPreviewEvtHandler(wxWindow* win)
        : m_window(win)
    {
        Bind(wxEVT_KEY_UP, &XRCPreviewEvtHandler::OnKeyUp, this);
        Bind(wxEVT_RIGHT_DOWN, &XRCPreviewEvtHandler::OnRightDown, this);
        Bind(wxEVT_CLOSE_WINDOW, &XRCPreviewEvtHandler::OnClose, this);
    }

protected:
    void OnKeyUp(wxKeyEvent& event)
    {
        if (event.GetKeyCode() == WXK_ESCAPE)
            m_window->Close();
    }

    void OnRightDown(wxMouseEvent& event)
    {
        wxMenu* menu = new XrcPreviewPopupMenu(m_window);
        wxPoint pos = event.GetPosition();
        m_window->PopupMenu(menu, pos.x, pos.y);
    }

    void RemoveEventHandler(wxWindow* window)
    {
        const wxWindowList& children = window->GetChildren();
        for (size_t i = 0; i < children.GetCount(); ++i)
            RemoveEventHandler(children.Item(i)->GetData());

        wxEvtHandler* handler = window->PopEventHandler();
        if (handler != this)
            delete handler;
    }

    void OnClose(wxCloseEvent&)
    {
        RemoveEventHandler(m_window);
        m_window->Destroy();
        delete this;
    }

private:
    wxWindow* m_window;
};

void XRCPreview::Show(PObjectBase form, const wxString& projectPath)
{
    AnnoyingDialog dlg(_("WARNING - For XRC Developers ONLY!!"),
                       _("The XRC language is not as powerful as C++.\n"
                         "It has limitations that will affect the GUI\n"
                         "layout. This preview will ONLY show how the\n"
                         "generated XRC will look, and it will probably\n"
                         "be different from the Designer.\n\n"
                         "If you are not using XRC, do NOT use the XRC\n"
                         "preview, it will only confuse you."),
                       wxART_WARNING,
                       AnnoyingDialog::OK_CANCEL,
                       wxID_CANCEL);

    if (wxID_CANCEL == dlg.ShowModal())
        return;

    wxString className = form->GetClassName();

    PStringCodeWriter cw(new StringCodeWriter);
    try {
        XrcCodeGenerator codegen;
        codegen.SetWriter(cw);
        codegen.GenerateCode(form);
    } catch (wxWeaverException& ex) {
        wxLogError(ex.what());
        return;
    }

    wxString workingDir = ::wxGetCwd();
    // We change the current directory so that the relative paths work properly
    if (!projectPath.IsEmpty())
        ::wxSetWorkingDirectory(projectPath);

    wxXmlResource* res = wxXmlResource::Get();
    res->InitAllHandlers();

    const std::string& data = _STDSTR(cw->GetString());
    wxMemoryFSHandler::AddFile("xrcpreview.xrc", data.c_str(), data.size());
    res->Load("memory:xrcpreview.xrc");

    wxWindow* window = NULL;
    if (className == wxT("Frame")) {
        wxFrame* frame = new wxFrame();
        res->LoadFrame(
            frame, wxTheApp->GetTopWindow(),
            form->GetPropertyAsString("name"));

        // Prevent events from propagating up to wxWeaver's frame
        frame->SetExtraStyle(frame->GetExtraStyle() | wxWS_EX_BLOCK_EVENTS);
        frame->Show();
        window = frame;
    } else if (className == "Dialog") {
        wxDialog* dialog = new wxDialog;
        res->LoadDialog(
            dialog, wxTheApp->GetTopWindow(),
            form->GetPropertyAsString(wxT("name")));

        // Prevent events from propagating up to wxWeaver's frame
        dialog->SetExtraStyle(dialog->GetExtraStyle() | wxWS_EX_BLOCK_EVENTS);
        dialog->Show();
        window = dialog;
    } else if (className == "Wizard") {
        wxString wizName = form->GetPropertyAsString("name");
        wxString pgName;
        wxObject* wizObj = res->LoadObject(nullptr, wizName, "wxWizard");
        wxWizard* wizard = wxDynamicCast(wizObj, wxWizard);
        wxWizardPageSimple* wizpge = nullptr;

        if (wizard)
            wizard->SetExtraStyle(wizard->GetExtraStyle() | wxWS_EX_BLOCK_EVENTS);

        if (form->GetChildCount() > 0) {
            pgName = form->GetChild(0)->GetPropertyAsString("name");
            wizpge = (wxWizardPageSimple*)wizard->FindWindow(pgName);
        }
        if (wizpge) {
            wizard->RunWizard(wizpge);
            wizard->Destroy();
            window = nullptr;
        }
    } else if (className == "Panel") {
        wxDialog* dialog
            = new wxDialog(wxTheApp->GetTopWindow(), wxID_ANY, "Dialog",
                           wxDefaultPosition, wxDefaultSize,
                           wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER);

        // Prevent events from propagating up to wxWeaver's frame
        dialog->SetExtraStyle(wxWS_EX_BLOCK_EVENTS);
        wxPanel* panel = new wxPanel();
        res->LoadPanel(panel, dialog, form->GetPropertyAsString("name"));
        dialog->SetClientSize(panel->GetSize());
        dialog->SetSize(form->GetPropertyAsSize(wxT("size")));
        dialog->CenterOnScreen();
        dialog->Show();
        window = dialog;
    } else if (className == "MenuBar") {
        wxFrame* frame
            = new wxFrame(nullptr, wxID_ANY, form->GetPropertyAsString("name"));

        // Prevent events from propagating up to wxWeaver's frame
        frame->SetExtraStyle(frame->GetExtraStyle() | wxWS_EX_BLOCK_EVENTS);
        frame->SetMenuBar(res->LoadMenuBar(form->GetPropertyAsString("name")));
        frame->CenterOnScreen();
        frame->Show();
        window = frame;
    } else if (className == "ToolBar") {
        wxFrame* frame
            = new wxFrame(nullptr, wxID_ANY, form->GetPropertyAsString("name"));

        // Prevent events from propagating up to wxWeaver's frame
        frame->SetExtraStyle(frame->GetExtraStyle() | wxWS_EX_BLOCK_EVENTS);
        frame->SetToolBar(res->LoadToolBar(frame, form->GetPropertyAsString("name")));
        frame->CenterOnScreen();
        frame->Show();
        window = frame;
    }
    if (window)
        AddEventHandler(window, window);

    ::wxSetWorkingDirectory(workingDir);
    res->Unload("memory:xrcpreview.xrc");
    wxMemoryFSHandler::RemoveFile("xrcpreview.xrc");
}

void XRCPreview::AddEventHandler(wxWindow* window, wxWindow* form)
{
    const wxWindowList& children = window->GetChildren();
    for (size_t i = 0; i < children.GetCount(); ++i)
        AddEventHandler(children.Item(i)->GetData(), form);

    window->PushEventHandler((new XRCPreviewEvtHandler(form)));
}
