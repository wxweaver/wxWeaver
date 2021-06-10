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
#include "utils/typeconv.h"
#include "appdata.h"
#include "gui/bitmaps.h"
#include "gui/panels/inspector/inspector.h"

#include <wx/artprov.h>
#include <wx/filesys.h>

/*
    Assuming that the locale is constant throughout one execution,
    store the locale so that numbers can be stored in the "C" locale,
    but the rest of the program works in the user's locale.
*/
class LocaleFinder {
public:
    LocaleFinder()
    {
        // get current locale
        char* localePtr = ::setlocale(LC_NUMERIC, 0);
        size_t size = ::strlen(localePtr) + 1;
        m_locale = new char[size];
        ::strncpy(m_locale, localePtr, size);
    }
    ~LocaleFinder() { delete[] m_locale; }

    const char* GetString() { return m_locale; }

private:
    char* m_locale;
};
/*
    Unfortunately, the locale is "C" at the start of execution,
    and is changed to the correct locale sometime later.
    This means that the LocaleFinder object cannot be simple declared like this:

        static LocaleFinder s_locale;

    Instead, it must be created the first time that it is used,
    stored for the duration of the program, and deleted on close.
*/
class LocaleHolder {
private:
    LocaleFinder* m_finder;

public:
    LocaleHolder()
        : m_finder(0)
    {
    }
    ~LocaleHolder() { delete m_finder; }

    const char* GetString()
    {
        if (!m_finder)
            m_finder = new LocaleFinder;

        return m_finder->GetString();
    }
};
// Creating this object will determine the current locale (when needed)
// and store it for the duration of the program
static LocaleHolder s_locale;

// Utility class for switching to "C" locale and back
class LocaleSwitcher {
public:
    LocaleSwitcher()
    {
        m_locale = s_locale.GetString(); // Get the locale first, or it will be lost!
        ::setlocale(LC_NUMERIC, "C");
    }
    ~LocaleSwitcher() { ::setlocale(LC_NUMERIC, m_locale); }

private:
    const char* m_locale;
};

using namespace TypeConv;

wxString TypeConv::_StringToWxString(const std::string& str)
{
    return _StringToWxString(str.c_str());
}

wxString TypeConv::_StringToWxString(const char* str)
{
    wxString newstr(str, wxConvUTF8);
    return newstr;
}

std::string TypeConv::_WxStringToString(const wxString& str)
{
    std::string newstr(str.mb_str(wxConvUTF8));
    return newstr;
}

std::string TypeConv::_WxStringToAnsiString(const wxString& str)
{
    std::string newstr(str.mb_str(wxConvISO8859_1));
    return newstr;
#if 0
    setlocale(LC_ALL, "");
    size_t len = wcstombs(nullptr, str.char_str(), 0);
    std::vector<char> buf(len + 1);
    wcstombs(&buf[0], str.char_str(), len);
    return std::string(&buf[0]);
#endif
}

bool TypeConv::StringToPoint(const wxString& val, wxPoint* point)
{
    wxPoint result;
    bool error = false;
    wxString str_x, str_y;
    long val_x = -1, val_y = -1;

    if (val != "") {
        wxStringTokenizer tkz(val, ",");
        if (tkz.HasMoreTokens()) {
            str_x = tkz.GetNextToken();
            str_x.Trim(true);
            str_x.Trim(false);
            if (tkz.HasMoreTokens()) {
                str_y = tkz.GetNextToken();
                str_y.Trim(true);
                str_y.Trim(false);
            } else
                error = true;
        } else
            error = true;

        if (!error)
            error = !str_x.ToLong(&val_x);

        if (!error)
            error = !str_y.ToLong(&val_y);

        if (!error)
            result = wxPoint(val_x, val_y);
    } else
        result = wxDefaultPosition;

    if (error)
        result = wxDefaultPosition;

    point->x = result.x;
    point->y = result.y;

    return !error;
}

wxPoint TypeConv::StringToPoint(const wxString& val)
{
    wxPoint result;
    StringToPoint(val, &result);
    return result;
}

wxSize TypeConv::StringToSize(const wxString& val)
{
    wxPoint point = StringToPoint(val);
    return wxSize(point.x, point.y);
}

