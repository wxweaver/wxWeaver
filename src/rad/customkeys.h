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

#ifndef __KEYS_HANDLER__
#define __KEYS_HANDLER__

#include <wx/wx.h>

// This class is not needed because shorcuts are made through acceletors
// of the MainFrame menubar.
class CustomKeysEvtHandler : public wxEvtHandler
{
 private:
  CustomKeysEvtHandler() {}

  DECLARE_EVENT_TABLE()
 public:
  void OnKeyPress(wxKeyEvent &event);
};

#endif //__KEYS_HANDLER__
