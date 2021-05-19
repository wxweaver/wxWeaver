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

#include <wx/ipc.h>
#include <wx/snglinst.h>

#include <memory>

// Only allow one instance of a project to be loaded at a time
class AppServer;

class wxWeaverIPC {
public:
    wxWeaverIPC()
        : m_port(4242)
    {
    }
    bool VerifySingleInstance(const wxString& file, bool switchTo = true);
    void Reset();

private:
    bool CreateServer(const wxString& name);

    std::unique_ptr<wxSingleInstanceChecker> m_checker;
    std::unique_ptr<AppServer> m_server;
    const int m_port;
};

// Connection class, for use by both communicationg instances
class AppConnection : public wxConnection {
public:
    AppConnection() { }

private:
    wxString m_data;
};

// Server class, for listening to connection requests
class AppServer : public wxServer {
public:
    AppServer(const wxString& name)
        : m_name(name)
    {
    }
    wxConnectionBase* OnAcceptConnection(const wxString& topic) override;

    const wxString m_name;
};

// Client class, to be used by subsequent instances in OnInit
class AppClient : public wxClient {
public:
    AppClient() { }
    wxConnectionBase* OnMakeConnection() override;
};
