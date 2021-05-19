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
#include "model/types.h"

#include <component.h>

#include <list>

class OptionList {
public:
    void AddOption(wxString option, wxString description = wxString())
    {
        m_options[option] = description;
    }
    size_t GetOptionCount() { return (size_t)m_options.size(); }
    const std::map<wxString, wxString>& GetOptions() { return m_options; }

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
    PropertyInfo(wxString name, PropertyType type, wxString defValue,
                 wxString description, wxString customEditor,
                 POptionList optList, const std::list<PropertyChild>& children);
    ~PropertyInfo();

    wxString GetDefaultValue() { return m_defValue; }
    PropertyType GetType() { return m_type; }
    wxString GetName() { return m_name; }
    POptionList GetOptionList() { return m_optList; }
    std::list<PropertyChild>* GetChildren() { return &m_children; }
    wxString GetDescription() { return m_description; }
    wxString GetCustomEditor() { return m_customEditor; }

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
    EventInfo(const wxString& name,
              const wxString& eventClass,
              const wxString& defValue,
              const wxString& description);

    wxString GetName() { return m_name; }
    wxString GetEventClassName() { return m_eventClass; }
    wxString GetDefaultValue() { return m_defaultValue; }
    wxString GetDescription() { return m_description; }

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
    wxString GetName() { return m_info->GetName(); }
    wxString GetValue() { return m_value; }
    void SetValue(wxString& val) { m_value = val; }
    void SetValue(const wxChar* val) { m_value = val; }

    PPropertyInfo GetPropertyInfo() { return m_info; }
    PropertyType GetType() { return m_info->GetType(); }

    bool IsDefaultValue();
    bool IsNull();
    void SetDefaultValue();
    void ChangeDefaultValue(const wxString& value) { m_info->m_defValue = value; }

    void SetValue(const wxFontContainer& font);
    void SetValue(const wxColour& colour);
    void SetValue(const wxString& str, bool format = false);
    void SetValue(const wxPoint& point);
    void SetValue(const wxSize& size);
    void SetValue(const int integer);
    void SetValue(const double val);

    wxFontContainer GetValueAsFont();
    wxColour GetValueAsColour();
    wxPoint GetValueAsPoint();
    wxSize GetValueAsSize();
    int GetValueAsInteger();
    wxString GetValueAsString();
    wxBitmap GetValueAsBitmap();
    wxString GetValueAsText(); // sustituye los ('\n',...) por ("\\n",...)

    wxArrayString GetValueAsArrayString();
    double GetValueAsFloat();
    void SplitParentProperty(std::map<wxString, wxString>* children);
    wxString GetChildFromParent(const wxString& childName);

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
    void SetValue(const wxString& value) { m_value = value; }
    wxString GetValue() { return m_value; }
    wxString GetName() { return m_info->GetName(); }
    PObjectBase GetObject() { return m_object.lock(); }
    PEventInfo GetEventInfo() { return m_info; }

private:
    PEventInfo m_info;     // pointer to its descriptor
    WPObjectBase m_object; // pointer to the owner object
    wxString m_value;      // handler function name
};

class PropertyCategory {
private:
    wxString m_name;
    std::vector<wxString> m_properties;
    std::vector<wxString> m_events;
    std::vector<PPropertyCategory> m_categories;

public:
    PropertyCategory(wxString name)
        : m_name(name)
    {
    }
    void AddProperty(wxString name) { m_properties.push_back(name); }
    void AddEvent(wxString name) { m_events.push_back(name); }
    void AddCategory(PPropertyCategory category) { m_categories.push_back(category); }
    wxString GetName() { return m_name; }
    wxString GetPropertyName(size_t index)
    {
        if (index < m_properties.size())
            return m_properties[index];
        else
            return wxEmptyString;
    }

    wxString GetEventName(size_t index)
    {
        if (index < m_events.size())
            return m_events[index];
        else
            return wxEmptyString;
    }

    PPropertyCategory GetCategory(size_t index)
    {
        if (index < m_categories.size())
            return m_categories[index];
        else
            return PPropertyCategory();
    }
    size_t GetPropertyCount() { return m_properties.size(); }
    size_t GetEventCount() { return m_events.size(); }
    size_t GetCategoryCount() { return m_categories.size(); }
};

namespace ticpp {
class Document;
class Element;
} // namespace ticpp

