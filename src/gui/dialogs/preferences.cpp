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
#include "preferences.h"
#include "gui/bitmaps.h"
#include "appdata.h"
#include "event.h"
#include "settings.h"

#include <wx/bmpcbox.h>
#include <wx/checkbox.h>
#include <wx/choice.h>
#include <wx/config.h>
#include <wx/fontpicker.h>
#include <wx/gbsizer.h>
#include <wx/sizer.h>
#include <wx/spinctrl.h>
#include <wx/statbox.h>
#include <wx/stattext.h>

PanelEditors::PanelEditors(wxWindow* parent)
    : wxPanel(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL)
    , m_prefs(new wxw::Preferences)
{
    wxStaticBoxSizer* sbsFont = new wxStaticBoxSizer(
        new wxStaticBox(this, wxID_ANY, _("Font")), wxHORIZONTAL);

    m_fontPicker = new wxFontPickerCtrl(sbsFont->GetStaticBox(), wxID_ANY);
    m_fontPicker->SetMaxPointSize(100);

    sbsFont->Add(m_fontPicker, 1, wxEXPAND | wxBOTTOM | wxLEFT | wxRIGHT, 5);

    // https://github.com/mirror/scintilla/blob/master/include/Scintilla.iface#L158
    wxArrayString viewWhitespace;
    viewWhitespace.Add(_("Invisible"));      // 0 SCWS_INVISIBLE
    viewWhitespace.Add(_("Always"));         // 1 SCWS_VISIBLEALWAYS
    viewWhitespace.Add(_("After Indent"));   // 2 SCWS_VISIBLEAFTERINDENT
    viewWhitespace.Add(_("Only In Indent")); // 3 SCWS_VISIBLEONLYININDENT

    wxStaticBoxSizer* sbsDisplay = new wxStaticBoxSizer(
        new wxStaticBox(this, wxID_ANY, _("Display")), wxHORIZONTAL);

    wxStaticText* lblWSpace = new wxStaticText(
        sbsDisplay->GetStaticBox(), wxID_ANY, _("View Whitespace:"));

    m_choWSpace = new wxChoice(
        sbsDisplay->GetStaticBox(), wxID_ANY,
        wxDefaultPosition, wxDefaultSize, viewWhitespace);

    m_chkEOL = new wxCheckBox(sbsDisplay->GetStaticBox(), wxID_ANY, _("View EOL"));

    wxFlexGridSizer* fgsDisplay = new wxFlexGridSizer(2, 0, 0);
    fgsDisplay->SetFlexibleDirection(wxBOTH);
    fgsDisplay->SetNonFlexibleGrowMode(wxFLEX_GROWMODE_ALL);
    fgsDisplay->Add(lblWSpace, 1, wxLEFT | wxALIGN_CENTER_VERTICAL, 5);
    fgsDisplay->Add(m_choWSpace, 1, wxEXPAND | wxLEFT | wxRIGHT, 5);
    fgsDisplay->Add(m_chkEOL, 1, wxBOTTOM, 5);
    fgsDisplay->AddGrowableCol(1);
    sbsDisplay->Add(fgsDisplay, 1, 0);

    wxStaticBoxSizer* sbsIndent = new wxStaticBoxSizer(
        new wxStaticBox(this, wxID_ANY, _("Indentation")), wxHORIZONTAL);

    m_chkUseTabs = new wxCheckBox(
        sbsIndent->GetStaticBox(), wxID_ANY, _("Use Tabs"));

    m_chkTabIndents = new wxCheckBox(
        sbsIndent->GetStaticBox(), wxID_ANY, _("Tabs Indents"));

    m_chkGuides = new wxCheckBox(
        sbsIndent->GetStaticBox(), wxID_ANY, _("Show Indentation Guides"));

    m_spnIndent = new wxSpinCtrl(
        sbsIndent->GetStaticBox(), wxID_ANY, wxEmptyString,
        wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 1, 10, 4);

    m_spnTabsWidth = new wxSpinCtrl(
        sbsIndent->GetStaticBox(), wxID_ANY, wxEmptyString,
        wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 1, 10, 4);

    wxStaticText* lblIndent = new wxStaticText(
        sbsIndent->GetStaticBox(), wxID_ANY, _("Indent Size:"));

    wxStaticText* lblTabsWidth = new wxStaticText(
        sbsIndent->GetStaticBox(), wxID_ANY, _("Tabs Width:"));

    lblIndent->Wrap(-1);
    lblTabsWidth->Wrap(-1);

    wxGridBagSizer* gbsIndent = new wxGridBagSizer;
    gbsIndent->SetFlexibleDirection(wxBOTH);
    gbsIndent->SetNonFlexibleGrowMode(wxFLEX_GROWMODE_ALL);
    gbsIndent->Add(m_chkUseTabs, wxGBPosition(0, 0), wxDefaultSpan, 0, 5);
    gbsIndent->Add(m_chkTabIndents, wxGBPosition(0, 1), wxDefaultSpan, 0, 5);
    gbsIndent->Add(m_chkGuides, wxGBPosition(1, 1), wxDefaultSpan, wxBOTTOM, 5);
    gbsIndent->Add(lblIndent, wxGBPosition(2, 0), wxDefaultSpan, wxBOTTOM | wxLEFT | wxRIGHT | wxALIGN_CENTER_VERTICAL, 5);
    gbsIndent->Add(m_spnIndent, wxGBPosition(2, 1), wxDefaultSpan, wxBOTTOM | wxLEFT | wxRIGHT | wxEXPAND, 5);
    gbsIndent->Add(lblTabsWidth, wxGBPosition(3, 0), wxDefaultSpan, wxBOTTOM | wxLEFT | wxRIGHT | wxALIGN_CENTER_VERTICAL, 5);
    gbsIndent->Add(m_spnTabsWidth, wxGBPosition(3, 1), wxDefaultSpan, wxBOTTOM | wxLEFT | wxRIGHT | wxEXPAND, 5);
    gbsIndent->AddGrowableCol(1);
    sbsIndent->Add(gbsIndent, 1, 0);

    wxStaticBoxSizer* sbsMisc = new wxStaticBoxSizer(
        new wxStaticBox(this, wxID_ANY, _("Misc")), wxHORIZONTAL);

    wxStaticText* lblCaretW = new wxStaticText(
        sbsMisc->GetStaticBox(), wxID_ANY, _("Caret Width:"));

    lblCaretW->Wrap(-1);

    m_spnCaretW = new wxSpinCtrl(
        sbsMisc->GetStaticBox(), wxID_ANY, wxEmptyString,
        wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 1, 10, 4);

    wxFlexGridSizer* fgsMisc = new wxFlexGridSizer(2, 0, 0);
    fgsMisc->SetFlexibleDirection(wxBOTH);
    fgsMisc->SetNonFlexibleGrowMode(wxFLEX_GROWMODE_ALL);
    fgsMisc->Add(lblCaretW, 1, wxBOTTOM | wxLEFT | wxRIGHT | wxALIGN_CENTER_VERTICAL, 5);
    fgsMisc->Add(m_spnCaretW, 1, wxBOTTOM | wxRIGHT | wxEXPAND, 5);
    fgsMisc->AddGrowableCol(1);
    sbsMisc->Add(fgsMisc, 1, 0);

    wxFlexGridSizer* sizer = new wxFlexGridSizer(1, 0, 0);
    fgsMisc->SetFlexibleDirection(wxBOTH);
    fgsMisc->SetNonFlexibleGrowMode(wxFLEX_GROWMODE_ALL);
    sizer->Add(sbsFont, 1, wxEXPAND | wxLEFT | wxRIGHT | wxTOP, 5);
    sizer->Add(sbsDisplay, 1, wxEXPAND | wxLEFT | wxRIGHT, 5);
    sizer->Add(sbsIndent, 1, wxEXPAND | wxLEFT | wxRIGHT, 5);
    sizer->Add(sbsMisc, 1, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 5);
    sizer->AddGrowableCol(0);
    SetSizer(sizer);
    Layout();

    LoadSettings();

    m_chkGuides->Bind(wxEVT_CHECKBOX, &PanelEditors::OnPrefsChanged, this);
    m_chkEOL->Bind(wxEVT_CHECKBOX, &PanelEditors::OnPrefsChanged, this);
    m_chkTabIndents->Bind(wxEVT_CHECKBOX, &PanelEditors::OnPrefsChanged, this);
    m_chkUseTabs->Bind(wxEVT_CHECKBOX, &PanelEditors::OnPrefsChanged, this);
    m_choWSpace->Bind(wxEVT_CHOICE, &PanelEditors::OnPrefsChanged, this);
    m_fontPicker->Bind(wxEVT_FONTPICKER_CHANGED, &PanelEditors::OnPrefsChanged, this);
    m_spnTabsWidth->Bind(wxEVT_SPINCTRL, &PanelEditors::OnPrefsChanged, this);
    m_spnIndent->Bind(wxEVT_SPINCTRL, &PanelEditors::OnPrefsChanged, this);
    m_spnCaretW->Bind(wxEVT_SPINCTRL, &PanelEditors::OnPrefsChanged, this);

    AppData()->AddHandler(this->GetEventHandler());
}

