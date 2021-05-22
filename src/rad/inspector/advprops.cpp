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
#include "rad/inspector/advprops.h"

#include "utils/typeconv.h"
#include "rad/appdata.h"

#include <wx/regex.h>
/*
    TODO: API changes after 2015: in between 3.0.2 (2014) and 3.0.3rc1 (2017)
    http://trac.wxwidgets.org/changeset/78444/svn-wx (#15541)

    Check for missing features from old wxPropertyGrid version,
    latest seems to be 1.4.15
    https://sourceforge.net/projects/wxpropgrid/files/wxPropertyGrid/1.4.15/
*/
// -----------------------------------------------------------------------
// wxWeaverSizeProperty
// -----------------------------------------------------------------------
#if wxCHECK_VERSION(3, 1, 0)
wxPG_IMPLEMENT_PROPERTY_CLASS(wxWeaverSizeProperty, wxPGProperty, TextCtrl)
#else
WX_PG_IMPLEMENT_PROPERTY_CLASS(wxWeaverSizeProperty, wxPGProperty,
                               wxSize, const wxSize&, TextCtrl)
#endif
    wxWeaverSizeProperty::wxWeaverSizeProperty(const wxString& label,
                                               const wxString& name,
                                               const wxSize& value)
    : wxPGProperty(label, name)
{
    DoSetValue(value);
    AddPrivateChild(new wxIntProperty("Width", wxPG_LABEL, value.x));
    AddPrivateChild(new wxIntProperty("Height", wxPG_LABEL, value.y));
}

void wxWeaverSizeProperty::RefreshChildren()
{
    if (GetChildCount() < 2)
        return;

    const wxSize& size = wxSizeRefFromVariant(m_value);

    Item(0)->SetValue((long)size.x);
    Item(1)->SetValue((long)size.y);
}

wxVariant wxWeaverSizeProperty::ChildChanged(wxVariant& thisValue,
                                             const int childIndex,
                                             wxVariant& childValue) const
{
    wxSize& size = wxSizeRefFromVariant(thisValue);

    wxVariant();

    int val = childValue.GetLong();
    switch (childIndex) {
    case 0:
        size.x = val;
        break;
    case 1:
        size.y = val;
        break;
    }
    wxVariant newVariant;
    newVariant << size;
    return newVariant;
}

// -----------------------------------------------------------------------
// wxWeaverPointProperty
// -----------------------------------------------------------------------

#if wxCHECK_VERSION(3, 1, 0)
wxPG_IMPLEMENT_PROPERTY_CLASS(wxWeaverPointProperty, wxPGProperty, TextCtrl)
#else
WX_PG_IMPLEMENT_PROPERTY_CLASS(wxWeaverPointProperty, wxPGProperty, wxPoint, const wxPoint&, TextCtrl)
#endif

    wxWeaverPointProperty::wxWeaverPointProperty(const wxString& label,
                                                 const wxString& name,
                                                 const wxPoint& value)
    : wxPGProperty(label, name)
{
    DoSetValue(value);
    AddPrivateChild(new wxIntProperty("X", wxPG_LABEL, value.x));
    AddPrivateChild(new wxIntProperty("Y", wxPG_LABEL, value.y));
}

wxWeaverPointProperty::~wxWeaverPointProperty() { }

void wxWeaverPointProperty::RefreshChildren()
{
    if (GetChildCount() < 2)
        return;

    const wxPoint& point = wxPointRefFromVariant(m_value);

    Item(0)->SetValue((long)point.x);
    Item(1)->SetValue((long)point.y);
}

wxVariant wxWeaverPointProperty::ChildChanged(wxVariant& thisValue, const int childIndex,
                                              wxVariant& childValue) const
{
    wxPoint& point = wxPointRefFromVariant(thisValue);

    wxVariant();

    int val = childValue.GetLong();
    switch (childIndex) {
    case 0:
        point.x = val;
        break;
    case 1:
        point.y = val;
        break;
    }
    wxVariant newVariant;
    newVariant << point;
    return newVariant;
}

// -----------------------------------------------------------------------
// wxWeaverBitmapProperty
// -----------------------------------------------------------------------
#if 0
// TODO: new wxPropertyGrid misses the wxPG_FILE_FILTER_INDEX attribute ID
static long gs_imageFilterIndex = -1;
#endif
static wxString gs_imageInitialPath = wxEmptyString;

#if wxCHECK_VERSION(3, 1, 0)
wxPG_IMPLEMENT_PROPERTY_CLASS(wxWeaverBitmapProperty, wxPGProperty, TextCtrl)
#else
WX_PG_IMPLEMENT_PROPERTY_CLASS(wxWeaverBitmapProperty, wxPGProperty,
                               wxString, const wxString&, TextCtrl)
