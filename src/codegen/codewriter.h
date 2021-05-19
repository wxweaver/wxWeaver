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

#include <wx/string.h>

/** Abstracts the code generation from the target.
    Because, in some cases the target is a file, sometimes a TextCtrl, and sometimes both.
*/
class CodeWriter {
public:
    /** Constructor.
    */
    CodeWriter();
    virtual ~CodeWriter();

    /** Increment the indent.
    */
    void Indent();

    /** Decrement the indent.
    */
    void Unindent();

    /** Write a block of code with trailing newline

        This is a general purpose method to output the input properly formatted.
        Cleans up whitespace and applies template indentation processing.

        @param code Block of code
        @param rawIndents If true, keep leading indenting whitespace and don't apply own indenting
     */
    void WriteLn(const wxString& code = wxEmptyString, bool rawIndents = false);

    /** Write a fragment of code without trailing newline

        This method is not intended for general purpose output but only to output preformatted input.
        No whitespace cleanup and no template indentation processing is performed!

        The initial call of this method initiates the output process, it can be called multiple times
        to continue the output process, but it must be terminated with a call to WriteLn(const wxString&, bool).

        @param code Block of code
        @param rawIndents If true, keep leading indenting whitespace and don't apply own indenting
     */
    void Write(const wxString& code, bool rawIndents = false);

    /** Sets the option to indent with spaces
    */
    void SetIndentWithSpaces(bool on);

    /** Deletes all the code previously written.
    */
    virtual void Clear() = 0;

protected:
    /** Write a wxString.
    */
    virtual void DoWrite(const wxString& code) = 0;

    // TODO: "was useful when using spaces, now it is 1 because using tabs"
    //       no shit like this, make spaces or tabs as options instead.
    /** Returns the size of the indentation.
    */
    virtual int GetIndentSize() const;

    /** Returns if code doesn't contain any newline character

        @param code Code fragment
    */
    bool IsSingleLine(const wxString& code) const;

    /** Outputs a single line

        Performs whitespace cleanup and indentation processing
        including the special markers of the TemplateParser.

        @param line Single line, must not contain newlines
        @param rawIndents If true, keep leading indenting whitespace and don't apply own indenting
     */
    void ProcessLine(wxString line, bool rawIndents);

private:
    int m_indent;                // Current indentation level in the file
    bool m_isLineWriting;        // Flag if line writing is in progress
    bool m_hasSpacesIndentation; // If using spaces for indentation
};

class wxStyledTextCtrl;

class TCCodeWriter : public CodeWriter {
public:
    TCCodeWriter();
    TCCodeWriter(wxStyledTextCtrl*);

    void SetTextCtrl(wxStyledTextCtrl*);
    void Clear() override;

protected:
    void DoWrite(const wxString&) override;

private:
    wxStyledTextCtrl* m_styledTextCtrl;
};

class StringCodeWriter : public CodeWriter {
public:
    StringCodeWriter();

    void Clear() override;
    const wxString& GetString() const;

protected:
    void DoWrite(const wxString& code) override;

    wxString m_buffer;
};

class FileCodeWriter : public StringCodeWriter {
public:
    FileCodeWriter(const wxString& file, bool useMicrosoftBOM = false,
                   bool useUtf8 = true);
    ~FileCodeWriter() override;

    void Clear() final;

protected:
    void WriteBuffer();

private:
    wxString m_filename;
    bool m_useMicrosoftBOM;
    bool m_useUtf8;
};
