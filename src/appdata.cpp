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
#include "appdata.h"

#include "gui/bitmaps.h"
#include "event.h"
#include "manager.h"

#include "codegen/codewriter.h"
#include "codegen/cppcg.h"
#include "codegen/luacg.h"
#include "codegen/phpcg.h"
#include "codegen/pythoncg.h"
#include "rtti/objectbase.h"
#include "utils/stringutils.h"
#include "utils/typeconv.h"
#include "utils/exception.h"
#include "utils/ipc.h"
#include "dataobject.h"
#include "gui/dialogs/xrcpreview.h"

#ifdef wxWEAVER_DEBUG
#include "gui/panels/debugwindow.h"
#endif

#include <ticpp.h>

#include <wx/clipbrd.h>
#include <wx/ffile.h>
#include <wx/fs_arc.h>
#include <wx/fs_filter.h>
#include <wx/fs_mem.h>
#include <wx/richmsgdlg.h>
#include <wx/tokenzr.h>

using namespace TypeConv;

const char* const VERSION = "0.1.0"; // TODO: Move this to CMake
const char* const REVISION = "";

/** Command for expanding an object in the object tree
*/
class ExpandObjectCmd : public Command {
public:
    ExpandObjectCmd(PObjectBase object, bool expand);

protected:
    void DoExecute() override;
    void DoRestore() override;

private:
    PObjectBase m_object;
    bool m_expand;
};

/** Command to insert an object into the tree.
*/
class InsertObjectCmd : public Command {
public:
    InsertObjectCmd(ApplicationData* data, PObjectBase object, PObjectBase parent, int pos = -1);

protected:
    void DoExecute() override;
    void DoRestore() override;

private:
    ApplicationData* m_data;
    PObjectBase m_parent;
    PObjectBase m_object;
    int m_pos;
    PObjectBase m_oldSelected;
};

/** Command to delete an object.
*/
class RemoveObjectCmd : public Command {
public:
    RemoveObjectCmd(ApplicationData* data, PObjectBase object);

protected:
    void DoExecute() override;
    void DoRestore() override;

private:
    ApplicationData* m_data;
    PObjectBase m_parent;
    PObjectBase m_object;
    int m_oldPos;
    PObjectBase m_oldSelected;
};

/** Property modification command.
*/
class ModifyPropertyCmd : public Command {
public:
    ModifyPropertyCmd(PProperty prop, wxString value);

protected:
    void DoExecute() override;
    void DoRestore() override;

private:
    PProperty m_property;
    wxString m_oldValue, m_newValue;
};

/** Command for modifying an event
*/
class ModifyEventHandlerCmd : public Command {
public:
    ModifyEventHandlerCmd(PEvent event, wxString value);

protected:
    void DoExecute() override;
    void DoRestore() override;

private:
    PEvent m_event;
    wxString m_oldValue, m_newValue;
};

/** Moving object command.
*/
class ShiftChildCmd : public Command {
public:
    ShiftChildCmd(PObjectBase object, int pos);

protected:
    void DoExecute() override;
    void DoRestore() override;

private:
    PObjectBase m_object;
    int m_oldPos, m_newPos;
};
/**
* CutObjectCmd ademas de eliminar el objeto del árbol se asegura
* de eliminar la referencia "clipboard" deshacer el cambio.
*/
class CutObjectCmd : public Command {
public:
    CutObjectCmd(ApplicationData* data, PObjectBase object);

protected:
    void DoExecute() override;
    void DoRestore() override;

private:
    // necesario para consultar/modificar el objeto "clipboard"
    ApplicationData* m_data;
#if 0
    PObjectBase m_clipboard;
#endif
    PObjectBase m_parent;
    PObjectBase m_object;
    int m_oldPos;
    PObjectBase m_oldSelected;
};

/** Reparent an object.
*/
class ReparentObjectCmd : public Command {
public:
    ReparentObjectCmd(PObjectBase sizeritem, PObjectBase sizer);

protected:
    void DoExecute() override;
    void DoRestore() override;

private:
    PObjectBase m_sizeritem;
    PObjectBase m_sizer;
    PObjectBase m_oldSizer;
    int m_oldPosition;
};

ExpandObjectCmd::ExpandObjectCmd(PObjectBase object, bool expand)
    : m_object(object)
    , m_expand(expand)
{
}

void ExpandObjectCmd::DoExecute()
{
    m_object->SetExpanded(m_expand);
}

void ExpandObjectCmd::DoRestore()
{
    m_object->SetExpanded(!m_expand);
}

InsertObjectCmd::InsertObjectCmd(ApplicationData* data, PObjectBase object,
                                 PObjectBase parent, int pos)
    : m_data(data)
    , m_parent(parent)
    , m_object(object)
    , m_pos(pos)
{
    m_oldSelected = data->GetSelectedObject();
}

void InsertObjectCmd::DoExecute()
{
    m_parent->AddChild(m_object);
    m_object->SetParent(m_parent);

    if (m_pos >= 0)
        m_parent->ChangeChildPosition(m_object, m_pos);

    PObjectBase obj = m_object;
    while (obj && obj->GetObjectInfo()->GetType()->IsItem()) {
        if (obj->GetChildCount() > 0)
            obj = obj->GetChild(0);
        else
            return;
    }
    m_data->SelectObject(obj, false, false);
}

void InsertObjectCmd::DoRestore()
{
    m_parent->RemoveChild(m_object);
    m_object->SetParent(PObjectBase());
    m_data->SelectObject(m_oldSelected);
}

RemoveObjectCmd::RemoveObjectCmd(ApplicationData* data, PObjectBase object)
{
    m_data = data;
    m_object = object;
    m_parent = object->GetParent();
    m_oldPos = m_parent->GetChildPosition(object);
    m_oldSelected = data->GetSelectedObject();
}

void RemoveObjectCmd::DoExecute()
{
    m_parent->RemoveChild(m_object);
    m_object->SetParent(PObjectBase());
    m_data->DetermineObjectToSelect(m_parent, m_oldPos);
}

void RemoveObjectCmd::DoRestore()
{
    m_parent->AddChild(m_object);
    m_object->SetParent(m_parent);

    // restore the position
    m_parent->ChangeChildPosition(m_object, m_oldPos);
    m_data->SelectObject(m_oldSelected, true, false);
}

ModifyPropertyCmd::ModifyPropertyCmd(PProperty prop, wxString value)
    : m_property(prop)
    , m_newValue(value)
{
    m_oldValue = prop->GetValueAsString();
}

void ModifyPropertyCmd::DoExecute()
{
    m_property->SetValue(m_newValue);
}

void ModifyPropertyCmd::DoRestore()
{
    m_property->SetValue(m_oldValue);
}

ModifyEventHandlerCmd::ModifyEventHandlerCmd(PEvent event, wxString value)
    : m_event(event)
    , m_newValue(value)
{
    m_oldValue = event->GetValue();
}

void ModifyEventHandlerCmd::DoExecute()
{
    m_event->SetValue(m_newValue);
}

void ModifyEventHandlerCmd::DoRestore()
{
    m_event->SetValue(m_oldValue);
}

ShiftChildCmd::ShiftChildCmd(PObjectBase object, int pos)
{
    m_object = object;
    PObjectBase parent = object->GetParent();

    assert(parent);

    m_oldPos = parent->GetChildPosition(object);
    m_newPos = pos;
}

void ShiftChildCmd::DoExecute()
{
    if (m_oldPos != m_newPos) {
        PObjectBase parent(m_object->GetParent());
        parent->ChangeChildPosition(m_object, m_newPos);
    }
}

void ShiftChildCmd::DoRestore()
{
    if (m_oldPos != m_newPos) {
        PObjectBase parent(m_object->GetParent());
        parent->ChangeChildPosition(m_object, m_oldPos);
    }
}

CutObjectCmd::CutObjectCmd(ApplicationData* data, PObjectBase object)
{
    m_data = data;
    m_object = object;
    m_parent = object->GetParent();
    m_oldPos = m_parent->GetChildPosition(object);
    m_oldSelected = data->GetSelectedObject();
}

void CutObjectCmd::DoExecute()
{
#if 0
    // guardamos el clipboard ???
    m_clipboard = m_data->GetClipboardObject();
#endif
    m_data->SetClipboardObject(m_object);
    m_parent->RemoveChild(m_object);
    m_object->SetParent(PObjectBase());
    m_data->DetermineObjectToSelect(m_parent, m_oldPos);
}

void CutObjectCmd::DoRestore()
{
    // reubicamos el objeto donde estaba
    m_parent->AddChild(m_object);
    m_object->SetParent(m_parent);
    m_parent->ChangeChildPosition(m_object, m_oldPos);

    // restauramos el clipboard
#if 0
    m_data->SetClipboardObject(m_clipboard);
#endif
    m_data->SetClipboardObject(PObjectBase());
    m_data->SelectObject(m_oldSelected, true, false);
}

ReparentObjectCmd ::ReparentObjectCmd(PObjectBase sizeritem, PObjectBase sizer)
{
    m_sizeritem = sizeritem;
    m_sizer = sizer;
    m_oldSizer = m_sizeritem->GetParent();
    m_oldPosition = m_oldSizer->GetChildPosition(sizeritem);
}

void ReparentObjectCmd::DoExecute()
{
    m_oldSizer->RemoveChild(m_sizeritem);
    m_sizeritem->SetParent(m_sizer);
    m_sizer->AddChild(m_sizeritem);
}

void ReparentObjectCmd::DoRestore()
{
    m_sizer->RemoveChild(m_sizeritem);
    m_sizeritem->SetParent(m_oldSizer);
    m_oldSizer->AddChild(m_sizeritem);
    m_oldSizer->ChangeChildPosition(m_sizeritem, m_oldPosition);
}

ApplicationData* ApplicationData::s_instance = nullptr;

ApplicationData* ApplicationData::Get(const wxString& rootdir)
{
    if (!s_instance)
        s_instance = new ApplicationData(rootdir);

    return s_instance;
}

void ApplicationData::Destroy()

{
    delete s_instance;

    s_instance = nullptr;
}

void ApplicationData::Initialize()
{
    ApplicationData* appData = ApplicationData::Get();
    appData->LoadApp();
    /*
        Use the color of a dominant text to determine if dark mode should be used.
        TODO: Depending on the used theme it is not clear which color that is,
        using the window text has given the best results so far.
    */
    const wxColour col = wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOWTEXT);
    const int lightness = (col.Red() * 299 + col.Green() * 587 + col.Blue() * 114) / 1000;
    appData->SetDarkMode(lightness > 127);
}

ApplicationData::ApplicationData(const wxString& rootdir)
    : m_fbpVerMajor(1)
    , m_fbpVerMinor(15)
    , m_objDb(new ObjectDatabase())
    , m_manager(new wxWeaverManager)
    , m_ipc(new wxWeaverIPC)
#ifdef wxWEAVER_DEBUG
    , m_log(nullptr)
    , m_debug(nullptr)
#endif
    , m_rootDir(rootdir)
    , m_modFlag(false)
    , m_warnOnAdditionsUpdate(true)
    , m_darkMode(false)
{
    m_objDb->SetXmlPath(m_rootDir + wxFILE_SEP_PATH + "xml" + wxFILE_SEP_PATH);

    m_objDb->SetIconPath(m_rootDir
                         + wxFILE_SEP_PATH + "resources"
                         + wxFILE_SEP_PATH + "icons" + wxFILE_SEP_PATH);

    m_objDb->SetPluginPath(m_rootDir + wxFILE_SEP_PATH
                           + "plugins" + wxFILE_SEP_PATH);

    // Support loading files from memory
    // Used to load the XRC preview, but could be useful elsewhere
    wxFileSystem::AddHandler(new wxMemoryFSHandler);

    // Support for loading files from archives
    wxFileSystem::AddHandler(new wxArchiveFSHandler);
    wxFileSystem::AddHandler(new wxFilterFSHandler);
}