#endif
    void wxWeaverBitmapProperty::GetChildValues(const wxString& parentValue,
                                                wxArrayString& childValues) const
{
    // some properties can contain value like "[-1;-1]"
    // which must be modified due to use of ";" as a string separator
    wxString values = parentValue;
    wxRegEx regex("\\[.+;.+\\]");
    if (regex.IsValid()) {
        if (regex.Matches(values)) {
            wxString sizeVal = regex.GetMatch(values);
            sizeVal.Replace(";", "<semicolon>");
            sizeVal.Replace("[", "");
            sizeVal.Replace("]", "");
            regex.Replace(&values, sizeVal);
        }
    }
    childValues = wxStringTokenize(values, ';', wxTOKEN_RET_EMPTY_ALL);
    for (wxArrayString::iterator value = childValues.begin();
         value != childValues.end(); ++value) {
        value->Trim(false);
        value->Replace("<semicolon>", ";");
    }
}

wxWeaverBitmapProperty::wxWeaverBitmapProperty(const wxString& label,
                                               const wxString& name,
                                               const wxString& value)
    : wxPGProperty(label, name)
{
    SetValue(WXVARIANT(value));
}

void wxWeaverBitmapProperty::CreateChildren()
{
    wxString propValue = m_value.GetString();
    wxVariant thisValue = WXVARIANT(propValue);
    wxVariant childValue;
    int childIndex = 0;
    wxArrayString childVals;
    GetChildValues(propValue, childVals);

    wxString source;
    if (childVals.Count() > 0)
        source = childVals.Item(0);
    else
        source = _("Load From File");

    prevSrc = wxNOT_FOUND;
    if (source == wxString(_("Load From File")))
        childIndex = 0;
    else if (source == wxString(_("Load From Embedded File")))
        childIndex = 1;
    else if (source == wxString(_("Load From Resource")))
        childIndex = 2;
    else if (source == wxString(_("Load From Icon Resource")))
        childIndex = 3;
    else if (source == wxString(_("Load From XRC")))
        childIndex = 4;
    else if (source == wxString(_("Load From Art Provider")))
        childIndex = 5;

    childValue = WXVARIANT(childIndex);
    CreatePropertySource(childIndex);
    ChildChanged(thisValue, 0, childValue);
}

wxPGProperty* wxWeaverBitmapProperty::CreatePropertySource(int sourceIndex)
{
    wxPGChoices sourceChoices;

    // Add 'source' property (common for all other children)
    sourceChoices.Add(_("Load From File"));
    sourceChoices.Add(_("Load From Embedded File"));
    sourceChoices.Add(_("Load From Resource"));
    sourceChoices.Add(_("Load From Icon Resource"));
    sourceChoices.Add(_("Load From XRC"));
    sourceChoices.Add(_("Load From Art Provider"));

    wxPGProperty* srcProp
        = new wxEnumProperty("source", wxPG_LABEL, sourceChoices, sourceIndex);

    srcProp->SetHelpString(
        wxString(
            _("Load From File:\n"))
        + wxString(_("Load the image from a file on disk.\n\n"))
        + wxString(_("Load From Embedded File:\n"))
        + wxString(_("C++ Only. Embed the image file in the exe and load it.\nFor other languages, behaves like \"Load From File\".\n\n"))
        + wxString(_("Load From Resource:\n"))
        + wxString(_("Windows Only. Load the image from a BITMAP resource in a .rc file\n\n"))
        + wxString(_("Load From Icon Resource:\n"))
        + wxString(_("Windows Only. Load the image from a ICON resource in a .rc file\n\n"))
        + wxString(_("Load From XRC:\n"))
        + wxString(_("Load the image from XRC ressources. The XRC ressources must be initialized by the application code.\n\n"))
        + wxString(_("Load From Art Provider:\n"))
        + wxString(_("Query registered providers for bitmap with given ID.\n\n")));

    AppendChild(srcProp);
    return srcProp;
}

wxPGProperty* wxWeaverBitmapProperty::CreatePropertyFilePath()
{
    // Add 'file_path' property
    // (common for 'Load From File' and 'Load From Embedded File' choices)
    wxPGProperty* propFilePath
        = new wxImageFileProperty("file_path", wxPG_LABEL);

    propFilePath->SetHelpString(_("Path to the image file."));
#if 0
    if (!gs_imageInitialPath.IsEmpty()) {
        wxVariant initialPath(gs_imageInitialPath);
        propFilePath->SetAttribute(wxPG_FILE_INITIAL_PATH, initialPath);
    }
#endif
    return propFilePath;
}

wxPGProperty* wxWeaverBitmapProperty::CreatePropertyResourceName()
{
    // Create 'resource_name' property
    // (common for 'Load From Resource' and 'Load From Icon Resource' choices)
    wxPGProperty* propResName
        = new wxStringProperty("resource_name", wxPG_LABEL);

    propResName->SetHelpString(
        _("Windows Only. Name of the resource in the .rc file."));

    return propResName;
}

wxPGProperty* wxWeaverBitmapProperty::CreatePropertyIconSize()
{
    // Create 'ico_size' property ('Load From Icon Resource' only)
    wxPGProperty* propIcoSize
        = new wxWeaverSizeProperty("ico_size", wxPG_LABEL, wxDefaultSize);

    propIcoSize->SetHelpString(
        _("The size of the icon to use from a ICON resource with multiple icons in it."));

    return propIcoSize;
}

