/*
    wxWeaver - A GUI Designer Editor for wxWidgets.
    Copyright (C) 2005 José Antonio Hurtado
    Copyright (C) 2005 Juan Antonio Ortega
    Copyright (C) 2009 Michal Bližňák (as wxFormBuilder)
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
#include "codegen/pythoncg.h"

#include "model/objectbase.h"
#include "appdata.h"
#include "utils/debug.h"
#include "utils/typeconv.h"
#include "utils/exception.h"
#include "codegen/codewriter.h"

#include <wx/filename.h>
#include <wx/tokenzr.h>

#include <algorithm>

PythonTemplateParser::PythonTemplateParser(PObjectBase obj, wxString _template,
                                           bool useI18N, bool useRelativePath,
                                           wxString basePath,
                                           wxString imagePathWrapperFunctionName)
    : TemplateParser(obj, _template)
    , m_basePath(basePath)
    , m_imagePathWrapperFunctionName(imagePathWrapperFunctionName)
    , m_useI18n(useI18N)
    , m_useRelativePath(useRelativePath)
{
    if (!wxFileName::DirExists(m_basePath))
        m_basePath.clear();

    SetupModulePrefixes();
}

PythonTemplateParser::PythonTemplateParser(const PythonTemplateParser& other,
                                           wxString _template)
    : TemplateParser(other, _template)
    , m_basePath(other.m_basePath)
    , m_imagePathWrapperFunctionName(other.m_imagePathWrapperFunctionName)
    , m_useI18n(other.m_useI18n)
    , m_useRelativePath(other.m_useRelativePath)
{
    SetupModulePrefixes();
}

wxString PythonTemplateParser::RootWxParentToCode()
{
    return "self";
}

PTemplateParser PythonTemplateParser::CreateParser(const TemplateParser* oldparser,
                                                   wxString _template)
{
    const PythonTemplateParser* pythonOldParser
        = dynamic_cast<const PythonTemplateParser*>(oldparser);
    if (pythonOldParser) {
        PTemplateParser newparser(new PythonTemplateParser(*pythonOldParser, _template));
        return newparser;
    }
    return PTemplateParser();
}

wxString PythonTemplateParser::ValueToCode(PropertyType type, wxString value)
{
    wxString result;

    switch (type) {
    case PT_WXPARENT: {
        result = "self." + value;
        break;
    }
    case PT_WXPARENT_SB: {
        result = value + ".GetStaticBox()";
        break;
    }
    case PT_WXPARENT_CP: {
        result = "self." + value + ".GetPane()";
        break;
    }
    case PT_WXSTRING:
    case PT_FILE:
    case PT_PATH: {
        if (value.empty())
            result << "wx.EmptyString";
        else
            result << "u\"" << PythonCodeGenerator::ConvertPythonString(value) << "\"";

        break;
    }
    case PT_WXSTRING_I18N: {
        if (value.empty()) {
            result << "wx.EmptyString";
        } else {
            if (m_useI18n)
                result << "_(u\"" << PythonCodeGenerator::ConvertPythonString(value) << "\")";
            else
                result << "u\"" << PythonCodeGenerator::ConvertPythonString(value) << "\"";
        }
        break;
    }
    case PT_CLASS:
    case PT_MACRO:
    case PT_OPTION:
    case PT_EDIT_OPTION: {
        result = value;
        wxString pred = m_predModulePrefix[value];
        if (!pred.empty())
            result.Replace("wx", pred);
        else {
            if (result.StartsWith("XRCID"))
                result.Prepend("wx.xrc.");
            else
                result.Replace("wx", "wx.");
        }
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
        wxString pred, bit;
        wxStringTokenizer bits(result, "|", wxTOKEN_STRTOK);

        while (bits.HasMoreTokens()) {
            bit = bits.GetNextToken();
            pred = m_predModulePrefix[bit];

            if (bit.Contains("wx")) {
                if (!pred.empty())
                    result.Replace(bit, pred + bit.AfterFirst('x'));
                else
                    result.Replace(bit, "wx." + bit.AfterFirst('x'));
            }
        }
        break;
    }
    case PT_WXPOINT: {
        if (value.empty())
            result = "wx.DefaultPosition";
        else
            result << "wx.Point(" << value << ")";

        break;
    }
    case PT_WXSIZE: {
        if (value.empty())
            result = "wx.DefaultSize";
        else
            result << "wx.Size(" << value << ")";

        break;
    }
    case PT_BOOL: {
        result = (value == "0" ? "False" : "True");
        break;
    }
    case PT_WXFONT: {
        if (!value.empty()) {
            wxFontContainer fontContainer = TypeConv::StringToFont(value);
            wxFont font = fontContainer.GetFont();
            const int pointSize = fontContainer.GetPointSize();
            result = wxString::Format(
                "wx.Font( %s, %s, %s, %s, %s, %s )",
                ((pointSize <= 0)
                     ? "wx.NORMAL_FONT.GetPointSize()"
                     : (wxString() << pointSize)),
                TypeConv::FontFamilyToString(fontContainer.GetFamily()).replace(0, 2, "wx."),
                font.GetStyleString().replace(0, 2, "wx."),
                font.GetWeightString().replace(0, 2, "wx."),
                (fontContainer.GetUnderlined() ? "True" : "False"),
                (fontContainer.m_faceName.empty()
                     ? "wx.EmptyString"
                     : ("\"" + fontContainer.m_faceName + "\"")));
        } else {
            result = "wx.NORMAL_FONT";
        }
        break;
    }
    case PT_WXCOLOUR: {
        if (!value.empty()) {
            if (!value.find_first_of("wx")) {
                // System Colour
                result << "wx.SystemSettings.GetColour("
                       << ValueToCode(PT_OPTION, value) << ")";
            } else {
                wxColour colour = TypeConv::StringToColour(value);
                result = wxString::Format(
                    "wx.Colour(%i, %i, %i)",
                    colour.Red(), colour.Green(), colour.Blue());
            }
        } else {
            result = "wx.Colour()";
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
            result = "wx.NullBitmap";
            break;
        }
        if (path.StartsWith("file:")) {
            wxLogWarning("Python code generation does not support using URLs"
                         "for bitmap properties:\n%s",
                         path.c_str());
            result = "wx.NullBitmap";
            break;
        }
        if (source == _("Load From File") || source == _("Load From Embedded File")) {
            wxString absPath;
            try {
                absPath = TypeConv::MakeAbsolutePath(path, AppData()->GetProjectPath());
            } catch (wxWeaverException& ex) {
                wxLogError(ex.what());
                result = "wx.NullBitmap";
                break;
            }
            wxString file
                = (m_useRelativePath
                       ? TypeConv::MakeRelativePath(absPath, m_basePath)
                       : absPath);

            result << "wx.Bitmap(";
            if (!m_imagePathWrapperFunctionName.empty()) {
                result << "self." << m_imagePathWrapperFunctionName << "(";
            }
            result << "u\""
                   << PythonCodeGenerator::ConvertPythonString(file) << "\"";
            if (!m_imagePathWrapperFunctionName.empty()) {
                result << ")";
            }
            result << ", wx.BITMAP_TYPE_ANY)";

        } else if (source == _("Load From Resource")) {
            result << "wx.Bitmap(u\"" << path << "\", wx.BITMAP_TYPE_RESOURCE)";
        } else if (source == _("Load From Icon Resource")) {
            if (wxDefaultSize == icoSize) {
                result << "wx.ICON(" << path << ")";
            } else {
                result.Printf(
                    "wx.Icon(u\"%s\", wx.BITMAP_TYPE_ICO_RESOURCE, %i, %i)",
                    path.c_str(), icoSize.GetWidth(),
                    icoSize.GetHeight());
            }
        } else if (source == _("Load From XRC")) {
            // NOTE: The module wx.xrc is part of the default code template
            result << "wx.xrc.XmlResource.Get().LoadBitmap(u\"" << path << "\")";
        } else if (source == _("Load From Art Provider")) {
            wxString rid = path.BeforeFirst(':');

            if (rid.StartsWith("gtk-"))
                rid = "u\"" + rid + "\"";
            else
                rid.Replace("wx", "wx.");

            wxString cid = path.AfterFirst(':');
            cid.Replace("wx", "wx.");

            result = "wx.ArtProvider.GetBitmap(" + rid + ", " + cid + ")";
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

PythonCodeGenerator::PythonCodeGenerator()
{
    SetupPredefinedMacros();
    m_useRelativePath = false;
    m_useI18n = false;
    m_firstID = 1000;
}

wxString PythonCodeGenerator::ConvertPythonString(wxString text)
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

void PythonCodeGenerator::GenerateInheritedClass(PObjectBase userClasses, PObjectBase form)
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
                    "def %s(self, event):", event->GetValue().c_str()));
                m_source->Indent();
                m_source->WriteLn(wxString::Format(
                    "# TODO: Implement %s", event->GetValue().c_str()));
                m_source->WriteLn("pass");
                m_source->Unindent();
                m_source->WriteLn(wxEmptyString);
                generatedHandlers.insert(event->GetValue());
            }
        }
        m_source->WriteLn(wxEmptyString);
    }
    m_source->Unindent();
}

bool PythonCodeGenerator::GenerateCode(PObjectBase project)
{
    if (!project) {
        wxLogError("There is no project to generate code");
        return false;
    }

    m_useI18n = false;
    PProperty i18nProperty = project->GetProperty("internationalize");
    if (i18nProperty && i18nProperty->GetValueAsInteger())
        m_useI18n = true;

    m_disconnectEvents = (project->GetPropertyAsInteger("disconnect_python_events"));

    m_source->Clear();

    // Insert python preamble

    wxString code = GetCode(project, "python_preamble");
    if (!code.empty())
        m_source->WriteLn(code);

    code = wxString::Format(
        "#\n"
        "#    Python code generated with wxWeaver (version %s%s " __DATE__ ")\n"
        "#    https://wxweaver.github.io/\n"
        "#\n"
        "#    PLEASE DO *NOT* EDIT THIS FILE!\n"
        "#",
        VERSION, REVISION);

    m_source->WriteLn(code);

    PProperty propFile = project->GetProperty("file");
    if (!propFile) {
        wxLogError("Missing \"file\" property on Project Object");
        return false;
    }
    wxString file = propFile->GetValue();
    if (file.empty()) {
        file = "noname";
    }
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
         includeIt != headerIncludes.end(); ++includeIt) {
        m_source->WriteLn(*includeIt);
    }
    if (!headerIncludes.empty())
        m_source->WriteLn(wxEmptyString);

    // Write internationalization support
    if (m_useI18n) {
        m_source->WriteLn("import gettext");
        m_source->WriteLn("_ = gettext.gettext");
        m_source->WriteLn(wxEmptyString);
    }
    // Generating "defines" for macros
    GenDefines(project);

    wxString eventHandlerPostfix;
    PProperty eventKindProp = project->GetProperty("skip_python_events");
    if (eventKindProp->GetValueAsInteger())
        eventHandlerPostfix = "event.Skip()";
    else
        eventHandlerPostfix = "pass";

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
        GenClassDeclaration(child, useEnum, classDecoration, events, eventHandlerPrefix, eventHandlerPostfix);
#endif
        GenClassDeclaration(child, false, wxEmptyString, events, eventHandlerPostfix, arrays);
    }
    code = GetCode(project, "python_epilogue");
    if (!code.empty())
        m_source->WriteLn(code);

    return true;
}

void PythonCodeGenerator::GenEvents(PObjectBase classObj, const EventVector& events, bool disconnect)
{
    if (events.empty())
        return;

    if (disconnect) {
        m_source->WriteLn("# Disconnect Events");
    } else {
        m_source->WriteLn();
        m_source->WriteLn("# Connect Events");
    }
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
    wxString baseClass;
    wxString handlerName;
    PProperty propSubclass = classObj->GetProperty("subclass");
    if (propSubclass) {
        wxString subclass = propSubclass->GetChildFromParent("name");
        if (!subclass.empty()) {
            baseClass = subclass;
        }
    }
    if (baseClass.empty())
        baseClass = "wx." + classObj->GetClassName();

    if (events.size() > 0) {
        for (size_t i = 0; i < events.size(); i++) {
            PEvent event = events[i];
            handlerName = event->GetValue();
            wxString templateName = wxString::Format("connect_%s", event->GetName().c_str());
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

bool PythonCodeGenerator::GenEventEntry(PObjectBase obj, PObjectInfo objInfo,
                                        const wxString& templateName,
                                        const wxString& handlerName, bool disconnect)
{
    PCodeInfo codeInfo = objInfo->GetCodeInfo("Python");
    if (codeInfo) {
        wxString _template = codeInfo->GetTemplate(wxString::Format(
            "evt_%s%s", disconnect ? wxS("dis") : wxEmptyString, templateName.c_str()));
        if (disconnect && _template.empty()) {
            _template = codeInfo->GetTemplate("evt_") + templateName;
            _template.Replace("Bind", "Unbind", true);
        }
        if (!_template.empty()) {
            if (disconnect) {
                if (m_disconnecMode == "handler_name")
                    _template.Replace("#handler",
                                      "handler = self." + handlerName);
                else if (m_disconnecMode == "source_name")
                    _template.Replace("#handler",
                                      "None"); //"self." + obj->GetProperty("name")->GetValueAsString());
            } else
                _template.Replace("#handler", "self." + handlerName);

            PythonTemplateParser parser(obj, _template, m_useI18n, m_useRelativePath,
                                        m_basePath, m_imagePathWrapperFunctionName);
            m_source->WriteLn(parser.ParseTemplate());
            return true;
        }
    }
    for (size_t i = 0; i < objInfo->GetBaseClassCount(false); i++) {
        PObjectInfo base_info = objInfo->GetBaseClass(i, false);
        if (GenEventEntry(obj, base_info, templateName, handlerName, disconnect))
            return true;
    }
    return false;
}

void PythonCodeGenerator::GenVirtualEventHandlers(const EventVector& events,
                                                  const wxString& eventHandlerPostfix)
{
    if (events.size() > 0) {
        /*
            There are problems if we create "pure" virtual handlers,
            because some events could be triggered in the constructor
            in which virtual methods are executed properly.
            So we create a default handler which will skip the event.
        */
        m_source->WriteLn(wxEmptyString);
        m_source->WriteLn("# Virtual event handlers, overide them in your derived class");

        std::set<wxString> generatedHandlers;
        for (size_t i = 0; i < events.size(); i++) {
            PEvent event = events[i];
            wxString aux = "def " + event->GetValue() + "(self, event):";

            if (generatedHandlers.find(aux) == generatedHandlers.end()) {
                m_source->WriteLn(aux);
                m_source->Indent();
                m_source->WriteLn(eventHandlerPostfix);
                m_source->Unindent();

                generatedHandlers.insert(aux);
            }
            if (i < (events.size() - 1))
                m_source->WriteLn();
        }
        m_source->WriteLn(wxEmptyString);
    }
}

