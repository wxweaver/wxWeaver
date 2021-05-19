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
#include "codegen/luacg.h"

#include "model/objectbase.h"
#include "rad/appdata.h"
#include "utils/debug.h"
#include "utils/typeconv.h"
#include "utils/exception.h"
#include "codegen/codewriter.h"

#include <wx/filename.h>
#include <wx/tokenzr.h>

#include <algorithm>

LuaTemplateParser::LuaTemplateParser(PObjectBase obj, wxString _template,
                                     bool useI18N, bool useRelativePath,
                                     wxString basePath,
                                     std::vector<wxString> strUserIDsVec)
    : TemplateParser(obj, _template)
    , m_basePath(basePath)
    , m_strUserIDsVec(strUserIDsVec)
    , m_useI18n(useI18N)
    , m_useRelativePath(useRelativePath)
{
    if (!wxFileName::DirExists(m_basePath))
        m_basePath.clear();

    SetupModulePrefixes();
}

LuaTemplateParser::LuaTemplateParser(const LuaTemplateParser& other,
                                     wxString _template,
                                     std::vector<wxString> strUserIDsVec)
    : TemplateParser(other, _template)
    , m_basePath(other.m_basePath)
    , m_strUserIDsVec(strUserIDsVec)
    , m_useI18n(other.m_useI18n)
    , m_useRelativePath(other.m_useRelativePath)
{
    SetupModulePrefixes();
}

wxString LuaTemplateParser::RootWxParentToCode()
{
    return "NS.";
}

PTemplateParser LuaTemplateParser::CreateParser(const TemplateParser* oldparser,
                                                wxString _template)
{
    const LuaTemplateParser* luaOldParser
        = dynamic_cast<const LuaTemplateParser*>(oldparser);
    if (luaOldParser) {
        std::vector<wxString> empty;
        PTemplateParser newparser(new LuaTemplateParser(*luaOldParser, _template, empty));
        return newparser;
    }
    return PTemplateParser();
}