class ObjectBase : public IObject, public std::enable_shared_from_this<ObjectBase> {
public:
    /** Constructor.
    */
    ObjectBase(wxString class_name);
    ~ObjectBase() override;

    /** Sets whether the object is expanded in the object tree or not.
    */
    void SetExpanded(bool expanded) { m_expanded = expanded; }

    /** Gets whether the object is expanded in the object tree or not.
    */
    bool GetExpanded() { return m_expanded; }

    /** Gets the object name.

        @note No confundir con la propiedad nombre que tienen algunos objetos.
              Cada objeto tiene un nombre, el cual será el mismo que el usado
              como clave en el registro de descriptores.
    */
    wxString GetClassName() override { return m_class; }

    /** Gets the parent object
    */
    PObjectBase GetParent() { return m_parent.lock(); }

    PObjectBase GetNonSizerParent();

    /** Links the object to a parent
    */
    void SetParent(PObjectBase parent) { m_parent = parent; }

    /** Obtiene la propiedad identificada por el nombre.

        @note Notar que no existe el método SetProperty, ya que la modificación
              se hace a través de la referencia.
    */
    PProperty GetProperty(wxString name);

    PEvent GetEvent(wxString name);

    /** Añade una propiedad al objeto.

        Este método será usado por el registro de descriptores para crear la
        instancia del objeto.
        Los objetos siempre se crearán a través del registro de descriptores.
    */
    void AddProperty(PProperty value);

    void AddEvent(PEvent event);

    /** Gets the object' property count.
    */
    size_t GetPropertyCount() { return m_properties.size(); }

    size_t GetEventCount() { return m_events.size(); }

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
    PProperty GetProperty(size_t idx); // throws ...;

    PEvent GetEvent(size_t idx); // throws ...;

    /** Devuelve el primer antecesor cuyo tipo coincida con el que se pasa
        como parámetro.

        Será útil para encontrar el widget padre.
    */
    PObjectBase FindNearAncestor(wxString type);
    PObjectBase FindNearAncestorByBaseClass(wxString type);
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
    size_t GetChildCount() override { return m_children.size(); }

    /** Comprueba si el tipo de objeto pasado es válido como hijo del objeto.

        Esta rutina es importante, ya que define las restricciónes de ubicación.
    */
    bool ChildTypeOk(PObjectType type);
#if 0
    bool ChildTypeOk (wxString type);
#endif
    bool IsContainer()
    {
        return (GetObjectTypeName() == "container");
    }

    PObjectBase GetLayout();

    /** Devuelve el tipo de objeto.

        Deberá ser redefinida en cada clase derivada.
    */
    wxString GetObjectTypeName() override { return m_type; }

    void SetObjectTypeName(wxString type) { m_type = type; }

    /** Devuelve el descriptor del objeto.
    */
    PObjectInfo GetObjectInfo() { return m_info; }

    void SetObjectInfo(PObjectInfo info) { m_info = info; }

    /** Devuelve la profundidad  del objeto en el arbol.
    */
    int Deep();
#if 0
    /** Imprime el arbol en un stream.
    */
    virtual void PrintOut(std::ostream &s, int indent);

    /** Sobrecarga del operador inserción.
    */
    friend std::ostream& operator<<(std::ostream& s, PObjectBase obj);
#endif
    // Implementación de la interfaz IObject para su uso dentro de los plugins
    bool IsNull(const wxString& name) override;
    int GetPropertyAsInteger(const wxString& name) override;
    wxFontContainer GetPropertyAsFont(const wxString& name) override;
    wxColour GetPropertyAsColour(const wxString& name) override;
    wxString GetPropertyAsString(const wxString& name) override;
    wxPoint GetPropertyAsPoint(const wxString& name) override;
    wxSize GetPropertyAsSize(const wxString& name) override;
    wxBitmap GetPropertyAsBitmap(const wxString& name) override;
    double GetPropertyAsFloat(const wxString& name) override;

    wxArrayInt GetPropertyAsArrayInt(const wxString& name) override;
    wxArrayString GetPropertyAsArrayString(const wxString& name) override;
    std::vector<std::pair<int, int>> GetPropertyAsVectorIntPair(const wxString& name) override;
    wxString GetChildFromParentProperty(const wxString& parentName,
                                        const wxString& childName) override;