void PythonCodeGenerator::GetGenEventHandlers(PObjectBase obj)
{
    GenDefinedEventHandlers(obj->GetObjectInfo(), obj);
    for (size_t i = 0; i < obj->GetChildCount(); i++) {
        PObjectBase child = obj->GetChild(i);
        GetGenEventHandlers(child);
    }
}

void PythonCodeGenerator::GenDefinedEventHandlers(PObjectInfo info, PObjectBase obj)
{
    PCodeInfo codeInfo = info->GetCodeInfo("Python");
    if (codeInfo) {
        wxString _template = codeInfo->GetTemplate("generated_event_handlers");
        if (!_template.empty()) {
            PythonTemplateParser parser(obj, _template, m_useI18n, m_useRelativePath,
                                        m_basePath, m_imagePathWrapperFunctionName);
            wxString code = parser.ParseTemplate();
            if (!code.empty())
                m_source->WriteLn(code);
        }
    }
    // Proceeding recursively with the base classes
    for (size_t i = 0; i < info->GetBaseClassCount(false); i++) {
        PObjectInfo base_info = info->GetBaseClass(i, false);
        GenDefinedEventHandlers(base_info, obj);
    }
}

void PythonCodeGenerator::GenImagePathWrapperFunction()
{
    if (!m_imagePathWrapperFunctionName.empty()) {
        m_source->WriteLn("# Virtual image path resolution method. Override this in your derived class.");
        wxString decl = "def " + m_imagePathWrapperFunctionName + "(self, bitmap_path):";
        m_source->WriteLn(decl);
        m_source->Indent();
        m_source->WriteLn("return bitmap_path");
        m_source->WriteLn("");
        m_source->Unindent();
    }
}

