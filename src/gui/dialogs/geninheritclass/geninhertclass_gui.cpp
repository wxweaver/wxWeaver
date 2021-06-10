/*
    C++ code generated with wxWeaver (version 0.1.0 May 7 2021)
    https://wxweaver.github.io/

    PLEASE DO *NOT* EDIT THIS FILE!
*/
#include "geninhertclass_gui.h"

GenInheritedClassDlgBase::GenInheritedClassDlgBase(wxWindow* parent,
                                                   wxWindowID id,
                                                   const wxString& title,
                                                   const wxPoint& pos,
                                                   const wxSize& size,
                                                   long style)
    : wxDialog(parent, id, title, pos, size, style)
{
    this->SetSizeHints(wxDefaultSize, wxDefaultSize);

    wxBoxSizer* mainSizer;
    mainSizer = new wxBoxSizer(wxVERTICAL);

    wxStaticBoxSizer* instructionsSbSizer;
    instructionsSbSizer = new wxStaticBoxSizer(
        new wxStaticBox(this, wxID_ANY, _("Instructions")), wxVERTICAL);

    m_instructionsStaticText = new wxStaticText(
        instructionsSbSizer->GetStaticBox(), wxID_ANY,
        _("1. Check the forms you would like to create the inherited class for.\n2. You can edit individual class details by clicking on their names in the list\nand then:\n\t2a. Edit the 'Class Name:' as required.\n\t2b. Edit the 'File Names: (.h/.cpp)' as required.\n3. Click 'OK'."),
        wxDefaultPosition, wxDefaultSize, wxST_NO_AUTORESIZE);

    m_instructionsStaticText->Wrap(-1);
    instructionsSbSizer->Add(m_instructionsStaticText, 0, wxALL | wxEXPAND, 5);

    mainSizer->Add(instructionsSbSizer, 0, wxEXPAND | wxLEFT | wxRIGHT | wxTOP, 5);

    wxArrayString m_formsCheckListChoices;
    m_formsCheckList = new wxCheckListBox(
        this, ID_FORMS_CHECK_LIST, wxDefaultPosition, wxDefaultSize,
        m_formsCheckListChoices, 0);
    m_formsCheckList->SetMinSize(wxSize(350, 150));

    mainSizer->Add(m_formsCheckList, 0, wxALL | wxEXPAND, 5);

    wxStaticBoxSizer* classDescriptionSbSizer = new wxStaticBoxSizer(
        new wxStaticBox(this, wxID_ANY, _("Class Details")), wxVERTICAL);

    m_classNameStaticText = new wxStaticText(
        classDescriptionSbSizer->GetStaticBox(), wxID_ANY, _("Class Name:"),
        wxDefaultPosition, wxDefaultSize, 0);

    m_classNameStaticText->Wrap(-1);
    classDescriptionSbSizer->Add(m_classNameStaticText, 0, wxLEFT | wxRIGHT | wxTOP, 5);

    m_classNameTextCtrl = new wxTextCtrl(
        classDescriptionSbSizer->GetStaticBox(), ID_CLASS_NAME_TEXT_CTRL,
        wxEmptyString, wxDefaultPosition, wxDefaultSize, 0);

    classDescriptionSbSizer->Add(m_classNameTextCtrl, 0, wxBOTTOM | wxRIGHT | wxLEFT | wxEXPAND, 5);

    m_fileNameStaticText = new wxStaticText(
        classDescriptionSbSizer->GetStaticBox(), wxID_ANY, _("File Names: (.cpp/.h)"),
        wxDefaultPosition, wxDefaultSize, 0);

    m_fileNameStaticText->Wrap(-1);
    classDescriptionSbSizer->Add(m_fileNameStaticText, 0, wxLEFT | wxRIGHT | wxTOP, 5);

    m_fileNameTextCtrl = new wxTextCtrl(
        classDescriptionSbSizer->GetStaticBox(), ID_FILE_NAME_TEXT_CTRL,
        wxEmptyString, wxDefaultPosition, wxDefaultSize, 0);

    classDescriptionSbSizer->Add(m_fileNameTextCtrl, 0, wxBOTTOM | wxRIGHT | wxLEFT | wxEXPAND, 5);

    mainSizer->Add(classDescriptionSbSizer, 0, wxEXPAND | wxRIGHT | wxLEFT, 5);

    m_sdbSizer = new wxStdDialogButtonSizer();
    m_sdbSizerOK = new wxButton(this, wxID_OK);
    m_sdbSizer->AddButton(m_sdbSizerOK);
    m_sdbSizerCancel = new wxButton(this, wxID_CANCEL);
    m_sdbSizer->AddButton(m_sdbSizerCancel);
    m_sdbSizer->Realize();

    mainSizer->Add(m_sdbSizer, 0, wxALL | wxALIGN_RIGHT, 5);

    this->SetSizer(mainSizer);
    this->Layout();
    mainSizer->Fit(this);

    this->Centre(wxBOTH);

    m_formsCheckList->Bind(wxEVT_COMMAND_LISTBOX_SELECTED, &GenInheritedClassDlgBase::OnFormsSelected, this);
    m_formsCheckList->Bind(wxEVT_COMMAND_CHECKLISTBOX_TOGGLED, &GenInheritedClassDlgBase::OnFormsToggle, this);
    m_classNameTextCtrl->Bind(wxEVT_COMMAND_TEXT_UPDATED, &GenInheritedClassDlgBase::OnClassNameChange, this);
    m_fileNameTextCtrl->Bind(wxEVT_COMMAND_TEXT_UPDATED, &GenInheritedClassDlgBase::OnFileNameChange, this);
}

GenInheritedClassDlgBase::~GenInheritedClassDlgBase()
{
    m_formsCheckList->Unbind(wxEVT_COMMAND_LISTBOX_SELECTED, &GenInheritedClassDlgBase::OnFormsSelected, this);
    m_formsCheckList->Unbind(wxEVT_COMMAND_CHECKLISTBOX_TOGGLED, &GenInheritedClassDlgBase::OnFormsToggle, this);
    m_classNameTextCtrl->Unbind(wxEVT_COMMAND_TEXT_UPDATED, &GenInheritedClassDlgBase::OnClassNameChange, this);
    m_fileNameTextCtrl->Unbind(wxEVT_COMMAND_TEXT_UPDATED, &GenInheritedClassDlgBase::OnFileNameChange, this);
}
