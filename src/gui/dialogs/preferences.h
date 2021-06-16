/*
    wxWeaver - A GUI Designer Editor for wxWidgets.
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

#include <wx/artprov.h>
#include <wx/panel.h>
#include <wx/preferences.h>

#include <memory>

namespace wxw {
class Preferences;
}
class wxBitmapComboBox;
class wxCheckBox;
class wxChoice;
class wxFontPickerCtrl;
class wxSpinCtrl;

class PanelEditors : public wxPanel {
public:
    PanelEditors(wxWindow*);
    ~PanelEditors();

    virtual bool TransferDataToWindow() override;
    virtual bool TransferDataFromWindow() override;

private:
    void OnPrefsChanged(wxCommandEvent&);

    void LoadSettings();
    void UpdateSettings();
    void SaveSettings();
    void SaveSettingsIfNecessary()
    {
        if (wxPreferencesEditor::ShouldApplyChangesImmediately())
            SaveSettings();
    }
#if 0
    // TODO: Per editor settings
    wxBitmapComboBox* m_bcbEditor;
#endif
    wxCheckBox* m_chkGuides;
    wxCheckBox* m_chkEOL;
    wxCheckBox* m_chkTabIndents;
    wxCheckBox* m_chkUseTabs;
    wxChoice* m_choWSpace;
    wxFontPickerCtrl* m_fontPicker;
    wxSpinCtrl* m_spnTabsWidth;
    wxSpinCtrl* m_spnIndent;
    wxSpinCtrl* m_spnCaretW;
#if 0
    int  m_selectedEditor;
#endif
    std::shared_ptr<wxw::Preferences> m_prefs;
};

class PageEditors : public wxPreferencesPage {
public:
    virtual wxString GetName() const override;
    virtual wxBitmap GetLargeIcon() const override; // TODO: Supported only on macOS
    virtual wxWindow* CreateWindow(wxWindow* parent) override;
};

class PanelLocale : public wxPanel {
public:
    PanelLocale(wxWindow*);
    ~PanelLocale();

    virtual bool TransferDataToWindow() override;
    virtual bool TransferDataFromWindow() override;

private:
    void OnPrefsChanged(wxCommandEvent&);

    void LoadSettings();
    void UpdateSettings();
    void SaveSettings();
    void SaveSettingsIfNecessary()
    {
        if (wxPreferencesEditor::ShouldApplyChangesImmediately())
            SaveSettings();
    }
    wxCheckBox* m_chkLocale;
    wxBitmapComboBox* m_bcbLocale;
    std::shared_ptr<wxw::Preferences> m_prefs;
};

class PageLocale : public wxPreferencesPage {
public:
    virtual wxString GetName() const override;
    virtual wxBitmap GetLargeIcon() const override; // TODO: Supported only on macOS
    virtual wxWindow* CreateWindow(wxWindow* parent) override;
};
