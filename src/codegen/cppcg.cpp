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
#include "codegen/cppcg.h"

#include "model/objectbase.h"
#include "rad/appdata.h"
#include "utils/filetocarray.h"
#include "utils/typeconv.h"
#include "utils/exception.h"
#include "codegen/codewriter.h"

#include <wx/filename.h>
#include <wx/tokenzr.h>

#include <algorithm>

// TODO: wxStrings by value

CppTemplateParser::CppTemplateParser(PObjectBase obj, wxString _template,
                                     bool useI18N, bool useRelativePath,
                                     wxString basePath)
    : TemplateParser(obj, _template)
    , m_basePath(basePath)
    , m_useI18n(useI18N)
    , m_useRelativePath(useRelativePath)
{
    if (!wxFileName::DirExists(m_basePath))
        m_basePath.clear();
}

CppTemplateParser::CppTemplateParser(const CppTemplateParser& other,
                                     wxString _template)
    : TemplateParser(other, _template)
    , m_basePath(other.m_basePath)
    , m_useI18n(other.m_useI18n)
    , m_useRelativePath(other.m_useRelativePath)
{
}

wxString CppTemplateParser::RootWxParentToCode()
{
    return "this";
}

PTemplateParser CppTemplateParser::CreateParser(const TemplateParser* oldparser,
                                                wxString _template)
{
    const CppTemplateParser* cppOldParser
        = dynamic_cast<const CppTemplateParser*>(oldparser);
    if (cppOldParser) {
        PTemplateParser newparser(new CppTemplateParser(*cppOldParser, _template));
        return newparser;
    }
    return PTemplateParser();
}

wxString CppTemplateParser::ValueToCode(PropertyType type, wxString value)
{
    wxString result;

    switch (type) {
    case PT_WXPARENT: {
        result = value;
        break;
    }
    case PT_WXPARENT_SB: {
        result = value + "->GetStaticBox()";
        break;
    }
    case PT_WXPARENT_CP: {
        result = value + "->GetPane()";
        break;
    }
    case PT_WXSTRING:
    case PT_FILE:
    case PT_PATH: {
        if (value.empty())
            result << "wxEmptyString";
        else
            result << "\"" << CppCodeGenerator::ConvertCppString(value) << "\"";

        break;
    }
    case PT_WXSTRING_I18N: {
        if (value.empty()) {
            result << "wxEmptyString";
        } else {
            if (m_useI18n)
                result << "_(\"" << CppCodeGenerator::ConvertCppString(value) << "\")";
            else
                result << "\"" << CppCodeGenerator::ConvertCppString(value) << "\"";
        }
        break;
    }
    case PT_CLASS:
    case PT_MACRO:
    case PT_TEXT:
    case PT_OPTION:
    case PT_EDIT_OPTION:
    case PT_FLOAT:
    case PT_INT:
    case PT_UINT: {
        result = value;
        break;
    }
    case PT_BITLIST: {
        result = (value.empty() ? "0" : value);
        break;
    }
    case PT_WXPOINT: {
        if (value.empty())
            result = "wxDefaultPosition";
        else
            result << "wxPoint(" << value << ")";

        break;
    }
    case PT_WXSIZE: {
        if (value.empty())
            result = "wxDefaultSize";
        else
            result << "wxSize(" << value << ")";

        break;
    }
    case PT_BOOL: {
        result = (value == "0" ? "false" : "true");
        break;
    }
    case PT_WXFONT: {
        if (!value.empty()) {
            wxFontContainer fontContainer = TypeConv::StringToFont(value);
            wxFont font = fontContainer.GetFont();

            const int pointSize = fontContainer.GetPointSize();

            result = wxString::Format(
                "wxFont(%s, %s, %s, %s, %s, %s)",
                ((pointSize <= 0)
                     ? "wxNORMAL_FONT->GetPointSize()"
                     : (wxString() << pointSize)),
                TypeConv::FontFamilyToString(fontContainer.GetFamily()),
                font.GetStyleString(),
                font.GetWeightString(),
                (fontContainer.GetUnderlined() ? "true" : "false"),
                (fontContainer.m_faceName.empty()
                     ? "wxEmptyString"
                     : ("\"" + fontContainer.m_faceName + "\"")));
        } else {
            result = "*wxNORMAL_FONT";
        }
        break;
    }
    case PT_WXCOLOUR: {
        if (!value.empty()) {
            if (!value.find_first_of("wx")) {
                result << "wxSystemSettings::GetColour(" << value << ")"; // System Colour
            } else {
                wxColour colour = TypeConv::StringToColour(value);
                result = wxString::Format(
                    "wxColour(%i, %i, %i)",
                    colour.Red(), colour.Green(), colour.Blue());
            }
        } else {
            result = "wxColour()";
        }
        break;
    }
    case PT_BITMAP: {
        wxString path;
        wxString source;
        wxSize icoSize;
        TypeConv::ParseBitmapWithResource(value, &path, &source, &icoSize);

        if (path.empty()) {
            // Empty path, generate Null Bitmap
            result = "wxNullBitmap";
            break;
        }
        if (path.StartsWith("file:")) {
            wxLogWarning(
                "C++ code generation does not support using URLs for bitmap properties:\n%s",
                path.c_str());
            result = "wxNullBitmap";
            break;
        }
        if (source == _("Load From File")) {
            wxString absPath;
            try {
                absPath = TypeConv::MakeAbsolutePath(path, AppData()->GetProjectPath());
            } catch (wxWeaverException& ex) {
                wxLogError(ex.what());
                result = "wxNullBitmap";
                break;
            }
            wxString file
                = (m_useRelativePath
                       ? TypeConv::MakeRelativePath(absPath, m_basePath)
                       : absPath);

            wxFileName bmpFileName(path);
            if (bmpFileName.GetExt().Upper() == "XPM") {
                // If the bitmap is an XPM we will embed it in the code,
                // otherwise it will be loaded from the file at run time.
                result << "wxBitmap("
                       << CppCodeGenerator::ConvertEmbeddedBitmapName(path)
                       << ")";
            } else {
                result << "wxBitmap(\""
                       << CppCodeGenerator::ConvertCppString(file)
                       << "\", wxBITMAP_TYPE_ANY)";
            }
        } else if (source == _("Load From Embedded File")) {
            wxString absPath;
            try {
                absPath = TypeConv::MakeAbsolutePath(
                    path, AppData()->GetProjectPath());
            } catch (wxWeaverException& ex) {
                wxLogError(ex.what());
                result = "wxNullBitmap";
                break;
            }

            wxFileName bmpFileName(path);
            if (bmpFileName.GetExt().Upper() == "XPM") {
                // If the bitmap is an XPM we will embed it as is, otherwise we'll generate a header
                result << "wxBitmap("
                       << CppCodeGenerator::ConvertEmbeddedBitmapName(path)
                       << ")";
            } else {
                result << CppCodeGenerator::ConvertEmbeddedBitmapName(path)
                       << "_to_wx_bitmap()";
            }
        } else if (source == _("Load From Resource")) {
            result << "wxBitmap(\""
                   << path
                   << "\", wxBITMAP_TYPE_RESOURCE)";
        } else if (source == _("Load From Icon Resource")) {
            if (wxDefaultSize == icoSize) {
                result << "wxICON("
                       << path
                       << ")";
            } else {
                result.Printf(
                    "wxIcon(\"%s\", wxBITMAP_TYPE_ICO_RESOURCE, %i, %i)"),
                    path.c_str(), icoSize.GetWidth(),
                    icoSize.GetHeight();
            }
        } else if (source == _("Load From XRC")) {
            result << "wxXmlResource::Get()->LoadBitmap(\""
                   << path
                   << "\")";
        } else if (source == _("Load From Art Provider")) {
            wxString rid = path.BeforeFirst(':');
            if (rid.StartsWith("gtk-"))
                rid = "\"" + rid + "\"";

            result = "wxArtProvider::GetBitmap(" + rid
                + ", " + path.AfterFirst(':') + ")";
        }
        break;
    }
    case PT_STRINGLIST: {
        // Stringlists are generated like a sequence of wxString separated by ', '.
        wxArrayString array = TypeConv::StringToArrayString(value);
        if (array.Count() > 0)
            result = ValueToCode(PT_WXSTRING_I18N, array[0]);

        for (size_t i = 1; i < array.Count(); i++)
            result << ", " << ValueToCode(PT_WXSTRING_I18N, array[i]);

        break;
    }
    default:
        break;
    }
    return result;
}

CppCodeGenerator::CppCodeGenerator()
{
    SetupPredefinedMacros();
    m_useRelativePath = false;
    m_useI18n = false;
    m_firstID = 1000;
}

