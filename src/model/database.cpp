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
#include "model/database.h"

#include "rad/bitmaps.h"
#include "utils/debug.h"
#include "utils/stringutils.h"
#include "utils/typeconv.h"
#include "utils/exception.h"
#include "model/objectbase.h"

#include <ticpp.h>

#include <wx/dir.h>
#include <wx/filename.h>
#include <wx/stdpaths.h>

#if 0
#define DEBUG_PRINT(x) cout << x
#endif

#define OBJINFO_TAG "objectinfo"
#define CODEGEN_TAG "codegen"
#define TEMPLATE_TAG "template"
#define NAME_TAG "name"
#define DESCRIPTION_TAG "help"
#define CUSTOM_EDITOR_TAG "editor"
#define PROPERTY_TAG "property"
#define CHILD_TAG "child"
#define EVENT_TAG "event"
#define EVENT_CLASS_TAG "class"
#define CATEGORY_TAG "category"
#define OBJECT_TAG "object"
#define CLASS_TAG "class"
#define PACKAGE_TAG "package"
#define PKGDESC_TAG "desc"
#define PRGLANG_TAG "language"
#define ICON_TAG "icon"
#define SMALL_ICON_TAG "smallIcon"
#define EXPANDED_TAG "expanded"
#define WXVERSION_TAG "wxversion"

#ifdef __WXMAC__
#include <dlfcn.h>
#endif

ObjectPackage::ObjectPackage(wxString name, wxString desc, wxBitmap icon)
    : m_name(name)
    , m_desc(desc)
    , m_icon(icon)
{
}

PObjectInfo ObjectPackage::GetObjectInfo(size_t idx)
{
    assert(idx < m_objs.size());
    return m_objs[idx];
}

void ObjectPackage::AppendPackage(PObjectPackage package)
{
    m_objs.insert(m_objs.end(), package->m_objs.begin(), package->m_objs.end());
}

ObjectDatabase::ObjectDatabase()
{
#if 0
    InitObjectTypes();
    InitWidgetTypes();
#endif
    InitPropertyTypes();
}

ObjectDatabase::~ObjectDatabase()
{
    for (ComponentLibraryMap::iterator lib = m_componentLibs.begin();
         lib != m_componentLibs.end(); ++lib)
        (*(lib->first))(lib->second);

    for (LibraryVector::iterator lib = m_libs.begin(); lib != m_libs.end(); ++lib) {
#ifdef wxWEAVER_DEBUG
// TODO: Only unload in release, can't get a good stack trace if the library is unloaded
#ifdef __WXMAC__
        dlclose(*lib);
#else
        (*lib)->Detach();
#endif
#endif
#ifndef __WXMAC__
        delete *lib;
#endif
    }
}

PObjectInfo ObjectDatabase::GetObjectInfo(wxString className)
{
    PObjectInfo info;
    ObjectInfoMap::iterator it = m_objs.find(className);

    if (it != m_objs.end())
        info = it->second;

    return info;
}

PObjectPackage ObjectDatabase::GetPackage(size_t idx)
{
    assert(idx < m_pkgs.size());
    return m_pkgs[idx];
}

// TODO: The inheritance of properties must be recursive.
PObjectBase ObjectDatabase::NewObject(PObjectInfo objInfo)
{
    PObjectBase object;

    // Llagados aquí el objeto se crea seguro...
    object = PObjectBase(new ObjectBase(objInfo->GetClassName()));
    object->SetObjectTypeName(objInfo->GetObjectTypeName()); // FIXME
    object->SetObjectInfo(objInfo);

    PPropertyInfo propertyInfo;
    PEventInfo eventInfo;
    PObjectInfo classInfo = objInfo;
    size_t base = 0;
    while (classInfo) {
        for (size_t i = 0; i < classInfo->GetPropertyCount(); i++) {
            propertyInfo = classInfo->GetPropertyInfo(i);
            PProperty property(new Property(propertyInfo, object));

            // Set the default value, either from the property info,
            // or an override from this class
            wxString defaultValue = propertyInfo->GetDefaultValue();
            if (base > 0) {
                wxString defaultValueTemp
                    = objInfo->GetBaseClassDefaultPropertyValue(
                        base - 1, propertyInfo->GetName());
                if (!defaultValueTemp.empty())
                    defaultValue = defaultValueTemp;
            }
            property->SetValue(defaultValue);
            /*
                The properties are implemented with a "map" structure,
                this implies that there will be no duplicate properties.
                Otherwise, it will be necessary to ensure that said property
                does not exist.
                Another important thing is that the order in which they are inserted
                properties, bottom-up, this allows you to redefine some property.
            */
            object->AddProperty(property);
        }
        for (size_t i = 0; i < classInfo->GetEventCount(); i++) {
            eventInfo = classInfo->GetEventInfo(i);
            PEvent event(new Event(eventInfo, object));
            // notice that for event there isn't a default value on its creation
            // because there is not handler at the moment
            object->AddEvent(event);
        }
        classInfo = (base < objInfo->GetBaseClassCount()
                         ? objInfo->GetBaseClass(base++)
                         : PObjectInfo());
    }
    /*
        If the object has the name property (reserved for the name of the
        object) we add the counter to avoid repeating names.
    */
    objInfo->IncrementInstanceCount();

    size_t ins = objInfo->GetInstanceCount();
    PProperty pname = object->GetProperty(NAME_TAG);
    if (pname)
        pname->SetValue(pname->GetValue() + StringUtils::IntToStr(ins));

    return object;
}

size_t ObjectDatabase::CountChildrenWithSameType(PObjectBase parent,
                                                 PObjectType type)
{
    size_t count = 0, childCount = parent->GetChildCount();
    for (size_t i = 0; i < childCount; i++) {
        if (type == parent->GetChild(i)->GetObjectInfo()->GetObjectType())
            count++;
    }
    return count;
}

size_t ObjectDatabase::CountChildrenWithSameType(PObjectBase parent,
                                                 const std::set<PObjectType>& types)
{
    size_t count = 0, childCount = parent->GetChildCount();
    for (size_t i = 0; i < childCount; ++i) {
        if (types.find(parent->GetChild(i)->GetObjectInfo()->GetObjectType()) != types.end())
            ++count;
    }
    return count;
}