ApplicationData::~ApplicationData()
{
#ifdef wxWEAVER_DEBUG
    delete wxLog::SetActiveTarget(m_log);
#endif
}

void ApplicationData::LoadApp()
{
    wxString bitmapPath = m_objDb->GetXmlPath() + "icons.xml";
    AppBitmaps::LoadBitmaps(bitmapPath, m_objDb->GetIconPath());
    m_objDb->LoadObjectTypes();
    m_objDb->LoadPlugins(m_manager);
}
#ifdef wxWEAVER_DEBUG
DebugWindow* ApplicationData::GetDebugWindow(wxWindow* parent)
{
    if (!m_debug) {
        m_debug = new DebugWindow(parent);
        m_log = wxLog::SetActiveTarget(new wxLogTextCtrl(m_debug));
        wxLogMessage("Started");
    }
    return m_debug;
}
#endif
wxDialog* ApplicationData::GetSettingsDialog(wxWindow* /*parent*/)
{
#if 0
    if (parent)
        return new DialogPrefs(parent); // TODO
#endif
    return nullptr;
}

PwxWeaverManager ApplicationData::GetManager()
{
    return m_manager;
}

PObjectBase ApplicationData::GetSelectedObject()
{
    return m_selObj;
}

PObjectBase ApplicationData::GetSelectedForm()
{
    if ((m_selObj->GetTypeName() == "form")
        || (m_selObj->GetTypeName() == "wizard")
        || (m_selObj->GetTypeName() == "menubar_form")
        || (m_selObj->GetTypeName() == "toolbar_form"))
        return m_selObj;
    else
        return m_selObj->FindParentForm();
}

PObjectBase ApplicationData::GetProjectData()
{
    return m_project;
}

void ApplicationData::BuildNameSet(PObjectBase obj, PObjectBase top,
                                   std::set<wxString>& nameSet)
{
    if (obj != top) {
        PProperty nameProp = top->GetProperty("name");
        if (nameProp)
            nameSet.insert(nameProp->GetValueAsString());
    }
    for (size_t i = 0; i < top->GetChildCount(); i++)
        BuildNameSet(obj, top->GetChild(i), nameSet);
}

void ApplicationData::ResolveNameConflict(PObjectBase obj)
{
    while (obj && obj->GetObjectInfo()->GetType()->IsItem()) {
        if (obj->GetChildCount() > 0)
            obj = obj->GetChild(0);
        else
            return;
    }
    PProperty nameProp = obj->GetProperty("name");
    if (!nameProp)
        return;

    // Save the original name for use later.
    wxString originalName = nameProp->GetValueAsString();

    // el nombre no puede estar repetido dentro del mismo form
#if 0
    PObjectBase top = obj->FindNearAncestor("form");
#endif
    PObjectBase top = obj->FindParentForm();
    if (!top)
        top = m_project; // the object is a form.

    // construimos el conjunto de nombres
    std::set<wxString> nameSet;
    BuildNameSet(obj, top, nameSet);

    // comprobamos si hay conflicto
    std::set<wxString>::iterator it = nameSet.find(originalName);
    int i = 0;
    wxString name = originalName; // The name that gets incremented.
    while (it != nameSet.end()) {
        i++;
        name = wxString::Format("%s%i", originalName.c_str(), i);
        it = nameSet.find(name);
    }
    nameProp->SetValue(name);
}

void ApplicationData::ResolveSubtreeNameConflicts(PObjectBase obj, PObjectBase topObj)
{
    if (!topObj) {
#if 0
        topObj = obj->FindNearAncestor("form");
#endif
        topObj = obj->FindParentForm();
        if (!topObj)
            topObj = m_project; // object is the project
    }
    // Ignore item objects
    while (obj && obj->GetObjectInfo()->GetType()->IsItem()) {
        if (obj->GetChildCount() > 0)
            obj = obj->GetChild(0);
        else
            return; // error
    }
    // Resolve a possible name conflict
    ResolveNameConflict(obj);

    // Recurse through all children
    for (size_t i = 0; i < obj->GetChildCount(); i++)
        ResolveSubtreeNameConflicts(obj->GetChild(i), topObj);
}

int ApplicationData::CalcPositionOfInsertion(PObjectBase selected, PObjectBase parent)
{
    int pos = -1;
    if (parent && selected) {
        PObjectBase parentSelected = selected->GetParent();

        while (parentSelected && parentSelected != parent) {
            selected = parentSelected;
            parentSelected = selected->GetParent();
        }
        if (parentSelected && parentSelected == parent)
            pos = parent->GetChildPosition(selected) + 1;
    }
    return pos;
}

void ApplicationData::RemoveEmptyItems(PObjectBase obj)
{
    if (!obj->GetObjectInfo()->GetType()->IsItem()) {
        bool emptyItem = true;
        // esto es un algoritmo ineficiente pero "seguro" con los índices
        while (emptyItem) {
            emptyItem = false;

            for (size_t i = 0; !emptyItem && i < obj->GetChildCount(); i++) {
                PObjectBase child = obj->GetChild(i);

                if (child->GetObjectInfo()->GetType()->IsItem()
                    && !child->GetChildCount()) {
                    obj->RemoveChild(child); // borramos el item
                    child->SetParent(PObjectBase());

                    emptyItem = true; // volvemos a recorrer
                    wxString msg;
                    msg.Printf("Empty item removed under %s",
                               obj->GetPropertyAsString("name").c_str());
                    wxLogWarning(msg);
                }
            }
        }
    }
    for (size_t i = 0; i < obj->GetChildCount(); i++)
        RemoveEmptyItems(obj->GetChild(i));
}

PObjectBase ApplicationData::SearchSizerInto(PObjectBase obj)
{
    PObjectBase theSizer;
    if (obj->GetObjectInfo()->IsSubclassOf("sizer")
        || obj->GetObjectInfo()->IsSubclassOf("gbsizer"))
        theSizer = obj;
    else {
        for (size_t i = 0; !theSizer && i < obj->GetChildCount(); i++)
            theSizer = SearchSizerInto(obj->GetChild(i));
    }
    return theSizer;
}

void ApplicationData::ExpandObject(PObjectBase obj, bool expand)
{
    PCommand command(new ExpandObjectCmd(obj, expand));
    Execute(command);

    PropagateExpansion(obj, expand, !expand); // Collapse also all children ...
    NotifyObjectExpanded(obj);
}

void ApplicationData::PropagateExpansion(PObjectBase obj, bool expand, bool up)
{
    if (obj) {
        if (up) {
            PObjectBase child;
            for (size_t i = 0; i < obj->GetChildCount(); i++) {
                child = obj->GetChild(i);
                PCommand command(new ExpandObjectCmd(child, expand));
                Execute(command);
                PropagateExpansion(child, expand, up);
            }
        } else {
            PropagateExpansion(obj->GetParent(), expand, up);
            PCommand command(new ExpandObjectCmd(obj, expand));
            Execute(command);
        }
    }
}

bool ApplicationData::SelectObject(PObjectBase obj,
                                   bool force /*= false*/, bool notify /*= true */)
{
    if ((obj == m_selObj) && !force)
        return false;

    m_selObj = obj;
    if (notify)
        NotifyObjectSelected(obj, force);

    return true;
}

void ApplicationData::CreateObject(wxString name)
{
    try {
        LogDebug("[ApplicationData::CreateObject] New " + name);
        PObjectBase old_selected = GetSelectedObject();
        PObjectBase parent = old_selected;
        PObjectBase obj;

        if (parent) {
            bool created = false;
            /*
                Para que sea más práctico, si el objeto no se puede crear debajo
                del objeto seleccionado vamos a intentarlo en el padre del seleccionado
                y seguiremos subiendo hasta que ya no podamos crear el objeto.
            */
            while (parent && !created) {
                // además, el objeto se insertará a continuación del objeto seleccionado
                obj = m_objDb->CreateObject(name.ToStdString(), parent);
                if (obj) {
                    int pos = CalcPositionOfInsertion(GetSelectedObject(), parent);
                    PCommand command(new InsertObjectCmd(this, obj, parent, pos));
                    Execute(command); //m_cmdProc.Execute(command);
                    created = true;
                    ResolveNameConflict(obj);
                } else {
                    /*
                        lo vamos a seguir intentando con el padre, pero cuidado,
                        el padre no puede ser un item!
                    */
                    parent = parent->GetParent();
                    while (parent && parent->GetObjectInfo()->GetType()->IsItem())
                        parent = parent->GetParent();
                }
            }
        }
        /*
            Seleccionamos el objeto, si este es un item entonces se selecciona
            el objeto contenido. ¿Tiene sentido tener un item debajo de un item?
        */
        while (obj && obj->GetObjectInfo()->GetType()->IsItem())
            obj = (obj->GetChildCount() > 0 ? obj->GetChild(0) : PObjectBase());

        NotifyObjectCreated(obj);
        if (obj)
            SelectObject(obj, true, true);
        else
            SelectObject(old_selected, true, true);

    } catch (wxWeaverException& ex) {
        wxLogError(ex.what());
    }
}

void ApplicationData::RemoveObject(PObjectBase obj)
{
    DoRemoveObject(obj, false);
}

void ApplicationData::CutObject(PObjectBase obj)
{
    DoRemoveObject(obj, true);
}

void ApplicationData::DoRemoveObject(PObjectBase obj, bool cutObject)
{
    // Note: When removing an object it is important that the "item" objects
    // are not left behind
    PObjectBase parent = obj->GetParent();
    PObjectBase deleted_obj = obj;
    if (parent) {
        // Get the top item
        while (parent && parent->GetObjectInfo()->GetType()->IsItem()) {
            obj = parent;
            parent = obj->GetParent();
        }
        if (cutObject) {
            m_copyOnPaste = false;
            PCommand command(new CutObjectCmd(this, obj));
            Execute(command);
        } else {
            PCommand command(new RemoveObjectCmd(this, obj));
            Execute(command);
        }
        NotifyObjectRemoved(deleted_obj);
        SelectObject(GetSelectedObject(), true, true);
    } else {
        if (obj->GetTypeName() != "project")
            assert(false);
    }
    CheckProjectTree(m_project);
}

void ApplicationData::DetermineObjectToSelect(PObjectBase parent, size_t pos)
{
    // get position of next control or last control
    PObjectBase objToSelect;
    size_t count = parent->GetChildCount();
    if (!count) {
        objToSelect = parent;
    } else {
        pos = (pos < count ? pos : count - 1);
        objToSelect = parent->GetChild(pos);
    }
    while (objToSelect && objToSelect->GetObjectInfo()->GetType()->IsItem())
        objToSelect = objToSelect->GetChild(0);

    SelectObject(objToSelect);
}

void ApplicationData::CopyObjectToClipboard(PObjectBase obj)
{
    // Write some text to the clipboard
    // Do not call Open() when the clipboard is opened
    if (!wxTheClipboard->IsOpened()) {
        if (!wxTheClipboard->Open())
            return;
    }
    // This data objects are held by the clipboard,
    // so do not delete them in the app.
    wxTheClipboard->SetData(new wxWeaverDataObject(obj));
    wxTheClipboard->Close();
}

