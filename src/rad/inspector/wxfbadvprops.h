/*
    wxWeaver - A GUI Designer Editor for wxWidgets.
    Copyright (C) 2005 José Antonio Hurtado (as wxFormBuilder)
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
#ifndef __wxWEAVERADVPROPS_H__
#define __wxWEAVERADVPROPS_H__

#include "fontcontainer.h"

#include <wx/propgrid/propgrid.h>
#include <wx/propgrid/advprops.h>

// -----------------------------------------------------------------------
// wxWeaverSizeProperty
// -----------------------------------------------------------------------

class wxWeaverSizeProperty : public wxPGProperty
{
    WX_PG_DECLARE_PROPERTY_CLASS( wxWeaverSizeProperty )
public:
    wxWeaverSizeProperty( const wxString& label = wxPG_LABEL,
                      const wxString& name  = wxPG_LABEL,
                      const wxSize&   value = wxSize() );

	wxVariant ChildChanged(wxVariant& thisValue, int childIndex,
	                       wxVariant& childValue) const override;

	void RefreshChildren() override;

protected:
    void DoSetValue( const wxSize& value ) { m_value = WXVARIANT( value ); }
};

// -----------------------------------------------------------------------
// wxWeaverPointProperty
// -----------------------------------------------------------------------

class wxWeaverPointProperty : public wxPGProperty
{
    WX_PG_DECLARE_PROPERTY_CLASS( wxWeaverPointProperty )
public:
    wxWeaverPointProperty( const wxString& label = wxPG_LABEL,
                       const wxString& name  = wxPG_LABEL,
                       const wxPoint&  value = wxPoint() );
	~wxWeaverPointProperty() override;

	wxVariant ChildChanged(wxVariant& thisValue, int childIndex,
	                       wxVariant& childValue) const override;

	void RefreshChildren() override;

protected:
    void DoSetValue( const wxPoint& value ) { m_value = WXVARIANT( value ); }
};

// -----------------------------------------------------------------------
// wxWeaverBitmapProperty
// -----------------------------------------------------------------------

class wxWeaverBitmapProperty : public wxPGProperty
{
    WX_PG_DECLARE_PROPERTY_CLASS( wxWeaverBitmapProperty )

public:
    wxWeaverBitmapProperty( const wxString& label = wxPG_LABEL,
                        const wxString& name  = wxPG_LABEL,
                        const wxString& value = wxString() );

	~wxWeaverBitmapProperty() override;

    wxPGProperty *CreatePropertySource( int sourceIndex = 0 );
    wxPGProperty *CreatePropertyFilePath() ;
    wxPGProperty *CreatePropertyResourceName();
    wxPGProperty *CreatePropertyIconSize();
	wxPGProperty *CreatePropertyXrcName();
    wxPGProperty *CreatePropertyArtId();
    wxPGProperty *CreatePropertyArtClient();

    wxString SetupImage( const wxString &imgPath = wxEmptyString ) ;
    wxString SetupResource( const wxString &resName = wxEmptyString ) ;

	int prevSrc;
	void SetPrevSource(int src){prevSrc = src;}

	wxVariant ChildChanged(wxVariant& thisValue, int childIndex,
	                       wxVariant& childValue) const override;

	void OnSetValue() override;
	void CreateChildren();

	void UpdateChildValues(const wxString& value);
protected:

	void GetChildValues( const wxString& parentValue, wxArrayString& childValues ) const;

    static wxArrayString m_ids;
    static wxArrayString m_clients;
    wxArrayString m_strings;


};

// -----------------------------------------------------------------------
// wxSlider-based property editor
// -----------------------------------------------------------------------

#if wxUSE_SLIDER
//
// Implement an editor control that allows using wxSlider to edit value of
// wxFloatProperty (and similar).
//
// Note that new editor classes needs to be registered before use.
// This can be accomplished using wxPGRegisterEditorClass macro.
// Registeration can also be performed in a constructor of a
// property that is likely to require the editor in question.
//

class wxPGSliderEditor : public wxPGEditor
{
    wxDECLARE_DYNAMIC_CLASS( wxPGSliderEditor );
public:
    wxPGSliderEditor()
    :
    m_max( 10000 )
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
};
#endif // wxUSE_SLIDER

// -----------------------------------------------------------------------
// wxWeaverFontProperty
// -----------------------------------------------------------------------

class wxWeaverFontProperty : public wxPGProperty
{
    WX_PG_DECLARE_PROPERTY_CLASS(wxWeaverFontProperty)
public:

    wxWeaverFontProperty( const wxString& label = wxPG_LABEL, const wxString& name = wxPG_LABEL, const wxFontContainer& value = *wxNORMAL_FONT);
	~wxWeaverFontProperty() override;

	wxVariant ChildChanged(wxVariant& thisValue, int childIndex,
	                       wxVariant& childValue) const override;

	void RefreshChildren() override;

	void OnSetValue() override;
	wxString GetValueAsString(int argFlags = 0) const override;

	bool OnEvent(wxPropertyGrid* propgrid, wxWindow* primary, wxEvent& event) override;
};

#endif //__wxWEAVERADVPROPS_H__