/*
    Creates an instance of classname below parent.
    The function performs type checking to create the object:

    - Checks if the type is a valid child-type for the "parent", in which case
      it will also be verified that the number of children of the same type
      does not exceed the defined maximum, otherwise the object will not
      be created.

    - If the child-type is not one of those defined for the type of "parent"
      will try to create it as a child of one of the child types
      with the item flag set to "1".
      To do this, it goes through all the types with flag item.
      If it can't create the object, either because the type is not valid
      or because it exceeds the maximum allowed, it tries the next one until
      there are no more left.

    Note: The method might want to create the object without linking it
    with the tree, to facilitate undo-redo.
*/
PObjectBase ObjectDatabase::CreateObject(std::string classname, PObjectBase parent)
{
    PObjectBase object;
    PObjectInfo objInfo = GetObjectInfo(_WXSTR(classname));
    if (!objInfo) {
        wxString message = wxString::Format(
            "Unknown Object Type: %s\n"
            "The most likely causes are that this copy of wxWeaver is out of date,\n"
            "or that there is a plugin missing.\n"
            "Please check at http://wxweaver.github.io\n",
            classname);
        wxWEAVER_THROW_EX(message)
    }
    PObjectType objType = objInfo->GetObjectType();
    if (parent) {
        // Type check
        PObjectType parentType = parent->GetObjectInfo()->GetObjectType();

        //AUI
        bool aui = false;
        if (parentType->GetName() == "form")
            aui = parent->GetPropertyAsInteger("aui_managed");

        int max = parentType->FindChildType(objType, aui);
        /*
            FIXME! Esto es un parche para evitar crear los tipos menubar,statusbar y
            toolbar en un form que no sea wxFrame.
            Hay que modificar el conjunto de tipos para permitir tener varios tipos
            de forms (como childType de project), pero hay mucho código no válido
            para forms que no sean de tipo "form". Dicho de otra manera, hay
            código que dependen del nombre del tipo, cosa que hay que evitar.
        */
        if (parentType->GetName() == "form"
            && parent->GetClassName() != "Frame"
            && (objType->GetName() == "statusbar"
                || objType->GetName() == "menubar"
                || objType->GetName() == "ribbonbar"
                || objType->GetName() == "toolbar"))
            return PObjectBase(); // Not a valid type
#if 1
        // TODO: Re implement wxITEM_DROPDOWN
        // No menu dropdown for wxToolBar until wx 2.9 :(
        if (parentType->GetName() == "tool") {
            PObjectBase gParent = parent->GetParent();
            if ((gParent->GetClassName() == "wxToolBar")
                && (objType->GetName() == "menu"))
                return PObjectBase(); // Not a valid type
        }
#endif
        if (max) // Valid type
        {
            bool create = true;

            // we check the number of instances
            int count;
            if (objType == GetObjectType("sizer")
                || objType == GetObjectType("gbsizer")) {
                count = CountChildrenWithSameType(
                    parent,
                    { GetObjectType("sizer"), GetObjectType("gbsizer") });
            } else {
                count = CountChildrenWithSameType(parent, objType);
            }
            if (max > 0 && count >= max)
                create = false;

            if (create)
                object = NewObject(objInfo);
        } else // max == 0
        {
            // el tipo no es válido, vamos a comprobar si podemos insertarlo
            // como hijo de un "item"
            bool created = false;
            for (size_t i = 0; !created && i < parentType->GetChildTypeCount(); i++) {
                PObjectType childType = parentType->GetChildType(i);
                size_t childMax = childType->FindChildType(objType, aui);

                if (childType->IsItem() && childMax != 0) {
                    childMax = parentType->FindChildType(childType, aui);

                    // si el tipo es un item y además el tipo del objeto a crear
                    // puede ser hijo del tipo del item vamos a intentar crear la
                    // instancia del item para crear el objeto como hijo de este
                    if (childMax < 0 || CountChildrenWithSameType(parent, childType) < childMax) {
                        // No hay problemas para crear el item debajo de parent
                        PObjectBase item = NewObject(GetObjectInfo(childType->GetName()));

                        //PObjectBase obj = CreateObject(classname,item);
                        PObjectBase obj = NewObject(objInfo);

                        // la siguiente condición debe cumplirse siempre
                        // ya que un item debe siempre contener a otro objeto
                        if (obj) {
                            // enlazamos item y obj
                            item->AddChild(obj);
                            obj->SetParent(item);

                            // sizeritem es un tipo de objeto reservado, para que el uso sea
                            // más práctico se asignan unos valores por defecto en función
                            // del tipo de objeto creado
                            if (item->GetObjectInfo()->IsSubclassOf("sizeritembase"))
                                SetDefaultLayoutProperties(item);

                            object = item;
                            created = true;
                        } else
                            wxLogError("Review your definitions file (objtypes.xml)");
                    }
                }
            }
        }
        // Nota: provisionalmente vamos a enlazar el objeto al padre pero
        //       esto debería hacerse fuera para poder implementar el Undo-Redo
#if 0
        if (object) {
            parent->AddChild(object);
            object->SetParent(parent);
        }
#endif
    } else // if (!parent);
    {
        object = NewObject(objInfo);
    }
    return object;
}

PObjectBase ObjectDatabase::CopyObject(PObjectBase obj)
{
    assert(obj);

    PObjectInfo objInfo = obj->GetObjectInfo();

    PObjectBase copyObj = NewObject(objInfo); // Make a copy
    assert(copyObj);

    // copiamos las propiedades
    size_t count = obj->GetPropertyCount();
    for (size_t i = 0; i < count; i++) {
        PProperty objProp = obj->GetProperty(i);
        assert(objProp);

        PProperty copyProp = copyObj->GetProperty(objProp->GetName());
        assert(copyProp);

        wxString propValue = objProp->GetValue();
        copyProp->SetValue(propValue);
    }
    // ...and the event handlers
    count = obj->GetEventCount();
    for (size_t i = 0; i < count; i++) {
        PEvent event = obj->GetEvent(i);
        PEvent copyEvent = copyObj->GetEvent(event->GetName());
        copyEvent->SetValue(event->GetValue());
    }
    // creamos recursivamente los hijos
    count = obj->GetChildCount();
    for (size_t i = 0; i < count; i++) {
        PObjectBase childCopy = CopyObject(obj->GetChild(i));
        copyObj->AddChild(childCopy);
        childCopy->SetParent(copyObj);
    }
    return copyObj;
}

