/*
    wxWeaver - A GUI Designer Editor for wxWidgets.
    Copyright (C) 2005 José Antonio Hurtado
    Copyright (C) 2005 Juan Antonio Ortega (as wxFormBuilder)
    Copyright (C) 2011-2021 Jefferson González <jgmdev@gmail.com>
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
#include "codegen/phpcg.h"

#include "codewriter.h"
#include "utils/typeconv.h"
#include "utils/debug.h"
#include "appdata.h"
#include "rtti/objectbase.h"
#include "utils/exception.h"

#include <algorithm>

#include <wx/filename.h>
#include <wx/tokenzr.h>

PHPTemplateParser::PHPTemplateParser(PObjectBase obj, wxString _template,
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

PHPTemplateParser::PHPTemplateParser(const PHPTemplateParser& other,
                                     wxString _template)
    : TemplateParser(other, _template)
    , m_basePath(other.m_basePath)
    , m_useI18n(other.m_useI18n)
    , m_useRelativePath(other.m_useRelativePath)
{
}

wxString PHPTemplateParser::RootWxParentToCode()
{
    return "$this";
}

PTemplateParser PHPTemplateParser::CreateParser(const TemplateParser* oldparser,
                                                wxString _template)
{
    const PHPTemplateParser* phpOldParser
        = dynamic_cast<const PHPTemplateParser*>(oldparser);

    if (phpOldParser) {
        PTemplateParser newparser(new PHPTemplateParser(*phpOldParser, _template));
        return newparser;
    }
    return PTemplateParser();
}

wxString PHPTemplateParser::ValueToCode(PropertyType type, wxString value)
{
    wxString result;

    switch (type) {
    case PT_WXPARENT: {
        result = "$this->" + value;
        break;
    }
    case PT_WXPARENT_SB: {
        result = "$" + value + "->GetStaticBox()";
        break;
    }
    case PT_WXPARENT_CP: {
        result = "$this->" + value + "->GetPane()";
        break;
    }
    case PT_WXSTRING:
    case PT_FILE:
    case PT_PATH: {
        if (value.empty())
            result << "wxEmptyString";
        else
            result << "\"" << PHPCodeGenerator::ConvertPHPString(value) << "\"";

        break;
    }
    case PT_WXSTRING_I18N: {
        if (value.empty()) {
            result << "wxEmptyString";
        } else {
            if (m_useI18n)
                result << "_(\"" << PHPCodeGenerator::ConvertPHPString(value) << "\")";
            else
                result << "\"" << PHPCodeGenerator::ConvertPHPString(value) << "\"";
        }
        break;
    }
    case PT_CLASS:
    case PT_MACRO:
    case PT_OPTION:
    case PT_EDIT_OPTION: {
        result = value;
        break;
    }
    case PT_TEXT:
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
            result << "new wxPoint(" << value << ")";

        break;
    }
    case PT_WXSIZE: {
        if (value.empty())
            result = "wxDefaultSize";
        else
            result << "new wxSize(" << value << ")";

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
                "new wxFont( %s, %s, %s, %s, %s, %s )",
                ((pointSize <= 0)
                     ? "wxC2D(wxNORMAL_FONT)->GetPointSize()"
                     : (wxString() << pointSize)),
                TypeConv::FontFamilyToString(fontContainer.GetFamily()),
                font.GetStyleString(),
                font.GetWeightString(),
                (fontContainer.GetUnderlined() ? "true" : "false"),
                (fontContainer.m_faceName.empty()
                     ? "wxEmptyString"
                     : ("\"" + fontContainer.m_faceName + "\"")));
        } else {
            result = "wxC2D(wxNORMAL_FONT)";
        }
        break;
    }
    case PT_WXCOLOUR: {
        if (!value.empty()) {
            if (!value.find_first_of("wx")) {
                // System Colour
                result << "wxSystemSettings::GetColour("
                       << ValueToCode(PT_OPTION, value) << ")";
            } else {
                wxColour colour = TypeConv::StringToColour(value);
                result = wxString::Format(
                    "new wxColour(%i, %i, %i)",
                    colour.Red(), colour.Green(), colour.Blue());
            }
        } else {
            result = "new wxColour()";
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
                "PHP code generation does not support using URLs for bitmap properties:\n%s",
                path.c_str());
            result = "wxNullBitmap";
            break;
        }
        if (source == "Load From File") {
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

            result << "new wxBitmap(\""
                   << PHPCodeGenerator::ConvertPHPString(file)
                   << "\", wxBITMAP_TYPE_ANY)";
        } else if (source == "Load From Embedded File") {
            result << "wxNullBitmap /* embedded files aren't supported */";
        } else if (source == "Load From Resource") {
            result << "new wxBitmap(\"" << path
                   << "\", wxBITMAP_TYPE_RESOURCE)";
        } else if (source == "Load From Icon Resource") {
            if (wxDefaultSize == icoSize) {
                result << "new wxICON(" << path << ")";
            } else {
                result.Printf(
                    "new wxIcon(\"%s\", wxBITMAP_TYPE_ICO_RESOURCE, %i, %i)",
                    path.c_str(),
                    icoSize.GetWidth(), icoSize.GetHeight());
            }
        } else if (source == "Load From XRC") {
            // This requires that the global wxXmlResource object is set
            result << "wxXmlResource::Get()->LoadBitmap(\"" << path
                   << "\")";
        } else if (source == "Load From Art Provider") {
            wxString rid = path.BeforeFirst(':');
            if (rid.StartsWith("gtk-"))
                rid = "\"" + rid + "\"";

            result = "wxArtProvider::GetBitmap(" + rid + ", "
                + path.AfterFirst(':') + ")";
        }
        break;
    }
    case PT_STRINGLIST: {
        // Stringlists are generated like a sequence of wxString separated by ", ".
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

PHPCodeGenerator::PHPCodeGenerator()
{
    SetupPredefinedMacros();
    m_useRelativePath = false;
    m_useI18n = false;
    m_firstID = 1000;
}

wxString PHPCodeGenerator::ConvertPHPString(wxString text)
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

void PHPCodeGenerator::GenerateInheritedClass(PObjectBase userClasses, PObjectBase form)
{
    if (!userClasses) {
        wxLogError("There is no object to generate inherited class");
        return;
    }
    if ("UserClasses" != userClasses->GetClassName()) {
        wxLogError("This not a UserClasses object");
        return;
    }
    wxString type = userClasses->GetPropertyAsString("type");

    // Start file
    wxString code = GetCode(userClasses, "file_comment");
    m_source->WriteLn(code);
    m_source->WriteLn(wxEmptyString);

    code = GetCode(userClasses, "source_include");
    m_source->WriteLn(code);
    m_source->WriteLn(wxEmptyString);

    code = GetCode(userClasses, "class_decl");
    m_source->WriteLn(code);
    m_source->Indent();

    code = GetCode(userClasses, type + "_cons_def");
    m_source->WriteLn(code);

    // Do events
    EventVector events;
    FindEventHandlers(form, events);

    if (events.size() > 0) {
        code = GetCode(userClasses, "event_handler_comment");
        m_source->WriteLn(code);

        std::set<wxString> generatedHandlers;
        for (size_t i = 0; i < events.size(); i++) {
            PEvent event = events[i];
            if (generatedHandlers.find(event->GetValue()) == generatedHandlers.end()) {
                m_source->WriteLn(wxString::Format(
                    "function %s(event){", event->GetValue().c_str()));
                m_source->Indent();
                m_source->WriteLn(wxString::Format(
                    "// TODO: Implement %s", event->GetValue().c_str()));
                m_source->Unindent();
                m_source->WriteLn("}");
                m_source->WriteLn(wxEmptyString);
                generatedHandlers.insert(event->GetValue());
            }
        }
        m_source->WriteLn(wxEmptyString);
    }
    m_source->Unindent();
}

bool PHPCodeGenerator::GenerateCode(PObjectBase project)
{
    if (!project) {
        wxLogError("There is no project to generate code");
        return false;
    }
    m_useI18n = false;
    PProperty i18nProperty = project->GetProperty("internationalize");
    if (i18nProperty && i18nProperty->GetValueAsInteger())
        m_useI18n = true;

    m_disconnectEvents = (project->GetPropertyAsInteger("disconnect_php_events"));

    m_source->Clear();

    // Insert php preamble
    wxString code = GetCode(project, "php_preamble");
    if (!code.empty())
        m_source->WriteLn(code);

    code = wxString::Format(
        "/*\n"
        "    PHP code generated with wxWeaver (version %s%s " __DATE__ ")\n"
        "    https://wxweaver.github.io/\n"
        "\n"
        "    PLEASE DO *NOT* EDIT THIS FILE!\n"
        "*/",
        VERSION, REVISION);

    m_source->WriteLn(code, true);

    PProperty propFile = project->GetProperty("file");
    if (!propFile) {
        wxLogError("Missing \"file\" property on Project Object");
        return false;
    }
    wxString file = propFile->GetValueAsString();
    if (file.empty())
        file = "noname";

    // Generate the subclass sets
    std::set<wxString> subclasses;
    std::vector<wxString> headerIncludes;

    GenSubclassSets(project, &subclasses, &headerIncludes);

    // Generating in the .h header file those include from components dependencies.
    std::set<wxString> templates;
    GenIncludes(project, &headerIncludes, &templates);

    // Write the include lines
    std::vector<wxString>::iterator includeIt;
    for (includeIt = headerIncludes.begin();
         includeIt != headerIncludes.end(); ++includeIt)
        m_source->WriteLn(*includeIt);

    if (!headerIncludes.empty())
        m_source->WriteLn(wxEmptyString);
#if 0
    // TODO: Write internationalization support
    if (m_useI18n) {
            // PHP gettext already implements this function
        m_source->WriteLn("function _(){/* TODO: Implement this function on wxPHP */}");
        m_source->WriteLn(wxEmptyString);
    }
#endif
    // Generating "defines" for macros
    GenDefines(project);

    wxString eventHandlerPostfix;
    PProperty eventKindProp = project->GetProperty("skip_php_events");
    if (eventKindProp->GetValueAsInteger())
        eventHandlerPostfix = "$event->Skip();";
    else
        eventHandlerPostfix = wxEmptyString;

    PProperty disconnectMode = project->GetProperty("disconnect_mode");
    m_disconnecMode = disconnectMode->GetValueAsString();

    for (size_t i = 0; i < project->GetChildCount(); i++) {
        PObjectBase child = project->GetChild(i);

        // Preprocess to find arrays
        ArrayItems arrays;
        FindArrayObjects(child, arrays, true);

        EventVector events;
        FindEventHandlers(child, events);
#if 0
        GenClassDeclaration(child, useEnum, classDecoration, events,
                            eventHandlerPrefix, eventHandlerPostfix);
#else
        GenClassDeclaration(child, false, wxEmptyString, events,
                            eventHandlerPostfix, arrays);
#endif
    }
    code = GetCode(project, "php_epilogue");
    if (!code.empty())
        m_source->WriteLn(code);

    return true;
}

void PHPCodeGenerator::GenEvents(PObjectBase classObj,
                                 const EventVector& events, bool disconnect)
{
    if (events.empty())
        return;

    if (disconnect) {
        m_source->WriteLn("// Disconnect Events");
    } else {
        m_source->WriteLn();
        m_source->WriteLn("// Connect Events");
    }
    PProperty propName = classObj->GetProperty("name");
    if (!propName) {
        wxLogError(
            "Missing \"name\" property on \"%s\" class. Review your XML object description",
            classObj->GetClassName().c_str());
        return;
    }
    wxString className = propName->GetValueAsString();
    if (className.empty()) {
        wxLogError("Object name cannot be null");
        return;
    }
    wxString baseClass, handlerName;

    PProperty propSubclass = classObj->GetProperty("subclass");
    if (propSubclass) {
        wxString subclass = propSubclass->GetChildFromParent("name");
        if (!subclass.empty())
            baseClass = subclass;
    }
    if (baseClass.empty())
        baseClass = "wx" + classObj->GetClassName();

    if (events.size() > 0) {
        for (size_t i = 0; i < events.size(); i++) {
            PEvent event = events[i];

            handlerName = event->GetValue();

            wxString templateName = wxString::Format(
                "connect_%s", event->GetName().c_str());

            PObjectBase obj = event->GetObject();
            if (!GenEventEntry(obj, obj->GetObjectInfo(),
                               templateName, handlerName, disconnect)) {
                wxLogError(
                    "Missing \"evt_%s\" template for \"%s\" class. Review your XML object description",
                    templateName.c_str(), className.c_str());
            }
        }
    }
}

bool PHPCodeGenerator::GenEventEntry(PObjectBase obj, PObjectInfo obj_info,
                                     const wxString& templateName,
                                     const wxString& handlerName, bool disconnect)
{
    wxString _template;
    PCodeInfo codeInfo = obj_info->GetCodeInfo("PHP");
    if (codeInfo) {
        _template = codeInfo->GetTemplate(wxString::Format(
            "evt_%s%s", disconnect ? wxS("dis") : wxEmptyString, templateName.c_str()));
        if (disconnect && _template.empty()) {
            _template = codeInfo->GetTemplate("evt_" + templateName);
            _template.Replace("Connect", "Disconnect", true);
        }
        if (!_template.empty()) {
            if (disconnect) {
                if (m_disconnecMode == "handler_name")
                    _template.Replace("#handler",
                                      "handler = array(@$this, \""
                                          + handlerName + "\")");
                else if (m_disconnecMode == "source_name")
                    _template.Replace(
                        "#handler",
#if 1
                        "null");
#else
                        "$this->" + obj->GetProperty("name")->GetValueAsString());
#endif
            } else
                _template.Replace(
                    "#handler", "array(@$this, \"" + handlerName + "\")");

            PHPTemplateParser parser(obj, _template,
                                     m_useI18n, m_useRelativePath, m_basePath);
            m_source->WriteLn(parser.ParseTemplate());
            return true;
        }
    }
    for (size_t i = 0; i < obj_info->GetBaseClassCount(); i++) {
        PObjectInfo baseInfo = obj_info->GetBaseClass(i);
        if (GenEventEntry(obj, baseInfo, templateName, handlerName, disconnect))
            return true;
    }
    return false;
}

