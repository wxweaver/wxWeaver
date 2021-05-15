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
#pragma once

#include <component.h>
#include <default.xpm>

#ifdef wxUSE_COLLPANE
#include <wx/collpane.h>
#endif
#include <wx/listbook.h>
#include <wx/choicebk.h>
#include <wx/simplebook.h>
#include <wx/aui/auibook.h>

#include <vector>

class SuppressEventHandlers {
private:
    std::vector<wxEvtHandler*> m_handlers;
    wxWindow* m_window;

public:
    SuppressEventHandlers(wxWindow* window)
        : m_window(window)
    {
        while (window->GetEventHandler() != window)
            m_handlers.push_back(window->PopEventHandler());
    }

    ~SuppressEventHandlers()
    {
        std::vector<wxEvtHandler*>::reverse_iterator handler;
        for (handler = m_handlers.rbegin(); handler != m_handlers.rend(); ++handler)
            m_window->PushEventHandler(*handler);
    }
};

namespace BookUtils {
template <class T>
void AddImageList(IObject* obj, T* book)
{
    if (!obj->GetPropertyAsString("bitmapsize").empty()) {
        wxSize imageSize = obj->GetPropertyAsSize("bitmapsize");
        wxImageList* images = new wxImageList(imageSize.GetWidth(), imageSize.GetHeight());
        wxImage image = wxBitmap(default_xpm).ConvertToImage();
        images->Add(image.Scale(imageSize.GetWidth(), imageSize.GetHeight()));
        book->AssignImageList(images);
    }
}

template <class T>
void OnCreated(wxObject* wxobject, wxWindow* wxparent,
               IManager* manager, wxString name)
{
    // Easy read-only property access
    IObject* obj = manager->GetIObject(wxobject);
    T* book = wxDynamicCast(wxparent, T);
#if 0
    // This wouldn't compile in MinGW - strange
    wxWindow* page = wxDynamicCast(manager->GetChild( wxobject, 0 ), wxWindow);

    // Do this instead
#endif
    wxObject* child = manager->GetChild(wxobject, 0);
    wxWindow* page = nullptr;
    if (child->IsKindOf(wxCLASSINFO(wxWindow)))
        page = (wxWindow*)child;

    // Error checking
    if (!(obj && book && page)) {
        wxLogError(
            "%s is missing its wxWeaver object(%p), its parent(%p), or its child(%p)",
            name.c_str(), obj, book, page);
        return;
    }
    // TODO: remove this by replacing static event tables with Bind().
    // Prevent event handling by wxWeaver, these aren't user generated events
    SuppressEventHandlers suppress(book);

    int selection = book->GetSelection(); // Save selection
    book->AddPage(page, obj->GetPropertyAsString("label"));

    IObject* parentObj = manager->GetIObject(wxparent);
    if (!parentObj) {
        wxLogError("%s's parent is missing its wxWeaver object", name.c_str());
        return;
    }
    if (!parentObj->GetPropertyAsString("bitmapsize").empty()
        && !obj->GetPropertyAsString("bitmap").empty()) {

        wxSize imageSize = parentObj->GetPropertyAsSize("bitmapsize");
        int width = imageSize.GetWidth();
        int height = imageSize.GetHeight();
        if (width > 0 && height > 0) {
            wxImageList* imageList = book->GetImageList();
            if (imageList) {
                wxImage image
                    = obj->GetPropertyAsBitmap(_("bitmap")).ConvertToImage();
                imageList->Add(image.Scale(width, height));
                book->SetPageImage( // Apply image to page
                    book->GetPageCount() - 1, imageList->GetImageCount() - 1);
            }
        }
    }
    if (obj->GetPropertyAsString("select") == "0" && selection >= 0)
        book->SetSelection(selection);
    else
        book->SetSelection(book->GetPageCount() - 1);
}

template <class T>
void OnSelected(wxObject* wxobject, IManager* manager)
{
    wxObject* page = manager->GetChild(wxobject, 0); // Get actual page, first child
    T* book = wxDynamicCast(manager->GetParent(wxobject), T);
    if (!book || !page)
        return;

    for (size_t i = 0; i < book->GetPageCount(); ++i) {
        if (page == book->GetPage(i)) {
            SuppressEventHandlers suppress(book); // Prevent infinite event loop
            book->SetSelection(i);                // Select Page
        }
    }
}
} // namespace BookUtils
