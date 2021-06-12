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

#include "model/database.h"
#include "cmdproc.h"

namespace ticpp {
class Document;
class Node;
class Element;
} // namespace ticpp

class Property;
class wxWeaverEvent;
class wxWeaverPrefsEditorEvent;
class wxWeaverManager;
class wxWeaverIPC;

#define AppData() (ApplicationData::Get())
#define AppDataCreate(path) (ApplicationData::Get(path))
#define AppDataInit() (ApplicationData::Initialize())
#define AppDataDestroy() (ApplicationData::Destroy())

extern const char* const VERSION;
extern const char* const REVISION;

#ifdef wxWEAVER_DEBUG
class DebugWindow;
#endif

/** Singleton Application Data.
*/
class ApplicationData {
public:
    ~ApplicationData();
    static ApplicationData* Get(const wxString& rootdir = ".");

    ApplicationData(const ApplicationData&) = delete;
    ApplicationData& operator=(const ApplicationData&) = delete;
    ApplicationData(ApplicationData&&) = delete;
    ApplicationData& operator=(ApplicationData&&) = delete;

    static void Initialize(); // Forces the static AppData instance to Init()
    static void Destroy();

    void LoadApp(); // Initialize application

    PwxWeaverManager GetManager(); // Hold a pointer to the wxWeaverManager

    void AddHandler(wxEvtHandler* handler);    // Procedures for register / unregister
    void RemoveHandler(wxEvtHandler* handler); // wxEvtHandlers to be notified of wxWeaverEvents

    // Operaciones sobre los datos
    bool LoadProject(const wxString& file, bool justGenerate = false);
    void SaveProject(const wxString& filename);
    void NewProject();

    /** Convert a project from an older version.

        @param path The path to the project file
        @param fileMajor The major revision of the file
        @param fileMinor The minor revision of the file

        @return true if successful, false otherwise
    */
    bool ConvertProject(ticpp::Document& doc, const wxString& path,
                        int fileMajor, int fileMinor);

    /** Recursive function used to convert the object tree in the project file
        to the latest version.

        @param object A pointer to the object element
        @param fileMajor The major revision of the file
        @param fileMinor The minor revision of the file
    */
    void ConvertObject(ticpp::Element* object, int fileMajor, int fileMinor);

    void ExpandObject(PObjectBase obj, bool expand);

    void DetermineObjectToSelect(PObjectBase parent, size_t pos);

    /** Object will not be selected if it already is selected,
        unless force == @true

        Returns true if selection changed, false if already selected
    */
    bool SelectObject(PObjectBase obj, bool force = false, bool notify = true);

    void CreateObject(wxString name);

    void RemoveObject(PObjectBase obj);

    void CutObject(PObjectBase obj);

    void CopyObject(PObjectBase obj);

    bool PasteObject(PObjectBase parent, PObjectBase objToPaste = PObjectBase());

    void CopyObjectToClipboard(PObjectBase obj);

    bool PasteObjectFromClipboard(PObjectBase parent);

    void InsertObject(PObjectBase obj, PObjectBase parent);

    void MergeProject(PObjectBase project);

    void ModifyProperty(PProperty prop, wxString value);

    void ModifyEventHandler(PEvent evt, wxString value);

    void GenerateCode(bool panelOnly = false, bool noDelayed = false);

    void NotifyEditorsPreferences(wxWeaverPrefsEditorEvent&);

    void GenerateInheritedClass(PObjectBase form, wxString className,
                                wxString path, wxString file);

    void MovePosition(PObjectBase, bool right, size_t num = 1);

    void MoveHierarchy(PObjectBase obj, bool up);

    void Undo();
    void Redo();

    void ToggleExpandLayout(PObjectBase obj);

    void ToggleStretchLayout(PObjectBase obj);

    void ChangeAlignment(PObjectBase obj, int align, bool vertical);

    void ToggleBorderFlag(PObjectBase obj, int border);