void ObjectDatabase::SetDefaultLayoutProperties(PObjectBase sizeritem)
{
    if (!sizeritem->GetObjectInfo()->IsSubclassOf("sizeritembase")) {
        LogDebug("SetDefaultLayoutProperties expects a subclass of sizeritembase");
        return;
    }
    PObjectBase child = sizeritem->GetChild(0);
    PObjectInfo childInfo = child->GetObjectInfo();
    wxString objType = child->GetObjectTypeName();
    PProperty proportion = sizeritem->GetProperty("proportion");

    if (childInfo->IsSubclassOf("sizer")
        || childInfo->IsSubclassOf("gbsizer")
        || objType == "splitter"
        || childInfo->GetClassName() == "spacer") {
        if (proportion)
            proportion->SetValue(wxS("1"));

        sizeritem->GetProperty("flag")->SetValue(wxS("wxEXPAND"));
    } else if (childInfo->GetClassName() == "wxStaticLine") {
        sizeritem->GetProperty("flag")->SetValue(wxS("wxEXPAND | wxALL"));
    } else if (childInfo->GetClassName() == "wxToolBar") {
        sizeritem->GetProperty("flag")->SetValue(wxS("wxEXPAND"));
    } else if (objType == "widget" || objType == "statusbar") {
        if (proportion)
            proportion->SetValue(wxS("0"));

        sizeritem->GetProperty("flag")->SetValue(wxS("wxALL"));
    } else if (objType == "notebook"
               || objType == "listbook"
               || objType == "simplebook"
               || objType == "choicebook"
               || objType == "auinotebook"
               || objType == "treelistctrl"
               || objType == "expanded_widget"
               || objType == "container") {
        if (proportion)
            proportion->SetValue(wxS("1"));

        sizeritem->GetProperty("flag")->SetValue(wxS("wxEXPAND | wxALL"));
    }
}

void ObjectDatabase::ResetObjectCounters()
{
    ObjectInfoMap::iterator it;
    for (it = m_objs.begin(); it != m_objs.end(); it++)
        it->second->ResetInstanceCount();
}

PObjectBase ObjectDatabase::CreateObject(ticpp::Element* xmlObj, PObjectBase parent)
{
    try {
        std::string className;
        xmlObj->GetAttribute(CLASS_TAG, &className, false);
        PObjectBase newobject = CreateObject(className, parent);

        // It is possible the CreateObject returns an "item" containing the object, e.g. SizerItem or SplitterItem
        // If that is the case, reassign "object" to the actual object
        PObjectBase object = newobject;
        if (object && object->GetChildCount())
            object = object->GetChild(0);

        if (object) {
            // Get the state of expansion in the object tree
            bool expanded;
            xmlObj->GetAttributeOrDefault(EXPANDED_TAG, &expanded, true);
            object->SetExpanded(expanded);

            // Load the properties
            ticpp::Element* xmlProp = xmlObj->FirstChildElement(PROPERTY_TAG, false);
            while (xmlProp) {
                std::string propName;
                xmlProp->GetAttribute(NAME_TAG, &propName, false);
                PProperty prop = object->GetProperty(_WXSTR(propName));
                if (prop) // does the property exist
                {
                    // load the value
                    prop->SetValue(_WXSTR(xmlProp->GetText(false)));
                } else {
                    std::string value = xmlProp->GetText(false);
                    if (!value.empty()) {
                        wxLogError(
                            "The property named \"%s\" of class \"%s\" is not supported by this version of wxWeaver.\n"
                            "If your project file was just converted from an older version, then the conversion was not complete.\n"
                            "Otherwise, this project is from a newer version of wxWeaver.\n\n"
                            "The property's value is: %s\n"
                            "If you save this project, YOU WILL LOSE DATA",
                            propName.c_str(), className.c_str(), value.c_str());
                    }
                }
                xmlProp = xmlProp->NextSiblingElement(PROPERTY_TAG, false);
            }
            // load the event handlers
            ticpp::Element* xmlEvent = xmlObj->FirstChildElement(EVENT_TAG, false);
            while (xmlEvent) {
                std::string eventName;
                xmlEvent->GetAttribute(NAME_TAG, &eventName, false);
                PEvent event = object->GetEvent(_WXSTR(eventName));
                if (event)
                    event->SetValue(_WXSTR(xmlEvent->GetText(false)));

                xmlEvent = xmlEvent->NextSiblingElement(EVENT_TAG, false);
            }
            if (parent) {
                // set up parent/child relationship
                parent->AddChild(newobject);
                newobject->SetParent(parent);
            }
            // create the children
            ticpp::Element* child = xmlObj->FirstChildElement(OBJECT_TAG, false);
            while (child) {
                CreateObject(child, object);
                child = child->NextSiblingElement(OBJECT_TAG, false);
            }
        }
        return newobject;
    } catch (ticpp::Exception&) {
        return PObjectBase();
    }
}

bool IncludeInPalette(wxString /*type*/)
{
    return true;
}