wxString LuaTemplateParser::ValueToCode(PropertyType type, wxString value)
{
    wxString result;

    switch (type) {
    case PT_WXPARENT: {
        result = wxT("NS.") + value;
        break;
    }
    case PT_WXPARENT_SB: {
        result = value + wxT(":GetStaticBox()");
        break;
    }
    case PT_WXPARENT_CP: {
        result = wxT("NS.") + value + wxT(":GetPane()");
        break;
    }
    case PT_WXSTRING:
    case PT_FILE:
    case PT_PATH: {
        if (value.empty())
            result << wxT("\"\"");
        else
            result << wxT("\"") << LuaCodeGenerator::ConvertLuaString(value) << wxT("\"");

        break;
    }
    case PT_WXSTRING_I18N: {
        if (value.empty()) {
            result << wxT("\"\"");
        } else {
            if (m_useI18n)
                result << wxT("\"") << LuaCodeGenerator::ConvertLuaString(value) << wxT("\"");
            else
                result << wxT("\"") << LuaCodeGenerator::ConvertLuaString(value) << wxT("\"");
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
            result.Replace(wxT("wx"), pred);
        else {
            //prepend "wx." if value isn't UserID and contains "wx" prefix
            if (m_strUserIDsVec.end()
                    == std::find(m_strUserIDsVec.begin(), m_strUserIDsVec.end(), value)
                && result.Find("wx") != wxNOT_FOUND) {
                result.Prepend("wx.");
            }
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
        if (value.empty()) {
            result = wxT("0");
            break;
        } else {
            result = value;
        }
        wxString pred, bit, res, pref;
        wxStringTokenizer bits(result, wxT("|"), wxTOKEN_STRTOK);

        while (bits.HasMoreTokens()) {
            bit = bits.GetNextToken();
            pred = m_predModulePrefix[bit];

            if (pred.empty())
                res += pref + wxT("wx.") + bit;
            else
                res += pref + pred + bit;

            pref = wxT(" + ");
        }
        result = res;
        break;
    }
    case PT_WXPOINT: {
        if (value.empty())
            result = wxT("wx.wxDefaultPosition");
        else
            result << wxT("wx.wxPoint( ") << value << wxT(" )");

        break;
    }
    case PT_WXSIZE: {
        if (value.empty())
            result = wxT("wx.wxDefaultSize");
        else
            result << wxT("wx.wxSize( ") << value << wxT(" )");

        break;
    }
    case PT_BOOL: {
        result = (value == wxT("0") ? wxT("False") : wxT("True"));
        break;
    }
    case PT_WXFONT: {
        if (!value.empty()) {
            wxFontContainer fontContainer = TypeConv::StringToFont(value);
            wxFont font = fontContainer.GetFont();

            const int pointSize = fontContainer.GetPointSize();

            result = wxString::Format(
                "wx.wxFont( %s, %s, %s, %s, %s, %s )",
                ((pointSize <= 0)
                     ? "wx.wxNORMAL_FONT:GetPointSize()"
                     : (wxString() << pointSize)),
                "wx." + TypeConv::FontFamilyToString(fontContainer.GetFamily()),
                "wx." + font.GetStyleString(),
                "wx." + font.GetWeightString(),
                (fontContainer.GetUnderlined() ? "True" : "False"),
                (fontContainer.m_faceName.empty()
                     ? "\"\""
                     : ("\"" + fontContainer.m_faceName + "\"")));
        } else {
            result = wxT("wx.wxNORMAL_FONT");
        }
        break;
    }
    case PT_WXCOLOUR: {
        if (!value.empty()) {
            if (!value.find_first_of(wxT("wx"))) {
                // System Colour
                result << wxT("wx.wxSystemSettings.GetColour( ")
                       << ValueToCode(PT_OPTION, value) << wxT(" )");
            } else {
                wxColour colour = TypeConv::StringToColour(value);
                result = wxString::Format(wxT("wx.wxColour( %i, %i, %i )"),
                                          colour.Red(), colour.Green(), colour.Blue());
            }
        } else {
            result = wxT("wx.wxColour()");
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
            result = wxT("wx.wxNullBitmap");
            break;
        }
        if (path.StartsWith(wxT("file:"))) {
            wxLogWarning(
                "Lua code generation does not support using URLs for bitmap properties:\n%s",
                path.c_str());
            result = wxT("wx.wxNullBitmap");
            break;
        }
        if (source == _("Load From File") || source == _("Load From Embedded File")) {
            wxString absPath;
            try {
                absPath = TypeConv::MakeAbsolutePath(path, AppData()->GetProjectPath());
            } catch (wxWeaverException& ex) {
                wxLogError(ex.what());
                result = wxT("wx.wxNullBitmap");
                break;
            }
            wxString file
                = (m_useRelativePath
                       ? TypeConv::MakeRelativePath(absPath, m_basePath)
                       : absPath);

            result << wxT("wx.wxBitmap( \"")
                   << LuaCodeGenerator::ConvertLuaString(file)
                   << wxT("\", wx.wxBITMAP_TYPE_ANY )");
        } else if (source == _("Load From Resource")) {
            result << wxT("wx.wxBitmap( \"") << path
                   << wxT("\", wx.wxBITMAP_TYPE_RESOURCE )");
        } else if (source == _("Load From Icon Resource")) {
            result << wxT("wx.wxBitmap( \"") << path << wxT("\")");
#if 0
            // TODO: load from icon isn't supported by wxLua
            if (icoSize == wxDefaultSize)
                result << "wx.wxICON(" << path << ")";
            else
                result.Printf(
                    "wx.Icon( u\"%s\", wx.wxBITMAP_TYPE_ICO_RESOURCE, %i, %i )",
                    path.c_str(), icoSize.GetWidth(), icoSize.GetHeight() );
#endif
        } else if (source == _("Load From XRC")) {
            // This requires that the global wxXmlResource object is set
            result << wxT("wx.wxXmlResource.Get():LoadBitmap( \"") << path << wxT("\" )");
        } else if (source == _("Load From Art Provider")) {
            wxString rid = path.BeforeFirst(wxT(':'));

            if (rid.StartsWith(wxT("gtk-")))
                rid = wxT("\"") + rid + wxT("\"");
            else
                rid.Replace(wxT("wx"), wxT("wx.wx"));

            wxString cid = path.AfterFirst(wxT(':'));
            cid.Replace(wxT("wx"), wxT("wx.wx"));

            result = wxT("wx.wxArtProvider.GetBitmap(")
                + rid + wxT(", ")
                + cid + wxT(")");
        }
        break;
    }
    case PT_STRINGLIST: {
        // Stringlists are generated like a sequence of wxString separated by ', '.
        wxArrayString array = TypeConv::StringToArrayString(value);
        if (array.Count() > 0)
            result = ValueToCode(PT_WXSTRING_I18N, array[0]);

        for (size_t i = 1; i < array.Count(); i++)
            result << wxT(", ") << ValueToCode(PT_WXSTRING_I18N, array[i]);

        break;
    }
    default:
        break;
    }
    return result;
}

LuaCodeGenerator::LuaCodeGenerator()
{
    SetupPredefinedMacros();
    m_useRelativePath = false;
    m_useI18n = false;
    m_firstID = 1000;

    // These classes aren't wrapped by wxLua, make an exception
    m_strUnsupportedClasses.push_back(wxT("wxRichTextCtrl"));
    m_strUnsupportedClasses.push_back(wxT("wxSearchCtrl"));
    m_strUnsupportedClasses.push_back(wxT("wxAuiToolBar"));
    m_strUnsupportedClasses.push_back(wxT("wxRibbonBar"));
    m_strUnsupportedClasses.push_back(wxT("wxDataViewCtrl"));
    m_strUnsupportedClasses.push_back(wxT("wxDataViewListCtrl"));
    m_strUnsupportedClasses.push_back(wxT("wxDataViewTreeCtrl"));
}

wxString LuaCodeGenerator::ConvertLuaString(wxString text)
{
    wxString result;

    for (size_t i = 0; i < text.length(); i++) {
        wxChar c = text[i];

        switch (c) {
        case wxT('"'):
            result += wxT("\\\"");
            break;

        case wxT('\\'):
            result += wxT("\\\\");
            break;

        case wxT('\t'):
            result += wxT("\\t");
            break;

        case wxT('\n'):
            result += wxT("\\n");
            break;

        case wxT('\r'):
            result += wxT("\\r");
            break;

        default:
            result += c;
            break;
        }
    }
    return result;
}

void LuaCodeGenerator::GenerateInheritedClass(PObjectBase userClasses,
                                              PObjectBase form,
                                              const wxString& genFileFullPath)
{
    if (!userClasses) {
        wxLogError(wxT("There is no object to generate inherited class"));
        return;
    }
    if (wxT("UserClasses") != userClasses->GetClassName()) {
        wxLogError(wxT("This not a UserClasses object"));
        return;
    }
    // Start file
    wxString code = GetCode(userClasses, wxT("file_comment"));
    m_source->WriteLn(code);

    wxString fullGenPath = genFileFullPath;
    fullGenPath.Replace(wxT("\\"), wxT("\\\\"));

    code = wxT("package.path = \"") + fullGenPath + wxT(".lua\"");
    m_source->WriteLn(code);

    code = GetCode(userClasses, wxT("source_include"));
    m_source->WriteLn(code);
    m_source->WriteLn(wxEmptyString);

    EventVector events;
    FindEventHandlers(form, events);

    if (events.size() > 0) {
        code = GetCode(userClasses, wxT("event_handler_comment"));
        m_source->WriteLn(code);
        m_source->WriteLn(wxEmptyString);

        std::set<wxString> generatedHandlers;
        wxString eventsGroupID;
        wxString strPrevClassName;
        for (size_t i = 0; i < events.size(); i++) {
            PEvent event = events[i];

            wxString handlerName = event->GetValue();
            wxString templateName = wxString::Format(
                wxT("connect_%s"), event->GetName().c_str());

            PObjectBase obj = event->GetObject();
            PObjectInfo objInfo = obj->GetObjectInfo();

            wxString strClassName;
            code = GenEventEntryForInheritedClass(obj, objInfo, templateName,
                                                  handlerName, strClassName);
            bool bAddCaption = false;
            PProperty propName = obj->GetProperty(wxT("name"));
            if (propName) {
                strClassName = propName->GetValue();
                if (strPrevClassName != strClassName) {
                    strPrevClassName = strClassName;
                    bAddCaption = true;
                    eventsGroupID = wxString::Format(
                        wxT("-- %s (%s) event handlers: "),
                        strClassName.c_str(), obj->GetClassName().c_str());
                }
            }
            if (code.length() > 0) {
                if (bAddCaption)
                    m_source->WriteLn(eventsGroupID);

                m_source->WriteLn(code);
                m_source->WriteLn();
            }
        }
        m_source->WriteLn(wxEmptyString);
        m_source->WriteLn(wxEmptyString);
    }
    m_source->Unindent();
}

wxString LuaCodeGenerator::GenEventEntryForInheritedClass(PObjectBase obj,
                                                          PObjectInfo objInfo,
                                                          const wxString& templateName,
                                                          const wxString& handlerName,
                                                          wxString& strClassName)
{
    PCodeInfo codeInfo = objInfo->GetCodeInfo("Lua");
    if (codeInfo) {
        wxString _template;
        _template = codeInfo->GetTemplate(wxString::Format(
            wxT("evt_%s"), templateName.c_str()));
        if (!_template.empty()) {
            _template.Replace(wxT("#handler"), handlerName);
            _template.Replace(wxT("#skip"), wxT("\n") + m_strEventHandlerPostfix);
        }
        LuaTemplateParser parser(obj, _template, m_useI18n, m_useRelativePath,
                                 m_basePath, m_strUserIDsVec);
        wxString code = parser.ParseTemplate();
        if (code.length())
            return code;

        for (size_t i = 0; i < objInfo->GetBaseClassCount(false); i++) {
            PObjectInfo base_info = objInfo->GetBaseClass(i, false);
            code = GenEventEntryForInheritedClass(
                obj, base_info, templateName, handlerName, strClassName);

            if (code.length())
                return code;
        }
    }
    return wxEmptyString;
}

bool LuaCodeGenerator::GenerateCode(PObjectBase project)
{
    if (!project) {
        wxLogError("There is no project to generate code");
        return false;
    }
    m_useI18n = false;
    PProperty i18nProperty = project->GetProperty(wxT("internationalize"));
    if (i18nProperty && i18nProperty->GetValueAsInteger())
        m_useI18n = true;

    m_disconnectEvents = (project->GetPropertyAsInteger("disconnect_lua_events"));

    m_source->Clear();

    wxString code = GetCode(project, wxT("lua_preamble")); // Insert Lua preamble
    if (!code.empty())
        m_source->WriteLn(code);

    code = wxString::Format(
        "--[[\n"
        "    Lua code generated with wxWeaver (version %s%s " __DATE__ ")\n"
        "    https://wxweaver.github.io/\n"
        "--]]",
        VERSION, REVISION);

    m_source->WriteLn(code, true);

    PProperty propFile = project->GetProperty("file");
    if (!propFile) {
        wxLogError(wxT("Missing \"file\" property on Project Object"));
        return false;
    }
    wxString file = propFile->GetValue();
    if (file.empty())
        file = "noname";

    // Generate the subclass sets
    std::set<wxString> subclasses;
    std::vector<wxString> headerIncludes;

    GenSubclassSets(project, &subclasses, &headerIncludes);

    // Generating  includes
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

    // Generating "defines" for macros
    GenDefines(project);

    PProperty propNamespace = project->GetProperty(wxT("ui_table"));
    if (propNamespace) {
        m_strUITable = propNamespace->GetValueAsString();
        if (!m_strUITable.length())
            m_strUITable = "UI"; //default value, m_strUITable shouldn't be empty

        code = m_strUITable + " = {} \n";
        m_source->WriteLn(code);
    }
    PProperty eventKindProp = project->GetProperty("skip_lua_events");
    if (eventKindProp->GetValueAsInteger())
        m_strEventHandlerPostfix = "event:Skip()";
    else
        m_strEventHandlerPostfix = wxEmptyString;

    PProperty disconnectMode = project->GetProperty("disconnect_mode");
    m_disconnecMode = disconnectMode->GetValueAsString();

    size_t dProjChildCount = project->GetChildCount();
    for (size_t i = 0; i < dProjChildCount; i++) {
        PObjectBase child = project->GetChild(i);

        // Preprocess to find arrays
        ArrayItems arrays;
        FindArrayObjects(child, arrays, true);

        EventVector events;
        FindEventHandlers(child, events);
        GenClassDeclaration(child, false, wxEmptyString, events,
                            m_strEventHandlerPostfix, arrays);
    }
    code = GetCode(project, "lua_epilogue");
    if (!code.empty())
        m_source->WriteLn(code);

    return true;
}

void LuaCodeGenerator::GenEvents(PObjectBase classObj, const EventVector& events,
                                 wxString& strClassName, bool disconnect)
{
    if (events.empty())
        return;

    if (disconnect) {
        m_source->WriteLn(wxT("-- Disconnect Events\n"));
    } else {
        m_source->WriteLn();
        m_source->WriteLn(wxT("-- Connect Events\n"));
    }
    PProperty propName = classObj->GetProperty(wxT("name"));
    if (!propName) {
        wxLogError(wxT("Missing \"name\" property on \"%s\" class. Review your XML object description"),
                   classObj->GetClassName().c_str());
        return;
    }
    wxString className = propName->GetValue();
    if (className.empty()) {
        wxLogError(wxT("Object name cannot be null"));
        return;
    }
#if 0
    wxString baseClass;
    PProperty propSubclass = classObj->GetProperty("subclass");
    if (propSubclass) {
        wxString subclass = propSubclass->GetChildFromParent("name");
        if (!subclass.empty())
            baseClass = subclass;
    }
    if (baseClass.empty())
        baseClass = "wx." + classObj->GetClassName();
#endif
    if (events.size() <= 0)
        return;

    for (size_t i = 0; i < events.size(); i++) {
        PEvent event = events[i];
        wxString handlerName = event->GetValue(); // + wxT("_") + className;
        wxString templateName = wxString::Format(
            wxT("connect_%s"), event->GetName().c_str());

        PObjectBase obj = event->GetObject();
        if (!GenEventEntry(obj, obj->GetObjectInfo(), templateName, handlerName,
                           strClassName, disconnect)) {
            wxLogError(
                "Missing \"evt_%s\" template for \"%s\" class. Review your XML object description",
                templateName.c_str(), className.c_str());
        }
    }
}

bool LuaCodeGenerator::GenEventEntry(PObjectBase obj, PObjectInfo objInfo,
                                     const wxString& templateName,
                                     const wxString& handlerName,
                                     wxString& strClassName, bool disconnect)
{
    PCodeInfo codeInfo = objInfo->GetCodeInfo("Lua");
    if (codeInfo) {
        wxString strTemplate = wxString::Format(
            "evt_%s%s", disconnect ? wxS("dis") : wxEmptyString, templateName);

        wxString _template = codeInfo->GetTemplate(strTemplate);
        if (disconnect && _template.empty()) {
            _template = codeInfo->GetTemplate(wxT("evt_") + templateName);
            _template.Replace(wxT("Connect"), wxT("Disconnect"), true);
        }
        if (!_template.empty()) {
            if (disconnect) {
                if (m_disconnecMode == wxT("handler_name"))
                    _template.Replace(wxT("#handler"), wxT("handler = ") + handlerName);
                else if (m_disconnecMode == wxT("source_name"))
                    _template.Replace(wxT("#handler"), wxT("None"));
            } else {
                _template.Replace(wxT("#handler"), handlerName);
                _template.Replace(wxT("#skip"), wxT("\n") + m_strEventHandlerPostfix);
            }

            LuaTemplateParser parser(obj, _template, m_useI18n, m_useRelativePath,
                                     m_basePath, m_strUserIDsVec);
            wxString code = parser.ParseTemplate();
            wxString strRootCode = parser.RootWxParentToCode();
            if (code.Find(strRootCode) != -1) {
                code.Replace(strRootCode, strClassName);
            }
            m_source->WriteLn(code);
            m_source->WriteLn();
            return true;
        }
    }
    for (size_t i = 0; i < objInfo->GetBaseClassCount(false); i++) {
        PObjectInfo base_info = objInfo->GetBaseClass(i, false);
        if (GenEventEntry(obj, base_info, templateName, handlerName,
                          strClassName, disconnect))
            return true;
    }
    return false;
}

void LuaCodeGenerator::GenVirtualEventHandlers(const EventVector& events, const wxString& eventHandlerPostfix, const wxString& strClassName)
{
    if (events.size() > 0) {
        /*
            There are problems if we create "pure" virtual handlers,
            because some events could be triggered in the constructor
            in which virtual methods are executed properly.
            So we create a default handler which will skip the event.
        */
        m_source->WriteLn(wxEmptyString);
        m_source->WriteLn(wxT("-- event handlers"));

        std::set<wxString> generatedHandlers;
        for (size_t i = 0; i < events.size(); i++) {
            PEvent event = events[i];
            wxString aux = wxT("function ") + event->GetValue() + '_'
                + strClassName + wxT("( event )");

            if (generatedHandlers.find(aux) == generatedHandlers.end()) {
                m_source->WriteLn(aux);
                m_source->Indent();
                m_source->WriteLn(wxT("\n") + eventHandlerPostfix);
                m_source->Unindent();
                m_source->WriteLn(wxT("end"));
                generatedHandlers.insert(aux);
            }
            if (i < (events.size() - 1))
                m_source->WriteLn();
        }
        m_source->WriteLn(wxEmptyString);
    }
}

void LuaCodeGenerator::GetGenEventHandlers(PObjectBase obj)
{
    GenDefinedEventHandlers(obj->GetObjectInfo(), obj);
    for (size_t i = 0; i < obj->GetChildCount(); i++) {
        PObjectBase child = obj->GetChild(i);
        GetGenEventHandlers(child);
    }
}

void LuaCodeGenerator::GenDefinedEventHandlers(PObjectInfo info, PObjectBase obj)
{
    PCodeInfo codeInfo = info->GetCodeInfo(wxT("Lua"));
    if (codeInfo) {
        wxString _template = codeInfo->GetTemplate(wxT("generated_event_handlers"));
        if (!_template.empty()) {
            LuaTemplateParser parser(obj, _template, m_useI18n, m_useRelativePath,
                                     m_basePath, m_strUserIDsVec);
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

wxString LuaCodeGenerator::GetCode(PObjectBase obj, wxString name, bool silent /*= false*/, wxString strSelf /*= wxEmptyString*/)
{
    PCodeInfo codeInfo = obj->GetObjectInfo()->GetCodeInfo(wxT("Lua"));
    if (!codeInfo) {
        if (!silent) {
            wxString msg(wxString::Format(wxT("Missing \"%s\" template for \"%s\" class. Review your XML object description"),
                                          name.c_str(), obj->GetClassName().c_str()));
            wxLogError(msg);
        }
        return wxEmptyString;
    }
    wxString _template = codeInfo->GetTemplate(name);
    _template.Replace(wxT("#parentname"), strSelf);
    if (!m_strUITable.empty())
        _template.Replace(wxT("#utbl"), m_strUITable + wxT("."));
    else
        _template.Replace(wxT("#utbl"), wxEmptyString);

    LuaTemplateParser parser(obj, _template, m_useI18n, m_useRelativePath,
                             m_basePath, m_strUserIDsVec);
    wxString code = parser.ParseTemplate();

    //handle unsupported classes
    std::vector<wxString>::iterator iter = m_strUnsupportedInstances.begin();
    for (; iter != m_strUnsupportedInstances.end(); ++iter) {
        if (code.Contains(*iter))
            break;
    }
    if (iter != m_strUnsupportedInstances.end())
        code.Prepend(wxT("--"));

    wxString strRootCode = parser.RootWxParentToCode();
    int pos = code.Find(strRootCode);
    if (pos != -1) {
        wxString strMid = code.Mid(pos + strRootCode.length(), 3);
        strMid.Trim(false);
        if (strMid.GetChar(0) == ',')
            code.Replace(strRootCode, strSelf);
        else
            code.Replace(strRootCode, wxEmptyString);
    }
    return code;
}

wxString LuaCodeGenerator::GetConstruction(PObjectBase obj, bool silent,
                                           wxString strSelf, ArrayItems& arrays)
{
    // Get the name
    const auto& propName = obj->GetProperty(wxT("name"));
    if (!propName) {
        // Object has no name, just get its code
        return GetCode(obj, wxT("construction"), silent, strSelf);
    }
    // Object has a name, check if its an array
    const auto& name = propName->GetValue();
    wxString baseName;
    ArrayItem unused;
    if (!ParseArrayName(name, baseName, unused)) {
        // Object is not an array, just get its code
        return GetCode(obj, wxT("construction"), silent, strSelf);
    }
    // Object is an array, check if it needs to be declared
    auto& item = arrays[baseName];
    if (item.isDeclared) {
        // Object is already declared, just get its code
        return GetCode(obj, wxT("construction"), silent, strSelf);
    }
    // UI table code copied from TemplateParser
    wxString strTableName;
    const auto& project = AppData()->GetProjectData();
    const auto& table = project->GetProperty(wxT("ui_table"));
    if (table) {
        strTableName = table->GetValueAsString();
        if (strTableName.empty()) {
            strTableName = wxT("UI");
        }
        strTableName.append(wxT("."));
    }
    wxString code;
    code.append(strTableName);
    code.append(baseName);
    code.append(wxT(" = {}\n"));

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
                    code.append(strTableName);
                    code.append(targetName);
                    code.append(" = {}\n");
                    stackNext.push_back(targetName);
                }
            }
            stackCurrent.swap(stackNext);
            stackNext.clear();
        }
    }
    code.append(
        GetCode(
            obj, "construction", silent, strSelf)); // Get the Code
    item.isDeclared = true;                         // Mark the array as declared
    return code;
}

void LuaCodeGenerator::GenClassDeclaration(PObjectBase classObj, bool /*useEnum*/,
                                           const wxString& /*classDecoration*/,
                                           const EventVector& events,
                                           const wxString& /*eventHandlerPostfix*/,
                                           ArrayItems& arrays)
{
    wxString strClassName = classObj->GetClassName();
    PProperty propName = classObj->GetProperty(wxT("name"));
    if (!propName) {
        wxLogError(
            wxT("Missing \"name\" property on \"%s\" class. Review your XML object description"),
            strClassName.c_str());
        return;
    }
    wxString strName = propName->GetValue();
    if (strName.empty()) {
        wxLogError(wxT("Object name can not be null"));
        return;
    }
    GetGenEventHandlers(classObj);
    GenConstructor(classObj, events, strName, arrays);
}

void LuaCodeGenerator::GenSubclassSets(PObjectBase obj,
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

        wxString include = "require(\"" + headerVal.Trim() + "\")\n";
        std::vector<wxString>::iterator it
            = std::find(headerIncludes->begin(), headerIncludes->end(), include);
        if (headerIncludes->end() == it)
            headerIncludes->push_back(include);
    }
}

