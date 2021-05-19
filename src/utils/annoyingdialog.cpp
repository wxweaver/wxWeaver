/*
    wxWeaver - A GUI Designer Editor for wxWidgets.
    Copyright (C) 2003 Yiannis An. Mandravellos
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
#include "utils/annoyingdialog.h"

#include <wx/button.h>
#include <wx/checkbox.h>
#include <wx/config.h>
#include <wx/log.h>
#include <wx/sizer.h>
#include <wx/statbmp.h>
#include <wx/stattext.h>

#include <memory>

AnnoyingDialog::AnnoyingDialog(const wxString& caption, const wxString& message,
                               const wxArtID icon, dStyle style, int defaultReturn,
                               bool /*separate*/,
                               const wxString& btn1text,
                               const wxString& btn2text,
                               const wxString& btn3text)

    : wxDialog(nullptr, wxID_ANY, caption, wxDefaultPosition, wxDefaultSize, wxCAPTION)
    , m_checkBox(nullptr)
    , m_dontAnnoy(false)
    , m_defRet(defaultReturn)
{
    wxConfigBase* config = wxConfigBase::Get();
    int defRet;
    if (config->Read(wxT("annoyingdialog/") + caption, &defRet)) {
        if (defRet != wxID_CANCEL) {
            m_dontAnnoy = true;
            m_defRet = defRet;
            return;
        }
    }
    auto outerSizer = std::make_unique<wxBoxSizer>(wxVERTICAL);

    wxFlexGridSizer* mainArea = new wxFlexGridSizer(2, 0, 0);

    wxStaticBitmap* bitmap = new wxStaticBitmap(
        this, wxID_ANY, wxArtProvider::GetBitmap(icon, wxART_MESSAGE_BOX), wxDefaultPosition);

    wxStaticText* txt = new wxStaticText(
        this, wxID_ANY, message, wxDefaultPosition, wxDefaultSize, 0);

    mainArea->Add(bitmap, 0, wxALL, 5);
    mainArea->Add(txt, 0, wxALIGN_CENTER | wxALL, 5);
    mainArea->Add(1, 1, 0, wxGROW | wxALIGN_CENTER_VERTICAL | wxLEFT | wxRIGHT | wxTOP, 5);

    int numButtons = 0;
    int id1 = wxID_ANY;
    int id2 = wxID_ANY;
    int id3 = wxID_ANY;
    wxString bTxt1;
    wxString bTxt2;
    wxString bTxt3;

    if (style == OK || style == ONE_BUTTON) {
        numButtons = 1;
        id1 = (style == OK ? wxID_OK : 1);
        bTxt1 = btn1text.IsEmpty() ? _("&OK") : btn1text;
    } else if (style == YES_NO || style == OK_CANCEL || style == TWO_BUTTONS) {
        numButtons = 2;
        id1 = (style == YES_NO ? wxID_YES : (style == OK_CANCEL ? wxID_OK : 1));
        id2 = (style == YES_NO ? wxID_NO : (style == OK_CANCEL ? wxID_CANCEL : 2));
        bTxt1 = btn1text.IsEmpty() ? (style == YES_NO ? _("&Yes") : _("&OK")) : btn1text;
        bTxt2 = btn2text.IsEmpty() ? (style == YES_NO ? _("&No") : _("&Cancel")) : btn2text;
    } else if (style == YES_NO_CANCEL || style == THREE_BUTTONS) {
        numButtons = 3;
        id1 = (style == YES_NO_CANCEL ? wxID_YES : 1);
        id2 = (style == YES_NO_CANCEL ? wxID_NO : 2);
        id3 = (style == YES_NO_CANCEL ? wxID_CANCEL : 3);
        bTxt1 = btn1text.IsEmpty() ? _("&Yes") : btn1text;
        bTxt2 = btn2text.IsEmpty() ? _("&No") : btn2text;
        bTxt3 = btn3text.IsEmpty() ? _("&Cancel") : btn3text;
    } else {
        wxLogError(
            "Fatal error:\nUndefined style in dialog %s", caption.c_str());
        return;
    }
    wxSizer* buttonSizer = nullptr;
    if (style < ONE_BUTTON) // standard buttons? use wxStdDialogButtonSizer
    {
        wxStdDialogButtonSizer* buttonArea = new wxStdDialogButtonSizer();

        wxButton* but1 = new wxButton(this, id1, bTxt1, wxDefaultPosition, wxDefaultSize, 0);
        but1->SetDefault();
        buttonArea->AddButton(but1);

        if (numButtons > 1) {
            wxButton* but2 = new wxButton(this, id2, bTxt2, wxDefaultPosition, wxDefaultSize, 0);
            but2->SetDefault();
            buttonArea->AddButton(but2);
        }
        if (numButtons > 2) {
            wxButton* but3 = new wxButton(this, id3, bTxt3, wxDefaultPosition, wxDefaultSize, 0);
            but3->SetDefault();
            buttonArea->AddButton(but3);
        }
        buttonArea->Realize();
        buttonSizer = buttonArea;
    } else {
        // wxStdDialogButtonSizer accepts only standard IDs for its buttons,
        // so we can't use it with custom buttons
        buttonSizer = new wxBoxSizer(wxHORIZONTAL);

        wxButton* but1 = new wxButton(this, id1, bTxt1, wxDefaultPosition, wxDefaultSize, 0);
        but1->SetDefault();
        buttonSizer->Add(but1, 0, wxRIGHT, 5);

        if (numButtons > 1) {
            wxButton* but2 = new wxButton(this, id2, bTxt2, wxDefaultPosition, wxDefaultSize, 0);
            but2->SetDefault();
            buttonSizer->Add(but2, 0, wxRIGHT, 5);
        }
        if (numButtons > 2) {
            wxButton* but3 = new wxButton(this, id3, bTxt3, wxDefaultPosition, wxDefaultSize, 0);
            but3->SetDefault();
            buttonSizer->Add(but3, 0, wxRIGHT, 5);
        }
    }
    outerSizer->Add(mainArea, 0, wxALIGN_CENTER | wxALL, 5);
    outerSizer->Add(buttonSizer, 0, wxALIGN_CENTER);

    m_checkBox = new wxCheckBox(
        this, wxID_ANY, _("Don't annoy me again!"),
        wxDefaultPosition, wxDefaultSize, 0);

    outerSizer->Add(m_checkBox, 0, wxALIGN_LEFT | wxLEFT | wxRIGHT | wxBOTTOM, 5);

    SetSizer(outerSizer.release());
    GetSizer()->SetSizeHints(this);

    Centre();

    Bind(wxEVT_COMMAND_BUTTON_CLICKED, &AnnoyingDialog::OnButton, this);
}

void AnnoyingDialog::OnButton(wxCommandEvent& event)
{
    if (!m_checkBox) {
        wxLogError("Ow... null pointer.");
        return;
    }
    if (event.GetId() != wxID_CANCEL) {
        wxConfigBase* config = wxConfigBase::Get();
        if (m_checkBox->IsChecked())
            config->Write(wxT("annoyingdialog/") + GetTitle(), event.GetId());
    }
    EndModal(event.GetId());
}

int AnnoyingDialog::ShowModal()
{
    if (m_dontAnnoy)
        return m_defRet;

    return wxDialog::ShowModal();
}
