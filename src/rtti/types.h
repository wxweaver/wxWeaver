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

#include <wx/wx.h>

#include <map>
#include <memory>
#include <utility>
#include <vector>

class ObjectType;

typedef std::shared_ptr<ObjectType> PObjectType;
typedef std::weak_ptr<ObjectType> WPObjectType;

/** Representa el tipo de objeto.

    Los tipos de objetos son necesarios para controlar las restricciones de
    ubicación de los objetos dentro del árbol. Dichas restricciones vendrán
    establecidas en el fichero objtypes.xml, y en principio se pueden definir
    tantos tipos de objetos como sean necesarios.

    Aunque el conjunto de tipos está pensado para que sea fácilmente modificable,
    actualmente, en el código hay muchas dependencias con los nombres de tipos
    concretos. Así que una modificación en el nombre de un tipo casi con toda
    seguridad causará fallos en el funcionamiento de la aplicación.

    @todo hay que eliminar las dependencias en el código con los nombres de los
          tipos. Para ello lo mejor será definir una serie de atributos asociados
          al tipo.
          Por ejemplo, los objetos que sean "items" (objetos ficticios
          que añaden ciertas propiedades al objeto que contiene como puede ser
          un sizeritem), no deben aparecer en el "object tree" y deben mostrar
          las propiedades junto con las del objeto que contiene en el "object
          inspector". En ese caso, tanto el "object tree" como el
          "object inspector" consultarán al tipo si éste tiene el
          atributo item a true.
 */
class ObjectType {
public:
    ObjectType(wxString name, int id, bool hidden = false, bool item = false);

    int GetId() { return m_id; }
    wxString GetName() { return m_name; }
#if 0
    bool IsHidden() { return m_hidden; }
#endif
    bool IsItem()
    {
        return m_item;
    }

    /** Añade el tipo de objeto a la lista de posibles hijos.
    */
    void AddChildType(PObjectType type, int max = -1, int auiMax = -1);

    /** Busca si el tipo pasado como parámetros está entre sus posibles hijos.

        @return numero máximo de ocurrencias del objeto como hijo.
        -1 = unlimited, 0 = none
    */
    size_t FindChildType(int typeId, bool aui);
    size_t FindChildType(PObjectType type, bool aui);

    size_t GetChildTypeCount();
    PObjectType GetChildType(size_t idx);

private:
    struct ChildCount {
        ChildCount(int m, int am)
            : max(m)
            , auiMax(am)
        {
        }
        size_t max;
        size_t auiMax;
    };

    /** Registro con los tipos de los hijos posibles y el número máximo de estos.

        @note vamos a usar smart-pointers de tipo "weak" ya que puede haber muchas
        referencias cruzadas.
    */
    typedef std::map<WPObjectType, ChildCount, std::owner_less<WPObjectType>> ChildTypeMap;
    ChildTypeMap m_childTypes; /**< registro de posibles hijos */

    wxString m_name; /**< cadena de texto asociado al tipo */
    int m_id;        /**< identificador numérico del tipo de objeto */
    bool m_hidden;   /**< indica si está oculto en el ObjectTree */
    bool m_item;     /**< indica si es un "item". Los objetos contenidos en
                          en un item, muestran las propiedades de éste junto
                          con las propias del objeto.
                     */
};

/** Property types.
 */
typedef enum {
    PT_ERROR,
    PT_BOOL,
    PT_TEXT,
    PT_INT,
    PT_UINT,
    PT_BITLIST,
    PT_INTLIST,
    PT_UINTLIST,
    PT_INTPAIRLIST,
    PT_UINTPAIRLIST,
    PT_OPTION,
    PT_MACRO,
    PT_WXSTRING,
    PT_WXPOINT,
    PT_WXSIZE,
    PT_WXFONT,
    PT_WXCOLOUR,
    PT_WXPARENT,
    PT_WXPARENT_SB,
    PT_WXPARENT_CP,
    PT_PATH,
    PT_FILE,
    PT_BITMAP,
    PT_STRINGLIST,
    PT_FLOAT,
    PT_WXSTRING_I18N,
    PT_PARENT,
    PT_CLASS,
    PT_EDIT_OPTION
} PropertyType;

/** Int list.
 */
class IntList {
public:
    explicit IntList(bool absoluteValue = false, bool pairValue = false);
    explicit IntList(const wxString& value,
                     bool absoluteValue = false, bool pairValue = false);

    size_t GetSize() const { return m_ints.size(); }
    int GetValue(size_t idx) const { return m_ints[idx].first; }
    std::pair<int, int> GetPair(size_t idx) const { return m_ints[idx]; }
    void Add(int value);
    void Add(int first, int second);
    void DeleteList();
    void SetList(const wxString& str);
    wxString ToString(bool skipZeroSecond = false);

private:
    std::vector<std::pair<int, int>> m_ints;
    bool m_abs;
    bool m_pairs;
};