void PHPCodeGenerator::GenVirtualEventHandlers(const EventVector& events, const wxString& eventHandlerPostfix)
{
    if (events.size() > 0) {
        /*
            There are problems if we create "pure" virtual handlers,
            because some events could be triggered in the constructor
            in which virtual methods are executed properly.
            So we create a default handler which will skip the event.
        */
        m_source->WriteLn(wxEmptyString);
        m_source->WriteLn("// Virtual event handlers, override them in your derived class");

        std::set<wxString> generatedHandlers;
        for (size_t i = 0; i < events.size(); i++) {
            PEvent event = events[i];
            wxString aux = "function " + event->GetValue() + "($event){";

            if (generatedHandlers.find(aux) == generatedHandlers.end()) {
                m_source->WriteLn(aux);
                m_source->Indent();
                m_source->WriteLn(eventHandlerPostfix);
                m_source->Unindent();
                m_source->WriteLn("}");

                generatedHandlers.insert(aux);
            }
            if (i < (events.size() - 1))
                m_source->WriteLn();
        }
        m_source->WriteLn(wxEmptyString);
    }
}

void PHPCodeGenerator::GetGenEventHandlers(PObjectBase obj)
{
    GenDefinedEventHandlers(obj->GetObjectInfo(), obj);

    for (size_t i = 0; i < obj->GetChildCount(); i++) {
        PObjectBase child = obj->GetChild(i);
        GetGenEventHandlers(child);
    }
}

