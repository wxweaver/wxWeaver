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
#include <ticpp.h>

#include <plugin.h>
#include <xrcconv.h>

#include <wx/gbsizer.h>
#include <wx/wrapsizer.h>

class SpacerComponent : public ComponentBase {
public:
    // ImportFromXRC is handled in sizeritem components

    ticpp::Element* ExportToXrc(IObject* obj) override
    {
        ObjectToXrcFilter xrc(obj, "spacer");
        xrc.AddPropertyPair("width", "height", "size");
        return xrc.GetXrcObject();
    }
};

class GBSizerItemComponent : public ComponentBase {
public:
    ticpp::Element* ExportToXrc(IObject* obj) override
    {
        ObjectToXrcFilter xrc(obj, "sizeritem");
        xrc.AddPropertyPair("row", "column", "cellpos");
        xrc.AddPropertyPair("rowspan", "colspan", "cellspan");
        xrc.AddProperty("flag", "flag", XRC_TYPE_BITLIST);
        xrc.AddProperty("border", "border", XRC_TYPE_INTEGER);
        return xrc.GetXrcObject();
    }

    ticpp::Element* ImportFromXrc(ticpp::Element* xrcObj) override
    {
        // XrcLoader::GetObject imports spacers as sizeritems
        XrcToXfbFilter filter(xrcObj, "gbsizeritem");
        filter.AddPropertyPair("cellpos", "row", "column");
        filter.AddPropertyPair("cellspan", "rowspan", "colspan");
        filter.AddProperty("flag", "flag", XRC_TYPE_BITLIST);
        filter.AddProperty("border", "border", XRC_TYPE_INTEGER);
        ticpp::Element* sizeritem = filter.GetXfbObject();

        // XrcLoader::GetObject imports spacers as sizeritems, so check for a spacer
        if (xrcObj->FirstChildElement("size", false)
            && !xrcObj->FirstChildElement("object", false)) {

            // it is a spacer
            XrcToXfbFilter spacer(xrcObj, "spacer");
            spacer.AddPropertyPair("size", "width", "height");
            sizeritem->LinkEndChild(spacer.GetXfbObject());
        }
        return sizeritem;
    }
};

class SizerItemComponent : public ComponentBase {
public:
    void OnCreated(wxObject* wxobject, wxWindow* /*wxparent*/) override
    {
        // Get parent sizer
        wxObject* parent = GetManager()->GetParent(wxobject);
        wxSizer* sizer = wxDynamicCast(parent, wxSizer);

        if (!sizer) {
            wxLogError(
                "The parent of a SizerItem is either missing or not a wxSizer:"
                "this shouldn't be possible!");
            return;
        }
        // Get child window
        wxObject* child = GetManager()->GetChild(wxobject, 0);
        if (!child) {
            wxLogError(
                "The SizerItem component has no child: this shouldn't be possible!");
            return;
        }
        // Get IObject for property access
        IObject* obj = GetManager()->GetIObject(wxobject);
        IObject* childObj = GetManager()->GetIObject(child);

        // Add the spacer
        if ("spacer" == childObj->GetClassName()) {
            sizer->Add(
                childObj->GetPropertyAsInteger("width"),
                childObj->GetPropertyAsInteger("height"),
                obj->GetPropertyAsInteger("proportion"),
                obj->GetPropertyAsInteger("flag"),
                obj->GetPropertyAsInteger("border"));
            return;
        }

        // Add the child ( window or sizer ) to the sizer
        wxWindow* windowChild = wxDynamicCast(child, wxWindow);
        wxSizer* sizerChild = wxDynamicCast(child, wxSizer);

        if (windowChild) {
            sizer->Add(
                windowChild,
                obj->GetPropertyAsInteger("proportion"),
                obj->GetPropertyAsInteger("flag"),
                obj->GetPropertyAsInteger("border"));
        } else if (sizerChild) {
            sizer->Add(
                sizerChild,
                obj->GetPropertyAsInteger("proportion"),
                obj->GetPropertyAsInteger("flag"),
                obj->GetPropertyAsInteger("border"));
        } else {
            wxLogError(
                "The SizerItem component's child is not a wxWindow or"
                "a wxSizer or a spacer: this shouldn't be possible!");
        }
    }

