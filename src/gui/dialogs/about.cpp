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
#include "gui/dialogs/about.h"

#include "appdata.h"

#include <wx/html/htmlwin.h>
#include <wx/mimetype.h>

class HtmlWindow : public wxHtmlWindow {
public:
    HtmlWindow(wxWindow* parent)
        : wxHtmlWindow(parent, wxID_ANY,
                       wxDefaultPosition, wxDefaultSize,
                       wxHW_SCROLLBAR_NEVER | wxHW_NO_SELECTION | wxRAISED_BORDER)
    {
    }

    void LaunchBrowser(const wxString& url)
    {
        wxFileType* fileType
            = wxTheMimeTypesManager->GetFileTypeFromExtension("html");
        if (!fileType) {
            wxLogError(
                "Impossible to determine the file type for extension html.\n"
                "Please edit your MIME types.");
            return;
        }
        wxString cmd;
        bool ok = fileType->GetOpenCommand(
            &cmd, wxFileType::MessageParameters(url));
        delete fileType;
        if (ok)
            wxExecute(cmd, wxEXEC_ASYNC);
    }

    void OnLinkClicked(const wxHtmlLinkInfo& link) override
    {
        ::wxLaunchDefaultBrowser(link.GetHref());
    }
};

AboutDialog::AboutDialog(wxWindow* parent, int id)
    : wxDialog(parent, id, _("About..."))
{
    wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);
    wxHtmlWindow* htmlWin = new HtmlWindow(this);
    /*
        I don't know where is the problem, but if you call SetBorders(b) with
        'b' between 0..6 it works, but if you use a bigger border,
        it doesn't fit correctly.
    */
    htmlWin->SetBorders(5);
    htmlWin->LoadFile(wxFileName(
        AppData()->GetApplicationPath() + wxFILE_SEP_PATH
        + "resources/about.html"));
#ifdef __WXMAC__
    // work around a wxMac bug
    htmlWin->SetSize(360, 400);
#else
    htmlWin->SetMinSize(wxSize(350, 400));
#endif
    mainSizer->Add(htmlWin, 1, wxEXPAND | wxALL, 5);
    mainSizer->Add(
        new wxButton(this, wxID_OK, "&OK"), 0, wxALIGN_CENTER | wxBOTTOM, 5);

    SetSizerAndFit(mainSizer);
    Center();
}