PanelEditors::~PanelEditors()
{
    m_chkGuides->Unbind(wxEVT_CHECKBOX, &PanelEditors::OnPrefsChanged, this);
    m_chkEOL->Unbind(wxEVT_CHECKBOX, &PanelEditors::OnPrefsChanged, this);
    m_chkTabIndents->Unbind(wxEVT_CHECKBOX, &PanelEditors::OnPrefsChanged, this);
    m_chkUseTabs->Unbind(wxEVT_CHECKBOX, &PanelEditors::OnPrefsChanged, this);
    m_choWSpace->Unbind(wxEVT_CHOICE, &PanelEditors::OnPrefsChanged, this);
    m_fontPicker->Unbind(wxEVT_FONTPICKER_CHANGED, &PanelEditors::OnPrefsChanged, this);
    m_spnTabsWidth->Unbind(wxEVT_SPINCTRL, &PanelEditors::OnPrefsChanged, this);
    m_spnIndent->Unbind(wxEVT_SPINCTRL, &PanelEditors::OnPrefsChanged, this);
    m_spnCaretW->Unbind(wxEVT_SPINCTRL, &PanelEditors::OnPrefsChanged, this);

    AppData()->RemoveHandler(this->GetEventHandler());
}

bool PanelEditors::TransferDataToWindow()
{
    LoadSettings();
    return true;
}

