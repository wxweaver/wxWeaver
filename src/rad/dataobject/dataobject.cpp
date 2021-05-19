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
#include "rad/dataobject/dataobject.h"

#include "model/objectbase.h"
#include "utils/typeconv.h"
#include "rad/appdata.h"

#include <ticpp.h>

wxWeaverDataObject::wxWeaverDataObject(PObjectBase obj)
{
    if (obj) {
        // create xml representation of ObjectBase
        ticpp::Element element;
        obj->SerializeObject(&element);

        // add version info to xml data,
        // just in case it is pasted into a different version of wxWeaver
        element.SetAttribute("fbp_version_major", AppData()->m_fbpVerMajor);
        element.SetAttribute("fbp_version_minor", AppData()->m_fbpVerMinor);

        ticpp::Document doc;
        doc.LinkEndChild(&element);
        TiXmlPrinter printer;
        printer.SetIndent("\t");

        printer.SetLineBreak("\n");

        doc.Accept(&printer);
        m_data = printer.Str();
    }
}

void wxWeaverDataObject::GetAllFormats(wxDataFormat* formats, Direction dir) const
{
    switch (dir) {
    case Get:
        formats[0] = wxWeaverDataObjectFormat;
        formats[1] = wxDF_TEXT;
        break;
    case Set:
        formats[0] = wxWeaverDataObjectFormat;
        break;
    default:
        break;
    }
}

bool wxWeaverDataObject::GetDataHere(const wxDataFormat&, void* buf) const
{
    if (!buf)
        return false;

    memcpy((char*)buf, m_data.c_str(), m_data.length());
    return true;
}

size_t wxWeaverDataObject::GetDataSize(const wxDataFormat& /*format*/) const
{
    return m_data.length();
}

size_t wxWeaverDataObject::GetFormatCount(Direction dir) const
{
    switch (dir) {
    case Get:
        return 2;
    case Set:
        return 1;
    default:
        return 0;
    }
}

wxDataFormat wxWeaverDataObject::GetPreferredFormat(Direction /*dir*/) const
{
    return wxWeaverDataObjectFormat;
}

bool wxWeaverDataObject::SetData(const wxDataFormat& format,
                                 size_t len, const void* buf)
{
    if (format != wxWeaverDataObjectFormat)
        return false;

    m_data.assign(reinterpret_cast<const char*>(buf), len);
    return true;
}

PObjectBase wxWeaverDataObject::GetObj()
{
    if (m_data.empty())
        return PObjectBase();

    try { // Read Object from xml
        ticpp::Document doc;
        doc.Parse(m_data, true, TIXML_ENCODING_UTF8);
        ticpp::Element* element = doc.FirstChildElement();

        int major, minor;
        element->GetAttribute("fbp_version_major", &major);
        element->GetAttribute("fbp_version_minor", &minor);

        if (major > AppData()->m_fbpVerMajor
            || (AppData()->m_fbpVerMajor == major
                && minor > AppData()->m_fbpVerMinor))
            wxLogError(_("This object cannot be pasted because it is from a newer version of wxWeaver"));

        if (major < AppData()->m_fbpVerMajor
            || (AppData()->m_fbpVerMajor == major
                && minor < AppData()->m_fbpVerMinor)) {
            AppData()->ConvertObject(element, major, minor);
        }

        PObjectDatabase db = AppData()->GetObjectDatabase();
        return db->CreateObject(element);
    } catch (ticpp::Exception& ex) {
        wxLogError(_WXSTR(ex.m_details));
        return PObjectBase();
    }
}
