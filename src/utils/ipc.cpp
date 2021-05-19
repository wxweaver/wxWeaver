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
#ifdef __BORLANDC__
#pragma hdrstop
#endif

#include "utils/ipc.h"
#include "utils/debug.h"

#include <wx/filename.h>
#include <wx/wx.h>

bool wxWeaverIPC::VerifySingleInstance(const wxString& file, bool switchTo)
{
    // Possible send a message to the running instance through this string later, for now it is left empty
    wxString expression = wxEmptyString;

    // Make path absolute
    wxFileName path(file);
    if (!path.IsOk()) {
        wxLogError("This path is invalid: %s", file.c_str());
        return false;
    }
    if (!path.IsAbsolute()) {
        if (!path.MakeAbsolute()) {
            wxLogError("Could not make path absolute: %s", file.c_str());
            return false;
        }
    }
    // Check for single instance
    // Create lockfile/mutex name
    wxString name = wxString::Format(
        "wxWeaver-%s-%s", wxGetUserId().c_str(), path.GetFullPath().c_str());

    // Get forbidden characters
    wxString forbidden = wxFileName::GetForbiddenChars();

    // Repace forbidded characters
    for (size_t c = 0; c < forbidden.Length(); ++c) {
        wxString bad(forbidden.GetChar(c));
        name.Replace(bad.c_str(), "_");
    }
// Paths are not case sensitive in windows
#ifdef __WXMSW__
    name = name.MakeLower();
#endif

// GetForbiddenChars is missing "/" in unix. Prepend '.' to make lockfiles hidden
#ifndef __WXMSW__
    name.Replace("/", "_");
    name.Prepend(".");
#endif
    // Check to see if I already have a server with this name - if so, no need to make another!
    if (m_server) {
        if (m_server->m_name == name)
            return true;
    }
    std::unique_ptr<wxSingleInstanceChecker> checker;
    {
// Suspend logging, because error messages here are not useful
#ifndef wxWEAVER_DEBUG
        wxLogNull stopLogging;
#endif
        checker.reset(new wxSingleInstanceChecker(name));
    }
    if (!checker->IsAnotherRunning()) {
        // This is the first instance of this project, so setup a server and save the single instance checker
        if (CreateServer(name)) {
            m_checker = std::move(checker);
            return true;
        } else {
            return false;
        }
    } else if (switchTo) {
// Suspend logging, because error messages here are not useful
#ifndef wxWEAVER_DEBUG
        wxLogNull stopLogging;
#endif
        /*
            There is another app, so connect and send the expression.
            Cannot have a client and a server at the same time,
            due to the implementation of wxTCPServer and wxTCPClient,
            so temporarily drop the server if there is one
        */
        bool hadServer = false;
        wxString oldName;
        if (m_server) {
            oldName = m_server->m_name;
            m_server.reset();
            hadServer = true;
        }
        // Create the client
        std::unique_ptr<AppClient> client(new AppClient);

        // Create the connection
        std::unique_ptr<wxConnectionBase> connection;
#ifdef __WXMSW__
        connection.reset(client->MakeConnection("localhost"), name, name));
#else
        bool connected = false;
        for (int i = m_port; i < m_port + 20; ++i) {
            wxString sPort = wxString::Format("%i", i);
            connection.reset(client->MakeConnection("localhost", sPort, name));
            if (connection) {
                connected = true;
                wxChar* pid = (wxChar*)connection->Request("PID", nullptr);
                if (pid) {
                    wxLogStatus("%s already open in process %s", file.c_str(), pid);
                }
                break;
            }
        }
        if (!connected)
            wxLogError(
                "There is a lockfile named '%s', but unable to make a connection to that instance.",
                name.c_str());
#endif

        // Drop the connection and client
        connection.reset();
        client.reset();

        // Create the server again, if necessary
        if (hadServer) {
            CreateServer(oldName);
        }
    }
    return false;
}

bool wxWeaverIPC::CreateServer(const wxString& name)
{
// Suspend logging, because error messages here are not useful
#ifndef wxWEAVER_DEBUG
    wxLogNull stopLogging;
#endif
    auto server = std::make_unique<AppServer>(name);

#ifdef __WXMSW__
    if (server->Create(name)) {
        m_server = std::move(server);
        return true;
    }
#else
    {
        for (int i = m_port; i < m_port + 20; ++i) {
            wxString nameWithPort = wxString::Format("%i%s", i, name.c_str());
            if (server->Create(nameWithPort)) {
                m_server = std::move(server);
                return true;
            } else {
                LogDebug("Server Creation Failed. " + nameWithPort);
            }
        }
    }
#endif
    wxLogError("Failed to create an IPC service with name %s", name.c_str());
    return false;
}

void wxWeaverIPC::Reset()
{
    m_server.reset();
    m_checker.reset();
}

wxConnectionBase* AppServer::OnAcceptConnection(const wxString& topic)
{
    if (topic == m_name) {
        wxFrame* frame = wxDynamicCast(wxTheApp->GetTopWindow(), wxFrame);
        if (!frame)
            return nullptr;

        frame->Enable();
        if (frame->IsIconized())
            frame->Iconize(false);

        frame->Raise();
        return new AppConnection;
    }
    return nullptr;
}

wxConnectionBase* AppClient::OnMakeConnection()
{
    return new AppConnection;
}
