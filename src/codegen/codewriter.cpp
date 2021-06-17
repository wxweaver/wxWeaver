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
#include "codewriter.h"

#include "md5/md5.hh"
#include "utils/typeconv.h"
#include "utils/exception.h"

#include <wx/file.h>
#include <wx/regex.h>
#include <wx/tokenzr.h>
#include <wx/stc/stc.h>

#include <cstring>
#include <fstream>

CodeWriter::CodeWriter()
    : m_indent(0)
    , m_isLineWriting(false)
    , m_hasSpacesIndentation(false)
{
}

CodeWriter::~CodeWriter() = default;

int CodeWriter::GetIndentSize() const
{
    return 1;
}

void CodeWriter::Indent()
{
    m_indent += GetIndentSize();
}

void CodeWriter::Unindent()
{
    m_indent -= GetIndentSize();

    if (m_indent < 0)
        m_indent = 0;
}

bool CodeWriter::IsSingleLine(const wxString& code) const
{
    return (code.find('\n') == wxString::npos);
}

void CodeWriter::ProcessLine(wxString line, bool rawIndents)
{
    static const wxRegEx reIndent = wxRegEx("%TAB%\\s*", wxRE_ADVANCED);

    // Cleanup whitespace
    if (!rawIndents)
        line.Trim(false);

    line.Trim();
    // Remove and count indentations defined in code templates by #indent and
    // #unindent macros to use own indentation mode
    auto templateIndents = reIndent.Replace(&line, "");
    if (templateIndents < 0)
        templateIndents = 0;

    m_indent += templateIndents;
    Write(line, rawIndents);
    m_indent -= templateIndents;

    // To prevent trailing whitespace in case the line was empty write the newline as-is
    Write('\n', true);
    m_isLineWriting = false;
}

void CodeWriter::WriteLn(const wxString& code, bool rawIndents)
{
    if (!IsSingleLine(code)) {
        wxStringTokenizer tkz(code, '\n', wxTOKEN_RET_EMPTY_ALL);

        while (tkz.HasMoreTokens())
            ProcessLine(tkz.GetNextToken(), rawIndents);
    } else {
        ProcessLine(code, rawIndents);
    }
}

void CodeWriter::Write(const wxString& code, bool rawIndents)
{
    // Early abort to not produce lines with trailing whitespace
    if (code.empty())
        return;

    if (!m_isLineWriting) {
        if (!rawIndents) {
            for (int i = 0; i < m_indent; ++i)
                DoWrite(m_hasSpacesIndentation ? "    " : "\t");
        }
        m_isLineWriting = true;
    }
    DoWrite(code);
}

void CodeWriter::SetIndentWithSpaces(bool on)
{
    m_hasSpacesIndentation = on;
}

TCCodeWriter::TCCodeWriter()
    : m_styledTextCtrl(nullptr)
{
}

TCCodeWriter::TCCodeWriter(wxStyledTextCtrl* tc)
{
    SetTextCtrl(tc);
}

void TCCodeWriter::SetTextCtrl(wxStyledTextCtrl* tc)
{
    m_styledTextCtrl = tc;
}

void TCCodeWriter::DoWrite(const wxString& code)
{
    if (m_styledTextCtrl)
        m_styledTextCtrl->AddText(code);
}

void TCCodeWriter::Clear()
{
    if (m_styledTextCtrl)
        m_styledTextCtrl->ClearAll();
}

StringCodeWriter::StringCodeWriter()
{
}

void StringCodeWriter::DoWrite(const wxString& code)
{
    m_buffer += code;
}

void StringCodeWriter::Clear()
{
    m_buffer.clear();
}

const wxString& StringCodeWriter::GetString() const
{
    return m_buffer;
}

FileCodeWriter::FileCodeWriter(const wxString& file, bool useMicrosoftBOM,
                               bool useUtf8)
    : m_filename(file)
    , m_useMicrosoftBOM(useMicrosoftBOM)
    , m_useUtf8(useUtf8)
{
    Clear();
}

FileCodeWriter::~FileCodeWriter()
{
    WriteBuffer();
}

void FileCodeWriter::WriteBuffer()
{
    static const unsigned char MICROSOFT_BOM[3] = { 0xEF, 0xBB, 0xBF };

    // TODO: Check this
    std::string data;
    if (m_useUtf8)
        data = m_buffer.ToStdString();
    else
        data = m_buffer.mb_str(wxConvISO8859_1);

    // Compare buffer with existing file (if any) to determine if
    // writing the file is necessary
    bool shouldWrite = true;
    std::ifstream fileIn(m_filename.mb_str(wxConvFile), std::ios::binary | std::ios::in);

    if (fileIn) {
        MD5 diskHash(fileIn);
        unsigned char* diskDigest = diskHash.raw_digest();

        MD5 bufferHash;
        if (m_useUtf8 && m_useMicrosoftBOM) {
            bufferHash.update(MICROSOFT_BOM, 3);
        }
        bufferHash.update(reinterpret_cast<const unsigned char*>(data.c_str()), data.size());
        bufferHash.finalize();
        unsigned char* bufferDigest = bufferHash.raw_digest();

        shouldWrite = (0 != std::memcmp(diskDigest, bufferDigest, 16));

        delete[] diskDigest;
        delete[] bufferDigest;
    }
    if (shouldWrite) {
        wxFile fileOut;
        if (!fileOut.Create(m_filename, true)) {
            wxLogError("Unable to create file: %s", m_filename.c_str());
            return;
        }
        if (m_useUtf8 && m_useMicrosoftBOM)
            fileOut.Write(MICROSOFT_BOM, 3);

        fileOut.Write(data.c_str(), data.length());
    }
}

void FileCodeWriter::Clear()
{
    StringCodeWriter::Clear();

    if (::wxFileExists(m_filename)) {
        // check for write access to the target file
        if (!wxFile::Access(m_filename, wxFile::write))
            wxWEAVER_THROW_EX("Unable to write file: " << m_filename);
    } else {
        wxFile file;
        if (!file.Create(m_filename, true))
            wxWEAVER_THROW_EX("Unable to create file: " << m_filename);
    }
}
