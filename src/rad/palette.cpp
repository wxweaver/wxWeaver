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
#include "rad/palette.h"

#include "model/objectbase.h"
#include "utils/debug.h"
#include "rad/appdata.h"
#include "rad/auibarart.h"
#include "rad/auitabart.h"

#include <wx/config.h>
#include <wx/tokenzr.h>

#include <map>
#include <vector>

#ifdef __WXMAC__
#include <wx/tooltip.h>
#endif

wxWindowID Palette::nextId = wxID_HIGHEST + 3000;

#if 0
BEGIN_EVENT_TABLE(Palette, wxPanel)
#ifdef __WXMAC__
EVT_BUTTON(wxID_ANY, Palette::OnButtonClick)
#else
EVT_TOOL(wxID_ANY, Palette::OnButtonClick)
#endif
EVT_SPIN_UP(wxID_ANY, Palette::OnSpinUp)
EVT_SPIN_DOWN(wxID_ANY, Palette::OnSpinDown)
END_EVENT_TABLE()
#endif

Palette::Palette(wxWindow* parent, int id)
    : wxPanel(parent, id)
    , m_notebook(nullptr)
{
#ifdef __WXMAC__
    Bind(wxEVT_BUTTON, &Palette::OnButtonClick, this);
#else
    Bind(wxEVT_TOOL, &Palette::OnButtonClick, this);
#endif
    Bind(wxEVT_SPIN_UP, &Palette::OnSpinUp, this);
    Bind(wxEVT_SPIN_DOWN, &Palette::OnSpinDown, this);
}

void Palette::PopulateToolbar(PObjectPackage pkg, wxAuiToolBar* toolbar)
{
    size_t j = 0;
    while (j < pkg->GetObjectCount()) {
        PObjectInfo info = pkg->GetObjectInfo(j);
        if (info->IsStartOfGroup()) {
            toolbar->AddSeparator();
        }
        if (!info->GetComponent()) {
            LogDebug(
                _("Missing Component for Class \""
                  + info->GetClassName()
                  + _("\" of Package \"")
                  + pkg->GetPackageName() + "\"."));
        } else {
            wxString widget(info->GetClassName());
            wxBitmap icon = info->GetIconFile();
#ifdef __WXMAC__
            wxBitmapButton* button = new wxBitmapButton(toolbar, nextId++, icon);
            button->SetToolTip(widget);
            toolbar->AddControl(button);
#else
            toolbar->AddTool(nextId++, widget, icon, widget);
#endif
            toolbar->Realize();
        }
        j++;
    }
}

void Palette::SaveSettings()
{
    wxConfigBase* config = wxConfigBase::Get();
    wxString pageOrder;

    for (size_t i = 0; i < m_notebook->GetPageCount(); ++i) {
        if (!pageOrder.empty())
            pageOrder.append(wxT(","));

        pageOrder.append(m_notebook->GetPageText(i));
    }
    config->Write("/MainWindow/Palette/PluginTabsOrder", pageOrder);
}

void Palette::Create()
{
    // Package count
    size_t pkg_count = AppData()->GetPackageCount();
    // Lookup map of all packages
    std::map<wxString, PObjectPackage> packages;
    // List of pages to add to the notebook in desired order
    std::vector<std::pair<wxString, PObjectPackage>> pages;
    pages.reserve(pkg_count);

    LogDebug("[Palette] Pages %d", pkg_count);

    // Fill lookup map of packages
    for (size_t i = 0; i < pkg_count; ++i) {
        auto pkg = AppData()->GetPackage(i);
        packages.insert(std::make_pair(pkg->GetPackageName(), pkg));
    }

    // Read the page order from settings and build the list of pages from it
    wxConfigBase* config = wxConfigBase::Get();
    wxStringTokenizer pageOrder(
        config->Read(
            "/MainWindow/Palette/PluginTabsOrder",
            "Forms,Layout,Common,Additional,Containers,Bars,Data,Ribbon"),
        ",");

    while (pageOrder.HasMoreTokens()) {
        const auto packageName = pageOrder.GetNextToken();
        auto package = packages.find(packageName);
        if (packages.end() == package) {
            // Plugin missing - move on
            continue;
        }
        // Add package to pages list and remove from lookup map
        pages.push_back(std::make_pair(package->first, package->second));
        packages.erase(package);
    }
    // The remaining packages from the lookup map need to be added to the page list
    for (auto& package : packages)
        pages.push_back(std::make_pair(package.first, package.second));

    packages.clear();

    wxBoxSizer* top_sizer = new wxBoxSizer(wxVERTICAL);

    m_notebook = new wxAuiNotebook(
        this, wxID_ANY, wxDefaultPosition, wxDefaultSize,
        wxAUI_NB_TOP | wxAUI_NB_SCROLL_BUTTONS | wxAUI_NB_TAB_MOVE);
    m_notebook->SetArtProvider(new AuiTabArt());

    wxSize minsize;
    for (size_t i = 0; i < pages.size(); ++i) {
        const auto& page = pages[i];

        wxPanel* panel = new wxPanel(m_notebook, wxID_ANY);
#if 0
        panel->SetBackgroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_3DFACE));
#endif
        wxBoxSizer* sizer = new wxBoxSizer(wxHORIZONTAL);

        wxAuiToolBar* toolbar = new wxAuiToolBar(
            panel, wxID_ANY, wxDefaultPosition, wxDefaultSize,
            wxAUI_TB_DEFAULT_STYLE | wxAUI_TB_OVERFLOW | wxNO_BORDER);

        toolbar->SetArtProvider(new ToolBarArt());
        toolbar->SetToolBitmapSize(wxSize(22, 22));
        PopulateToolbar(page.second, toolbar);
        m_tv.push_back(toolbar);

        sizer->Add(toolbar, 1, wxEXPAND, 0);

        panel->SetAutoLayout(true);
        panel->SetSizer(sizer);
        sizer->Fit(panel);
        sizer->SetSizeHints(panel);

        wxSize cursize = panel->GetSize();
        if (cursize.x > minsize.x)
            minsize.x = cursize.x;
        if (cursize.y > minsize.y)
            minsize.y = cursize.y + 30;

        m_notebook->AddPage(panel, page.first, false, i);
        m_notebook->SetPageBitmap(i, page.second->GetPackageIcon());
    }
#if 0
    Title* title = new Title(this, _("Widgets"));
    top_sizer->Add(title, 0, wxEXPAND, 0);
#endif
    top_sizer->Add(m_notebook, 1, wxEXPAND, 0);
    SetSizer(top_sizer);
    SetSize(minsize);
    SetMinSize(minsize);
    Layout();
    Fit();
}

void Palette::OnSpinUp(wxSpinEvent&)
{
}

void Palette::OnSpinDown(wxSpinEvent&)
{
}

void Palette::OnButtonClick(wxCommandEvent& event)
{
#ifdef __WXMAC__
    wxWindow* win = dynamic_cast<wxWindow*>(event.GetEventObject());
    if (win != 0) {
        AppData()->CreateObject(win->GetToolTip()->GetTip());
    }
#else
    for (size_t i = 0; i < m_tv.size(); i++) {
        if (m_tv[i]->GetToolIndex(event.GetId()) != wxNOT_FOUND) {
            wxString name = m_tv[i]->GetToolShortHelp(event.GetId());
            AppData()->CreateObject(name);
            return;
        }
    }
#endif
}
