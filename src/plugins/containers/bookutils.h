/*
    wxWeaver - A GUI Designer Editor for wxWidgets.
    Copyright (C) 2005 José Antonio Hurtado
    Copyright (C) 2005 Juan Antonio Ortega
    Copyright (C) 2005 Ryan Mulder (as wxFormBuilder)
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
#include <xrcconv.h>

#include <ticpp.h>

#include <wx/listbook.h>
#include <wx/choicebk.h>
#include <wx/simplebook.h>
#include <wx/aui/auibook.h>

#include <vector>

#if 0
BEGIN_EVENT_TABLE(BooksEvtHandler, wxEvtHandler)
EVT_NOTEBOOK_PAGE_CHANGED(wxID_ANY, BooksEvtHandler::OnNotebookPageChanged)
EVT_LISTBOOK_PAGE_CHANGED(wxID_ANY, BooksEvtHandler::OnListbookPageChanged)
EVT_CHOICEBOOK_PAGE_CHANGED(wxID_ANY, BooksEvtHandler::OnChoicebookPageChanged)
EVT_AUINOTEBOOK_PAGE_CHANGED(wxID_ANY, BooksEvtHandler::OnAuiNotebookPageChanged)
EVT_AUINOTEBOOK_PAGE_CLOSE(wxID_ANY, BooksEvtHandler::OnAuiNotebookPageClosed)
EVT_AUINOTEBOOK_ALLOW_DND(wxID_ANY, BooksEvtHandler::OnAuiNotebookAllowDND)
END_EVENT_TABLE()
#endif

class BooksEvtHandler : public wxEvtHandler {
public:
    BooksEvtHandler(wxWindow* win, IManager* manager)
        : m_window(win)
        , m_manager(manager)
    {
        Bind(wxEVT_NOTEBOOK_PAGE_CHANGED,
             &BooksEvtHandler::OnBookPageChanged, this);

        Bind(wxEVT_LISTBOOK_PAGE_CHANGED,
             &BooksEvtHandler::OnBookPageChanged, this);

        Bind(wxEVT_CHOICEBOOK_PAGE_CHANGED,
             &BooksEvtHandler::OnBookPageChanged, this);

        Bind(wxEVT_AUINOTEBOOK_PAGE_CHANGED,
             &BooksEvtHandler::OnBookPageChanged, this);

        Bind(wxEVT_AUINOTEBOOK_PAGE_CLOSE,
             &BooksEvtHandler::OnAuiNotebookPageClosed, this);

        Bind(wxEVT_AUINOTEBOOK_ALLOW_DND,
             &BooksEvtHandler::OnAuiNotebookAllowDND, this);
    }

protected:
    void OnNotebookPageChanged(wxBookCtrlEvent& event);
    void OnListbookPageChanged(wxBookCtrlEvent& event);
    void OnChoicebookPageChanged(wxBookCtrlEvent& event);
    void OnAuiNotebookPageChanged(wxAuiNotebookEvent& event);

    void OnBookPageChanged(wxBookCtrlEvent& event)
    {
        /*
            TODO: Avoid this by using ids if possible.
            Only handle events from this book: prevents problems with nested books,
            because OnSelected is fired on an object and all of its parents
        */
        if (m_window != event.GetEventObject())
            return;

        int selPage = event.GetSelection();
        if (selPage < 0)
            return;

        size_t count = m_manager->GetChildCount(m_window);
        for (size_t i = 0; i < count; i++) {

            // TODO: use wxobject and iobject variables for all components
            wxObject* wxchild = m_manager->GetChild(m_window, i);
            IObject* iChild = m_manager->GetIObject(wxchild);

            if (iChild) {
                if (selPage == (int)i && !iChild->GetPropertyAsInteger("select"))
                    m_manager->ModifyProperty(wxchild, "select", "1", false);

                else if ((int)i != selPage && iChild->GetPropertyAsInteger("select"))
                    m_manager->ModifyProperty(wxchild, "select", "0", false);
            }
        }
        // Select the corresponding panel in the object tree
        wxBookCtrlBase* book = wxDynamicCast(m_window, wxBookCtrlBase);
        if (book)
            m_manager->SelectObject(book->GetPage(selPage));

        event.Skip();
    }

    void OnAuiNotebookPageClosed(wxAuiNotebookEvent& event)
    {
        wxMessageBox(
            "wxAuiNotebook pages can normally be closed.\n"
            "However, it is difficult to design a page that has been closed,"
            "so this action has been vetoed.",
            "Page Close Vetoed!", wxICON_INFORMATION, nullptr);

        event.Veto();
    }

    void OnAuiNotebookAllowDND(wxAuiNotebookEvent& event)
    {
        wxMessageBox(
            "wxAuiNotebook pages can be dragged to other wxAuiNotebooks if the"
            "wxEVT_COMMAND_AUINOTEBOOK_ALLOW_DND event is caught and allowed.\n"
            "However, it is difficult to design a page that has been moved,"
            "so this action was not allowed.",
            "Page Move Not Allowed!", wxICON_INFORMATION, nullptr);

        event.Veto();
    }

