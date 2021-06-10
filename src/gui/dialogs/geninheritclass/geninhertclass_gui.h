/*
    C++ code generated with wxWeaver (version 0.1.0 May 7 2021)
    https://wxweaver.github.io/

    PLEASE DO *NOT* EDIT THIS FILE!
*/
#pragma once

#include <wx/artprov.h>
#include <wx/xrc/xmlres.h>
#include <wx/string.h>
#include <wx/stattext.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/sizer.h>
#include <wx/statbox.h>
#include <wx/checklst.h>
#include <wx/textctrl.h>
#include <wx/button.h>
#include <wx/dialog.h>

class GenInheritedClassDlgBase : public wxDialog {
public:
    GenInheritedClassDlgBase(wxWindow* parent, wxWindowID id = wxID_ANY,
                             const wxString& title = "Generate Inherited Class",
                             const wxPoint& pos = wxDefaultPosition,
                             const wxSize& size = wxDefaultSize,
                             long style = wxDEFAULT_DIALOG_STYLE);
    ~GenInheritedClassDlgBase();

protected:
    // Virtual event handlers, override them in your derived class
    virtual void OnFormsSelected(wxCommandEvent& event) { event.Skip(); }
    virtual void OnFormsToggle(wxCommandEvent& event) { event.Skip(); }
    virtual void OnClassNameChange(wxCommandEvent& event) { event.Skip(); }
    virtual void OnFileNameChange(wxCommandEvent& event) { event.Skip(); }

    wxButton* m_sdbSizerOK;
    wxButton* m_sdbSizerCancel;
    wxCheckListBox* m_formsCheckList;
    wxStaticText* m_instructionsStaticText;
    wxStaticText* m_classNameStaticText;
    wxStdDialogButtonSizer* m_sdbSizer;
    wxStaticText* m_fileNameStaticText;
    wxTextCtrl* m_classNameTextCtrl;
    wxTextCtrl* m_fileNameTextCtrl;

    enum {
        ID_FORMS_CHECK_LIST = 1000,
        ID_CLASS_NAME_TEXT_CTRL,
        ID_FILE_NAME_TEXT_CTRL
    };
};
