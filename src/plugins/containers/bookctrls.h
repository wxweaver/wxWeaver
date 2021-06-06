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
#include "bookutils.h"

#include <plugin.h>
#include <xrcconv.h>

#include <ticpp.h>

#include <wx/listctrl.h>

class AuiNotebookComponent : public ComponentBase {
public:
    wxObject* Create(IObject* obj, wxObject* parent) override
    {
        wxAuiNotebook* auiNotebook = new wxAuiNotebook(
            (wxWindow*)parent, wxID_ANY,
            obj->GetPropertyAsPoint("pos"),
            obj->GetPropertyAsSize("size"),
            obj->GetPropertyAsInteger("style")
                | obj->GetPropertyAsInteger("window_style"));

        auiNotebook->SetTabCtrlHeight(obj->GetPropertyAsInteger("tab_ctrl_height"));
        auiNotebook->SetUniformBitmapSize(obj->GetPropertyAsSize("bitmapsize"));

        auiNotebook->PushEventHandler(new BooksEvtHandler(auiNotebook, GetManager()));
        return auiNotebook;
    }

    void Cleanup(wxObject* obj) override
    {
        wxAuiNotebook* auiNotebook = wxDynamicCast(obj, wxAuiNotebook);
        if (auiNotebook)
            auiNotebook->PopEventHandler(true);
    }

    ticpp::Element* ExportToXrc(IObject* obj) override
    {
        ObjectToXrcFilter xrc(obj, "wxAuiNotebook",
                              obj->GetPropertyAsString("name"));
        xrc.AddWindowProperties();
        return xrc.GetXrcObject();
    }

    ticpp::Element* ImportFromXrc(ticpp::Element* xrcObj) override
    {
        XrcToXfbFilter filter(xrcObj, "wxAuiNotebook");
        filter.AddWindowProperties();
        return filter.GetXfbObject();
    }
};

class ChoicebookComponent : public ComponentBase {
public:
    wxObject* Create(IObject* obj, wxObject* parent) override
    {
        wxChoicebook* choiceBook = new wxChoicebook(
            (wxWindow*)parent, wxID_ANY,
            obj->GetPropertyAsPoint("pos"),
            obj->GetPropertyAsSize("size"),
            obj->GetPropertyAsInteger("style")
                | obj->GetPropertyAsInteger("window_style"));

        choiceBook->PushEventHandler(new BooksEvtHandler(choiceBook, GetManager()));
        return choiceBook;
    }

    void Cleanup(wxObject* obj) override
    {
        wxChoicebook* choiceBook = wxDynamicCast(obj, wxChoicebook);
        if (choiceBook)
            choiceBook->PopEventHandler(true);
    }

    ticpp::Element* ExportToXrc(IObject* obj) override
    {
        ObjectToXrcFilter xrc(obj, "wxChoicebook",
                              obj->GetPropertyAsString("name"));
        xrc.AddWindowProperties();
        return xrc.GetXrcObject();
    }

    ticpp::Element* ImportFromXrc(ticpp::Element* xrcObj) override
    {
        XrcToXfbFilter filter(xrcObj, "wxChoicebook");
        filter.AddWindowProperties();
        return filter.GetXfbObject();
    }
};

class ListbookComponent : public ComponentBase {
public:
    wxObject* Create(IObject* obj, wxObject* parent) override
    {
        wxListbook* listBook = new wxListbook(
            (wxWindow*)parent, wxID_ANY,
            obj->GetPropertyAsPoint("pos"),
            obj->GetPropertyAsSize("size"),
            obj->GetPropertyAsInteger("style")
                | obj->GetPropertyAsInteger("window_style"));

        listBook->PushEventHandler(new BooksEvtHandler(listBook, GetManager()));
        return listBook;
    }

    void Cleanup(wxObject* obj) override
    {
        wxListbook* listBook = wxDynamicCast(obj, wxListbook);
        if (listBook)
            listBook->PopEventHandler(true);
    }
#ifndef __WXGTK__
    // TODO: Check this on Windows, it crashes in wxGTK, try with suppress events in case

