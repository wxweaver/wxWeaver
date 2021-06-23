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
#include "gui/panels/designer/visualobj.h"

#include "gui/panels/designer/visualeditor.h"
#include "rtti/objectbase.h"
#include "utils/typeconv.h"
#include "appdata.h"

using namespace TypeConv;

VObjEvtHandler::VObjEvtHandler(wxWindow* win, PObjectBase obj)
    : m_object(obj)
    , m_window(win)
{
    Bind(wxEVT_LEFT_DOWN, &VObjEvtHandler::OnLeftClick, this);
    Bind(wxEVT_RIGHT_DOWN, &VObjEvtHandler::OnRightClick, this);
    Bind(wxEVT_PAINT, &VObjEvtHandler::OnPaint, this);
    Bind(wxEVT_SET_CURSOR, &VObjEvtHandler::OnSetCursor, this);
}

void VObjEvtHandler::OnLeftClick(wxMouseEvent& event)
{
    PObjectBase obj = m_object.lock();

    if (obj) {
        if (obj->GetTypeName() == "ribbonbar") {
            if (AppData()->GetSelectedObject() != obj)
                AppData()->SelectObject(obj);

            event.Skip();
            return;
        }
        if (AppData()->GetSelectedObject() != obj) {
            AppData()->SelectObject(obj);
            /*
                this event is always redirected to parent (aui)toolbar
                even if its tool was clicked so it is important to skip the event
                to select clicked tool later in "common" plugin.
            */
            if (obj->GetTypeName() == "toolbar"
                || obj->GetTypeName() == "toolbar_form")
                event.Skip();
        } else {
            /*
                Event should be skipped only in the case of the object selected
                is the same that the object clicked. You will experiment rare things
                in other case.
            */
            event.Skip();
        }
    }
    m_window->ClientToScreen(&event.m_x, &event.m_y);
    m_window->GetParent()->ScreenToClient(&event.m_x, &event.m_y);
    ::wxPostEvent(m_window->GetParent(), event);
}

void VObjEvtHandler::OnRightClick(wxMouseEvent& event)
{
    // show context menu associated with the widget if any
    PObjectBase obj = m_object.lock();
    if (obj) {
        if (obj->GetPropertyAsInteger("context_menu")) {
            PObjectBase menu;
            for (size_t i = 0; i < obj->GetChildCount(); i++) {
                if (obj->GetChild(i)->GetTypeName() == "menu") {
                    menu = obj->GetChild(i);
                    break;
                }
            }
            if (menu)
                m_window->PopupMenu(
                    DesignerWindow::GetMenuFromObject(menu), event.GetPosition());
        } else
            event.Skip();
    }
    m_window->ClientToScreen(&event.m_x, &event.m_y);
    m_window->GetParent()->ScreenToClient(&event.m_x, &event.m_y);
    ::wxPostEvent(m_window->GetParent(), event);
}

void VObjEvtHandler::OnPaint(wxPaintEvent& event)
{
#if 0
    PObjectBase obj = std::dynamic_pointer_cast<ObjectBase>(m_object.lock());
    if (obj->IsContainer()) { // TODO: why only containers?
#endif
    wxWindow* aux = m_window;
    while (!aux->IsKindOf(wxCLASSINFO(DesignerWindow)))
        aux = aux->GetParent();
    DesignerWindow* dsgnWin = (DesignerWindow*)aux;
    if (dsgnWin->GetActivePanel() == m_window) {
        wxPaintDC dc(m_window);
        dsgnWin->HighlightSelection(dc);
    }
#if 0
    }
#endif
    event.Skip();
}

void VObjEvtHandler::OnSetCursor(wxSetCursorEvent& event)
{
    wxCoord x = event.GetX(), y = event.GetY();
    m_window->ClientToScreen(&x, &y);
    m_window->GetParent()->ScreenToClient(&x, &y);
    wxSetCursorEvent sce(x, y);
    ::wxPostEvent(m_window->GetParent(), sce);
}