    void CreateBoxSizerWithObject(PObjectBase obj);

    void ShowXrcPreview();

    // Servicios para los observadores
    PObjectBase GetSelectedObject();

    PObjectBase GetProjectData();

    PObjectBase GetSelectedForm();

    bool CanUndo() { return m_cmdProc.CanUndo(); }
    bool CanRedo() { return m_cmdProc.CanRedo(); }

    bool GetLayoutSettings(PObjectBase obj, int* flag, int* option, int* border, int* orient);
    bool CanPasteObject();
    bool CanPasteObjectFromClipboard();
    bool CanCopyObject();
    bool IsModified();

    void SetDarkMode(bool darkMode);
    bool IsDarkMode() const;

    PObjectPackage GetPackage(size_t idx) { return m_objDb->GetPackage(idx); }

    size_t GetPackageCount() { return m_objDb->GetPackageCount(); }

    PObjectDatabase GetObjectDatabase()
    {
        return m_objDb;
    }

    // Servicios específicos, no definidos en DataObservable
    void SetClipboardObject(PObjectBase obj) { m_clipboard = obj; }

    PObjectBase GetClipboardObject() { return m_clipboard; }

    wxString GetProjectFileName() { return m_projectFile; }

    const int m_fbpVerMajor;
    const int m_fbpVerMinor;

    /** Path to the fbp file that is opened.
    */
    const wxString& GetProjectPath() { return m_projectPath; }

    /** Path where the files will be generated.
    */
    wxString GetOutputPath();

    /** Path where the embedded bitmap files will be generated.
    */
    wxString GetEmbeddedFilesOutputPath();

    void SetProjectPath(const wxString& path) { m_projectPath = path; }

    const wxString& GetApplicationPath() { return m_rootDir; }

    void SetApplicationPath(const wxString& path) { m_rootDir = path; }

    // Allow a single instance check from outsid the AppData class
    bool VerifySingleInstance(const wxString& file, bool switchTo = true);

#ifdef wxWEAVER_DEBUG
    DebugWindow* GetDebugWindow(wxWindow* parent);
#endif
    wxDialog* GetSettingsDialog(wxWindow* parent);

private:
    void NotifyEvent(wxWeaverEvent& event, bool forcedelayed = false);

    // Notifican a cada observador el evento correspondiente
    void NotifyProjectLoaded();

    void NotifyProjectSaved();

    void NotifyObjectExpanded(PObjectBase obj);

    void NotifyObjectSelected(PObjectBase obj, bool force = false);

    void NotifyObjectCreated(PObjectBase obj);

    void NotifyObjectRemoved(PObjectBase obj);

    void NotifyPropertyModified(PProperty prop);

    void NotifyEventHandlerModified(PEvent evtHandler);

    void NotifyProjectRefresh();

    void NotifyCodeGeneration(bool panelOnly = false, bool forcedelayed = false);

    /** Comprueba las referencias cruzadas de todos los nodos del árbol
    */
    void CheckProjectTree(PObjectBase obj);

    /** Resuelve un posible conflicto de nombres.

        @note el objeto a comprobar debe estar insertado en proyecto,
        por tanto no es válida para arboles "flotantes".
    */
    void ResolveNameConflict(PObjectBase obj);

    /** Renames all objects that have the same name than any object of a subtree.
    */
    void ResolveSubtreeNameConflicts(PObjectBase obj, PObjectBase topObj = PObjectBase());

    /** Rutina auxiliar de ResolveNameConflict
    */
    void BuildNameSet(PObjectBase obj, PObjectBase top, std::set<wxString>& name_set);