bool PanelEditors::TransferDataFromWindow()
{
    SaveSettings();
    return true;
}

void PanelEditors::OnPrefsChanged(wxCommandEvent&)
{
    UpdateSettings();
    SaveSettingsIfNecessary();
    wxWeaverPreferencesEvent* event = new wxWeaverPreferencesEvent;
    event->SetPrefs(m_prefs);
    AppData()->NotifyPreferencesChanged(*event);
}

void PanelEditors::LoadSettings()
{
    wxFont font;
    font.SetFamily(wxFONTFAMILY_MODERN);
    font.SetPointSize(m_prefs->fontSize);
    font.SetFaceName(m_prefs->fontFace);

    m_chkGuides->SetValue(m_prefs->showIndentationGuides);
    m_chkEOL->SetValue(m_prefs->showEOL);
    m_chkTabIndents->SetValue(m_prefs->tabIndents);
    m_chkUseTabs->SetValue(m_prefs->useTabs);
    m_choWSpace->SetSelection(m_prefs->showWhiteSpace);
    m_fontPicker->SetSelectedFont(font);
    m_spnTabsWidth->SetValue(m_prefs->tabsWidth);
    m_spnIndent->SetValue(m_prefs->indentSize);
    m_spnCaretW->SetValue(m_prefs->caretWidth);
}

void PanelEditors::SaveSettings()
{
    wxFont font = m_fontPicker->GetSelectedFont();

    wxConfigBase* config = wxConfigBase::Get();
    config->Write("/Editors/ShowIndentationGuides", m_chkGuides->IsChecked());
    config->Write("/Editors/ShowWhitespace", m_choWSpace->GetSelection());
    config->Write("/Editors/ShowEOL", m_chkEOL->IsChecked());
    config->Write("/Editors/TabIndents", m_chkTabIndents->IsChecked());
    config->Write("/Editors/UseTabs", m_chkUseTabs->IsChecked());
    config->Write("/Editors/TabsWidth", m_spnTabsWidth->GetValue());
    config->Write("/Editors/IndentSize", m_spnIndent->GetValue());
    config->Write("/Editors/CaretWidth", m_spnCaretW->GetValue());
    config->Write("/Editors/FontFace", font.GetFaceName());
    config->Write("/Editors/FontSize", font.GetPointSize());
    config->Flush();
}

void PanelEditors::UpdateSettings()
{
    wxFont font = m_fontPicker->GetSelectedFont();
#if 0
    m_selectedEditor = m_bcbEditor->GetSelection();
#endif
    m_prefs->showIndentationGuides = m_chkGuides->IsChecked();
    m_prefs->showWhiteSpace = m_choWSpace->GetSelection();
    m_prefs->showEOL = m_chkEOL->IsChecked();
    m_prefs->tabIndents = m_chkTabIndents->IsChecked();
    m_prefs->useTabs = m_chkUseTabs->IsChecked();
    m_prefs->tabsWidth = m_spnTabsWidth->GetValue();
    m_prefs->indentSize = m_spnIndent->GetValue();
    m_prefs->caretWidth = m_spnCaretW->GetValue();
    m_prefs->fontFace = font.GetFaceName();
    m_prefs->fontSize = font.GetPointSize();
}