wxString CppCodeGenerator::ConvertCppString(wxString text)
{
    wxString result;

    for (size_t i = 0; i < text.length(); i++) {
        wxChar c = text[i];

        switch (c) {
        case '"':
            result += "\\\"";
            break;

        case '\\':
            result += "\\\\";
            break;

        case '\t':
            result += "\\t";
            break;

        case '\n':
            result += "\\n";
            break;

        case '\r':
            result += "\\r";
            break;

        default:
            result += c;
            break;
        }
    }
    return result;
}

wxString CppCodeGenerator::ConvertEmbeddedBitmapName(const wxString& text)
{
    wxString name = text;
    // the name consists of extracting the name of the file (without the directory)
    // and replacing the character '.' by ' _ '.

    size_t last_slash = name.find_last_of("\\/");
    if (last_slash != name.npos)
        name = name.substr(last_slash + 1);

    name.Replace(".", "_");

    return name;
}

void CppCodeGenerator::GenerateInheritedClass(PObjectBase userClasses,
                                              PObjectBase form)
{
    if (!userClasses) {
        wxLogError("There is no object to generate inherited class");
        return;
    }
    if ("UserClasses" != userClasses->GetClassName()) {
        wxLogError("This not a UserClasses object");
        return;
    }
    m_inheritedCodeParser.ParseCFiles(userClasses->GetPropertyAsString("name"));

    //(FileCodeWriter*)m_header->
    wxString type = userClasses->GetPropertyAsString("type");
    wxString userCode;

    // Start header file
    wxString code = GetCode(userClasses, "guard_macro_open");
    m_header->WriteLn(code);
    m_header->WriteLn(wxEmptyString);

    code = GetCode(userClasses, "header_comment");
    m_header->WriteLn(code);
    m_header->WriteLn(wxEmptyString);

    code = GetCode(userClasses, "header_include");
    m_header->WriteLn(code);
    m_header->WriteLn(wxEmptyString);
    m_header->WriteLn("//// end generated include");

    m_header->WriteLn(m_inheritedCodeParser.GetUserIncludes());
    if (!userCode.IsEmpty())
        m_header->WriteLn(userCode);

    code = GetCode(userClasses, "class_decl");
    m_header->WriteLn(code);
    m_header->WriteLn("{");
    m_header->Indent();

    // Start source file
    code = GetCode(userClasses, "source_include");
    m_source->WriteLn(code);
    m_source->WriteLn(wxEmptyString);

    code = GetCode(userClasses, type + "_cons_def");
    LogDebug(code + "<");
    m_source->WriteLn(code);
    m_source->WriteLn("{");

    userCode = m_inheritedCodeParser.GetFunctionContents(code);
    if (!userCode.IsEmpty())
        m_source->WriteLn(userCode, true);
    else
        m_source->WriteLn(wxEmptyString);

    m_source->WriteLn("}");

    // Do events in both files
    EventVector events;
    FindEventHandlers(form, events);

    if (events.size() > 0) {
        m_header->WriteLn("protected:");
        m_header->Indent();
        code = GetCode(userClasses, "event_handler_comment");
        m_header->WriteLn(code);

        wxString className = userClasses->GetPropertyAsString("name");
        std::set<wxString> generatedHandlers;
        for (size_t i = 0; i < events.size(); i++) {
            PEvent event = events[i];
            wxString prototype;
            if (generatedHandlers.find(event->GetValue()) == generatedHandlers.end()) {
                prototype = wxString::Format(
                    "%s(%s& event)", event->GetValue().c_str(),
                    event->GetEventInfo()->GetEventClassName().c_str());
                m_header->WriteLn(wxString::Format("void %s;", prototype.c_str()));

                userCode = m_inheritedCodeParser.GetFunctionDocumentation(
                    event->GetValue());

                if (!userCode.IsEmpty())
                    m_source->Write(userCode);
                else
                    m_source->WriteLn();

                m_source->WriteLn(wxString::Format(
                    "void %s::%s", className.c_str(), prototype.c_str()));
                m_source->WriteLn("{");
                userCode = m_inheritedCodeParser.GetFunctionContents(
                    wxString::Format("void %s::%s",
                                     className.c_str(),
                                     prototype.c_str()));
                LogDebug(wxString::Format(
                             "void %s::%s", className.c_str(),
                             prototype.c_str())
                         + "<");

                if (!userCode.IsEmpty()) {
                    m_source->WriteLn(userCode);
                } else {
                    m_source->Indent();
                    m_source->WriteLn(
                        wxString::Format(
                            "// TODO: Implement %s", event->GetValue().c_str()),
                        true);
                    m_source->Unindent();
                }
                m_source->WriteLn("}");
                generatedHandlers.insert(event->GetValue());
            }
        }
        m_header->Unindent();
    }
    // Finish header file
    m_header->WriteLn("public:");
    m_header->Indent();

    code = GetCode(userClasses, type + "_cons_decl");
    m_header->WriteLn(code);
    m_header->Unindent();
    m_header->WriteLn("//// end generated class members");

    userCode = m_inheritedCodeParser.GetUserMembers();
    if (!userCode.IsEmpty())
        m_header->WriteLn(userCode, true);
    else
        m_header->WriteLn(wxEmptyString);

    m_header->Unindent();
    m_header->WriteLn("};");
    m_header->WriteLn(wxEmptyString);

    code = GetCode(userClasses, "guard_macro_close");
    m_header->WriteLn(code);

    userCode = m_inheritedCodeParser.GetRemainingFunctions();
    if (!userCode.IsEmpty())
        m_source->Write(userCode);

    userCode = m_inheritedCodeParser.GetTrailingCode();
    if (!userCode.IsEmpty())
        m_source->WriteLn(userCode, true);
}

