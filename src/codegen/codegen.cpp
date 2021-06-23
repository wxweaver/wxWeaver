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
#include "codegen/codegen.h"

#include "rtti/objectbase.h"
#include "appdata.h"
#include "utils/exception.h"

#include <wx/tokenzr.h>

TemplateParser::TemplateParser(PObjectBase obj, wxString _template)
    : m_obj(obj)
    , m_in(_template)
    , m_indent(0)
{
}

TemplateParser::TemplateParser(const TemplateParser& other, wxString _template)
    : m_obj(other.m_obj)
    , m_in(_template)
    , m_indent(0)
{
}

TemplateParser::~TemplateParser() = default;

TemplateParser::Token TemplateParser::GetNextToken()
{
    /*
        There are 3 special characters
        #xxxx -> command
        $xxxx -> property
        %xxxx -> local variable
        @x -> Escape a special character. Example: @# is the character #.
    */
    Token result = TOK_ERROR;

    if (!m_in.Eof()) {
        wxChar c = m_in.Peek();
        if (c == '#')
            result = TOK_MACRO;
        else if (c == '$')
            result = TOK_PROPERTY;
        else
            result = TOK_TEXT;
    }
    return result;
}

bool TemplateParser::ParseMacro()
{
    Ident ident = ParseIdent();
    switch (ident) {
    case ID_WXPARENT:
        return ParseWxParent();
        break;
    case ID_PARENT:
        return ParseParent();
        break;
    case ID_FORM:
        return ParseForm();
        break;
    case ID_IFNOTNULL:
        return ParseIfNotNull();
        break;
    case ID_IFNULL:
        return ParseIfNull();
        break;
    case ID_FOREACH:
        return ParseForEach();
        break;
    case ID_PREDEFINED:
        return ParsePred();
        break;
    case ID_PREDEFINED_INDEX:
        return ParseNPred();
        break;
    case ID_CHILD:
        return ParseChild();
        break;
    case ID_NEWLINE:
        return ParseNewLine();
        break;
    case ID_IFEQUAL:
        ParseIfEqual();
        break;
    case ID_IFNOTEQUAL:
        ParseIfNotEqual();
        break;
    case ID_IFPARENTTYPEEQUAL:
        ParseIfParentTypeEqual();
        break;
    case ID_IFPARENTCLASSEQUAL:
        ParseIfParentClassEqual();
        break;
    case ID_IFPARENTTYPENOTEQUAL:
        ParseIfParentTypeNotEqual();
        break;
    case ID_IFPARENTCLASSNOTEQUAL:
        ParseIfParentClassNotEqual();
        break;
    case ID_APPEND:
        ParseAppend();
        break;
    case ID_CLASS:
        ParseClass();
        break;
    case ID_INDENT:
        ParseIndent();
        break;
    case ID_UNINDENT:
        ParseUnindent();
        break;
    case ID_IFTYPEEQUAL:
        ParseIfTypeEqual();
        break;
    case ID_IFTYPENOTEQUAL:
        ParseIfTypeNotEqual();
        break;
    case ID_UTBL:
        ParseLuaTable();
        break;
    default:
        wxWEAVER_THROW_EX("Invalid Macro Type");
        break;
    }
    return true;
}

TemplateParser::Ident TemplateParser::ParseIdent()
{
    Ident ident = ID_ERROR;
    if (!m_in.Eof()) {
        wxString macro;
        m_in.GetC();

        wxChar peek(m_in.Peek());
        while (peek != wxChar(EOF) && !m_in.Eof() && peek != '#' && peek != '$'
               && ((peek >= 'a'
                    && peek <= 'z')
                   || (peek >= 'A'
                       && peek <= 'Z')
                   || (peek >= '0'
                       && peek <= '9'))) {
            macro += wxChar(m_in.GetC());
            peek = wxChar(m_in.Peek());
        }
        // Searching the identifier
        ident = SearchIdent(macro);
    }
    return ident;
}