void LuaCodeGenerator::GenIncludes(PObjectBase project,
                                   std::vector<wxString>* includes,
                                   std::set<wxString>* templates)
{
    GenObjectIncludes(project, includes, templates);
}

void LuaCodeGenerator::GenObjectIncludes(PObjectBase project,
                                         std::vector<wxString>* includes,
                                         std::set<wxString>* templates)
{
    // Fill the set
    PCodeInfo codeInfo = project->GetObjectInfo()->GetCodeInfo("Lua");
    if (codeInfo) {
        LuaTemplateParser parser(project, codeInfo->GetTemplate("include"),
                                 m_useI18n, m_useRelativePath, m_basePath, m_strUserIDsVec);
        wxString include = parser.ParseTemplate();
        if (!include.empty()) {
            if (templates->insert(include).second)
                AddUniqueIncludes(include, includes);
        }
    }
    // Call GenIncludes on all children as well
    for (size_t i = 0; i < project->GetChildCount(); i++) {
        GenObjectIncludes(project->GetChild(i), includes, templates);
    }
    // Generate includes for base classes
    GenBaseIncludes(project->GetObjectInfo(), project, includes, templates);
}

void LuaCodeGenerator::GenBaseIncludes(PObjectInfo info, PObjectBase obj,
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
    PCodeInfo codeInfo = info->GetCodeInfo(wxT("Lua"));
    if (!codeInfo)
        return;

    LuaTemplateParser parser(obj, codeInfo->GetTemplate(wxT("include")),
                             m_useI18n, m_useRelativePath, m_basePath, m_strUserIDsVec);

    wxString include = parser.ParseTemplate();
    if (!include.empty())
        if (templates->insert(include).second)
            AddUniqueIncludes(include, includes);
}