    ticpp::Element* ExportToXrc(IObject* obj) override
    {
        ObjectToXrcFilter xrc(obj, "sizeritem");
        xrc.AddProperty("proportion", "option", XRC_TYPE_INTEGER);
        xrc.AddProperty("flag", "flag", XRC_TYPE_BITLIST);
        xrc.AddProperty("border", "border", XRC_TYPE_INTEGER);
        return xrc.GetXrcObject();
    }

    ticpp::Element* ImportFromXrc(ticpp::Element* xrcObj) override
    {
        XrcToXfbFilter filter(xrcObj, "sizeritem");
        filter.AddProperty("option", "proportion", XRC_TYPE_INTEGER);
        filter.AddProperty("flag", "flag", XRC_TYPE_BITLIST);
        filter.AddProperty("border", "border", XRC_TYPE_INTEGER);
        ticpp::Element* sizeritem = filter.GetXfbObject();

        // XrcLoader::GetObject imports spacers as sizeritems, so check for a spacer
        if (xrcObj->FirstChildElement("size", false)
            && !xrcObj->FirstChildElement("object", false)) {

            // it is a spacer
            XrcToXfbFilter spacer(xrcObj, "spacer");
            spacer.AddPropertyPair("size", "width", "height");
            sizeritem->LinkEndChild(spacer.GetXfbObject());
        }
        return sizeritem;
    }
};

class BoxSizerComponent : public ComponentBase {
public:
    wxObject* Create(IObject* obj, wxObject* /*parent*/) override
    {
        wxBoxSizer* boxSizer = new wxBoxSizer(obj->GetPropertyAsInteger("orient"));
        boxSizer->SetMinSize(obj->GetPropertyAsSize("minimum_size"));
        return boxSizer;
    }

    ticpp::Element* ExportToXrc(IObject* obj) override
    {
        ObjectToXrcFilter xrc(obj, "wxBoxSizer");
        if (obj->GetPropertyAsSize("minimum_size") != wxDefaultSize)
            xrc.AddProperty("minimum_size", "minsize", XRC_TYPE_SIZE);
        xrc.AddProperty("orient", "orient", XRC_TYPE_TEXT);
        return xrc.GetXrcObject();
    }

    ticpp::Element* ImportFromXrc(ticpp::Element* xrcObj) override
    {
        XrcToXfbFilter filter(xrcObj, "wxBoxSizer");
        filter.AddProperty("minsize", "minsize", XRC_TYPE_SIZE);
        filter.AddProperty("orient", "orient", XRC_TYPE_TEXT);
        return filter.GetXfbObject();
    }
};

class WrapSizerComponent : public ComponentBase {
public:
    wxObject* Create(IObject* obj, wxObject* /*parent*/) override
    {
        const auto wrapSizer = new wxWrapSizer(obj->GetPropertyAsInteger("orient"),
                                               obj->GetPropertyAsInteger("flags"));
        wrapSizer->SetMinSize(obj->GetPropertyAsSize("minimum_size"));
        return wrapSizer;
    }

    ticpp::Element* ExportToXrc(IObject* obj) override
    {
        ObjectToXrcFilter xrc(obj, "wxWrapSizer");
        if (obj->GetPropertyAsSize("minimum_size") != wxDefaultSize) {
            xrc.AddProperty("minimum_size", "minsize", XRC_TYPE_SIZE);
        }
        xrc.AddProperty("orient", "orient", XRC_TYPE_TEXT);
        xrc.AddProperty("flags", "flags", XRC_TYPE_BITLIST);
        return xrc.GetXrcObject();
    }

    ticpp::Element* ImportFromXrc(ticpp::Element* xrcObj) override
    {
        XrcToXfbFilter filter(xrcObj, "wxWrapSizer");
        filter.AddProperty("minsize", "minsize", XRC_TYPE_SIZE);
        filter.AddProperty("orient", "orient", XRC_TYPE_TEXT);
        filter.AddProperty("flags", "flags", XRC_TYPE_BITLIST);
        return filter.GetXfbObject();
    }
};

