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
/*
    BEWARE!
    ObjectBase::GetPropertyCount() != ObjectInfo::GetPropertyCount()

    En el primer caso devolverá el numero total de propiedades del objeto.
    En el segundo caso sólo devolverá el número de propiedades definidas
    para esa clase.
*/
#pragma once

#include "utils/defs.h"
#include "rtti/types.h"

#include <component.h>

#include <list>

class OptionList {
public:
    void AddOption(const wxString& option, const wxString& description = wxString())
    {
        m_options[option] = description;
    }
    size_t GetOptionCount() const { return m_options.size(); }
    std::map<wxString, wxString> GetOptions() const { return m_options; }

private:
    std::map<wxString, wxString> m_options;
};

/** @internal Data Container for children of a Parent property
*/
struct PropertyChild {
    wxString m_name;
    wxString m_defaultValue;
    wxString m_description;
    PropertyType m_type;
};

class PropertyInfo {
public:
    PropertyInfo(const wxString& name, PropertyType type, const wxString& defValue,
                 const wxString& description, const wxString& customEditor,
                 POptionList optList, const std::list<PropertyChild>& children);
    ~PropertyInfo();

    wxString GetCustomEditor() const { return m_customEditor; }
    wxString GetDefaultValue() const { return m_defValue; }
    wxString GetDescription() const { return m_description; }
    wxString GetName() const { return m_name; }
    wxString GetLabel() const { return _(m_name); }

    POptionList GetOptionList() const { return m_optList; }
    PropertyType GetType() const { return m_type; }
    std::list<PropertyChild>* GetChildren() { return &m_children; }

private:
    friend class Property;

    wxString m_name;
    wxString m_defValue;
    wxString m_description;
    wxString m_customEditor;             // An optional custom editor for the property grid
    std::list<PropertyChild> m_children; // Only used for parent properties
    PropertyType m_type;
    POptionList m_optList;
};

class EventInfo {
public:
    EventInfo(const wxString& name, const wxString& eventClass,
              const wxString& defValue, const wxString& description);

    wxString GetDefaultValue() const { return m_defaultValue; }
    wxString GetDescription() const { return m_description; }
    wxString GetEventClassName() const { return m_eventClass; }
    wxString GetLabel() const { return _(m_name); }
    wxString GetName() const { return m_name; }

private:
    wxString m_name;
    wxString m_eventClass;
    wxString m_defaultValue;
    wxString m_description;
};

class Property {
public:
    Property(PPropertyInfo info, PObjectBase obj = PObjectBase())
        : m_info(info)
        , m_object(obj)
    {
    }

    PObjectBase GetObject() { return m_object.lock(); }
    PPropertyInfo GetPropertyInfo() { return m_info; }
    PropertyType GetType() const { return m_info->GetType(); }

    bool IsDefaultValue() const;
    bool IsNull() const;

    // TODO: const, probably needs to be renamed
    wxString GetChildFromParent(const wxString& childName);
    wxString GetName() const { return m_info->GetName(); }

    wxArrayString GetValueAsArrayString() const;
    wxBitmap GetValueAsBitmap() const;
    wxColour GetValueAsColour() const;
    wxFontContainer GetValueAsFont() const;
    wxPoint GetValueAsPoint() const;
    wxSize GetValueAsSize() const;
    wxString GetValueAsString() const;
    wxString GetValueAsText() const; // replaces ('\n',...) with ("\\n",...)
    double GetValueAsFloat() const;
    int GetValueAsInteger() const;

    void SetValue(const wxChar* value) { m_value = value; }
    void SetValue(const wxColour& value);
    void SetValue(const wxFontContainer& value);
    void SetValue(const wxSize& value);
    void SetValue(const wxString& value, bool format = false);
    void SetValue(const wxPoint& value);
    void SetValue(const double value);
    void SetValue(const int value);

    // TODO: Return a value instead using an output parameter
    void SplitParentProperty(std::map<wxString, wxString>* children);

private:
    PPropertyInfo m_info;  // pointer to its descriptor
    WPObjectBase m_object; // pointer to the owner object
    wxString m_value;
};

class Event {
public:
    Event(PEventInfo info, PObjectBase obj)
        : m_info(info)
        , m_object(obj)
    {
    }

    PObjectBase GetObject() { return m_object.lock(); }
    PEventInfo GetEventInfo() { return m_info; }

    wxString GetName() const { return m_info->GetName(); }
    wxString GetValue() const { return m_value; }

    void SetValue(const wxString& value) { m_value = value; }

private:
    PEventInfo m_info;     // pointer to its descriptor
    WPObjectBase m_object; // pointer to the owner object
    wxString m_value;      // handler function name
};