    IObject* GetChildPtr(size_t idx) override { return GetChild(idx).get(); }

protected:
    // utilites for implementing the tree
    static const int INDENT;              // size of indent
    wxString GetIndentString(int indent); // obtiene la cadena con el indentado

    ObjectBaseVector& GetChildren() { return m_children; }
    PropertyMap& GetProperties() { return m_properties; }

    // Crea un elemento del objeto
    void SerializeObject(ticpp::Element* serializedElement);

    // devuelve el puntero "this"
    PObjectBase GetThis() { return shared_from_this(); }

private:
    friend class wxWeaverDataObject;

    ObjectBaseVector m_children;
    PropertyMap m_properties;
    EventMap m_events;
    PObjectInfo m_info;
    WPObjectBase m_parent; // weak pointer, no reference loops please!
    wxString m_class;      // class name
    wxString m_type;       // type of object
    bool m_expanded;       // is expanded in the object tree, allows for saving to file
};

/** Clase que guarda un conjunto de plantillas de código.
*/
class CodeInfo {
private:
    typedef std::map<wxString, wxString> TemplateMap;
    TemplateMap m_templates;

public:
    wxString GetTemplate(wxString name);
    void AddTemplate(wxString name, wxString _template);
    void Merge(PCodeInfo merger);
};

/** Información de objeto o MetaObjeto.
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
    wxString GetBaseClassDefaultPropertyValue(size_t baseIndex, const wxString& propertyName);

    /** Devuelve el tipo de objeto.

        Será util para que el constructor de objetos sepa la clase derivada
        de ObjectBase que ha de crear a partir del descriptor.
    */
    wxString GetObjectTypeName() { return m_type->GetName(); }

    PObjectType GetObjectType() { return m_type; }

    wxString GetClassName() { return m_class; }
#if 0
    /** Imprime el descriptor en un stream.
    */
    virtual void PrintOut(std::ostream &s, int indent);

    /** Sobrecarga del operador inserción.
    */
    friend std::ostream& operator<<(std::ostream& s, PObjectInfo obj);
#endif
    // nos serán utiles para generar el nombre del objeto
    size_t GetInstanceCount() { return m_numIns; }
    void IncrementInstanceCount() { m_numIns++; }
    void ResetInstanceCount() { m_numIns = 0; }

    /** Añade la información de un objeto al conjunto de clases base.
    */
    size_t AddBaseClass(PObjectInfo base)
    {
        m_base.push_back(base);
        return m_base.size() - 1;
    }

    /** Comprueba si el tipo es derivado del que se pasa como parámetro.
    */
    bool IsSubclassOf(wxString classname);

    PObjectInfo GetBaseClass(size_t idx, bool inherited = true);
    void GetBaseClasses(std::vector<PObjectInfo>& classes, bool inherited = true);
    size_t GetBaseClassCount(bool inherited = true);

    void SetIconFile(wxBitmap icon) { m_icon = icon; }
    wxBitmap GetIconFile() { return m_icon; }

    void SetSmallIconFile(wxBitmap icon) { m_smallIcon = icon; }
    wxBitmap GetSmallIconFile() { return m_smallIcon; }

    void AddCodeInfo(wxString lang, PCodeInfo codeinfo);
    PCodeInfo GetCodeInfo(wxString lang);

    PObjectPackage GetPackage();

    bool IsStartOfGroup() { return m_startGroup; }

    /** Le asigna un componente a la clase.
    */
    void SetComponent(IComponent* c) { m_component = c; }
    IComponent* GetComponent() { return m_component; }

private:
    wxString m_class;          // nombre de la clase (tipo de objeto)
    PObjectType m_type;        // tipo del objeto
    WPObjectPackage m_package; // Package that the object comes from
    PPropertyCategory m_category;
    IComponent* m_component; // componente asociado a la clase los objetos del designer

    std::vector<PObjectInfo> m_base;          // base classes
    std::map<wxString, PCodeInfo> m_codeTemp; // plantillas de codigo K=language_name T=PCodeInfo
    std::map<wxString, PPropertyInfo> m_properties;
    std::map<wxString, PEventInfo> m_events;
    std::map<size_t, std::map<wxString, wxString>> m_baseClassDefaultPropertyValues;

    wxBitmap m_icon;
    wxBitmap m_smallIcon; // The icon for the property grid toolbar

    bool m_startGroup; // Place a separator in the palette toolbar just before this widget
    size_t m_numIns;   // número de instancias del objeto
};