wxString TemplateParser::ParsePropertyName(wxString* child)
{
    wxString propname;

    // children of parent properties can be referred to with a '/'
    // like "$parent/child"
    bool foundSlash = false;
    /*
        property names used in templates may be encapsulated by curly brackets
        (e.g. ${name}) so they can be surrounded by the template content
        without any white spaces now.
    */
    bool foundLeftCurlyBracket = false;
    bool saveChild = (child != nullptr);

    if (!m_in.Eof()) {
        m_in.GetC();

        wxChar peek(m_in.Peek());
        while (peek != wxChar(EOF) && !m_in.Eof() && peek != '#' && peek != '$'
               && ((peek >= 'a' && peek <= 'z')
                   || (peek >= 'A' && peek <= 'Z')
                   || (peek >= '0' && peek <= '9')
                   || (peek >= '{' && peek <= '}')
                   || peek == '_' || peek == '/')) {
            if (foundSlash) {
                if (saveChild)
                    (*child) << wxChar(m_in.GetC());
            } else {
                wxChar next = wxChar(m_in.GetC());
                if ('{' == next)
                    foundLeftCurlyBracket = true;
                else if (('}' == next) && (foundLeftCurlyBracket == true))
                    break;
                else if ('/' == next)
                    foundSlash = true;
                else
                    propname << next;
            }
            peek = wxChar(m_in.Peek());
        }
    }
    return propname;
}

bool TemplateParser::ParseProperty()
{
    wxString childName;
    wxString propname = ParsePropertyName(&childName);

    PProperty property = m_obj->GetProperty(propname);
    if (!property) {
        wxLogError("The property '%s' does not exist for objects of class '%s'",
                   propname.c_str(), m_obj->GetClassName().c_str());
        return true;
    }
    if (childName.empty()) {
        wxString code = PropertyToCode(property);
        m_out << code;
    } else {
        m_out << property->GetChildFromParent(childName);
    }
#if 0
    LogDebug("parsing property %s", propname.c_str());
#endif
    return true;
}

bool TemplateParser::ParseText()
{
    wxString text;
    int sspace = 0;

    if (!m_in.Eof()) {
        wxChar peek(m_in.Peek());

        while (peek != wxChar(EOF)
               && !m_in.Eof()
               && peek != '#'
               && peek != '$') {
            wxChar c(m_in.GetC());
            if (c == '@') {
                c = wxChar(m_in.GetC());
                if (c == ' ')
                    sspace++;
            }
            text << c;
            peek = wxChar(m_in.Peek());
        }
        if (text.find_first_not_of("\r\n\t ") != text.npos) {
            // If text is all whitespace, ignore it
            m_out << text;
        } else {
            // ... but allow all '@ ' instances
            wxString spaces(' ', sspace);
            m_out << spaces;
        }
    }
#if 0
    LogDebug("Parsed Text: %s", text.c_str());
#endif
    return true;
}

bool TemplateParser::ParseInnerTemplate()
{
    return true;
}

PObjectBase TemplateParser::GetWxParent()
{
    PObjectBase wxparent, prev_wxparent;

    std::vector<PObjectBase> candidates;
    candidates.push_back(m_obj->FindNearAncestor("container"));
    candidates.push_back(m_obj->FindNearAncestor("notebook"));
    candidates.push_back(m_obj->FindNearAncestor("splitter"));
    candidates.push_back(m_obj->FindNearAncestor("listbook"));
    candidates.push_back(m_obj->FindNearAncestor("choicebook"));
    candidates.push_back(m_obj->FindNearAncestor("simplebook"));
    candidates.push_back(m_obj->FindNearAncestor("toolbook"));
    candidates.push_back(m_obj->FindNearAncestor("treebook"));
    candidates.push_back(m_obj->FindNearAncestor("auinotebook"));
    candidates.push_back(m_obj->FindNearAncestor("toolbar"));
    candidates.push_back(m_obj->FindNearAncestor("wizardpagesimple"));
    candidates.push_back(m_obj->FindNearAncestorByBaseClass("wxStaticBoxSizer"));

    for (size_t i = 0; i < candidates.size(); i++) {
        if (!wxparent) {
            wxparent = candidates[i];
        } else {
            if (candidates[i] && candidates[i]->Deep() > wxparent->Deep())
                wxparent = candidates[i];
        }
        if (wxparent && wxparent->GetClassName() == "wxStaticBoxSizer"
            && !wxparent->GetProperty("parent")->GetValueAsInteger()) {
            wxparent = prev_wxparent;
        }
        prev_wxparent = wxparent;
    }
    return wxparent;
}