void LuaCodeGenerator::AddUniqueIncludes(const wxString& include,
                                         std::vector<wxString>* includes)
{
    // Split on newlines to only generate unique include lines
    // This strips blank lines and trims
    wxStringTokenizer tkz(include, wxT("\n"), wxTOKEN_STRTOK);

    while (tkz.HasMoreTokens()) {
        wxString line = tkz.GetNextToken();
        line.Trim(false);
        line.Trim(true);

        // If it is an include, it must be unique to be written
        std::vector<wxString>::iterator it
            = std::find(includes->begin(), includes->end(), line);
        if (includes->end() == it)
            includes->push_back(line);
    }
}

void LuaCodeGenerator::FindDependencies(PObjectBase obj, std::set<PObjectInfo>& infoSet)
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

void LuaCodeGenerator::GenConstructor(PObjectBase classObj,
                                      const EventVector& events,
                                      wxString& strClassName, ArrayItems& arrays)
{
    PProperty propName = classObj->GetProperty(wxT("name"));
    if (!propName) {
        wxLogError(
            "Missing \"name\" property on \"%s\" class. Review your XML object description",
            classObj->GetClassName().c_str());
        return;
    }
    wxString strName = propName->GetValue();
    if (m_strUITable.length() > 0)
        strName = m_strUITable + "." + strName;

    m_source->WriteLn();
    m_source->WriteLn("-- create " + strClassName);
    m_source->WriteLn(strName + " = " + GetCode(classObj, "cons_call"));
    m_source->Indent();

    wxString settings = GetCode(classObj, "settings");
    if (!settings.IsEmpty())
        m_source->WriteLn(settings);

    if (classObj->GetObjectTypeName() == "wizard"
        && classObj->GetChildCount() > 0) {
        m_source->WriteLn("function add_page(page)");
        m_source->Indent();
        m_source->WriteLn("if ( #" + m_strUITable + "."
                          + strClassName + ".m_pages) > 0 then");
        m_source->Indent();
        m_source->WriteLn("local previous_page = "
                          + m_strUITable + "." + strClassName + ".m_pages[ #"
                          + m_strUITable + "." + strClassName + ".m_pages ]");
        m_source->WriteLn("page:SetPrev(previous_page)");
        m_source->WriteLn("previous_page:SetNext(page)");
        m_source->Unindent();
        m_source->WriteLn("end");
        m_source->WriteLn("table.insert( " + m_strUITable + "."
                          + strClassName + ".m_pages, page)");
        m_source->Unindent();
        m_source->WriteLn("end");
    }

    for (size_t i = 0; i < classObj->GetChildCount(); i++)
        GenConstruction(classObj->GetChild(i), true, strClassName, arrays);

    wxString afterAddChild = GetCode(classObj, "after_addchild");
    if (!afterAddChild.IsEmpty())
        m_source->WriteLn(afterAddChild);

    GenEvents(classObj, events, strClassName);

    m_source->Unindent();
}