int TypeConv::BitlistToInt(const wxString& str)
{
    int result = 0;
    wxStringTokenizer tkz(str, "|");
    while (tkz.HasMoreTokens()) {
        wxString token;
        token = tkz.GetNextToken();
        token.Trim(true);
        token.Trim(false);

        result |= GetMacroValue(token);
    }
    return result;
}

wxString TypeConv::PointToString(const wxPoint& point)
{
    wxString value = wxString::Format("%d,%d", point.x, point.y);
    return value;
}

wxString TypeConv::SizeToString(const wxSize& size)
{
    wxString value = wxString::Format("%d,%d", size.GetWidth(), size.GetHeight());
    return value;
}

int TypeConv::GetMacroValue(const wxString& str)
{
    int value = 0;
    PMacroDictionary dic = MacroDictionary::GetInstance();
    if (!dic->SearchMacro(str, &value))
        value = StringToInt(str);

    return value;
}

int TypeConv::StringToInt(const wxString& str)
{
    long l = 0;
    str.ToLong(&l);
    return (int)l;
}

wxFontContainer TypeConv::StringToFont(const wxString& str)
{
    wxFontContainer font;

    // face name, style, weight, point size, family, underlined
    wxStringTokenizer tkz(str, ",");

    if (tkz.HasMoreTokens()) {
        wxString faceName = tkz.GetNextToken();
        faceName.Trim(true);
        faceName.Trim(false);
        font.SetFaceName(faceName);
    }
    if (tkz.HasMoreTokens()) {
        long l_style;
        wxString s_style = tkz.GetNextToken();
        if (s_style.ToLong(&l_style)) {
            if (l_style >= wxFONTSTYLE_NORMAL && l_style < wxFONTSTYLE_MAX)
                font.SetStyle(static_cast<wxFontStyle>(l_style));
            else
                font.SetStyle(wxFONTSTYLE_NORMAL);
        }
    }
    if (tkz.HasMoreTokens()) {
        long l_weight;
        wxString s_weight = tkz.GetNextToken();
        if (s_weight.ToLong(&l_weight)) {
            if (l_weight >= wxFONTWEIGHT_NORMAL && l_weight < wxFONTWEIGHT_MAX)
                font.SetWeight(static_cast<wxFontWeight>(l_weight));
            else
                font.SetWeight(wxFONTWEIGHT_NORMAL);
        }
    }
    if (tkz.HasMoreTokens()) {
        long l_size;
        wxString s_size = tkz.GetNextToken();
        if (s_size.ToLong(&l_size))
            font.SetPointSize((int)l_size);
    }
    if (tkz.HasMoreTokens()) {
        long l_family;
        wxString s_family = tkz.GetNextToken();
        if (s_family.ToLong(&l_family)) {
            if (l_family >= wxFONTFAMILY_DEFAULT && l_family < wxFONTFAMILY_MAX)
                font.SetFamily(static_cast<wxFontFamily>(l_family));
            else
                font.SetFamily(wxFONTFAMILY_DEFAULT);
        }
    }
    if (tkz.HasMoreTokens()) {
        long l_underlined;
        wxString s_underlined = tkz.GetNextToken();
        if (s_underlined.ToLong(&l_underlined))
            font.SetUnderlined(l_underlined);
    }
    return font;
}

wxString TypeConv::FontToString(const wxFontContainer& font)
{
    // face name, style, weight, point size, family, underlined
    return wxString::Format(
        "%s,%d,%d,%d,%d,%d", font.GetFaceName().c_str(),
        font.GetStyle(), font.GetWeight(), font.GetPointSize(),
        font.GetFamily(), font.GetUnderlined() ? 1 : 0);
}

wxString TypeConv::FontFamilyToString(wxFontFamily family)
{
    wxString result;

    switch (family) {
    case wxFONTFAMILY_DECORATIVE:
        result = "wxFONTFAMILY_DECORATIVE";
        break;
    case wxFONTFAMILY_ROMAN:
        result = "wxFONTFAMILY_ROMAN";
        break;
    case wxFONTFAMILY_SCRIPT:
        result = "wxFONTFAMILY_SCRIPT";
        break;
    case wxFONTFAMILY_SWISS:
        result = "wxFONTFAMILY_SWISS";
        break;
    case wxFONTFAMILY_MODERN:
        result = "wxFONTFAMILY_MODERN";
        break;
    case wxFONTFAMILY_TELETYPE:
        result = "wxFONTFAMILY_TELETYPE";
        break;
    default:
        result = "wxFONTFAMILY_DEFAULT";
        break;
    }
    return result;
}