void ObjectDatabase::LoadPlugins(PwxWeaverManager manager)
{
    // Load some default templates
    LoadCodeGen(m_xmlPath + "properties.cppcode");
    LoadCodeGen(m_xmlPath + "properties.pythoncode");
    LoadCodeGen(m_xmlPath + "properties.luacode");
    LoadCodeGen(m_xmlPath + "properties.phpcode");
    LoadPackage(m_xmlPath + "default.xml", m_iconPath);
    LoadCodeGen(m_xmlPath + "default.cppcode");
    LoadCodeGen(m_xmlPath + "default.pythoncode");
    LoadCodeGen(m_xmlPath + "default.luacode");
    LoadCodeGen(m_xmlPath + "default.phpcode");

    // Map to temporarily hold plugins.
    // Used to both set page order and to prevent two plugins with the same name.
    typedef std::map<wxString, PObjectPackage> PackageMap;
    PackageMap packages;

    // Open plugins directory for iteration
    if (!wxDir::Exists(m_pluginPath))
        return;

    wxDir pluginsDir(m_pluginPath);
    if (!pluginsDir.IsOpened())
        return;

    // Iterate through plugin directories and load the package from the xml subdirectory
    wxString pluginDirName;
    bool moreDirectories
        = pluginsDir.GetFirst(&pluginDirName, wxEmptyString, wxDIR_DIRS | wxDIR_HIDDEN);
    while (moreDirectories) {
        // Iterate through .xml files in the xml directory
        wxString nextPluginPath = m_pluginPath + pluginDirName;
        wxString nextPluginXmlPath = nextPluginPath + wxFILE_SEP_PATH + "xml";
        wxString nextPluginIconPath = nextPluginPath + wxFILE_SEP_PATH + "icons";
        if (wxDir::Exists(nextPluginPath)) {
            if (wxDir::Exists(nextPluginXmlPath)) {
                wxDir pluginXmlDir(nextPluginXmlPath);
                if (pluginXmlDir.IsOpened()) {
                    std::map<wxString, PObjectPackage> packagesToSetup;
                    wxString packageXmlFile;
                    bool moreXmlFiles
                        = pluginXmlDir.GetFirst(&packageXmlFile, "*.xml",
                                                wxDIR_FILES | wxDIR_HIDDEN);
                    while (moreXmlFiles) {
                        try {
                            wxFileName nextXmlFile(nextPluginXmlPath
                                                   + wxFILE_SEP_PATH + packageXmlFile);
                            if (!nextXmlFile.IsAbsolute())
                                nextXmlFile.MakeAbsolute();

                            PObjectPackage package
                                = LoadPackage(nextXmlFile.GetFullPath(), nextPluginIconPath);
                            if (package) {
                                // Load all packages, then setup all packages
                                // this allows multiple packages sharing one library
                                packagesToSetup[nextXmlFile.GetFullPath()] = package;
                            }
                        } catch (wxWeaverException& ex) {
                            wxLogError(ex.what());
                        }
                        moreXmlFiles = pluginXmlDir.GetNext(&packageXmlFile);
                    }
                    std::map<wxString, PObjectPackage>::iterator packageIt;
                    for (packageIt = packagesToSetup.begin();
                         packageIt != packagesToSetup.end(); ++packageIt) {
                        // Setup the inheritance for base classes
                        wxFileName fullNextPluginPath(nextPluginPath);
                        if (!fullNextPluginPath.IsAbsolute()) {
                            fullNextPluginPath.MakeAbsolute();
                        }
                        wxFileName xmlFileName(packageIt->first);
                        try {
                            SetupPackage(xmlFileName.GetFullPath(),
                                         fullNextPluginPath.GetFullPath(), manager);

                            // Load the C++ code tempates
                            xmlFileName.SetExt("cppcode");
                            LoadCodeGen(xmlFileName.GetFullPath());

                            // Load the Python code tempates
                            xmlFileName.SetExt("pythoncode");
                            LoadCodeGen(xmlFileName.GetFullPath());

                            // Load the PHP code tempates
                            xmlFileName.SetExt("phpcode");
                            LoadCodeGen(xmlFileName.GetFullPath());

                            // Load the Lua code tempates
                            xmlFileName.SetExt("luacode");
                            LoadCodeGen(xmlFileName.GetFullPath());

                            std::pair<PackageMap::iterator, bool> addedPackage
                                = packages.insert(PackageMap::value_type(
                                    packageIt->second->GetPackageName(),
                                    packageIt->second));
                            if (!addedPackage.second) {
                                addedPackage.first->second->AppendPackage(packageIt->second);
                                LogDebug("Merged plugins named \""
                                         + packageIt->second->GetPackageName() + "\"");
                            }
                        } catch (wxWeaverException& ex) {
                            wxLogError(ex.what());
                        }
                    }
                }
            }
        }
        moreDirectories = pluginsDir.GetNext(&pluginDirName);
    }
    // Add packages to final data structure
    m_pkgs.reserve(packages.size());
    for (auto& package : packages)
        m_pkgs.push_back(package.second);
}

void ObjectDatabase::SetupPackage(const wxString& file,
#ifdef __WXMSW__
                                  const wxString& path,
#else
                                  const wxString& /*path*/,
#endif
                                  PwxWeaverManager manager)
{
#ifdef __WXMSW__
    wxString libPath = path;
#else
    wxStandardPathsBase& stdpaths = wxStandardPaths::Get();
    wxString libPath = stdpaths.GetPluginsDir();
    libPath.Replace(wxTheApp->GetAppName().c_str(), "wxweaver");
#endif
    // Renamed libraries for convenience in debug using a "-xx" wx version as suffix.
    // This will also prevent loading debug libraries in release and vice versa,
    // that used to cause crashes when trying to debug.
    wxString wxver = "";

#ifdef DEBUG
#ifdef APPEND_WXVERSION
    wxver = wxver + wxString::Format("-%i%i"), wxMAJOR_VERSION, wxMINOR_VERSION);
#endif
#endif
    try {
        ticpp::Document doc;
        XMLUtils::LoadXMLFile(doc, true, file);
        ticpp::Element* root = doc.FirstChildElement(PACKAGE_TAG);

        // get the library to import
        std::string lib;
        root->GetAttributeOrDefault("lib", &lib, "");
        if (!lib.empty()) {
            // Allows plugin dependency dlls to be next to plugin dll in windows
            wxString workingDir = ::wxGetCwd();
            wxFileName::SetCwd(libPath);
            try {
                wxString fullLibPath
                    = libPath + wxFILE_SEP_PATH + _WXSTR(lib) + wxver;
                if (m_importedLibraries.insert(fullLibPath).second) {
                    ImportComponentLibrary(fullLibPath, manager);
                }
            } catch (...) {
                // Put Cwd back
                wxFileName::SetCwd(workingDir);
                throw;
            }
            // Put Cwd back
            wxFileName::SetCwd(workingDir);
        }
        ticpp::Element* elemObj = root->FirstChildElement(OBJINFO_TAG, false);
        while (elemObj) {
            std::string wxverObj;
            elemObj->GetAttributeOrDefault(WXVERSION_TAG, &wxverObj, "");
            if (!wxverObj.empty()) {
                long wxversion = 0;
                // skip widgets supported by higher wxWidgets version than used for the build
                if ((!_WXSTR(wxverObj).ToLong(&wxversion))
                    || (wxversion > wxVERSION_NUMBER)) {
                    elemObj = elemObj->NextSiblingElement(OBJINFO_TAG, false);
                    continue;
                }
            }
            std::string className;
            elemObj->GetAttribute(CLASS_TAG, &className);
            PObjectInfo classInfo = GetObjectInfo(_WXSTR(className));

            ticpp::Element* elemBase = elemObj->FirstChildElement("inherits", false);
            while (elemBase) {
                std::string baseName;
                elemBase->GetAttribute(CLASS_TAG, &baseName);

                // Add a reference to its base class
                PObjectInfo baseInfo = GetObjectInfo(_WXSTR(baseName));
                if (classInfo && baseInfo) {
                    size_t baseIndex = classInfo->AddBaseClass(baseInfo);

                    std::string propName, value;
                    ticpp::Element* inheritedProperty
                        = elemBase->FirstChildElement("property", false);
                    while (inheritedProperty) {
                        inheritedProperty->GetAttribute(NAME_TAG, &propName);
                        value = inheritedProperty->GetText();
                        classInfo->AddBaseClassDefaultPropertyValue(
                            baseIndex, _WXSTR(propName), _WXSTR(value));
                        inheritedProperty
                            = inheritedProperty->NextSiblingElement("property", false);
                    }
                }
                elemBase = elemBase->NextSiblingElement("inherits", false);
            }
            // Add the "C++" base class, predefined for the components and widgets
            wxString typeName = classInfo->GetObjectTypeName();
            if (HasCppProperties(typeName)) {
                PObjectInfo cpp_interface = GetObjectInfo("C++");
                if (cpp_interface) {
                    size_t baseIndex = classInfo->AddBaseClass(cpp_interface);
                    if (typeName == "sizer"
                        || typeName == "gbsizer"
                        || typeName == "menuitem") {
                        classInfo->AddBaseClassDefaultPropertyValue(
                            baseIndex, "permission", "none");
                    }
                }
            }
            elemObj = elemObj->NextSiblingElement(OBJINFO_TAG, false);
        }
    } catch (ticpp::Exception& ex) {
        wxWEAVER_THROW_EX(_WXSTR(ex.m_details));
    }
}