class StaticBoxSizerComponent : public ComponentBase {
public:
    int m_count;
    StaticBoxSizerComponent()
    {
        m_count = 0;
    }
    wxObject* Create(IObject* obj, wxObject* parent) override
    {
        m_count++;
        wxStaticBox* box = new wxStaticBox((wxWindow*)parent, wxID_ANY,
                                           obj->GetPropertyAsString("label"));

        wxStaticBoxSizer* staticBoxSizer = new wxStaticBoxSizer(box,
                                                                obj->GetPropertyAsInteger("orient"));

        staticBoxSizer->SetMinSize(obj->GetPropertyAsSize("minimum_size"));
        return staticBoxSizer;
    }

    ticpp::Element* ExportToXrc(IObject* obj) override
    {
        ObjectToXrcFilter xrc(obj, "wxStaticBoxSizer");
        if (obj->GetPropertyAsSize("minimum_size") != wxDefaultSize)
            xrc.AddProperty("minimum_size", "minsize", XRC_TYPE_SIZE);
        xrc.AddProperty("orient", "orient", XRC_TYPE_TEXT);
        xrc.AddProperty("label", "label", XRC_TYPE_TEXT);
        return xrc.GetXrcObject();
    }

    ticpp::Element* ImportFromXrc(ticpp::Element* xrcObj) override
    {
        XrcToXfbFilter filter(xrcObj, "wxStaticBoxSizer");
        filter.AddProperty("minsize", "minsize", XRC_TYPE_SIZE);
        filter.AddProperty("orient", "orient", XRC_TYPE_TEXT);
        filter.AddProperty("label", "label", XRC_TYPE_TEXT);
        return filter.GetXfbObject();
    }
};

class GridSizerComponent : public ComponentBase {
public:
    wxObject* Create(IObject* obj, wxObject* /*parent*/) override
    {
        wxGridSizer* gridSizer = new wxGridSizer(
            obj->GetPropertyAsInteger("rows"),
            obj->GetPropertyAsInteger("cols"),
            obj->GetPropertyAsInteger("vgap"),
            obj->GetPropertyAsInteger("hgap"));

        gridSizer->SetMinSize(obj->GetPropertyAsSize("minimum_size"));
        return gridSizer;
    }

    ticpp::Element* ExportToXrc(IObject* obj) override
    {
        ObjectToXrcFilter xrc(obj, "wxGridSizer");
        if (obj->GetPropertyAsSize("minimum_size") != wxDefaultSize)
            xrc.AddProperty("minimum_size", "minsize", XRC_TYPE_SIZE);
        xrc.AddProperty("rows", "rows", XRC_TYPE_INTEGER);
        xrc.AddProperty("cols", "cols", XRC_TYPE_INTEGER);
        xrc.AddProperty("vgap", "vgap", XRC_TYPE_INTEGER);
        xrc.AddProperty("hgap", "hgap", XRC_TYPE_INTEGER);
        return xrc.GetXrcObject();
    }

    ticpp::Element* ImportFromXrc(ticpp::Element* xrcObj) override
    {
        XrcToXfbFilter filter(xrcObj, "wxGridSizer");
        filter.AddProperty("minsize", "minsize", XRC_TYPE_SIZE);
        filter.AddProperty("rows", "rows", XRC_TYPE_INTEGER);
        filter.AddProperty("cols", "cols", XRC_TYPE_INTEGER);
        filter.AddProperty("vgap", "vgap", XRC_TYPE_INTEGER);
        filter.AddProperty("hgap", "hgap", XRC_TYPE_INTEGER);
        return filter.GetXfbObject();
    }
};

class FlexGridSizerBase : public ComponentBase {
public:
    void AddProperties(IObject* obj, wxFlexGridSizer* flexGridSizer)
    {
        for (const auto& col : obj->GetPropertyAsVectorIntPair("growablecols"))
            flexGridSizer->AddGrowableCol(col.first, col.second);

        for (const auto& row : obj->GetPropertyAsVectorIntPair("growablerows"))
            flexGridSizer->AddGrowableRow(row.first, row.second);

        flexGridSizer->SetMinSize(obj->GetPropertyAsSize("minimum_size"));
        flexGridSizer->SetFlexibleDirection(obj->GetPropertyAsInteger("flexible_direction"));
        flexGridSizer->SetNonFlexibleGrowMode(
            (wxFlexSizerGrowMode)obj->GetPropertyAsInteger("non_flexible_grow_mode"));
    }