wxString TypeConv::FontStyleToString(wxFontStyle style)
{
    wxString result;

    switch (style) {
    case wxFONTSTYLE_ITALIC:
        result = "wxFONTSTYLE_ITALIC";
        break;
    case wxFONTSTYLE_SLANT:
        result = "wxFONTSTYLE_SLANT";
        break;
    default:
        result = "wxFONTSTYLE_NORMAL";
        break;
    }
    return result;
}

wxString TypeConv::FontWeightToString(wxFontWeight weight)
{
    wxString result;

    switch (weight) {
    case wxFONTWEIGHT_LIGHT:
        result = "wxFONTWEIGHT_LIGHT";
        break;
    case wxFONTWEIGHT_BOLD:
        result = "wxFONTWEIGHT_BOLD";
        break;
    default:
        result = "wxFONTWEIGHT_NORMAL";
        break;
    }
    return result;
}

wxBitmap TypeConv::StringToBitmap(const wxString& filename)
{
#ifndef wxWEAVER_DEBUG
    wxLogNull stopLogging;
#endif
    // Get bitmap from art provider
    if (filename.Contains(_("Load From Art Provider"))) {
        wxString image = filename.AfterFirst(';').Trim(false);
        wxString rid = image.BeforeFirst(';').Trim(false);
        wxString cid = image.AfterFirst(';').Trim(false);

        if (rid.IsEmpty() || cid.IsEmpty()) {
            return AppBitmaps::GetBitmap("unknown");
        } else {
#if 0
            return wxArtProvider::GetBitmap( rid, cid + "_C") {
#endif
            wxBitmap bmp = wxArtProvider::GetBitmap(rid, cid + "_C");
            if (!bmp.IsOk()) {
                return AppBitmaps::GetBitmap("unknown");
#if 0
                /*
                    Create another bitmap of the appropriate size to show it's invalid.
                    We can get here if the user entered a custom wxArtID which, presumably,
                    they will have already installed in their app.
                */
                bmp = wxArtProvider::GetBitmap("wxART_MISSING_IMAGE", cid + "_C");
                if (bmp.IsOk()) {
                    wxMemoryDC dc;
                    dc.SelectObject(bmp);
                    dc.SetPen(wxPen(*wxRED, 3));
                    dc.DrawLine(wxPoint(0, 0), wxPoint(bmp.GetWidth(), bmp.GetHeight()));
                    dc.SelectObject(wxNullBitmap);
                }
#endif
            } else {
                return bmp;
            }
        }
    }
    // Get path from bitmap property
    wxString path = filename.AfterFirst(';').Trim(false);
    // No value - default bitmap
    if (!wxFileName(path).IsOk())
        return AppBitmaps::GetBitmap("unknown");

    // Setup the working directory to the project path - paths should be saved in the .fbp file relative to the location
    // of the .fbp file
    wxFileSystem system;
    system.ChangePathTo(AppData()->GetProjectPath(), true);

    // The loader can get goofy on linux if it starts with file:, not sure why (wxGTK 2.8.7)
    wxFSFile* fsfile = nullptr;
    wxString remainder;
    if (path.StartsWith("file:", &remainder))
        fsfile = system.OpenFile(remainder, wxFS_READ | wxFS_SEEKABLE);
    else
        fsfile = system.OpenFile(path, wxFS_READ | wxFS_SEEKABLE);

    // Unable to open the file
    if (!fsfile)
        return AppBitmaps::GetBitmap("unknown");

    // Create a wxImage from the file stream
    wxImage img(*(fsfile->GetStream()));
    delete fsfile;

    // The stream is not an image
    if (!img.Ok())
        return AppBitmaps::GetBitmap("unknown");

    // Create a wxBitmap from the image
    return wxBitmap(img);
}

void TypeConv::ParseBitmapWithResource(const wxString& value, wxString* image,
                                       wxString* source, wxSize* icoSize)
{
    // Splitting bitmap resource property value
    // it is of the form "path; source [width; height]"
    *image = value;
    *source = _("Load From File");
    *icoSize = wxDefaultSize;

    wxArrayString children;
    wxStringTokenizer tkz(value, "[];", wxTOKEN_RET_EMPTY);
    while (tkz.HasMoreTokens()) {
        wxString child = tkz.GetNextToken();
        child.Trim(false);
        child.Trim(true);
        children.Add(child);
    }
    if (children.Index(_("Load From Art Provider")) == wxNOT_FOUND) {
        long temp;
        switch (children.size()) {
        case 5:
        case 4:
            if (children.size() > 4) {
                children[4].ToLong(&temp);
                icoSize->SetHeight(temp);
            }
            wxFALLTHROUGH;
        case 3:
            if (children.size() > 3) {
                children[3].ToLong(&temp);
                icoSize->SetWidth(temp);
            }
            wxFALLTHROUGH;
        case 2:
            if (children.size() > 1)
                *image = children[1];
            wxFALLTHROUGH;
        case 1:
            if (children.size() > 0)
                *source = children[0];
            break;
        default:
            break;
        }
    } else {
        if (children.size() == 3) {
            *image = children[1] + ":" + children[2];
            *source = children[0];
        } else {
            *image = "";
            *source = children[0];
        }
    }
    wxLogDebug(
        "TypeConv:ParseBitmap: source:%s image:%s ",
        source->c_str(), image->c_str());
}

wxString TypeConv::MakeAbsolutePath(const wxString& filename, const wxString& basePath)
{
    wxFileName fnFile(filename);
    wxFileName noChanges = fnFile;
    if (fnFile.IsRelative()) {
        // Es una ruta relativa, por tanto hemos de obtener la ruta completa
        // a partir de basePath
        wxFileName fnBasePath(basePath);
        if (fnBasePath.IsAbsolute()) {
            if (fnFile.MakeAbsolute(basePath)) {
                wxString path = fnFile.GetFullPath();
                return path;
            }
        }
    }

    // Either it is already absolute, or it could not be made absolute, so give it back - but change to '/' for separators
    wxString path = noChanges.GetFullPath();
    return path;
}

wxString TypeConv::MakeRelativePath(const wxString& filename, const wxString& basePath)
{
    wxFileName fnFile(filename);
    wxFileName noChanges = fnFile;
    if (fnFile.IsAbsolute()) {
        wxFileName fnBasePath(basePath);
        if (fnBasePath.IsAbsolute()) {
            if (fnFile.MakeRelativeTo(basePath))
                return fnFile.GetFullPath(wxPATH_UNIX);
        }
    }
    // Either it is already relative, or it could not be made relative, so give it back - but change to '/' for separators
    if (noChanges.IsAbsolute()) {
        wxString path = noChanges.GetFullPath();
        return path;
    } else {
        return noChanges.GetFullPath(wxPATH_UNIX);
    }
}

void TypeConv::SplitFileSystemURL(const wxString& url, wxString* protocol, wxString* path, wxString* anchor)
{
    wxString remainder;
    if (url.StartsWith("file:", &remainder)) {
        *protocol = "file:";
    } else {
        protocol->clear();
        remainder = url;
    }
    *path = remainder.BeforeFirst('#');
    if (remainder.size() > path->size()) {
        *anchor = remainder.substr(path->size());
    } else {
        anchor->clear();
    }
}

wxString TypeConv::MakeAbsoluteURL(const wxString& url, const wxString& basePath)
{
    wxString protocol, path, anchor;
    SplitFileSystemURL(url, &protocol, &path, &anchor);
    return protocol + MakeAbsolutePath(path, basePath) + anchor;
}

wxString TypeConv::MakeRelativeURL(const wxString& url, const wxString& basePath)
{
    wxString protocol, path, anchor;
    SplitFileSystemURL(url, &protocol, &path, &anchor);
    return protocol + MakeRelativePath(path, basePath) + anchor;
}

wxSystemColour TypeConv::StringToSystemColour(const wxString& str)
{
    wxSystemColour systemVal = wxSYS_COLOUR_BTNFACE;
#define ElseIfSystemColourConvert(NAME, value) \
    else if (value == #NAME)                   \
    {                                          \
        systemVal = NAME;                      \
    }
    if (false) { }
    // clang-format off
    ElseIfSystemColourConvert(wxSYS_COLOUR_SCROLLBAR, str)
    ElseIfSystemColourConvert(wxSYS_COLOUR_BACKGROUND, str)
    ElseIfSystemColourConvert(wxSYS_COLOUR_ACTIVECAPTION, str)
    ElseIfSystemColourConvert(wxSYS_COLOUR_INACTIVECAPTION, str)
    ElseIfSystemColourConvert(wxSYS_COLOUR_MENU, str)
    ElseIfSystemColourConvert(wxSYS_COLOUR_WINDOW, str)
    ElseIfSystemColourConvert(wxSYS_COLOUR_WINDOWFRAME, str)
    ElseIfSystemColourConvert(wxSYS_COLOUR_MENUTEXT, str)
    ElseIfSystemColourConvert(wxSYS_COLOUR_WINDOWTEXT, str)
    ElseIfSystemColourConvert(wxSYS_COLOUR_CAPTIONTEXT, str)
    ElseIfSystemColourConvert(wxSYS_COLOUR_ACTIVEBORDER, str)
    ElseIfSystemColourConvert(wxSYS_COLOUR_INACTIVEBORDER, str)
    ElseIfSystemColourConvert(wxSYS_COLOUR_APPWORKSPACE, str)
    ElseIfSystemColourConvert(wxSYS_COLOUR_HIGHLIGHT, str)
    ElseIfSystemColourConvert(wxSYS_COLOUR_HIGHLIGHTTEXT, str)
    ElseIfSystemColourConvert(wxSYS_COLOUR_BTNFACE, str)
    ElseIfSystemColourConvert(wxSYS_COLOUR_BTNSHADOW, str)
    ElseIfSystemColourConvert(wxSYS_COLOUR_GRAYTEXT, str)
    ElseIfSystemColourConvert(wxSYS_COLOUR_BTNTEXT, str)
    ElseIfSystemColourConvert(wxSYS_COLOUR_INACTIVECAPTIONTEXT, str)
    ElseIfSystemColourConvert(wxSYS_COLOUR_BTNHIGHLIGHT, str)
    ElseIfSystemColourConvert(wxSYS_COLOUR_3DDKSHADOW, str)
    ElseIfSystemColourConvert(wxSYS_COLOUR_3DLIGHT, str)
    ElseIfSystemColourConvert(wxSYS_COLOUR_INFOTEXT, str)
    ElseIfSystemColourConvert(wxSYS_COLOUR_INFOBK, str) return systemVal;
    // clang-format on
#undef ElseIfSystemColourConvert
}

wxColour TypeConv::StringToColour(const wxString& str)
{
    // check for system colour
    if (!str.find_first_of("wx")) {
        return wxSystemSettings::GetColour(StringToSystemColour(str));
    } else {
        wxStringTokenizer tkz(str, ",");
        unsigned int red, green, blue;
        red = green = blue = 0;
#if 0
        bool set_red, set_green, set_blue;
        set_red = set_green = set_blue = false;
#endif
        if (tkz.HasMoreTokens()) {
            wxString s_red = tkz.GetNextToken();
            long l_red;
            if (s_red.ToLong(&l_red) && (l_red >= 0 && l_red <= 255)) {
                red = (int)l_red;
#if 0
                set_size = true;
#endif
            }
        }
        if (tkz.HasMoreTokens()) {
            wxString s_green = tkz.GetNextToken();
            long l_green;

            if (s_green.ToLong(&l_green) && (l_green >= 0 && l_green <= 255)) {
                green = (int)l_green;
#if 0
                set_size = true;
#endif
            }
        }
        if (tkz.HasMoreTokens()) {
            wxString s_blue = tkz.GetNextToken();
            long l_blue;

            if (s_blue.ToLong(&l_blue) && (l_blue >= 0 && l_blue <= 255)) {
                blue = (int)l_blue;
#if 0
                set_size = true;
#endif
            }
        }
        return wxColour(red, green, blue);
    }
}

wxString TypeConv::ColourToString(const wxColour& colour)
{
    return wxString::Format("%d,%d,%d", colour.Red(), colour.Green(), colour.Blue());
}

wxString TypeConv::SystemColourToString(long colour)
{
    wxString s;
#define SystemColourConvertCase(NAME) \
    case NAME:                        \
        s = #NAME;                    \
        break;
    // clang-format off
    switch (colour) {
    SystemColourConvertCase(wxSYS_COLOUR_SCROLLBAR)
    SystemColourConvertCase(wxSYS_COLOUR_BACKGROUND)
    SystemColourConvertCase(wxSYS_COLOUR_ACTIVECAPTION)
    SystemColourConvertCase(wxSYS_COLOUR_INACTIVECAPTION)
    SystemColourConvertCase(wxSYS_COLOUR_MENU)
    SystemColourConvertCase(wxSYS_COLOUR_WINDOW)
    SystemColourConvertCase(wxSYS_COLOUR_WINDOWFRAME)
    SystemColourConvertCase(wxSYS_COLOUR_MENUTEXT)
    SystemColourConvertCase(wxSYS_COLOUR_WINDOWTEXT)
    SystemColourConvertCase(wxSYS_COLOUR_CAPTIONTEXT)
    SystemColourConvertCase(wxSYS_COLOUR_ACTIVEBORDER)
    SystemColourConvertCase(wxSYS_COLOUR_INACTIVEBORDER)
    SystemColourConvertCase(wxSYS_COLOUR_APPWORKSPACE)
    SystemColourConvertCase(wxSYS_COLOUR_HIGHLIGHT)
    SystemColourConvertCase(wxSYS_COLOUR_HIGHLIGHTTEXT)
    SystemColourConvertCase(wxSYS_COLOUR_BTNFACE)
    SystemColourConvertCase(wxSYS_COLOUR_BTNSHADOW)
    SystemColourConvertCase(wxSYS_COLOUR_GRAYTEXT)
    SystemColourConvertCase(wxSYS_COLOUR_BTNTEXT)
    SystemColourConvertCase(wxSYS_COLOUR_INACTIVECAPTIONTEXT)
    SystemColourConvertCase(wxSYS_COLOUR_BTNHIGHLIGHT)
    SystemColourConvertCase(wxSYS_COLOUR_3DDKSHADOW)
    SystemColourConvertCase(wxSYS_COLOUR_3DLIGHT)
    SystemColourConvertCase(wxSYS_COLOUR_INFOTEXT)
    SystemColourConvertCase(wxSYS_COLOUR_INFOBK)
        // clang-format on
    }
    return s;
#undef SystemColourConvertCase
}

bool TypeConv::FlagSet(const wxString& flag, const wxString& currentValue)
{
    bool set = false;
    wxStringTokenizer tkz(currentValue, "|");
    while (!set && tkz.HasMoreTokens()) {
        wxString token;
        token = tkz.GetNextToken();
        token.Trim(true);
        token.Trim(false);
        if (token == flag)
            set = true;
    }
    return set;
}

wxString TypeConv::ClearFlag(const wxString& flag, const wxString& currentValue)
{
    if (flag == "")
        return currentValue;

    wxString result;
    wxStringTokenizer tkz(currentValue, "|");
    while (tkz.HasMoreTokens()) {
        wxString token;
        token = tkz.GetNextToken();
        token.Trim(true);
        token.Trim(false);
        if (token != flag) {
            if (result != "")
                result = result + '|';

            result = result + token;
        }
    }
    return result;
}

wxString TypeConv::SetFlag(const wxString& flag, const wxString& currentValue)
{
    if (flag == "")
        return currentValue;

    bool found = false;
    wxString result = currentValue;
    wxStringTokenizer tkz(currentValue, "|");
    while (tkz.HasMoreTokens()) {
        wxString token;
        token = tkz.GetNextToken();
        token.Trim(true);
        token.Trim(false);
        if (token == flag)
            found = true;
    }
    if (!found) {
        if (result != "")
            result = result + '|';

        result = result + flag;
    }
    return result;
}
/*
    la representación de un array de cadenas será:
    'string1' 'string2' 'string3'
    el caracter (') se representa dentro de una cadena como ('')
    'wxString''1'''
*/
wxArrayString TypeConv::OldStringToArrayString(const wxString& str)
{
    int i = 0, size = (int)str.Length(), state = 0;
    wxArrayString result;
    wxString substr;
    while (i < size) {
        wxChar c = str[i];
        switch (state) {
        case 0: // esperando (') de comienzo de cadena
            if (c == '\'')
                state = 1;
            break;
        case 1: // guardando cadena
            if (c == '\'') {
                if (i + 1 < size && str[i + 1] == '\'') {
                    substr = substr + '\''; // sustitución ('') por (') y seguimos
                    i++;
                } else {
                    result.Add(substr); // fin de cadena
                    substr.Clear();
                    state = 0;
                }
            } else
                substr = substr + c; // seguimos guardado la cadena

            break;
        }
        i++;
    }
    return result;
}

wxArrayString TypeConv::StringToArrayString(const wxString& str)
{
#if 0
    wxArrayString result = wxStringTokenize(str, ";");
#endif
    wxArrayString result;
    WX_PG_TOKENIZER2_BEGIN(str, '"')
    result.Add(token);
    WX_PG_TOKENIZER2_END()
    return result;
}

wxString TypeConv::ArrayStringToString(const wxArrayString& arrayStr)
{
    wxString result;
    wxArrayStringProperty::ArrayStringToString(result, arrayStr, '"', 1);
    return result;
}

wxString TypeConv::ReplaceSynonymous(const wxString& bitlist)
{
    wxMessageBox(_("Before: ") + bitlist);
    wxString result;
    wxString translation;
    wxStringTokenizer tkz(bitlist, "|");
    while (tkz.HasMoreTokens()) {
        wxString token;
        token = tkz.GetNextToken();
        token.Trim(true);
        token.Trim(false);
        if (result != "")
            result = result + wxChar('|');

        if (MacroDictionary::GetInstance()->SearchSynonymous(token, translation))
            result += translation;
        else
            result += token;
    }
    wxMessageBox(_("After: ") + result);
    return result;
}

wxString TypeConv::TextToString(const wxString& str)
{
    wxString result;
    for (size_t i = 0; i < str.length(); i++) {
        wxChar c = str[i];
        if (c == '\\') {
            if (i < str.length() - 1) {
                wxChar next = str[i + 1];
                switch (next) {
                case 'n':
                    result += '\n';
                    i++;
                    break;

                case 't':
                    result += '\t';
                    i++;
                    break;

                case 'r':
                    result += '\r';
                    i++;
                    break;

                case '\\':
                    result += '\\';
                    i++;
                    break;
                }
            }
        } else {
            result += c;
        }
    }
    return result;
}

wxString TypeConv::StringToText(const wxString& str)
{
    wxString result;
    for (size_t i = 0; i < str.length(); i++) {
        wxChar c = str[i];
        switch (c) {
        case '\n':
            result += "\\n";
            break;

        case '\t':
            result += "\\t";
            break;

        case '\r':
            result += "\\r";
            break;

        case '\\':
            result += "\\\\";
            break;

        default:
            result += c;
            break;
        }
    }
    return result;
}

double TypeConv::StringToFloat(const wxString& str)
{
    // Numbers are stored in "C" locale
    LocaleSwitcher switcher;
    double out;
    str.ToDouble(&out);
    return out;
}

wxString TypeConv::FloatToString(const double& val)
{
    // Numbers are stored in "C" locale
    LocaleSwitcher switcher;
    wxString convert;
    convert << val;
    return convert;
}

PMacroDictionary MacroDictionary::s_instance = nullptr;

PMacroDictionary MacroDictionary::GetInstance()
{
    if (!s_instance)
        s_instance = new MacroDictionary();

    return s_instance;
}

void MacroDictionary::Destroy()
{
    delete s_instance;
    s_instance = nullptr;
}

bool MacroDictionary::SearchMacro(wxString name, int* result)
{
    bool found = false;
    MacroMap::iterator it = m_map.find(name);
    if (it != m_map.end()) {
        found = true;
        *result = it->second;
    }
    return found;
}

bool MacroDictionary::SearchSynonymous(wxString synName, wxString& result)
{
    bool found = false;
    SynMap::iterator it = m_synMap.find(synName);
    if (it != m_synMap.end()) {
        found = true;
        result = it->second;
    }
    return found;
}

#if 0
#define MACRO(x) m_map.insert(MacroMap::value_type(#x, x))
#define MACRO2(x, y) m_map.insert(MacroMap::value_type(#x, y))
#endif

void MacroDictionary::AddMacro(wxString name, int value)
{
    m_map.insert(MacroMap::value_type(name, value));
}

void MacroDictionary::AddSynonymous(wxString synName, wxString name)
{
    m_synMap.insert(SynMap::value_type(synName, name));
}

MacroDictionary::MacroDictionary()
{
    // Las macros serán incluidas en las bibliotecas de componentes...
    // Sizers macros
}
