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

#include <wx/choicebk.h>
#include <wx/listbook.h>
#include <wx/notebook.h>
#include <wx/simplebook.h>
#include <wx/toolbook.h>
#include <wx/treebook.h>
#include <wx/aui/auibook.h>

#include <wx/imaglist.h>

#ifndef __WXGTK__
#include <wx/listctrl.h>
#endif

#include <vector>

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

#if 0
class BooksEvtHandler : public wxEvtHandler {
public:
    void OnPageChanged(wxBookCtrlEvent& /*event*/)
    {
        /*  FIXME: This doesn't work correctly:

            The desired behavior should be that when clicking on the `select`
            page property checkbox, the related property in other pages should
            be swapped, but this should be done in the property grid
            "OnPropertyChanged" event handler, not here.

            What this is trying to do is to change the `select` property
            when clicking on a book tab, which is wrong because is not intentional
            from an user to change the selection property when editing a page.
        */
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

        wxBookCtrlBase* book = wxDynamicCast(m_window, wxBookCtrlBase);
        if (!book) {
            // FIXME: wxAuiNotebook fails when casting to wxBookCtrlBase (wx v3.0.5.1)
            book = wxDynamicCast(m_window, wxAuiNotebook);
        }
        size_t count = m_manager->GetChildCount(m_window);
        SuppressEventHandlers suppress(book);

        for (size_t i = 0; i < count; i++) {

            wxObject* page = m_manager->GetChild(m_window, i);
            IObject* pageObject = m_manager->GetIObject(page);

            if (pageObject) {
                if (selPage == (int)i && !pageObject->GetPropertyAsInteger("select"))
                    m_manager->ModifyProperty(page, "select", "1", false);

                else if ((int)i != selPage && pageObject->GetPropertyAsInteger("select"))
                    m_manager->ModifyProperty(page, "select", "0", false);
            }
        }
        // Select the corresponding panel in the object tree
        if (book)
            m_manager->SelectObject(book->GetPage(selPage));

        event.Skip();
    }
};
#endif