    void ExportXRCProperties(ObjectToXrcFilter* xrc, IObject* obj)
    {
        if (obj->GetPropertyAsSize("minimum_size") != wxDefaultSize)
            xrc->AddProperty("minimum_size", "minsize", XRC_TYPE_SIZE);
        xrc->AddProperty("vgap", "vgap", XRC_TYPE_INTEGER);
        xrc->AddProperty("hgap", "hgap", XRC_TYPE_INTEGER);
        xrc->AddPropertyValue("growablecols", obj->GetPropertyAsString("growablecols"));
        xrc->AddPropertyValue("growablerows", obj->GetPropertyAsString("growablerows"));
    }

    void ImportXRCProperties(XrcToXfbFilter* filter)
    {
        filter->AddProperty("minsize", "minsize", XRC_TYPE_SIZE);
        filter->AddProperty("vgap", "vgap", XRC_TYPE_INTEGER);
        filter->AddProperty("hgap", "hgap", XRC_TYPE_INTEGER);
        filter->AddProperty("growablecols", "growablecols", XRC_TYPE_TEXT);
        filter->AddProperty("growablerows", "growablerows", XRC_TYPE_TEXT);
    }
};

class FlexGridSizerComponent : public FlexGridSizerBase {
public:
    wxObject* Create(IObject* obj, wxObject* /*parent*/) override
    {
        wxFlexGridSizer* flexGridSizer = new wxFlexGridSizer(
            obj->GetPropertyAsInteger("rows"),
            obj->GetPropertyAsInteger("cols"),
            obj->GetPropertyAsInteger("vgap"),
            obj->GetPropertyAsInteger("hgap"));

        AddProperties(obj, flexGridSizer);
        return flexGridSizer;
    }

    ticpp::Element* ExportToXrc(IObject* obj) override
    {
        ObjectToXrcFilter xrc(obj, "wxFlexGridSizer");
        xrc.AddProperty("rows", "rows", XRC_TYPE_INTEGER);
        xrc.AddProperty("cols", "cols", XRC_TYPE_INTEGER);
        ExportXRCProperties(&xrc, obj);
        return xrc.GetXrcObject();
    }

    ticpp::Element* ImportFromXrc(ticpp::Element* xrcObj) override
    {
        XrcToXfbFilter filter(xrcObj, "wxFlexGridSizer");
        filter.AddProperty("rows", "rows", XRC_TYPE_INTEGER);
        filter.AddProperty("cols", "cols", XRC_TYPE_INTEGER);
        ImportXRCProperties(&filter);
        return filter.GetXfbObject();
    }
};

class GridBagSizerComponent : public FlexGridSizerBase {
private:
    wxGBSizerItem* GetGBSizerItem(IObject* sizeritem, const wxGBPosition& position, const wxGBSpan& span, wxObject* child)
    {
        IObject* childObj = GetManager()->GetIObject(child);

        if ("spacer" == childObj->GetClassName()) {
            return new wxGBSizerItem(
                childObj->GetPropertyAsInteger("width"),
                childObj->GetPropertyAsInteger("height"),
                position, span,
                sizeritem->GetPropertyAsInteger("flag"),
                sizeritem->GetPropertyAsInteger("border"), nullptr);
        }

        // Add the child to the sizer
        wxWindow* windowChild = wxDynamicCast(child, wxWindow);
        wxSizer* sizerChild = wxDynamicCast(child, wxSizer);

        if (!windowChild) {
            return new wxGBSizerItem(
                windowChild, position, span,
                sizeritem->GetPropertyAsInteger("flag"),
                sizeritem->GetPropertyAsInteger("border"), nullptr);
        } else if (sizerChild) {
            return new wxGBSizerItem(
                sizerChild, position, span,
                sizeritem->GetPropertyAsInteger("flag"),
                sizeritem->GetPropertyAsInteger("border"), nullptr);
        } else {
            wxLogError(
                "The GBSizerItem component's child is not a wxWindow "
                "or a wxSizer or a Spacer: this should not be possible!");
            return nullptr;
        }
    }

public:
    wxObject* Create(IObject* obj, wxObject* /*parent*/) override
    {
        wxGridBagSizer* gridBagSizer = new wxGridBagSizer(
            obj->GetPropertyAsInteger("vgap"),
            obj->GetPropertyAsInteger("hgap"));

        if (!obj->IsNull("empty_cell_size"))
            gridBagSizer->SetEmptyCellSize(
                obj->GetPropertyAsSize("empty_cell_size"));

        return gridBagSizer;
    }