wxString PythonCodeGenerator::GetCode(PObjectBase obj, wxString name, bool silent)
{
    PCodeInfo codeInfo = obj->GetObjectInfo()->GetCodeInfo("Python");
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
    PythonTemplateParser parser(obj, _template, m_useI18n, m_useRelativePath,
                                m_basePath, m_imagePathWrapperFunctionName);
    wxString code = parser.ParseTemplate();
    return code;
}

wxString PythonCodeGenerator::GetConstruction(PObjectBase obj, bool silent,
                                              ArrayItems& arrays)
{
    // Get the name
    const auto& propName = obj->GetProperty("name");
    if (!propName) {
        // Object has no name, just get its code
        return GetCode(obj, "construction", silent);
    }
    // Object has a name, check if its an array
    const auto& name = propName->GetValue();
    wxString baseName;
    ArrayItem unused;
    if (!ParseArrayName(name, baseName, unused)) {
        // Object is not an array, just get its code
        return GetCode(obj, "construction", silent);
    }
    // Object is an array, check if it needs to be declared
    auto& item = arrays[baseName];
    if (item.isDeclared) {
        // Object is already declared, just get its code
        return GetCode(obj, "construction", silent);
    }
    // Array needs to be declared
    // Base array
    wxString code;
    code.append("self.");
    code.append(baseName);
    code.append(" = {}\n");

    // Need to fill all dimensions up to the last
    if (item.maxIndex.size() > 1) {
        std::vector<wxString> stackCurrent;
        std::vector<wxString> stackNext;

        stackCurrent.push_back(baseName);
        for (size_t dimension = 0; dimension < item.maxIndex.size() - 1; ++dimension) {
            const auto size = item.maxIndex[dimension] + 1;

            stackNext.reserve(stackCurrent.size() * size);
            for (const auto& array : stackCurrent) {
                for (size_t index = 0; index < size; ++index) {
                    const auto targetName = wxString::Format("%s[%z]", array, index);

                    code.append("self.");
                    code.append(targetName);
                    code.append(" = {}\n");

                    stackNext.push_back(targetName);
                }
            }
            stackCurrent.swap(stackNext);
            stackNext.clear();
        }
    }
    // Get the Code
    code.append(GetCode(obj, "construction", silent));

    // Mark the array as declared
    item.isDeclared = true;
    return code;
}

void PythonCodeGenerator::GenClassDeclaration(PObjectBase classObj, bool /*useEnum*/,
                                              const wxString& classDecoration,
                                              const EventVector& events,
                                              const wxString& eventHandlerPostfix,
                                              ArrayItems& arrays)
{
    PProperty propName = classObj->GetProperty("name");
    if (!propName) {
        wxLogError("Missing \"name\" property on \"%s\" class. Review your XML object description",
                   classObj->GetClassName().c_str());
        return;
    }
    wxString className = propName->GetValue();
    if (className.empty()) {
        wxLogError("Object name can not be null");
        return;
    }
#if 0
    m_source->WriteLn("###########################################################################"));
    m_source->WriteLn("## Class ") + className);
    m_source->WriteLn("###########################################################################"));
    m_source->WriteLn();
#endif
    m_source->WriteLn("class " + classDecoration + className + "("
                      + GetCode(classObj, "base").Trim() + "):");
    m_source->Indent();

    // The constructor is also included within public
    GenConstructor(classObj, events, arrays);
    GenDestructor(classObj, events);

    m_source->WriteLn(wxEmptyString);

    // event handlers
    GenVirtualEventHandlers(events, eventHandlerPostfix);
    GetGenEventHandlers(classObj);

    // Bitmap wrapper code
    GenImagePathWrapperFunction();

    m_source->Unindent();
    m_source->WriteLn(wxEmptyString);
}

void PythonCodeGenerator::GenSubclassSets(PObjectBase obj, std::set<wxString>* subclasses, std::vector<wxString>* headerIncludes)
{
    // Call GenSubclassForwardDeclarations on all children as well
    for (size_t i = 0; i < obj->GetChildCount(); i++)
        GenSubclassSets(obj->GetChild(i), subclasses, headerIncludes);

    // Fill the set
    PProperty subclass = obj->GetProperty("subclass");
    if (!subclass)
        return;

    std::map<wxString, wxString> children;
    subclass->SplitParentProperty(&children);

    std::map<wxString, wxString>::iterator name;
    name = children.find("name");
    if (children.end() == name)
        return; // No name, so do nothing

    wxString nameVal = name->second;
    if (nameVal.empty())
        return; // No name, so do nothing

    // Now get the header
    std::map<wxString, wxString>::iterator header;
    header = children.find("header");
    if (children.end() == header)
        return; // No header, so do nothing

    wxString headerVal = header->second;
    if (headerVal.empty())
        return; // No header, so do nothing

    // Got a header
    PObjectInfo info = obj->GetObjectInfo();
    if (!info)
        return;

    PObjectPackage pkg = info->GetPackage();
    if (!pkg)
        return;

    wxString include = "from " + headerVal + " import " + nameVal;
    std::vector<wxString>::iterator it
        = std::find(headerIncludes->begin(), headerIncludes->end(), include);
    if (headerIncludes->end() == it)
        headerIncludes->push_back(include);
}