bool TemplateParser::ParseWxParent()
{
    PObjectBase wxparent(GetWxParent());

    if (wxparent) {
        PProperty property = GetRelatedProperty(wxparent);
#if 0
        m_out << PropertyToCode(property);
#endif
        const auto& classname = wxparent->GetClassName();
        if (classname == "wxStaticBoxSizer") {
            // We got a wxStaticBoxSizer as parent,
            // use the special PT_WXPARENT_SB type to generate code
            // to get its static box
            m_out << ValueToCode(PT_WXPARENT_SB, property->GetValue());
        } else if (classname == "wxCollapsiblePane") {
            // We got a wxCollapsiblePane as parent,
            // use the special PT_WXPARENT_CP type to generate code
            // to get its pane
            m_out << ValueToCode(PT_WXPARENT_CP, property->GetValue());
        } else {
            m_out << ValueToCode(PT_WXPARENT, property->GetValue());
        }
    } else {
        ignoreLeadingWhitespaces();
        ParsePropertyName();
        m_out << RootWxParentToCode();
    }
    return true;
}

bool TemplateParser::ParseForm()
{
    PObjectBase form(m_obj);
    PObjectBase parent(form->GetParent());

    if (!parent)
        return false;

    // form is a form when grandparent is null
    PObjectBase grandparent = parent->GetParent();
    while (grandparent) {
        form = parent;
        parent = grandparent;
        grandparent = grandparent->GetParent();
    }

    PProperty property = GetRelatedProperty(form);
    m_out << PropertyToCode(property);

    return true;
}

void TemplateParser::ParseLuaTable()
{
    const auto& project = AppData()->GetProjectData();
    const auto& table = project->GetProperty("ui_table");
    if (table) {
        auto strTableName = table->GetValueAsString();
        if (strTableName.empty())
            strTableName = "UI";

        m_out << strTableName << ".";
    }
}

bool TemplateParser::ParseParent()
{
    PObjectBase parent(m_obj->GetParent());
    if (parent) {
        PProperty property = GetRelatedProperty(parent);
        m_out << PropertyToCode(property);
    } else {
        m_out << "ERROR";
    }
    return true;
}

bool TemplateParser::ParseChild()
{
    // Get the first child
    PObjectBase child(m_obj->GetChild(0));

    if (child) {
        PProperty property = GetRelatedProperty(child);
        m_out << PropertyToCode(property);
    } else
        m_out << RootWxParentToCode();

    return true;
}

PProperty TemplateParser::GetRelatedProperty(PObjectBase relative)
{
    ignoreLeadingWhitespaces();
    wxString propname = ParsePropertyName();
    return relative->GetProperty(propname);
}

bool TemplateParser::ParseForEach()
{
    // Whitespaces at the very start are ignored
    ignoreLeadingWhitespaces();

    // parsing the property
    if (GetNextToken() == TOK_PROPERTY) {
        wxString propname = ParsePropertyName();
        wxString inner_template = ExtractInnerTemplate();

        PProperty property = m_obj->GetProperty(propname);
        wxString propvalue = property->GetValue();

        // Property value must be an string using ',' as separator.
        // The template will be generated nesting as many times as
        // tokens were found in the property value.

        if (property->GetType() == PT_INTLIST
            || property->GetType() == PT_UINTLIST
            || property->GetType() == PT_INTPAIRLIST
            || property->GetType() == PT_UINTPAIRLIST) {

            // For doing that we will use wxStringTokenizer class from wxWidgets
            wxStringTokenizer tkz(propvalue, ",");
            int i = 0;
            while (tkz.HasMoreTokens()) {
                wxString token;
                token = tkz.GetNextToken();
                token.Trim(true);
                token.Trim(false);

                // Pair values get interpreted as adjacent parameters,
                // all supported languages use comma as parameter separator
                token.Replace(":", ", ");

                // Parsing the internal template
                {
                    wxString code;
                    PTemplateParser parser = CreateParser(this, inner_template);
                    parser->SetPredefined(token, wxString::Format("%i", i++));
                    code = parser->ParseTemplate();
                    m_out << "\n"
                          << code;
                }
            }
        } else if (property->GetType() == PT_STRINGLIST) {
            wxArrayString array = property->GetValueAsArrayString();
            for (size_t i = 0; i < array.Count(); i++) {
                wxString code;
                PTemplateParser parser = CreateParser(this, inner_template);
                parser->SetPredefined(
                    ValueToCode(PT_WXSTRING_I18N, array[i]),
                    wxString::Format("%i", i));

                code = parser->ParseTemplate();
                m_out << "\n"
                      << code;
            }
        } else
            wxLogError("Property type not compatible with \"foreach\" macro");
    }
    return true;
}