    // Small icon style not supported by GTK
    void OnCreated(wxObject* wxobject, wxWindow* wxparent) override
    {
        // TODO: cast works only against wxobject on wxGTK
        wxListbook* book = wxDynamicCast(wxparent, wxListbook);
        if (book) {
            IObject* obj = GetManager()->GetIObject(wxobject);

            wxSize bmpSize = obj->GetPropertyAsSize("bitmapsize");
            int bmpWidth = bmpSize.GetWidth();
            int bmpHeight = bmpSize.GetHeight();

            // Small icon style if bitmapsize is not set
            if (bmpWidth <= 0 && bmpHeight <= 0) {
                wxListView* tmpListView = book->GetListView();
                long flags = tmpListView->GetWindowStyleFlag();
                flags = (flags & ~wxLC_ICON) | wxLC_SMALL_ICON;
                tmpListView->SetWindowStyleFlag(flags);
            }
        }
    }
#endif
    ticpp::Element* ExportToXrc(IObject* obj) override
    {
        ObjectToXrcFilter xrc(obj, "wxListbook",
                              obj->GetPropertyAsString("name"));
        xrc.AddWindowProperties();
        return xrc.GetXrcObject();
    }

    ticpp::Element* ImportFromXrc(ticpp::Element* xrcObj) override
    {
        XrcToXfbFilter filter(xrcObj, "wxListbook");
        filter.AddWindowProperties();
        return filter.GetXfbObject();
    }
};

#if 0
//defined(__WXGTK__)
/*
    TODO: Testing if this is still a problem in wx v3.0+

    Since wxGTK 2.8, wxNotebook has been sending page changed events
    in its destructor; this causes strange behavior
*/
class wxCustomNotebook : public wxNotebook {
public:
    wxCustomNotebook(wxWindow* parent, wxWindowID id,
                     const wxPoint& point = wxDefaultPosition,
                     const wxSize& size = wxDefaultSize,
                     long style = 0)
        : wxNotebook(parent, id, point, size, style)
    {
    }

    ~wxCustomNotebook() override
    {
        while (this != GetEventHandler()) {
            // Remove and delete extra event handlers
            PopEventHandler(true);
        }
    }
};
#else
typedef wxNotebook wxCustomNotebook;
#endif

class NotebookComponent : public ComponentBase {
public:
    wxObject* Create(IObject* obj, wxObject* parent) override
    {
        wxNotebook* noteBook = new wxCustomNotebook(
            (wxWindow*)parent, wxID_ANY,
            obj->GetPropertyAsPoint("pos"),
            obj->GetPropertyAsSize("size"),
            obj->GetPropertyAsInteger("style")
                | obj->GetPropertyAsInteger("window_style"));

        noteBook->PushEventHandler(new BooksEvtHandler(noteBook, GetManager()));
        return noteBook;
    }

    void Cleanup(wxObject* obj) override
    {
        wxNotebook* noteBook = wxDynamicCast(obj, wxNotebook);
        if (noteBook)
            noteBook->PopEventHandler(true);
    }

    ticpp::Element* ExportToXrc(IObject* obj) override
    {
        ObjectToXrcFilter xrc(obj, "wxNotebook",
                              obj->GetPropertyAsString("name"));
        xrc.AddWindowProperties();
        return xrc.GetXrcObject();
    }

    ticpp::Element* ImportFromXrc(ticpp::Element* xrcObj) override
    {
        XrcToXfbFilter filter(xrcObj, "wxNotebook");
        filter.AddWindowProperties();
        return filter.GetXfbObject();
    }
};

class SimplebookComponent : public ComponentBase {
public:
    wxObject* Create(IObject* obj, wxObject* parent) override
    {
        return new wxSimplebook(
            (wxWindow*)parent, wxID_ANY,
            obj->GetPropertyAsPoint("pos"),
            obj->GetPropertyAsSize("size"),
            obj->GetPropertyAsInteger("window_style"));
    }

    ticpp::Element* ExportToXrc(IObject* obj) override
    {
        ObjectToXrcFilter xrc(obj, "wxSimplebook",
                              obj->GetPropertyAsString("name"));
        xrc.AddWindowProperties();
        return xrc.GetXrcObject();
    }

    ticpp::Element* ImportFromXrc(ticpp::Element* xrcObj) override
    {
        XrcToXfbFilter filter(xrcObj, "wxSimplebook");
        filter.AddWindowProperties();
        return filter.GetXfbObject();
    }
};

class AuiNotebookPageComponent : public ComponentBase {
public:
    void OnCreated(wxObject* wxobject, wxWindow* wxparent) override
    {
        BookUtils::OnPageCreated<wxAuiNotebook>(
            wxobject, wxparent, GetManager(), "AuiNotebookPageComponent");
    }