bool ApplicationData::PasteObjectFromClipboard(PObjectBase parent)
{
    // Do not call Open() when the clipboard is opened
    if (!wxTheClipboard->IsOpened()) {
        if (!wxTheClipboard->Open())
            return false;
    }
    if (wxTheClipboard->IsSupported(wxWeaverDataObjectFormat)) {
        wxWeaverDataObject data;
        if (wxTheClipboard->GetData(data)) {
            PObjectBase obj = data.GetObj();
            if (obj) {
                wxTheClipboard->Close();
                return PasteObject(parent, obj);
            }
        }
    }
    wxTheClipboard->Close();
    return false;
}

bool ApplicationData::CanPasteObjectFromClipboard()
{
    // Do not call Open() when the clipboard is opened
    if (!wxTheClipboard->IsOpened()) {
        if (!wxTheClipboard->Open())
            return false;
    }
    bool canPaste = wxTheClipboard->IsSupported(wxWeaverDataObjectFormat);
    if (wxTheClipboard->IsOpened())
        wxTheClipboard->Close();

    return canPaste;
}

void ApplicationData::CopyObject(PObjectBase obj)
{
    m_copyOnPaste = true;
    /*
        Make a copy of the object on the clipboard,
        otherwise modifications to the object after the copy
        will also be made on the clipboard.
    */
    m_clipboard = m_objDb->CopyObject(obj);
    CheckProjectTree(m_project);
}

bool ApplicationData::PasteObject(PObjectBase parent, PObjectBase objToPaste)
{
    try {
        PObjectBase clipboard;
        if (objToPaste) {
            clipboard = objToPaste;
        } else if (m_clipboard) {
            if (m_copyOnPaste)
                clipboard = m_objDb->CopyObject(m_clipboard);
            else
                clipboard = m_clipboard;
        }
        if (!clipboard)
            return false;

        // Remove parent/child relationship from clipboard object
        PObjectBase clipParent = clipboard->GetParent();
        if (clipParent) {
            clipParent->RemoveChild(clipboard);
            clipboard->SetParent(PObjectBase());
        }
        /*
            Vamos a hacer un pequeño truco, intentaremos crear un objeto nuevo
            del mismo tipo que el guardado en m_clipboard debajo de parent.
            El objeto devuelto quizá no sea de la misma clase que m_clipboard debido
            a que esté incluido dentro de un "item".
            Por tanto, si el objeto devuelto es no-nulo, entonces vamos a descender
            en el arbol hasta que el objeto sea de la misma clase que m_clipboard,
            momento en que cambiaremos dicho objeto por m_clipboard.

            Ejemplo:

             m_clipboard :: wxButton
             parent      :: wxBoxSizer

             obj = CreateObject(m_clipboard->GetObjectInfo()->GetClassName(), parent)

             obj :: sizeritem
                         /
             wxButton   <- Cambiamos este por m_clipboard
        */
        PObjectBase old_parent = parent;
        PObjectBase obj = m_objDb->CreateObject(
            clipboard->GetObjectInfo()->GetClassName().ToStdString(), parent);

        // If the object is already contained in an item,
        // we may need to get the object out of the first item before pasting
        if (!obj) {
            PObjectBase tempItem = clipboard;
            while (tempItem->GetObjectInfo()->GetType()->IsItem()) {
                tempItem = tempItem->GetChild(0);
                if (!tempItem)
                    break;

                obj = m_objDb->CreateObject(
                    tempItem->GetObjectInfo()->GetClassName().ToStdString(), parent);

                if (obj) {
                    clipboard = tempItem;
                    break;
                }
            }
        }
        int pos = -1;
        if (!obj) {
            // si no se ha podido crear el objeto vamos a intentar crearlo colgado
            // del padre de "parent" y además vamos a insertarlo en la posición
            // siguiente a "parent"
            PObjectBase selected = parent;
            parent = selected->GetParent();

            while (parent && parent->GetObjectInfo()->GetType()->IsItem()) {
                selected = parent;
                parent = selected->GetParent();
            }
            if (parent) {
                obj = m_objDb->CreateObject(
                    clipboard->GetObjectInfo()->GetClassName().ToStdString(), parent);

                if (obj)
                    pos = CalcPositionOfInsertion(selected, parent);
            }
        }
        if (!obj)
            return false;

        PObjectBase aux = obj;

        while (aux && aux->GetObjectInfo() != clipboard->GetObjectInfo())
            aux = (aux->GetChildCount() > 0 ? aux->GetChild(0) : PObjectBase());

        if (aux && aux != obj) {
            // sustituimos aux por clipboard
            PObjectBase auxParent = aux->GetParent();
            auxParent->RemoveChild(aux);
            aux->SetParent(PObjectBase());

            auxParent->AddChild(clipboard);
            clipboard->SetParent(auxParent);
        } else
            obj = clipboard;

        // y finalmente insertamos en el arbol
        PCommand command(new InsertObjectCmd(this, obj, parent, pos));

        Execute(command);

        if (!m_copyOnPaste)
            m_clipboard.reset();

        ResolveSubtreeNameConflicts(obj);
        NotifyProjectRefresh();

        // vamos a mantener seleccionado el nuevo objeto creado
        // pero hay que tener en cuenta que es muy probable que el objeto creado
        // sea un "item"
        while (obj && obj->GetObjectInfo()->GetType()->IsItem()) {
            assert(obj->GetChildCount() > 0);
            obj = obj->GetChild(0);
        }
        SelectObject(obj, true, true);

        CheckProjectTree(m_project);
    } catch (wxWeaverException& ex) {
        wxLogError(ex.what());
        return false;
    }
    return true;
}

void ApplicationData::InsertObject(PObjectBase obj, PObjectBase parent)
{
#if 0
    // FIXME! comprobar obj se puede colgar de parent
    if (parent->GetObjectInfo()->GetType()->FindChildType(
            obj->GetObjectInfo()->GetType())) {
#endif
    PCommand command(new InsertObjectCmd(this, obj, parent));
    Execute(command); //m_cmdProc.Execute(command);
    NotifyProjectRefresh();
#if 0
    }
#endif
}

void ApplicationData::MergeProject(PObjectBase project)
{
    // FIXME! comprobar obj se puede colgar de parent
    for (size_t i = 0; i < project->GetChildCount(); i++) {
#if 0
        m_project->AddChild(project->GetChild(i));
        project->GetChild(i)->SetParent(m_project);
#endif
        PObjectBase child = project->GetChild(i);
        RemoveEmptyItems(child);

        InsertObject(child, m_project);
    }
    // Merge bitmaps and icons properties
    PObjectBase thisProject = GetProjectData();
    PProperty prop = thisProject->GetProperty("bitmaps");
    if (prop) {
        wxString value = prop->GetValueAsString();
        value.Trim();
        value << " " << project->GetPropertyAsString("bitmaps");
        prop->SetValue(value);
    }
    prop = thisProject->GetProperty("icons");
    if (prop) {
        wxString value = prop->GetValueAsString();
        value.Trim();
        value << " " << project->GetPropertyAsString("icons");
        prop->SetValue(value);
    }
    NotifyProjectRefresh();
}

void ApplicationData::ModifyProperty(PProperty prop, wxString str)
{
#if 0
    // TODO: what is this for?
    ObjectBase object = prop->GetObject();
#endif
    if (str != prop->GetValueAsString()) {
        PCommand command(new ModifyPropertyCmd(prop, str));
        Execute(command); //m_cmdProc.Execute(command);

        NotifyPropertyModified(prop);
    }
}

void ApplicationData::ModifyEventHandler(PEvent evt, wxString value)
{
#if 0
    // TODO: what is this for?
    PObjectBase object = evt->GetObject();
#endif
    if (value != evt->GetValue()) {
        PCommand command(new ModifyEventHandlerCmd(evt, value));
        Execute(command); //m_cmdProc.Execute(command);

        NotifyEventHandlerModified(evt);
    }
}

void ApplicationData::SaveProject(const wxString& filename)
{
    // Make sure this file is not already open
    if (!m_ipc->VerifySingleInstance(filename, false)) {
        if (wxMessageBox(
                _("You cannot save over a file that is currently open in another instance.\n"
                  "Would you like to switch to that instance?"),
                _("Open in Another Instance"),
                wxICON_QUESTION | wxYES_NO, wxTheApp->GetTopWindow())
            == wxYES)
            m_ipc->VerifySingleInstance(filename, true);

        return;
    }
    try {
        ticpp::Document doc;
        m_project->Serialize(&doc);
        doc.SaveFile(std::string(filename.mb_str(wxConvFile)));

        m_projectFile = filename;
        SetProjectPath(::wxPathOnly(filename));
        m_modFlag = false;
        m_cmdProc.SetSavePoint();
        NotifyProjectSaved();
    } catch (ticpp::Exception& ex) {
        wxString message = ex.m_details;
        wxWEAVER_THROW_EX(message)
    }
}

bool ApplicationData::LoadProject(const wxString& file, bool justGenerate)

{
    LogDebug("LOADING");

    if (!wxFileName::FileExists(file)) {
        wxLogError("This file does not exist: %s", file.c_str());
        return false;
    }
    if (!justGenerate) {
        if (!m_ipc->VerifySingleInstance(file))
            return false;
    }
    try {
        ticpp::Document doc;
        XMLUtils::LoadXMLFile(doc, false, file);

        ticpp::Element* root = doc.FirstChildElement();

        m_objDb->ResetObjectCounters();

        int fbpVerMajor = 0;
        int fbpVerMinor = 0;

        if (root->Value() != std::string("object")) {
            try {
                ticpp::Element* fileVersion = root->FirstChildElement("FileVersion");
                fileVersion->GetAttributeOrDefault("major", &fbpVerMajor, 0);
                fileVersion->GetAttributeOrDefault("minor", &fbpVerMinor, 0);
            } catch (ticpp::Exception&) {
            }
        }
        bool older = false;
        bool newer = false;

        if (m_fbpVerMajor == fbpVerMajor) {
            older = (fbpVerMinor < m_fbpVerMinor);
            newer = (fbpVerMinor > m_fbpVerMinor);
        } else {
            older = (fbpVerMajor < m_fbpVerMajor);
            newer = (fbpVerMajor > m_fbpVerMajor);
        }
        if (newer) {
            if (justGenerate) {
                wxLogError(
                    "This project file is newer than this version of wxWeaver.\n");
            } else {
                wxMessageBox(
                    _("This project file is newer than this version of wxWeaver.\n"
                      "It cannot be opened.\n\n"
                      "Please download an updated version from https://wxweaver.github.io"),
                    _("New Version"), wxICON_ERROR);
            }
            return false;
        }
        if (older) {
            if (justGenerate) {
                wxLogError(
                    "This project file is out of date.  Update your .fbp before using --generate");
                return false;
            }
            wxMessageBox(
                _("This project file is using an older file format, it will be updated during loading.\n\n"
                  "WARNING: Saving the project will update the format of the project file on disk!"),
                _("Older file format"));

            if (ConvertProject(doc, file, fbpVerMajor, fbpVerMinor)) {
                // Document has changed -- reacquire the root node
                root = doc.FirstChildElement();
            } else {
                wxLogError("Unable to convert project");
                return false;
            }
        }
        ticpp::Element* object = root->FirstChildElement("object");
        PObjectBase proj;

        try {
            proj = m_objDb->CreateObject(object);
        } catch (wxWeaverException& ex) {
            wxLogError(ex.what());
            return false;
        }
        if (proj && proj->GetTypeName() == "project") {
            PObjectBase old_proj = m_project;
            m_project = proj;
            m_selObj = m_project;
            // Set the modification to true if the project was older and has been converted
            m_modFlag = older;
            m_cmdProc.Reset();
            m_projectFile = file;
            SetProjectPath(::wxPathOnly(file));
            NotifyProjectLoaded();
            NotifyProjectRefresh();
        }
    } catch (ticpp::Exception& ex) {
        wxLogError(wxString(ex.m_details));
        return false;
    }
    return true;
}

