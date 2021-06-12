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
#include "event.h"

wxDEFINE_EVENT(wxEVT_WVR_PROJECT_LOADED, wxWeaverEvent);
wxDEFINE_EVENT(wxEVT_WVR_PROJECT_SAVED, wxWeaverEvent);
wxDEFINE_EVENT(wxEVT_WVR_PROJECT_REFRESH, wxWeaverEvent);
wxDEFINE_EVENT(wxEVT_WVR_CODE_GENERATION, wxWeaverEvent);
wxDEFINE_EVENT(wxEVT_WVR_OBJECT_EXPANDED, wxWeaverObjectEvent);
wxDEFINE_EVENT(wxEVT_WVR_OBJECT_SELECTED, wxWeaverObjectEvent);
wxDEFINE_EVENT(wxEVT_WVR_OBJECT_CREATED, wxWeaverObjectEvent);
wxDEFINE_EVENT(wxEVT_WVR_OBJECT_REMOVED, wxWeaverObjectEvent);
wxDEFINE_EVENT(wxEVT_WVR_PROPERTY_MODIFIED, wxWeaverPropertyEvent);
wxDEFINE_EVENT(wxEVT_WVR_EVENT_HANDLER_MODIFIED, wxWeaverEventHandlerEvent);
wxDEFINE_EVENT(wxEVT_WVR_PREFS_EDITOR_CHANGED, wxWeaverPrefsEditorEvent);

wxWeaverPrefsEditorEvent::wxWeaverPrefsEditorEvent(wxEventType eventType)
    : wxWeaverEvent(eventType)
{
}

wxWeaverPrefsEditorEvent::wxWeaverPrefsEditorEvent(const wxWeaverPrefsEditorEvent& event)
    : wxWeaverEvent(event)
    , m_prefsEditor(event.m_prefsEditor)
{
}

wxEvent* wxWeaverPrefsEditorEvent::Clone() const
{
    return new wxWeaverPrefsEditorEvent(*this);
}

wxWeaverEvent::wxWeaverEvent(wxEventType commandType)
    : wxEvent(0, commandType)
{
}

wxEvent* wxWeaverEvent::Clone() const
{
    return new wxWeaverEvent(*this);
}

wxWeaverEvent::wxWeaverEvent(const wxWeaverEvent& event)
    : wxEvent(event)
    , m_string(event.m_string)
{
}

wxWeaverEvent::~wxWeaverEvent()
{
}

wxString wxWeaverEvent::GetEventName() const
{
#define CASE(EVENT)           \
    if (EVENT == m_eventType) \
        return #EVENT;
    CASE(wxEVT_WVR_PROJECT_LOADED)
    CASE(wxEVT_WVR_PROJECT_SAVED)
    CASE(wxEVT_WVR_PROJECT_REFRESH)
    CASE(wxEVT_WVR_CODE_GENERATION)
    CASE(wxEVT_WVR_OBJECT_EXPANDED)
    CASE(wxEVT_WVR_OBJECT_SELECTED)
    CASE(wxEVT_WVR_OBJECT_CREATED)
    CASE(wxEVT_WVR_OBJECT_REMOVED)
    CASE(wxEVT_WVR_PROPERTY_MODIFIED)
    CASE(wxEVT_WVR_EVENT_HANDLER_MODIFIED)
    CASE(wxEVT_WVR_PREFS_EDITOR_CHANGED)
#undef CASE
    return "Unknown Type";
}

void wxWeaverEvent::SetString(const wxString& newString)
{
    m_string = newString;
}

wxString wxWeaverEvent::GetString() const
{
    return m_string;
}

wxWeaverPropertyEvent::wxWeaverPropertyEvent(wxEventType commandType,
                                             PProperty property)
    : wxWeaverEvent(commandType)
    , m_property(property)
{
}

wxWeaverPropertyEvent::wxWeaverPropertyEvent(const wxWeaverPropertyEvent& event)
    : wxWeaverEvent(event)
    , m_property(event.m_property)
{
}

wxEvent* wxWeaverPropertyEvent::Clone() const
{
    return new wxWeaverPropertyEvent(*this);
}

wxWeaverObjectEvent::wxWeaverObjectEvent(wxEventType commandType,
                                         PObjectBase object)
    : wxWeaverEvent(commandType)
    , m_object(object)
{
}

wxWeaverObjectEvent::wxWeaverObjectEvent(const wxWeaverObjectEvent& event)
    : wxWeaverEvent(event)
    , m_object(event.m_object)
{
}

wxEvent* wxWeaverObjectEvent::Clone() const
{
    return new wxWeaverObjectEvent(*this);
}

wxWeaverEventHandlerEvent::wxWeaverEventHandlerEvent(wxEventType commandType,
                                                     PEvent event)
    : wxWeaverEvent(commandType)
    , m_event(event)
{
}

wxWeaverEventHandlerEvent::wxWeaverEventHandlerEvent(const wxWeaverEventHandlerEvent& event)
    : wxWeaverEvent(event)
    , m_event(event.m_event)
{
}

wxEvent* wxWeaverEventHandlerEvent::Clone() const
{
    return new wxWeaverEventHandlerEvent(*this);
}