void LuaCodeGenerator::GenDestructor(PObjectBase classObj, const EventVector& events)
{
    m_source->WriteLn();
    m_source->Indent();

    wxString strClassName;
    if (m_disconnectEvents && !events.empty())
        GenEvents(classObj, events, strClassName, true);
    else if (!classObj->GetPropertyAsInteger(wxT("aui_managed")))
        m_source->WriteLn(wxT("pass"));

    GenDestruction(classObj); // destruct objects

    m_source->Unindent();
}
wxString GetParentWindow(PObjectBase obj)
{
    wxString strName;
    PObjectBase layoutObj = obj->GetLayout();
    if (layoutObj)
        strName = layoutObj->GetClassName();

    return strName;
}
void LuaCodeGenerator::GenConstruction(PObjectBase obj, bool is_widget,
                                       wxString& strClassName, ArrayItems& arrays)
{
    wxString type = obj->GetObjectTypeName();
    PObjectInfo info = obj->GetObjectInfo();

    if (ObjectDatabase::HasCppProperties(type)) {

        wxString strName;
        PProperty propName = obj->GetProperty(wxT("name"));
        if (propName)
            strName = propName->GetValue();

        wxString strClass = obj->GetClassName();

        std::vector<wxString>::iterator itr;
        if (m_strUnsupportedClasses.end()
            != (itr = std::find(m_strUnsupportedClasses.begin(),
                                m_strUnsupportedClasses.end(), strClass))) {

            m_source->WriteLn(wxT("--Instance ") + strName + wxT(" of Control ")
                              + *itr
                              + wxT(" you try to use isn't unfortunately wrapped by wxLua."));
            m_source->WriteLn(wxT("--Please try to use another control\n"));
            m_strUnsupportedInstances.push_back(strName);
            return;
        }
        m_source->WriteLn(GetConstruction(obj, false, strClassName, arrays));

        GenSettings(obj->GetObjectInfo(), obj, strClassName);

        bool isWidget = !info->IsSubclassOf(wxT("sizer"));

        for (size_t i = 0; i < obj->GetChildCount(); i++) {
            PObjectBase child = obj->GetChild(i);
            GenConstruction(child, isWidget, strClassName, arrays);

            if (type == wxT("toolbar"))
                GenAddToolbar(child->GetObjectInfo(), child);
        }
        if (!isWidget) // sizers
        {
            wxString afterAddChild = GetCode(obj, wxT("after_addchild"));
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
                wxString _template = "#utbl#wxparent$name:SetSizer( #utbl$name ) #nl"
                                     "#utbl#wxparent$name:Layout()"
                                     "#ifnull #parent $size"
                                     "@{ #nl #utbl$name:Fit( #utbl#wxparent $name ) @}";

                LuaTemplateParser parser(obj, _template,
                                         m_useI18n, m_useRelativePath,
                                         m_basePath, m_strUserIDsVec);
                wxString res = parser.ParseTemplate();
                res.Replace(parser.RootWxParentToCode(), wxEmptyString);
                m_source->WriteLn(res);
            }
        } else if (type == wxT("splitter")) {
            // Generating the split
            switch (obj->GetChildCount()) {
            case 1: {
                PObjectBase sub1 = obj->GetChild(0)->GetChild(0);
                wxString _template = wxT("#utbl$name:Initialize(#utbl")
                    + sub1->GetProperty(wxT("name"))->GetValue() + wxT(" )");
                _template.Replace(wxT("#utbl"), m_strUITable + wxT("."));

                LuaTemplateParser parser(obj, _template, m_useI18n, m_useRelativePath,
                                         m_basePath, m_strUserIDsVec);
                m_source->WriteLn(parser.ParseTemplate());
                break;
            }
            case 2: {
                PObjectBase sub1, sub2;
                sub1 = obj->GetChild(0)->GetChild(0);
                sub2 = obj->GetChild(1)->GetChild(0);

                wxString _template;
                wxString strMode = obj->GetProperty(wxT("splitmode"))->GetValue();
                bool bSplitVertical = (strMode == wxT("wxSPLIT_VERTICAL"));
                if (bSplitVertical)
                    _template = wxT("#utbl$name:SplitVertically( ");
                else
                    _template = wxT("#utbl$name:SplitHorizontally( ");

                _template = _template + wxT("#utbl")
                    + sub1->GetProperty(wxT("name"))->GetValue() + wxT(", #utbl")
                    + sub2->GetProperty(wxT("name"))->GetValue() + wxT(", $sashpos )");
                _template = _template + wxT("#nl #utbl$name")
                    + wxT(":SetSplitMode(")
                    + wxString::Format(wxT("%d"), (bSplitVertical ? 1 : 0)) + wxT(")");
                _template.Replace(wxT("#utbl"), m_strUITable + wxT("."));

                LuaTemplateParser parser(obj, _template, m_useI18n, m_useRelativePath,
                                         m_basePath, m_strUserIDsVec);
                m_source->WriteLn(parser.ParseTemplate());
                break;
            }
            default:
                wxLogError(wxT("Missing subwindows for wxSplitterWindow widget."));
                break;
            }
        } else if (type == wxT("menubar")
                   || type == wxT("menu")
                   || type == wxT("submenu")
                   || type == wxT("toolbar")
                   || type == wxT("tool")
                   || type == wxT("listbook")
                   || type == wxT("simplebook")
                   || type == wxT("notebook")
                   || type == wxT("auinotebook")
                   || type == wxT("treelistctrl")
                   || type == wxT("wizard")) {
            wxString afterAddChild = GetCode(obj, wxT("after_addchild"));
            if (!afterAddChild.empty()) {
                m_source->WriteLn(afterAddChild);
            }
            m_source->WriteLn();
        }
    } else if (info->IsSubclassOf(wxT("sizeritembase"))) {

        // The child must be added to the sizer having in mind the
        // child object type (there are 3 different routines)
        GenConstruction(obj->GetChild(0), false, strClassName, arrays);

        PObjectInfo childInfo = obj->GetChild(0)->GetObjectInfo();
        wxString temp_name;
        if (childInfo->IsSubclassOf(
                wxT("wxWindow"))
            || wxT("CustomControl") == childInfo->GetClassName()) {
            temp_name = wxT("window_add");
        } else if (childInfo->IsSubclassOf(wxT("sizer"))) {
            temp_name = wxT("sizer_add");
        } else if (childInfo->GetClassName() == wxT("spacer")) {
            temp_name = wxT("spacer_add");
        } else {
            LogDebug(wxT("SizerItem child is not a Spacer and is not a subclass of wxWindow or of sizer."));
            return;
        }
        m_source->WriteLn(GetCode(obj, temp_name));

    } else if (type == wxT("notebookpage")
               || type == wxT("listbookpage")
               || type == wxT("simplebookpage")
               || type == wxT("choicebookpage")
               || type == wxT("auinotebookpage")) {
        GenConstruction(obj->GetChild(0), false, strClassName, arrays);
        m_source->WriteLn(GetCode(obj, wxT("page_add")));
        GenSettings(obj->GetObjectInfo(), obj, strClassName);
    } else if (type == wxT("treelistctrlcolumn")) {
        m_source->WriteLn(GetCode(obj, wxT("column_add")));
        GenSettings(obj->GetObjectInfo(), obj, strClassName);
    } else if (type == wxT("tool")) {
        // If loading bitmap from ICON resource, and size is not set, set size to toolbars bitmapsize
        // So hacky, yet so useful ...
        wxSize toolbarsize = obj->GetParent()->GetPropertyAsSize(_("bitmapsize"));
        if (wxDefaultSize != toolbarsize) {
            PProperty prop = obj->GetProperty(_("bitmap"));
            if (prop) {
                wxString oldVal = prop->GetValueAsString();
                wxString path, source;
                wxSize toolsize;
                TypeConv::ParseBitmapWithResource(oldVal, &path, &source, &toolsize);
                if (_("Load From Icon Resource") == source && wxDefaultSize == toolsize) {
                    prop->SetValue(wxString::Format(
                        wxT("%s; %s [%i; %i]"), path.c_str(), source.c_str(),
                        toolbarsize.GetWidth(), toolbarsize.GetHeight()));
                    m_source->WriteLn(GetConstruction(obj, false, wxEmptyString, arrays));
                    prop->SetValue(oldVal);
                    return;
                }
            }
        }
        m_source->WriteLn(GetConstruction(obj, false, wxEmptyString, arrays));
    } else {
        // Generate the children
        for (size_t i = 0; i < obj->GetChildCount(); i++)
            GenConstruction(obj->GetChild(i), false, strClassName, arrays);
    }
}

