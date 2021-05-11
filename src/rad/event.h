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

#ifndef __wxWEAVEREVENT__
#define __wxWEAVEREVENT__

#include "utils/defs.h"

#include <wx/event.h>

class wxWeaverEvent : public wxEvent
{
	private:
		wxString m_string;

	public:
		wxWeaverEvent( wxEventType commandType = wxEVT_NULL );
		wxWeaverEvent( const wxWeaverEvent& event );
	~wxWeaverEvent() override;

		wxString GetEventName();

		void SetString( const wxString& newString );
		wxString GetString();

		// required for sending with wxPostEvent()
	wxEvent* Clone() const override;
};

class wxWeaverPropertyEvent : public wxWeaverEvent
{
public:
  wxWeaverPropertyEvent(wxEventType commandType, PProperty property);
  wxWeaverPropertyEvent( const wxWeaverPropertyEvent& event );
	wxEvent* Clone() const override;
  PProperty GetFBProperty() { return m_property; }
private:
  PProperty m_property;
};

class wxWeaverEventHandlerEvent : public wxWeaverEvent
{
public:
  wxWeaverEventHandlerEvent (wxEventType commandType, PEvent event);
  wxWeaverEventHandlerEvent( const wxWeaverEventHandlerEvent& event );
	wxEvent* Clone() const override;
  PEvent GetFBEventHandler() { return m_event; }
private:
  PEvent m_event;
};

class wxWeaverObjectEvent : public wxWeaverEvent
{
public:
  wxWeaverObjectEvent(wxEventType commandType, PObjectBase object);
  wxWeaverObjectEvent( const wxWeaverObjectEvent& event );
	wxEvent* Clone() const override;
  PObjectBase GetFBObject() { return m_object; }

private:
  PObjectBase m_object;
};


typedef void (wxEvtHandler::*wxWeaverEventFunction)        (wxWeaverEvent&);
typedef void (wxEvtHandler::*wxWeaverPropertyEventFunction)(wxWeaverPropertyEvent&);
typedef void (wxEvtHandler::*wxWeaverObjectEventFunction)  (wxWeaverObjectEvent&);
typedef void (wxEvtHandler::*wxWeaverEventHandlerEventFunction)  (wxWeaverEventHandlerEvent&);

#define wxWeaverEventHandler(fn) \
  (wxObjectEventFunction)(wxEventFunction)wxStaticCastEvent(wxWeaverEventFunction, &fn)

#define wxWeaverPropertyEventHandler(fn) \
  (wxObjectEventFunction)(wxEventFunction)wxStaticCastEvent(wxWeaverPropertyEventFunction, &fn)

#define wxWeaverObjectEventHandler(fn) \
  (wxObjectEventFunction)(wxEventFunction)wxStaticCastEvent(wxWeaverObjectEventFunction, &fn)

#define wxWeaverEventEventHandler(fn) \
  (wxObjectEventFunction)(wxEventFunction)wxStaticCastEvent(wxWeaverEventHandlerEventFunction, &fn)


BEGIN_DECLARE_EVENT_TYPES()
  DECLARE_LOCAL_EVENT_TYPE( wxEVT_FB_PROJECT_LOADED,    -1 )
  DECLARE_LOCAL_EVENT_TYPE( wxEVT_FB_PROJECT_SAVED,     -1 )
  DECLARE_LOCAL_EVENT_TYPE( wxEVT_FB_OBJECT_EXPANDED,   -1 )
  DECLARE_LOCAL_EVENT_TYPE( wxEVT_FB_OBJECT_SELECTED,   -1 )
  DECLARE_LOCAL_EVENT_TYPE( wxEVT_FB_OBJECT_CREATED,    -1 )
  DECLARE_LOCAL_EVENT_TYPE( wxEVT_FB_OBJECT_REMOVED,    -1 )
  DECLARE_LOCAL_EVENT_TYPE( wxEVT_FB_PROPERTY_MODIFIED, -1 )
  DECLARE_LOCAL_EVENT_TYPE( wxEVT_FB_PROJECT_REFRESH,   -1 )
  DECLARE_LOCAL_EVENT_TYPE( wxEVT_FB_CODE_GENERATION,   -1 )
  DECLARE_LOCAL_EVENT_TYPE( wxEVT_FB_EVENT_HANDLER_MODIFIED, -1 )
END_DECLARE_EVENT_TYPES()

#define EVT_FB_PROJECT_LOADED(fn) \
  wx__DECLARE_EVT0(wxEVT_FB_PROJECT_LOADED,wxWeaverEventHandler(fn))

#define EVT_FB_PROJECT_SAVED(fn) \
  wx__DECLARE_EVT0(wxEVT_FB_PROJECT_SAVED,wxWeaverEventHandler(fn))

#define EVT_FB_OBJECT_EXPANDED(fn) \
  wx__DECLARE_EVT0(wxEVT_FB_OBJECT_EXPANDED,wxWeaverObjectEventHandler(fn))

#define EVT_FB_OBJECT_SELECTED(fn) \
  wx__DECLARE_EVT0(wxEVT_FB_OBJECT_SELECTED,wxWeaverObjectEventHandler(fn))

#define EVT_FB_OBJECT_CREATED(fn) \
  wx__DECLARE_EVT0(wxEVT_FB_OBJECT_CREATED,wxWeaverObjectEventHandler(fn))

#define EVT_FB_OBJECT_REMOVED(fn) \
  wx__DECLARE_EVT0(wxEVT_FB_OBJECT_REMOVED,wxWeaverObjectEventHandler(fn))

#define EVT_FB_PROPERTY_MODIFIED(fn) \
  wx__DECLARE_EVT0(wxEVT_FB_PROPERTY_MODIFIED,wxWeaverPropertyEventHandler(fn))

#define EVT_FB_EVENT_HANDLER_MODIFIED(fn) \
  wx__DECLARE_EVT0(wxEVT_FB_EVENT_HANDLER_MODIFIED,wxWeaverEventEventHandler(fn))

#define EVT_FB_PROJECT_REFRESH(fn) \
  wx__DECLARE_EVT0(wxEVT_FB_PROJECT_REFRESH,wxWeaverEventHandler(fn))

#define EVT_FB_CODE_GENERATION(fn) \
    wx__DECLARE_EVT0(wxEVT_FB_CODE_GENERATION,wxWeaverEventHandler(fn))

#endif // __wxWEAVEREVENT__
