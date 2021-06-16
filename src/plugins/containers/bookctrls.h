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

#include <functional>

class AuiNotebookComponent : public ComponentBase {
public:
    wxObject* Create(IObject* object, wxObject* parent) override
    {
        wxAuiNotebook* auiNotebook = new wxAuiNotebook(
            (wxWindow*)parent, wxID_ANY,
            object->GetPropertyAsPoint("pos"),
            object->GetPropertyAsSize("size"),
            object->GetPropertyAsInteger("style")
                | object->GetPropertyAsInteger("window_style"));

        auiNotebook->SetTabCtrlHeight(object->GetPropertyAsInteger("tab_ctrl_height"));
        auiNotebook->SetUniformBitmapSize(object->GetPropertyAsSize("bitmapsize"));

        BookCtrl::AssignImageList(auiNotebook, object);

        auiNotebook->Bind(wxEVT_AUINOTEBOOK_ALLOW_DND,
                          &BookCtrl::OnAuiNotebookAllowDND);

        auiNotebook->Bind(wxEVT_AUINOTEBOOK_PAGE_CLOSE,
                          &BookCtrl::OnAuiNotebookPageClosed);

        std::function<void(wxBookCtrlEvent&)> onPageChanged(
            std::bind(&BookCtrl::OnPageChanged<wxAuiNotebook>,
                      std::placeholders::_1, GetManager()));

        auiNotebook->Bind(wxEVT_AUINOTEBOOK_PAGE_CHANGED, onPageChanged);

        return auiNotebook;
    }

    ticpp::Element* ExportToXrc(IObject* object) override
    {
        ObjectToXrcFilter xrc(object, "wxAuiNotebook",
                              object->GetPropertyAsString("name"));
        xrc.AddWindowProperties();
        return xrc.GetXrcObject();
    }

    ticpp::Element* ImportFromXrc(ticpp::Element* xrcObject) override
    {
        XrcToXfbFilter filter(xrcObject, "wxAuiNotebook");
        filter.AddWindowProperties();
        return filter.GetXfbObject();
    }
};

class ChoicebookComponent : public ComponentBase {
public:
    wxObject* Create(IObject* object, wxObject* parent) override
    {
        wxChoicebook* choiceBook = new wxChoicebook(
            (wxWindow*)parent, wxID_ANY,
            object->GetPropertyAsPoint("pos"),
            object->GetPropertyAsSize("size"),
            object->GetPropertyAsInteger("style")
                | object->GetPropertyAsInteger("window_style"));

        std::function<void(wxBookCtrlEvent&)> onPageChanged(
            std::bind(&BookCtrl::OnPageChanged<wxChoicebook>,
                      std::placeholders::_1, GetManager()));

        choiceBook->Bind(wxEVT_CHOICEBOOK_PAGE_CHANGED, onPageChanged);

        return choiceBook;
    }

    ticpp::Element* ExportToXrc(IObject* object) override
    {
        ObjectToXrcFilter xrc(object, "wxChoicebook",
                              object->GetPropertyAsString("name"));
        xrc.AddWindowProperties();
        return xrc.GetXrcObject();
    }

    ticpp::Element* ImportFromXrc(ticpp::Element* xrcObject) override
    {
        XrcToXfbFilter filter(xrcObject, "wxChoicebook");
        filter.AddWindowProperties();
        return filter.GetXfbObject();
    }
};

class ListbookComponent : public ComponentBase {
public:
    wxObject* Create(IObject* object, wxObject* parent) override
    {
        wxListbook* listBook = new wxListbook(
            (wxWindow*)parent, wxID_ANY,
            object->GetPropertyAsPoint("pos"),
            object->GetPropertyAsSize("size"),
            object->GetPropertyAsInteger("style")
                | object->GetPropertyAsInteger("window_style"));

        BookCtrl::AssignImageList(listBook, object);

        std::function<void(wxBookCtrlEvent&)> onPageChanged(
            std::bind(&BookCtrl::OnPageChanged<wxListbook>,
                      std::placeholders::_1, GetManager()));

        listBook->Bind(wxEVT_LISTBOOK_PAGE_CHANGED, onPageChanged);

        return listBook;
    }

#ifndef __WXGTK__
    void OnCreated(wxObject* wxobject, wxWindow* wxparent) override
    {
        BookCtrl::OnListbookCreated(wxobject, wxparent, GetManager());
    }
#endif

