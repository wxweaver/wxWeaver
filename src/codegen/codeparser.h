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

#include "utils/debug.h"

#include <wx/textfile.h>
#include <wx/msgdlg.h>

#include <unordered_map>

wxString RemoveWhiteSpace(wxString str);

/** Stores all of the information for all of the parsed functions
*/
class Function {
public:
    Function() { }
    ~Function() { }

    /** Stores the code contained in the body of the function
    */
    void SetContents(wxString contents);

    /** Stores the whole first line of the function as a single string
        ex: "void fubar::DoSomething(int number)"
     */
    void SetHeading(wxString heading);

    /** Stores any code/documentation located between the previous function
        and the current function
    */
    void SetDocumentation(wxString documentation)
    {
        if (documentation.Left(1) == '\n')
            documentation.Remove(0, 1);

        if (documentation.Right(1) == '\n')
            documentation.Remove(documentation.Len() - 1, 1);

        m_documentation = documentation;
    }

    /** Retrieves the body code
    */
    wxString GetHeading() { return m_functionHeading; }

    /** Retrieves the body code
    */
    wxString GetContents() { return m_functionContents; }

    /** Retrieves the documentation
    */
    wxString GetDocumentation() { return m_documentation; }

    /** Retrieves everything including documentation
    */
    wxString GetFunction();

protected:
    wxString m_functionContents;
    wxString m_functionHeading;
    wxString m_documentation;
};

/** Parses the source and header files for all code added to the generated
*/
class CodeParser {
public:
    /** constructor */
    CodeParser() { }
    ~CodeParser() { }

    /** Returns all user header include code before the class declaration
    */
    wxString GetUserIncludes() { return m_userInclude; }

    /** Returns user class members
    */
    wxString GetUserMembers() { return m_userMemebers; }

    /** Returns the Documentation of a function by name
    */
    wxString GetFunctionDocumentation(wxString function);

    /** Returns the contents of a function by name and then removes it
        from the list of remaining functions
    */
    wxString GetFunctionContents(wxString function);

    /** Returns all ramaining functions including documentation as one string.
        This may rearange functions, but should keep them intact
    */
    wxString GetRemainingFunctions();

    wxString GetTrailingCode() { return m_trailingCode; }

protected:
    wxString m_userInclude;
    wxString m_className;
    wxString m_userMemebers;
    wxString m_trailingCode;

    typedef std::unordered_map<std::string, Function*> FunctionMap;
    FunctionMap m_functions;
    FunctionMap::iterator m_functionIter;
};

/** Parses the source and header files for all code added to the generated
*/
class CCodeParser : public CodeParser {
private:
    wxString m_hFile;
    wxString m_cFile;

public:
    /** Constructor
    */
    CCodeParser() { }
    CCodeParser(wxString headerFileName, wxString sourceFileName)
    {
        m_hFile = headerFileName;
        m_cFile = sourceFileName;
    }

    ~CCodeParser() { }

    /** c++ Parser */

    /** Opens the header and source,  'className' is the Inherited class
    */
    void ParseCFiles(wxString className);

    /** Extracts the contents of the files.
        Takes the the entire contents of both files in string form
    */
    void ParseCCode(wxString header, wxString source);

    /** Extracts all user header include code before the class declaration
    */
    void ParseCInclude(wxString code);

    /** Extracts the contents of the generated class declaration
    */
    void ParseCClass(wxString code);

    void ParseSourceFunctions(wxString code);

    wxString ParseBrackets(wxString code, int& functionStart);

    void ParseCUserMembers(wxString code);
};
