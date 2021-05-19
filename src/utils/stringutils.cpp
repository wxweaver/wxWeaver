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
#include "utils/stringutils.h"
#include "utils/typeconv.h"
#include "utils/exception.h"

#include <ticpp.h>

#include <wx/ffile.h>
#include <wx/fontmap.h>

wxString StringUtils::IntToStr(const int num)
{
    wxString result;
    result.Printf("%d", num);
    return result;
}

wxString StringUtils::GetSupportedEncodings(bool columnateWithTab, wxArrayString* array)
{
    wxString result = wxEmptyString;
    size_t count = wxFontMapper::GetSupportedEncodingsCount();
    size_t max = 40;
    for (size_t i = 0; i < count; ++i) {
        wxFontEncoding encoding = wxFontMapper::GetEncoding(i);
        wxString name = wxFontMapper::GetEncodingName(encoding);
        size_t length = name.length();
        if (length > max)
            max = length + 10;

        if (columnateWithTab)
            name = name.Pad((size_t)((max - length) / 8 + 1), '\t');
        else
            name = name.Pad(max - length);

        name += wxFontMapper::GetEncodingDescription(encoding);
        if (array)
            array->Add(name);

        result += name;
        result += "\n";
    }
    return result;
}

wxFontEncoding StringUtils::GetEncodingFromUser(const wxString& message)
{
    wxArrayString array;
    GetSupportedEncodings(false, &array);
    int selection = ::wxGetSingleChoiceIndex(message, _("Choose an Encoding"),
                                             array, wxTheApp->GetTopWindow());
    if (selection == -1)
        return wxFONTENCODING_MAX;

    return wxFontMapper::GetEncoding(selection);
}

namespace XMLUtils {
template <class T, class U>
void LoadXMLFileImp(T& doc, bool condenseWhiteSpace, const wxString& path,
                    U* declaration)
{
    if (!declaration) {
        // Ask user to all wxWeaver to convert the file to UTF-8 and add the XML declaration
        wxString msg = _("This xml file has no declaration.\n");
        msg += _("Would you like wxWeaver to backup the file and convert it to UTF-8\?\n");
        msg += _("You will be prompted for an encoding.\n\n");
        msg += _("Path: ");
        msg += path;
        int result = wxMessageBox(msg, _("Missing Declaration"),
                                  wxICON_QUESTION | wxYES_NO | wxYES_DEFAULT,
                                  wxTheApp->GetTopWindow());
        if (result == wxNO)
            wxWEAVER_THROW_EX(_("Missing Declaration on XML File: ") << path); // User declined, give up

        // User accepted, convert the file
        wxFontEncoding chosenEncoding
            = StringUtils::GetEncodingFromUser(_("Please choose the original encoding."));

        if (wxFONTENCODING_MAX == chosenEncoding)
            wxWEAVER_THROW_EX(_("Missing Declaration on XML File: ") << path);

        ConvertAndAddDeclaration(path, chosenEncoding);

        // Reload
        LoadXMLFile(doc, condenseWhiteSpace, path);
        return;
    }

    // The file will have a declaration at this point
    wxString version = _WXSTR(declaration->Version());
    if (version.empty())
        version = "1.0";

    wxString standalone = _WXSTR(declaration->Standalone());
    if (standalone.empty())
        standalone = "yes";

    wxString encodingName = _WXSTR(declaration->Encoding());
    if (encodingName.empty()) {
        // Ask user to all wxWeaver to convert the file to UTF-8 and add the XML declaration
        wxString msg = _("This xml file has no encoding specified.\n");
        msg += _("Would you like wxWeaver to backup the file and convert it to UTF-8\?\n");
        msg += _("You will be prompted for an encoding.\n\n");
        msg += _("Path: ");
        msg += path;
        if (wxMessageBox(
                msg, _("Unknown Encoding"),
                wxICON_QUESTION | wxYES_NO | wxYES_DEFAULT,
                wxTheApp->GetTopWindow())
            == wxNO)
            wxWEAVER_THROW_EX(_("Unknown Encoding for XML File: ") << path); // User declined, give up

        // User accepted, convert the file
        wxFontEncoding chosenEncoding
            = StringUtils::GetEncodingFromUser(_("Please choose the original encoding."));

        if (wxFONTENCODING_MAX == chosenEncoding)
            wxWEAVER_THROW_EX(_("Unknown Encoding for XML File: ") << path);

        ConvertAndChangeDeclaration(path, version, standalone, chosenEncoding);

        // Reload
        LoadXMLFile(doc, condenseWhiteSpace, path);
        return;
    }

    // The file will have an encoding at this point
    wxFontEncoding encoding = wxFontMapperBase::GetEncodingFromName(encodingName.MakeLower());
    if (wxFONTENCODING_UTF8 == encoding) {
        return; // This is what we want
    } else if (wxFONTENCODING_MAX == encoding) {
        wxString msg = wxString::Format(
            _("The encoding of this xml file is not supported.\n\nFile: %s\nEncoding: %s\nSupported Encodings:\n\n%s"),
            path.c_str(),
            encodingName.c_str(),
            StringUtils::GetSupportedEncodings().c_str());

        wxMessageBox(msg, wxString::Format(_("Unsupported Encoding: %s"), encodingName.c_str()));
        wxWEAVER_THROW_EX(_("Unsupported encoding for XML File: ") << path);
    } else {
        // Ask user to all wxWeaver to convert the file to UTF-8 and add the XML declaration
        wxString msg = wxString::Format(_("This xml file has specified encoding %s. wxWeaver only works with UTF-8.\n"),
                                        wxFontMapper::GetEncodingDescription(encoding).c_str());
        msg += _("Would you like wxWeaver to backup the file and convert it to UTF-8\?\n\n");
        msg += _("Path: ");
        msg += path;
        if (wxMessageBox(
                msg, _("Not UTF-8"),
                wxICON_QUESTION | wxYES_NO | wxYES_DEFAULT,
                wxTheApp->GetTopWindow())
            == wxNO)
            wxWEAVER_THROW_EX(_("Wrong Encoding for XML File: ") << path); // User declined, give up

        // User accepted, convert the file
        ConvertAndChangeDeclaration(path, version, standalone, encoding);

        // Reload
        LoadXMLFile(doc, condenseWhiteSpace, path);
        return;
    }
}
} // namespace XMLUtils