    ticpp::Element* ExportToXrc(IObject* object) override
    {
        ObjectToXrcFilter xrc(object, "wxListbook",
                              object->GetPropertyAsString("name"));
        xrc.AddWindowProperties();
        return xrc.GetXrcObject();
    }

    ticpp::Element* ImportFromXrc(ticpp::Element* xrcObject) override
    {
        XrcToXfbFilter filter(xrcObject, "wxListbook");
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
    wxObject* Create(IObject* object, wxObject* parent) override
    {
        wxNotebook* noteBook = new wxCustomNotebook(
            (wxWindow*)parent, wxID_ANY,
            object->GetPropertyAsPoint("pos"),
            object->GetPropertyAsSize("size"),
            object->GetPropertyAsInteger("style")
                | object->GetPropertyAsInteger("window_style"));

        BookCtrl::AssignImageList(noteBook, object);

        std::function<void(wxBookCtrlEvent&)> onPageChanged(
            std::bind(&BookCtrl::OnPageChanged<wxNotebook>,
                      std::placeholders::_1, GetManager()));

        noteBook->Bind(wxEVT_NOTEBOOK_PAGE_CHANGED, onPageChanged);

        return noteBook;
    }

    ticpp::Element* ExportToXrc(IObject* object) override
    {
        ObjectToXrcFilter xrc(object, "wxNotebook",
                              object->GetPropertyAsString("name"));
        xrc.AddWindowProperties();
        return xrc.GetXrcObject();
    }

    ticpp::Element* ImportFromXrc(ticpp::Element* xrcObject) override
    {
        XrcToXfbFilter filter(xrcObject, "wxNotebook");
        filter.AddWindowProperties();
        return filter.GetXfbObject();
    }
};

class ToolbookComponent : public ComponentBase {
public:
    wxObject* Create(IObject* object, wxObject* parent) override
    {
        wxToolbook* toolBook = new wxToolbook(
            (wxWindow*)parent, wxID_ANY,
            object->GetPropertyAsPoint("pos"),
            object->GetPropertyAsSize("size"),
            object->GetPropertyAsInteger("style")
                | object->GetPropertyAsInteger("window_style"));

        BookCtrl::AssignImageList(toolBook, object);

        std::function<void(wxBookCtrlEvent&)> onPageChanged(
            std::bind(&BookCtrl::OnPageChanged<wxToolbook>,
                      std::placeholders::_1, GetManager()));

        toolBook->Bind(wxEVT_TOOLBOOK_PAGE_CHANGED, onPageChanged);

        return toolBook;
    }

    ticpp::Element* ExportToXrc(IObject* object) override
    {
        ObjectToXrcFilter xrc(object, "wxToolbook",
                              object->GetPropertyAsString("name"));
        xrc.AddWindowProperties();
        return xrc.GetXrcObject();
    }

    ticpp::Element* ImportFromXrc(ticpp::Element* xrcObject) override
    {
        XrcToXfbFilter filter(xrcObject, "wxToolbook");
        filter.AddWindowProperties();
        return filter.GetXfbObject();
    }
};

class TreebookComponent : public ComponentBase {
public:
    wxObject* Create(IObject* object, wxObject* parent) override
    {
        wxTreebook* treeBook = new wxTreebook(
            (wxWindow*)parent, wxID_ANY,
            object->GetPropertyAsPoint("pos"),
            object->GetPropertyAsSize("size"),
            object->GetPropertyAsInteger("style")
                | object->GetPropertyAsInteger("window_style"));

        BookCtrl::AssignImageList(treeBook, object);

        std::function<void(wxBookCtrlEvent&)> onPageChanged(
            std::bind(&BookCtrl::OnPageChanged<wxTreebook>,
                      std::placeholders::_1, GetManager()));

        treeBook->Bind(wxEVT_TREEBOOK_PAGE_CHANGED, onPageChanged);

        return treeBook;
    }

