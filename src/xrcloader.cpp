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
#include "xrcloader.h"

#include "utils/debug.h"
#include "utils/typeconv.h"
#include "rtti/objectbase.h"

PObjectBase XrcLoader::GetProject(ticpp::Document* xrcDoc)
{
    assert(m_objDb);
    LogDebug("[XrcFilter::GetProject]");

    PObjectBase project(m_objDb->CreateObject("Project"));

    ticpp::Element* root = xrcDoc->FirstChildElement("resource", false);
    if (!root) {
        wxLogError("Missing root element \"resource\"");
        return project;
    }
    ticpp::Element* element = root->FirstChildElement("object", false);
    while (element) {
        PObjectBase obj = GetObject(element, project);
        element = element->NextSiblingElement("object", false);
    }
    return project;
}

PObjectBase XrcLoader::GetObject(ticpp::Element* xrcObj, PObjectBase parent)
{
    // First, create the object by the name, the modify the properties
    std::string className = xrcObj->GetAttribute("class");
    if (className == "") {
        className = xrcObj->Value();
    }
    if (parent->GetTypeName() == "project") {
        if (className == "wxBitmap") {
            PProperty bitmapsProp = parent->GetProperty("bitmaps");
            if (bitmapsProp) {
                wxString value = bitmapsProp->GetValue();
                wxString text = xrcObj->GetText();
                text.Replace("\'", "\'\'", true);
                value << "\'" << text << "\' ";
                bitmapsProp->SetValue(value);
                return PObjectBase();
            }
        }
        if (className == "wxIcon") {
            PProperty iconsProp = parent->GetProperty("icons");
            if (iconsProp) {
                wxString value = iconsProp->GetValue();
                wxString text = xrcObj->GetText();
                text.Replace("\'", "\'\'", true);
                value << "\'" << text << "\' ";
                iconsProp->SetValue(value);
                return PObjectBase();
            }
        }
        // Forms wxPanel, wxFrame, wxDialog are stored internally as Panel, Frame, and Dialog
        // to prevent conflicts with wxPanel as a container
        className = className.substr(2, className.size() - 2);
    }
    // Well, this is not nice. wxMenu class name is ambiguous, so we'll get the
    // correct class by the context. If the parent of a wxMenu is another wxMenu
    // then the class name will be "submenu"
    else if ((className == "wxMenu" && parent->GetClassName() == "wxMenu")
             || parent->GetClassName() == "submenu")
        className = "submenu";

    // "separator" is also ambiguous:
    // it could be a toolbar separator or a menu separator
    else if (className == "separator") {
        if (parent->GetClassName() == "wxToolBar" || parent->GetClassName() == "ToolBar")
            className = "toolSeparator";
    }
    // replace "spacer" with "sizeritem" so it will be imported as a "sizeritem"
    // "sizeritem" is ambiguous - could also be a grid bag sizeritem
    else if (className == "spacer" || className == "sizeritem") {
        if (parent->GetClassName() == "wxGridBagSizer")
            className = "gbsizeritem";
        else
            className = "sizeritem";
    } else if (className == "imagelist") {
        className = "wxImageList";
    } else if (className == "bitmap" && parent->GetClassName() == "wxImageList") {
        className = "bitmapitem";
    }
    PObjectBase object;
    PObjectInfo objInfo = m_objDb->GetObjectInfo(className);
    if (objInfo) {
        IComponent* comp = objInfo->GetComponent();
        if (!comp) {
            wxLogError(
                "No component found for class \"%s\", found on line %i.",
                className, xrcObj->Row());
        } else {
            ticpp::Element* fbObj = comp->ImportFromXrc(xrcObj);
            if (!fbObj) {
                wxLogError(
                    "ImportFromXrc returned nullptr for class \"%s\", found on line %i.",
                    className, xrcObj->Row());
            } else {
                object = m_objDb->CreateObject(fbObj, parent);
                if (!object) {
                    // Unable to create the object and add it to the parent,
                    // probably needs a sizer
                    PObjectBase newsizer = m_objDb->CreateObject("wxBoxSizer", parent);
                    if (newsizer) {
                        /*
                            It is possible the CreateObject returns an "item"
                            containing the object, e.g. SizerItem or SplitterItem
                            If that is the case, reassign "object" to the actual object
                        */
                        PObjectBase sizer = newsizer;
                        if (sizer->GetChildCount() > 0)
                            sizer = sizer->GetChild(0);

                        if (sizer) {
                            object = m_objDb->CreateObject(fbObj, sizer);
                            if (object) {
                                parent->AddChild(newsizer);
                                newsizer->SetParent(parent);
                            }
                        }
                    }
                }
                if (!object) {
                    wxLogError(
                        "CreateObject failed for class \"%s\", with parent \"%s\", found on line %i",
                        className,
                        parent->GetClassName().c_str(), xrcObj->Row());
                } else {
                    /*
                        It is possible the CreateObject returns an "item"
                        containing the object, e.g. SizerItem or SplitterItem
                        If that is the case, reassign "object" to the actual object
                    */
                    if (object && object->GetChildCount() > 0)
                        object = object->GetChild(0);

                    if (object) {
                        // Recursively import the children
#if 0
                        ticpp::Element* element = xrcObj->FirstChildElement("object", false);
#else
                        /*
                            FIXME: Although this works for at least imagelists
                            and 1 bitmap it's not correct and clearly bad,
                            need to refactor some things
                        */
                        ticpp::Element* element = xrcObj->FirstChildElement("imagelist", false);
                        if (!element)
                            element = xrcObj->FirstChildElement("imagelist-small", false);

                        if (!element)
                            element = xrcObj->FirstChildElement("bitmap", false);

                        if (!element)
                            element = xrcObj->FirstChildElement("object", false);
#endif
                        while (element) {
                            GetObject(element, object);
                            element = element->NextSiblingElement("object", false);
                        }
                    }
                }
            }
        }
    } else {
        // Create a wxPanel to represent unknown classes
        object = m_objDb->CreateObject("wxPanel", parent);
        if (object) {
            parent->AddChild(object);
            object->SetParent(parent);
            wxLogError("Unknown class \"%s\" found on line %i, replaced with a wxPanel",
                       className, xrcObj->Row());
        } else {
            wxString msg(wxString::Format(
                             "Unknown class \"%s\" found on line %i, and could not replace with a wxPanel as child of \"%s:%s\"",
                             className,
                             xrcObj->Row(),
                             parent->GetPropertyAsString("name"))
                             .c_str(),
                         parent->GetClassName().c_str());

            wxLogError(msg);
        }
    }
    return object;
}