bool ApplicationData::ConvertProject(ticpp::Document& doc, const wxString& path,
                                     int fileMajor, int fileMinor)
{
    try {
        XMLUtils::LoadXMLFile(doc, false, path);

        ticpp::Element* root = doc.FirstChildElement();
        if (root->Value() == std::string("object")) {
            ConvertProjectProperties(root, path, fileMajor, fileMinor);
            ConvertObject(root, fileMajor, fileMinor);

            // Create a clone of now-converted object tree, so it can be linked
            // underneath the root element
            std::unique_ptr<ticpp::Node> objectTree = root->Clone();

            // Clear the document to add the declaration and the root element
            doc.Clear();

            // Add the declaration
            doc.LinkEndChild(new ticpp::Declaration("1.0", "UTF-8", "yes"));

            // Add the root element, with file version
            ticpp::Element* newRoot = new ticpp::Element("wxFormBuilder_Project");
            ticpp::Element* fileVersion = new ticpp::Element("FileVersion");
            fileVersion->SetAttribute("major", m_fbpVerMajor);
            fileVersion->SetAttribute("minor", m_fbpVerMinor);

            newRoot->LinkEndChild(fileVersion);

            // Add the object tree
            newRoot->LinkEndChild(objectTree.release());

            doc.LinkEndChild(newRoot);
        } else {
            // Handle project separately because it only occurs once
            ticpp::Element* project = root->FirstChildElement("object");
            ConvertProjectProperties(project, path, fileMajor, fileMinor);
            ConvertObject(project, fileMajor, fileMinor);
            ticpp::Element* fileVersion = root->FirstChildElement("FileVersion");
            fileVersion->SetAttribute("major", m_fbpVerMajor);
            fileVersion->SetAttribute("minor", m_fbpVerMinor);
        }
    } catch (ticpp::Exception& ex) {
        wxLogError(wxString(ex.m_details));
        return false;
    }
    return true;
}

void ApplicationData::ConvertProjectProperties(ticpp::Element* project,
                                               const wxString& path,
                                               int fileMajor, int fileMinor)

{
    // Ensure that this is the "project" element
    std::string objClass;
    project->GetAttribute("class", &objClass);

    if (objClass != "Project")
        return;

    // Reusable sets for finding properties
    std::set<std::string> oldProps;
    std::set<ticpp::Element*> newProps;

    if (fileMajor < 1 || (1 == fileMajor && fileMinor < 5)) {
        // Find the user_headers property
        oldProps.insert("user_headers");
        GetPropertiesToConvert(project, oldProps, &newProps);

        std::string user_headers;
        if (!newProps.empty()) {
            user_headers = (*newProps.begin())->GetText(false);
            project->RemoveChild(*newProps.begin());
        }
        if (!user_headers.empty()) {
            wxString msg = _("The \"user_headers\" property has been removed.\n");
            msg += _("Its purpose was to provide a place to include precompiled headers or\n");
            msg += _("headers for subclasses.\n");
            msg += _("There is now a \"precompiled_header\" property and a \"header\" subitem\n");
            msg += _("on the subclass property.\n\n");
            msg += _("Would you like the current value of the \"user_headers\" property to be saved\n");
            msg += _("to a file so that you can distribute the headers among the \"precompiled_header\"\n");
            msg += _("and \"subclass\" properties\?");

            if (wxMessageBox(
                    msg,
                    _("The \"user_headers\" property has been removed"),
                    wxICON_QUESTION | wxYES_NO | wxYES_DEFAULT,
                    wxTheApp->GetTopWindow())
                == wxYES) {
                wxString name;
                wxFileName::SplitPath(path, nullptr, nullptr, &name, nullptr);
                wxFileDialog dialog(
                    wxTheApp->GetTopWindow(),
                    _("Save \"user_headers\""), ::wxPathOnly(path),
                    name + "_user_headers.txt",
                    _("All files") + " (*.*)|*.*", wxFD_SAVE);

                if (dialog.ShowModal() == wxID_OK) {
                    wxString wxuser_headers = user_headers;
                    wxString filename = dialog.GetPath();
                    bool success = false;
                    wxFFile output(filename, "w");
                    if (output.IsOpened()) {
                        if (output.Write(wxuser_headers)) {
                            output.Close();
                            success = true;
                        }
                    }
                    if (!success) {
                        wxLogError(
                            "Unable to open %s for writing.\nUser Headers:\n%s",
                            filename.c_str(), wxuser_headers.c_str());
                    }
                }
            }
        }
    }
    /*
        The pch property is now the exact code to be generated, not just the header filename.
        The goal of this conversion block is to determine which of two possible pch blocks to use
        The pch block that wxWeaver generated changed in version 1.6
    */
    if (fileMajor < 1 || (fileMajor == 1 && fileMinor < 8)) {
        oldProps.clear();
        newProps.clear();
        oldProps.insert("precompiled_header");
        GetPropertiesToConvert(project, oldProps, &newProps);

        if (!newProps.empty()) {
            std::string pch = (*newProps.begin())->GetText(false);
            if (!pch.empty()) {
                if (fileMajor < 1 || (1 == fileMajor && fileMinor < 6)) {
                    // use the older block
                    (*newProps.begin())->SetText("#include \"" + pch + "\""
                                                                       "\n"
                                                                       "\n#ifdef __BORLANDC__"
                                                                       "\n#pragma hdrstop"
                                                                       "\n#endif //__BORLANDC__"
                                                                       "\n"
                                                                       "\n#ifndef WX_PRECOMP"
                                                                       "\n#include <wx/wx.h>"
                                                                       "\n#endif //WX_PRECOMP");
                } else {
                    // use the newer block
                    (*newProps.begin())->SetText("#ifdef WX_PRECOMP"
                                                 "\n"
                                                 "\n#include \""
                                                 + pch + "\""
                                                         "\n"
                                                         "\n#ifdef __BORLANDC__"
                                                         "\n#pragma hdrstop"
                                                         "\n#endif //__BORLANDC__"
                                                         "\n"
                                                         "\n#else"
                                                         "\n#include <wx/wx.h>"
                                                         "\n#endif //WX_PRECOMP");
                }
            }
        }
    }
    // The format of string list properties changed in version 1.9
    if (fileMajor < 1 || (fileMajor == 1 && fileMinor < 9)) {
        oldProps.clear();
        newProps.clear();
        oldProps.insert("namespace");
        oldProps.insert("bitmaps");
        oldProps.insert("icons");
        GetPropertiesToConvert(project, oldProps, &newProps);

        std::set<ticpp::Element*>::iterator prop;
        for (prop = newProps.begin(); prop != newProps.end(); ++prop) {
            std::string value = (*prop)->GetText(false);
            if (!value.empty()) {
                wxArrayString array = TypeConv::OldStringToArrayString(value);
                (*prop)->SetText(TypeConv::ArrayStringToString(array).ToStdString());
            }
        }
    }
    // event_handler moved to the forms in version 1.10
    if (fileMajor < 1 || (fileMajor == 1 && fileMinor < 10)) {
        oldProps.clear();
        newProps.clear();
        oldProps.insert("event_handler");
        GetPropertiesToConvert(project, oldProps, &newProps);

        if (!newProps.empty()) {
            ticpp::Iterator<ticpp::Element> object("object");
            for (object = project->FirstChildElement("object", false);
                 object != object.end(); ++object)
                object->LinkEndChild((*newProps.begin())->Clone().get());

            project->RemoveChild(*newProps.begin());
        }
    }
}