class PropertyCategory {
public:
    PropertyCategory(wxString name)
        : m_name(name)
    {
    }

    PPropertyCategory GetCategory(size_t index) const;

    wxString GetName() const { return m_name; }
    wxString GetLabel() const { return _(m_name); }

    wxString GetEventName(size_t index) const;
    wxString GetPropertyName(size_t index) const;

    size_t GetCategoryCount() const { return m_categories.size(); }
    size_t GetEventCount() const { return m_events.size(); }
    size_t GetPropertyCount() const { return m_properties.size(); }

    void AddCategory(PPropertyCategory category) { m_categories.push_back(category); }
    void AddEvent(const wxString& name) { m_events.push_back(name); }
    void AddProperty(const wxString& name) { m_properties.push_back(name); }

private:
    wxString m_name;
    std::vector<wxString> m_properties;
    std::vector<wxString> m_events;
    std::vector<PPropertyCategory> m_categories;
};

namespace ticpp {
class Document;
class Element;
} // namespace ticpp

class ObjectBase : public IObject, public std::enable_shared_from_this<ObjectBase> {
public:
    /** Constructor.
    */
    ObjectBase(const wxString& className);
    ~ObjectBase() override;

    /** Sets whether the object is expanded in the object tree or not.
    */
    void SetExpanded(bool expanded) { m_expanded = expanded; }

    /** Gets whether the object is expanded in the object tree or not.
    */
    bool GetExpanded() const { return m_expanded; }

    /** Gets the object name.

        @note No confundir con la propiedad nombre que tienen algunos objetos.
              Cada objeto tiene un nombre, el cual será el mismo que el usado
              como clave en el registro de descriptores.
    */
    wxString GetClassName() const override { return m_class; }

    /** Gets the parent object
    */
    PObjectBase GetParent() const { return m_parent.lock(); }

    PObjectBase GetNonSizerParent();

    /** Links the object to a parent
    */
    void SetParent(PObjectBase parent) { m_parent = parent; }

    /** Obtiene la propiedad identificada por el nombre.

        @note Notar que no existe el método SetProperty, ya que la modificación
              se hace a través de la referencia.
    */
    PProperty GetProperty(const wxString& name) const;

    PEvent GetEvent(const wxString& name);

    /** Añade una propiedad al objeto.

        Este método será usado por el registro de descriptores para crear la
        instancia del objeto.
        Los objetos siempre se crearán a través del registro de descriptores.
    */
    void AddProperty(PProperty value);

    void AddEvent(PEvent event);

    /** Gets the object' property count.
    */
    size_t GetPropertyCount() const { return m_properties.size(); }

    size_t GetEventCount() const { return m_events.size(); }

    /** Obtiene una propiedad del objeto.
        @todo esta función deberá lanzar una excepción en caso de no encontrarse
              dicha propiedad, así se simplifica el código al no tener que hacer
              tantas comprobaciones.

     Por ejemplo, el código sin excepciones sería algo así:

     @code
    PProperty plabel = obj->GetProperty("label");
    PProperty ppos = obj->GetProperty("pos");
    PProperty psize = obj->GetProperty("size");
    PProperty pstyle = obj->GetProperty("style");

    if (plabel && ppos && psize && pstyle) {
        wxButton* button = new wxButton(parent,wxID_ANY,
        plabel->GetValueAsString(),
        ppos->GetValueAsPoint(),
        psize->GetValueAsSize(),
        pstyle->GetValueAsInteger());
    } else {
        // manejo del error
    }
     @endcode

     y con excepciones:

     @code
    try
    {
        wxButton* button = new wxButton(parent,wxID_ANY,
        obj->GetProperty("label")->GetValueAsString(),
        obj->GetProperty("pos")->GetValueAsPoint(),
        obj->GetProperty("size")->GetValueAsSize(),
        obj->GetProperty("style")->GetValueAsInteger());
    }
    catch (...)
    {
        // manejo del error
    }
     @endcode
    */
    PProperty GetProperty(size_t index) const; // throws ...;

    PEvent GetEvent(size_t index); // throws ...;

    /** Devuelve el primer antecesor cuyo tipo coincida con el que se pasa
        como parámetro.

        Será útil para encontrar el widget padre.
    */
    PObjectBase FindNearAncestor(const wxString& type);
    PObjectBase FindNearAncestorByBaseClass(const wxString& type);
    PObjectBase FindParentForm();