PanelLocale::PanelLocale(wxWindow* parent)
    : wxPanel(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL)
    , m_prefs(new wxw::Preferences)
{
    wxStaticBoxSizer* sbsLocale = new wxStaticBoxSizer(
        new wxStaticBox(this, wxID_ANY, _("Locale")), wxVERTICAL);

    m_chkLocale = new wxCheckBox(
        sbsLocale->GetStaticBox(), wxID_ANY, _("Enable Localization"));

    m_bcbLocale = new wxBitmapComboBox(
        sbsLocale->GetStaticBox(), wxID_ANY, wxEmptyString,
        wxDefaultPosition, wxDefaultSize, 0, nullptr, wxCB_READONLY);

    m_bcbLocale->Append(_("System Default"), AppBitmaps::GetBitmap("locale/default"));
    m_bcbLocale->Append(_("English"), AppBitmaps::GetBitmap("locale/gb"));
#if 0
    m_bcbLocale->Append(_("English (U.S.)"), AppBitmaps::GetBitmap("locale/us"));
    m_bcbLocale->Append(_("German"), AppBitmaps::GetBitmap("locale/de"));
#endif
    m_bcbLocale->Append(_("Italian"), AppBitmaps::GetBitmap("locale/it"));
    m_bcbLocale->SetSelection(0);

    sbsLocale->Add(m_chkLocale, 0, wxLEFT | wxRIGHT, 5);
    sbsLocale->Add(m_bcbLocale, 0, wxEXPAND | wxBOTTOM | wxLEFT | wxRIGHT, 5);

    wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
    sizer->Add(sbsLocale, 1, wxEXPAND | wxALL, 5);
    this->SetSizer(sizer);
    this->Layout();

    LoadSettings();

    m_chkLocale->Bind(wxEVT_CHECKBOX, &PanelLocale::OnPrefsChanged, this);
    m_bcbLocale->Bind(wxEVT_COMBOBOX, &PanelLocale::OnPrefsChanged, this);

    AppData()->AddHandler(this->GetEventHandler());
}

PanelLocale::~PanelLocale()
{
    m_chkLocale->Unbind(wxEVT_CHECKBOX, &PanelLocale::OnPrefsChanged, this);
    m_bcbLocale->Unbind(wxEVT_COMBOBOX, &PanelLocale::OnPrefsChanged, this);

    AppData()->RemoveHandler(this->GetEventHandler());
}

bool PanelLocale::TransferDataToWindow()
{
    LoadSettings();
    return true;
}

bool PanelLocale::TransferDataFromWindow()
{
    SaveSettings();
    return true;
}

void PanelLocale::OnPrefsChanged(wxCommandEvent&)
{
    UpdateSettings();
    SaveSettingsIfNecessary();
    wxWeaverPreferencesEvent* event = new wxWeaverPreferencesEvent;
    event->SetPrefs(m_prefs);
    AppData()->NotifyPreferencesChanged(*event);
}

void PanelLocale::LoadSettings()
{
    m_chkLocale->SetValue(m_prefs->localeEnabled);
    m_bcbLocale->SetSelection(m_prefs->localeSelected);
}

void PanelLocale::SaveSettings()
{
    wxConfigBase* config = wxConfigBase::Get();
    config->Write("/Locale/Enabled", m_chkLocale->GetValue());
    config->Write("/Locale/Language", m_bcbLocale->GetSelection());
    config->Flush();
}

void PanelLocale::UpdateSettings()
{
    m_prefs->localeEnabled = m_chkLocale->GetValue();
    m_prefs->localeSelected = m_bcbLocale->GetSelection();
}

wxString PageEditors::GetName() const
{
    return _("Editor");
}

wxBitmap PageEditors::GetLargeIcon() const
{
    return AppBitmaps::GetBitmap("accessories-text-editor");
}

wxWindow* PageEditors::CreateWindow(wxWindow* parent)
{
    return new PanelEditors(parent);
}

wxString PageLocale::GetName() const
{
    return _("Locale");
}

wxBitmap PageLocale::GetLargeIcon() const
{
    return AppBitmaps::GetBitmap("preferences-desktop-locale");
}

wxWindow* PageLocale::CreateWindow(wxWindow* parent)
{
    return new PanelLocale(parent);
}