    /** Calcula la posición donde deberá ser insertado el objeto.

        Dado un objeto "padre" y un objeto "seleccionado", esta rutina calcula la
        posición de inserción de un objeto debajo de "parent" de forma que el objeto
        quede a continuación del objeto "seleccionado".

        El algoritmo consiste ir subiendo en el arbol desde el objeto "selected"
        hasta encontrar un objeto cuyo padre sea el mismo que "parent" en cuyo
        caso se toma la posición siguiente a ese objeto.

        @param parent Parent object
        @param selected Selected object

        @return Insertion position, -1 if not possible.
     */
    int CalcPositionOfInsertion(PObjectBase selected, PObjectBase parent);

    /** Elimina aquellos items que no contengan hijos.

        Esta rutina se utiliza cuando el árbol que se carga de un fichero
        no está bien formado, o la importación no ha sido correcta.
     */
    void RemoveEmptyItems(PObjectBase obj);

    /** Eliminar un objeto.
    */
    void DoRemoveObject(PObjectBase object, bool cutObject);

    void Execute(PCommand cmd);

    /** Search a size in the hierarchy of an object
    */
    PObjectBase SearchSizerInto(PObjectBase obj);

    /** Convert the properties of the project element.

        Handle this separately because it does not repeat.
        @param project The project element.
        @param path The path to the project file.
        @param fileMajor The major revision of the file.
        @param fileMinor The minor revision of the file.
    */
    void ConvertProjectProperties(ticpp::Element* project, const wxString& path,
                                  int fileMajor, int fileMinor);

    /** Iterates through 'property' element children of @a parent.

        Saves all properties with a 'name' attribute matching one of @a names
        into @a properties
        @param parent Object element with child properties.
        @param names Set of property names to search for.
        @param properties Pointer to a set in which to store the result of the search.
    */
    void GetPropertiesToConvert(ticpp::Node* parent,
                                const std::set<std::string>& names,
                                std::set<ticpp::Element*>* properties);

    /** Iterates through 'property' element children of @a parent.

        Removes all properties with a 'name' attribute matching one of @a names
        @param parent Object element with child properties.
        @param names Set of property names to search for.
    */
    void RemoveProperties(ticpp::Node* parent, const std::set<std::string>& names);

    /** Transfers @a options from the text of @a prop
        to the text of @a newPropName, which will be created if it doesn't exist.

        @param prop Property containing the options to transfer.
        @param options Set of options to search for and transfer.
        @param newPropName Name of property to transfer to, will be created if non-existant.
    */
    void TransferOptionList(ticpp::Element* prop, std::set<wxString>* options,
                            const std::string& newPropName);

    void PropagateExpansion(PObjectBase obj, bool expand, bool up);

    ApplicationData(const wxString& rootdir = "."); // hidden constructor

    /** Helper for GetOutputPath and GetEmbeddedFilesOutputPath
    */
    wxString GetPathProperty(const wxString& pathName);

    static ApplicationData* s_instance;

    typedef std::map<std::string, std::set<std::string>> PropertiesToRemove;
    PropertiesToRemove& GetPropertiesToRemove_v1_12() const;

    PObjectDatabase m_objDb; // Base de datos de objetos
    PObjectBase m_project;   // Proyecto
    PObjectBase m_selObj;    // Objeto seleccionado
    PObjectBase m_clipboard;

    CommandProcessor m_cmdProc; // Procesador de comandos Undo/Redo

    PwxWeaverManager m_manager;
    std::shared_ptr<wxWeaverIPC> m_ipc; // Prevents more than one instance of a project

    typedef std::vector<wxEvtHandler*> HandlerVector;
    HandlerVector m_handlers;

#ifdef wxWEAVER_DEBUG
    wxLog* m_log;
    DebugWindow* m_debug;
#endif
    wxString m_projectFile;
    wxString m_projectPath;
    wxString m_rootDir;           // directorio raíz (mismo que el ejecutable)
    bool m_copyOnPaste;           // flag que indica si hay que copiar el objeto al pegar
    bool m_modFlag;               // flag de proyecto modificado
    bool m_warnOnAdditionsUpdate; // flag to warn on additions update / class renames
    bool m_darkMode;
};