wxPGProperty* wxWeaverBitmapProperty::CreatePropertyXrcName()
{
    // Create 'xrc_name' property ('Load From XRC' only)
    wxPGProperty* propXRCName = new wxStringProperty("xrc_name", wxPG_LABEL);
    propXRCName->SetHelpString(_("Name of the item in the XRC ressources."));
    return propXRCName;
}

wxPGProperty* wxWeaverBitmapProperty::CreatePropertyArtId()
{
    // Create 'id' property ('Load From Art Provider' only)
    wxPGChoices artIdChoices;
    artIdChoices.Add("wxART_ADD_BOOKMARK");
    artIdChoices.Add("wxART_DEL_BOOKMARK");
    artIdChoices.Add("wxART_HELP_SIDE_PANEL");
    artIdChoices.Add("wxART_HELP_SETTINGS");
    artIdChoices.Add("wxART_HELP_BOOK");
    artIdChoices.Add("wxART_HELP_FOLDER");
    artIdChoices.Add("wxART_HELP_PAGE");
    artIdChoices.Add("wxART_GO_BACK");
    artIdChoices.Add("wxART_GO_FORWARD");
    artIdChoices.Add("wxART_GO_UP");
    artIdChoices.Add("wxART_GO_DOWN");
    artIdChoices.Add("wxART_GO_TO_PARENT");
    artIdChoices.Add("wxART_GO_HOME");
    artIdChoices.Add("wxART_FILE_OPEN");
    artIdChoices.Add("wxART_FILE_SAVE");
    artIdChoices.Add("wxART_FILE_SAVE_AS");
    artIdChoices.Add("wxART_GOTO_FIRST");
    artIdChoices.Add("wxART_GOTO_LAST");
    artIdChoices.Add("wxART_PRINT");
    artIdChoices.Add("wxART_HELP");
    artIdChoices.Add("wxART_TIP");
    artIdChoices.Add("wxART_REPORT_VIEW");
    artIdChoices.Add("wxART_LIST_VIEW");
    artIdChoices.Add("wxART_NEW_DIR");
    artIdChoices.Add("wxART_HARDDISK");
    artIdChoices.Add("wxART_FLOPPY");
    artIdChoices.Add("wxART_CDROM");
    artIdChoices.Add("wxART_REMOVABLE");
    artIdChoices.Add("wxART_FOLDER");
    artIdChoices.Add("wxART_FOLDER_OPEN");
    artIdChoices.Add("wxART_GO_DIR_UP");
    artIdChoices.Add("wxART_EXECUTABLE_FILE");
    artIdChoices.Add("wxART_NORMAL_FILE");
    artIdChoices.Add("wxART_TICK_MARK");
    artIdChoices.Add("wxART_CROSS_MARK");
    artIdChoices.Add("wxART_ERROR");
    artIdChoices.Add("wxART_QUESTION");
    artIdChoices.Add("wxART_WARNING");
    artIdChoices.Add("wxART_INFORMATION");
    artIdChoices.Add("wxART_MISSING_IMAGE");
    artIdChoices.Add("wxART_COPY");
    artIdChoices.Add("wxART_CUT");
    artIdChoices.Add("wxART_PASTE");
    artIdChoices.Add("wxART_DELETE");
    artIdChoices.Add("wxART_NEW");
    artIdChoices.Add("wxART_UNDO");
    artIdChoices.Add("wxART_REDO");
    artIdChoices.Add("wxART_PLUS");
    artIdChoices.Add("wxART_MINUS");
    artIdChoices.Add("wxART_CLOSE");
    artIdChoices.Add("wxART_QUIT");
    artIdChoices.Add("wxART_FIND");
    artIdChoices.Add("wxART_FIND_AND_REPLACE");
    artIdChoices.Add("wxART_FULL_SCREEN");
    artIdChoices.Add("wxART_EDIT");

    // TODO: Replace with freedesktop ones onlym use #if defined(__wxGTK__) macro
    artIdChoices.Add("gtk-about");
    artIdChoices.Add("gtk-add");
    artIdChoices.Add("gtk-apply");
    artIdChoices.Add("gtk-bold");
    artIdChoices.Add("gtk-cancel");
    artIdChoices.Add("gtk-caps-lock-warning");
    artIdChoices.Add("gtk-cdrom");
    artIdChoices.Add("gtk-clear");
    artIdChoices.Add("gtk-close");
    artIdChoices.Add("gtk-color-picker");
    artIdChoices.Add("gtk-convert");
    artIdChoices.Add("gtk-copy");
    artIdChoices.Add("gtk-cut");
    artIdChoices.Add("gtk-delete");
    artIdChoices.Add("gtk-dialog-authentication");
    artIdChoices.Add("gtk-dialog-error");
    artIdChoices.Add("gtk-dialog-info");
    artIdChoices.Add("gtk-dialog-question");
    artIdChoices.Add("gtk-dialog-warning");
    artIdChoices.Add("gtk-warning");
    artIdChoices.Add("gtk-discard");
    artIdChoices.Add("gtk-disconnect");
    artIdChoices.Add("gtk-dnd");
    artIdChoices.Add("gtk-dnd-multiple");
    artIdChoices.Add("gtk-edit");
    artIdChoices.Add("gtk-execute");
    artIdChoices.Add("gtk-file");
    artIdChoices.Add("gtk-find");
    artIdChoices.Add("gtk-find-and-replace");
    artIdChoices.Add("gtk-fullscreen");
    artIdChoices.Add("gtk-goto-bottom");
    artIdChoices.Add("gtk-goto-first");
    artIdChoices.Add("gtk-goto-last");
    artIdChoices.Add("gtk-goto-top");
    artIdChoices.Add("gtk-go-back");
    artIdChoices.Add("gtk-go-down");
    artIdChoices.Add("gtk-go-forward");
    artIdChoices.Add("gtk-go-up");
    artIdChoices.Add("gtk-harddisk");
    artIdChoices.Add("gtk-indent");
    artIdChoices.Add("gtk-index");
    artIdChoices.Add("gtk-info");
    artIdChoices.Add("gtk-italic");
    artIdChoices.Add("gtk-jump-to");
    artIdChoices.Add("gtk-justify-center");
    artIdChoices.Add("gtk-justify-fill");
    artIdChoices.Add("gtk-justify-left");
    artIdChoices.Add("gtk-justify-right");
    artIdChoices.Add("gtk-leave-fullscreen");
    artIdChoices.Add("gtk-media-forward");
    artIdChoices.Add("gtk-media-next");
    artIdChoices.Add("gtk-media-forward");
    artIdChoices.Add("gtk-media-pause");
    artIdChoices.Add("gtk-media-play");
    artIdChoices.Add("gtk-media-previous");
    artIdChoices.Add("gtk-media-record");
    artIdChoices.Add("gtk-media-rewind");
    artIdChoices.Add("gtk-media-stop");
    artIdChoices.Add("gtk-missing-image");
    artIdChoices.Add("gtk-network");
    artIdChoices.Add("gtk-new");
    artIdChoices.Add("gtk-no");
    artIdChoices.Add("gtk-ok");
    artIdChoices.Add("gtk-open");
    artIdChoices.Add("gtk-orientation-landscape");
    artIdChoices.Add("gtk-orientation-portrait");
    artIdChoices.Add("gtk-orientation-reverse-landscape");
    artIdChoices.Add("gtk-orientation-reverse-portrait");
    artIdChoices.Add("gtk-page-setup");
    artIdChoices.Add("gtk-paste");
    artIdChoices.Add("gtk-preferences");
    artIdChoices.Add("gtk-print");
    artIdChoices.Add("gtk-print-paused");
    artIdChoices.Add("gtk-print-report");
    artIdChoices.Add("gtk-print-warning");
    artIdChoices.Add("gtk-properties");
    artIdChoices.Add("gtk-quit");
    artIdChoices.Add("gtk-redo");
    artIdChoices.Add("gtk-refresh");
    artIdChoices.Add("gtk-remove");
    artIdChoices.Add("gtk-save");
    artIdChoices.Add("gtk-save-as");
    artIdChoices.Add("gtk-select-all");
    artIdChoices.Add("gtk-select-color");
    artIdChoices.Add("gtk-select-font");
    artIdChoices.Add("gtk-sort-ascending");
    artIdChoices.Add("gtk-sort-descending");
    artIdChoices.Add("gtk-spell-check");
    artIdChoices.Add("gtk-stop");
    artIdChoices.Add("gtk-strikethrough");
    artIdChoices.Add("gtk-undelete");
    artIdChoices.Add("gtk-underline");
    artIdChoices.Add("gtk-undo");
    artIdChoices.Add("gtk-unindent");
    artIdChoices.Add("gtk-yes");
    artIdChoices.Add("gtk-zoom-100");
    artIdChoices.Add("gtk-zoom-fit");
    artIdChoices.Add("gtk-zoom-in");
    artIdChoices.Add("gtk-zoom-out");

    wxPGProperty* propArtId
        = new wxEditEnumProperty("id", wxPG_LABEL, artIdChoices);

    propArtId->SetHelpString(
        _("Choose a wxArtID unique identifier of the bitmap or enter a wxArtID for your custom wxArtProvider. IDs with prefix 'gtk-' are available under wxGTK only."));

    return propArtId;
}