PProperty TemplateParser::GetProperty(wxString* childName)
{
    PProperty property;

    // Check for #wxparent, #parent, or #child
    if (GetNextToken() == TOK_MACRO) {
        try {
            Ident ident = ParseIdent();
            switch (ident) {
            case ID_WXPARENT: {
                PObjectBase wxparent(GetWxParent());
                if (wxparent)
                    property = GetRelatedProperty(wxparent);

                break;
            }
            case ID_PARENT: {
                PObjectBase parent(m_obj->GetParent());
                if (parent)
                    property = GetRelatedProperty(parent);

                break;
            }
            case ID_CHILD: {
                PObjectBase child(m_obj->GetChild(0));
                if (child)
                    property = GetRelatedProperty(child);

                break;
            }
            default:
                break;
            }
        } catch (wxWeaverException& ex) {
            wxLogError(ex.what());
        }
    }

    if (!property) {
        if (GetNextToken() == TOK_PROPERTY) {
            wxString propname = ParsePropertyName(childName);
            property = m_obj->GetProperty(propname);
        }
    }
    return property;
}

void TemplateParser::ignoreLeadingWhitespaces()
{
    wxChar peek(m_in.Peek());
    while (peek != wxChar(EOF) && !m_in.Eof() && peek == ' ') {
        m_in.GetC();
        peek = wxChar(m_in.Peek());
    }
}

bool TemplateParser::ParseIfNotNull()
{
    ignoreLeadingWhitespaces();

    // Get the property
    wxString childName;
    PProperty property(GetProperty(&childName));
    if (!property)
        return false;

    wxString innerTemplate = ExtractInnerTemplate();

    if (!property->IsNull()) {
        if (!childName.empty()) {
            if (property->GetChildFromParent(childName).empty())
                return true;
        }
        // Generate the code from the block
        PTemplateParser parser = CreateParser(this, innerTemplate);
        m_out << parser->ParseTemplate();
    }
    return true;
}

bool TemplateParser::ParseIfNull()
{
    ignoreLeadingWhitespaces();

    // Get the property
    wxString childName;
    PProperty property(GetProperty(&childName));
    if (!property)
        return false;

    wxString inner_template = ExtractInnerTemplate();

    if (property->IsNull()) {
        // Generate the code from the block
        PTemplateParser parser = CreateParser(this, inner_template);
        m_out << parser->ParseTemplate();
    } else {
        if (!childName.empty()) {
            if (property->GetChildFromParent(childName).empty()) {
                // Generate the code from the block
                PTemplateParser parser = CreateParser(this, inner_template);
                m_out << parser->ParseTemplate();
            }
        }
    }
    return true;
}

wxString TemplateParser::ExtractLiteral()
{
    wxString os;
    wxChar c;

    // Whitespaces at the very start are ignored
    ignoreLeadingWhitespaces();

    c = wxChar(m_in.GetC()); // Initial quotation mark

    if (c == '"') {
        bool end = false;
        // Beginning the template extraction
        while (!end && !m_in.Eof() && m_in.Peek() != EOF) {
            c = wxChar(m_in.GetC()); // obtaining one char

            // Checking for a possible closing quotation mark
            if (c == '"') {
                if (m_in.Peek() == '"') // Char (") denoted as ("")
                {
                    m_in.GetC(); // Second quotation mark is ignored
                    os << '"';
                } else // Closing
                {
                    end = true;

                    // All the following chars are ignored up to an space char,
                    // so we can avoid errors like "hello"world" -> "hello"
                    wxChar peek(m_in.Peek());
                    while (peek != wxChar(EOF) && !m_in.Eof() && peek != ' ') {
                        m_in.GetC();
                        peek = wxChar(m_in.Peek());
                    }
                }
            } else // one char from literal (N.B. ??)
                os << c;
        }
    }
    return os;
}