void PythonCodeGenerator::GenIncludes(PObjectBase project,
                                      std::vector<wxString>* includes,
                                      std::set<wxString>* templates)
{
    GenObjectIncludes(project, includes, templates);
}

void PythonCodeGenerator::GenObjectIncludes(PObjectBase project,
                                            std::vector<wxString>* includes,
                                            std::set<wxString>* templates)
{
    // Fill the set
    PCodeInfo codeInfo = project->GetObjectInfo()->GetCodeInfo("Python");
    if (codeInfo) {
        PythonTemplateParser parser(project, codeInfo->GetTemplate("include"),
                                    m_useI18n, m_useRelativePath, m_basePath,
                                    m_imagePathWrapperFunctionName);
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

void PythonCodeGenerator::GenBaseIncludes(PObjectInfo info, PObjectBase obj,
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
    PCodeInfo codeInfo = info->GetCodeInfo("Python");
    if (codeInfo) {
        PythonTemplateParser parser(obj, codeInfo->GetTemplate("include"),
                                    m_useI18n, m_useRelativePath, m_basePath,
                                    m_imagePathWrapperFunctionName);
        wxString include = parser.ParseTemplate();
        if (!include.empty()) {
            if (templates->insert(include).second)
                AddUniqueIncludes(include, includes);
        }
    }
}

void PythonCodeGenerator::AddUniqueIncludes(const wxString& include,
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
        if (includes->end() == it)
            includes->push_back(line);
    }
}

void PythonCodeGenerator::FindDependencies(PObjectBase obj, std::set<PObjectInfo>& infoSet)
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

void PythonCodeGenerator::GenConstructor(PObjectBase classObj,
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

    if (classObj->GetTypeName() == "wizard"
        && classObj->GetChildCount() > 0) {
        m_source->WriteLn("def add_page(self, page):");
        m_source->Indent();
        m_source->WriteLn("if self.m_pages:");
        m_source->Indent();
        m_source->WriteLn("previous_page = self.m_pages[-1]");
        m_source->WriteLn("page.SetPrev(previous_page)");
        m_source->WriteLn("previous_page.SetNext(page)");
        m_source->Unindent();
        m_source->WriteLn("self.m_pages.append(page)");
        m_source->Unindent();
    }
}

void PythonCodeGenerator::GenDestructor(PObjectBase classObj,
                                        const EventVector& events)
{
    m_source->WriteLn();
    m_source->WriteLn("def __del__(self):"); // generate function definition
    m_source->Indent();

    if (m_disconnectEvents && !events.empty())
        GenEvents(classObj, events, true);
    else if (!classObj->GetPropertyAsInteger("aui_managed"))
        m_source->WriteLn("pass");

    GenDestruction(classObj); // destruct objects
    m_source->Unindent();
}

void PythonCodeGenerator::GenConstruction(PObjectBase obj, bool is_widget,
                                          ArrayItems& arrays)
{
    wxString type = obj->GetTypeName();
    PObjectInfo info = obj->GetObjectInfo();
    if (ObjectDatabase::HasCppProperties(type)) {
        m_source->WriteLn(GetConstruction(obj, false, arrays));
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
                wxString _template = "#wxparent $name.SetSizer( $name ) #nl"
                                     "#wxparent $name.Layout()"
                                     "#ifnull #parent $size"
                                     "@{ #nl $name.Fit( #wxparent $name ) @}";

                PythonTemplateParser parser(obj, _template, m_useI18n,
                                            m_useRelativePath, m_basePath,
                                            m_imagePathWrapperFunctionName);
                m_source->WriteLn(parser.ParseTemplate());
            }
        } else if (type == "splitter") {
            // Generating the split
            switch (obj->GetChildCount()) {
            case 1: {
                PObjectBase sub1 = obj->GetChild(0)->GetChild(0);
                wxString _template = "self.$name.Initialize(";
                _template = _template + "self."
                    + sub1->GetProperty("name")->GetValue() + ")";

                PythonTemplateParser parser(obj, _template, m_useI18n,
                                            m_useRelativePath, m_basePath,
                                            m_imagePathWrapperFunctionName);
                m_source->WriteLn(parser.ParseTemplate());
                break;
            }
            case 2: {
                PObjectBase sub1, sub2;
                sub1 = obj->GetChild(0)->GetChild(0);
                sub2 = obj->GetChild(1)->GetChild(0);

                wxString _template;
                if (obj->GetProperty("splitmode")->GetValue() == "wxSPLIT_VERTICAL")
                    _template = "self.$name.SplitVertically(";
                else
                    _template = "self.$name.SplitHorizontally(";

                _template = _template + "self."
                    + sub1->GetProperty("name")->GetValue()
                    + ", self." + sub2->GetProperty("name")->GetValue()
                    + ", $sashpos)";

                PythonTemplateParser parser(obj, _template, m_useI18n,
                                            m_useRelativePath, m_basePath,
                                            m_imagePathWrapperFunctionName);
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
            LogDebug("SizerItem child is not a Spacer and is not a subclass of wxWindow or of sizer.");
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
    } else if (type == "toolbookpage") {
        GenConstruction(obj->GetChild(0), false, arrays);
        GenSettings(obj->GetObjectInfo(), obj);
        m_source->WriteLn(GetCode(obj, "page_add"));
    } else if (type == "treebookpage") {
        GenConstruction(obj->GetChild(0), false, arrays);

        int depth = obj->GetPropertyAsInteger("depth");
        if (!depth) {
            m_source->WriteLn(GetCode(obj, "page_add"));
        } else if (depth > 0) {
            m_source->WriteLn(GetCode(obj, "subpage_add"));
        }
        GenSettings(obj->GetObjectInfo(), obj);
    } else if (type == "treelistctrlcolumn") {
        m_source->WriteLn(GetCode(obj, "column_add"));
        GenSettings(obj->GetObjectInfo(), obj);
    } else if (type == "tool") {
        // If loading bitmap from ICON resource, and size is not set, set size to toolbars bitmapsize
        // So hacky, yet so useful ...
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
                    prop->SetValue(wxString::Format(
                        "%s; %s [%i; %i]",
                        path.c_str(), source.c_str(),
                        toolbarsize.GetWidth(), toolbarsize.GetHeight()));
                    m_source->WriteLn(GetConstruction(obj, false, arrays));
                    prop->SetValue(oldVal);
                    return;
                }
            }
        }
        m_source->WriteLn(GetConstruction(obj, false, arrays));
    } else {
        // Generate the children
        for (size_t i = 0; i < obj->GetChildCount(); i++)
            GenConstruction(obj->GetChild(i), false, arrays);
    }
}