void PHPCodeGenerator::GenDefinedEventHandlers(PObjectInfo info, PObjectBase obj)
{
    PCodeInfo codeInfo = info->GetCodeInfo("PHP");
    if (codeInfo) {
        wxString _template = codeInfo->GetTemplate("generated_event_handlers");
        if (!_template.empty()) {
            PHPTemplateParser parser(obj, _template,
                                     m_useI18n, m_useRelativePath, m_basePath);

            wxString code = parser.ParseTemplate();
            if (!code.empty())
                m_source->WriteLn(code);
        }
    }
    // Proceeding recursively with the base classes
    for (size_t i = 0; i < info->GetBaseClassCount(); i++) {
        PObjectInfo base_info = info->GetBaseClass(i);
        GenDefinedEventHandlers(base_info, obj);
    }
}

wxString PHPCodeGenerator::GetCode(PObjectBase obj, wxString name, bool silent)
{
    PCodeInfo codeInfo = obj->GetObjectInfo()->GetCodeInfo("PHP");
    if (!codeInfo) {
        if (!silent) {
            wxString msg(wxString::Format(
                "Missing \"%s\" template for \"%s\" class. Review your XML object description",
                name.c_str(), obj->GetClassName().c_str()));
            wxLogError(msg);
        }
        return wxEmptyString;
    }
    wxString _template = codeInfo->GetTemplate(name);
    PHPTemplateParser parser(obj, _template, m_useI18n, m_useRelativePath, m_basePath);
    wxString code = parser.ParseTemplate();
    return code;
}