bool TemplateParser::ParseIfEqual()
{
    // ignore leading whitespace
    ignoreLeadingWhitespaces();

    // Get the property
    wxString childName;
    PProperty property(GetProperty(&childName));
    if (property) {
        // Get the value to compare to
        wxString value = ExtractLiteral();

        // Get the template to generate if comparison is true
        wxString inner_template = ExtractInnerTemplate();

        // Get the value of the property
        wxString propValue;
        if (childName.empty())
            propValue = property->GetValue();
        else
            propValue = property->GetChildFromParent(childName);

        // Compare
        //if ( propValue == value )
        if (IsEqual(propValue, value)) {
            // Generate the code
            PTemplateParser parser = CreateParser(this, inner_template);
            m_out << parser->ParseTemplate();
            return true;
        }
    }
    return false;
}

bool TemplateParser::ParseIfNotEqual()
{
    ignoreLeadingWhitespaces();

    // Get the property
    wxString childName;
    PProperty property(GetProperty(&childName));
    if (property) {
        // Get the value to compare to
        wxString value = ExtractLiteral();

        // Get the template to generate if comparison is false
        wxString inner_template = ExtractInnerTemplate();

        // Get the value of the property
        wxString propValue;
        if (childName.empty()) {
            propValue = property->GetValue();
        } else {
            propValue = property->GetChildFromParent(childName);
        }

        // Compare
        if (propValue != value) {
            // Generate the code
            PTemplateParser parser = CreateParser(this, inner_template);
            m_out << parser->ParseTemplate();
            ;
            return true;
        }
    }
    return false;
}

bool TemplateParser::ParseIfParentTypeEqual()
{
    PObjectBase parent(m_obj->GetParent());

    // get examined type name
    wxString type = ExtractLiteral();

    // get the template to generate if comparison is true
    wxString inner_template = ExtractInnerTemplate();

    // compare give type name with type of the wx parent object
    if (parent && IsEqual(parent->GetTypeName(), type)) {
        // generate the code
        PTemplateParser parser = CreateParser(this, inner_template);
        m_out << parser->ParseTemplate();
        return true;
    }
    return false;
}

bool TemplateParser::ParseIfParentTypeNotEqual()
{
    PObjectBase parent(m_obj->GetParent());

    // get examined type name
    wxString type = ExtractLiteral();

    // get the template to generate if comparison is true
    wxString inner_template = ExtractInnerTemplate();

    // compare give type name with type of the wx parent object
    if (parent && !IsEqual(parent->GetTypeName(), type)) {
        // generate the code
        PTemplateParser parser = CreateParser(this, inner_template);
        m_out << parser->ParseTemplate();
        return true;
    }

    return false;
}

bool TemplateParser::ParseIfParentClassEqual()
{
    PObjectBase parent(m_obj->GetParent());

    // get examined type name
    wxString type = ExtractLiteral();

    // get the template to generate if comparison is true
    wxString inner_template = ExtractInnerTemplate();

    // compare give type name with type of the wx parent object
    if (parent && IsEqual(parent->GetClassName(), type)) {
        // generate the code
        PTemplateParser parser = CreateParser(this, inner_template);
        m_out << parser->ParseTemplate();
        return true;
    }
    return false;
}

bool TemplateParser::ParseIfParentClassNotEqual()
{
    PObjectBase parent(m_obj->GetParent());

    // get examined type name
    wxString type = ExtractLiteral();

    // get the template to generate if comparison is true
    wxString inner_template = ExtractInnerTemplate();

    // compare give type name with type of the wx parent object
    if (parent && !IsEqual(parent->GetClassName(), type)) {
        // generate the code
        PTemplateParser parser = CreateParser(this, inner_template);
        m_out << parser->ParseTemplate();
        return true;
    }
    return false;
}

bool TemplateParser::ParseIfTypeEqual()
{
    // get examined type name
    wxString type = ExtractLiteral();

    // get the template to generate if comparison is true
    wxString inner_template = ExtractInnerTemplate();

    // compare give type name with type of the wx parent object
    if (IsEqual(m_obj->GetTypeName(), type)) {
        // generate the code
        PTemplateParser parser = CreateParser(this, inner_template);
        m_out << parser->ParseTemplate();
        return true;
    }
    return false;
}