    void OnSelected(wxObject* wxobject) override
    {
        BookUtils::OnPageSelected<wxAuiNotebook>(wxobject, GetManager());
    }

    ticpp::Element* ExportToXrc(IObject* obj) override
    {
        return BookUtils::BookPageWithImagesXrcProperties(obj, "notebookpage");
    }

    ticpp::Element* ImportFromXrc(ticpp::Element* xrcObj) override
    {
        return BookUtils::BookPageWithImagesPrjProperties(xrcObj, "notebookpage");
    }
};

class ListbookPageComponent : public ComponentBase {
public:
    void OnCreated(wxObject* wxobject, wxWindow* wxparent) override
    {
        BookUtils::OnPageCreated<wxListbook>(
            wxobject, wxparent, GetManager(), "ListbookPageComponent");
    }

    void OnSelected(wxObject* wxobject) override
    {
        BookUtils::OnPageSelected<wxListbook>(wxobject, GetManager());
    }

    ticpp::Element* ExportToXrc(IObject* obj) override
    {
        return BookUtils::BookPageWithImagesXrcProperties(obj, "listbookpage");
    }

    ticpp::Element* ImportFromXrc(ticpp::Element* xrcObj) override
    {
        return BookUtils::BookPageWithImagesPrjProperties(xrcObj, "listbookpage");
    }
};

class NotebookPageComponent : public ComponentBase {
public:
    void OnCreated(wxObject* wxobject, wxWindow* wxparent) override
    {
        BookUtils::OnPageCreated<wxNotebook>(
            wxobject, wxparent, GetManager(), "NotebookPageComponent");
    }

    void OnSelected(wxObject* wxobject) override
    {
        BookUtils::OnPageSelected<wxNotebook>(wxobject, GetManager());
    }

    ticpp::Element* ExportToXrc(IObject* obj) override
    {
        return BookUtils::BookPageWithImagesXrcProperties(obj, "notebookpage");
    }

    ticpp::Element* ImportFromXrc(ticpp::Element* xrcObj) override
    {
        return BookUtils::BookPageWithImagesPrjProperties(xrcObj, "notebookpage");
    }
};

class ChoicebookPageComponent : public ComponentBase {
public:
    void OnCreated(wxObject* wxobject, wxWindow* wxparent) override
    {
        BookUtils::OnPageCreated<wxChoicebook>(
            wxobject, wxparent, GetManager(), "ChoicebookPageComponent");
    }

    void OnSelected(wxObject* wxobject) override
    {
        BookUtils::OnPageSelected<wxChoicebook>(wxobject, GetManager());
    }

    ticpp::Element* ExportToXrc(IObject* obj) override
    {
        ObjectToXrcFilter xrc(obj, "choicebookpage");
        xrc.AddProperty("label", "label", XRC_TYPE_TEXT);
        xrc.AddProperty("select", "selected", XRC_TYPE_BOOL);
        return xrc.GetXrcObject();
    }

    ticpp::Element* ImportFromXrc(ticpp::Element* xrcObj) override
    {
        XrcToXfbFilter filter(xrcObj, "choicebookpage");
        filter.AddWindowProperties();
        filter.AddProperty("label", "label", XRC_TYPE_TEXT);
        filter.AddProperty("selected", "select", XRC_TYPE_BOOL);
        return filter.GetXfbObject();
    }
};

class SimplebookPageComponent : public ComponentBase {
public:
    void OnCreated(wxObject* wxobject, wxWindow* wxparent) override
    {
        BookUtils::OnPageCreated<wxSimplebook>(
            wxobject, wxparent, GetManager(), "SimplebookPageComponent");
    }

    void OnSelected(wxObject* wxobject) override
    {
        BookUtils::OnPageSelected<wxSimplebook>(wxobject, GetManager());
    }

    ticpp::Element* ExportToXrc(IObject* obj) override
    {
        ObjectToXrcFilter xrc(obj, "simplebookpage");
        xrc.AddProperty("label", "label", XRC_TYPE_TEXT);
        xrc.AddProperty("select", "selected", XRC_TYPE_BOOL);
        return xrc.GetXrcObject();
    }

    ticpp::Element* ImportFromXrc(ticpp::Element* xrcObj) override
    {
        XrcToXfbFilter filter(xrcObj, "simplebookpage");
        filter.AddWindowProperties();
        filter.AddProperty("label", "label", XRC_TYPE_TEXT);
        filter.AddProperty("selected", "select", XRC_TYPE_BOOL);
        return filter.GetXfbObject();
    }
};