wxString PHPCodeGenerator::GetConstruction(PObjectBase obj, ArrayItems& arrays)
{
    // Get the name
    const auto& propName = obj->GetProperty("name");
    if (!propName) {
        // Object has no name, just get its code
        return GetCode(obj, "construction");
    }
    // Object has a name, check if its an array
    const auto& name = propName->GetValueAsString();
    wxString baseName;
    ArrayItem unused;
    if (!ParseArrayName(name, baseName, unused)) {
        // Object is not an array, just get its code
        return GetCode(obj, "construction");
    }
    // Object is an array, check if it needs to be declared
    auto& item = arrays[baseName];
    if (item.isDeclared) {
        // Object is already declared, just get its code
        return GetCode(obj, "construction");
    }
    // Array needs to be declared
    wxString code;

    // Array declaration
    // Base array
    code.append("$this->");
    code.append(baseName);
    code.append(" = array();\n");

    // If more dimensions are present they will get created automatically

    // Get the Code
    code.append(GetCode(obj, "construction"));

    // Mark the array as declared
    item.isDeclared = true;

    return code;
}

void PHPCodeGenerator::GenClassDeclaration(PObjectBase classObj, bool /*use_enum*/,
                                           const wxString& classDecoration,
                                           const EventVector& events,
                                           const wxString& eventHandlerPostfix,
                                           ArrayItems& arrays)
{
    PProperty propName = classObj->GetProperty("name");
    if (!propName) {
        wxLogError(
            "Missing \"name\" property on \"%s\" class. Review your XML object description",
            classObj->GetClassName().c_str());
        return;
    }
    wxString className = propName->GetValueAsString();
    if (className.empty()) {
        wxLogError("Object name can not be null");
        return;
    }
    m_source->WriteLn("/*");
    m_source->WriteLn(" * Class " + className);
    m_source->WriteLn(" */");
    m_source->WriteLn();
    m_source->WriteLn(
        "class " + classDecoration + className
        + " extends " + GetCode(classObj, "base").Trim() + " {");
    m_source->Indent();

    // The constructor is also included within public
    GenConstructor(classObj, events, arrays);
    GenDestructor(classObj, events);

    m_source->WriteLn(wxEmptyString);

    // event handlers
    GenVirtualEventHandlers(events, eventHandlerPostfix);
    GetGenEventHandlers(classObj);

    m_source->Unindent();
    m_source->WriteLn("}");
    m_source->WriteLn(wxEmptyString);
}