namespace BookCtrl {

void AssignImageList(wxBookCtrlBase* book, IObject* bookObject)
{
    /* Using page bitmaps
       When using an imagelist object this will be replaced with the other.
    */
    wxSize bmpSize = bookObject->GetPropertyAsSize("bitmapsize");
    int bmpWidth = bmpSize.GetWidth();
    int bmpHeight = bmpSize.GetHeight();
    if (bmpWidth > 0 && bmpHeight > 0)
        book->AssignImageList(new wxImageList(bmpWidth, bmpHeight));
}

template <class T>
void OnPageCreated(wxObject* page, wxWindow* bookWindow,
                   IManager* manager, wxString name)
{
    IObject* bookObject = manager->GetIObject(bookWindow);
    if (!bookObject) {
        wxLogError("%s's parent is missing its wxWeaver object", name.c_str());
        return;
    }
    // Easy read-only property access
    IObject* pageObject = manager->GetIObject(page);
    T* book = wxDynamicCast(bookWindow, T);
#if 0
    // This wouldn't compile in MinGW - strange
    wxWindow* pageWindow = wxDynamicCast(manager->GetChild(page, 0), wxWindow);

    // Do this instead
    wxObject* pageContent = manager->GetChild(page, 0);
    wxWindow* pageWindow = nullptr;
    if (pageContent->IsKindOf(wxCLASSINFO(wxWindow)))
        pageWindow = wxDynamicCast(pageContent, wxWindow);
#else
    // TODO: Check on MinGW
    wxObject* pageContent = manager->GetChild(page, 0);
    wxWindow* pageWindow = wxDynamicCast(pageContent, wxWindow);
#endif
    // Error checking
    if (!(pageObject && book && pageWindow)) {
        wxLogError(
            "%s is missing its wxWeaver object(%p), its parent(%p), or its child(%p)",
            name.c_str(), pageObject, book, pageWindow);
        return;
    }
    // Prevent event handling by wxWeaver, these aren't user generated events
    SuppressEventHandlers suppress(book);

    // Handle wxToolbook and wxTreebook special case later
    wxString pageClassName = pageObject->GetClassName();
    wxString label = pageObject->GetPropertyAsString("label");

    if (pageClassName != "toolbookpage" && pageClassName != "treebookpage")
        book->AddPage(pageWindow, label);

    // Skip imagelist where not used
    if (pageClassName == "simplebookpage" || pageClassName == "choicebookpage")
        return;

    wxImageList* imageList = book->GetImageList();
    if (!imageList)
        return;

    // Using page bitmaps
    wxSize bmpSize = bookObject->GetPropertyAsSize("bitmapsize");
    int imageIndex = pageObject->GetPropertyAsInteger("image");
    int bmpHeight = bmpSize.GetHeight();
    int bmpWidth = bmpSize.GetWidth();

    // Invalid image is returned with a valid "unknown" image anyway
    // pageBitmap.Ok() is always true
    if (bmpWidth > 0 && bmpHeight > 0) {
        wxImage image = pageObject->GetPropertyAsBitmap("bitmap").ConvertToImage();
        imageIndex = imageList->Add(image.Scale(bmpWidth, bmpHeight));
    }
    if (pageClassName == "toolbookpage") {
        book->AddPage(pageWindow, label, false, imageIndex);
    } else if (pageClassName == "treebookpage") {
        wxTreebook* treeBook = wxDynamicCast(bookWindow, wxTreebook);
        if (!treeBook) {
            // TODO: LogError
            return;
        }
        size_t depth = pageObject->GetPropertyAsInteger("depth");
#if 0
        // FIXME or REMOVE
        if (depth > treeBook->GetPageCount()) {
            LogError("TreebookPageComponent has an invalid depth.");
            manager->ModifyProperty(page, "depth", "0", false);
        }
#endif
        // TODO: Set a different value in cpp/pythoncode than $depth - 1
        if (depth > 0)
            treeBook->InsertSubPage(depth - 1, pageWindow, label, false, imageIndex);
        else
            treeBook->AddPage(pageWindow, label, false, imageIndex);
    } else {
        book->SetPageImage(book->GetPageCount() - 1, imageIndex);
    }
}

/* Selects the corresponding page on the bookctrl when selected on the object tree.

    This might be a non template function using `wxBookCtrlBase` instead,
    but due a 3.0.x bug wxAuiNotebook is not working in that case.
    At least a wxDynamicCast can be avoided for the page.
    As consequence
    ```
    int selection = book->FindPage(pageContent);
    ```
    doesn't work either for wxAuiNotebook, so we use a loop instead to find a page,
    a `wxDynamicCast` to `wxWindow` can also be avoided there as well.
*/
template <class T>
void OnPageSelected(wxObject* page, IManager* manager)
{
    // Get the actual page, first (and only) child of notebookpage wxObject
    wxObject* pageContent = manager->GetChild(page, 0);
    T* book = wxDynamicCast(manager->GetParent(page), T);
    if (!book || !pageContent)
        return;

    for (size_t selection = 0; selection < book->GetPageCount(); ++selection) {
        if (pageContent == book->GetPage(selection)) {
            SuppressEventHandlers suppress(book);
            book->SetSelection(selection);
        }
    }
}

/* Selects the corresponding object in the tree when clicking on a bookctrl page.

    Basically the inverse of the above. Here also the same template "trick"
    because the wxDynamicCast and the missing cast for wxAuiNotebook.
*/
template <class T>
void OnPageChanged(wxBookCtrlEvent& event, IManager* manager)
{
    T* book = wxDynamicCast(event.GetEventObject(), T);
    if (!book)
        return;

    manager->SelectObject(book->GetPage(event.GetSelection()));
    event.Skip();
}

void OnAuiNotebookPageClosed(wxAuiNotebookEvent& event)
{
    wxMessageBox(
        "wxAuiNotebook pages can normally be closed.\n"
        "However, it is difficult to design a page that has been closed, "
        "so this action has been vetoed.",
        "Page Close Vetoed!", wxICON_INFORMATION, nullptr);

    event.Veto();
}

void OnAuiNotebookAllowDND(wxAuiNotebookEvent& event)
{
    wxMessageBox(
        "wxAuiNotebook pages can be dragged to other wxAuiNotebooks if the "
        "wxEVT_COMMAND_AUINOTEBOOK_ALLOW_DND event is caught and allowed.\n"
        "However, it is difficult to design a page that has been moved, "
        "so this action was not allowed.",
        "Page Move Not Allowed!", wxICON_INFORMATION, nullptr);

    event.Veto();
}

ticpp::Element* PageToXrc(IObject* object,
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

    if (className == "treebookpage") {
        xrc.AddProperty("depth", "depth", XRC_TYPE_INTEGER);
        xrc.AddProperty("expanded", "expanded", XRC_TYPE_BOOL);
    }
    return xrc.GetXrcObject();
}

ticpp::Element* PageToProject(ticpp::Element* xrcObject,
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

    if (className == "treebookpage") {
        filter.AddProperty("depth", "depth", XRC_TYPE_INTEGER);
        filter.AddProperty("expanded", "expanded", XRC_TYPE_BOOL);
    }
    return filter.GetXfbObject();
}

#ifndef __WXGTK__
void OnListbookCreated(wxObject* wxobject, wxWindow* wxparent, IManager* manager)
{
    // TODO: Check this on Windows, it crashes in wxGTK, try with suppress events in case
    // Small icon style not supported by wxGTK?

    // TODO: cast works only against wxobject on wxGTK
    wxListbook* listBook = wxDynamicCast(wxparent, wxListbook);
    if (!listBook)
        return;

    IObject* object = manager->GetIObject(wxobject);
    wxSize bmpSize = object->GetPropertyAsSize("bitmapsize");
    int bmpWidth = bmpSize.GetWidth();
    int bmpHeight = bmpSize.GetHeight();

    // Small icon style if bitmapsize is not set
    if (bmpWidth <= 0 && bmpHeight <= 0) {
        wxListView* tmpListView = listBook->GetListView();
        long flags = tmpListView->GetWindowStyleFlag();
        flags = (flags & ~wxLC_ICON) | wxLC_SMALL_ICON;
        tmpListView->SetWindowStyleFlag(flags);
    }
}
#endif

/* Expands all `expanded` pages with depth 0.

    TODO: This should be recursive.
*/
void OnTreebookCreated(wxObject* wxobject, IManager* manager)
{
    wxTreebook* treeBook = wxDynamicCast(wxobject, wxTreebook);
    if (!treeBook) {
        wxLogDebug("wxTreebook is NULL!");
        return;
    }
    int count = manager->GetChildCount(treeBook);
    wxObject* page;
    IObject* pageObj;

    int pageIndex = 0;
    for (int i = 0; i < count; i++) {

        page = manager->GetChild(treeBook, i);
        pageObj = manager->GetIObject(page);

        if (pageObj->GetClassName() == "treebookpage") {
            if (pageObj->GetPropertyAsString("expanded") != "0")
                treeBook->ExpandNode(pageIndex, true);

            pageIndex++;
        }
    }
}
} // namespace BookCtrl