    void OnCreated(wxObject* wxobject, wxWindow* /*wxparent*/) override
    {
        BookCtrl::OnTreebookCreated(wxobject, GetManager());
    }

    ticpp::Element* ExportToXrc(IObject* object) override
    {
        ObjectToXrcFilter xrc(object, "wxTreebook",
                              object->GetPropertyAsString("name"));
        xrc.AddWindowProperties();
        return xrc.GetXrcObject();
    }

    ticpp::Element* ImportFromXrc(ticpp::Element* xrcObject) override
    {
        XrcToXfbFilter filter(xrcObject, "wxTreebook");
        filter.AddWindowProperties();
        return filter.GetXfbObject();
    }
};

class SimplebookComponent : public ComponentBase {
public:
    wxObject* Create(IObject* object, wxObject* parent) override
    {
        return new wxSimplebook(
            (wxWindow*)parent, wxID_ANY,
            object->GetPropertyAsPoint("pos"),
            object->GetPropertyAsSize("size"),
            object->GetPropertyAsInteger("window_style"));
    }

    ticpp::Element* ExportToXrc(IObject* object) override
    {
        ObjectToXrcFilter xrc(object, "wxSimplebook",
                              object->GetPropertyAsString("name"));
        xrc.AddWindowProperties();
        return xrc.GetXrcObject();
    }

    ticpp::Element* ImportFromXrc(ticpp::Element* xrcObject) override
    {
        XrcToXfbFilter filter(xrcObject, "wxSimplebook");
        filter.AddWindowProperties();
        return filter.GetXfbObject();
    }
};

class AuiNotebookPageComponent : public ComponentBase {
public:
    void OnCreated(wxObject* wxobject, wxWindow* wxparent) override
    {
        BookCtrl::OnPageCreated<wxAuiNotebook>(
            wxobject, wxparent, GetManager(), "AuiNotebookPageComponent");
    }

    void OnSelected(wxObject* wxobject) override
    {
        BookCtrl::OnPageSelected<wxAuiNotebook>(wxobject, GetManager());
    }

    ticpp::Element* ExportToXrc(IObject* object) override
    {
        return BookCtrl::PageToXrc(object, "notebookpage");
    }

    ticpp::Element* ImportFromXrc(ticpp::Element* xrcObject) override
    {
        return BookCtrl::PageToProject(xrcObject, "auinotebookpage");
    }
};

class ListbookPageComponent : public ComponentBase {
public:
    void OnCreated(wxObject* wxobject, wxWindow* wxparent) override
    {
        BookCtrl::OnPageCreated<wxListbook>(
            wxobject, wxparent, GetManager(), "ListbookPageComponent");
    }

    void OnSelected(wxObject* wxobject) override
    {
        BookCtrl::OnPageSelected<wxListbook>(wxobject, GetManager());
    }

    ticpp::Element* ExportToXrc(IObject* object) override
    {
        return BookCtrl::PageToXrc(object, "listbookpage");
    }

    ticpp::Element* ImportFromXrc(ticpp::Element* xrcObject) override
    {
        return BookCtrl::PageToProject(xrcObject, "listbookpage");
    }
};

class NotebookPageComponent : public ComponentBase {
public:
    void OnCreated(wxObject* wxobject, wxWindow* wxparent) override
    {
        BookCtrl::OnPageCreated<wxNotebook>(
            wxobject, wxparent, GetManager(), "NotebookPageComponent");
    }

    void OnSelected(wxObject* wxobject) override
    {
        BookCtrl::OnPageSelected<wxNotebook>(wxobject, GetManager());
    }