bool CppCodeGenerator::GenerateCode(PObjectBase project)
{
    if (!project) {
        wxLogError("There is no project to generate code");
        return false;
    }
    bool useEnum = false;
    PProperty useEnumProperty = project->GetProperty("use_enum");

    if (useEnumProperty && useEnumProperty->GetValueAsInteger())
        useEnum = true;

    m_useArrayEnum = false;
    const auto& useArrayEnumProperty = project->GetProperty("use_array_enum");
    if (useArrayEnumProperty && useArrayEnumProperty->GetValueAsInteger())
        m_useArrayEnum = true;

    m_useI18n = false;
    PProperty i18nProperty = project->GetProperty("internationalize");
    if (i18nProperty && i18nProperty->GetValueAsInteger())
        m_useI18n = true;

    m_useConnect = (project->GetPropertyAsString("event_generation") != "table");
    m_disconnectEvents = (project->GetPropertyAsInteger("disconnect_events"));

    m_header->Clear();
    m_source->Clear();

    wxString code = wxString::Format(
        "/*\n"
        "    C++ code generated with wxWeaver (version %s%s " __DATE__ ")\n"
        "    https://wxweaver.github.io/\n"
        "\n"
        "    PLEASE DO *NOT* EDIT THIS FILE!\n"
        "*/",
        VERSION, REVISION);

    m_header->WriteLn(code, true);
    m_source->WriteLn(code, true);

    PProperty propFile = project->GetProperty("file");
    if (!propFile) {
        wxLogError("Missing \"file\" property on Project Object");
        return false;
    }
    wxString file = propFile->GetValue();
    if (file.empty())
        file = "noname";

    m_header->WriteLn("#pragma once");
    m_header->WriteLn(wxEmptyString);

    code = GetCode(project, "header_preamble");
    if (!code.empty())
        m_header->WriteLn(code);

    // Generate the subclass sets
    std::set<wxString> subclasses;
    std::set<wxString> subclassSourceIncludes;
    std::vector<wxString> headerIncludes;

    GenSubclassSets(project, &subclasses, &subclassSourceIncludes, &headerIncludes);

    // Write the forward declaration lines
    std::set<wxString>::iterator subclassIt;
    for (subclassIt = subclasses.begin(); subclassIt != subclasses.end(); ++subclassIt)
        m_header->WriteLn(*subclassIt);

    if (!subclasses.empty())
        m_header->WriteLn(wxEmptyString);

    // Generating in the .h header file those include from components dependencies.
    std::set<wxString> templates;
    GenIncludes(project, &headerIncludes, &templates);

    // Write the include lines
    std::vector<wxString>::iterator includeIt;
    for (includeIt = headerIncludes.begin();
         includeIt != headerIncludes.end(); ++includeIt) {
        m_header->WriteLn(*includeIt);
    }
    if (!headerIncludes.empty())
        m_header->WriteLn(wxEmptyString);

    // class decoration
    PProperty propClassDecoration = project->GetProperty("class_decoration");
    wxString classDecoration;
    if (propClassDecoration) {
        // get the decoration to be used by GenClassDeclaration
        std::map<wxString, wxString> children;
        propClassDecoration->SplitParentProperty(&children);

        std::map<wxString, wxString>::iterator decoration;
        decoration = children.find("decoration");

        if (decoration != children.end()) {
            classDecoration = decoration->second;
            if (!classDecoration.empty())
                classDecoration += " ";
        }
        // Now get the header
        std::map<wxString, wxString>::iterator header;
        header = children.find("header");

        if (header != children.end()) {
            wxString headerVal = header->second;
            if (!headerVal.empty()) {
                wxString include = "#include \"" + headerVal + "\"";
                std::vector<wxString>::iterator findInclude
                    = std::find(headerIncludes.begin(), headerIncludes.end(), include);
                if (findInclude == headerIncludes.end()) {
                    m_header->WriteLn(include);
                    m_header->WriteLn(wxEmptyString);
                }
            }
        }
    }
    code = GetCode(project, "header_epilogue");
    m_header->WriteLn(code);
    m_header->WriteLn(wxEmptyString);

    // Inserting in the .cpp source file the include corresponding to the
    // generated .h header file and the related xpm includes
    code = GetCode(project, "cpp_preamble");
    if (!code.empty()) {
        m_source->WriteLn(code);
        m_source->WriteLn(wxEmptyString);
    }
    // Write include lines for subclasses
    for (subclassIt = subclassSourceIncludes.begin();
         subclassIt != subclassSourceIncludes.end(); ++subclassIt)
        m_source->WriteLn(*subclassIt);

    if (!subclassSourceIncludes.empty())
        m_source->WriteLn(wxEmptyString);

    // Generated header
    m_source->WriteLn("#include \"" + file + ".h\"");
    m_source->WriteLn(wxEmptyString);
    GenEmbeddedBitmapIncludes(project);

    code = GetCode(project, "cpp_epilogue");
    m_source->WriteLn(code);

    // namespace
    PProperty propNamespace = project->GetProperty("namespace");
    wxArrayString namespaceArray;
    if (propNamespace) {
        namespaceArray = propNamespace->GetValueAsArrayString();
        wxString usingNamespaceStr;
        for (size_t i = 0; i < namespaceArray.Count(); ++i) {
            m_header->WriteLn("namespace " + namespaceArray[i]);
            m_header->WriteLn("{");
            m_header->Indent();

            if (usingNamespaceStr.empty())
                usingNamespaceStr = "using namespace ";
            else
                usingNamespaceStr += "::";

            usingNamespaceStr += namespaceArray[i];
        }
        if (namespaceArray.Count() && !usingNamespaceStr.empty()) {
            usingNamespaceStr += ';';
            m_source->WriteLn(usingNamespaceStr);
        }
    }
    // Generating "defines" for macros
    if (!useEnum)
        GenDefines(project);

    for (size_t i = 0; i < project->GetChildCount(); i++) {
        PObjectBase child = project->GetChild(i);

        // Preprocess to find arrays
        ArrayItems arrays;
        FindArrayObjects(child, arrays, true);

        EventVector events;
        FindEventHandlers(child, events);
        GenClassDeclaration(child, useEnum, classDecoration, events, arrays);
        if (!m_useConnect)
            GenEvents(child, events);

        GenConstructor(child, events, arrays);
        GenDestructor(child, events);
    }
    // namespace
    if (namespaceArray.Count() > 0) {
        for (size_t i = namespaceArray.Count(); i > 0; --i) {
            m_header->Unindent();
            m_header->WriteLn("} // namespace " + namespaceArray[i - 1]);
        }
        m_header->WriteLn(wxEmptyString);
    }
    return true;
}

void CppCodeGenerator::GenEvents(PObjectBase classObj, const EventVector& events,
                                 bool disconnect)
{
    if (events.empty())
        return;

    PProperty propName = classObj->GetProperty("name");
    if (!propName) {
        wxLogError(
            "Missing \"name\" property on \"%s\" class. Review your XML object description",
            classObj->GetClassName().c_str());
        return;
    }
    wxString className = propName->GetValue();
    if (className.empty()) {
        wxLogError("Object name cannot be null");
        return;
    }
    wxString base_class;
    PProperty propSubclass = classObj->GetProperty("subclass");
    if (propSubclass) {
        wxString subclass = propSubclass->GetChildFromParent("name");
        if (!subclass.empty())
            base_class = subclass;
    }
    if (base_class.empty())
        base_class = "wx" + classObj->GetClassName();

    if (events.size() > 0) {
        if (!m_useConnect) {
            m_source->WriteLn();
            m_source->WriteLn("wxBEGIN_EVENT_TABLE("
                              + className + ", " + base_class + ")");
            m_source->Indent();
        }
        for (size_t i = 0; i < events.size(); i++) {
            PEvent event = events[i];
            wxString handlerName;
            if (m_useConnect) {
                handlerName.Printf("%sHandler(%s::%s)",
                                   event->GetEventInfo()->GetEventClassName().c_str(),
                                   className.c_str(), event->GetValue().c_str());
            } else {
                handlerName.Printf("%s::_wxWEAVER_%s",
                                   className.c_str(),
                                   event->GetValue().c_str());
            }
            wxString templateName = wxString::Format(
                "%s_%s", m_useConnect ? "connect" : "entry",
                event->GetName().c_str());

            PObjectBase obj = event->GetObject();
            if (!GenEventEntry(obj, obj->GetObjectInfo(),
                               templateName, handlerName, disconnect)
                && (!disconnect || event->GetName() != "OnMenuSelection")) {
                // We don't bother with disconnecting OnMenuSelection events,
                // that's why we skip the error message in that case.
                wxLogError("Missing \"evt_%s\" template for \"%s\" class."
                           "Review your XML object description",
                           templateName.c_str(), className.c_str());
            }
        }
        if (!m_useConnect) {
            m_source->Unindent();
            m_source->WriteLn("wxEND_EVENT_TABLE()");
        }
    }
}

bool CppCodeGenerator::GenEventEntry(PObjectBase obj, PObjectInfo obj_info,
                                     const wxString& templateName,
                                     const wxString& handlerName, bool disconnect)
{
    wxString _template;
    PCodeInfo codeInfo = obj_info->GetCodeInfo("C++");
    if (codeInfo) {
        _template = codeInfo->GetTemplate(
            wxString::Format(
                "evt_%s%s",
                disconnect ? wxS("dis") : wxEmptyString, templateName.c_str()));

        if (disconnect && _template.empty()) {
            _template = codeInfo->GetTemplate("evt_" + templateName);
            if (!_template.Replace("Connect", "Disconnect", true))
                _template.clear(); // "Connect" wasn't found, skip this entry
        }
        if (!_template.empty()) {
            _template.Replace("#handler", handlerName.c_str()); // TODO: ugly patch!
            CppTemplateParser parser(obj, _template,
                                     m_useI18n, m_useRelativePath, m_basePath);
            m_source->WriteLn(parser.ParseTemplate());
            return true;
        }
    }
    for (size_t i = 0; i < obj_info->GetBaseClassCount(false); i++) {
        PObjectInfo base_info = obj_info->GetBaseClass(i, false);
        if (GenEventEntry(obj, base_info, templateName, handlerName, disconnect))
            return true;
    }
    return false;
}

void CppCodeGenerator::GenPrivateEventHandlers(const EventVector& events)
{
    if (events.size() > 0) {
        m_header->WriteLn(wxEmptyString);
        m_header->Indent();
        m_header->WriteLn("// Private event handlers");

        std::set<wxString> generatedHandlers;

        for (size_t i = 0; i < events.size(); i++) {
            PEvent event = events[i];
            wxString aux;

            if (generatedHandlers.find(event->GetValue()) == generatedHandlers.end()) {
                aux = "void _wxWEAVER_" + event->GetValue()
                    + "(" + event->GetEventInfo()->GetEventClassName()
                    + "& event){ " + event->GetValue() + "(event); }";

                m_header->WriteLn(aux);
                generatedHandlers.insert(event->GetValue());
            }
        }
        m_header->Unindent();
    }
}