void PHPCodeGenerator::GenSubclassSets(PObjectBase obj,
                                       std::set<wxString>* subclasses,
                                       std::vector<wxString>* headerIncludes)
{
    // Call GenSubclassForwardDeclarations on all children as well
    for (size_t i = 0; i < obj->GetChildCount(); i++)
        GenSubclassSets(obj->GetChild(i), subclasses, headerIncludes);

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
        // Now get the header
        std::map<wxString, wxString>::iterator header;
        header = children.find("header");

        if (children.end() == header) {
            // No header, so do nothing
            return;
        }
        wxString headerVal = header->second;
        if (headerVal.empty()) {
            // No header, so do nothing
            return;
        }
        // Got a header
        PObjectInfo info = obj->GetObjectInfo();
        if (!info)
            return;

        PObjectPackage pkg = info->GetPackage();
        if (!pkg)
            return;

        wxString include = "include_once " + headerVal + ";";
        std::vector<wxString>::iterator it
            = std::find(headerIncludes->begin(), headerIncludes->end(), include);
        if (headerIncludes->end() == it)
            headerIncludes->push_back(include);
    }
}

void PHPCodeGenerator::GenIncludes(PObjectBase project,
                                   std::vector<wxString>* includes,
                                   std::set<wxString>* templates)
{
    GenObjectIncludes(project, includes, templates);
}

void PHPCodeGenerator::GenObjectIncludes(PObjectBase project,
                                         std::vector<wxString>* includes,
                                         std::set<wxString>* templates)
{
    // Fill the set
    PCodeInfo codeInfo = project->GetObjectInfo()->GetCodeInfo("PHP");
    if (codeInfo) {
        PHPTemplateParser parser(project, codeInfo->GetTemplate("include"),
                                 m_useI18n, m_useRelativePath, m_basePath);
        wxString include = parser.ParseTemplate();
        if (!include.empty()) {
            if (templates->insert(include).second)
                AddUniqueIncludes(include, includes);
        }
    }
    // Call GenIncludes on all children as well
    for (size_t i = 0; i < project->GetChildCount(); i++)
        GenObjectIncludes(project->GetChild(i), includes, templates);

    // Generate includes for base classes
    GenBaseIncludes(project->GetObjectInfo(), project, includes, templates);
}

