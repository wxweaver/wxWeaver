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

#include "rad/designer/innerframe.h"
#include "rad/designer/visualobj.h"

#include <forms/wizard.h>

/** Extends the InnerFrame to show the object highlight
*/
class DesignerWindow : public InnerFrame {
public:
    DesignerWindow(wxWindow* parent, int id,
                   const wxPoint& pos,
                   const wxSize& size = wxDefaultSize,
                   long style = 0, const wxString& name = "designer_win");
    ~DesignerWindow() override;

    void SetGrid(int x, int y);
    void SetSelectedSizer(wxSizer* sizer) { m_selSizer = sizer; }
    void SetSelectedItem(wxObject* item) { m_selItem = item; }
    void SetSelectedObject(PObjectBase object) { m_selObj = object; }
    void SetSelectedPanel(wxWindow* actPanel) { m_actPanel = actPanel; }
    wxSizer* GetSelectedSizer() { return m_selSizer; }
    wxObject* GetSelectedItem() { return m_selItem; }
    PObjectBase GetSelectedObject() { return m_selObj.lock(); }
    wxWindow* GetActivePanel() { return m_actPanel; }
    static wxMenu* GetMenuFromObject(PObjectBase menu);
    void SetFrameWidgets(PObjectBase menubar, wxWindow* toolbar,
                         wxWindow* statusbar, wxWindow* auipanel);
    void HighlightSelection(wxDC& dc);
    void OnPaint(wxPaintEvent& event);

private:
    void DrawRectangle(wxDC& dc, const wxPoint& point, const wxSize& size,
                       PObjectBase object);

    // This class paints the highlight in the frame content panel.
    class HighlightPaintHandler : public wxEvtHandler {
    public:
        HighlightPaintHandler(wxWindow* win);
        void OnPaint(wxPaintEvent& event);

    private:
        wxWindow* m_window;
    };
    WPObjectBase m_selObj;

    wxSizer* m_selSizer;
    wxObject* m_selItem;
    wxWindow* m_actPanel;

    int m_x;
    int m_y;

    wxDECLARE_CLASS(DesignerWindow);
};

class wxWeaverEvent;
class wxWeaverPropertyEvent;
class wxWeaverObjectEvent;

class VisualEditor : public wxScrolledWindow {
public:
    VisualEditor(wxWindow* parent);
    ~VisualEditor() override;

    void OnResizeBackPanel(wxCommandEvent& event);
    void OnClickBackPanel(wxMouseEvent& event);
    void PreventOnSelected(bool prevent = true);
    void PreventOnModified(bool prevent = true);
    void UpdateVirtualSize();

    PObjectBase GetObjectBase(wxObject* wxobject);
    wxObject* GetWxObject(PObjectBase baseobject);

    // TODO
    wxAuiManager* m_auiMgr;
    Wizard* m_wizard;

    // Give components an opportunity to cleanup
    void ClearComponents(wxWindow* parent);

    // Events
    void OnProjectLoaded(wxWeaverEvent& event);
    void OnProjectSaved(wxWeaverEvent& event);
    void OnObjectSelected(wxWeaverObjectEvent& event);
    void OnObjectCreated(wxWeaverObjectEvent& event);
    void OnObjectRemoved(wxWeaverObjectEvent& event);
    void OnPropertyModified(wxWeaverPropertyEvent& event);
    void OnProjectRefresh(wxWeaverEvent& event);

protected:
    /** Generates wxObjects from ObjectBase

        @param obj ObjectBase to generate.
        @param parent wxWindow parent, necessary to instantiate a widget.
        @param parentObject ObjectBase parent, not always the same as the wxparent
                            (e.g. an abstract component).
    */
    void Generate(PObjectBase obj, wxWindow* parent, wxObject* parentObject);
    void SetupWindow(PObjectBase obj, wxWindow* window);
    void SetupSizer(PObjectBase obj, wxSizer* sizer);

    /** Crea la vista preliminar borrando la previa.
    */
    void Create();
    void DeleteAbstractObjects();

    void ClearAui();
    void SetupAui(PObjectBase obj, wxWindow* window);
    void ScanPanes(wxWindow* parent);

    void OnAuiScan(wxTimerEvent& event);

    void ClearWizard();
    void SetupWizard(PObjectBase obj, wxWindow* window, bool pageAdding = false);
    void OnWizardPageChanged(WizardEvent& event);

private:
    typedef std::map<wxObject*, PObjectBase> wxObjectMap;
    wxObjectMap m_wxobjects;

    typedef std::map<ObjectBase*, wxObject*> ObjectBaseMap;
    ObjectBaseMap m_baseobjects;

    DesignerWindow* m_designer;
    PObjectBase m_form; // Pointer to last form created

    wxPanel* m_auiPanel;
    wxTimer m_auiScanTimer;

    bool m_stopSelectedEvent; // Prevent OnSelected in components
    bool m_stopModifiedEvent; // Prevent OnModified in components
};