    void OnCreated(wxObject* wxobject, wxWindow* /*wxparent*/) override
    {
        // For storing objects whose position needs to be determined
        std::vector<std::pair<wxObject*, wxGBSizerItem*>> newObjects;
        wxGBPosition lastPosition(0, 0);

        // Get gridBagSizer
        wxGridBagSizer* gridBagSizer = wxDynamicCast(wxobject, wxGridBagSizer);
        if (!gridBagSizer) {
            wxLogError("This should be a wxGridBagSizer!");
            return;
        }

        // Add the children
        IManager* manager = GetManager();
        size_t count = manager->GetChildCount(wxobject);
        if (!count) {
            // wxGridBagSizer gets upset sometimes without children
            gridBagSizer->Add(0, 0, wxGBPosition(0, 0));
            return;
        }

        for (size_t i = 0; i < count; ++i) {
            // Should be a GBSizerItem
            wxObject* wxsizerItem = manager->GetChild(wxobject, i);
            IObject* isizerItem = manager->GetIObject(wxsizerItem);

            // Get the location of the item
            wxGBSpan span(isizerItem->GetPropertyAsInteger("rowspan"),
                          isizerItem->GetPropertyAsInteger("colspan"));

            // TODO: Replace all of these "< 0" with wxNOT_FOUND?
            int column = isizerItem->GetPropertyAsInteger("column");
            if (column < 0) {
                // Needs to be auto positioned after the other children are added
                wxGBSizerItem* item = GetGBSizerItem(
                    isizerItem, lastPosition, span,
                    manager->GetChild(wxsizerItem, 0));

                if (item)
                    newObjects.push_back(
                        std::pair<wxObject*, wxGBSizerItem*>(wxsizerItem, item));

                continue;
            }

            wxGBPosition position(isizerItem->GetPropertyAsInteger("row"), column);

            // Check for intersection
            if (gridBagSizer->CheckForIntersection(position, span))
                continue;

            lastPosition = position;

            // Add the child to the gridBagSizer
            wxGBSizerItem* item = GetGBSizerItem(
                isizerItem, position, span, manager->GetChild(wxsizerItem, 0));

            if (item)
                gridBagSizer->Add(item);
        }

        std::vector<std::pair<wxObject*, wxGBSizerItem*>>::iterator it;
        for (it = newObjects.begin(); it != newObjects.end(); ++it) {
            wxGBPosition position = it->second->GetPos();
            wxGBSpan span = it->second->GetSpan();
            int column = position.GetCol();
            while (gridBagSizer->CheckForIntersection(position, span)) {
                column++;
                position.SetCol(column);
            }
            it->second->SetPos(position);
            gridBagSizer->Add(it->second);

            GetManager()->ModifyProperty(
                it->first, "row",
                wxString::Format("%i", position.GetRow()), false);

            GetManager()->ModifyProperty(
                it->first, "column",
                wxString::Format("%i", column), false);
        }
        AddProperties(manager->GetIObject(wxobject), gridBagSizer);
    }

    ticpp::Element* ExportToXrc(IObject* obj) override
    {
        ObjectToXrcFilter xrc(obj, "wxGridBagSizer");
        ExportXRCProperties(&xrc, obj);
        return xrc.GetXrcObject();
    }

    ticpp::Element* ImportFromXrc(ticpp::Element* xrcObj) override
    {
        XrcToXfbFilter filter(xrcObj, "wxGridBagSizer");
        ImportXRCProperties(&filter);
        return filter.GetXfbObject();
    }
};

