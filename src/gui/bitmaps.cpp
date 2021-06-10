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
#include "gui/bitmaps.h"

#include "utils/stringutils.h"
#include "utils/typeconv.h"
#include "utils/exception.h"

#include <default.xpm>
#include <ticpp.h>

static std::map<wxString, wxBitmap> m_bitmaps;

wxBitmap AppBitmaps::GetBitmap(wxString iconname, int size)
{
    std::map<wxString, wxBitmap>::iterator bitmap = m_bitmaps.find(iconname);
    wxBitmap bmp;
    if (bitmap != m_bitmaps.end())
        bmp = m_bitmaps[iconname];
    else
        bmp = m_bitmaps["unknown"];

    if (size) {
        // rescale it to requested size
        if (size != bmp.GetWidth() || size != bmp.GetHeight()) {
            wxImage image = bmp.ConvertToImage();
            bmp = wxBitmap(image.Scale(size, size));
        }
    }
    return bmp;
}

void AppBitmaps::LoadBitmaps(wxString filepath, wxString iconpath)
{
    try {
        m_bitmaps["unknown"] = wxBitmap(default_xpm);

        ticpp::Document doc;
        XMLUtils::LoadXMLFile(doc, true, filepath);

        ticpp::Element* root = doc.FirstChildElement("icons");
        ticpp::Element* elem = root->FirstChildElement("icon", false);
        while (elem) {
            wxString name = _WXSTR(elem->GetAttribute("name"));
            wxString file = _WXSTR(elem->GetAttribute("file"));
            m_bitmaps[name] = wxBitmap(iconpath + file, wxBITMAP_TYPE_ANY);

            elem = elem->NextSiblingElement("icon", false);
        }
    } catch (ticpp::Exception& ex) {
        wxLogError("Error loading images: %s", _WXSTR(ex.m_details).c_str());
    } catch (wxWeaverException& ex) {
        wxLogError("Error loading images: %s", ex.what());
    }
}