void CppCodeGenerator::GenVirtualEventHandlers(const EventVector& events,
                                               const wxString& eventHandlerPrefix,
                                               const wxString& eventHandlerPostfix)
{
    if (events.size() > 0) {
        /*
            There are problems if we create "pure" virtual handlers,
            because some events could be triggered in the constructor
            in which virtual methods are executed properly.
            So we create a default handler which will skip the event.
        */
        m_header->WriteLn(wxEmptyString);
        m_header->WriteLn(
            "// Virtual event handlers, override them in your derived class");

        std::set<wxString> generatedHandlers;
        for (size_t i = 0; i < events.size(); i++) {
            PEvent event = events[i];
            wxString aux = eventHandlerPrefix
                + "void " + event->GetValue() + "("
                + event->GetEventInfo()->GetEventClassName()
                + "& event)" + eventHandlerPostfix;

            if (generatedHandlers.find(aux) == generatedHandlers.end()) {
                m_header->WriteLn(aux);
                generatedHandlers.insert(aux);
            }
        }
    }
}

void CppCodeGenerator::GenAttributeDeclaration(PObjectBase obj, Permission perm,
                                               ArrayItems& arrays)
{
    wxString typeName = obj->GetTypeName();
    if (ObjectDatabase::HasCppProperties(typeName)) {
        wxString permStr = obj->GetProperty("permission")->GetValue();

        if ((perm == P_PUBLIC && permStr == "public")
            || (perm == P_PROTECTED && permStr == "protected")
            || (perm == P_PRIVATE && permStr == "private")) {
            const auto& code = GetDeclaration(obj, arrays, perm != P_PRIVATE && m_useArrayEnum);
            if (!code.empty())
                m_header->WriteLn(code);
        }
    }
    // Generate recursively the rest of the attributes
    for (size_t i = 0; i < obj->GetChildCount(); i++) {
        PObjectBase child = obj->GetChild(i);
        GenAttributeDeclaration(child, perm, arrays);
    }
}
void CppCodeGenerator::GenValidatorVariables(PObjectBase obj)
{
    GenValVarsBase(obj->GetObjectInfo(), obj);

    // Proceeding recursively with the children
    for (size_t i = 0; i < obj->GetChildCount(); i++) {
        PObjectBase child = obj->GetChild(i);
        GenValidatorVariables(child);
    }
}

void CppCodeGenerator::GenValVarsBase(PObjectInfo info, PObjectBase obj)
{
    PCodeInfo codeInfo = info->GetCodeInfo("C++");
    if (!codeInfo)
        return;

    wxString _template = codeInfo->GetTemplate("valvar_declaration");
    if (!_template.empty()) {
        CppTemplateParser parser(obj, _template,
                                 m_useI18n, m_useRelativePath, m_basePath);
        wxString code = parser.ParseTemplate();
        if (!code.empty())
            m_header->WriteLn(code);
    }
    // Proceeding recursively with the base classes
    for (size_t i = 0; i < info->GetBaseClassCount(false); i++) {
        PObjectInfo base_info = info->GetBaseClass(i, false);
        GenValVarsBase(base_info, obj);
    }
}

void CppCodeGenerator::GetGenEventHandlers(PObjectBase obj)
{
    GenDefinedEventHandlers(obj->GetObjectInfo(), obj);

    // Process child widgets
    for (size_t i = 0; i < obj->GetChildCount(); i++) {
        PObjectBase child = obj->GetChild(i);
        GetGenEventHandlers(child);
    }
}

void CppCodeGenerator::GenDefinedEventHandlers(PObjectInfo info, PObjectBase obj)
{
    PCodeInfo codeInfo = info->GetCodeInfo("C++");
    if (!codeInfo)
        return;

    wxString _template = codeInfo->GetTemplate("generated_event_handlers");
    if (!_template.empty()) {
        CppTemplateParser parser(obj, _template,
                                 m_useI18n, m_useRelativePath, m_basePath);
        wxString code = parser.ParseTemplate();
        if (!code.empty())
            m_header->WriteLn(code);
    }
    // Proceeding recursively with the base classes
    for (size_t i = 0; i < info->GetBaseClassCount(false); i++) {
        PObjectInfo base_info = info->GetBaseClass(i, false);
        GenDefinedEventHandlers(base_info, obj);
    }
}

wxString CppCodeGenerator::GetCode(PObjectBase obj, wxString name)
{
    PCodeInfo codeInfo = obj->GetObjectInfo()->GetCodeInfo("C++");
    if (!codeInfo) {
        wxString msg = wxString::Format(
            "Missing \"%s\" template for \"%s\" class. Review your XML object description",
            name.c_str(), obj->GetClassName().c_str());
        wxLogError(msg);
        return wxEmptyString;
    }
    wxString _template = codeInfo->GetTemplate(name);
    CppTemplateParser parser(obj, _template, m_useI18n, m_useRelativePath, m_basePath);
    wxString code = parser.ParseTemplate();
    return code;
}

wxString CppCodeGenerator::GetDeclaration(PObjectBase obj,
                                          ArrayItems& arrays, bool useEnum)
{
    // Get the name
    const auto& propName = obj->GetProperty("name");
    if (!propName) {
        // Object has no name, just get its code
        return GetCode(obj, "declaration");
    }
    // Object has a name, check if its an array
    const auto& name = propName->GetValue();
    wxString baseName;
    ArrayItem unused;
    if (!ParseArrayName(name, baseName, unused)) {
        // Object is not an array, just get its code
        return GetCode(obj, "declaration");
    }
    // Object is an array, check if it needs to be declared
    auto& item = arrays[baseName];
    if (item.isDeclared) {
        // Object is already declared, return empty result
        return wxEmptyString;
    }
    // Array needs to be declared
    wxString code;

    if (useEnum) {
        // Generate enum value with element count of the array dimensions
        const auto enumBaseName = baseName.Upper();
        if (item.maxIndex.size() == 1) {
            // Without dimension index
            code.append(wxString::Format(
                "enum { %s_SIZE = %z };\n", enumBaseName, item.maxIndex[0] + 1));
        } else {
            // With dimension index
            code.append("enum { ");
            for (size_t i = 0; i < item.maxIndex.size(); ++i) {
                if (i > 0) {
                    code.append(", ");
                }
                code.append(wxString::Format(
                    "%s_%z_SIZE = %z", enumBaseName, i, item.maxIndex[i] + 1));
            }
            code.append(" };\n");
        }
    }
    // Construct proper name for declaration
    auto targetName = baseName;
    for (const auto& index : item.maxIndex)
        targetName.append(wxString::Format("[%z]", index + 1));

    propName->SetValue(targetName);           // Set declaration name
    code.append(GetCode(obj, "declaration")); // Get the Code
    propName->SetValue(name);                 // Restore the name

    item.isDeclared = true; // Mark the array as declared
    return code;
}

void CppCodeGenerator::GenClassDeclaration(PObjectBase classObj, bool useEnum,
                                           const wxString& classDecoration,
                                           const EventVector& events,
                                           ArrayItems& arrays)
{
    PProperty propName = classObj->GetProperty("name");
    if (!propName) {
        wxLogError(
            "Missing \"name\" property on \"%s\" class. Review your XML object description",
            classObj->GetClassName().c_str());
        return;
    }
    wxString className = propName->GetValue();
    if (className.empty()) {
        wxLogError("Object name can not be null");
        return;
    }
    m_header->WriteLn(
        "class " + classDecoration + className + " : "
        + GetCode(classObj, "base"));

    m_header->WriteLn("{");

    // public
    m_header->WriteLn("public:");
    m_header->Indent();
    GenAttributeDeclaration(classObj, P_PUBLIC, arrays);

    // Validators' variables
    GenValidatorVariables(classObj);

    // The constructor is also included within public
    m_header->WriteLn(GetCode(classObj, "cons_decl"));

    // Destructor
    m_header->WriteLn(wxString::Format("~%s();", className.c_str()));

    GetGenEventHandlers(classObj);
    m_header->Unindent();
    m_header->WriteLn(wxEmptyString);

    // protected
    m_header->WriteLn("protected:");
    m_header->Indent();

    if (useEnum)
        GenEnumIds(classObj);

    GenAttributeDeclaration(classObj, P_PROTECTED, arrays);

    wxString eventHandlerKind;
    wxString eventHandlerPrefix;
    wxString eventHandlerPostfix;

    PProperty eventHandlerKindProp = classObj->GetProperty("event_handler");
    if (eventHandlerKindProp)
        eventHandlerKind = eventHandlerKindProp->GetValueAsString();

    if (!eventHandlerKind.compare("decl_pure_virtual")) {
        eventHandlerPrefix = "virtual ";
        eventHandlerPostfix = " = 0;";
    } else if (!eventHandlerKind.compare("decl")) {
        eventHandlerPrefix = wxEmptyString;
        eventHandlerPostfix = ";";
    } else // Default: impl_virtual
    {
        eventHandlerPrefix = "virtual ";
        eventHandlerPostfix = " { event.Skip(); }";
    }
    GenVirtualEventHandlers(events, eventHandlerPrefix, eventHandlerPostfix);

    m_header->Unindent();
    m_header->WriteLn(wxEmptyString);

    // private
    m_header->WriteLn("private:");
    GenAttributeDeclaration(classObj, P_PRIVATE, arrays);

    if (!m_useConnect) {
        GenPrivateEventHandlers(events);
        if (events.size()) {
            m_header->WriteLn(wxEmptyString);
            m_header->Indent();
            m_header->WriteLn("wxDECLARE_EVENT_TABLE();");
            m_header->Unindent();
        }
    }
    m_header->Unindent();
    m_header->WriteLn("};");
    m_header->WriteLn(wxEmptyString);
}

