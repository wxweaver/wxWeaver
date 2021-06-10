/*
    wxWeaver - A GUI Designer Editor for wxWidgets.
    Copyright (C) 2005 José Antonio Hurtado
    Copyright (C) 2005 Juan Antonio Ortega (as wxFormBuilder)
    Copyright (C) 2012-2021 Andrea Zanellato <redtid3@gmail.com>

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
#include "barart.h"

#include <wx/aui/framemanager.h>
#include <wx/renderer.h>
#include <wx/settings.h>

void ToolBarArt::DrawBackground(wxDC& dc, wxWindow*, const wxRect& rect)
{
    dc.SetBrush(wxSystemSettings::GetColour(wxSYS_COLOUR_3DFACE));
    dc.SetPen(*wxTRANSPARENT_PEN);
    dc.DrawRectangle(rect);
}

void ToolBarArt::DrawOverflowButton(wxDC& dc, wxWindow* wnd,
                                    const wxRect& rect, int state)
{
    wxAuiToolBar* tb = wxDynamicCast(wnd, wxAuiToolBar);
    if (tb && !tb->GetToolBarFits()) {
        int flags = 0;
        if (state & wxAUI_BUTTON_STATE_HOVER)
            flags = wxCONTROL_CURRENT;
        if (state & wxAUI_BUTTON_STATE_PRESSED)
            flags = flags | wxCONTROL_PRESSED;
        if (flags)
            wxRendererNative::GetDefault().DrawPushButton(wnd, dc, rect, flags);
        wxRendererNative::GetDefault().DrawDropArrow(wnd, dc, rect, flags);
    }
}