wxPGProperty* wxWeaverBitmapProperty::CreatePropertyArtClient()
{
    // Create 'client' property ('Load From Art Provider' only)
    wxPGChoices artClientChoices;
    artClientChoices.Add("wxART_TOOLBAR");
    artClientChoices.Add("wxART_MENU");
    artClientChoices.Add("wxART_BUTTON");
    artClientChoices.Add("wxART_FRAME_ICON");
    artClientChoices.Add("wxART_CMN_DIALOG");
    artClientChoices.Add("wxART_HELP_BROWSER");
    artClientChoices.Add("wxART_MESSAGE_BOX");
    artClientChoices.Add("wxART_OTHER");

    wxPGProperty* propArtClient
        = new wxEditEnumProperty("client", wxPG_LABEL, artClientChoices);

    propArtClient->SetHelpString(
        _("Choose a wxArtClient identifier of the client (i.e. who is asking for the bitmap) or enter a wxArtClient for your custom wxArtProvider."));

    return propArtClient;
}

wxWeaverBitmapProperty::~wxWeaverBitmapProperty()
{
}

wxVariant wxWeaverBitmapProperty::ChildChanged(wxVariant& thisValue, const int childIndex,
                                               wxVariant& childValue) const
{
    wxWeaverBitmapProperty* bp = const_cast<wxWeaverBitmapProperty*>(this);

    const auto val = thisValue.GetString();
    wxArrayString childVals;
    GetChildValues(val, childVals);
    auto newVal = val;

    // Find the appropriate new state
    switch (childIndex) {
    // source
    case 0: {
        const auto count = GetChildCount();

        // childValue.GetInteger() returns the chosen item index
        switch (childValue.GetInteger()) {
        // 'Load From File' and 'Load From Embedded File'
        case 0:
        case 1: {
            if (prevSrc != 0 && prevSrc != 1) {
                for (size_t i = 1; i < count; ++i) {
                    if (auto* p = Item(i)) {
                        wxLogDebug(
                            "wxWeaverBP::ChildChanged: Removing:%s",
                            p->GetLabel().c_str());

                        GetGrid()->DeleteProperty(p);
                    }
                }
                bp->AppendChild(bp->CreatePropertyFilePath());
            }
            if (childVals.GetCount() == 2)
                newVal = childVals.Item(0) + "; " + childVals.Item(1);
            else if (childVals.GetCount() > 0)
                newVal = childVals.Item(0) + "; ";

            break;
        }
        // 'Load From Resource'
        case 2: {
            if (prevSrc != 2) {
                for (size_t i = 1; i < count; ++i) {
                    if (auto* p = Item(i)) {
                        wxLogDebug(
                            "wxWeaverBP::ChildChanged: Removing:%s",
                            p->GetLabel().c_str());

                        GetGrid()->DeleteProperty(p);
                    }
                }
                bp->AppendChild(bp->CreatePropertyResourceName());
            }
            if (childVals.GetCount() == 2)
                newVal = childVals.Item(0) + "; " + childVals.Item(1);
            else if (childVals.GetCount() > 0)
                newVal = childVals.Item(0) + "; ";

            break;
        }
        // 'Load From Icon Resource'
        case 3: {
            if (prevSrc != 3) {
                for (size_t i = 1; i < count; ++i) {
                    if (auto* p = Item(i)) {
                        wxLogDebug(
                            "wxWeaverBP::ChildChanged: Removing:%s",
                            p->GetLabel().c_str());

                        GetGrid()->DeleteProperty(p);
                    }
                }
                bp->AppendChild(bp->CreatePropertyResourceName());
                bp->AppendChild(bp->CreatePropertyIconSize());
            }

            if (childVals.GetCount() == 3) {
                newVal = childVals.Item(0) + "; " + childVals.Item(1)
                    + "; [" + childVals.Item(2) + "]";
            } else if (childVals.GetCount() > 0) {
                newVal = childVals.Item(0) + "; ; []";
            }
            break;
        }
        // 'Load From XRC'
        case 4: {
            if (prevSrc != 4) {
                for (size_t i = 1; i < count; ++i) {
                    if (auto* p = Item(i)) {
                        wxLogDebug(
                            "wxWeaverBP::ChildChanged: Removing:%s",
                            p->GetLabel().c_str());

                        GetGrid()->DeleteProperty(p);
                    }
                }
                bp->AppendChild(bp->CreatePropertyXrcName());
            }
            if (childVals.GetCount() == 2)
                newVal = childVals.Item(0) + "; " + childVals.Item(1);
            else if (childVals.GetCount() > 0)
                newVal = childVals.Item(0) + "; ";

            break;
        }
        // 'Load From Art Provider'
        case 5: {
            if (prevSrc != 5) {
                for (size_t i = 1; i < count; ++i) {
                    if (auto* p = Item(i)) {
                        wxLogDebug(
                            "wxWeaverBP::ChildChanged: Removing:%s",
                            p->GetLabel().c_str());

                        GetGrid()->DeleteProperty(p);
                    }
                }
                bp->AppendChild(bp->CreatePropertyArtId());
                bp->AppendChild(bp->CreatePropertyArtClient());
            }

            if (childVals.GetCount() == 3) {
                newVal = childVals.Item(0) + "; " + childVals.Item(1)
                    + "; " + childVals.Item(2);
            } else if (childVals.GetCount() > 0) {
                newVal = childVals.Item(0) + "; ; ";
            }
            break;
        }
        } // switch (childValue.GetInteger())
        break;
    }
    // file_path || id || resource_name || xrc_name
    case 1: {
        if ((Item(0)->GetValueAsString() == _("Load From File")) || (Item(0)->GetValueAsString() == _("Load From Embedded File"))) {
            // Save the initial file path TODO: Save the image filter index
            if (Item(1)) {
                wxString img = childValue.GetString();
                img = bp->SetupImage(img);
                wxFileName imgPath(img);
                gs_imageInitialPath = imgPath.GetPath();

                if (!img.IsEmpty()) {
                    Item(1)->SetValue(WXVARIANT(img));
                } else {
                    Item(1)->SetValueToUnspecified();
                }
                newVal = Item(0)->GetValueAsString() + "; " + img;
            }
        }
        break;
    }
    }
    bp->SetPrevSource(childValue.GetInteger());

    if (newVal != val) {
        wxVariant ret = WXVARIANT(newVal);
        bp->SetValue(ret);

        return ret;
    }
    return thisValue;
}