void ApplicationData::ConvertObject(ticpp::Element* parent, int fileMajor, int fileMinor)
{
    ticpp::Iterator<ticpp::Element> object("object");

    for (object = parent->FirstChildElement("object", false);
         object != object.end(); ++object)
        ConvertObject(object.Get(), fileMajor, fileMinor);

    // Reusable sets to find properties with
    std::set<std::string> oldProps;
    std::set<ticpp::Element*> newProps;
    std::set<ticpp::Element*>::iterator newProp;

    std::string objClass; // Get the class of the object

    parent->GetAttribute("class", &objClass);

    // The changes below will convert an unversioned file to version 1.3
    if (fileMajor < 1 || (fileMajor == 1 && fileMinor < 3)) {
        // The property 'option' became 'proportion'
        if (objClass == "sizeritem"
            || objClass == "gbsizeritem"
            || objClass == "spacer") {
            oldProps.clear();
            newProps.clear();
            oldProps.insert("option");
            GetPropertiesToConvert(parent, oldProps, &newProps);

            if (!newProps.empty())
                (*newProps.begin())->SetAttribute("name", "proportion"); // One in, one out
        }
        /*
            The 'style' property used to have both wxWindow styles
            and the styles of the specific controls now it only has the styles
            of the specific controls, and wxWindow styles are saved in window_style
            This also applies to 'extra_style', which was once combined with 'style'.
            And they were named 'WindowStyle' and one point, too...
        */
        std::set<wxString> windowStyles;
        windowStyles.insert("wxSIMPLE_BORDER");
        windowStyles.insert("wxDOUBLE_BORDER");
        windowStyles.insert("wxSUNKEN_BORDER");
        windowStyles.insert("wxRAISED_BORDER");
        windowStyles.insert("wxSTATIC_BORDER");
        windowStyles.insert("wxNO_BORDER");
        windowStyles.insert("wxTRANSPARENT_WINDOW");
        windowStyles.insert("wxTAB_TRAVERSAL");
        windowStyles.insert("wxWANTS_CHARS");
        windowStyles.insert("wxVSCROLL");
        windowStyles.insert("wxHSCROLL");
        windowStyles.insert("wxALWAYS_SHOW_SB");
        windowStyles.insert("wxCLIP_CHILDREN");
        windowStyles.insert("wxFULL_REPAINT_ON_RESIZE");

        // Transfer the window styles
        oldProps.clear();
        newProps.clear();

        oldProps.insert("style");
        oldProps.insert("WindowStyle");

        GetPropertiesToConvert(parent, oldProps, &newProps);

        for (newProp = newProps.begin(); newProp != newProps.end(); ++newProp)
            TransferOptionList(*newProp, &windowStyles, "window_style");

        std::set<wxString> extraWindowStyles;
        extraWindowStyles.insert("wxWS_EX_VALIDATE_RECURSIVELY");
        extraWindowStyles.insert("wxWS_EX_BLOCK_EVENTS");
        extraWindowStyles.insert("wxWS_EX_TRANSIENT");
        extraWindowStyles.insert("wxWS_EX_PROCESS_IDLE");
        extraWindowStyles.insert("wxWS_EX_PROCESS_UI_UPDATES");

        // Transfer the window extra styles
        oldProps.clear();
        newProps.clear();

        oldProps.insert("style");
        oldProps.insert("extra_style");
        oldProps.insert("WindowStyle");

        GetPropertiesToConvert(parent, oldProps, &newProps);

        for (newProp = newProps.begin(); newProp != newProps.end(); ++newProp)
            TransferOptionList(*newProp, &extraWindowStyles, "window_extra_style");
    }
    // The file is now at least version 1.3
    if (fileMajor < 1 || (fileMajor == 1 && fileMinor < 4)) {
        if (objClass == "wxCheckList") {
            /*
                The class we once named "wxCheckList" really represented
                a "wxCheckListBox", now that we use the #class macro in
                code generation, it generates the wrong code
            */
            parent->SetAttribute("class", "wxCheckListBox");
        }
    }
    // The file is now at least version 1.4
    if (fileMajor < 1 || (fileMajor == 1 && fileMinor < 6)) {
        if (objClass == "spacer") {
            /*
                spacer used to be represented by its own class,
                it is now under a sizeritem like everything else.
                No need to check for a wxGridBagSizer,
                because it was introduced at the same time.
                The goal is to change the class to sizeritem,
                then create a spacer child, then move "width" and "height"
                to the spacer.
            */
            parent->SetAttribute("class", "sizeritem");
            ticpp::Element spacer("object");
            spacer.SetAttribute("class", "spacer");

            oldProps.clear();
            newProps.clear();
            oldProps.insert("width");
            GetPropertiesToConvert(parent, oldProps, &newProps);

            if (!newProps.empty()) {
                // One in, one out
                ticpp::Element* width = *newProps.begin();
                spacer.LinkEndChild(width->Clone().release());
                parent->RemoveChild(width);
            }

            oldProps.clear();
            newProps.clear();
            oldProps.insert("height");
            GetPropertiesToConvert(parent, oldProps, &newProps);

            if (!newProps.empty()) {
                // One in, one out
                ticpp::Element* height = *newProps.begin();
                spacer.LinkEndChild(height->Clone().release());
                parent->RemoveChild(height);
            }
            parent->LinkEndChild(&spacer);
        }
    }
    /*
        The file is now at least version 1.6
        Version 1.7 now stores all font properties.
        The font property conversion is automatic
        because it is just an extension of the old values.
    */
    if (fileMajor < 1 || (fileMajor == 1 && fileMinor < 7)) {
        // Remove deprecated 2.6 things
        // wxDialog styles wxTHICK_FRAME and wxNO_3D
        if (objClass == "Dialog") {
            oldProps.clear();
            newProps.clear();
            oldProps.insert("style");
            GetPropertiesToConvert(parent, oldProps, &newProps);

            if (!newProps.empty()) {
                ticpp::Element* style = *newProps.begin();
                wxString styles = style->GetText(false);
                if (!styles.empty()) {
                    if (TypeConv::FlagSet("wxTHICK_FRAME", styles)) {
                        styles = TypeConv::ClearFlag("wxTHICK_FRAME", styles);
                        styles = TypeConv::SetFlag("wxRESIZE_BORDER", styles);
                    }
                    styles = TypeConv::ClearFlag("wxNO_3D", styles);
                    style->SetText(styles.ToStdString());
                }
            }
        }
    }
    /*
        The file is now at least version 1.7.
        The update to 1.8 only affected project properties.
        See ConvertProjectProperties.
        The file is now at least version 1.8
        stringlist properties are stored in a different format as of version 1.9
    */
    if (fileMajor < 1 || (fileMajor == 1 && fileMinor < 9)) {
        oldProps.clear();
        newProps.clear();

        if (objClass == "wxComboBox"
            || objClass == "wxChoice"
            || objClass == "wxListBox"
            || objClass == "wxRadioBox"
            || objClass == "wxCheckListBox") {
            oldProps.insert("choices");
        } else if (objClass == "wxGrid") {
            oldProps.insert("col_label_values");
            oldProps.insert("row_label_values");
        }
        if (!oldProps.empty()) {
            GetPropertiesToConvert(parent, oldProps, &newProps);

            std::set<ticpp::Element*>::iterator prop;
            for (prop = newProps.begin(); prop != newProps.end(); ++prop) {
                std::string value = (*prop)->GetText(false);
                if (!value.empty()) {
                    wxArrayString array = TypeConv::OldStringToArrayString(value);
                    (*prop)->SetText(TypeConv::ArrayStringToString(array).ToStdString());
                }
            }
        }
    }
    /*
        The file is now at least version 1.9
        Version 1.11 now stores bitmap property in the following format:
        'source'; 'data' instead of old form 'data'; 'source'.
    */
    if (fileMajor < 1 || (fileMajor == 1 && fileMinor < 11)) {
        oldProps.clear();
        newProps.clear();
        oldProps.insert("bitmap");
        GetPropertiesToConvert(parent, oldProps, &newProps);

        std::set<ticpp::Element*>::iterator prop;
        for (prop = newProps.begin(); prop != newProps.end(); ++prop) {
            ticpp::Element* bitmap = *prop;
            wxString image = bitmap->GetText(false);
            if (!image.empty()) {
                if (image.AfterLast(';').Contains("Load From")) {
                    wxString source = image.AfterLast(';').Trim().Trim(false);
                    wxString data = image.BeforeLast(';').Trim().Trim(false);
                    bitmap->SetText(wxString(source + "; " + data).ToStdString());
                }
            }
        }
#if 0
        oldProps.clear();
        newProps.clear();
        oldProps.insert("choices");
        GetPropertiesToConvert(parent, oldProps, &newProps);
        for (prop = newProps.begin(); prop != newProps.end(); ++prop) {
            ticpp::Element* choices = *prop;
            wxString content = choices->GetText(false);
            if (!content.empty()) {
                content.Replace("\" \"", ";");
                content.Replace("\"", "");
                choices->SetText(content.ToStdString());
            }
        }
#endif
    }
    // The file is now at least version 1.11
    if (fileMajor < 1 || (fileMajor == 1 && fileMinor < 12)) {
        bool classUpdated = false;
        if (objClass == "wxScintilla") {
            objClass = "wxStyledTextCtrl";
            parent->SetAttribute("class", objClass);
            classUpdated = true;
        }
        if (objClass == "wxTreeListCtrl") {
            objClass = "wxadditions::wxTreeListCtrl";
            parent->SetAttribute("class", objClass);
            classUpdated = true;
        }
        if (objClass == "wxTreeListCtrlColumn") {
            objClass = "wxadditions::wxTreeListCtrlColumn";
            parent->SetAttribute("class", objClass);
            classUpdated = true;
        }
        if (m_warnOnAdditionsUpdate && classUpdated) {
            m_warnOnAdditionsUpdate = false;
            wxLogWarning(
                "Updated classes from wxAdditions. "
                "You must use the latest version of wxAdditions to continue.\n"
                "Note wxScintilla is now wxStyledListCtrl, "
                "wxTreeListCtrl is now wxadditions::wxTreeListCtrl, "
                "and wxTreeListCtrlColumn is now wxadditions::wxTreeListCtrlColumn");
        }
        typedef std::map<std::string, std::set<std::string>> PropertiesToRemove;

        static std::set<std::string> propertyRemovalWarnings;
        const PropertiesToRemove& propertiesToRemove = GetPropertiesToRemove_v1_12();
        PropertiesToRemove::const_iterator it = propertiesToRemove.find(objClass);
        if (it != propertiesToRemove.end()) {
            RemoveProperties(parent, it->second);
            if (!propertyRemovalWarnings.count(objClass)) {
                std::stringstream ss;
                std::ostream_iterator<std::string> out_it(ss, ", ");
                std::copy(it->second.begin(), it->second.end(), out_it);
                wxLogMessage(
                    "Removed properties for class %s "
                    "because they are no longer supported: %s",
                    objClass, ss.str());
                propertyRemovalWarnings.insert(objClass);
            }
        }
    }
    // The file is now at least version 1.12
    // TODO: Dont know where Version 1.13 comes from, so this is for Version 1.14
    if (fileMajor < 1 || (fileMajor == 1 && fileMinor < 14)) {
        // Rename all wx*_BORDER-Styles to wxBORDER_*-Styles and remove wxDOUBLE_BORDER
        oldProps.clear();
        newProps.clear();
        oldProps.insert("style");
        oldProps.insert("window_style");
        GetPropertiesToConvert(parent, oldProps, &newProps);

        for (newProp = newProps.begin(); newProp != newProps.end(); ++newProp) {
            wxString styles = (*newProp)->GetText(false);
            if (!styles.empty()) {
                if (TypeConv::FlagSet("wxSIMPLE_BORDER", styles)) {
                    styles = TypeConv::ClearFlag("wxSIMPLE_BORDER", styles);
                    styles = TypeConv::SetFlag("wxBORDER_SIMPLE", styles);
                }
                if (TypeConv::FlagSet("wxDOUBLE_BORDER", styles))
                    styles = TypeConv::ClearFlag("wxDOUBLE_BORDER", styles);

                if (TypeConv::FlagSet("wxSUNKEN_BORDER", styles)) {
                    styles = TypeConv::ClearFlag("wxSUNKEN_BORDER", styles);
                    styles = TypeConv::SetFlag("wxBORDER_SUNKEN", styles);
                }
                if (TypeConv::FlagSet("wxRAISED_BORDER", styles)) {
                    styles = TypeConv::ClearFlag("wxRAISED_BORDER", styles);
                    styles = TypeConv::SetFlag("wxBORDER_RAISED", styles);
                }
                if (TypeConv::FlagSet("wxSTATIC_BORDER", styles)) {
                    styles = TypeConv::ClearFlag("wxSTATIC_BORDER", styles);
                    styles = TypeConv::SetFlag("wxBORDER_STATIC", styles);
                }
                if (TypeConv::FlagSet("wxNO_BORDER", styles)) {
                    styles = TypeConv::ClearFlag("wxNO_BORDER", styles);
                    styles = TypeConv::SetFlag("wxBORDER_NONE", styles);
                }
                (*newProp)->SetText(styles.ToStdString());
            }
        }
        // wxBitmapButton: Remove wxBU_AUTODRAW and rename properties
        // selected -> pressed, hover -> current
        if ("wxBitmapButton" == objClass) {
            oldProps.clear();
            newProps.clear();
            oldProps.insert("style");
            GetPropertiesToConvert(parent, oldProps, &newProps);

            if (!newProps.empty()) {
                ticpp::Element* style = *newProps.begin();
                wxString styles = style->GetText(false);
                if (!styles.empty()) {
                    if (TypeConv::FlagSet("wxBU_AUTODRAW", styles))
                        styles = TypeConv::ClearFlag("wxBU_AUTODRAW", styles);

                    style->SetText(styles.ToStdString());
                }
            }
            oldProps.clear();
            newProps.clear();
            oldProps.insert("selected");
            GetPropertiesToConvert(parent, oldProps, &newProps);

            if (!newProps.empty())
                (*newProps.begin())->SetAttribute("name", "pressed");

            oldProps.clear();
            newProps.clear();
            oldProps.insert("hover");
            GetPropertiesToConvert(parent, oldProps, &newProps);

            if (!newProps.empty())
                (*newProps.begin())->SetAttribute("name", "current");
        }
        // wxStaticText: Rename wxALIGN_CENTRE -> wxALIGN_CENTER_HORIZONTAL
        else if ("wxStaticText" == objClass) {
            oldProps.clear();
            newProps.clear();
            oldProps.insert("style");
            GetPropertiesToConvert(parent, oldProps, &newProps);

            if (!newProps.empty()) {
                ticpp::Element* style = *newProps.begin();
                wxString styles = style->GetText(false);
                if (!styles.empty()) {
                    if (TypeConv::FlagSet("wxALIGN_CENTRE", styles)) {
                        styles = TypeConv::ClearFlag("wxALIGN_CENTRE", styles);
                        styles = TypeConv::SetFlag("wxALIGN_CENTER_HORIZONTAL", styles);
                    }
                    style->SetText(styles.ToStdString());
                }
            }
        }
        // wxRadioBox: Remove wxRA_USE_CHECKBOX
        else if (objClass == "wxRadioBox") {
            oldProps.clear();
            newProps.clear();
            oldProps.insert("style");
            GetPropertiesToConvert(parent, oldProps, &newProps);

            if (!newProps.empty()) {
                ticpp::Element* style = *newProps.begin();
                wxString styles = style->GetText(false);
                if (!styles.empty()) {
                    if (TypeConv::FlagSet("wxRA_USE_CHECKBOX", styles))
                        styles = TypeConv::ClearFlag("wxRA_USE_CHECKBOX", styles);

                    style->SetText(styles.ToStdString());
                }
            }
        }

        // wxRadioButton: Remove wxRB_USE_CHECKBOX
        else if (objClass == "wxRadioButton") {
            oldProps.clear();
            newProps.clear();
            oldProps.insert("style");
            GetPropertiesToConvert(parent, oldProps, &newProps);

            if (!newProps.empty()) {
                ticpp::Element* style = *newProps.begin();
                wxString styles = style->GetText(false);
                if (!styles.empty()) {
                    if (TypeConv::FlagSet("wxRB_USE_CHECKBOX", styles))
                        styles = TypeConv::ClearFlag("wxRB_USE_CHECKBOX", styles);

                    style->SetText(styles.ToStdString());
                }
            }
        }
        // wxStatusBar: Rename wxST_SIZEGRIP -> wxSTB_SIZEGRIP
        else if (objClass == "wxStatusBar") {
            oldProps.clear();
            newProps.clear();
            oldProps.insert("style");
            GetPropertiesToConvert(parent, oldProps, &newProps);

            if (!newProps.empty()) {
                ticpp::Element* style = *newProps.begin();
                wxString styles = style->GetText(false);
                if (!styles.empty()) {
                    if (TypeConv::FlagSet("wxST_SIZEGRIP", styles)) {
                        styles = TypeConv::ClearFlag("wxST_SIZEGRIP", styles);
                        styles = TypeConv::SetFlag("wxSTB_SIZEGRIP", styles);
                    }
                    style->SetText(styles.ToStdString());
                }
            }
        }
        // wxMenuBar: Remove wxMB_DOCKABLE
        else if (objClass == "wxMenuBar") {
            oldProps.clear();
            newProps.clear();
            oldProps.insert("style");
            GetPropertiesToConvert(parent, oldProps, &newProps);

            if (!newProps.empty()) {
                ticpp::Element* style = *newProps.begin();
                wxString styles = style->GetText(false);
                if (!styles.empty()) {
                    if (TypeConv::FlagSet("wxMB_DOCKABLE", styles))
                        styles = TypeConv::ClearFlag("wxMB_DOCKABLE", styles);

                    style->SetText(styles.ToStdString());
                }
            }
        }
    }
    // The file is now at least version 1.14
    if (fileMajor < 1 || (fileMajor == 1 && fileMinor < 15)) {
        // Rename wxTE_CENTRE -> wxTE_CENTER
        if (objClass == "wxTextCtrl" || objClass == "wxSearchCtrl") {
            oldProps.clear();
            newProps.clear();
            oldProps.insert("style");
            GetPropertiesToConvert(parent, oldProps, &newProps);

            if (!newProps.empty()) {
                ticpp::Element* style = *newProps.begin();
                wxString styles = style->GetText(false);
                if (!styles.empty()) {
                    if (TypeConv::FlagSet("wxTE_CENTRE", styles)) {
                        styles = TypeConv::ClearFlag("wxTE_CENTRE", styles);
                        styles = TypeConv::SetFlag("wxTE_CENTER", styles);
                    }
                    style->SetText(styles.ToStdString());
                }
            }
        }
        // Rename wxALIGN_CENTRE -> wxALIGN_CENTER
        else if (objClass == "wxGrid") {
            oldProps.clear();
            newProps.clear();
            oldProps.insert("col_label_horiz_alignment");
            oldProps.insert("col_label_vert_alignment");
            oldProps.insert("row_label_horiz_alignment");
            oldProps.insert("row_label_vert_alignment");
            oldProps.insert("cell_horiz_alignment");
            oldProps.insert("cell_vert_alignment");
            GetPropertiesToConvert(parent, oldProps, &newProps);

            for (newProp = newProps.begin(); newProp != newProps.end(); ++newProp) {
                wxString styles = (*newProp)->GetText(false);
                if (!styles.empty()) {
                    if (TypeConv::FlagSet("wxALIGN_CENTRE", styles)) {
                        styles = TypeConv::ClearFlag("wxALIGN_CENTRE", styles);
                        styles = TypeConv::SetFlag("wxALIGN_CENTER", styles);
                    }
                    (*newProp)->SetText(styles.ToStdString());
                }
            }
        }
        // wxNotebook: Remove wxNB_FLAT
        else if (objClass == "wxNotebook") {
            oldProps.clear();
            newProps.clear();
            oldProps.insert("style");
            GetPropertiesToConvert(parent, oldProps, &newProps);

            if (!newProps.empty()) {
                ticpp::Element* style = *newProps.begin();
                wxString styles = style->GetText(false);
                if (!styles.empty()) {
                    if (TypeConv::FlagSet("wxNB_FLAT", styles))
                        styles = TypeConv::ClearFlag("wxNB_FLAT", styles);

                    style->SetText(styles.ToStdString());
                }
            }
        }
    }
    // The file is now at least version 1.15
    if (fileMajor < 1 || (fileMajor == 1 && fileMinor < 16)) {
        static bool showRemovalWarnings = true;
        if (objClass == "wxMenuBar") {
            RemoveProperties(parent, std::set<std::string> { "label" });
            if (showRemovalWarnings) {
                wxLogMessage(
                    "Removed property label for class wxMenuBar because it is no longer used");
                showRemovalWarnings = false;
            }
        }
    }
}