void PythonCodeGenerator::GenDestruction(PObjectBase obj)
{
    PCodeInfo codeInfo = obj->GetObjectInfo()->GetCodeInfo("Python");
    if (codeInfo) {
        wxString _template = codeInfo->GetTemplate("destruction");
        if (!_template.empty()) {
            PythonTemplateParser parser(obj, _template, m_useI18n,
                                        m_useRelativePath, m_basePath,
                                        m_imagePathWrapperFunctionName);
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

void PythonCodeGenerator::FindMacros(PObjectBase obj, std::vector<wxString>* macros)
{
    // iterate through all of the properties of all objects, add the macros
    // to the vector
    for (size_t i = 0; i < obj->GetPropertyCount(); i++) {
        PProperty prop = obj->GetProperty(i);
        if (prop->GetType() == PT_MACRO) {
            wxString value = prop->GetValue();
            if (value.IsEmpty())
                continue;
#if 0
            if (value.Contains("wx") && !value.Contains("wx."))
                value.Replace("wx", "wx.");
#endif
            value.Replace("wx", "wx.");

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

void PythonCodeGenerator::FindEventHandlers(PObjectBase obj, EventVector& events)
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

void PythonCodeGenerator::GenDefines(PObjectBase project)
{
    std::vector<wxString> macros;
    FindMacros(project, &macros);

    // Remove the default macro from the set, for backward compatiblity
    std::vector<wxString>::iterator it
        = std::find(macros.begin(), macros.end(), "ID_DEFAULT");
    if (it != macros.end()) {
        // The default macro is defined to wxID_ANY
        m_source->WriteLn("ID_DEFAULT = wx.ID_ANY # Default");
        macros.erase(it);
    }
    size_t id = m_firstID;
    if (id < 1000)
        wxLogWarning("First ID is Less than 1000");

    for (it = macros.begin(); it != macros.end(); it++) {
        // Don't redefine wx IDs
        m_source->WriteLn(wxString::Format("%s = %i", it->c_str(), id));
        id++;
    }
    if (!macros.empty())
        m_source->WriteLn(wxEmptyString);
}

void PythonCodeGenerator::GenSettings(PObjectInfo info, PObjectBase obj)
{
    PCodeInfo codeInfo = info->GetCodeInfo("Python");
    if (!codeInfo)
        return;

    wxString _template = codeInfo->GetTemplate("settings");
    if (!_template.empty()) {
        PythonTemplateParser parser(obj, _template, m_useI18n, m_useRelativePath,
                                    m_basePath, m_imagePathWrapperFunctionName);
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

void PythonCodeGenerator::GenAddToolbar(PObjectInfo info, PObjectBase obj)
{
    wxArrayString arrCode;
    GetAddToolbarCode(info, obj, arrCode);
    for (size_t i = 0; i < arrCode.GetCount(); i++)
        m_source->WriteLn(arrCode[i]);
}

void PythonCodeGenerator::GetAddToolbarCode(PObjectInfo info, PObjectBase obj, wxArrayString& codelines)
{
    PCodeInfo codeInfo = info->GetCodeInfo("Python");
    if (!codeInfo)
        return;

    wxString _template = codeInfo->GetTemplate("toolbar_add");
    if (!_template.empty()) {
        PythonTemplateParser parser(obj, _template, m_useI18n, m_useRelativePath,
                                    m_basePath, m_imagePathWrapperFunctionName);
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

void PythonCodeGenerator::UseRelativePath(bool relative, wxString basePath)
{
    if (relative) {
        if (wxFileName::DirExists(basePath))
            m_basePath = basePath;
        else
            m_basePath = wxEmptyString;
    }
    m_useRelativePath = relative;
}

void PythonCodeGenerator::SetImagePathWrapperFunctionName(wxString imagePathWrapperFunctionName)
{
    m_imagePathWrapperFunctionName = imagePathWrapperFunctionName;
}

void PythonCodeGenerator::SetupPredefinedMacros()
{
#define ADD_PREDEFINED_MACRO(x) m_predMacros.insert(#x)

    // No id matches this one when compared to it
    ADD_PREDEFINED_MACRO(wx.ID_NONE);

    // Id for a separator line in the menu (invalid for normal item)
    ADD_PREDEFINED_MACRO(wx.ID_SEPARATOR);

    ADD_PREDEFINED_MACRO(wx.ID_ANY);

    ADD_PREDEFINED_MACRO(wx.ID_LOWEST);

    ADD_PREDEFINED_MACRO(wx.ID_OPEN);
    ADD_PREDEFINED_MACRO(wx.ID_CLOSE);
    ADD_PREDEFINED_MACRO(wx.ID_NEW);
    ADD_PREDEFINED_MACRO(wx.ID_SAVE);
    ADD_PREDEFINED_MACRO(wx.ID_SAVEAS);
    ADD_PREDEFINED_MACRO(wx.ID_REVERT);
    ADD_PREDEFINED_MACRO(wx.ID_EXIT);
    ADD_PREDEFINED_MACRO(wx.ID_UNDO);
    ADD_PREDEFINED_MACRO(wx.ID_REDO);
    ADD_PREDEFINED_MACRO(wx.ID_HELP);
    ADD_PREDEFINED_MACRO(wx.ID_PRINT);
    ADD_PREDEFINED_MACRO(wx.ID_PRINT_SETUP);
    ADD_PREDEFINED_MACRO(wx.ID_PREVIEW);
    ADD_PREDEFINED_MACRO(wx.ID_ABOUT);
    ADD_PREDEFINED_MACRO(wx.ID_HELP_CONTENTS);
    ADD_PREDEFINED_MACRO(wx.ID_HELP_COMMANDS);
    ADD_PREDEFINED_MACRO(wx.ID_HELP_PROCEDURES);
    ADD_PREDEFINED_MACRO(wx.ID_HELP_CONTEXT);
    ADD_PREDEFINED_MACRO(wx.ID_CLOSE_ALL);
    ADD_PREDEFINED_MACRO(wx.ID_PAGE_SETUP);
    ADD_PREDEFINED_MACRO(wx.ID_HELP_INDEX);
    ADD_PREDEFINED_MACRO(wx.ID_HELP_SEARCH);
    ADD_PREDEFINED_MACRO(wx.ID_PREFERENCES);

    ADD_PREDEFINED_MACRO(wx.ID_EDIT);
    ADD_PREDEFINED_MACRO(wx.ID_CUT);
    ADD_PREDEFINED_MACRO(wx.ID_COPY);
    ADD_PREDEFINED_MACRO(wx.ID_PASTE);
    ADD_PREDEFINED_MACRO(wx.ID_CLEAR);
    ADD_PREDEFINED_MACRO(wx.ID_FIND);

    ADD_PREDEFINED_MACRO(wx.ID_DUPLICATE);
    ADD_PREDEFINED_MACRO(wx.ID_SELECTALL);
    ADD_PREDEFINED_MACRO(wx.ID_DELETE);
    ADD_PREDEFINED_MACRO(wx.ID_REPLACE);
    ADD_PREDEFINED_MACRO(wx.ID_REPLACE_ALL);
    ADD_PREDEFINED_MACRO(wx.ID_PROPERTIES);

    ADD_PREDEFINED_MACRO(wx.ID_VIEW_DETAILS);
    ADD_PREDEFINED_MACRO(wx.ID_VIEW_LARGEICONS);
    ADD_PREDEFINED_MACRO(wx.ID_VIEW_SMALLICONS);
    ADD_PREDEFINED_MACRO(wx.ID_VIEW_LIST);
    ADD_PREDEFINED_MACRO(wx.ID_VIEW_SORTDATE);
    ADD_PREDEFINED_MACRO(wx.ID_VIEW_SORTNAME);
    ADD_PREDEFINED_MACRO(wx.ID_VIEW_SORTSIZE);
    ADD_PREDEFINED_MACRO(wx.ID_VIEW_SORTTYPE);

    ADD_PREDEFINED_MACRO(wx.ID_FILE);
    ADD_PREDEFINED_MACRO(wx.ID_FILE1);
    ADD_PREDEFINED_MACRO(wx.ID_FILE2);
    ADD_PREDEFINED_MACRO(wx.ID_FILE3);
    ADD_PREDEFINED_MACRO(wx.ID_FILE4);
    ADD_PREDEFINED_MACRO(wx.ID_FILE5);
    ADD_PREDEFINED_MACRO(wx.ID_FILE6);
    ADD_PREDEFINED_MACRO(wx.ID_FILE7);
    ADD_PREDEFINED_MACRO(wx.ID_FILE8);
    ADD_PREDEFINED_MACRO(wx.ID_FILE9);

    // Standard button and menu IDs
    ADD_PREDEFINED_MACRO(wx.ID_OK);
    ADD_PREDEFINED_MACRO(wx.ID_CANCEL);

    ADD_PREDEFINED_MACRO(wx.ID_APPLY);
    ADD_PREDEFINED_MACRO(wx.ID_YES);
    ADD_PREDEFINED_MACRO(wx.ID_NO);
    ADD_PREDEFINED_MACRO(wx.ID_STATIC);
    ADD_PREDEFINED_MACRO(wx.ID_FORWARD);
    ADD_PREDEFINED_MACRO(wx.ID_BACKWARD);
    ADD_PREDEFINED_MACRO(wx.ID_DEFAULT);
    ADD_PREDEFINED_MACRO(wx.ID_MORE);
    ADD_PREDEFINED_MACRO(wx.ID_SETUP);
    ADD_PREDEFINED_MACRO(wx.ID_RESET);
    ADD_PREDEFINED_MACRO(wx.ID_CONTEXT_HELP);
    ADD_PREDEFINED_MACRO(wx.ID_YESTOALL);
    ADD_PREDEFINED_MACRO(wx.ID_NOTOALL);
    ADD_PREDEFINED_MACRO(wx.ID_ABORT);
    ADD_PREDEFINED_MACRO(wx.ID_RETRY);
    ADD_PREDEFINED_MACRO(wx.ID_IGNORE);
    ADD_PREDEFINED_MACRO(wx.ID_ADD);
    ADD_PREDEFINED_MACRO(wx.ID_REMOVE);

    ADD_PREDEFINED_MACRO(wx.ID_UP);
    ADD_PREDEFINED_MACRO(wx.ID_DOWN);
    ADD_PREDEFINED_MACRO(wx.ID_HOME);
    ADD_PREDEFINED_MACRO(wx.ID_REFRESH);
    ADD_PREDEFINED_MACRO(wx.ID_STOP);
    ADD_PREDEFINED_MACRO(wx.ID_INDEX);

    ADD_PREDEFINED_MACRO(wx.ID_BOLD);
    ADD_PREDEFINED_MACRO(wx.ID_ITALIC);
    ADD_PREDEFINED_MACRO(wx.ID_JUSTIFY_CENTER);
    ADD_PREDEFINED_MACRO(wx.ID_JUSTIFY_FILL);
    ADD_PREDEFINED_MACRO(wx.ID_JUSTIFY_RIGHT);

    ADD_PREDEFINED_MACRO(wx.ID_JUSTIFY_LEFT);
    ADD_PREDEFINED_MACRO(wx.ID_UNDERLINE);
    ADD_PREDEFINED_MACRO(wx.ID_INDENT);
    ADD_PREDEFINED_MACRO(wx.ID_UNINDENT);
    ADD_PREDEFINED_MACRO(wx.ID_ZOOM_100);
    ADD_PREDEFINED_MACRO(wx.ID_ZOOM_FIT);
    ADD_PREDEFINED_MACRO(wx.ID_ZOOM_IN);
    ADD_PREDEFINED_MACRO(wx.ID_ZOOM_OUT);
    ADD_PREDEFINED_MACRO(wx.ID_UNDELETE);
    ADD_PREDEFINED_MACRO(wx.ID_REVERT_TO_SAVED);

    // System menu IDs (used by wxUniv)
    ADD_PREDEFINED_MACRO(wx.ID_SYSTEM_MENU);
    ADD_PREDEFINED_MACRO(wx.ID_CLOSE_FRAME);
    ADD_PREDEFINED_MACRO(wx.ID_MOVE_FRAME);
    ADD_PREDEFINED_MACRO(wx.ID_RESIZE_FRAME);
    ADD_PREDEFINED_MACRO(wx.ID_MAXIMIZE_FRAME);
    ADD_PREDEFINED_MACRO(wx.ID_ICONIZE_FRAME);
    ADD_PREDEFINED_MACRO(wx.ID_RESTORE_FRAME);

    // IDs used by generic file dialog (13 consecutive starting from this value)
    ADD_PREDEFINED_MACRO(wx.ID_FILEDLGG);

    ADD_PREDEFINED_MACRO(wx.ID_HIGHEST);

#undef ADD_PREDEFINED_MACRO
}

void PythonTemplateParser::SetupModulePrefixes()
{
#define ADD_PREDEFINED_PREFIX(k, v) m_predModulePrefix[#k] = #v

    // Altered class names
    ADD_PREDEFINED_PREFIX(wxCalendarCtrl, wx.adv.);
    ADD_PREDEFINED_PREFIX(wxRichTextCtrl, wx.richtext.);
    ADD_PREDEFINED_PREFIX(wxStyledTextCtrl, wx.stc.);
    ADD_PREDEFINED_PREFIX(wxHtmlWindow, wx.html.);
    ADD_PREDEFINED_PREFIX(wxAuiNotebook, wx.aui.);
    ADD_PREDEFINED_PREFIX(wxGrid, wx.grid.);
    ADD_PREDEFINED_PREFIX(wxAnimationCtrl, wx.adv.);
    ADD_PREDEFINED_PREFIX(wxDatePickerCtrl, wx.adv.);
    ADD_PREDEFINED_PREFIX(wxTimePickerCtrl, wx.adv.);
    ADD_PREDEFINED_PREFIX(wxHyperlinkCtrl, wx.adv.);
    ADD_PREDEFINED_PREFIX(wxMediaCtrl, wx.media.);

    // Altered macros
    ADD_PREDEFINED_PREFIX(wxCAL_SHOW_HOLIDAYS, wx.adv.);
    ADD_PREDEFINED_PREFIX(wxCAL_MONDAY_FIRST, wx.adv.);
    ADD_PREDEFINED_PREFIX(wxCAL_NO_MONTH_CHANGE, wx.adv.);
    ADD_PREDEFINED_PREFIX(wxCAL_NO_YEAR_CHANGE, wx.adv.);
    ADD_PREDEFINED_PREFIX(wxCAL_SEQUENTIAL_MONTH_SELECTION, wx.adv.);
    ADD_PREDEFINED_PREFIX(wxCAL_SHOW_SURROUNDING_WEEKS, wx.adv.);
    ADD_PREDEFINED_PREFIX(wxCAL_SHOW_WEEK_NUMBERS, wx.adv.);
    ADD_PREDEFINED_PREFIX(wxCAL_SUNDAY_FIRST, wx.adv.);

    ADD_PREDEFINED_PREFIX(wxHL_ALIGN_CENTRE, wx.adv.);
    ADD_PREDEFINED_PREFIX(wxHL_ALIGN_LEFT, wx.adv.);
    ADD_PREDEFINED_PREFIX(wxHL_ALIGN_RIGHT, wx.adv.);
    ADD_PREDEFINED_PREFIX(wxHL_CONTEXTMENU, wx.adv.);
    ADD_PREDEFINED_PREFIX(wxHL_DEFAULT_STYLE, wx.adv.);

    ADD_PREDEFINED_PREFIX(wxHW_DEFAULT_STYLE, wx.html.);
    ADD_PREDEFINED_PREFIX(wxHW_NO_SELECTION, wx.html.);
    ADD_PREDEFINED_PREFIX(wxHW_SCROLLBAR_AUTO, wx.html.);
    ADD_PREDEFINED_PREFIX(wxHW_SCROLLBAR_NEVER, wx.html.);

    ADD_PREDEFINED_PREFIX(wxAUI_NB_BOTTOM, wx.aui.);
    ADD_PREDEFINED_PREFIX(wxAUI_NB_CLOSE_BUTTON, wx.aui.);
    ADD_PREDEFINED_PREFIX(wxAUI_NB_CLOSE_ON_ACTIVE_TAB, wx.aui.);
    ADD_PREDEFINED_PREFIX(wxAUI_NB_CLOSE_ON_ALL_TABS, wx.aui.);
    ADD_PREDEFINED_PREFIX(wxAUI_NB_DEFAULT_STYLE, wx.aui.);
    ADD_PREDEFINED_PREFIX(wxAUI_NB_LEFT, wx.aui.);
    ADD_PREDEFINED_PREFIX(wxAUI_NB_MIDDLE_CLICK_CLOSE, wx.aui.);
    ADD_PREDEFINED_PREFIX(wxAUI_NB_RIGHT, wx.aui.);
    ADD_PREDEFINED_PREFIX(wxAUI_NB_SCROLL_BUTTONS, wx.aui.);
    ADD_PREDEFINED_PREFIX(wxAUI_NB_TAB_EXTERNAL_MOVE, wx.aui.);
    ADD_PREDEFINED_PREFIX(wxAUI_NB_TAB_FIXED_WIDTH, wx.aui.);
    ADD_PREDEFINED_PREFIX(wxAUI_NB_TAB_MOVE, wx.aui.);
    ADD_PREDEFINED_PREFIX(wxAUI_NB_TAB_SPLIT, wx.aui.);
    ADD_PREDEFINED_PREFIX(wxAUI_NB_TOP, wx.aui.);
    ADD_PREDEFINED_PREFIX(wxAUI_NB_WINDOWLIST_BUTTON, wx.aui.);

    ADD_PREDEFINED_PREFIX(wxAUI_TB_TEXT, wx.aui.);
    ADD_PREDEFINED_PREFIX(wxAUI_TB_NO_TOOLTIPS, wx.aui.);
    ADD_PREDEFINED_PREFIX(wxAUI_TB_NO_AUTORESIZE, wx.aui.);
    ADD_PREDEFINED_PREFIX(wxAUI_TB_GRIPPER, wx.aui.);
    ADD_PREDEFINED_PREFIX(wxAUI_TB_OVERFLOW, wx.aui.);
    ADD_PREDEFINED_PREFIX(wxAUI_TB_VERTICAL, wx.aui.);
    ADD_PREDEFINED_PREFIX(wxAUI_TB_HORZ_LAYOUT, wx.aui.);
    ADD_PREDEFINED_PREFIX(wxAUI_TB_HORZ_TEXT, wx.aui.);
    ADD_PREDEFINED_PREFIX(wxAUI_TB_PLAIN_BACKGROUND, wx.aui.);
    ADD_PREDEFINED_PREFIX(wxAUI_TB_DEFAULT_STYLE, wx.aui.);

    ADD_PREDEFINED_PREFIX(wxAUI_MGR_ALLOW_FLOATING, wx.aui.);
    ADD_PREDEFINED_PREFIX(wxAUI_MGR_ALLOW_ACTIVE_PANE, wx.aui.);
    ADD_PREDEFINED_PREFIX(wxAUI_MGR_TRANSPARENT_DRAG, wx.aui.);
    ADD_PREDEFINED_PREFIX(wxAUI_MGR_TRANSPARENT_HINT, wx.aui.);
    ADD_PREDEFINED_PREFIX(wxAUI_MGR_VENETIAN_BLINDS_HINT, wx.aui.);
    ADD_PREDEFINED_PREFIX(wxAUI_MGR_RECTANGLE_HINT, wx.aui.);
    ADD_PREDEFINED_PREFIX(wxAUI_MGR_HINT_FADE, wx.aui.);
    ADD_PREDEFINED_PREFIX(wxAUI_MGR_NO_VENETIAN_BLINDS_FADE, wx.aui.);
    ADD_PREDEFINED_PREFIX(wxAUI_MGR_LIVE_RESIZE, wx.aui.);
    ADD_PREDEFINED_PREFIX(wxAUI_MGR_DEFAULT, wx.aui.);

    ADD_PREDEFINED_PREFIX(wxGRID_AUTOSIZE, wx.grid.);

    ADD_PREDEFINED_PREFIX(wxAC_DEFAULT_STYLE, wx.adv.);
    ADD_PREDEFINED_PREFIX(wxAC_NO_AUTORESIZE, wx.adv.);

    ADD_PREDEFINED_PREFIX(wxRIBBON_BAR_DEFAULT_STYLE, wx.lib.agw.ribbon.);
    ADD_PREDEFINED_PREFIX(wxRIBBON_BAR_FOLDBAR_STYLE, wx.lib.agw.ribbon.);
    ADD_PREDEFINED_PREFIX(wxRIBBON_BAR_SHOW_PAGE_LABELS, wx.lib.agw.ribbon.);
    ADD_PREDEFINED_PREFIX(wxRIBBON_BAR_SHOW_PAGE_ICONS, wx.lib.agw.ribbon.);
    ADD_PREDEFINED_PREFIX(wxRIBBON_BAR_FLOW_HORIZONTAL, wx.lib.agw.ribbon.);
    ADD_PREDEFINED_PREFIX(wxRIBBON_BAR_FLOW_VERTICAL, wx.lib.agw.ribbon.);
    ADD_PREDEFINED_PREFIX(wxRIBBON_BAR_SHOW_PANEL_EXT_BUTTONS, wx.lib.agw.ribbon.);
    ADD_PREDEFINED_PREFIX(wxRIBBON_BAR_SHOW_PANEL_MINIMISE_BUTTONS, wx.lib.agw.ribbon.);
    ADD_PREDEFINED_PREFIX(wxRIBBON_BAR_SHOW_TOGGLE_BUTTON, wx.lib.agw.ribbon.);
    ADD_PREDEFINED_PREFIX(wxRIBBON_BAR_SHOW_HELP_BUTTON, wx.lib.agw.ribbon.);

    ADD_PREDEFINED_PREFIX(wxRIBBON_PANEL_DEFAULT_STYLE, wx.lib.agw.ribbon.);
    ADD_PREDEFINED_PREFIX(wxRIBBON_PANEL_NO_AUTO_MINIMISE, wx.lib.agw.ribbon.);
    ADD_PREDEFINED_PREFIX(wxRIBBON_PANEL_EXT_BUTTON, wx.lib.agw.ribbon.);
    ADD_PREDEFINED_PREFIX(wxRIBBON_PANEL_MINIMISE_BUTTON, wx.lib.agw.ribbon.);
    ADD_PREDEFINED_PREFIX(wxRIBBON_PANEL_STRETCH, wx.lib.agw.ribbon.);
    ADD_PREDEFINED_PREFIX(wxRIBBON_PANEL_FLEXIBLE, wx.lib.agw.ribbon.);

    ADD_PREDEFINED_PREFIX(wxPG_AUTO_SORT, wx.propgrid.);
    ADD_PREDEFINED_PREFIX(wxPG_HIDE_CATEGORIES, wx.propgrid.);
    ADD_PREDEFINED_PREFIX(wxPG_ALPHABETIC_MODE, wx.propgrid.);
    ADD_PREDEFINED_PREFIX(wxPG_BOLD_MODIFIED, wx.propgrid.);
    ADD_PREDEFINED_PREFIX(wxPG_SPLITTER_AUTO_CENTER, wx.propgrid.);
    ADD_PREDEFINED_PREFIX(wxPG_TOOLTIPS, wx.propgrid.);
    ADD_PREDEFINED_PREFIX(wxPG_HIDE_MARGIN, wx.propgrid.);
    ADD_PREDEFINED_PREFIX(wxPG_STATIC_SPLITTER, wx.propgrid.);
    ADD_PREDEFINED_PREFIX(wxPG_STATIC_LAYOUT, wx.propgrid.);
    ADD_PREDEFINED_PREFIX(wxPG_LIMITED_EDITING, wx.propgrid.);
    ADD_PREDEFINED_PREFIX(wxPG_DEFAULT_STYLE, wx.propgrid.);
    ADD_PREDEFINED_PREFIX(wxPG_EX_INIT_NOCAT, wx.propgrid.);
    ADD_PREDEFINED_PREFIX(wxPG_EX_NO_FLAT_TOOLBAR, wx.propgrid.);
    ADD_PREDEFINED_PREFIX(wxPG_EX_MODE_BUTTONS, wx.propgrid.);
    ADD_PREDEFINED_PREFIX(wxPG_EX_HELP_AS_TOOLTIPS, wx.propgrid.);
    ADD_PREDEFINED_PREFIX(wxPG_EX_NATIVE_DOUBLE_BUFFERING, wx.propgrid.);
    ADD_PREDEFINED_PREFIX(wxPG_EX_AUTO_UNSPECIFIED_VALUES, wx.propgrid.);
    ADD_PREDEFINED_PREFIX(wxPG_EX_WRITEONLY_BUILTIN_ATTRIBUTES, wx.propgrid.);
    ADD_PREDEFINED_PREFIX(wxPG_EX_HIDE_PAGE_BUTTONS, wx.propgrid.);
    ADD_PREDEFINED_PREFIX(wxPG_EX_MULTIPLE_SELECTION, wx.propgrid.);
    ADD_PREDEFINED_PREFIX(wxPG_EX_ENABLE_TLP_TRACKING, wx.propgrid.);
    ADD_PREDEFINED_PREFIX(wxPG_EX_NO_TOOLBAR_DIVIDER, wx.propgrid.);
    ADD_PREDEFINED_PREFIX(wxPG_EX_TOOLBAR_SEPARATOR, wx.propgrid.);
    ADD_PREDEFINED_PREFIX(wxPG_DESCRIPTION, wx.propgrid.);
    ADD_PREDEFINED_PREFIX(wxPG_TOOLBAR, wx.propgrid.);
    ADD_PREDEFINED_PREFIX(wxPGMAN_DEFAULT_STYLE, wx.propgrid.);
    ADD_PREDEFINED_PREFIX(wxPG_NO_INTERNAL_BORDER, wx.propgrid.);

    ADD_PREDEFINED_PREFIX(wxDATAVIEW_CELL_ACTIVATABLE, wx.dataview.);
    ADD_PREDEFINED_PREFIX(wxDATAVIEW_CELL_INERT, wx.dataview.);
    ADD_PREDEFINED_PREFIX(wxDATAVIEW_COL_HIDDEN, wx.dataview.);
    ADD_PREDEFINED_PREFIX(wxDATAVIEW_COL_REORDERABLE, wx.dataview.);
    ADD_PREDEFINED_PREFIX(wxDATAVIEW_COL_RESIZABLE, wx.dataview.);
    ADD_PREDEFINED_PREFIX(wxDATAVIEW_COL_SORTABLE, wx.dataview.);

    ADD_PREDEFINED_PREFIX(wxDV_SINGLE, wx.dataview.);
    ADD_PREDEFINED_PREFIX(wxDV_MULTIPLE, wx.dataview.);
    ADD_PREDEFINED_PREFIX(wxDV_ROW_LINES, wx.dataview.);
    ADD_PREDEFINED_PREFIX(wxDV_HORIZ_RULES, wx.dataview.);
    ADD_PREDEFINED_PREFIX(wxDV_VERT_RULES, wx.dataview.);
    ADD_PREDEFINED_PREFIX(wxDV_VARIABLE_LINE_HEIGHT, wx.dataview.);
    ADD_PREDEFINED_PREFIX(wxDV_NO_HEADER, wx.dataview.);

    ADD_PREDEFINED_PREFIX(wxTL_SINGLE, wx.dataview.);
    ADD_PREDEFINED_PREFIX(wxTL_MULTIPLE, wx.dataview.);
    ADD_PREDEFINED_PREFIX(wxTL_CHECKBOX, wx.dataview.);
    ADD_PREDEFINED_PREFIX(wxTL_3STATE, wx.dataview.);
    ADD_PREDEFINED_PREFIX(wxTL_USER_3STATE, wx.dataview.);
    ADD_PREDEFINED_PREFIX(wxTL_DEFAULT_STYLE, wx.dataview.);

    ADD_PREDEFINED_PREFIX(wxDP_SPIN, wx.adv.);
    ADD_PREDEFINED_PREFIX(wxDP_DROPDOWN, wx.adv.);
    ADD_PREDEFINED_PREFIX(wxDP_SHOWCENTURY, wx.adv.);
    ADD_PREDEFINED_PREFIX(wxDP_ALLOWNONE, wx.adv.);
    ADD_PREDEFINED_PREFIX(wxDP_DEFAULT, wx.adv.);

    ADD_PREDEFINED_PREFIX(wxTP_DEFAULT, wx.adv.);

#undef ADD_PREDEFINED_PREFIX
}