void CppCodeGenerator::GenEnumIds(PObjectBase classObj)
{
    std::vector<wxString> macros;
    FindMacros(classObj, &macros);

    std::vector<wxString>::iterator it = macros.begin();
    if (it != macros.end()) {
        m_header->WriteLn("enum");
        m_header->WriteLn("{");
        m_header->Indent();

        // Remove the default macro from the set, for backward compatiblity
        it = std::find(macros.begin(), macros.end(), "ID_DEFAULT");
        if (it != macros.end()) {
            // TODO: The default macro is defined to wxID_ANY
            m_header->WriteLn("ID_DEFAULT = wxID_ANY, // Default");
            macros.erase(it);
        }
        size_t idx;
        size_t count = macros.size();
        wxString sId;

        for (idx = 0; idx < count; idx++) {
            sId = macros.at(idx);

            if (!idx)
                sId = wxString::Format("%s = %i", sId.c_str(), m_firstID);

            if (idx < (count - 1))
                sId << ",";

            m_header->WriteLn(sId);
        }
#if 0
        m_header->WriteLn(id);
#endif
        m_header->Unindent();
        m_header->WriteLn("};");
        m_header->WriteLn(wxEmptyString);
    }
}

void CppCodeGenerator::GenSubclassSets(PObjectBase obj,
                                       std::set<wxString>* subclasses,
                                       std::set<wxString>* sourceIncludes,
                                       std::vector<wxString>* headerIncludes)
{
    // Call GenSubclassForwardDeclarations on all children as well
    for (size_t i = 0; i < obj->GetChildCount(); i++)
        GenSubclassSets(obj->GetChild(i), subclasses, sourceIncludes, headerIncludes);

    // Fill the set
    PProperty subclass = obj->GetProperty("subclass");
    if (subclass) {
        std::map<wxString, wxString> children;
        subclass->SplitParentProperty(&children);

        std::map<wxString, wxString>::iterator name;
        name = children.find("name");

        if (children.end() == name) {
            // No name, so do nothing
            return;
        }
        wxString nameVal = name->second;
        if (nameVal.empty()) {
            // No name, so do nothing
            return;
        }
        //check if user wants to include the header or forward declare
        bool forward_declare = true;
        std::map<wxString, wxString>::iterator forward_declare_it
            = children.find("forward_declare");
        if (children.end() != forward_declare_it) {
            // The value needs to be tested like ObjectInspector does
            forward_declare = (forward_declare_it->second == forward_declare_it->first);
        }
        //get namespaces
        wxString originalValue = nameVal;
        int delimiter = nameVal.Find("::");
        wxString forwardDecl = "class " + nameVal + ";";
        if (wxNOT_FOUND != delimiter) {
            wxString subClassPrefix, subClassSuffix;
            do {
                wxString namespaceStr;
                namespaceStr = nameVal.Mid(0, delimiter);
                if (namespaceStr.empty()) {
                    break;
                }
                subClassPrefix += "namespace " + namespaceStr + "{ ";
                subClassSuffix += " }";

                nameVal = nameVal.Mid(delimiter + 2);
                delimiter = nameVal.Find("::");

            } while (delimiter != wxNOT_FOUND);

            if (delimiter != wxNOT_FOUND || nameVal.empty()) {
                wxLogError(
                    "Invalid Value for Property\n\tObject: %s\n\tProperty: %s\n\tValue: %s",
                    obj->GetPropertyAsString(_("name")).c_str(),
                    "subclass",
                    originalValue.c_str());
                return;
            }
            forwardDecl = subClassPrefix + "class "
                + nameVal + ";" + subClassSuffix;
        }
        // Now get the header
        wxString headerVal;
        auto header = children.find("header");
        if (children.end() != header)
            headerVal = header->second;

        if (headerVal.empty()) {
            // No header, do a forward declare if requested, otherwise do nothing
            if (forward_declare)
                subclasses->insert(forwardDecl);

            return;
        }
        // Got a header
        PObjectInfo info = obj->GetObjectInfo();
        if (!info)
            return;

        PObjectPackage pkg = info->GetPackage();
        if (!pkg)
            return;

        wxString include = "#include \"" + headerVal + "\"";
        if (pkg->GetPackageName() == "Forms"
            || obj->GetChild(1, "menu")
            || !forward_declare) {
            std::vector<wxString>::iterator it
                = std::find(headerIncludes->begin(),
                            headerIncludes->end(), include);
            if (headerIncludes->end() == it)
                headerIncludes->push_back(include);
        } else {
            subclasses->insert(forwardDecl);
            sourceIncludes->insert(include);
        }
    }
}

void CppCodeGenerator::GenIncludes(PObjectBase project,
                                   std::vector<wxString>* includes,
                                   std::set<wxString>* templates)
{
    GenObjectIncludes(project, includes, templates);
}

void CppCodeGenerator::GenObjectIncludes(PObjectBase project,
                                         std::vector<wxString>* includes,
                                         std::set<wxString>* templates)
{
    // Call GenIncludes on all children as well
    for (size_t i = 0; i < project->GetChildCount(); i++) {
        GenObjectIncludes(project->GetChild(i), includes, templates);
    }
    // Fill the set
    PCodeInfo codeInfo = project->GetObjectInfo()->GetCodeInfo("C++");
    if (codeInfo) {
        CppTemplateParser parser(project, codeInfo->GetTemplate("include"),
                                 m_useI18n, m_useRelativePath, m_basePath);
        wxString include = parser.ParseTemplate();
        if (!include.empty()) {
            if (templates->insert(include).second)
                AddUniqueIncludes(include, includes);
        }
    }
    // Generate includes for base classes
    GenBaseIncludes(project->GetObjectInfo(), project, includes, templates);
}

void CppCodeGenerator::GenBaseIncludes(PObjectInfo info, PObjectBase obj,
                                       std::vector<wxString>* includes,
                                       std::set<wxString>* templates)
{
    if (!info)
        return;

    // Process all the base classes recursively
    for (size_t i = 0; i < info->GetBaseClassCount(false); i++) {
        PObjectInfo base_info = info->GetBaseClass(i, false);
        GenBaseIncludes(base_info, obj, includes, templates);
    }
    PCodeInfo codeInfo = info->GetCodeInfo("C++");
    if (codeInfo) {
        CppTemplateParser parser(obj, codeInfo->GetTemplate("include"),
                                 m_useI18n, m_useRelativePath, m_basePath);
        wxString include = parser.ParseTemplate();
        if (!include.empty()) {
            if (templates->insert(include).second)
                AddUniqueIncludes(include, includes);
        }
    }
}

void CppCodeGenerator::AddUniqueIncludes(const wxString& include,
                                         std::vector<wxString>* includes)
{
    // Split on newlines to only generate unique include lines
    // This strips blank lines and trims
    wxStringTokenizer tkz(include, "\n", wxTOKEN_STRTOK);
    bool inPreproc = false;

    while (tkz.HasMoreTokens()) {
        wxString line = tkz.GetNextToken();
        line.Trim(false);
        line.Trim(true);

        // Anything within a #if preprocessor block will be written
        if (line.StartsWith("#if"))
            inPreproc = true;
        else if (line.StartsWith("#endif"))
            inPreproc = false;

        if (inPreproc) {
            includes->push_back(line);
            continue;
        }
        // If it is not an include line, it will be written
        if (!line.StartsWith("#include")) {
            includes->push_back(line);
            continue;
        }
        // If it is an include, it must be unique to be written
        std::vector<wxString>::iterator it = std::find(includes->begin(),
                                                       includes->end(), line);
        if (it == includes->end())
            includes->push_back(line);
    }
}

void CppCodeGenerator::FindDependencies(PObjectBase obj,
                                        std::set<PObjectInfo>& objectInfoSet)
{
    size_t count = obj->GetChildCount();
    if (count) {
        for (size_t i = 0; i < count; i++) {
            PObjectBase child = obj->GetChild(i);
            objectInfoSet.insert(child->GetObjectInfo());
            FindDependencies(child, objectInfoSet);
        }
    }
}