void ApplicationData::GetPropertiesToConvert(ticpp::Node* parent,
                                             const std::set<std::string>& names,
                                             std::set<ticpp::Element*>* properties)
{
    properties->clear(); // Clear result set

    ticpp::Iterator<ticpp::Element> prop("property");
    for (prop = parent->FirstChildElement("property", false);
         prop != prop.end(); ++prop) {
        std::string name;
        prop->GetAttribute("name", &name);

        if (names.find(name) != names.end())
            properties->insert(prop.Get());
    }
}

void ApplicationData::RemoveProperties(ticpp::Node* parent, const std::set<std::string>& names)
{
    ticpp::Iterator<ticpp::Element> prop("property");
    for (prop = parent->FirstChildElement("property", false); prop != prop.end();) {
        ticpp::Element element = *prop;
        ++prop;

        std::string name;
        element.GetAttribute("name", &name);

        if (names.find(name) != names.end())
            parent->RemoveChild(&element);
    }
}

void ApplicationData::TransferOptionList(ticpp::Element* prop,
                                         std::set<wxString>* options,
                                         const std::string& newPropName)
{
    wxString value = prop->GetText(false);
    std::set<wxString> transfer;
    std::set<wxString> keep;

    // Sort options; if in the 'options' set, they should be transferred
    // to a property named 'newPropName' otherwise, they should stay
    wxStringTokenizer tkz(value, "|", wxTOKEN_RET_EMPTY_ALL);
    while (tkz.HasMoreTokens()) {
        wxString option = tkz.GetNextToken();
        option.Trim(false);
        option.Trim(true);

        if (options->find(option) != options->end())
            transfer.insert(option); // Needs to be transferred
        else
            keep.insert(option); // Should be kept
    }
    // Reusable sets to find properties with
    std::set<std::string> oldProps;
    std::set<ticpp::Element*> newProps;

    // If there are any to transfer, add to the target property, or make a new one
    ticpp::Node* parent = prop->Parent();

    if (!transfer.empty()) {
        // Check for the target property
        ticpp::Element* newProp;
        wxString newOptionList;

        oldProps.clear();
        oldProps.insert(newPropName);
        GetPropertiesToConvert(parent, oldProps, &newProps);

        std::unique_ptr<ticpp::Element> tmpProp;
        if (!newProps.empty()) {
            newProp = *newProps.begin();
            newOptionList << "|" << newProp->GetText(false);
        } else {
            tmpProp = std::make_unique<ticpp::Element>("property");
            newProp = tmpProp.get();
            newProp->SetAttribute("name", newPropName);
        }
        std::set<wxString>::iterator option;
        for (option = transfer.begin(); option != transfer.end(); ++option)
            newOptionList << "|" << *option;

        newProp->SetText(newOptionList.substr(1).ToStdString());

        if (newProps.empty())
            parent->InsertBeforeChild(prop, *newProp);
    }
    // Set the value of the property to whatever is left
    if (keep.empty()) {
        parent->RemoveChild(prop);
    } else {
        std::set<wxString>::iterator option;
        wxString newOptionList;
        for (option = keep.begin(); option != keep.end(); ++option)
            newOptionList << "|" << *option;

        prop->SetText(newOptionList.substr(1).ToStdString());
    }
}

void ApplicationData::NewProject()

{
    m_project = m_objDb->CreateObject("Project");
    m_selObj = m_project;
    m_modFlag = false;
    m_cmdProc.Reset();
    m_projectFile = "";
    SetProjectPath("");
    m_ipc->Reset();
    NotifyProjectRefresh();
}

void ApplicationData::GenerateCode(bool panelOnly, bool noDelayed)
{
    NotifyCodeGeneration(panelOnly, !noDelayed);
}

void ApplicationData::NotifyPreferencesChanged(wxWeaverPreferencesEvent& event)
{
    NotifyEvent(event);
}