    ticpp::Element* ExportToXrc(IObject* object) override
    {
        return BookCtrl::PageToXrc(object, "notebookpage");
    }

    ticpp::Element* ImportFromXrc(ticpp::Element* xrcObject) override
    {
        return BookCtrl::PageToProject(xrcObject, "notebookpage");
    }
};

class ToolbookPageComponent : public ComponentBase {
public:
    void OnCreated(wxObject* wxobject, wxWindow* wxparent)
    {
        BookCtrl::OnPageCreated<wxToolbook>(
            wxobject, wxparent, GetManager(), "ToolbookPageComponent");
    }

    void OnSelected(wxObject* wxobject)
    {
        BookCtrl::OnPageSelected<wxToolbook>(wxobject, GetManager());
    }

    ticpp::Element* ExportToXrc(IObject* object)
    {
        return BookCtrl::PageToXrc(object, "toolbookpage");
    }

    ticpp::Element* ImportFromXrc(ticpp::Element* xrcObject)
    {
        return BookCtrl::PageToProject(xrcObject, "toolbookpage");
    }
};

class TreebookPageComponent : public ComponentBase {
public:
    void OnCreated(wxObject* wxobject, wxWindow* wxparent)
    {
        BookCtrl::OnPageCreated<wxTreebook>(
            wxobject, wxparent, GetManager(), "TreebookPageComponent");
    }

    void OnSelected(wxObject* wxobject)
    {
        BookCtrl::OnPageSelected<wxTreebook>(wxobject, GetManager());
    }

    ticpp::Element* ExportToXrc(IObject* object)
    {
        return BookCtrl::PageToXrc(object, "treebookpage");
    }

    ticpp::Element* ImportFromXrc(ticpp::Element* xrcObject)
    {
        return BookCtrl::PageToProject(xrcObject, "treebookpage");
    }
};

class ChoicebookPageComponent : public ComponentBase {
public:
    void OnCreated(wxObject* wxobject, wxWindow* wxparent) override
    {
        BookCtrl::OnPageCreated<wxChoicebook>(
            wxobject, wxparent, GetManager(), "ChoicebookPageComponent");
    }

    void OnSelected(wxObject* wxobject) override
    {
        BookCtrl::OnPageSelected<wxChoicebook>(wxobject, GetManager());
    }

    ticpp::Element* ExportToXrc(IObject* object) override
    {
        ObjectToXrcFilter xrc(object, "choicebookpage");
        xrc.AddProperty("label", "label", XRC_TYPE_TEXT);
        xrc.AddProperty("select", "selected", XRC_TYPE_BOOL);
        return xrc.GetXrcObject();
    }

    ticpp::Element* ImportFromXrc(ticpp::Element* xrcObject) override
    {
        XrcToXfbFilter filter(xrcObject, "choicebookpage");
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
        BookCtrl::OnPageCreated<wxSimplebook>(
            wxobject, wxparent, GetManager(), "SimplebookPageComponent");
    }

    void OnSelected(wxObject* wxobject) override
    {
        BookCtrl::OnPageSelected<wxSimplebook>(wxobject, GetManager());
    }

    ticpp::Element* ExportToXrc(IObject* object) override
    {
        ObjectToXrcFilter xrc(object, "simplebookpage");
        xrc.AddProperty("label", "label", XRC_TYPE_TEXT);
        xrc.AddProperty("select", "selected", XRC_TYPE_BOOL);
        return xrc.GetXrcObject();
    }

    ticpp::Element* ImportFromXrc(ticpp::Element* xrcObject) override
    {
        XrcToXfbFilter filter(xrcObject, "simplebookpage");
        filter.AddWindowProperties();
        filter.AddProperty("label", "label", XRC_TYPE_TEXT);
        filter.AddProperty("selected", "select", XRC_TYPE_BOOL);
        return filter.GetXfbObject();
    }
};