void CppCodeGenerator::GenConstructor(PObjectBase classObj,
                                      const EventVector& events,
                                      ArrayItems& arrays)
{
    m_source->WriteLn();
    m_source->WriteLn(GetCode(classObj, "cons_def"));
    m_source->WriteLn("{");
    m_source->Indent();

    wxString settings = GetCode(classObj, "settings");
    if (!settings.empty())
        m_source->WriteLn(settings);

    for (size_t i = 0; i < classObj->GetChildCount(); i++)
        GenConstruction(classObj->GetChild(i), true, arrays);

    wxString afterAddChild = GetCode(classObj, "after_addchild");
    if (!afterAddChild.empty()) {
        m_source->WriteLn(afterAddChild);
        if (classObj->GetTypeName() == "wizard" && classObj->GetChildCount() > 0) {
            m_source->WriteLn("for (size_t i = 1; i < m_pages.GetCount(); i++)");
            m_source->WriteLn("{");
            m_source->Indent();
            m_source->WriteLn("m_pages.Item(i)->SetPrev(m_pages.Item(i - 1));");
            m_source->WriteLn("m_pages.Item(i - 1)->SetNext(m_pages.Item(i));");
            m_source->Unindent();
            m_source->WriteLn("}");
        }
    }
    if (m_useConnect && !events.empty()) {
        m_source->WriteLn();
        m_source->WriteLn("// Connect Events");
        GenEvents(classObj, events);
    }
    m_source->Unindent();
    m_source->WriteLn("}");
}

void CppCodeGenerator::GenDestructor(PObjectBase classObj, const EventVector& events)
{
    m_source->WriteLn();
    wxString className = classObj->GetPropertyAsString("name");
    m_source->WriteLn(wxString::Format(
        "%s::~%s()", className.c_str(), className.c_str()));
    m_source->WriteLn("{");
    m_source->Indent();

    if (m_disconnectEvents && m_useConnect && !events.empty()) {
        m_source->WriteLn("// Disconnect Events");
        GenEvents(classObj, events, true);
        m_source->WriteLn();
    }
    // destruct objects
    GenDestruction(classObj);

    m_source->Unindent();
    m_source->WriteLn("}");
}

// TODO: 2 variables "isWidgets"
void CppCodeGenerator::GenConstruction(PObjectBase obj, bool is_widget,
                                       ArrayItems& arrays)
{
    wxString type = obj->GetTypeName();
    PObjectInfo info = obj->GetObjectInfo();

    if (ObjectDatabase::HasCppProperties(type)) {
        // Checking if it has not been declared as class attribute
        // so that, we will declare it inside the constructor

        wxString permStr = obj->GetProperty("permission")->GetValue();
        if (permStr == "none") {
            const auto& decl = GetDeclaration(obj, arrays, false);
            if (!decl.empty())
                m_source->WriteLn(decl);
        }
        m_source->WriteLn(GetCode(obj, "construction"));

        GenSettings(obj->GetObjectInfo(), obj);

        bool isWidget = !info->IsSubclassOf("sizer");

        for (size_t i = 0; i < obj->GetChildCount(); i++) {
            PObjectBase child = obj->GetChild(i);
            GenConstruction(child, isWidget, arrays);

            if (type == "toolbar")
                GenAddToolbar(child->GetObjectInfo(), child);
        }

        if (!isWidget) // sizers
        {
            wxString afterAddChild = GetCode(obj, "after_addchild");
            if (!afterAddChild.empty())
                m_source->WriteLn(afterAddChild);

            m_source->WriteLn();

            if (is_widget) {
                /*
                     the parent object is not a sizer.
                     There is no template for this so we'll make it manually.
                     It's not a good practice to embed templates into the source code,
                     because you will need to recompile...
                */
                wxString _template = "#wxparent $name->SetSizer($name); #nl"
                                     "#wxparent $name->Layout();"
                                     "#ifnull #parent $size"
                                     "@{ #nl $name->Fit(#wxparent $name); @}";

                CppTemplateParser parser(obj, _template,
                                         m_useI18n, m_useRelativePath, m_basePath);
                m_source->WriteLn(parser.ParseTemplate());
            }
        } else if (type == "splitter") {
            // Generating the split
            switch (obj->GetChildCount()) {
            case 1: {
                PObjectBase sub1 = obj->GetChild(0)->GetChild(0);
                wxString _template = "$name->Initialize(";
                _template = _template + sub1->GetProperty("name")->GetValue() + ");";

                CppTemplateParser parser(obj, _template,
                                         m_useI18n, m_useRelativePath, m_basePath);
                m_source->WriteLn(parser.ParseTemplate());
                break;
            }
            case 2: {
                PObjectBase sub1, sub2;
                sub1 = obj->GetChild(0)->GetChild(0);
                sub2 = obj->GetChild(1)->GetChild(0);

                wxString _template;
                if (obj->GetProperty("splitmode")->GetValue() == "wxSPLIT_VERTICAL")
                    _template = "$name->SplitVertically(";
                else
                    _template = "$name->SplitHorizontally(";

                _template = _template
                    + sub1->GetProperty("name")->GetValue() + ", "
                    + sub2->GetProperty("name")->GetValue()
                    + ", $sashpos);";

                CppTemplateParser parser(obj, _template,
                                         m_useI18n, m_useRelativePath, m_basePath);
                m_source->WriteLn(parser.ParseTemplate());
                break;
            }
            default:
                wxLogError("Missing subwindows for wxSplitterWindow widget.");
                break;
            }
        } else if (type == "menubar"
                   || type == "menu"
                   || type == "submenu"
                   || type == "ribbonbar"
                   || type == "toolbar"
                   || type == "tool"
                   || type == "listbook"
                   || type == "simplebook"
                   || type == "notebook"
                   || type == "auinotebook"
                   || type == "treelistctrl") {

            wxString afterAddChild = GetCode(obj, "after_addchild");
            if (!afterAddChild.empty())
                m_source->WriteLn(afterAddChild);

            m_source->WriteLn();
        }
    } else if (info->IsSubclassOf("sizeritembase")) {
        // The child must be added to the sizer having in mind the
        // child object type (there are 3 different routines)
        GenConstruction(obj->GetChild(0), false, arrays);

        PObjectInfo childInfo = obj->GetChild(0)->GetObjectInfo();
        wxString tempName;

        if (childInfo->IsSubclassOf("wxWindow")
            || "CustomControl" == childInfo->GetClassName())
            tempName = "window_add";
        else if (childInfo->IsSubclassOf("sizer"))
            tempName = "sizer_add";
        else if (childInfo->GetClassName() == "spacer")
            tempName = "spacer_add";
        else
            LogDebug(
                "SizerItem child is not a Spacer and is not a subclass of wxWindow or of sizer.");
        return;

        m_source->WriteLn(GetCode(obj, tempName));
    } else if (type == "notebookpage"
               || type == "listbookpage"
               || type == "choicebookpage"
               || type == "simplebookpage"
               || type == "auinotebookpage"
               || type == " wizardpagesimple") { // TODO: typo? Check consequence
        GenConstruction(obj->GetChild(0), false, arrays);
        m_source->WriteLn(GetCode(obj, "page_add"));
        GenSettings(obj->GetObjectInfo(), obj);
    } else if (type == "treebookpage") {
        GenConstruction(obj->GetChild(0), false, arrays);

        int depth = obj->GetPropertyAsInteger("depth");
        if (!depth) {
            m_source->WriteLn(GetCode(obj, "page_add"));
        } else if (depth > 0) {
            m_source->WriteLn(GetCode(obj, "subpage_add"));
        }
        GenSettings(obj->GetObjectInfo(), obj);
    } else if (type == "toolbookpage") {
        GenConstruction(obj->GetChild(0), false, arrays);
        GenSettings(obj->GetObjectInfo(), obj);
        m_source->WriteLn(GetCode(obj, "page_add"));
    } else if (type == "treelistctrlcolumn") {
        m_source->WriteLn(GetCode(obj, "column_add"));
        GenSettings(obj->GetObjectInfo(), obj);
    } else if (type == "tool") {
        // If loading bitmap from ICON resource, and size is not set,
        // set size to toolbars bitmapsize
        // TODO: So hacky, yet so useful ...
        wxSize toolbarsize = obj->GetParent()->GetPropertyAsSize("bitmapsize");
        if (wxDefaultSize != toolbarsize) {
            PProperty prop = obj->GetProperty("bitmap");
            if (prop) {
                wxString oldVal = prop->GetValueAsString();
                wxString path, source;
                wxSize toolsize;
                TypeConv::ParseBitmapWithResource(oldVal, &path, &source, &toolsize);
                if (_("Load From Icon Resource") == source
                    && wxDefaultSize == toolsize) {
                    prop->SetValue(wxString::Format("%s; %s [%i; %i]", path.c_str(),
                                                    source.c_str(),
                                                    toolbarsize.GetWidth(),
                                                    toolbarsize.GetHeight()));
                    m_source->WriteLn(GetCode(obj, "construction"));
                    prop->SetValue(oldVal);
                    return;
                }
            }
        }
        m_source->WriteLn(GetCode(obj, "construction"));
    } else {
        // Generate the children
        for (size_t i = 0; i < obj->GetChildCount(); i++)
            GenConstruction(obj->GetChild(i), false, arrays);
    }
}