private:
    wxWindow* m_window;
    IManager* m_manager;
};

class SuppressEventHandlers {
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

private:
    std::vector<wxEvtHandler*> m_handlers;
    wxWindow* m_window;
};

namespace BookUtils {

ticpp::Element* BookPageWithImagesXrcProperties(IObject* object,
                                                const wxString& className)
{
    ObjectToXrcFilter xrc(object, className);
    xrc.AddProperty("label", "label", XRC_TYPE_TEXT);
    xrc.AddProperty("select", "selected", XRC_TYPE_BOOL);

    wxString bmpProp = object->GetPropertyAsString("bitmap");
    wxString filename = bmpProp.AfterFirst(';');
    bool haveBitmap = !bmpProp.empty() && !(filename.Trim() == "");
    if (haveBitmap)
        xrc.AddProperty("bitmap", "bitmap", XRC_TYPE_BITMAP);
    else
        xrc.AddProperty("image", "image", XRC_TYPE_INTEGER);

    return xrc.GetXrcObject();
}

ticpp::Element* BookPageWithImagesPrjProperties(ticpp::Element* xrcObject,
                                                const wxString& className)
{
    XrcToXfbFilter filter(xrcObject, className);
    filter.AddWindowProperties();
    filter.AddProperty("label", "label", XRC_TYPE_TEXT);
    filter.AddProperty("selected", "select", XRC_TYPE_BOOL);
#if 0
        std::string text = xrcObject->GetText(false);
        bool isArt = xrcObject->HasAttribute("stock_id");
        if (text != "" || isArt)
#endif
    filter.AddProperty("bitmap", "bitmap", XRC_TYPE_BITMAP);
#if 0
        else
#endif
    filter.AddProperty("image", "image", XRC_TYPE_INTEGER);

    return filter.GetXfbObject();
}

template <class T>
void OnPageCreated(wxObject* wxobject, wxWindow* wxparent,
                   IManager* manager, wxString name)
{
    // Easy read-only property access
    IObject* obj = manager->GetIObject(wxobject);
    T* book = wxDynamicCast(wxparent, T);
#if 0
    // This wouldn't compile in MinGW - strange
    wxWindow* page = wxDynamicCast(manager->GetChild(wxobject, 0), wxWindow);

    // Do this instead
#else
    wxObject* child = manager->GetChild(wxobject, 0);
    wxWindow* page = nullptr;
    if (child->IsKindOf(wxCLASSINFO(wxWindow)))
        page = (wxWindow*)child;
#endif
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

    book->AddPage(page, obj->GetPropertyAsString("label"));

    IObject* parentObj = manager->GetIObject(wxparent);
    if (!parentObj) {
        wxLogError("%s's parent is missing its wxWeaver object", name.c_str());
        return;
    }
    // Stop here if not using bitmaps
    if (parentObj->GetClassName() == "wxChoicebook"
        || parentObj->GetClassName() == "wxSimplebook")
        return;

    // TODO: IManager::FindChildObjectByName(const wxString&)
    // Using wxImageList object
    wxImageList* imageList = nullptr;
    for (int i = 0; i < manager->GetChildCount(book); i++) {
        wxObject* childWxObj = manager->GetChild(book, i);
        IObject* childIObj = manager->GetIObject(childWxObj);
        if (!childWxObj || !childIObj)
            continue;

        if (childIObj->GetClassName() == "wxImageList") {
            imageList = wxDynamicCast(childWxObj, wxImageList);
            if (imageList)
                break;
        }
    }
    if (imageList) {
        if (!book->GetImageList())
            book->AssignImageList(imageList);

        int imageIndex = obj->GetPropertyAsInteger("image");
        if (imageIndex < imageList->GetImageCount())
            book->SetPageImage(book->GetPageCount() - 1, imageIndex);

        return;
    }
    // Using page bitmaps
    wxSize bmpSize = parentObj->GetPropertyAsSize("bitmapsize");
    int bmpWidth = bmpSize.GetWidth();
    int bmpHeight = bmpSize.GetHeight();

    if (bmpWidth > 0 && bmpHeight > 0
        && !obj->GetPropertyAsString("bitmap").empty()) {

        imageList = book->GetImageList();
        if (!imageList) {
            imageList = new wxImageList(bmpWidth, bmpHeight);
            book->AssignImageList(imageList);
        }
        wxImage image = obj->GetPropertyAsBitmap("bitmap").ConvertToImage();
        if (!image.Ok())
            image = wxBitmap(default_xpm).ConvertToImage();

        imageList->Add(image.Scale(bmpWidth, bmpHeight));
        book->SetPageImage(book->GetPageCount() - 1, imageList->GetImageCount() - 1);
    }
}

template <class T>
void OnPageSelected(wxObject* wxobject, IManager* manager)
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
