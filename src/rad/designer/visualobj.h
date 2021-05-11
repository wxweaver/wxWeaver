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

#ifndef __VISUAL_OBJS__
#define __VISUAL_OBJS__

#include "utils/defs.h"

#include <wx/wx.h>

/**
 * Processes events from visual objects.
 */
class VObjEvtHandler : public wxEvtHandler
{
 private:
   WPObjectBase m_object;
   wxWindow *m_window;

   VObjEvtHandler() = default;

 protected:
  DECLARE_EVENT_TABLE()

 public:
   VObjEvtHandler(wxWindow *win, PObjectBase obj);
   void OnLeftClick(wxMouseEvent &event);
   void OnRightClick(wxMouseEvent &event);
   void OnPaint(wxPaintEvent &event);
   void OnSetCursor(wxSetCursorEvent &event);
};


///////////////////////////////////////////////////////////////////////////////


#endif //__VISUAL_OBJS__


