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
#include "utils/filetocarray.h"

#include "rad/appdata.h"
#include "model/objectbase.h"
#include "codegen/codewriter.h"
#include "codegen/cppcg.h"
#include "utils/typeconv.h"
#include "utils/exception.h"

#include <wx/filename.h>

#include <fstream>

wxString GetBitmapTypeName(long type)
{
#define CASE_BITMAP_TYPE(x) \
    case x:                 \
        return #x;

    switch (type) {
        CASE_BITMAP_TYPE(wxBITMAP_TYPE_BMP)
        CASE_BITMAP_TYPE(wxBITMAP_TYPE_ICO)
        CASE_BITMAP_TYPE(wxBITMAP_TYPE_CUR)
        CASE_BITMAP_TYPE(wxBITMAP_TYPE_XBM)
        CASE_BITMAP_TYPE(wxBITMAP_TYPE_XBM_DATA)
        CASE_BITMAP_TYPE(wxBITMAP_TYPE_XPM)
        CASE_BITMAP_TYPE(wxBITMAP_TYPE_XPM_DATA)
        CASE_BITMAP_TYPE(wxBITMAP_TYPE_TIF)
        CASE_BITMAP_TYPE(wxBITMAP_TYPE_GIF)
        CASE_BITMAP_TYPE(wxBITMAP_TYPE_PNG)
        CASE_BITMAP_TYPE(wxBITMAP_TYPE_JPEG)
        CASE_BITMAP_TYPE(wxBITMAP_TYPE_PNM)
        CASE_BITMAP_TYPE(wxBITMAP_TYPE_PCX)
        CASE_BITMAP_TYPE(wxBITMAP_TYPE_PICT)
        CASE_BITMAP_TYPE(wxBITMAP_TYPE_ICON)
        CASE_BITMAP_TYPE(wxBITMAP_TYPE_ANI)
        CASE_BITMAP_TYPE(wxBITMAP_TYPE_IFF)
        CASE_BITMAP_TYPE(wxBITMAP_TYPE_TGA)
        CASE_BITMAP_TYPE(wxBITMAP_TYPE_MACCURSOR)
    default:
        return "wxBITMAP_TYPE_ANY";
    };
#undef CASE_BITMAP_TYPE
}

wxString GetBitmapType(const wxFileName& sourceFileName)
{
    wxImageHandler* handler
        = wxImage::FindHandler(sourceFileName.GetExt(), wxBITMAP_TYPE_ANY);
    if (handler)
        return GetBitmapTypeName(handler->GetType());
    else
        return "wxBITMAP_TYPE_ANY";
}

wxString FileToCArray::Generate(const wxString& sourcePath)
{
    wxFileName sourceFileName(sourcePath);
    const wxString& sourceFullName = sourceFileName.GetFullName();
    const wxString targetFullName = sourceFullName + ".h";
    wxString arrayName = CppCodeGenerator::ConvertEmbeddedBitmapName(sourcePath);

    if (!sourceFileName.FileExists()) {
        wxLogWarning(sourcePath + " does not exist");
        return targetFullName;
    }
    PObjectBase project = AppData()->GetProjectData();

    // Get the output path
    wxString outputPath;
    wxString embeddedFilesOutputPath;
    try {
        outputPath = AppData()->GetOutputPath();
        embeddedFilesOutputPath = AppData()->GetEmbeddedFilesOutputPath();
    } catch (wxWeaverException& ex) {
        wxLogWarning(ex.what());
        return targetFullName;
    }
    // Determine if Microsoft BOM should be used
    bool useMicrosoftBOM = false;
    PProperty pUseMicrosoftBOM = project->GetProperty("use_microsoft_bom");
    if (pUseMicrosoftBOM)
        useMicrosoftBOM = (pUseMicrosoftBOM->GetValueAsInteger());

    // Determine if UTF8 or ANSI is to be created
    bool useUtf8 = false;
    PProperty pUseUtf8 = project->GetProperty(_("encoding"));

    if (pUseUtf8)
        useUtf8 = (pUseUtf8->GetValueAsString() != "ANSI");

    // setup output file
    PCodeWriter arrayCodeWriter(
        new FileCodeWriter(
            embeddedFilesOutputPath + targetFullName, useMicrosoftBOM, useUtf8));

    const wxString headerGuardName = arrayName.Upper() + "_H";
    arrayCodeWriter->WriteLn("#ifndef " + headerGuardName);
    arrayCodeWriter->WriteLn("#define " + headerGuardName);
    arrayCodeWriter->WriteLn();
    arrayCodeWriter->WriteLn("#include <wx/mstream.h>");
    arrayCodeWriter->WriteLn("#include <wx/image.h>");
    arrayCodeWriter->WriteLn("#include <wx/bitmap.h>");
    arrayCodeWriter->WriteLn();
    arrayCodeWriter->WriteLn("static const unsigned char " + arrayName + "[] = ");
    arrayCodeWriter->WriteLn("{");
    arrayCodeWriter->Indent();

    unsigned int count = 1;
    const unsigned int bytesPerLine = 10;
    std::ifstream binFile(
        static_cast<const char*>(sourcePath.mb_str(wxConvFile)), std::ios::binary);

    for (std::istreambuf_iterator<char> byte(binFile), end; byte != end; ++byte, ++count) {
        arrayCodeWriter->Write(
            wxString::Format(
                "0x%02X, ", static_cast<unsigned int>(static_cast<unsigned char>(*byte))));

        if (count >= bytesPerLine) {
            arrayCodeWriter->WriteLn();
            count = 0;
        }
    }
    if ((count < bytesPerLine) && (count != 1))
        arrayCodeWriter->WriteLn();

    arrayCodeWriter->Unindent();
    arrayCodeWriter->WriteLn("};");
    arrayCodeWriter->WriteLn();
    arrayCodeWriter->WriteLn("wxBitmap& " + arrayName + "_to_wx_bitmap()");
    arrayCodeWriter->WriteLn("{");
    arrayCodeWriter->Indent();
    arrayCodeWriter->WriteLn("static wxMemoryInputStream memIStream( "
                             + arrayName + ", sizeof( "
                             + arrayName + " ) );");
    arrayCodeWriter->WriteLn("static wxImage image( memIStream, "
                             + GetBitmapType(sourceFileName) + " );");
    arrayCodeWriter->WriteLn("static wxBitmap bmp( image );");
    arrayCodeWriter->WriteLn("return bmp;");
    arrayCodeWriter->Unindent();
    arrayCodeWriter->WriteLn("}");
    arrayCodeWriter->WriteLn();
    arrayCodeWriter->WriteLn();
    arrayCodeWriter->WriteLn("#endif //" + headerGuardName);

    return TypeConv::MakeRelativePath(
        embeddedFilesOutputPath + targetFullName, outputPath);
}