    /** Obtiene el documento xml del arbol tomando como raíz el nodo actual.
    */
    void Serialize(ticpp::Document* serializedDocument);

    /** Añade un hijo al objeto.

        Esta función es virtual, debido a que puede variar el comportamiento
        según el tipo de objeto.

        @return true si se añadió el hijo con éxito y false en caso contrario.
    */
    virtual bool AddChild(PObjectBase);
    virtual bool AddChild(size_t idx, PObjectBase obj);

    /** Devuelve la posicion del hijo o GetChildCount() en caso de no encontrarlo
    */
    size_t GetChildPosition(PObjectBase obj);
    bool ChangeChildPosition(PObjectBase obj, size_t pos);
#if 0
    /** Devuelve la posición entre sus hermanos
    */
    size_t GetPosition();
    bool ChangePosition(size_t pos);
#endif
    /** Removes an object' child.
    */
    void RemoveChild(PObjectBase obj);
    void RemoveChild(size_t idx);
    void RemoveAllChildren() { m_children.clear(); }

    /** Gets an object' child.
    */
    PObjectBase GetChild(size_t idx);

    PObjectBase GetChild(size_t idx, const wxString& type);

    /** Returns the child objects count.
    */
    size_t GetChildCount() const override { return m_children.size(); }

    /** Comprueba si el tipo de objeto pasado es válido como hijo del objeto.

        Esta rutina es importante, ya que define las restricciónes de ubicación.
    */
    bool ChildTypeOk(PObjectType type);
#if 0
    bool ChildTypeOk (wxString type);
#endif
    bool IsContainer()
    {
        return (GetTypeName() == "container");
    }

    PObjectBase GetLayout();

    /** Devuelve el tipo de objeto.

        Deberá ser redefinida en cada clase derivada.
    */
    wxString GetTypeName() const override { return m_type; }

    void SetObjectTypeName(wxString type) { m_type = type; }

    /** Devuelve el descriptor del objeto.
    */
    PObjectInfo GetObjectInfo() { return m_info; }

    void SetObjectInfo(PObjectInfo info) { m_info = info; }

    /** Returns the depth of the object in the tree.
    */
    int Depth() const;
#if 0
    /** Imprime el arbol en un stream.
    */
    virtual void PrintOut(std::ostream &s, int indent);

    /** Sobrecarga del operador inserción.
    */
    friend std::ostream& operator<<(std::ostream& s, PObjectBase obj);
#endif
    // Implementación de la interfaz IObject para su uso dentro de los plugins
    bool IsNull(const wxString& name) const override;
    int GetPropertyAsInteger(const wxString& name) const override;
    wxFontContainer GetPropertyAsFont(const wxString& name) const override;
    wxColour GetPropertyAsColour(const wxString& name) const override;
    wxString GetPropertyAsString(const wxString& name) const override;
    wxPoint GetPropertyAsPoint(const wxString& name) const override;
    wxSize GetPropertyAsSize(const wxString& name) const override;
    wxBitmap GetPropertyAsBitmap(const wxString& name) const override;
    double GetPropertyAsFloat(const wxString& name) const override;

    wxArrayInt GetPropertyAsArrayInt(const wxString& name) const override;
    wxArrayString GetPropertyAsArrayString(const wxString& name) const override;
    std::vector<std::pair<int, int>> GetPropertyAsVectorIntPair(const wxString& name) override;
    wxString GetChildFromParentProperty(const wxString& parentName,
                                        const wxString& childName) const override;

    IObject* GetChildPtr(size_t idx) override { return GetChild(idx).get(); }

protected:
    // Utilites for implementing the tree
    static const int INDENT;              // size of indent
    wxString GetIndentString(int indent); // get the string with indentation

    ObjectBaseVector& GetChildren() { return m_children; }
    PropertyMap& GetProperties() { return m_properties; }

    // Create an object element
    void SerializeObject(ticpp::Element* serializedElement);

    // Returns the "this" pointer
    PObjectBase GetThis() { return shared_from_this(); }

private:
    friend class wxWeaverDataObject;

    ObjectBaseVector m_children;
    PropertyMap m_properties;
    EventMap m_events;
    PObjectInfo m_info;
    WPObjectBase m_parent; // weak pointer
    wxString m_class;      // class name
    wxString m_type;       // object type
    bool m_expanded;       // is expanded in the object tree, allows for saving to file
};

/** Class that stores a set of code templates.
*/
class CodeInfo {
public:
    wxString GetTemplate(const wxString& name);
    void AddTemplate(const wxString& name, const wxString& template_);
    void Merge(PCodeInfo merger);

private:
    typedef std::map<wxString, wxString> TemplateMap;
    TemplateMap m_templates;
};