void PHPCodeGenerator::GenBaseIncludes(PObjectInfo info, PObjectBase obj,
                                       std::vector<wxString>* includes,
                                       std::set<wxString>* templates)
{
    if (!info)
        return;

    // Process all the base classes recursively
    for (size_t i = 0; i < info->GetBaseClassCount(); i++) {
        PObjectInfo base_info = info->GetBaseClass(i);
        GenBaseIncludes(base_info, obj, includes, templates);
    }
    PCodeInfo codeInfo = info->GetCodeInfo("PHP");
    if (codeInfo) {
        PHPTemplateParser parser(obj, codeInfo->GetTemplate("include"),
                                 m_useI18n, m_useRelativePath, m_basePath);
        wxString include = parser.ParseTemplate();
        if (!include.empty()) {
            if (templates->insert(include).second)
                AddUniqueIncludes(include, includes);
        }
    }
}

void PHPCodeGenerator::AddUniqueIncludes(const wxString& include,
                                         std::vector<wxString>* includes)
{
    // Split on newlines to only generate unique include lines
    // This strips blank lines and trims
    wxStringTokenizer tkz(include, "\n", wxTOKEN_STRTOK);

    while (tkz.HasMoreTokens()) {
        wxString line = tkz.GetNextToken();
        line.Trim(false);
        line.Trim(true);

        // If it is not an include line, it will be written
        if (!line.StartsWith("import")) {
            includes->push_back(line);
            continue;
        }
        // If it is an include, it must be unique to be written
        std::vector<wxString>::iterator it = std::find(includes->begin(), includes->end(), line);
        if (includes->end() == it) {
            includes->push_back(line);
        }
    }
}

void PHPCodeGenerator::FindDependencies(PObjectBase obj, std::set<PObjectInfo>& infoSet)
{
    size_t count = obj->GetChildCount();
    if (count) {
        for (size_t i = 0; i < count; i++) {
            PObjectBase child = obj->GetChild(i);
            infoSet.insert(child->GetObjectInfo());
            FindDependencies(child, infoSet);
        }
    }
}

void PHPCodeGenerator::GenConstructor(PObjectBase classObj,
                                      const EventVector& events, ArrayItems& arrays)
{
    m_source->WriteLn();
    // generate function definition
    m_source->WriteLn(GetCode(classObj, "cons_def"));
    m_source->Indent();

    m_source->WriteLn(GetCode(classObj, "cons_call"));
    m_source->WriteLn();

    wxString settings = GetCode(classObj, "settings");
    if (!settings.IsEmpty())
        m_source->WriteLn(settings);

    for (size_t i = 0; i < classObj->GetChildCount(); i++)
        GenConstruction(classObj->GetChild(i), true, arrays);

    wxString afterAddChild = GetCode(classObj, "after_addchild");
    if (!afterAddChild.IsEmpty())
        m_source->WriteLn(afterAddChild);

    GenEvents(classObj, events);

    m_source->Unindent();
    m_source->WriteLn("}");
    m_source->WriteLn(wxEmptyString);

    if (classObj->GetTypeName() == "wizard"
        && classObj->GetChildCount() > 0) {
        m_source->WriteLn("function AddPage($page){");
        m_source->Indent();
        m_source->WriteLn("if(count($this->m_pages) > 0){");
        m_source->Indent();
        m_source->WriteLn("$previous_page = $this->m_pages[count($this->m_pages)-1];");
        m_source->WriteLn("$page->SetPrev($previous_page);");
        m_source->WriteLn("$previous_page->SetNext($page);");
        m_source->Unindent();
        m_source->WriteLn("}");
        m_source->WriteLn("$this->m_pages[] = $page;");
        m_source->Unindent();
        m_source->WriteLn("}");
    }
}

void PHPCodeGenerator::GenDestructor(PObjectBase classObj, const EventVector& events)
{
    m_source->WriteLn();
    m_source->WriteLn("function __destruct( ){"); // generate function definition
    m_source->Indent();

    if (m_disconnectEvents && !events.empty())
        GenEvents(classObj, events, true);

    // destruct objects
    GenDestruction(classObj);

    m_source->Unindent();
    m_source->WriteLn("}");
}