void ApplicationData::GenerateInheritedClass(PObjectBase form, wxString className,
                                             wxString path, wxString file)
{
    try {
        PObjectBase project = GetProjectData();
        if (!project) {
            wxLogWarning("No Project?!");
            return;
        }
        if (!::wxDirExists(path)) {
            wxLogWarning("Invalid Path: %s", path.c_str());
            return;
        }
        PObjectBase obj = m_objDb->CreateObject("UserClasses", PObjectBase());

        PProperty baseNameProp = obj->GetProperty("basename");
        PProperty nameProp = obj->GetProperty("name");
        PProperty fileProp = obj->GetProperty("file");
        PProperty genfileProp = obj->GetProperty("gen_file");
        PProperty typeProp = obj->GetProperty("type");
        PProperty pchProp = obj->GetProperty("precompiled_header");

        if (!(baseNameProp && nameProp && fileProp && typeProp && genfileProp && pchProp)) {
            wxLogWarning("Missing Property");
            return;
        }
        wxFileName inherFile(file);
        if (!inherFile.MakeAbsolute(path)) {
            wxLogWarning("Unable to make \"%s\" absolute to \"%s\"",
                         file.c_str(), path.c_str());
            return;
        }
        const wxString& genFileValue = project->GetPropertyAsString("file");
        wxFileName genFile(genFileValue);
        if (!genFile.MakeAbsolute(path)) {
            wxLogWarning("Unable to make \"%s\" absolute to \"%s\"",
                         genFileValue.c_str(), path.c_str());
            return;
        }
        const wxString& genFileFullPath = genFile.GetFullPath();
        if (!genFile.MakeRelativeTo(inherFile.GetPath(wxPATH_GET_VOLUME))) {
            wxLogWarning("Unable to make \"%s\" relative to \"%s\"",
                         genFileFullPath.c_str(),
                         inherFile.GetPath(wxPATH_GET_VOLUME).c_str());
            return;
        }
        baseNameProp->SetValue(form->GetPropertyAsString("name"));
        nameProp->SetValue(className);
        fileProp->SetValue(inherFile.GetName());
        genfileProp->SetValue(genFile.GetFullPath());
        typeProp->SetValue(form->GetClassName());

        PProperty pchValue = project->GetProperty("precompiled_header");
        if (pchValue)
            pchProp->SetValue(pchValue->GetValueAsString());

        // Determine if Microsoft BOM should be used
        bool useMicrosoftBOM = false;
        PProperty pUseMicrosoftBOM = project->GetProperty("use_microsoft_bom");
        if (pUseMicrosoftBOM)
            useMicrosoftBOM = (pUseMicrosoftBOM->GetValueAsInteger());

        // Determine if Utf8 or Ansi is to be created
        bool useUtf8 = false;
        PProperty pUseUtf8 = project->GetProperty("encoding");
        if (pUseUtf8)
            useUtf8 = (pUseUtf8->GetValueAsString() != "ANSI");

        PProperty pCodeGen = project->GetProperty("code_generation");
        if (pCodeGen && TypeConv::FlagSet("C++", pCodeGen->GetValueAsString())) {
            CppCodeGenerator codegen;
            const wxString& fullPath = inherFile.GetFullPath();
            codegen.ParseFiles(fullPath + ".h", fullPath + ".cpp");

            PCodeWriter h_cw(
                new FileCodeWriter(fullPath + ".h", useMicrosoftBOM, useUtf8));

            PCodeWriter cpp_cw(
                new FileCodeWriter(fullPath + ".cpp", useMicrosoftBOM, useUtf8));

            codegen.SetHeaderWriter(h_cw);
            codegen.SetSourceWriter(cpp_cw);
            codegen.GenerateInheritedClass(obj, form);
        } else if (pCodeGen && TypeConv::FlagSet("Python", pCodeGen->GetValueAsString())) {
            PythonCodeGenerator codegen;
            const wxString& fullPath = inherFile.GetFullPath();
            PCodeWriter python_cw(new FileCodeWriter(fullPath + ".py",
                                                     useMicrosoftBOM, useUtf8));
            codegen.SetSourceWriter(python_cw);
            codegen.GenerateInheritedClass(obj, form);
        } else if (pCodeGen && TypeConv::FlagSet("PHP", pCodeGen->GetValueAsString())) {
            PHPCodeGenerator codegen;
            const wxString& fullPath = inherFile.GetFullPath();
            PCodeWriter php_cw(
                new FileCodeWriter(fullPath + ".php", useMicrosoftBOM, useUtf8));

            codegen.SetSourceWriter(php_cw);
            codegen.GenerateInheritedClass(obj, form);
        } else if (pCodeGen && TypeConv::FlagSet("Lua", pCodeGen->GetValueAsString())) {
            LuaCodeGenerator codegen;
            const wxString& fullPath = inherFile.GetFullPath();
            PCodeWriter lua_cw(
                new FileCodeWriter(fullPath + ".lua", useMicrosoftBOM, useUtf8));

            codegen.SetSourceWriter(lua_cw);
            codegen.GenerateInheritedClass(obj, form, genFileFullPath);
        }

        wxLogStatus("Class generated at \'%s\'.", path.c_str());
    } catch (wxWeaverException& ex) {
        wxLogError(ex.what());
    }
}

void ApplicationData::MovePosition(PObjectBase obj, bool right, size_t num)
{
    PObjectBase noItemObj = obj;
    PObjectBase parent = obj->GetParent();

    if (parent) {
        // Si el objeto está incluido dentro de un item hay que desplazar el item
        while (parent && parent->GetObjectInfo()->GetType()->IsItem()) {
            obj = parent;
            parent = obj->GetParent();
        }
        size_t pos = parent->GetChildPosition(obj);

        // nos aseguramos de que los límites son correctos
        size_t childrenCount = parent->GetChildCount();
        if ((right && num + pos < childrenCount) || (!right && (num <= pos))) {
            pos = (right ? pos + num : pos - num);

            PCommand command(new ShiftChildCmd(obj, pos));
            Execute(command); //m_cmdProc.Execute(command);
            NotifyProjectRefresh();
            SelectObject(noItemObj, true);
        }
    }
}

void ApplicationData::MoveHierarchy(PObjectBase obj, bool up)
{
    PObjectBase sizeritem = obj->GetParent();
    if (!(sizeritem && sizeritem->GetObjectInfo()->IsSubclassOf("sizeritembase")))
        return;

    PObjectBase nextSizer = sizeritem->GetParent(); // points to the object's sizer
    if (nextSizer) {
        if (up) {
            do {
                nextSizer = nextSizer->GetParent();
            } while (nextSizer && !nextSizer->GetObjectInfo()->IsSubclassOf("sizer")
                     && !nextSizer->GetObjectInfo()->IsSubclassOf("gbsizer"));

            if (nextSizer
                && (nextSizer->GetObjectInfo()->IsSubclassOf("sizer")
                    || nextSizer->GetObjectInfo()->IsSubclassOf("gbsizer"))) {
                PCommand cmdReparent(new ReparentObjectCmd(sizeritem, nextSizer));
                Execute(cmdReparent);
                NotifyProjectRefresh();
                SelectObject(obj, true);
            }
        } else {
            // object will be move to the top sizer of the next sibling object subtree.
            size_t pos = nextSizer->GetChildPosition(sizeritem) + 1;
            if (pos < nextSizer->GetChildCount()) {
                nextSizer = SearchSizerInto(nextSizer->GetChild(pos));
                if (nextSizer) {
                    PCommand cmdReparent(new ReparentObjectCmd(sizeritem, nextSizer));
                    Execute(cmdReparent);
                    NotifyProjectRefresh();
                    SelectObject(obj, true);
                }
            }
        }
    }
}

void ApplicationData::Undo()
{
    m_cmdProc.Undo();
    m_modFlag = !m_cmdProc.IsAtSavePoint();
    NotifyProjectRefresh();
    CheckProjectTree(m_project);
    NotifyObjectSelected(GetSelectedObject());
}

void ApplicationData::Redo()
{
    m_cmdProc.Redo();
    m_modFlag = !m_cmdProc.IsAtSavePoint();
    NotifyProjectRefresh();
    CheckProjectTree(m_project);
    NotifyObjectSelected(GetSelectedObject());
}

void ApplicationData::ToggleExpandLayout(PObjectBase obj)
{
    if (!obj)
        return;

    PObjectBase parent = obj->GetParent();
    if (!parent)
        return;

    if (!parent->GetObjectInfo()->IsSubclassOf("sizeritembase"))
        return;

    PProperty propFlag = parent->GetProperty("flag");

    if (!propFlag)
        return;

    wxString currentValue = propFlag->GetValueAsString();
    wxString value
        = (TypeConv::FlagSet("wxEXPAND", currentValue)
               ? TypeConv::ClearFlag("wxEXPAND", currentValue)
               : TypeConv::SetFlag("wxEXPAND", currentValue));

    ModifyProperty(propFlag, value);
}

void ApplicationData::ToggleStretchLayout(PObjectBase obj)
{
    if (!obj)
        return;

    PObjectBase parent = obj->GetParent();
    if (!parent)
        return;

    if (parent->GetTypeName() != "sizeritem"
        && parent->GetTypeName() != "gbsizeritem")
        return;

    PProperty proportion = parent->GetProperty("proportion");
    if (!proportion)
        return;

    wxString value = (proportion->GetValueAsString() != "0" ? "0" : "1");
    ModifyProperty(proportion, value);
}

void ApplicationData::CheckProjectTree(PObjectBase obj)
{
    assert(obj);

    for (size_t i = 0; i < obj->GetChildCount(); i++) {
        PObjectBase child = obj->GetChild(i);
        if (child->GetParent() != obj)
            wxLogError(wxString::Format(
                "Parent of object \'"
                + child->GetPropertyAsString("name") + "\' is wrong!"));

        CheckProjectTree(child);
    }
}

bool ApplicationData::GetLayoutSettings(PObjectBase obj, int* flag, int* option,
                                        int* border, int* orient)
{
    if (!obj)
        return false;

    PObjectBase parent = obj->GetParent();
    if (!parent)
        return false;

    if (parent->GetObjectInfo()->IsSubclassOf("sizeritembase")) {
        PProperty propOption = parent->GetProperty("proportion");
        if (propOption)
            *option = propOption->GetValueAsInteger();

        *flag = parent->GetPropertyAsInteger("flag");
        *border = parent->GetPropertyAsInteger("border");

        PObjectBase sizer = parent->GetParent();
        if (sizer) {
            wxString parentName = sizer->GetClassName();
            if (parentName == "wxBoxSizer" || parentName == "wxStaticBoxSizer") {
                PProperty propOrient = sizer->GetProperty("orient");
                if (propOrient)
                    *orient = propOrient->GetValueAsInteger();
            }
        }
        return true;
    }
    return false;
}

void ApplicationData::ChangeAlignment(PObjectBase obj, int align, bool vertical)
{
    if (!obj)
        return;

    PObjectBase parent = obj->GetParent();
    if (!parent)
        return;

    if (!parent->GetObjectInfo()->IsSubclassOf("sizeritembase"))
        return;

    PProperty propFlag = parent->GetProperty("flag");
    if (!propFlag)
        return;

    wxString value = propFlag->GetValueAsString();

    // Primero borramos los flags de la configuración previa, para así
    // evitar conflictos de alineaciones.
    if (vertical) {
        value = TypeConv::ClearFlag("wxALIGN_TOP", value);
        value = TypeConv::ClearFlag("wxALIGN_BOTTOM", value);
        value = TypeConv::ClearFlag("wxALIGN_CENTER_VERTICAL", value);
    } else {
        value = TypeConv::ClearFlag("wxALIGN_LEFT", value);
        value = TypeConv::ClearFlag("wxALIGN_RIGHT", value);
        value = TypeConv::ClearFlag("wxALIGN_CENTER_HORIZONTAL", value);
    }
    wxString alignStr;
    switch (align) {
    case wxALIGN_RIGHT:
        alignStr = "wxALIGN_RIGHT";
        break;

    case wxALIGN_CENTER_HORIZONTAL:
        alignStr = "wxALIGN_CENTER_HORIZONTAL";
        break;

    case wxALIGN_BOTTOM:
        alignStr = "wxALIGN_BOTTOM";
        break;

    case wxALIGN_CENTER_VERTICAL:
        alignStr = "wxALIGN_CENTER_VERTICAL";
        break;
    }
    value = TypeConv::SetFlag(alignStr, value);
    ModifyProperty(propFlag, value);
}

void ApplicationData::ToggleBorderFlag(PObjectBase obj, int border)
{
    if (!obj)
        return;

    PObjectBase parent = obj->GetParent();
    if (!parent)
        return;

    if (!parent->GetObjectInfo()->IsSubclassOf("sizeritembase"))
        return;

    PProperty propFlag = parent->GetProperty("flag");
    if (!propFlag)
        return;

    wxString value = propFlag->GetValueAsString();
    value = TypeConv::ClearFlag("wxALL", value);
    value = TypeConv::ClearFlag("wxTOP", value);
    value = TypeConv::ClearFlag("wxBOTTOM", value);
    value = TypeConv::ClearFlag("wxRIGHT", value);
    value = TypeConv::ClearFlag("wxLEFT", value);

    int intVal = propFlag->GetValueAsInteger();
    intVal ^= border;

    if ((intVal & wxALL) == wxALL)
        value = TypeConv::SetFlag("wxALL", value);
    else {
        if ((intVal & wxTOP))
            value = TypeConv::SetFlag("wxTOP", value);

        if ((intVal & wxBOTTOM))
            value = TypeConv::SetFlag("wxBOTTOM", value);

        if ((intVal & wxRIGHT))
            value = TypeConv::SetFlag("wxRIGHT", value);

        if ((intVal & wxLEFT))
            value = TypeConv::SetFlag("wxLEFT", value);
    }
    ModifyProperty(propFlag, value);
}

