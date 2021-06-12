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

#include "utils/defs.h"
#include <wx/event.h>

struct PrefsEditor;

class wxWeaverEvent;
class wxWeaverObjectEvent;
class wxWeaverPropertyEvent;
class wxWeaverEventHandlerEvent;
class wxWeaverPrefsEditorEvent;

wxDECLARE_EVENT(wxEVT_WVR_PROJECT_LOADED, wxWeaverEvent);
wxDECLARE_EVENT(wxEVT_WVR_PROJECT_SAVED, wxWeaverEvent);
wxDECLARE_EVENT(wxEVT_WVR_PROJECT_REFRESH, wxWeaverEvent);
wxDECLARE_EVENT(wxEVT_WVR_CODE_GENERATION, wxWeaverEvent);
wxDECLARE_EVENT(wxEVT_WVR_OBJECT_EXPANDED, wxWeaverObjectEvent);
wxDECLARE_EVENT(wxEVT_WVR_OBJECT_SELECTED, wxWeaverObjectEvent);
wxDECLARE_EVENT(wxEVT_WVR_OBJECT_CREATED, wxWeaverObjectEvent);
wxDECLARE_EVENT(wxEVT_WVR_OBJECT_REMOVED, wxWeaverObjectEvent);
wxDECLARE_EVENT(wxEVT_WVR_PROPERTY_MODIFIED, wxWeaverPropertyEvent);
wxDECLARE_EVENT(wxEVT_WVR_EVENT_HANDLER_MODIFIED, wxWeaverEventHandlerEvent);
wxDECLARE_EVENT(wxEVT_WVR_PREFS_EDITOR_CHANGED, wxWeaverPrefsEditorEvent);

class wxWeaverEvent : public wxEvent {
public:
    wxWeaverEvent(wxEventType commandType = wxEVT_NULL);
    wxWeaverEvent(const wxWeaverEvent& event);
    ~wxWeaverEvent() override;

    wxString GetEventName() const;
    void SetString(const wxString& newString);
    wxString GetString() const;
    wxEvent* Clone() const override;

private:
    wxString m_string;
};

class wxWeaverPropertyEvent : public wxWeaverEvent {
public:
    wxWeaverPropertyEvent(wxEventType commandType, PProperty property);
    wxWeaverPropertyEvent(const wxWeaverPropertyEvent& event);

    wxEvent* Clone() const override;
    PProperty GetWvrProperty() { return m_property; }

private:
    PProperty m_property;
};

class wxWeaverEventHandlerEvent : public wxWeaverEvent {
public:
    wxWeaverEventHandlerEvent(wxEventType commandType, PEvent event);
    wxWeaverEventHandlerEvent(const wxWeaverEventHandlerEvent& event);

    wxEvent* Clone() const override;
    PEvent GetWvrEventHandler() { return m_event; }

private:
    PEvent m_event;
};

class wxWeaverObjectEvent : public wxWeaverEvent {
public:
    wxWeaverObjectEvent(wxEventType commandType, PObjectBase object);
    wxWeaverObjectEvent(const wxWeaverObjectEvent& event);

    wxEvent* Clone() const override;
    PObjectBase GetWvrObject() { return m_object; }

private:
    PObjectBase m_object;
};

class wxWeaverPrefsEditorEvent : public wxWeaverEvent {
public:
    wxWeaverPrefsEditorEvent(wxEventType eventType = wxEVT_WVR_PREFS_EDITOR_CHANGED);
    wxWeaverPrefsEditorEvent(const wxWeaverPrefsEditorEvent& event);

    wxEvent* Clone() const override;
    std::shared_ptr<PrefsEditor> GetPrefs() const { return m_prefsEditor; }
    void SetPrefs(std::shared_ptr<PrefsEditor> prefs) { m_prefsEditor = prefs; }

private:
    std::shared_ptr<PrefsEditor> m_prefsEditor;
};

#define wxWeaverEventHandler(func) (&func)
#define wxWeaverPropertyEventHandler(func) (&func)
#define wxWeaverObjectEventHandler(func) (&func)
#define wxWeaverEventHandlerEventHandler(func) (&func)
#define wxWeaverEditorPrefsEventHandler(func) (&func)

#if 0
typedef void (wxEvtHandler::*wxWeaverEventFunction)(wxWeaverEvent&);
typedef void (wxEvtHandler::*wxWeaverPropertyEventFunction)(wxWeaverPropertyEvent&);
typedef void (wxEvtHandler::*wxWeaverObjectEventFunction)(wxWeaverObjectEvent&);
typedef void (wxEvtHandler::*wxWeaverEventHandlerEventFunction)(wxWeaverEventHandlerEvent&);