class StdDialogButtonSizerComponent : public ComponentBase {
private:
    void AddXRCButton(ticpp::Element* stdDialogButtonSizer,
                      const std::string& id, const std::string& label)
    {
        try {
            ticpp::Element button("object");
            button.SetAttribute("class", "button");

            ticpp::Element flag("flag");
            flag.SetText("wxALIGN_CENTER_HORIZONTAL|wxALL");
            button.LinkEndChild(&flag);

            ticpp::Element border("border");
            border.SetText("5");
            button.LinkEndChild(&border);

            ticpp::Element wxbutton("object");
            wxbutton.SetAttribute("class", "wxButton");
            wxbutton.SetAttribute("name", id);

            ticpp::Element labelEl("label");
            labelEl.SetText(label);
            wxbutton.LinkEndChild(&labelEl);

            button.LinkEndChild(&wxbutton);

            stdDialogButtonSizer->LinkEndChild(&button);

        } catch (ticpp::Exception& ex) {
            wxLogError(wxString(ex.m_details.c_str(), wxConvUTF8));
        }
    }

public:
    wxObject* Create(IObject* obj, wxObject* parent) override
    {
        wxStdDialogButtonSizer* stdDialogButtonSizer = new wxStdDialogButtonSizer();
        stdDialogButtonSizer->SetMinSize(obj->GetPropertyAsSize("minimum_size"));

        if (obj->GetPropertyAsInteger("OK"))
            stdDialogButtonSizer->AddButton(new wxButton((wxWindow*)parent, wxID_OK));

        if (obj->GetPropertyAsInteger("Yes"))
            stdDialogButtonSizer->AddButton(new wxButton((wxWindow*)parent, wxID_YES));

        if (obj->GetPropertyAsInteger("Save"))
            stdDialogButtonSizer->AddButton(new wxButton((wxWindow*)parent, wxID_SAVE));

        if (obj->GetPropertyAsInteger("Apply"))
            stdDialogButtonSizer->AddButton(new wxButton((wxWindow*)parent, wxID_APPLY));

        if (obj->GetPropertyAsInteger("No"))
            stdDialogButtonSizer->AddButton(new wxButton((wxWindow*)parent, wxID_NO));

        if (obj->GetPropertyAsInteger("Cancel"))
            stdDialogButtonSizer->AddButton(new wxButton((wxWindow*)parent, wxID_CANCEL));

        if (obj->GetPropertyAsInteger("Help"))
            stdDialogButtonSizer->AddButton(new wxButton((wxWindow*)parent, wxID_HELP));

        if (obj->GetPropertyAsInteger("ContextHelp"))
            stdDialogButtonSizer->AddButton(new wxButton((wxWindow*)parent, wxID_CONTEXT_HELP));

        stdDialogButtonSizer->Realize();
        return stdDialogButtonSizer;
    }

    ticpp::Element* ExportToXrc(IObject* obj) override
    {
        ObjectToXrcFilter xrc(obj, "wxStdDialogButtonSizer");
        ticpp::Element* stdDialogButtonSizer = xrc.GetXrcObject();

        if (obj->GetPropertyAsSize("minimum_size") != wxDefaultSize)
            xrc.AddProperty("minimum_size", "minsize", XRC_TYPE_SIZE);

        if (obj->GetPropertyAsInteger("OK"))
            AddXRCButton(stdDialogButtonSizer, "wxID_OK", "&OK");

        if (obj->GetPropertyAsInteger("Yes"))
            AddXRCButton(stdDialogButtonSizer, "wxID_YES", "&Yes");

        if (obj->GetPropertyAsInteger("Save"))
            AddXRCButton(stdDialogButtonSizer, "wxID_SAVE", "&Save");

        if (obj->GetPropertyAsInteger("Apply"))
            AddXRCButton(stdDialogButtonSizer, "wxID_APPLY", "&Apply");

        if (obj->GetPropertyAsInteger("No"))
            AddXRCButton(stdDialogButtonSizer, "wxID_NO", "&No");

        if (obj->GetPropertyAsInteger("Cancel"))
            AddXRCButton(stdDialogButtonSizer, "wxID_CANCEL", "&Cancel");

        if (obj->GetPropertyAsInteger("Help"))
            AddXRCButton(stdDialogButtonSizer, "wxID_HELP", "&Help");

        if (obj->GetPropertyAsInteger("ContextHelp"))
            AddXRCButton(stdDialogButtonSizer, "wxID_CONTEXT_HELP", "");

        return stdDialogButtonSizer;
    }

