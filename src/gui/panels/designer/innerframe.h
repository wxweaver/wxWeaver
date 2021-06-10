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

#include "utils/debug.h"

#include <wx/aui/aui.h>
#include <wx/wx.h>

wxDECLARE_EVENT(wxEVT_WVR_INNER_FRAME_RESIZED, wxCommandEvent);

class InnerFrame : public wxPanel {
public:
    InnerFrame(wxWindow* parent, wxWindowID id,
               const wxPoint& pos = wxDefaultPosition,
               const wxSize& size = wxDefaultSize,
               long style = 0);

    wxPanel* GetFrameContentPanel() { return m_frameContent; }

    void OnMouseMotion(wxMouseEvent& e);
    void OnLeftDown(wxMouseEvent& e);
    void OnLeftUp(wxMouseEvent& e);

    void SetTitle(const wxString& title);
    wxString GetTitle();

    void SetTitleStyle(long style);

    void ShowTitleBar(bool show = true);
    void SetToBaseSize();
    bool IsTitleBarShown();

protected:
    wxSize DoGetBestSize() const override;

private:
    class TitleBar;
    TitleBar* m_titleBar;
    wxPanel* m_frameContent;

    enum {
        NONE,
        RIGHTBOTTOM,
        RIGHT,
        BOTTOM
    } m_sizing;

    wxSize m_minSize;
    wxSize m_baseMinSize;

    int m_curX, m_curY, m_difX, m_difY;
    int m_resizeBorder;
};