bool TemplateParser::ParseIfTypeNotEqual()
{
    // get examined type name
    wxString type = ExtractLiteral();

    // get the template to generate if comparison is true
    wxString inner_template = ExtractInnerTemplate();

    // compare give type name with type of the wx parent object
    if (!IsEqual(m_obj->GetTypeName(), type)) {
        // generate the code
        PTemplateParser parser = CreateParser(this, inner_template);
        m_out << parser->ParseTemplate();
        return true;
    }
    return false;
}

TemplateParser::Ident TemplateParser::SearchIdent(wxString ident)
{
    //  LogDebug("Parsing command %s",ident.c_str());

    if (ident == "wxparent")
        return ID_WXPARENT;
    else if (ident == "ifnotnull")
        return ID_IFNOTNULL;
    else if (ident == "ifnull")
        return ID_IFNULL;
    else if (ident == "foreach")
        return ID_FOREACH;
    else if (ident == "pred")
        return ID_PREDEFINED;
    else if (ident == "npred")
        return ID_PREDEFINED_INDEX;
    else if (ident == "child")
        return ID_CHILD;
    else if (ident == "parent")
        return ID_PARENT;
    else if (ident == "nl")
        return ID_NEWLINE;
    else if (ident == "ifequal")
        return ID_IFEQUAL;
    else if (ident == "ifnotequal")
        return ID_IFNOTEQUAL;
    else if (ident == "ifparenttypeequal")
        return ID_IFPARENTTYPEEQUAL;
    else if (ident == "ifparentclassequal")
        return ID_IFPARENTCLASSEQUAL;
    else if (ident == "ifparenttypenotequal")
        return ID_IFPARENTTYPENOTEQUAL;
    else if (ident == "ifparentclassnotequal")
        return ID_IFPARENTCLASSNOTEQUAL;
    else if (ident == "append")
        return ID_APPEND;
    else if (ident == "class")
        return ID_CLASS;
    else if (ident == "form" || ident == "wizard")
        return ID_FORM;
    else if (ident == "indent")
        return ID_INDENT;
    else if (ident == "unindent")
        return ID_UNINDENT;
    else if (ident == "iftypeequal")
        return ID_IFTYPEEQUAL;
    else if (ident == "iftypenotequal")
        return ID_IFTYPENOTEQUAL;
    else if (ident == "utbl")
        return ID_UTBL;
    else
        wxWEAVER_THROW_EX(wxString::Format("Unknown macro: \"%s\"", ident.c_str()));
}

wxString TemplateParser::ParseTemplate()
{
    try {
        while (!m_in.Eof()) {
            Token token = GetNextToken();
            switch (token) {
            case TOK_MACRO:
                ParseMacro();
                break;
            case TOK_PROPERTY:
                ParseProperty();
                break;
            case TOK_TEXT:
                ParseText();
                break;
            default:
                return wxEmptyString;
            }
        }
    } catch (wxWeaverException& ex) {
        wxLogError(ex.what());
    }
    return m_out;
}

/**
* Obtaining the template enclosed between '@{' y '@}'.
* Note: whitespaces at the very start will be ignored.
*/
wxString TemplateParser::ExtractInnerTemplate()
{
    wxString os;
    wxChar c1, c2;
    ignoreLeadingWhitespaces();

    // The two following characters must be '@{'
    c1 = wxChar(m_in.GetC());
    c2 = wxChar(m_in.GetC());

    if (c1 == '@' && c2 == '{') {
        ignoreLeadingWhitespaces();

        int level = 1;
        bool end = false;
        // Beginning with the template extraction
        while (!end && !m_in.Eof() && m_in.Peek() != EOF) {
            c1 = wxChar(m_in.GetC());

            // Checking if there are initial or closing braces
            if (c1 == '@') {
                c2 = wxChar(m_in.GetC());

                if (c2 == '}') {
                    level--;
                    if (!level) {
                        end = true;
                    } else {
                        // There isn't a final closing brace, so that we put in
                        // the chars and continue
                        os << c1;
                        os << c2;
                    }
                } else {
                    os << c1;
                    os << c2;

                    if (c2 == '{')
                        level++;
                }
            } else {
                os << c1;
            }
        }
    }
    return os;
}

bool TemplateParser::ParsePred()
{
    if (!m_pred.empty())
        m_out << m_pred;

    return true;
}