void XMLUtils::LoadXMLFile(ticpp::Document& doc, bool condenseWhiteSpace,
                           const wxString& path)
{
    try {
        if (path.empty()) {
            wxWEAVER_THROW_EX(_("LoadXMLFile needs a path"))
        }
        if (!::wxFileExists(path)) {
            wxWEAVER_THROW_EX(_("The file does not exist.\nFile: ") << path)
        }
        TiXmlBase::SetCondenseWhiteSpace(condenseWhiteSpace);

        doc.SetValue(std::string(path.mb_str(wxConvFile)));
        doc.LoadFile();
    } catch (ticpp::Exception&) {
        // Ask user to all wxWeaver to convert the file to UTF-8 and add the XML declaration
        wxString msg = _("This xml file could not be loaded. This could be the result of an unsupported encoding.\n");
        msg += _("Would you like wxWeaver to backup the file and convert it to UTF-8\?\n");
        msg += _("You will be prompted for the original encoding.\n\n");
        msg += _("Path: ");
        msg += path;
        if (wxMessageBox(
                msg, _("Unable to load file"),
                wxICON_QUESTION | wxYES_NO | wxYES_DEFAULT,
                wxTheApp->GetTopWindow())
            == wxNO)
            wxWEAVER_THROW_EX(_("Unable to load file: ") << path); // User declined, give up

        // User accepted, convert the file
        wxFontEncoding chosenEncoding
            = StringUtils::GetEncodingFromUser(_("Please choose the original encoding."));

        if (wxFONTENCODING_MAX == chosenEncoding)
            wxWEAVER_THROW_EX(_("Unable to load file: ") << path);

        ConvertAndAddDeclaration(path, chosenEncoding);

        LoadXMLFile(doc, condenseWhiteSpace, path);
    }
    ticpp::Declaration* declaration;
    try {
        ticpp::Node* firstChild = doc.FirstChild();
        declaration = firstChild->ToDeclaration();
    } catch (ticpp::Exception&) {
        declaration = NULL;
    }
    LoadXMLFileImp(doc, condenseWhiteSpace, path, declaration);
}

void XMLUtils::LoadXMLFile(TiXmlDocument& doc, bool condenseWhiteSpace, const wxString& path)
{
    if (path.empty()) {
        wxWEAVER_THROW_EX(_("LoadXMLFile needs a path"))
    }
    if (!::wxFileExists(path)) {
        wxWEAVER_THROW_EX(_("The file does not exist.\nFile: ") << path)
    }

    TiXmlBase::SetCondenseWhiteSpace(condenseWhiteSpace);
    doc.SetValue(std::string(path.mb_str(wxConvFile)));
    if (!doc.LoadFile()) {
        // Ask user to all wxWeaver to convert the file to UTF-8 and add the XML declaration
        wxString msg = _("This xml file could not be loaded. This could be the result of an unsupported encoding.\n");
        msg += _("Would you like wxWeaver to backup the file and convert it to UTF-8\?\n");
        msg += _("You will be prompted for the original encoding.\n\n");
        msg += _("Path: ");
        msg += path;
        if (wxMessageBox(
                msg, _("Unable to load file"),
                wxICON_QUESTION | wxYES_NO | wxYES_DEFAULT,
                wxTheApp->GetTopWindow())
            == wxNO)
            wxWEAVER_THROW_EX(_("Unable to load file: ") << path); // User declined, give up

        // User accepted, convert the file
        wxFontEncoding chosenEncoding
            = StringUtils::GetEncodingFromUser(
                _("Please choose the original encoding."));

        if (wxFONTENCODING_MAX == chosenEncoding)
            wxWEAVER_THROW_EX(_("Unable to load file: ") << path);

        ConvertAndAddDeclaration(path, chosenEncoding);
        LoadXMLFile(doc, condenseWhiteSpace, path);
    }
    TiXmlDeclaration* declaration = NULL;
    TiXmlNode* firstChild = doc.FirstChild();
    if (firstChild)
        declaration = firstChild->ToDeclaration();

    LoadXMLFileImp(doc, condenseWhiteSpace, path, declaration);
}