void PHPCodeGenerator::GenConstruction(PObjectBase obj, bool is_widget, ArrayItems& arrays)
{
    wxString type = obj->GetTypeName();
    PObjectInfo info = obj->GetObjectInfo();

    if (ObjectDatabase::HasCppProperties(type)) {
        m_source->WriteLn(GetConstruction(obj, arrays));

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
            if (!afterAddChild.empty()) {
                m_source->WriteLn(afterAddChild);
            }
            m_source->WriteLn();

            if (is_widget) {
                /*
                     the parent object is not a sizer.
                     There is no template for this so we'll make it manually.
                     It's not a good practice to embed templates into the source code,
                     because you will need to recompile...
                */
                wxString _template = "#wxparent $name->SetSizer( @$$name ); #nl"
                                     "#wxparent $name->Layout();"
                                     "#ifnull #parent $size"
                                     "@{ #nl @$$name->Fit( #wxparent $name ); @}";

                PHPTemplateParser parser(obj, _template, m_useI18n,
                                         m_useRelativePath, m_basePath);
                m_source->WriteLn(parser.ParseTemplate());
            }
        } else if (type == "splitter") {
            // Generating the split
            switch (obj->GetChildCount()) {
            case 1: {
                PObjectBase sub1 = obj->GetChild(0)->GetChild(0);
                wxString _template = "@$this->$name->Initialize(@$this->"
                    + sub1->GetProperty("name")->GetValueAsString() + ");";

                PHPTemplateParser parser(obj, _template, m_useI18n,
                                         m_useRelativePath, m_basePath);
                m_source->WriteLn(parser.ParseTemplate());
                break;
            }
            case 2: {
                PObjectBase sub1, sub2;
                sub1 = obj->GetChild(0)->GetChild(0);
                sub2 = obj->GetChild(1)->GetChild(0);

                wxString _template;
                if (obj->GetProperty("splitmode")->GetValueAsString() == "wxSPLIT_VERTICAL")
                    _template = "@$this->$name->SplitVertically(";
                else
                    _template = "@$this->$name->SplitHorizontally(";

                _template = _template + "@$this->"
                    + sub1->GetProperty("name")->GetValueAsString()
                    + ", @$this->"
                    + sub2->GetProperty("name")->GetValueAsString()
                    + ", $sashpos);";

                PHPTemplateParser parser(obj, _template, m_useI18n,
                                         m_useRelativePath, m_basePath);
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
                   || type == "toolbar"
                   || type == "ribbonbar"
                   || type == "listbook"
                   || type == "simplebook"
                   || type == "notebook"
                   || type == "auinotebook"
                   || type == "treelistctrl"
                   || type == "wizard") {
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
        wxString temp_name;
        if (childInfo->IsSubclassOf("wxWindow")
            || childInfo->GetClassName() == "CustomControl") {
            temp_name = "window_add";
        } else if (childInfo->IsSubclassOf("sizer")) {
            temp_name = "sizer_add";
        } else if (childInfo->GetClassName() == "spacer") {
            temp_name = "spacer_add";
        } else {
            LogDebug(
                "SizerItem child is not a Spacer and is not a subclass of wxWindow or of sizer.");
            return;
        }
        m_source->WriteLn(GetCode(obj, temp_name));
    } else if (type == "notebookpage"
               || type == "listbookpage"
               || type == "simplebookpage"
               || type == "choicebookpage"
               || type == "auinotebookpage") {
        GenConstruction(obj->GetChild(0), false, arrays);
        m_source->WriteLn(GetCode(obj, "page_add"));
        GenSettings(obj->GetObjectInfo(), obj);
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
                if (source == "Load From Icon Resource"
                    && toolsize == wxDefaultSize) {
                    prop->SetValue(wxString::Format(
                        "%s; %s [%i; %i]", path.c_str(), source.c_str(),
                        toolbarsize.GetWidth(), toolbarsize.GetHeight()));
                    m_source->WriteLn(GetConstruction(obj, arrays));
                    prop->SetValue(oldVal);
                    return;
                }
            }
        }
        m_source->WriteLn(GetConstruction(obj, arrays));
    } else {
        // Generate the children
        for (size_t i = 0; i < obj->GetChildCount(); i++)
            GenConstruction(obj->GetChild(i), false, arrays);
    }
}