// TODO: Replace this horror with a vector or something
bool ObjectDatabase::HasCppProperties(wxString type)
{
    return (type == "notebook"
            || type == "listbook"
            || type == "simplebook"
            || type == "choicebook"
            || type == "auinotebook"
            || type == "widget"
            || type == "expanded_widget"
            || type == "propgrid"
            || type == "propgridman"
            || type == "statusbar"
            || type == "component"
            || type == "container"
            || type == "menubar"
            || type == "menu"
            || type == "menuitem"
            || type == "submenu"
            || type == "toolbar"
            || type == "ribbonbar"
            || type == "ribbonpage"
            || type == "ribbonpanel"
            || type == "ribbonbuttonbar"
            || type == "ribbonbutton"
            || type == "ribbondropdownbutton"
            || type == "ribbonhybridbutton"
            || type == "ribbontogglebutton"
            || type == "ribbontoolbar"
            || type == "ribbontool"
            || type == "ribbondropdowntool"
            || type == "ribbonhybridtool"
            || type == "ribbontoggletool"
            || type == "ribbongallery"
            || type == "ribbongalleryitem"
            || type == "dataviewctrl"
            || type == "dataviewtreectrl"
            || type == "dataviewlistctrl"
            || type == "dataviewlistcolumn"
            || type == "dataviewcolumn"
            || type == "tool"
            || type == "splitter"
            || type == "treelistctrl"
            || type == "sizer"
            || type == "nonvisual"
            || type == "gbsizer"
            || type == "propgriditem"
            || type == "propgridpage"
            || type == "gbsizer"
            || type == "wizardpagesimple");
}

void ObjectDatabase::LoadCodeGen(const wxString& file)
{
    try {
        ticpp::Document doc;
        XMLUtils::LoadXMLFile(doc, true, file);

        // read the codegen element
        ticpp::Element* elem_codegen = doc.FirstChildElement("codegen");
        std::string language;
        elem_codegen->GetAttribute("language", &language);
        wxString lang = _WXSTR(language);

        // read the templates
        ticpp::Element* elemTemplates
            = elem_codegen->FirstChildElement("templates", false);
        while (elemTemplates) {

            std::string propName;
            elemTemplates->GetAttribute("property", &propName, false);
            bool hasProp = !propName.empty();

            std::string className;
            elemTemplates->GetAttribute("class", &className, !hasProp);

            PCodeInfo codeInfo(new CodeInfo());
            ticpp::Element* elemTemplate
                = elemTemplates->FirstChildElement("template", false);
            while (elemTemplate) {
                std::string templateName;
                elemTemplate->GetAttribute("name", &templateName);

                std::string templateCode = elemTemplate->GetText(false);
                codeInfo->AddTemplate(_WXSTR(templateName), _WXSTR(templateCode));

                elemTemplate = elemTemplate->NextSiblingElement("template", false);
            }
            if (hasProp) {
                // store code info for properties
                if (!m_propertyTypeTemplates[ParsePropertyType(_WXSTR(propName))]
                         .insert(
                             LangTemplateMap::value_type(lang, codeInfo))
                         .second) {
                    wxLogError("Found second template definition for property \"%s\" for language \"%s\"",
                               propName.c_str(), lang.c_str());
                }
            } else {
                // store code info for objects
                PObjectInfo objInfo = GetObjectInfo(_WXSTR(className));
                if (objInfo)
                    objInfo->AddCodeInfo(lang, codeInfo);
            }
            elemTemplates = elemTemplates->NextSiblingElement("templates", false);
        }
    } catch (ticpp::Exception& ex) {
        wxLogError(_WXSTR(ex.m_details));
    } catch (wxWeaverException& ex) {
        wxLogError(ex.what());
    }
}