// TODO: are pointers necessary?
void CppCodeGenerator::FindMacros(PObjectBase obj, std::vector<wxString>* macros)
{
    // iterate through all of the properties of all objects, add the macros
    // to the vector
    for (size_t i = 0; i < obj->GetPropertyCount(); i++) {
        PProperty prop = obj->GetProperty(i);
        if (prop->GetType() == PT_MACRO) {
            wxString value = prop->GetValue();
            // Skip wx IDs
            if ((!value.Contains("XRCID"))
                && (m_predMacros.end() == m_predMacros.find(value))) {
                if (macros->end() == std::find(macros->begin(), macros->end(), value))
                    macros->push_back(value);
            }
        }
    }
    for (size_t i = 0; i < obj->GetChildCount(); i++)
        FindMacros(obj->GetChild(i), macros);
}

void CppCodeGenerator::FindEventHandlers(PObjectBase obj, EventVector& events)
{
    for (size_t i = 0; i < obj->GetEventCount(); i++) {
        PEvent event = obj->GetEvent(i);
        if (!event->GetValue().IsEmpty())
            events.push_back(event);
    }
    for (size_t i = 0; i < obj->GetChildCount(); i++) {
        PObjectBase child = obj->GetChild(i);
        FindEventHandlers(child, events);
    }
}

void CppCodeGenerator::GenDefines(PObjectBase project)
{
    std::vector<wxString> macros;
    FindMacros(project, &macros);

    // Remove the default macro from the set, for backward compatiblity
    std::vector<wxString>::iterator it;
    it = std::find(macros.begin(), macros.end(), "ID_DEFAULT");
    if (it != macros.end()) {
        // The default macro is defined to wxID_ANY
        m_header->WriteLn("#define ID_DEFAULT wxID_ANY // Default");
        macros.erase(it);
    }
    size_t id = m_firstID;
    if (id < 1000)
        wxLogWarning("First ID is Less than 1000");

    for (it = macros.begin(); it != macros.end(); it++) {
        // Don't redefine wx IDs
        m_header->WriteLn(wxString::Format("#define %s %i", it->c_str(), id));
        id++;
    }
    m_header->WriteLn(wxEmptyString);
}

void CppCodeGenerator::GenSettings(PObjectInfo info, PObjectBase obj)
{
    PCodeInfo codeInfo = info->GetCodeInfo("C++");
    if (!codeInfo)
        return;

    wxString _template = codeInfo->GetTemplate("settings");
    if (!_template.empty()) {
        CppTemplateParser parser(obj, _template,
                                 m_useI18n, m_useRelativePath, m_basePath);
        wxString code = parser.ParseTemplate();
        if (!code.empty())
            m_source->WriteLn(code);
    }
    // Proceeding recursively with the base classes
    for (size_t i = 0; i < info->GetBaseClassCount(false); i++) {
        PObjectInfo base_info = info->GetBaseClass(i, false);
        GenSettings(base_info, obj);
    }
}

void CppCodeGenerator::GenDestruction(PObjectBase obj)
{
    PCodeInfo codeInfo = obj->GetObjectInfo()->GetCodeInfo("C++");
    if (codeInfo) {
        wxString _template = codeInfo->GetTemplate("destruction");

        if (!_template.empty()) {
            CppTemplateParser parser(obj, _template,
                                     m_useI18n, m_useRelativePath, m_basePath);
            wxString code = parser.ParseTemplate();
            if (!code.empty())
                m_source->WriteLn(code);
        }
    }
    // Process child widgets
    for (size_t i = 0; i < obj->GetChildCount(); i++) {
        PObjectBase child = obj->GetChild(i);
        GenDestruction(child);
    }
}

void CppCodeGenerator::GenAddToolbar(PObjectInfo info, PObjectBase obj)
{
    wxArrayString arrCode;
    GetAddToolbarCode(info, obj, arrCode);

    for (size_t i = 0; i < arrCode.GetCount(); i++)
        m_source->WriteLn(arrCode[i]);
}

void CppCodeGenerator::GetAddToolbarCode(PObjectInfo info, PObjectBase obj, wxArrayString& codelines)
{
    PCodeInfo codeInfo = info->GetCodeInfo("C++");
    if (!codeInfo)
        return;

    wxString _template = codeInfo->GetTemplate("toolbar_add");
    if (!_template.empty()) {
        CppTemplateParser parser(obj, _template,
                                 m_useI18n, m_useRelativePath, m_basePath);
        wxString code = parser.ParseTemplate();
        if (!code.empty()) {
            if (codelines.Index(code) == wxNOT_FOUND)
                codelines.Add(code);
        }
    }
    // Proceeding recursively with the base classes
    for (size_t i = 0; i < info->GetBaseClassCount(false); i++) {
        PObjectInfo base_info = info->GetBaseClass(i, false);
        GetAddToolbarCode(base_info, obj, codelines);
    }
}

void CppCodeGenerator::GenEmbeddedBitmapIncludes(PObjectBase project)
{
    std::set<wxString> includeSet;

    // We begin obtaining the "include" list
    FindEmbeddedBitmapProperties(project, includeSet);

    if (includeSet.empty())
        return;

    // and then, we generate them
    std::set<wxString>::iterator it;
    for (it = includeSet.begin(); it != includeSet.end(); it++) {
        if (!it->empty())
            m_source->WriteLn(*it);
    }
    m_source->WriteLn();
}

void CppCodeGenerator::FindEmbeddedBitmapProperties(PObjectBase obj,
                                                    std::set<wxString>& embedSet)
{
    /*
        We go through (browse) for each property in "obj" object. If any of the
        PT_BITMAP type is found, then the proper "include" string is added
        in "set". After that, we recursively do it the same with the child objects.
    */
    size_t i, count = obj->GetPropertyCount();
    for (i = 0; i < count; i++) {
        PProperty property = obj->GetProperty(i);
        if (property->GetType() == PT_BITMAP) {
            wxString propValue = property->GetValue();
            wxString path;
            wxString source;
            wxSize icoSize;
            TypeConv::ParseBitmapWithResource(propValue, &path, &source, &icoSize);

            wxFileName bmpFileName(path);
            if (bmpFileName.GetExt().Upper() == "XPM") {
                wxString absPath = TypeConv::MakeAbsolutePath(
                    path, AppData()->GetProjectPath());

                // It's supposed that "path" contains an absolut path to the file
                // and not a relative one.
                wxString relPath
                    = (m_useRelativePath
                           ? TypeConv::MakeRelativePath(absPath, m_basePath)
                           : absPath);
                wxString inc;
                inc << "#include \"" << relPath << "\"";
                embedSet.insert(inc);
            } else if (source == _("Load From Embedded File")) {
                wxString absPath = TypeConv::MakeAbsolutePath(
                    path, AppData()->GetProjectPath());
                wxString includePath = FileToCArray::Generate(absPath);
                wxString inc;
                inc << "#include \"" << includePath << "\"";
                embedSet.insert(inc);
            }
#if 0
            /*
                NOTE: This is currently not necessary because the default code
                already contains this header.
                Because the unique include filtering is not global
                this cannot be enabled without creating a duplicate entry.
            */
            else if (source == _("Load From XRC")) {
                embedSet.insert("#include <wx/xrc/xmlres.h>");
            }
#endif
        }
    }
    count = obj->GetChildCount();
    for (i = 0; i < count; i++) {
        PObjectBase child = obj->GetChild(i);
        FindEmbeddedBitmapProperties(child, embedSet);
    }
}

void CppCodeGenerator::UseRelativePath(bool relative, wxString basePath)
{
    if (relative) {
        if (wxFileName::DirExists(basePath))
            m_basePath = basePath;
        else
            m_basePath = wxEmptyString;
    }
    m_useRelativePath = relative;
}
#if 0
wxString CppCodeGenerator::ConvertToRelativePath(wxString path, wxString basePath)
{
    wxString auxPath = path;
    if (basePath != "")
    {
        wxFileName filename(_WXSTR(auxPath));
        if (filename.MakeRelativeTo(_WXSTR(basePath)))
            auxPath = _STDSTR(filename.GetFullPath());
    }
    return auxPath;
}
#endif