void PHPCodeGenerator::GenDestruction(PObjectBase obj)
{
    PCodeInfo codeInfo = obj->GetObjectInfo()->GetCodeInfo("PHP");
    if (codeInfo) {
        wxString _template = codeInfo->GetTemplate("destruction");
        if (!_template.empty()) {
            PHPTemplateParser parser(obj, _template,
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

void PHPCodeGenerator::FindMacros(PObjectBase obj, std::vector<wxString>* macros)
{
    // iterate through all of the properties of all objects,
    // add the macros to the vector
    for (size_t i = 0; i < obj->GetPropertyCount(); i++) {
        PProperty prop = obj->GetProperty(i);
        if (prop->GetType() == PT_MACRO) {
            wxString value = prop->GetValueAsString();
            if (value.IsEmpty())
                continue;

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

void PHPCodeGenerator::FindEventHandlers(PObjectBase obj, EventVector& events)
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

void PHPCodeGenerator::GenDefines(PObjectBase project)
{
    std::vector<wxString> macros;
    FindMacros(project, &macros);

    // Remove the default macro from the set, for backward compatibility
    std::vector<wxString>::iterator it
        = std::find(macros.begin(), macros.end(), "ID_DEFAULT");
    if (it != macros.end()) {
        // The default macro is defined to wxID_ANY
        m_source->WriteLn("const ID_DEFAULT = wxID_ANY; // Default");
        macros.erase(it);
    }
    size_t id = m_firstID;
    if (id < 1000)
        wxLogWarning("First ID is Less than 1000");

    for (it = macros.begin(); it != macros.end(); it++) {
        // Don't redefine wx IDs
        m_source->WriteLn(wxString::Format("const %s = %z;", it->c_str(), id));
        id++;
    }
    if (!macros.empty())
        m_source->WriteLn(wxEmptyString);
}

void PHPCodeGenerator::GenSettings(PObjectInfo info, PObjectBase obj)
{
    PCodeInfo codeInfo = info->GetCodeInfo("PHP");
    if (!codeInfo)
        return;

    wxString _template = codeInfo->GetTemplate("settings");
    if (!_template.empty()) {
        PHPTemplateParser parser(obj, _template, m_useI18n, m_useRelativePath, m_basePath);
        wxString code = parser.ParseTemplate();
        if (!code.empty())
            m_source->WriteLn(code);
    }
    // Proceeding recursively with the base classes
    for (size_t i = 0; i < info->GetBaseClassCount(); i++) {
        PObjectInfo base_info = info->GetBaseClass(i);
        GenSettings(base_info, obj);
    }
}

void PHPCodeGenerator::GenAddToolbar(PObjectInfo info, PObjectBase obj)
{
    wxArrayString arrCode;
    GetAddToolbarCode(info, obj, arrCode);
    for (size_t i = 0; i < arrCode.GetCount(); i++)
        m_source->WriteLn(arrCode[i]);
}

void PHPCodeGenerator::GetAddToolbarCode(PObjectInfo info, PObjectBase obj,
                                         wxArrayString& codelines)
{
    PCodeInfo codeInfo = info->GetCodeInfo("PHP");
    if (!codeInfo)
        return;

    wxString _template = codeInfo->GetTemplate("toolbar_add");
    if (!_template.empty()) {
        PHPTemplateParser parser(obj, _template, m_useI18n,
                                 m_useRelativePath, m_basePath);
        wxString code = parser.ParseTemplate();
        if (!code.empty()) {
            if (codelines.Index(code) == wxNOT_FOUND)
                codelines.Add(code);
        }
    }
    // Proceeding recursively with the base classes
    for (size_t i = 0; i < info->GetBaseClassCount(); i++) {
        PObjectInfo base_info = info->GetBaseClass(i);
        GetAddToolbarCode(base_info, obj, codelines);
    }
}

void PHPCodeGenerator::UseRelativePath(bool relative, wxString basePath)
{
    if (relative) {
        if (wxFileName::DirExists(basePath))
            m_basePath = basePath;
        else
            m_basePath = wxEmptyString;
    }
    m_useRelativePath = relative;
}

void PHPCodeGenerator::SetupPredefinedMacros()
{
#define ADD_PREDEFINED_MACRO(x) m_predMacros.insert(#x)
#define ADD_PREDEFINED_PREFIX(k, v) m_predModulePrefix[#k] = #v

    // No id matches this one when compared to it
    ADD_PREDEFINED_MACRO(wxID_NONE);

    // Id for a separator line in the menu (invalid for normal item)
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

    // Standard button and menu IDs
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

    // System menu IDs (used by wxUniv)
    ADD_PREDEFINED_MACRO(wxID_SYSTEM_MENU);
    ADD_PREDEFINED_MACRO(wxID_CLOSE_FRAME);
    ADD_PREDEFINED_MACRO(wxID_MOVE_FRAME);
    ADD_PREDEFINED_MACRO(wxID_RESIZE_FRAME);
    ADD_PREDEFINED_MACRO(wxID_MAXIMIZE_FRAME);
    ADD_PREDEFINED_MACRO(wxID_ICONIZE_FRAME);
    ADD_PREDEFINED_MACRO(wxID_RESTORE_FRAME);

    // IDs used by generic file dialog (13 consecutive starting from this value)
    ADD_PREDEFINED_MACRO(wxID_FILEDLGG);

    ADD_PREDEFINED_MACRO(wxID_HIGHEST);

#undef ADD_PREDEFINED_MACRO
#undef ADD_PREDEFINED_PREFIX
}