PObjectPackage ObjectDatabase::LoadPackage(const wxString& file,
                                           const wxString& iconPath)
{
    PObjectPackage package;
    try {
        ticpp::Document doc;
        XMLUtils::LoadXMLFile(doc, true, file);
        ticpp::Element* root = doc.FirstChildElement(PACKAGE_TAG);

        // Name Attribute
        std::string pkgName;
        root->GetAttribute(NAME_TAG, &pkgName);

        // Description Attribute
        std::string pkgDesc;
        root->GetAttributeOrDefault(PKGDESC_TAG, &pkgDesc, "");

        // Icon Path Attribute
        std::string pkgIconName;
        root->GetAttributeOrDefault(ICON_TAG, &pkgIconName, "");
        wxString pkgIconPath = iconPath + wxFILE_SEP_PATH + _WXSTR(pkgIconName);

        wxBitmap pkgIcon;
        if (!pkgIconName.empty() && wxFileName::FileExists(pkgIconPath)) {
            wxImage image(pkgIconPath, wxBITMAP_TYPE_ANY);
            pkgIcon = wxBitmap(image.Scale(16, 16));
        } else {
            pkgIcon = AppBitmaps::GetBitmap("unknown", 16);
        }

        package = PObjectPackage(new ObjectPackage(
            _WXSTR(pkgName), _WXSTR(pkgDesc), pkgIcon));

        ticpp::Element* elemObj = root->FirstChildElement(OBJINFO_TAG, false);

        while (elemObj) {
            std::string className;
            elemObj->GetAttribute(CLASS_TAG, &className);

            std::string type;
            elemObj->GetAttribute("type", &type);

            std::string icon;
            elemObj->GetAttributeOrDefault("icon", &icon, "");
            wxString iconFullPath = iconPath + wxFILE_SEP_PATH + _WXSTR(icon);

            std::string smallIcon;
            elemObj->GetAttributeOrDefault("smallIcon", &smallIcon, "");
            wxString smallIconFullPath
                = iconPath + wxFILE_SEP_PATH + _WXSTR(smallIcon);

            std::string wxver;
            elemObj->GetAttributeOrDefault(WXVERSION_TAG, &wxver, "");
            if (wxver != "") {
                long wxversion = 0;
                // skip widgets supported by higher wxWidgets version than used for the build
                if ((!_WXSTR(wxver).ToLong(&wxversion))
                    || (wxversion > wxVERSION_NUMBER)) {
                    elemObj = elemObj->NextSiblingElement(OBJINFO_TAG, false);
                    continue;
                }
            }
            bool startGroup;
            elemObj->GetAttributeOrDefault("startgroup", &startGroup, false);

            PObjectInfo objInfo(new ObjectInfo(
                _WXSTR(className),
                GetObjectType(_WXSTR(type)), package, startGroup));

            if (!icon.empty() && wxFileName::FileExists(iconFullPath)) {
                wxImage img(iconFullPath, wxBITMAP_TYPE_ANY);
                objInfo->SetIconFile(wxBitmap(img.Scale(ICON_SIZE, ICON_SIZE)));
            } else {
                objInfo->SetIconFile(AppBitmaps::GetBitmap("unknown", ICON_SIZE));
            }
            if (!smallIcon.empty() && wxFileName::FileExists(smallIconFullPath)) {
                wxImage img(smallIconFullPath, wxBITMAP_TYPE_ANY);
                objInfo->SetSmallIconFile(
                    wxBitmap(img.Scale(SMALL_ICON_SIZE, SMALL_ICON_SIZE)));
            } else {
                wxImage img = objInfo->GetIconFile().ConvertToImage();
                objInfo->SetSmallIconFile(
                    wxBitmap(img.Scale(SMALL_ICON_SIZE, SMALL_ICON_SIZE)));
            }
            // Parse the Properties
            std::set<PropertyType> types;
            ParseProperties(elemObj, objInfo, objInfo->GetCategory(), &types);
            ParseEvents(elemObj, objInfo, objInfo->GetCategory());

            // Add the ObjectInfo to the map
            m_objs.insert(ObjectInfoMap::value_type(_WXSTR(className), objInfo));

            // Add the object to the palette
            if (ShowInPalette(objInfo->GetObjectTypeName()))
                package->Add(objInfo);

            elemObj = elemObj->NextSiblingElement(OBJINFO_TAG, false);
        }
    } catch (ticpp::Exception& ex) {
        wxWEAVER_THROW_EX(_WXSTR(ex.m_details));
    }
    return package;
}

void ObjectDatabase::ParseProperties(ticpp::Element* elemObj, PObjectInfo objInfo,
                                     PPropertyCategory category,
                                     std::set<PropertyType>* types)
{
    ticpp::Element* elemCategory = elemObj->FirstChildElement(CATEGORY_TAG, false);
    while (elemCategory) {
        // Category name attribute
        std::string cname;
        elemCategory->GetAttribute(NAME_TAG, &cname);
        PPropertyCategory newCat(new PropertyCategory(_WXSTR(cname)));

        // Add category
        category->AddCategory(newCat);

        // Recurse
        ParseProperties(elemCategory, objInfo, newCat, types);
        elemCategory = elemCategory->NextSiblingElement(CATEGORY_TAG, false);
    }
    ticpp::Element* elem_prop = elemObj->FirstChildElement(PROPERTY_TAG, false);
    while (elem_prop) {
        // Property Name Attribute
        std::string pname;
        elem_prop->GetAttribute(NAME_TAG, &pname);
        category->AddProperty(_WXSTR(pname));

        std::string description;
        elem_prop->GetAttributeOrDefault(DESCRIPTION_TAG, &description, "");

        std::string customEditor;
        elem_prop->GetAttributeOrDefault(CUSTOM_EDITOR_TAG, &customEditor, "");

        std::string propType;
        elem_prop->GetAttribute("type", &propType);
        PropertyType ptype;
        try {
            ptype = ParsePropertyType(_WXSTR(propType));
        } catch (wxWeaverException& ex) {
            wxLogError(
                "Error: %s\nWhile parsing property \"%s\" of class \"%s\"",
                ex.what(), pname.c_str(), objInfo->GetClassName().c_str());
            elem_prop = elem_prop->NextSiblingElement(PROPERTY_TAG, false);
            continue;
        }
        // Get default value
        std::string defValue;
        try {
            ticpp::Node* lastChild = elem_prop->LastChild(false);
            if (lastChild && lastChild->Type() == TiXmlNode::TEXT) {
                ticpp::Text* text = lastChild->ToText();
                wxASSERT(text);
                defValue = text->Value();
            }
        } catch (ticpp::Exception& ex) {
            wxLogDebug(ex.what());
        }
        // if the property is a "bitlist" then parse all of the options
        POptionList optList;
        std::list<PropertyChild> children;
        if (ptype == PT_BITLIST || ptype == PT_OPTION || ptype == PT_EDIT_OPTION) {
            optList = POptionList(new OptionList());
            ticpp::Element* elemOpt = elem_prop->FirstChildElement("option", false);
            while (elemOpt) {
                std::string macroName;
                elemOpt->GetAttribute(NAME_TAG, &macroName);

                std::string macroDescription;
                elemOpt->GetAttributeOrDefault(DESCRIPTION_TAG, &macroDescription, "");
                optList->AddOption(_WXSTR(macroName), _WXSTR(macroDescription));
                m_macroSet.insert(_WXSTR(macroName));
                elemOpt = elemOpt->NextSiblingElement("option", false);
            }
        } else if (ptype == PT_PARENT) {
            // If the property is a parent, then get the children
            defValue.clear();
            ticpp::Element* elemChild = elem_prop->FirstChildElement("child", false);
            while (elemChild) {
                PropertyChild child;

                std::string child_name;
                elemChild->GetAttribute(NAME_TAG, &child_name);
                child.m_name = _WXSTR(child_name);

                std::string childDescription;
                elemChild->GetAttributeOrDefault(DESCRIPTION_TAG, &childDescription, "");
                child.m_description = _WXSTR(childDescription);

                std::string childType;
                elemChild->GetAttributeOrDefault("type", &childType, "wxString");
                child.m_type = ParsePropertyType(_WXSTR(childType));

                // Get default value
                // Empty tags don't contain any child so this will throw in that case
                std::string childValue;
                try {
                    ticpp::Node* lastChild = elemChild->LastChild(false);
                    if (lastChild && lastChild->Type() == TiXmlNode::TEXT) {
                        ticpp::Text* text = lastChild->ToText();
                        wxASSERT(text);
                        childValue = text->Value();
                    }
                } catch (ticpp::Exception& ex) {
                    wxLogDebug(ex.what());
                }
                child.m_defaultValue = _WXSTR(childValue);

                // build parent default value
                if (children.size())
                    defValue += "; ";

                defValue += childValue;
                children.push_back(child);
                elemChild = elemChild->NextSiblingElement("child", false);
            }
        }
        // create an instance of PropertyInfo
        PPropertyInfo propertyInfo(new PropertyInfo(
            _WXSTR(pname), ptype, _WXSTR(defValue), _WXSTR(description),
            _WXSTR(customEditor), optList, children));

        // add the PropertyInfo to the property
        objInfo->AddPropertyInfo(propertyInfo);

        // merge property code templates, once per property type
        if (types->insert(ptype).second) {
            LangTemplateMap& propLangTemplates = m_propertyTypeTemplates[ptype];
            LangTemplateMap::iterator lang;
            for (lang = propLangTemplates.begin();
                 lang != propLangTemplates.end(); ++lang) {
                if (lang->second)
                    objInfo->AddCodeInfo(lang->first, lang->second);
            }
        }
        elem_prop = elem_prop->NextSiblingElement(PROPERTY_TAG, false);
    }
}