void CppCodeGenerator::SetupPredefinedMacros()
{
#define ADD_PREDEFINED_MACRO(x) m_predMacros.insert(#x)

    ADD_PREDEFINED_MACRO(wxID_NONE);
    ADD_PREDEFINED_MACRO(wxID_SEPARATOR);
    ADD_PREDEFINED_MACRO(wxID_ANY);
    ADD_PREDEFINED_MACRO(wxID_LOWEST);

    ADD_PREDEFINED_MACRO(wxID_OPEN);
    ADD_PREDEFINED_MACRO(wxID_CLOSE);
    ADD_PREDEFINED_MACRO(wxID_NEW);
    ADD_PREDEFINED_MACRO(wxID_SAVE);
    ADD_PREDEFINED_MACRO(wxID_SAVEAS);
    ADD_PREDEFINED_MACRO(wxID_REVERT);
    ADD_PREDEFINED_MACRO(wxID_EXIT);
    ADD_PREDEFINED_MACRO(wxID_UNDO);
    ADD_PREDEFINED_MACRO(wxID_REDO);
    ADD_PREDEFINED_MACRO(wxID_HELP);
    ADD_PREDEFINED_MACRO(wxID_PRINT);
    ADD_PREDEFINED_MACRO(wxID_PRINT_SETUP);
    ADD_PREDEFINED_MACRO(wxID_PREVIEW);
    ADD_PREDEFINED_MACRO(wxID_ABOUT);
    ADD_PREDEFINED_MACRO(wxID_HELP_CONTENTS);
    ADD_PREDEFINED_MACRO(wxID_HELP_COMMANDS);
    ADD_PREDEFINED_MACRO(wxID_HELP_PROCEDURES);
    ADD_PREDEFINED_MACRO(wxID_HELP_CONTEXT);
    ADD_PREDEFINED_MACRO(wxID_CLOSE_ALL);
    ADD_PREDEFINED_MACRO(wxID_PAGE_SETUP);
    ADD_PREDEFINED_MACRO(wxID_HELP_INDEX);
    ADD_PREDEFINED_MACRO(wxID_HELP_SEARCH);
    ADD_PREDEFINED_MACRO(wxID_PREFERENCES);

    ADD_PREDEFINED_MACRO(wxID_EDIT);
    ADD_PREDEFINED_MACRO(wxID_CUT);
    ADD_PREDEFINED_MACRO(wxID_COPY);
    ADD_PREDEFINED_MACRO(wxID_PASTE);
    ADD_PREDEFINED_MACRO(wxID_CLEAR);
    ADD_PREDEFINED_MACRO(wxID_FIND);

    ADD_PREDEFINED_MACRO(wxID_DUPLICATE);
    ADD_PREDEFINED_MACRO(wxID_SELECTALL);
    ADD_PREDEFINED_MACRO(wxID_DELETE);
    ADD_PREDEFINED_MACRO(wxID_REPLACE);
    ADD_PREDEFINED_MACRO(wxID_REPLACE_ALL);
    ADD_PREDEFINED_MACRO(wxID_PROPERTIES);

    ADD_PREDEFINED_MACRO(wxID_VIEW_DETAILS);
    ADD_PREDEFINED_MACRO(wxID_VIEW_LARGEICONS);
    ADD_PREDEFINED_MACRO(wxID_VIEW_SMALLICONS);
    ADD_PREDEFINED_MACRO(wxID_VIEW_LIST);
    ADD_PREDEFINED_MACRO(wxID_VIEW_SORTDATE);
    ADD_PREDEFINED_MACRO(wxID_VIEW_SORTNAME);
    ADD_PREDEFINED_MACRO(wxID_VIEW_SORTSIZE);
    ADD_PREDEFINED_MACRO(wxID_VIEW_SORTTYPE);

    ADD_PREDEFINED_MACRO(wxID_FILE);
    ADD_PREDEFINED_MACRO(wxID_FILE1);
    ADD_PREDEFINED_MACRO(wxID_FILE2);
    ADD_PREDEFINED_MACRO(wxID_FILE3);
    ADD_PREDEFINED_MACRO(wxID_FILE4);
    ADD_PREDEFINED_MACRO(wxID_FILE5);
    ADD_PREDEFINED_MACRO(wxID_FILE6);
    ADD_PREDEFINED_MACRO(wxID_FILE7);
    ADD_PREDEFINED_MACRO(wxID_FILE8);
    ADD_PREDEFINED_MACRO(wxID_FILE9);

    /*  Standard button and menu IDs */
    ADD_PREDEFINED_MACRO(wxID_OK);
    ADD_PREDEFINED_MACRO(wxID_CANCEL);

    ADD_PREDEFINED_MACRO(wxID_APPLY);
    ADD_PREDEFINED_MACRO(wxID_YES);
    ADD_PREDEFINED_MACRO(wxID_NO);
    ADD_PREDEFINED_MACRO(wxID_STATIC);
    ADD_PREDEFINED_MACRO(wxID_FORWARD);
    ADD_PREDEFINED_MACRO(wxID_BACKWARD);
    ADD_PREDEFINED_MACRO(wxID_DEFAULT);
    ADD_PREDEFINED_MACRO(wxID_MORE);
    ADD_PREDEFINED_MACRO(wxID_SETUP);
    ADD_PREDEFINED_MACRO(wxID_RESET);
    ADD_PREDEFINED_MACRO(wxID_CONTEXT_HELP);
    ADD_PREDEFINED_MACRO(wxID_YESTOALL);
    ADD_PREDEFINED_MACRO(wxID_NOTOALL);
    ADD_PREDEFINED_MACRO(wxID_ABORT);
    ADD_PREDEFINED_MACRO(wxID_RETRY);
    ADD_PREDEFINED_MACRO(wxID_IGNORE);
    ADD_PREDEFINED_MACRO(wxID_ADD);
    ADD_PREDEFINED_MACRO(wxID_REMOVE);

    ADD_PREDEFINED_MACRO(wxID_UP);
    ADD_PREDEFINED_MACRO(wxID_DOWN);
    ADD_PREDEFINED_MACRO(wxID_HOME);
    ADD_PREDEFINED_MACRO(wxID_REFRESH);
    ADD_PREDEFINED_MACRO(wxID_STOP);
    ADD_PREDEFINED_MACRO(wxID_INDEX);

    ADD_PREDEFINED_MACRO(wxID_BOLD);
    ADD_PREDEFINED_MACRO(wxID_ITALIC);
    ADD_PREDEFINED_MACRO(wxID_JUSTIFY_CENTER);
    ADD_PREDEFINED_MACRO(wxID_JUSTIFY_FILL);
    ADD_PREDEFINED_MACRO(wxID_JUSTIFY_RIGHT);

    ADD_PREDEFINED_MACRO(wxID_JUSTIFY_LEFT);
    ADD_PREDEFINED_MACRO(wxID_UNDERLINE);
    ADD_PREDEFINED_MACRO(wxID_INDENT);
    ADD_PREDEFINED_MACRO(wxID_UNINDENT);
    ADD_PREDEFINED_MACRO(wxID_ZOOM_100);
    ADD_PREDEFINED_MACRO(wxID_ZOOM_FIT);
    ADD_PREDEFINED_MACRO(wxID_ZOOM_IN);
    ADD_PREDEFINED_MACRO(wxID_ZOOM_OUT);
    ADD_PREDEFINED_MACRO(wxID_UNDELETE);
    ADD_PREDEFINED_MACRO(wxID_REVERT_TO_SAVED);

    /*  System menu IDs (used by wxUniv): */
    ADD_PREDEFINED_MACRO(wxID_SYSTEM_MENU);
    ADD_PREDEFINED_MACRO(wxID_CLOSE_FRAME);
    ADD_PREDEFINED_MACRO(wxID_MOVE_FRAME);
    ADD_PREDEFINED_MACRO(wxID_RESIZE_FRAME);
    ADD_PREDEFINED_MACRO(wxID_MAXIMIZE_FRAME);
    ADD_PREDEFINED_MACRO(wxID_ICONIZE_FRAME);
    ADD_PREDEFINED_MACRO(wxID_RESTORE_FRAME);

    /*  IDs used by generic file dialog (13 consecutive starting from this value) */
    ADD_PREDEFINED_MACRO(wxID_FILEDLGG);
    ADD_PREDEFINED_MACRO(wxID_HIGHEST);

#undef ADD_PREDEFINED_MACRO
}
