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

#include "fontcontainer.h"

#include <wx/propgrid/propgrid.h>
#include <wx/propgrid/advprops.h>

// TODO: Slider editor
#if 0
#if wxUSE_SLIDER
WX_PG_DECLARE_EDITOR(Slider)
#endif
#endif

// -----------------------------------------------------------------------
// wxWeaverSizeProperty
// -----------------------------------------------------------------------
class wxWeaverSizeProperty : public wxPGProperty {
public:
    wxWeaverSizeProperty(const wxString& label = wxPG_LABEL,
                         const wxString& name = wxPG_LABEL,
                         const wxSize& value = wxSize());

    wxVariant ChildChanged(wxVariant& thisValue, int childIndex,
                           wxVariant& childValue) const override;

    void RefreshChildren() override;

protected:
    void DoSetValue(const wxSize& value) { m_value = WXVARIANT(value); }

private:
    WX_PG_DECLARE_PROPERTY_CLASS(wxWeaverSizeProperty)
};

// -----------------------------------------------------------------------
// wxWeaverPointProperty
// -----------------------------------------------------------------------
class wxWeaverPointProperty : public wxPGProperty {
public:
    wxWeaverPointProperty(const wxString& label = wxPG_LABEL,
                          const wxString& name = wxPG_LABEL,
                          const wxPoint& value = wxPoint());
    ~wxWeaverPointProperty() override;

    wxVariant ChildChanged(wxVariant& thisValue, int childIndex,
                           wxVariant& childValue) const override;

    void RefreshChildren() override;

protected:
    void DoSetValue(const wxPoint& value) { m_value = WXVARIANT(value); }

private:
    WX_PG_DECLARE_PROPERTY_CLASS(wxWeaverPointProperty)
};

// -----------------------------------------------------------------------
// wxWeaverBitmapProperty
// -----------------------------------------------------------------------
class wxWeaverBitmapProperty : public wxPGProperty {
public:
    wxWeaverBitmapProperty(const wxString& label = wxPG_LABEL,
                           const wxString& name = wxPG_LABEL,
                           const wxString& value = wxString());
    ~wxWeaverBitmapProperty() override;

    wxPGProperty* CreatePropertySource(int sourceIndex = 0);
    wxPGProperty* CreatePropertyFilePath();
    wxPGProperty* CreatePropertyResourceName();
    wxPGProperty* CreatePropertyIconSize();
    wxPGProperty* CreatePropertyXrcName();
    wxPGProperty* CreatePropertyArtId();
    wxPGProperty* CreatePropertyArtClient();

    wxString SetupImage(const wxString& imgPath = wxEmptyString);
    wxString SetupResource(const wxString& resName = wxEmptyString);

    void SetPrevSource(int src) { prevSrc = src; }

    wxVariant ChildChanged(wxVariant& thisValue, int childIndex,
                           wxVariant& childValue) const override;

    void OnSetValue() override;
    void CreateChildren();
    void UpdateChildValues(const wxString& value);

private:
    void GetChildValues(const wxString& parentValue, wxArrayString& childValues) const;

    int prevSrc;

    WX_PG_DECLARE_PROPERTY_CLASS(wxWeaverBitmapProperty)
};

#if wxUSE_SLIDER
// -----------------------------------------------------------------------
// wxSlider-based property editor
// -----------------------------------------------------------------------
/*
    Implement an editor control that allows using wxSlider to edit value of
    wxFloatProperty (and similar).

    Note that new editor classes needs to be registered before use.
    This can be accomplished using wxPGRegisterEditorClass macro.
    Registration can also be performed in a constructor of a
    property that is likely to require the editor in question.
*/
class wxPGSliderEditor : public wxPGEditor {
public:
    wxPGSliderEditor()
        : m_max(10000)
    {
    }
    ~wxPGSliderEditor() override;

    wxPGWindowList CreateControls(wxPropertyGrid* propgrid, wxPGProperty* property,
                                  const wxPoint& pos, const wxSize& size) const override;
    void UpdateControl(wxPGProperty* property, wxWindow* wnd) const override;
    bool OnEvent(wxPropertyGrid* propgrid, wxPGProperty* property, wxWindow* wnd,
                 wxEvent& event) const override;
    bool GetValueFromControl(wxVariant& variant, wxPGProperty* property,
                             wxWindow* ctrl) const override;
    void SetValueToUnspecified(wxPGProperty* property, wxWindow* ctrl) const override;

private:
    int m_max;

    wxDECLARE_DYNAMIC_CLASS(wxPGSliderEditor);
};
#endif // wxUSE_SLIDER

// -----------------------------------------------------------------------
// wxWeaverFontProperty
// -----------------------------------------------------------------------
class wxWeaverFontProperty : public wxPGProperty {
public:
    wxWeaverFontProperty(const wxString& label = wxPG_LABEL,
                         const wxString& name = wxPG_LABEL,
                         const wxFontContainer& value = *wxNORMAL_FONT);
    ~wxWeaverFontProperty() override;

    wxVariant ChildChanged(wxVariant& thisValue, int childIndex,
                           wxVariant& childValue) const override;

    void RefreshChildren() override;
    wxString GetValueAsString(int argFlags = 0) const override;
    void OnSetValue() override;
    bool OnEvent(wxPropertyGrid* propgrid, wxWindow* primary, wxEvent& event) override;

private:
    WX_PG_DECLARE_PROPERTY_CLASS(wxWeaverFontProperty)
};