void wxWeaverBitmapProperty::UpdateChildValues(const wxString& value)
{
    wxArrayString childVals;
    GetChildValues(value, childVals);

    if (childVals[0].Contains(_("Load From File"))
        || childVals[0].Contains(_("Load From Embedded File"))) {
        if (childVals.Count() > 1) {
            wxString img = childVals[1];
            img = SetupImage(img);
            wxFileName imgPath(img);
            gs_imageInitialPath = imgPath.GetPath();

            if (!img.IsEmpty())
                Item(1)->SetValue(WXVARIANT(img));
            else
                Item(1)->SetValueToUnspecified();
        }
    } else if (childVals[0].Contains(_("Load From Resource"))) {
        if (childVals.Count() > 1)
            Item(1)->SetValue(childVals[1]);

    } else if (childVals[0].Contains(_("Load From Icon Resource"))) {
        if (childVals.Count() > 1)
            Item(1)->SetValue(childVals[1]);

        if (childVals.Count() > 2) {
            // This child requires a wxSize as data type, not a wxString
            // The string format of a wxSize doesn't match the display format,
            // convert it like ObjectInspector does
            wxString aux = childVals[2];
            aux.Replace(";", ",");
            Item(2)->SetValue(WXVARIANT(TypeConv::StringToSize(aux)));
        }
    } else if (childVals[0].Contains(_("Load From XRC"))) {
        if (childVals.Count() > 1)
            Item(1)->SetValue(childVals[1]);

    } else if (childVals[0].Contains(_("Load From Art Provider"))) {
        if (childVals.Count() > 1)
            Item(1)->SetValue(childVals[1]);

        if (childVals.Count() > 2)
            Item(2)->SetValue(childVals[2]);
    }
}