void ApplicationData::CreateBoxSizerWithObject(PObjectBase obj)
{
    PObjectBase parent = obj->GetParent();
    if (!parent)
        return;

    PObjectBase grandParent = parent->GetParent();
    if (!grandParent)
        return;

    int childPos = -1;
    if (parent->GetObjectInfo()->IsSubclassOf("sizeritembase")) {
        childPos = (int)grandParent->GetChildPosition(parent);
        parent = grandParent;
    }
    // Must first cut the old object in case it is the only allowable object
    PObjectBase clipboard = m_clipboard;
    CutObject(obj);

    // Create the wxBoxSizer
    PObjectBase newSizer = m_objDb->CreateObject("wxBoxSizer", parent);

    if (newSizer) {
        PCommand cmd(new InsertObjectCmd(this, newSizer, parent, childPos));
        Execute(cmd);

        if (newSizer->GetTypeName() == "sizeritem")
            newSizer = newSizer->GetChild(0);

        PasteObject(newSizer);
        m_clipboard = clipboard;

        //NotifyProjectRefresh();
    } else {
        Undo();
        m_clipboard = clipboard;
    }
}

void ApplicationData::ShowXrcPreview()
{
    PObjectBase form = GetSelectedForm();

    if (!form) {
        wxMessageBox(_("Please select a form and try again."),
                     _("XRC Preview"), wxICON_ERROR);
        return;
    } else if (form->GetPropertyAsInteger("aui_managed")) {
        wxMessageBox(
            _("XRC preview doesn't support AUI-managed frames."),
            _("XRC Preview"), wxICON_ERROR);
        return;
    }
    XRCPreview::Show(form, GetProjectPath());
}

bool ApplicationData::CanPasteObject()
{
    PObjectBase obj = GetSelectedObject();
    if (obj && obj->GetTypeName() != "project")
        return (m_clipboard != nullptr);

    return false;
}

bool ApplicationData::CanCopyObject()
{
    PObjectBase obj = GetSelectedObject();
    if (obj && obj->GetTypeName() != "project")
        return true;

    return false;
}

bool ApplicationData::IsModified()
{
    return m_modFlag;
}

void ApplicationData::SetDarkMode(bool darkMode)
{
    m_darkMode = darkMode;
}

bool ApplicationData::IsDarkMode() const
{
    return m_darkMode;
}

void ApplicationData::Execute(PCommand cmd)
{
    m_modFlag = true;
    m_cmdProc.Execute(cmd);
}

void ApplicationData::AddHandler(wxEvtHandler* handler)
{
    m_handlers.push_back(handler);
}

void ApplicationData::RemoveHandler(wxEvtHandler* handler)
{
    for (HandlerVector::iterator it = m_handlers.begin();
         it != m_handlers.end(); ++it) {
        if (*it == handler) {
            m_handlers.erase(it);
            break;
        }
    }
}

void ApplicationData::NotifyEvent(wxWeaverEvent& event, bool forcedelayed)
{
    if (!forcedelayed) {
        LogDebug("event: %s", event.GetEventName().c_str());

        for (std::vector<wxEvtHandler*>::iterator handler = m_handlers.begin();
             handler != m_handlers.end(); handler++)
            (*handler)->ProcessEvent(event);

    } else {
        LogDebug("Pending event: %s", event.GetEventName().c_str());

        for (std::vector<wxEvtHandler*>::iterator handler = m_handlers.begin();
             handler != m_handlers.end(); handler++)
            (*handler)->AddPendingEvent(event);
    }
}

void ApplicationData::NotifyProjectLoaded()
{
    wxWeaverEvent event(wxEVT_WVR_PROJECT_LOADED);
    NotifyEvent(event);
}

void ApplicationData::NotifyProjectSaved()
{
    wxWeaverEvent event(wxEVT_WVR_PROJECT_SAVED);
    NotifyEvent(event);
}

void ApplicationData::NotifyObjectExpanded(PObjectBase obj)
{
    wxWeaverObjectEvent event(wxEVT_WVR_OBJECT_EXPANDED, obj);
    NotifyEvent(event);
}

void ApplicationData::NotifyObjectSelected(PObjectBase obj, bool force)
{
    wxWeaverObjectEvent event(wxEVT_WVR_OBJECT_SELECTED, obj);
    if (force)
        event.SetString("force");

    NotifyEvent(event, false);
}

void ApplicationData::NotifyObjectCreated(PObjectBase obj)
{
    wxWeaverObjectEvent event(wxEVT_WVR_OBJECT_CREATED, obj);
    NotifyEvent(event, false);
}

void ApplicationData::NotifyObjectRemoved(PObjectBase obj)
{
    wxWeaverObjectEvent event(wxEVT_WVR_OBJECT_REMOVED, obj);
    NotifyEvent(event, false);
}

void ApplicationData::NotifyPropertyModified(PProperty prop)
{
    wxWeaverPropertyEvent event(wxEVT_WVR_PROPERTY_MODIFIED, prop);
    NotifyEvent(event);
}

void ApplicationData::NotifyEventHandlerModified(PEvent evtHandler)
{
    wxWeaverEventHandlerEvent event(wxEVT_WVR_EVENT_HANDLER_MODIFIED, evtHandler);
    NotifyEvent(event);
}

void ApplicationData::NotifyCodeGeneration(bool panelOnly, bool forcedelayed)
{
    wxWeaverEvent event(wxEVT_WVR_CODE_GENERATION);
    // Using the previously unused Id field in the event to carry a boolean
    event.SetId((panelOnly ? 1 : 0));
    NotifyEvent(event, forcedelayed);
}

void ApplicationData::NotifyProjectRefresh()
{
    wxWeaverEvent event(wxEVT_WVR_PROJECT_REFRESH);
    NotifyEvent(event);
}

bool ApplicationData::VerifySingleInstance(const wxString& file, bool switchTo)
{
    return m_ipc->VerifySingleInstance(file, switchTo);
}

wxString ApplicationData::GetPathProperty(const wxString& pathName)
{
    PObjectBase project = GetProjectData();
    wxFileName path;
    // Get the output path
    PProperty ppath = project->GetProperty(pathName);

    if (ppath) {
        wxString pathEntry = ppath->GetValueAsString();

        if (pathEntry.empty()) {
            wxWEAVER_THROW_EX(
                "You must set the \"" + pathName
                + "\" property of the project to a valid path for output files");
        }
        path = wxFileName::DirName(pathEntry);
        if (!path.IsAbsolute()) {
            wxString projectPath = AppData()->GetProjectPath();
            if (projectPath.empty())
                wxWEAVER_THROW_EX(
                    "You must save the project when using a relative path for output files");

            path = wxFileName(projectPath + wxFileName::GetPathSeparator() + pathEntry + wxFileName::GetPathSeparator());
            path.Normalize();
#if 0
            // this approach is probably incorrect if the fb project is located under a symlink
            path.SetCwd(projectPath);
            path.MakeAbsolute();
#endif
        }
    }
    if (!path.DirExists())
        wxWEAVER_THROW_EX(
            "Invalid Path: "
            << path.GetPath()
            << "\nYou must set the \""
                + pathName + "\" property of the project to a valid path for output files");

    return path.GetPath(wxPATH_GET_VOLUME | wxPATH_GET_SEPARATOR);
}

wxString ApplicationData::GetOutputPath()
{
    return GetPathProperty("path");
}

wxString ApplicationData::GetEmbeddedFilesOutputPath()
{
    return GetPathProperty("embedded_files_path");
}

ApplicationData::PropertiesToRemove& ApplicationData::GetPropertiesToRemove_v1_12() const
{
    static PropertiesToRemove propertiesToRemove;
    if (propertiesToRemove.empty()) {
        propertiesToRemove["Dialog"].insert("BottomDockable");
        propertiesToRemove["Dialog"].insert("LeftDockable");
        propertiesToRemove["Dialog"].insert("RightDockable");
        propertiesToRemove["Dialog"].insert("TopDockable");
        propertiesToRemove["Dialog"].insert("caption_visible");
        propertiesToRemove["Dialog"].insert("center_pane");
        propertiesToRemove["Dialog"].insert("close_button");
        propertiesToRemove["Dialog"].insert("default_pane");
        propertiesToRemove["Dialog"].insert("dock");
        propertiesToRemove["Dialog"].insert("dock_fixed");
        propertiesToRemove["Dialog"].insert("docking");
        propertiesToRemove["Dialog"].insert("floatable");
        propertiesToRemove["Dialog"].insert("gripper");
        propertiesToRemove["Dialog"].insert("maximize_button");
        propertiesToRemove["Dialog"].insert("minimize_button");
        propertiesToRemove["Dialog"].insert("moveable");
        propertiesToRemove["Dialog"].insert("pane_border");
        propertiesToRemove["Dialog"].insert("pin_button");
        propertiesToRemove["Dialog"].insert("resize");
        propertiesToRemove["Dialog"].insert("show");
        propertiesToRemove["Dialog"].insert("toolbar_pane");
        propertiesToRemove["Dialog"].insert("validator_style");
        propertiesToRemove["Dialog"].insert("validator_type");
        propertiesToRemove["Dialog"].insert("aui_name");

        propertiesToRemove["Panel"].insert("BottomDockable");
        propertiesToRemove["Panel"].insert("LeftDockable");
        propertiesToRemove["Panel"].insert("RightDockable");
        propertiesToRemove["Panel"].insert("TopDockable");
        propertiesToRemove["Panel"].insert("caption_visible");
        propertiesToRemove["Panel"].insert("center_pane");
        propertiesToRemove["Panel"].insert("close_button");
        propertiesToRemove["Panel"].insert("default_pane");
        propertiesToRemove["Panel"].insert("dock");
        propertiesToRemove["Panel"].insert("dock_fixed");
        propertiesToRemove["Panel"].insert("docking");
        propertiesToRemove["Panel"].insert("floatable");
        propertiesToRemove["Panel"].insert("gripper");
        propertiesToRemove["Panel"].insert("maximize_button");
        propertiesToRemove["Panel"].insert("minimize_button");
        propertiesToRemove["Panel"].insert("moveable");
        propertiesToRemove["Panel"].insert("pane_border");
        propertiesToRemove["Panel"].insert("pin_button");
        propertiesToRemove["Panel"].insert("resize");
        propertiesToRemove["Panel"].insert("show");
        propertiesToRemove["Panel"].insert("toolbar_pane");
        propertiesToRemove["Panel"].insert("validator_style");
        propertiesToRemove["Panel"].insert("validator_type");

        propertiesToRemove["wxStaticText"].insert("validator_style");
        propertiesToRemove["wxStaticText"].insert("validator_type");
        propertiesToRemove["CustomControl"].insert("validator_style");
        propertiesToRemove["CustomControl"].insert("validator_type");
        propertiesToRemove["wxAuiNotebook"].insert("validator_style");
        propertiesToRemove["wxAuiNotebook"].insert("validator_type");
        propertiesToRemove["wxPanel"].insert("validator_style");
        propertiesToRemove["wxPanel"].insert("validator_type");
        propertiesToRemove["wxToolBar"].insert("validator_style");
        propertiesToRemove["wxToolBar"].insert("validator_type");
        propertiesToRemove["wxStyledTextCtrl"].insert("use_wxAddition");
        propertiesToRemove["wxStyledTextCtrl"].insert("validator_style");
        propertiesToRemove["wxStyledTextCtrl"].insert("validator_type");
        propertiesToRemove["wxPropertyGridManager"].insert("use_wxAddition");
        propertiesToRemove["wxPropertyGridManager"].insert("validator_style");
        propertiesToRemove["wxPropertyGridManager"].insert("validator_type");

        propertiesToRemove["wxadditions::wxTreeListCtrl"].insert("validator_style");
        propertiesToRemove["wxadditions::wxTreeListCtrl"].insert("validator_type");
    }
    return propertiesToRemove;
}