void XMLUtils::ConvertAndAddDeclaration(const wxString& path,
                                        wxFontEncoding encoding, bool backup)
{
    ConvertAndChangeDeclaration(path, "1.0", "yes", encoding, backup);
}

void XMLUtils::ConvertAndChangeDeclaration(const wxString& path,
                                           const wxString& version,
                                           const wxString& standalone,
                                           wxFontEncoding encoding, bool backup)
{
    // Backup the file
    if (backup) {
        if (!::wxCopyFile(path, path + ".bak")) {
            wxString msg = wxString::Format(
                _("Unable to backup file.\nFile: %s\nBackup: %s.bak"),
                path.c_str(), path.c_str());
            wxWEAVER_THROW_EX(msg)
        }
    }
    // Read the entire contents into a string
    wxFFile oldEncoding(path.c_str(), "r");
    wxString contents;
    wxCSConv encodingConv(encoding);
    if (!oldEncoding.ReadAll(&contents, encodingConv)) {
        wxString msg = wxString::Format(
            _("Unable to read the file in the specified encoding.\nFile: %s\nEncoding: %s"),
            path.c_str(), wxFontMapper::GetEncodingDescription(encoding).c_str());
        wxWEAVER_THROW_EX(msg);
    }
    if (contents.empty()) {
        wxString msg = wxString::Format(
            _("The file is either empty or read with the wrong encoding.\nFile: %s\nEncoding: %s"),
            path.c_str(), wxFontMapper::GetEncodingDescription(encoding).c_str());
        wxWEAVER_THROW_EX(msg);
    }
    if (!oldEncoding.Close()) {
        wxString msg = wxString::Format(
            _("Unable to close original file.\nFile: %s"), path.c_str());
        wxWEAVER_THROW_EX(msg);
    }
    // Modify the declaration, so TinyXML correctly determines the new encoding
    int declStart = contents.Find("<\?");
    int declEnd = contents.Find("\?>");
    if (declStart == wxNOT_FOUND && declEnd == wxNOT_FOUND) {
        int firstElement = contents.Find("<");
        if (firstElement == wxNOT_FOUND)
            firstElement = 0;

        contents.insert(firstElement, wxString::Format("<\?xml version=\"%s\" encoding=\"UTF-8\" standalone=\"%s\" \?>\n", version.c_str(), standalone.c_str()));
    } else {
        if (declStart == wxNOT_FOUND) {
            wxString msg = wxString::Format(
                _("Found a declaration end tag \"\?>\" but could not find the start \"<\?\".\nFile: %s"),
                path.c_str());
            wxWEAVER_THROW_EX(msg);
        }
        if (declEnd == wxNOT_FOUND) {
            wxString msg = wxString::Format(
                _("Found a declaration start tag \"<\?\" but could not find the end \"\?>\".\nFile: %s"),
                path.c_str());
            wxWEAVER_THROW_EX(msg);
        }
        // declStart and declEnd are both valid, replace that section with a new declaration
        contents.replace(
            declStart, declEnd - declStart + 2,
            wxString::Format("<\?xml version=\"%s\" encoding=\"UTF-8\" standalone=\"%s\" \?>",
                             version, standalone));
    }
    // Remove the old file
    if (!::wxRemoveFile(path)) {
        wxString msg = wxString::Format(
            _("Unable to delete original file.\nFile: %s"), path.c_str());
        wxWEAVER_THROW_EX(msg);
    }
    // Write the new file
    wxFFile newEncoding(path.c_str(), "w");
    if (!newEncoding.Write(contents, wxConvUTF8)) {
        wxString msg = wxString::Format(
            _("Unable to write file in its new encoding.\nFile: %s\nEncoding: %s"),
            path.c_str(),
            wxFontMapper::GetEncodingDescription(wxFONTENCODING_UTF8).c_str());
        wxWEAVER_THROW_EX(msg);
    }
    if (!newEncoding.Close()) {
        wxString msg = wxString::Format(
            _("Unable to close file after converting the encoding.\nFile: %s\nOld Encoding: %s\nNew Encoding: %s"),
            path.c_str(),
            wxFontMapper::GetEncodingDescription(encoding).c_str(),
            wxFontMapper::GetEncodingDescription(wxFONTENCODING_UTF8).c_str());
        wxWEAVER_THROW_EX(msg);
    }
}