bool TemplateParser::ParseNPred()
{
    if (!m_npred.empty())
        m_out << m_npred;

    return true;
}

bool TemplateParser::ParseNewLine()
{
    m_out << '\n';

    // append custom indentation define in code templates
    // (will be replace by '\t' in code writer)
    for (int i = 0; i < m_indent; i++)
        m_out << "%TAB%";

    return true;
}

void TemplateParser::ParseAppend()
{
    ignoreLeadingWhitespaces();
    /*
    NOTE: This macro is usually used to attach some postfix to a name
          to create another unique name.
          If the name contains array brackets the resulting name is not a valid identifier.
          You cannot simply replace all brackets, depending on how many times
          #append is used in a template there might be preceeding brackets that
          need to be preserved. Here we assume #append is used directly after
          an array name to attach something to it, we have to search for the last
          delimiter or start of line and replace all brackets after this one, not before.
    */
    if (!m_out.empty() && m_out.GetChar(m_out.size() - 1) == ']') {
        auto pos = m_out.find_last_of(" \t\r\n.>");
        if (pos == wxString::npos)
            pos = 0;
        else
            ++pos;

        for (pos = m_out.find_first_of("[]", pos);
             pos != wxString::npos;
             pos = m_out.find_first_of("[]", pos + 1)) {
            m_out[pos] = '_';
        }
    }
}

void TemplateParser::ParseClass()
{
    PProperty subclass_prop = m_obj->GetProperty("subclass");
    if (subclass_prop) {
        wxString subclass = subclass_prop->GetChildFromParent("name");
        if (!subclass.empty()) {
            m_out << subclass;
            return;
        }
    }
    m_out << ValueToCode(PT_CLASS, m_obj->GetClassName());
}

void TemplateParser::ParseIndent()
{
    m_indent++;
}

void TemplateParser::ParseUnindent()
{
    m_indent--;

    if (m_indent < 0)
        m_indent = 0;
}

wxString TemplateParser::PropertyToCode(PProperty property)
{
    if (property)
        return ValueToCode(property->GetType(), property->GetValue());
    else
        return wxEmptyString;
}

bool TemplateParser::IsEqual(const wxString& value, const wxString& set)
{
    bool contains = false;

    wxStringTokenizer tokens(set, "||");
    while (tokens.HasMoreTokens()) {
        wxString token = tokens.GetNextToken();
        token.Trim().Trim(false);
        if (token == value) {
            contains = true;
            break;
        }
    }
    return contains;
}

CodeGenerator::~CodeGenerator() = default;

void CodeGenerator::FindArrayObjects(PObjectBase obj, ArrayItems& arrays, bool skipRoot)
{
    if (!skipRoot) {
        const auto& propName = obj->GetProperty("name");
        if (propName) {
            const auto& name = propName->GetValue();
            wxString baseName;
            ArrayItem item;
            if (ParseArrayName(name, baseName, item)) {
                auto& baseItem = arrays[baseName];
                for (size_t i = 0; i < item.maxIndex.size(); ++i) {
                    if (i < baseItem.maxIndex.size()) {
                        if (baseItem.maxIndex[i] < item.maxIndex[i])
                            baseItem.maxIndex[i] = item.maxIndex[i];
                    } else {
                        baseItem.maxIndex.push_back(item.maxIndex[i]);
                    }
                }
            }
        }
    }
    for (size_t i = 0; i < obj->GetChildCount(); ++i)
        FindArrayObjects(obj->GetChild(i), arrays, false);
}

bool CodeGenerator::ParseArrayName(const wxString& name, wxString& baseName, ArrayItem& item)
{
    bool isArray = false;
    auto indexStart = name.find_first_of('[');

    while (indexStart != wxString::npos) {
        const auto indexEnd = name.find_first_of(']', indexStart + 1);
        if (indexEnd == wxString::npos)
            break;

        unsigned long indexValue;
        if (!name.substr(indexStart + 1, indexEnd - indexStart - 1).ToULong(&indexValue))
            return false;

        if (!isArray) {
            baseName = name.substr(0, indexStart);
            item.maxIndex.clear();
            isArray = true;
        }
        item.maxIndex.push_back(indexValue);
        indexStart = name.find_first_of('[', indexEnd + 1);
    }
    return isArray;
}
