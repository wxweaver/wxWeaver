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
#include "appdata.h"

#include <wx/html/forcelnk.h>
#include <wx/html/m_templ.h>

FORCE_LINK_ME(m_wxweaver)

TAG_HANDLER_BEGIN(wxWeaverVersion, "WXWEAVER-VERSION")
TAG_HANDLER_PROC(WXUNUSED(tag))
{
    auto* cell = new wxHtmlWordCell(VERSION, *m_WParser->GetDC());
    m_WParser->ApplyStateToCell(cell);
    m_WParser->GetContainer()->InsertCell(cell);

    return false;
}
TAG_HANDLER_END(wxWeaverVersion)

TAG_HANDLER_BEGIN(wxWeaverRevision, "WXWEAVER-REVISION")
TAG_HANDLER_PROC(WXUNUSED(tag))
{
    auto* cell = new wxHtmlWordCell(REVISION, *m_WParser->GetDC());
    m_WParser->ApplyStateToCell(cell);
    m_WParser->GetContainer()->InsertCell(cell);

    return false;
}
TAG_HANDLER_END(wxWeaverRevision)

TAGS_MODULE_BEGIN(wxWeaver)
TAGS_MODULE_ADD(wxWeaverVersion)
TAGS_MODULE_ADD(wxWeaverRevision)
TAGS_MODULE_END(wxWeaver)