void LuaCodeGenerator::GenDestruction(PObjectBase obj)
{
    PCodeInfo codeInfo = obj->GetObjectInfo()->GetCodeInfo(wxT("Lua"));

    if (codeInfo) {
        wxString _template = codeInfo->GetTemplate(wxT("destruction"));
        if (!_template.empty()) {
            LuaTemplateParser parser(obj, _template, m_useI18n, m_useRelativePath,
                                     m_basePath, m_strUserIDsVec);
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

void LuaCodeGenerator::FindMacros(PObjectBase obj, std::vector<wxString>* macros)
{
    // iterate through all of the properties of all objects, add the macros
    // to the vector
    for (size_t i = 0; i < obj->GetPropertyCount(); i++) {
        PProperty prop = obj->GetProperty(i);
        if (prop->GetType() == PT_MACRO) {
            wxString value = prop->GetValue();
            if (value.IsEmpty())
                continue;

            // Skip wx IDs
            if ((!value.Contains(wxT("XRCID")))
                && (m_predMacros.end() == m_predMacros.find(value))) {
                if (macros->end() == std::find(macros->begin(), macros->end(), value))
                    macros->push_back(value);
            }
        }
    }
    for (size_t i = 0; i < obj->GetChildCount(); i++)
        FindMacros(obj->GetChild(i), macros);
}

void LuaCodeGenerator::FindEventHandlers(PObjectBase obj, EventVector& events)
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

void LuaCodeGenerator::GenDefines(PObjectBase project)
{
    std::vector<wxString> macros;
    FindMacros(project, &macros);
    m_strUserIDsVec.erase(m_strUserIDsVec.begin(), m_strUserIDsVec.end());

    // Remove the default macro from the set, for backward compatiblity
    std::vector<wxString>::iterator it
        = std::find(macros.begin(), macros.end(), wxT("ID_DEFAULT"));
    if (it != macros.end()) {
        // The default macro is defined to wxID_ANY
        m_source->WriteLn(wxT("wxID_DEFAULT = wxID_ANY -- Default"));
        macros.erase(it);
    }
    size_t id = m_firstID;
    if (id < 1000)
        wxLogWarning(wxT("First ID is Less than 1000"));

    for (it = macros.begin(); it != macros.end(); it++) {
        // Don't redefine wx IDs
        m_source->WriteLn(wxString::Format("%s = %z", it->c_str(), id));
        m_strUserIDsVec.push_back(*it);
        id++;
    }
    if (!macros.empty())
        m_source->WriteLn(wxEmptyString);
}

void LuaCodeGenerator::GenSettings(PObjectInfo info, PObjectBase obj, wxString& strClassName)
{
    PCodeInfo codeInfo = info->GetCodeInfo(wxT("Lua"));
    if (!codeInfo)
        return;

    wxString _template = codeInfo->GetTemplate(wxT("settings"));
    if (!_template.empty()) {
        LuaTemplateParser parser(obj, _template, m_useI18n, m_useRelativePath,
                                 m_basePath, m_strUserIDsVec);
        wxString code = parser.ParseTemplate();
        wxString strRootCode = parser.RootWxParentToCode();
        if (code.Find(strRootCode) != -1)
            code.Replace(strRootCode, strClassName);

        if (!code.empty())
            m_source->WriteLn(code);
    }
    // Proceeding recursively with the base classes
    for (size_t i = 0; i < info->GetBaseClassCount(false); i++) {
        PObjectInfo base_info = info->GetBaseClass(i, false);
        GenSettings(base_info, obj, strClassName);
    }
}

void LuaCodeGenerator::GenAddToolbar(PObjectInfo info, PObjectBase obj)
{
    wxArrayString arrCode;
    GetAddToolbarCode(info, obj, arrCode);
    for (size_t i = 0; i < arrCode.GetCount(); i++)
        m_source->WriteLn(arrCode[i]);
}

void LuaCodeGenerator::GetAddToolbarCode(PObjectInfo info, PObjectBase obj, wxArrayString& codelines)
{
    PCodeInfo codeInfo = info->GetCodeInfo(wxT("Lua"));
    if (!codeInfo)
        return;

    wxString _template = codeInfo->GetTemplate(wxT("toolbar_add"));
    if (!_template.empty()) {
        LuaTemplateParser parser(obj, _template, m_useI18n, m_useRelativePath,
                                 m_basePath, m_strUserIDsVec);
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

void LuaCodeGenerator::UseRelativePath(bool useRelative, wxString basePath)
{
    if (useRelative) {
        if (wxFileName::DirExists(basePath))
            m_basePath = basePath;
        else
            m_basePath = wxEmptyString;
    }
    m_useRelativePath = useRelative;
}

void LuaCodeGenerator::SetupPredefinedMacros()
{
#define ADD_PREDEFINED_MACRO(x) m_predMacros.insert(wxT(#x))

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
}

void LuaTemplateParser::SetupModulePrefixes()
{
#define ADD_PREDEFINED_PREFIX(k, v) m_predModulePrefix[wxT(#k)] = wxT(#v)

    // altered class names
    ADD_PREDEFINED_PREFIX(wxCalendarCtrl, wx.wx);
    ADD_PREDEFINED_PREFIX(wxRichTextCtrl, wx.wx);
    ADD_PREDEFINED_PREFIX(wxHtmlWindow, wx.wx);
    ADD_PREDEFINED_PREFIX(wxAuiNotebook, wxaui.wx);
    ADD_PREDEFINED_PREFIX(wxGrid, wx.wx);
    ADD_PREDEFINED_PREFIX(wxAnimationCtrl, wx.wx);

    // altered macros
    ADD_PREDEFINED_PREFIX(wxCAL_SHOW_HOLIDAYS, wx.);
    ADD_PREDEFINED_PREFIX(wxCAL_MONDAY_FIRST, wx.);
    ADD_PREDEFINED_PREFIX(wxCAL_NO_MONTH_CHANGE, wx.);
    ADD_PREDEFINED_PREFIX(wxCAL_NO_YEAR_CHANGE, wx.);
    ADD_PREDEFINED_PREFIX(wxCAL_SEQUENTIAL_MONTH_SELECTION, wx.);
    ADD_PREDEFINED_PREFIX(wxCAL_SHOW_SURROUNDING_WEEKS, wx.);
    ADD_PREDEFINED_PREFIX(wxCAL_SUNDAY_FIRST, wx.);

    ADD_PREDEFINED_PREFIX(wxHW_DEFAULT_STYLE, wx.);
    ADD_PREDEFINED_PREFIX(wxHW_NO_SELECTION, wx.);
    ADD_PREDEFINED_PREFIX(wxHW_SCROLLBAR_AUTO, wx.);
    ADD_PREDEFINED_PREFIX(wxHW_SCROLLBAR_NEVER, wx.);

    ADD_PREDEFINED_PREFIX(wxAUI_NB_BOTTOM, wxaui.);
    ADD_PREDEFINED_PREFIX(wxAUI_NB_CLOSE_BUTTON, wxaui.);
    ADD_PREDEFINED_PREFIX(wxAUI_NB_CLOSE_ON_ACTIVE_TAB, wxaui.);
    ADD_PREDEFINED_PREFIX(wxAUI_NB_CLOSE_ON_ALL_TABS, wxaui.);
    ADD_PREDEFINED_PREFIX(wxAUI_NB_DEFAULT_STYLE, wxaui.);
    ADD_PREDEFINED_PREFIX(wxAUI_NB_LEFT, wxaui.);
    ADD_PREDEFINED_PREFIX(wxAUI_NB_MIDDLE_CLICK_CLOSE, wxaui.);
    ADD_PREDEFINED_PREFIX(wxAUI_NB_RIGHT, wxaui.);
    ADD_PREDEFINED_PREFIX(wxAUI_NB_SCROLL_BUTTONS, wxaui.);
    ADD_PREDEFINED_PREFIX(wxAUI_NB_TAB_EXTERNAL_MOVE, wxaui.);
    ADD_PREDEFINED_PREFIX(wxAUI_NB_TAB_FIXED_WIDTH, wxaui.);
    ADD_PREDEFINED_PREFIX(wxAUI_NB_TAB_MOVE, wxaui.);
    ADD_PREDEFINED_PREFIX(wxAUI_NB_TAB_SPLIT, wxaui.);
    ADD_PREDEFINED_PREFIX(wxAUI_NB_TOP, wxaui.);
    ADD_PREDEFINED_PREFIX(wxAUI_NB_WINDOWLIST_BUTTON, wxaui.);

    ADD_PREDEFINED_PREFIX(wxAUI_TB_TEXT, wxaui.);
    ADD_PREDEFINED_PREFIX(wxAUI_TB_NO_TOOLTIPS, wxaui.);
    ADD_PREDEFINED_PREFIX(wxAUI_TB_NO_AUTORESIZE, wxaui.);
    ADD_PREDEFINED_PREFIX(wxAUI_TB_GRIPPER, wxaui.);
    ADD_PREDEFINED_PREFIX(wxAUI_TB_OVERFLOW, wxaui.);
    ADD_PREDEFINED_PREFIX(wxAUI_TB_VERTICAL, wxaui.);
    ADD_PREDEFINED_PREFIX(wxAUI_TB_HORZ_LAYOUT, wxaui.);
    ADD_PREDEFINED_PREFIX(wxAUI_TB_HORZ_TEXT, wxaui.);
    ADD_PREDEFINED_PREFIX(wxAUI_TB_PLAIN_BACKGROUND, wxaui.);
    ADD_PREDEFINED_PREFIX(wxAUI_TB_DEFAULT_STYLE, wxaui.);

    ADD_PREDEFINED_PREFIX(wxAC_DEFAULT_STYLE, wx.);
    ADD_PREDEFINED_PREFIX(wxAC_NO_AUTORESIZE, wx.);

#undef ADD_PREDEFINED_PREFIX
}