void wxWeaverBitmapProperty::OnSetValue()
{
}

wxString wxWeaverBitmapProperty::SetupImage(const wxString& imgPath)
{
    if (!imgPath.IsEmpty()) {
        wxFileName imgName = wxFileName(imgPath);

        // Allow user to specify any file path he needs (even if it seemingly doesn't exist)
        if (!imgName.FileExists())
            return imgPath;

        wxString res = "";
        wxImage img = wxImage(imgPath);
        if (!img.IsOk())
            return res;

        // Setup for correct file_path
        if (imgName.IsAbsolute()) {
            return TypeConv::MakeRelativeURL(imgPath, AppData()->GetProjectPath());
        } else {
            imgName.MakeAbsolute(AppData()->GetProjectPath());

            if (!imgName.FileExists())
                return res;
        }
    }
    return imgPath;
}

wxString wxWeaverBitmapProperty::SetupResource(const wxString& resName)
{
    wxString res = wxEmptyString;
    // Keep old value from an icon resource only
    if (resName.Contains(";") && resName.Contains("["))
        return resName.BeforeFirst(';');
    else if (resName.Contains(";"))
        return res;

    return resName;
}

#ifdef wxUSE_SLIDER
// -----------------------------------------------------------------------
// wxSlider-based property editor
// -----------------------------------------------------------------------
wxIMPLEMENT_DYNAMIC_CLASS(wxPGSliderEditor, wxPGEditor);

wxPGSliderEditor::~wxPGSliderEditor()
{
}

// Create controls and initialize event handling.
wxPGWindowList wxPGSliderEditor::CreateControls(wxPropertyGrid* propgrid,
                                                wxPGProperty* property,
                                                const wxPoint& pos,
                                                const wxSize& sz) const
{
    wxCHECK_MSG(property->IsKindOf(wxCLASSINFO(wxFloatProperty)),
                nullptr,
                "Slider editor can only be used with wxFloatProperty or derivative.");

    // Use two stage creation to allow cleaner display on wxMSW
    wxSlider* ctrl = new wxSlider();
#ifdef __WXMSW__
    ctrl->Hide();
#endif
    wxString s = property->GetValueAsString();
    double v_d = 0;
    if (s.ToDouble(&v_d)) {
        if (v_d < 0)
            v_d = 0;
        else if (v_d > 1)
            v_d = 1;
    }
    ctrl->Create(
        propgrid->GetPanel(),
        wxID_ANY, (int)(v_d * m_max), 0, m_max, pos, sz, wxSL_HORIZONTAL);
/*
    Connect all required events to grid's OnCustomEditorEvent
    (all relevenat wxTextCtrl, wxComboBox and wxButton events are already connected)
*/
#ifdef __WXMSW__
    ctrl->Show();
#endif
    return ctrl;
}

