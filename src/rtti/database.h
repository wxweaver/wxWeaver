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
#pragma once

#include "utils/defs.h"
#include "rtti/types.h"

#include <wx/dynlib.h>

#include <set>

class ObjectDatabase;
class ObjectTypeDictionary;
class PropertyCategory;

typedef std::shared_ptr<ObjectDatabase> PObjectDatabase;

namespace ticpp {
class Element;
}
/** Object class package.

    It will determine the grouping in the component palette.
 */
class ObjectPackage {
public:
    /** Constructor.
    */
    ObjectPackage(const wxString& name, const wxString& description,
                  const wxBitmap& icon);

    /** Adds an object information into the package.
    */
    void Add(PObjectInfo obj) { m_objs.push_back(obj); }

    /** Returns the package name.
    */
    wxString GetPackageName() { return m_name; }

    /** Returns the package description.
    */
    wxString GetPackageDescription() { return m_desc; }

    /** Returns the package icon.
    */
    wxBitmap GetPackageIcon() { return m_icon; }

    /** Returns the package objects count.
    */
    size_t GetObjectCount() { return m_objs.size(); }

    /** Returns the information about the object contained in the package.
    */
    PObjectInfo GetObjectInfo(size_t idx);

    /** If two xml files specify the same package name,
        then they merged to one package with this.

        This allows one package to be splitted across multiple xml files.
    */
    void AppendPackage(PObjectPackage package);

private:
    wxString m_name;                 // Package name
    wxString m_desc;                 // Brief package description
    wxBitmap m_icon;                 // The notebook page icon
    std::vector<PObjectInfo> m_objs; // Objects contained in the package
};

class IComponentLibrary;

/** Object database.

    All imported objects information from XML files will be stored in this class.
 */
class ObjectDatabase {
public:
    ObjectDatabase();
    ~ObjectDatabase();

    PObjectBase NewObject(PObjectInfo obj_info);

    /** Gets the information of an object from the name of the class.
    */
    PObjectInfo GetObjectInfo(const wxString& className);

    /** Sets the path where the files with the description are located.
    */
    void SetXmlPath(const wxString& path) { m_xmlPath = path; }

    /** Sets the path where the icons associated with the objects are located.
    */
    void SetIconPath(const wxString& path) { m_iconPath = path; }

    void SetPluginPath(const wxString& path) { m_pluginPath = path; }

    /** Obtains the path where the files with the object description are located.
    */
    wxString GetXmlPath() const { return m_xmlPath; }
    wxString GetIconPath() const { return m_iconPath; }
    wxString GetPluginPath() const { return m_pluginPath; }

    /** Loads the object type definitions.
    */
    bool LoadObjectTypes();

    /** Find and load plugins from the plugins directory
    */
    void LoadPlugins(PwxWeaverManager manager);

    /** Objects factory.

        A new instance of an object is created from the class name.
    */
    PObjectBase CreateObject(const std::string& className, PObjectBase parent = PObjectBase());

    /** Factory of objects from an XML object.

        This method is used to load a saved project.
    */
    PObjectBase CreateObject(ticpp::Element* obj, PObjectBase parent = PObjectBase());

    /** Copies an object
    */
    PObjectBase CopyObject(PObjectBase obj);

    /** Gets a package of objects from the given index.
    */
    PObjectPackage GetPackage(size_t idx);

    /** Gets the number of registered packages.
    */
    size_t GetPackageCount() { return m_pkgs.size(); }

    /** Reset the counters that accompany the name.

        The "name" property is a special property,
        reserved for the name of the object instance.
        Each object class has a counter associated with it so as not to duplicate
        the name when creating new objects (e.g. m_button1, m_button2...)
   */
    void ResetObjectCounters();

    static bool HasCppProperties(const wxString& type);

private:
    /** Initialize the property type map.
    */
    void InitPropertyTypes();

    /** Loads the code generation templates from a XML file.
    */
    void LoadCodeGen(const wxString& file);

    /** Loads the objects of a package with all their properties except
        inherited objects.
    */
    PObjectPackage LoadPackage(const wxString& file, const wxString& iconPath = wxEmptyString);

    void ParseProperties(ticpp::Element* elem_obj, PObjectInfo obj_info, PPropertyCategory category, std::set<PropertyType>* types);

    void ParseEvents(ticpp::Element* elem_obj, PObjectInfo obj_info, PPropertyCategory category);

    /** Imports a components library and associates it with each class.

        @throw wxWeaverException If the library could not be imported.
    */
    void ImportComponentLibrary(const wxString& libfile, PwxWeaverManager manager);

    /** Includes information inherited from objects in a package.

        In the second pass configure each package with its base objects.
    */
    void SetupPackage(const wxString& file, const wxString& path, PwxWeaverManager manager);

    /** Determines whether the object type should be exposed in the palette.
    */
    bool ShowInPalette(const wxString& type) const;

    PropertyType ParsePropertyType(const wxString& name);

    PObjectType GetObjectType(const wxString& name);

    size_t CountChildrenWithSameType(PObjectBase parent, PObjectType type);
    size_t CountChildrenWithSameType(PObjectBase parent, const std::set<PObjectType>& types);

    void SetDefaultLayoutProperties(PObjectBase obj);

    typedef std::vector<PObjectPackage> PackageVector;

    // Map the property type string to the property type number
    typedef std::map<wxString, PropertyType> PTMap;
    typedef std::map<wxString, PObjectType> ObjectTypeMap;
#ifdef __WXOSX__
    typedef std::vector<void*> LibraryVector;
#else
    typedef std::vector<wxDynamicLibrary*> LibraryVector;
#endif
    typedef void (*PFFreeComponentLibrary)(IComponentLibrary* lib);
    typedef std::map<PFFreeComponentLibrary, IComponentLibrary*> ComponentLibraryMap;
    typedef std::set<wxString> MacroSet;
    typedef std::map<wxString, PCodeInfo> LangTemplateMap;
    typedef std::map<PropertyType, LangTemplateMap> PTLangTemplateMap;

    PackageVector m_pkgs;
    LibraryVector m_libs;
    ComponentLibraryMap m_componentLibs;
    /*
        para comprobar que no se nos han quedado macros sin añadir en las
        liberias de componentes, vamos a crear un conjunto con las macros
        definidas en los XML, y al importar las librerías vamos a ir eliminando
        dichas macros del conjunto, quedando al final las macros que faltan
        por registrar en la librería.
    */
    MacroSet m_macroSet;
    ObjectTypeMap m_types; // register object types
    PTLangTemplateMap m_propertyTypeTemplates;
    PTMap m_propTypes;

    wxString m_xmlPath;
    wxString m_iconPath;
    wxString m_pluginPath;
    std::map<wxString, PObjectInfo> m_objs;

    // Used so libraries are only imported once, even if multiple libraries use them
    std::set<wxString> m_importedLibraries;
};