void ObjectDatabase::ParseEvents(ticpp::Element* elemObj, PObjectInfo objInfo,
                                 PPropertyCategory category)
{
    ticpp::Element* elemCategory = elemObj->FirstChildElement(CATEGORY_TAG, false);
    while (elemCategory) {
        // Category name attribute
        std::string cname;
        elemCategory->GetAttribute(NAME_TAG, &cname);
        PPropertyCategory newCat(new PropertyCategory(_WXSTR(cname)));

        category->AddCategory(newCat); // Add category

        ParseEvents(elemCategory, objInfo, newCat); // Recurse
        elemCategory = elemCategory->NextSiblingElement(CATEGORY_TAG, false);
    }

    ticpp::Element* elemEvt = elemObj->FirstChildElement(EVENT_TAG, false);
    while (elemEvt) {
        // Event Name Attribute
        std::string evtName;
        elemEvt->GetAttribute(NAME_TAG, &evtName);
        category->AddEvent(_WXSTR(evtName));

        // Event class
        std::string evtClass;
        elemEvt->GetAttributeOrDefault(EVENT_CLASS_TAG, &evtClass, "wxEvent");

        // Help string
        std::string description;
        elemEvt->GetAttributeOrDefault(DESCRIPTION_TAG, &description, "");

        // Get default value
        std::string defValue;
        try {
            ticpp::Node* lastChild = elemEvt->LastChild(false);
            if (lastChild && lastChild->Type() == TiXmlNode::TEXT) {
                ticpp::Text* text = lastChild->ToText();
                wxASSERT(text);
                defValue = text->Value();
            }
        } catch (ticpp::Exception& ex) {
            wxLogDebug(ex.what());
        }
        // create an instance of EventInfo
        PEventInfo evt_info(
            new EventInfo(_WXSTR(evtName), _WXSTR(evtClass),
                          _WXSTR(defValue), _WXSTR(description)));

        // add the EventInfo to the event
        objInfo->AddEventInfo(evt_info);

        elemEvt = elemEvt->NextSiblingElement(EVENT_TAG, false);
    }
}

// TODO: Replace this horror with a vector or something
bool ObjectDatabase::ShowInPalette(wxString type)
{
    return (type == "form"
            || type == "wizard"
            || type == "wizardpagesimple"
            || type == "menubar_form"
            || type == "toolbar_form"
            || type == "sizer"
            || type == "gbsizer"
            || type == "menu"
            || type == "menuitem"
            || type == "submenu"
            || type == "tool"
            || type == "ribbonbar"
            || type == "ribbonpage"
            || type == "ribbonpanel"
            || type == "ribbonbuttonbar"
            || type == "ribbonbutton"
            || type == "ribbondropdownbutton"
            || type == "ribbonhybridbutton"
            || type == "ribbontogglebutton"
            || type == "ribbontoolbar"
            || type == "ribbontool"
            || type == "ribbondropdowntool"
            || type == "ribbonhybridtool"
            || type == "ribbontoggletool"
            || type == "ribbongallery"
            || type == "ribbongalleryitem"
            || type == "dataviewctrl"
            || type == "dataviewtreectrl"
            || type == "dataviewlistctrl"
            || type == "dataviewlistcolumn"
            || type == "dataviewcolumn"
            || type == "notebook"
            || type == "listbook"
            || type == "simplebook"
            || type == "choicebook"
            || type == "auinotebook"
            || type == "widget"
            || type == "expanded_widget"
            || type == "propgrid"
            || type == "propgridman"
            || type == "propgridpage"
            || type == "propgriditem"
            || type == "statusbar"
            || type == "component"
            || type == "container"
            || type == "menubar"
            || type == "treelistctrl"
            || type == "treelistctrlcolumn"
            || type == "toolbar"
            || type == "nonvisual"
            || type == "splitter");
}