// Copies value from property to control
void wxPGSliderEditor::UpdateControl(wxPGProperty* property, wxWindow* wnd) const
{
    wxSlider* ctrl = (wxSlider*)wnd;
    assert(ctrl && ctrl->IsKindOf(CLASSINFO(wxSlider)));

    double val = property->GetValue().GetDouble();
    if (val < 0)
        val = 0;
    else if (val > 1)
        val = 1;

    ctrl->SetValue((int)(val * m_max));
}

// Control's events are redirected here
bool wxPGSliderEditor::OnEvent(wxPropertyGrid* WXUNUSED(propgrid),
                               wxPGProperty* property, wxWindow* wnd, wxEvent& event) const
{
    if (event.GetEventType() == wxEVT_SCROLL_THUMBTRACK) {
        wxSlider* ctrl = wxDynamicCast(wnd, wxSlider);
        if (ctrl) {
            double val = (double)(ctrl->GetValue()) / (double)(m_max);
            property->SetValue(WXVARIANT(val));
            return true;
        }
    }
    return false;
}

bool wxPGSliderEditor::GetValueFromControl(wxVariant& variant,
                                           wxPGProperty* WXUNUSED(property),
                                           wxWindow* wnd) const
{
    wxSlider* ctrl = (wxSlider*)wnd;
    assert(ctrl && ctrl->IsKindOf(wxCLASSINFO(wxSlider)));

    variant = WXVARIANT((double)ctrl->GetValue() / (double)(m_max));
    return true;
}

void wxPGSliderEditor::SetValueToUnspecified(wxPGProperty* WXUNUSED(property),
                                             wxWindow* WXUNUSED(ctrl)) const
{
}
#endif //wxUSE_SLIDER

// -----------------------------------------------------------------------
// wxWeaverFontProperty
// -----------------------------------------------------------------------

#include <wx/fontdlg.h>
#include <wx/fontenum.h>

static const wxChar* gs_fp_es_family_labels[] = {
    wxS("Default"), wxS("Decorative"),
    wxS("Roman"), wxS("Script"),
    wxS("Swiss"), wxS("Modern"),
    wxS("Teletype"), wxS("Unknown"),
    (const wxChar*)nullptr
};

static long gs_fp_es_family_values[] = {
    wxFONTFAMILY_DEFAULT, wxFONTFAMILY_DECORATIVE,
    wxFONTFAMILY_ROMAN, wxFONTFAMILY_SCRIPT,
    wxFONTFAMILY_SWISS, wxFONTFAMILY_MODERN,
    wxFONTFAMILY_TELETYPE, wxFONTFAMILY_UNKNOWN
};

static const wxChar* gs_fp_es_style_labels[] = {
    wxS("Normal"),
    wxS("Slant"),
    wxS("Italic"),
    (const wxChar*)nullptr
};

static long gs_fp_es_style_values[] = {
    wxFONTSTYLE_NORMAL,
    wxFONTSTYLE_SLANT,
    wxFONTSTYLE_ITALIC
};

static const wxChar* gs_fp_es_weight_labels[] = {
    wxS("Normal"),
    wxS("Light"),
    wxS("Bold"),
    (const wxChar*)nullptr
};

static long gs_fp_es_weight_values[] = {
    wxFONTWEIGHT_NORMAL,
    wxFONTWEIGHT_LIGHT,
    wxFONTWEIGHT_BOLD
};

#if wxCHECK_VERSION(3, 1, 0)
wxPG_IMPLEMENT_PROPERTY_CLASS(wxWeaverFontProperty, wxPGProperty, TextCtrlAndButton)
#else
WX_PG_IMPLEMENT_PROPERTY_CLASS(wxWeaverFontProperty, wxPGProperty,
                               wxFont, const wxFont&, TextCtrlAndButton)
