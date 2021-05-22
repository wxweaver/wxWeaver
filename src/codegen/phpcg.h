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
#pragma once

#include "codegen.h"

#include <set>
#include <vector>

/** Parse the PHP templates.
*/
class PHPTemplateParser : public TemplateParser {
public:
    PHPTemplateParser(PObjectBase obj, wxString _template,
                      bool useI18N, bool useRelativePath, wxString basePath);

    PHPTemplateParser(const PHPTemplateParser& other, wxString _template);

    // overrides for PHP
    PTemplateParser CreateParser(const TemplateParser* oldparser,
                                 wxString _template) override;
    wxString RootWxParentToCode() override;

    /** Convert the value of the property to PHP code
    */
    wxString ValueToCode(PropertyType type, wxString value) override;

private:
    wxString m_basePath;

    bool m_useI18n;
    bool m_useRelativePath;
};

/** Generate the PHP code
*/
class PHPCodeGenerator : public CodeGenerator {
public:
    PHPCodeGenerator();

    /** Convert a wxString to the "C/C++" format.
    */
    static wxString ConvertPHPString(wxString text);

    /** Set the codewriter for the source file
    */
    void SetSourceWriter(PCodeWriter cw) { m_source = cw; }

    /** Configures the reference path for generating relative paths to
        that passed as parameter.

        @note path is generated with the separators, '/',
              since on Windows the compilers interpret path correctly.
    */
    void UseRelativePath(bool relative = false, wxString basePath = wxEmptyString);

    /** Set the First ID used during Code Generation.
    */
    void SetFirstID(const size_t id) { m_firstID = id; }

    /** Generate the project's code
    */
    bool GenerateCode(PObjectBase project) override;

    /** Generate an inherited class
    */
    void GenerateInheritedClass(PObjectBase userClasses, PObjectBase form);

private:
    void SetupPredefinedMacros();

    /** Given an object and the name for a template, obtains the code.
    */
    wxString GetCode(PObjectBase obj, wxString name, bool silent = false);

    /** Gets the construction fragment for the specified object.

        This method encapsulates the adjustments that need to be made for array declarations.
    */
    wxString GetConstruction(PObjectBase obj, ArrayItems& arrays);

    /** Stores the project's objects classes set, for generating the includes.
    */
    void FindDependencies(PObjectBase obj, std::set<PObjectInfo>& info_set);

    /** Stores all the properties for "macro" type objects,
        so that their related '#define' can be generated subsequently.
    */
    void FindMacros(PObjectBase obj, std::vector<wxString>* macros);

    /** Looks for "non-null" event handlers (PEvent) and collects it into a vector.
     */
    void FindEventHandlers(PObjectBase obj, EventVector& events);

    /** Generates classes declarations inside the header file.
    */
    void GenClassDeclaration(PObjectBase class_obj, bool use_enum,
                             const wxString& classDecoration,
                             const EventVector& events,
                             const wxString& eventHandlerPostfix,
                             ArrayItems& arrays);

    /** Generates the event table.
    */
    void GenEvents(PObjectBase class_obj, const EventVector& events,
                   bool disconnect = false);

    /** Helper function to find the event table entry template in the class
        or its base classes.
    */
    bool GenEventEntry(PObjectBase obj, PObjectInfo obj_info,
                       const wxString& templateName,
                       const wxString& handlerName, bool disconnect = false);

    /** Generates the generated_event_handlers template
    */
    void GetGenEventHandlers(PObjectBase obj);

    /** Generates the generated_event_handlers template
    */
    void GenDefinedEventHandlers(PObjectInfo info, PObjectBase obj);

    /** Generates the 'import' section for files.
    */
    void GenIncludes(PObjectBase project, std::vector<wxString>* includes,
                     std::set<wxString>* templates);

    void GenObjectIncludes(PObjectBase project, std::vector<wxString>* includes,
                           std::set<wxString>* templates);

    void GenBaseIncludes(PObjectInfo info, PObjectBase obj,
                         std::vector<wxString>* includes,
                         std::set<wxString>* templates);

    void AddUniqueIncludes(const wxString& include, std::vector<wxString>* includes);

    /** Generate a set of all subclasses to forward declare in the generated file.
    */
    void GenSubclassSets(PObjectBase obj, std::set<wxString>* subclasses,
                         std::vector<wxString>* headerIncludes);

    /** Generates the '#define' section for macros.
    */
    void GenDefines(PObjectBase project);

    /** Generates the constructor for a class
    */
    void GenConstructor(PObjectBase class_obj, const EventVector& events,
                        ArrayItems& arrays);

    /** Generates the destructor for a class
    */
    void GenDestructor(PObjectBase classObj, const EventVector& events);

    /** Makes the objects construction, setting up the objects' and Layout properties.

        The algorithm is simmilar to that used in the designer preview generation.
    */
    void GenConstruction(PObjectBase obj, bool is_widget, ArrayItems& arrays);

    /** Makes the objects destructions.
    */
    void GenDestruction(PObjectBase obj);

    /** Configures the object properties, both own and inherited ones.

        Information for the class is given, because it will recursively make the
        configuration in the "super-classes".
    */
    void GenSettings(PObjectInfo info, PObjectBase obj);

    /** Adds a control for a toolbar.

        Needs the objectinfo (wxWindow type) where the template is found,
        and the objectbase for the control.
    */
    void GenAddToolbar(PObjectInfo info, PObjectBase obj);

    void GetAddToolbarCode(PObjectInfo info, PObjectBase obj, wxArrayString& codelines);

    void GenVirtualEventHandlers(const EventVector& events,
                                 const wxString& eventHandlerPostfix);
    PCodeWriter m_source;

    wxString m_basePath;
    wxString m_disconnecMode;

    std::set<wxString> m_predMacros; // Predefined macros won't generate defines.

    size_t m_firstID;
    bool m_useRelativePath;
    bool m_useI18n;
    bool m_disconnectEvents;
};
