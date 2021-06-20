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
#include "gui/panels/palette.h"

#include "model/objectbase.h"
#include "utils/debug.h"
#include "appdata.h"
#include "gui/aui/barart.h"
#include "gui/aui/tabart.h"

#include <wx/config.h>
#include <wx/tokenzr.h>

#include <map>
#include <vector>

#ifdef __WXOSX__
#include <wx/tooltip.h>
#endif

Palette::Palette(wxWindow* parent, int id)
    : wxPanel(parent, id)
    , m_notebook(nullptr)
{
#ifdef __WXOSX__
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
                "Missing Component for Class \""
                + info->GetClassName()
                + "\" of Package \""
                + pkg->GetPackageName() + "\".");
        } else {
            wxString widget(info->GetClassName());
            wxBitmap icon = info->GetIconFile();
#ifdef __WXOSX__
            wxBitmapButton* button = new wxBitmapButton(toolbar, wxID_ANY, icon);
            button->SetToolTip(widget);
            toolbar->AddControl(button);
#else
            toolbar->AddTool(wxID_ANY, widget, icon, widget);
#endif
            toolbar->Realize();
        }
        j++;
    }
}

void Palette::SaveSettings()
{
    wxString pageOrder;

    for (size_t index = 0; index < m_notebook->GetPageCount(); ++index) {
        wxString translated = m_notebook->GetPageText(index);
        wxString untranslated;

        for (auto it = m_pkgNames.begin(); it != m_pkgNames.end(); ++it) {
            untranslated = *it;
            if (translated == _(untranslated)) {
                if (!pageOrder.empty())
                    pageOrder.append(',');

                pageOrder.append(untranslated);
                break;
            }
        }
    }
    wxConfigBase::Get()->Write("/Palette/TabOrder", pageOrder);
}

void Palette::Create()
{
    size_t pkgCount = AppData()->GetPackageCount();
    typedef std::map<wxString, PObjectPackage> PackageMap;
    typedef std::pair<wxString, PObjectPackage> PackagePair;

    PackageMap packages;
    // List of pages to add to the notebook in desired order
    std::vector<PackagePair> pages;

    pages.reserve(pkgCount);

    LogDebug("[Palette] Pages %zd", pkgCount);

    // Fill lookup map of packages
    for (size_t i = 0; i < pkgCount; ++i) {
        PObjectPackage pkg = AppData()->GetPackage(i);
        packages.insert(std::make_pair(pkg->GetPackageName(), pkg));
    }

    // Read the page order from settings and build the list of pages from it
    wxStringTokenizer pageOrder(
        wxConfigBase::Get()->Read("/Palette/TabOrder"), ',');

    while (pageOrder.HasMoreTokens()) {
        wxString packageName = pageOrder.GetNextToken();
        PackageMap::const_iterator package = packages.find(packageName);
        if (packages.end() == package) {
            // Plugin missing - move on
            continue;
        }
        // Add package to pages list and remove from lookup map
        pages.emplace_back(std::make_pair(package->first, package->second));
        packages.erase(package);
    }
    // The remaining packages from the lookup map need to be added to the page list
    for (PackagePair package : packages)
        pages.emplace_back(std::make_pair(package.first, package.second));

    packages.clear();

    m_notebook = new wxAuiNotebook(
        this, wxID_ANY, wxDefaultPosition, wxDefaultSize,
        wxAUI_NB_TOP | wxAUI_NB_SCROLL_BUTTONS | wxAUI_NB_TAB_MOVE);

    m_notebook->SetArtProvider(new AuiTabArt());

    wxSize minsize;
    for (size_t i = 0; i < pages.size(); ++i) {

        wxPanel* panel = new wxPanel(m_notebook, wxID_ANY);
#if 0
        panel->SetBackgroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_3DFACE));
#endif
        wxAuiToolBar* toolbar = new wxAuiToolBar(
            panel, wxID_ANY, wxDefaultPosition, wxDefaultSize,
            wxAUI_TB_DEFAULT_STYLE | wxAUI_TB_OVERFLOW | wxNO_BORDER);

        toolbar->SetArtProvider(new ToolBarArt());
        toolbar->SetToolBitmapSize(wxSize(22, 22));

        PackagePair& page = pages[i];
        wxString pageName = page.first;
        PObjectPackage pkg = page.second;

        PopulateToolbar(pkg, toolbar);

        m_toolbars.push_back(toolbar);
        m_pkgNames.insert(pageName);

        wxBoxSizer* sizer = new wxBoxSizer(wxHORIZONTAL);
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

        m_notebook->AddPage(panel, _(pageName), false, i);
        m_notebook->SetPageBitmap(i, pkg->GetPackageIcon());
    }
#if 0
    Title* title = new Title(this, _("Widgets"));
    top_sizer->Add(title, 0, wxEXPAND, 0);
#endif
    wxBoxSizer* topSizer = new wxBoxSizer(wxVERTICAL);
    topSizer->Add(m_notebook, 1, wxEXPAND, 0);
    SetSizer(topSizer);
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
#ifdef __WXOSX__
    wxWindow* win = dynamic_cast<wxWindow*>(event.GetEventObject());
    if (win != 0) {
        AppData()->CreateObject(win->GetToolTip()->GetTip());
    }
#else
    for (size_t i = 0; i < m_toolbars.size(); i++) {
        if (m_toolbars[i]->GetToolIndex(event.GetId()) != wxNOT_FOUND) {
            wxString name = m_toolbars[i]->GetToolShortHelp(event.GetId());
            AppData()->CreateObject(name);
            return;
        }
    }
#endif
}