#endif
    wxWeaverFontProperty::wxWeaverFontProperty(const wxString& label,
                                               const wxString& name,
                                               const wxFontContainer& value)
    : wxPGProperty(label, name)
{
    SetValue(WXVARIANT(TypeConv::FontToString(value)));

    // Initialize font family choices list
    if (!wxPGGlobalVars->m_fontFamilyChoices) {
        wxFontEnumerator enumerator;
        enumerator.EnumerateFacenames();

        wxArrayString faceNames = enumerator.GetFacenames();
        faceNames.Sort();
        faceNames.Insert(wxEmptyString, 0);

        wxPGGlobalVars->m_fontFamilyChoices = new wxPGChoices(faceNames);
    }
    wxString emptyString(wxEmptyString); // TODO: ?????????

    AddPrivateChild(new wxIntProperty(_("Point Size"), "Point Size",
                                      value.m_pointSize));

    AddPrivateChild(new wxEnumProperty(_("Family"), "Family",
                                       gs_fp_es_family_labels, gs_fp_es_family_values,
                                       value.m_family));

    wxString faceName = value.m_faceName;
    // If font was not in there, add it now
    if (faceName.length()
        && wxPGGlobalVars->m_fontFamilyChoices->Index(faceName) == wxNOT_FOUND)

        wxPGGlobalVars->m_fontFamilyChoices->AddAsSorted(faceName);

    wxPGProperty* p = new wxEnumProperty(_("Face Name"), "Face Name",
                                         *wxPGGlobalVars->m_fontFamilyChoices);
    p->SetValueFromString(faceName, wxPG_FULL_VALUE);
    AddPrivateChild(p);

    AddPrivateChild(new wxEnumProperty(_("Style"), "Style",
                                       gs_fp_es_style_labels, gs_fp_es_style_values,
                                       value.m_style));

    AddPrivateChild(new wxEnumProperty(_("Weight"), "Weight",
                                       gs_fp_es_weight_labels, gs_fp_es_weight_values,
                                       value.m_weight));

    AddPrivateChild(new wxBoolProperty(_("Underlined"), "Underlined",
                                       value.m_underlined));
}

wxWeaverFontProperty::~wxWeaverFontProperty() { }

void wxWeaverFontProperty::OnSetValue()
{
    // do nothing
}

wxString wxWeaverFontProperty::GetValueAsString(int argFlags) const
{
    return wxPGProperty::GetValueAsString(argFlags);
}

bool wxWeaverFontProperty::OnEvent(wxPropertyGrid* propgrid,
                                   wxWindow* WXUNUSED(primary),
                                   wxEvent& event)
{
    if (propgrid->IsMainButtonEvent(event)) {
        // Update value from last minute changes
        wxFontData data;
        wxFont font = TypeConv::StringToFont(m_value.GetString());
        data.SetInitialFont(font);
        data.SetColour(*wxBLACK);

        wxFontDialog dlg(propgrid, data);
        if (dlg.ShowModal() == wxID_OK) {
            propgrid->EditorsValueWasModified();

            wxFontContainer fcont(dlg.GetFontData().GetChosenFont());
            wxVariant variant = WXVARIANT(TypeConv::FontToString(fcont));
            SetValueInEvent(variant);
            return true;
        }
    }
    return false;
}

void wxWeaverFontProperty::RefreshChildren()
{
    wxString fstr = m_value.GetString();
    wxFontContainer font = TypeConv::StringToFont(fstr);
    Item(0)->SetValue(font.m_pointSize);
    Item(1)->SetValue(font.m_family);
    Item(2)->SetValueFromString(font.m_faceName, wxPG_FULL_VALUE);
    Item(3)->SetValue(font.m_style);
    Item(4)->SetValue(font.m_weight);
    Item(5)->SetValue(font.m_underlined);
}

wxVariant
wxWeaverFontProperty::ChildChanged(wxVariant& thisValue, int index,
                                   wxVariant& childValue) const
{
    wxFontContainer font = TypeConv::StringToFont(thisValue.GetString());
    if (index == 0) {
        font.m_pointSize = childValue.GetLong();
    } else if (index == 1) {
        int fam = childValue.GetLong();
        if (fam < wxFONTFAMILY_DEFAULT || fam > wxFONTFAMILY_TELETYPE)
            fam = wxFONTFAMILY_DEFAULT;

        font.m_family = static_cast<wxFontFamily>(fam);
    } else if (index == 2) {
        wxString faceName;
        int faceIndex = childValue.GetLong();

        if (faceIndex >= 0)
            faceName = wxPGGlobalVars->m_fontFamilyChoices->GetLabel(faceIndex);

        font.m_faceName = faceName;
    } else if (index == 3) {
        int st = childValue.GetLong();
        if (st != wxFONTSTYLE_NORMAL && st != wxFONTSTYLE_SLANT
            && st != wxFONTSTYLE_ITALIC) {
            st = wxFONTSTYLE_NORMAL;
        }
        font.m_style = static_cast<wxFontStyle>(st);
    } else if (index == 4) {
        int wt = childValue.GetLong();
        if (wt != wxFONTWEIGHT_NORMAL && wt != wxFONTWEIGHT_LIGHT
            && wt != wxFONTWEIGHT_BOLD)
            wt = wxFONTWEIGHT_NORMAL;
        font.m_weight = static_cast<wxFontWeight>(wt);
    } else if (index == 5) {
        font.m_underlined = childValue.GetBool();
    }
    thisValue = WXVARIANT(TypeConv::FontToString(font));
    return thisValue;
}