    ticpp::Element* ImportFromXrc(ticpp::Element* xrcObj) override
    {
        std::map<wxString, wxString> buttons;
        buttons["OK"] = "0";
        buttons["Yes"] = "0";
        buttons["Save"] = "0";
        buttons["Apply"] = "0";
        buttons["No"] = "0";
        buttons["Cancel"] = "0";
        buttons["Help"] = "0";
        buttons["ContextHelp"] = "0";

        XrcToXfbFilter filter(xrcObj, "wxStdDialogButtonSizer");
        filter.AddProperty("minsize", "minsize", XRC_TYPE_SIZE);

        ticpp::Element* button = xrcObj->FirstChildElement("object", false);
        for (; button != 0; button = button->NextSiblingElement("object", false)) {
            try {
                std::string button_class;
                button->GetAttribute("class", &button_class);
                if (std::string("button") != button_class) {
                    continue;
                }

                ticpp::Element* wxbutton = button->FirstChildElement("object");
                std::string wxbutton_class;
                wxbutton->GetAttribute("class", &wxbutton_class);
                if (std::string("wxButton") != wxbutton_class) {
                    continue;
                }
                std::string name;
                wxbutton->GetAttribute("name", &name);

                if (name == "wxID_OK")
                    buttons["OK"] = "1";
                else if (name == "wxID_YES")
                    buttons["Yes"] = "1";
                else if (name == "wxID_SAVE")
                    buttons["Save"] = "1";
                else if (name == "wxID_APPLY")
                    buttons["Apply"] = "1";
                else if (name == "wxID_NO")
                    buttons["No"] = "1";
                else if (name == "wxID_CANCEL")
                    buttons["Cancel"] = "1";
                else if (name == "wxID_HELP")
                    buttons["Help"] = "1";
                else if (name == "wxID_CONTEXT_HELP")
                    buttons["ContextHelp"] = "1";

            } catch (ticpp::Exception&) {
                continue;
            }
        }
        std::map<wxString, wxString>::iterator prop;
        for (prop = buttons.begin(); prop != buttons.end(); ++prop)
            filter.AddPropertyValue(prop->first, prop->second);

        xrcObj->Clear();
        return filter.GetXfbObject();
    }
};

BEGIN_LIBRARY()
ABSTRACT_COMPONENT("spacer", SpacerComponent)
ABSTRACT_COMPONENT("sizeritem", SizerItemComponent)
ABSTRACT_COMPONENT("gbsizeritem", GBSizerItemComponent)

SIZER_COMPONENT("wxBoxSizer", BoxSizerComponent)
SIZER_COMPONENT("wxWrapSizer", WrapSizerComponent)
SIZER_COMPONENT("wxStaticBoxSizer", StaticBoxSizerComponent)
SIZER_COMPONENT("wxGridSizer", GridSizerComponent)
SIZER_COMPONENT("wxFlexGridSizer", FlexGridSizerComponent)
SIZER_COMPONENT("wxGridBagSizer", GridBagSizerComponent)
SIZER_COMPONENT("wxStdDialogButtonSizer", StdDialogButtonSizerComponent)

// wxBoxSizer
MACRO(wxHORIZONTAL)
MACRO(wxVERTICAL)

// wxWrapSizer
MACRO(wxEXTEND_LAST_ON_EACH_LINE)
MACRO(wxREMOVE_LEADING_SPACES)
MACRO(wxWRAPSIZER_DEFAULT_FLAGS)

// wxFlexGridSizer
MACRO(wxBOTH)
MACRO(wxFLEX_GROWMODE_NONE)
MACRO(wxFLEX_GROWMODE_SPECIFIED)
MACRO(wxFLEX_GROWMODE_ALL)

// Add
MACRO(wxALL)
MACRO(wxLEFT)
MACRO(wxRIGHT)
MACRO(wxTOP)
MACRO(wxBOTTOM)
MACRO(wxEXPAND)
MACRO(wxALIGN_BOTTOM)
MACRO(wxALIGN_CENTER)
MACRO(wxALIGN_CENTER_HORIZONTAL)
MACRO(wxALIGN_CENTER_VERTICAL)
MACRO(wxSHAPED)
MACRO(wxFIXED_MINSIZE)
MACRO(wxRESERVE_SPACE_EVEN_IF_HIDDEN)

SYNONYMOUS(wxGROW, wxEXPAND)
SYNONYMOUS(wxALIGN_CENTRE, wxALIGN_CENTER)
SYNONYMOUS(wxALIGN_CENTRE_HORIZONTAL, wxALIGN_CENTER_HORIZONTAL)
SYNONYMOUS(wxALIGN_CENTRE_VERTICAL, wxALIGN_CENTER_VERTICAL)
END_LIBRARY()