#define wxWeaverEventHandler(func) wxEVENT_HANDLER_CAST(wxWeaverEventFunction, func)
#define wxWeaverPropertyEventHandler(func) wxEVENT_HANDLER_CAST(wxWeaverPropertyEventFunction, func)
#define wxWeaverObjectEventHandler(func) wxEVENT_HANDLER_CAST(wxWeaverObjectEventFunction, func)
#define wxWeaverEventHandlerEventHandler(func) wxEVENT_HANDLER_CAST(wxWeaverEventHandlerEventFunction, func)

#define wxWeaverEventHandler(fn) \
    (wxObjectEventFunction)(wxEventFunction) wxStaticCastEvent(wxWeaverEventFunction, &fn)

#define wxWeaverPropertyEventHandler(fn) \
    (wxObjectEventFunction)(wxEventFunction) wxStaticCastEvent(wxWeaverPropertyEventFunction, &fn)

#define wxWeaverObjectEventHandler(fn) \
    (wxObjectEventFunction)(wxEventFunction) wxStaticCastEvent(wxWeaverObjectEventFunction, &fn)

#define wxWeaverEventEventHandler(fn) \
    (wxObjectEventFunction)(wxEventFunction) wxStaticCastEvent(wxWeaverEventHandlerEventFunction, &fn)

#define EVT_WVR_PROJECT_LOADED(fn) \
    wx__DECLARE_EVT0(wxEVT_WVR_PROJECT_LOADED, wxWeaverEventHandler(fn))

#define EVT_WVR_PROJECT_SAVED(fn) \
    wx__DECLARE_EVT0(wxEVT_WVR_PROJECT_SAVED, wxWeaverEventHandler(fn))

#define EVT_WVR_OBJECT_EXPANDED(fn) \
    wx__DECLARE_EVT0(wxEVT_WVR_OBJECT_EXPANDED, wxWeaverObjectEventHandler(fn))

#define EVT_WVR_OBJECT_SELECTED(fn) \
    wx__DECLARE_EVT0(wxEVT_WVR_OBJECT_SELECTED, wxWeaverObjectEventHandler(fn))

#define EVT_WVR_OBJECT_CREATED(fn) \
    wx__DECLARE_EVT0(wxEVT_WVR_OBJECT_CREATED, wxWeaverObjectEventHandler(fn))

#define EVT_WVR_OBJECT_REMOVED(fn) \
    wx__DECLARE_EVT0(wxEVT_WVR_OBJECT_REMOVED, wxWeaverObjectEventHandler(fn))

#define EVT_WVR_PROPERTY_MODIFIED(fn) \
    wx__DECLARE_EVT0(wxEVT_WVR_PROPERTY_MODIFIED, wxWeaverPropertyEventHandler(fn))

#define EVT_WVR_EVENT_HANDLER_MODIFIED(fn) \
    wx__DECLARE_EVT0(wxEVT_WVR_EVENT_HANDLER_MODIFIED, wxWeaverEventEventHandler(fn))

#define EVT_WVR_PROJECT_REFRESH(fn) \
    wx__DECLARE_EVT0(wxEVT_WVR_PROJECT_REFRESH, wxWeaverEventHandler(fn))

#define EVT_WVR_CODE_GENERATION(fn) \
    wx__DECLARE_EVT0(wxEVT_WVR_CODE_GENERATION, wxWeaverEventHandler(fn))

BEGIN_DECLARE_EVENT_TYPES()
DECLARE_LOCAL_EVENT_TYPE(wxEVT_WVR_PROJECT_LOADED, -1)
DECLARE_LOCAL_EVENT_TYPE(wxEVT_WVR_PROJECT_SAVED, -1)
DECLARE_LOCAL_EVENT_TYPE(wxEVT_WVR_OBJECT_EXPANDED, -1)
DECLARE_LOCAL_EVENT_TYPE(wxEVT_WVR_OBJECT_SELECTED, -1)
DECLARE_LOCAL_EVENT_TYPE(wxEVT_WVR_OBJECT_CREATED, -1)
DECLARE_LOCAL_EVENT_TYPE(wxEVT_WVR_OBJECT_REMOVED, -1)
DECLARE_LOCAL_EVENT_TYPE(wxEVT_WVR_PROPERTY_MODIFIED, -1)
DECLARE_LOCAL_EVENT_TYPE(wxEVT_WVR_PROJECT_REFRESH, -1)
DECLARE_LOCAL_EVENT_TYPE(wxEVT_WVR_CODE_GENERATION, -1)
DECLARE_LOCAL_EVENT_TYPE(wxEVT_WVR_EVENT_HANDLER_MODIFIED, -1)
END_DECLARE_EVENT_TYPES()
#endif
