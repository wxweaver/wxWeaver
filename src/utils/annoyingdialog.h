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
#pragma once

#include <wx/dialog.h>
#include <wx/artprov.h>

class wxCheckBox;

/** Dialog that contains a "Don't annoy me" checkbox.

    Using this dialog, the user can select not to display this dialog again.
    The dialog can be then re-enabled in the settings
*/
class AnnoyingDialog : public wxDialog {
public:
    enum dStyle {
        OK,
        YES_NO,
        YES_NO_CANCEL,
        OK_CANCEL,
        ONE_BUTTON,
        TWO_BUTTONS,
        THREE_BUTTONS
    };

    AnnoyingDialog(const wxString& caption,
                   const wxString& message,
                   const wxArtID icon = wxART_INFORMATION,
                   dStyle style = YES_NO,
                   int defaultReturn = wxID_YES,
                   bool separate = true,
                   const wxString& btn1text = wxEmptyString,
                   const wxString& btn2text = wxEmptyString,
                   const wxString& btn3text = wxEmptyString);

    int ShowModal() override;

private:
    void OnButton(wxCommandEvent& event);

    wxCheckBox* m_checkBox;
    bool m_dontAnnoy;
    int m_defRet;
};