/** Object or MetaObject information.
*/
class ObjectInfo {
public:
    /** Constructor.
    */
    ObjectInfo(wxString className, PObjectType type, WPObjectPackage package,
               bool startGroup = false);
    virtual ~ObjectInfo() = default;

    PPropertyCategory GetCategory() { return m_category; }
    size_t GetPropertyCount() { return m_properties.size(); }
    size_t GetEventCount() { return m_events.size(); }

    /** Obtiene el descriptor de la propiedad.
    */
    PPropertyInfo GetPropertyInfo(wxString name);
    PPropertyInfo GetPropertyInfo(size_t idx);

    PEventInfo GetEventInfo(wxString name);
    PEventInfo GetEventInfo(size_t idx);

    /** Añade un descriptor de propiedad al descriptor de objeto.
    */
    void AddPropertyInfo(PPropertyInfo pinfo);

    /** Adds an event descriptor.
     */
    void AddEventInfo(PEventInfo evtInfo);

    /** Adds a default value for an inherited property.

        @param baseIndex Index of base class returned from AddBaseClass.
        @param propertyName Property of base class to assign a default value to.
        @param defaultValue Default value of the inherited property.
    */
    void AddBaseClassDefaultPropertyValue(size_t baseIndex, const wxString& propertyName, const wxString& defaultValue);

    /** Gets a default value for an inherited property.

        @param baseIndex Index of base class in the base class vector
        @param propertName Name of the property to get the default value for
        @return The default value for the property
    */
    wxString GetBaseClassDefaultPropertyValue(size_t baseIndex, const wxString& propertyName) const;

    /** Returns the name of the object type.

        It will be useful for the object constructor to know the class derived
        from ObjectBase to create from the descriptor.
    */
    wxString GetTypeName() const { return m_type->GetName(); }

    PObjectType GetType() { return m_type; }

    wxString GetClassName() const { return m_class; }
#if 0
    /** Imprime el descriptor en un stream.
    */
    virtual void PrintOut(std::ostream &s, int indent);

    /** Sobrecarga del operador inserción.
    */
    friend std::ostream& operator<<(std::ostream& s, PObjectInfo obj);
#endif
    // These will help to generate the name of the object
    size_t GetInstanceCount() const { return m_numIns; }
    void IncrementInstanceCount() { m_numIns++; }
    void ResetInstanceCount() { m_numIns = 0; }

    /** Añade la información de un objeto al conjunto de clases base.
    */
    size_t AddBaseClass(PObjectInfo base)
    {
        m_base.push_back(base);
        return m_base.size() - 1;
    }

    // TODO: Rewrite these 4 functions with constness
    /** Checks if the class is derived from the one passed as a parameter.
    */
    bool IsSubclassOf(const wxString& className) const;

    PObjectInfo GetBaseClass(size_t index, bool inherited = true) const;
    std::vector<PObjectInfo> GetBaseClasses(bool inherited = true) const;
    size_t GetBaseClassCount(bool inherited = true) const;

    void SetIconFile(wxBitmap icon) { m_icon = icon; }
    wxBitmap GetIconFile() { return m_icon; }

    void SetSmallIconFile(wxBitmap icon) { m_smallIcon = icon; }
    wxBitmap GetSmallIconFile() { return m_smallIcon; }

    void AddCodeInfo(const wxString& lang, PCodeInfo codeinfo);
    PCodeInfo GetCodeInfo(const wxString& language);

    PObjectPackage GetPackage();

    bool IsStartOfGroup() const { return m_startGroup; }

    /** Le asigna un componente a la clase.
    */
    void SetComponent(IComponent* component) { m_component = component; }
    IComponent* GetComponent() { return m_component; }

private:
    wxString m_class;          // Class name
    PObjectType m_type;        // Object type
    WPObjectPackage m_package; // Package that the object comes from
    PPropertyCategory m_category;
    IComponent* m_component; // Component associated with the designer objects class

    std::vector<PObjectInfo> m_base;          // Base classes
    std::map<wxString, PCodeInfo> m_codeTemp; // Code templates
    std::map<wxString, PPropertyInfo> m_properties;
    std::map<wxString, PEventInfo> m_events;
    std::map<size_t, std::map<wxString, wxString>> m_baseClassDefaultPropertyValues;

    wxBitmap m_icon;
    wxBitmap m_smallIcon; // The icon for the property grid toolbar

    bool m_startGroup; // Place a separator in the palette toolbar just before this widget
    size_t m_numIns;   // Number of instances of the object
};