void ObjectDatabase::ImportComponentLibrary(wxString libfile, PwxWeaverManager manager)
{
    wxString path = libfile;

    // Find the GetComponentLibrary function - all plugins must implement this
    typedef IComponentLibrary* (*PFGetComponentLibrary)(IManager * manager);

#ifdef __WXMAC__
    path += ".dylib";

    // open the library
    void* handle = dlopen(path.mb_str(), RTLD_LAZY);

    if (!handle) {
        wxString error = wxString(dlerror(), wxConvUTF8);

        wxWEAVER_THROW_EX("Error loading library " << path << " " << error)
    }
    dlerror(); // reset errors

    // load the symbol

    PFGetComponentLibrary GetComponentLibrary
        = (PFGetComponentLibrary)dlsym(handle, "GetComponentLibrary");
    PFFreeComponentLibrary FreeComponentLibrary
        = (PFFreeComponentLibrary)dlsym(handle, "FreeComponentLibrary");

    const char* dlsym_error = dlerror();
    if (dlsym_error) {
        wxString error = wxString(dlsym_error, wxConvUTF8);
        wxWEAVER_THROW_EX(path << " is not a valid component library: " << error)
            dlclose(handle);
    } else {
        m_libs.push_back(handle);
    }
#else
    // Attempt to load the DLL
    wxDynamicLibrary* library = new wxDynamicLibrary(path);
    if (!library->IsLoaded()) {
        wxWEAVER_THROW_EX("Error loading library " << path)
    }
    m_libs.push_back(library);

    PFGetComponentLibrary GetComponentLibrary
        = (PFGetComponentLibrary)library->GetSymbol("GetComponentLibrary");
    PFFreeComponentLibrary FreeComponentLibrary
        = (PFFreeComponentLibrary)library->GetSymbol("FreeComponentLibrary");

    if (!(GetComponentLibrary && FreeComponentLibrary)) {
        wxWEAVER_THROW_EX(path << " is not a valid component library")
    }
#endif
    LogDebug("[Database::ImportComponentLibrary] Importing " + path + " library");
    // Get the component library
    IComponentLibrary* comp_lib = GetComponentLibrary((IManager*)manager.get());

    // Store the function to free the library
    m_componentLibs[FreeComponentLibrary] = comp_lib;

    // Import all of the components
    for (size_t i = 0; i < comp_lib->GetComponentCount(); i++) {
        wxString className = comp_lib->GetComponentName(i);
        IComponent* comp = comp_lib->GetComponent(i);

        // Look for the class in the data read from the .xml files
        PObjectInfo classInfo = GetObjectInfo(className);
        if (classInfo) {
            classInfo->SetComponent(comp);
        } else {
            LogDebug("ObjectInfo for <" + className + "> not found while loading library <" + path + ">");
        }
    }

    // Add all of the macros in the library to the macro dictionary
    PMacroDictionary dic = MacroDictionary::GetInstance();
    for (size_t i = 0; i < comp_lib->GetMacroCount(); i++) {
        wxString name = comp_lib->GetMacroName(i);
        int value = comp_lib->GetMacroValue(i);
        dic->AddMacro(name, value);
        m_macroSet.erase(name);
    }
}

PropertyType ObjectDatabase::ParsePropertyType(wxString str)
{
    PropertyType result;
    PTMap::iterator it = m_propTypes.find(str);
    if (it != m_propTypes.end())
        result = it->second;
    else {
        wxWEAVER_THROW_EX(wxString::Format(
            "Unknown property type \"%s\"",
            str.c_str()));
    }
    return result;
}

wxString ObjectDatabase::ParseObjectType(wxString str)
{
    return str;
}

#define PT(x, y) m_propTypes.insert(PTMap::value_type(x, y))
void ObjectDatabase::InitPropertyTypes()
{
    PT("bool", PT_BOOL);
    PT("text", PT_TEXT);
    PT("int", PT_INT);
    PT("uint", PT_UINT);
    PT("bitlist", PT_BITLIST);
    PT("intlist", PT_INTLIST);
    PT("uintlist", PT_UINTLIST);
    PT("intpairlist", PT_INTPAIRLIST);
    PT("uintpairlist", PT_UINTPAIRLIST);
    PT("option", PT_OPTION);
    PT("macro", PT_MACRO);
    PT("path", PT_PATH);
    PT("file", PT_FILE);
    PT("wxString", PT_WXSTRING);
    PT("wxPoint", PT_WXPOINT);
    PT("wxSize", PT_WXSIZE);
    PT("wxFont", PT_WXFONT);
    PT("wxColour", PT_WXCOLOUR);
    PT("bitmap", PT_BITMAP);
    PT("wxString_i18n", PT_WXSTRING_I18N);
    PT("stringlist", PT_STRINGLIST);
    PT("float", PT_FLOAT);
    PT("parent", PT_PARENT);
    PT("editoption", PT_EDIT_OPTION);
}

bool ObjectDatabase::LoadObjectTypes()
{
    ticpp::Document doc;
    wxString xmlPath = m_xmlPath + "objtypes.xml";
    XMLUtils::LoadXMLFile(doc, true, xmlPath);

    // First load the object types, then the children
    try {
        ticpp::Element* root = doc.FirstChildElement("definitions");
        ticpp::Element* elem = root->FirstChildElement("objtype");
        while (elem) {
            bool hidden;
            elem->GetAttributeOrDefault("hidden", &hidden, false);

            bool item;
            elem->GetAttributeOrDefault("item", &item, false);

            wxString name = _WXSTR(elem->GetAttribute("name"));

            PObjectType objType(new ObjectType(name, (int)m_types.size(), hidden, item));
            m_types.insert(ObjectTypeMap::value_type(name, objType));

            elem = elem->NextSiblingElement("objtype", false);
        }
        // now load the children
        elem = root->FirstChildElement("objtype");
        while (elem) {
            wxString name = _WXSTR(elem->GetAttribute("name"));
            PObjectType objType = GetObjectType(name); // get the objType
            ticpp::Element* child = elem->FirstChildElement("childtype", false);
            while (child) {
                int max = -1;    // no limit
                int auiMax = -1; // no limit
                child->GetAttributeOrDefault("max", &max, -1);
                child->GetAttributeOrDefault("auiMax", &auiMax, -1);

                wxString childName = _WXSTR(child->GetAttribute("name"));
                PObjectType childType = GetObjectType(childName);
                if (!childType) {
                    wxLogError("No Object Type found for \"%s\"", childName.c_str());
                    continue;
                }
                objType->AddChildType(childType, max, auiMax);

                child = child->NextSiblingElement("childtype", false);
            }
            elem = elem->NextSiblingElement("objtype", false);
        }
    } catch (ticpp::Exception& ex) {
        wxLogError(_WXSTR(ex.m_details));
        return false;
    }
    return true;
}

PObjectType ObjectDatabase::GetObjectType(wxString name)
{
    PObjectType type;
    ObjectTypeMap::iterator it = m_types.find(name);
    if (it != m_types.end())
        type = it->second;

    return type;
}
