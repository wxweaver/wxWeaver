/*
    wxWeaver - A GUI Designer Editor for wxWidgets.
    Copyright (C) 2005 José Antonio Hurtado
    Copyright (C) 2005 Juan Antonio Ortega
    Copyright (C) 2007 Ryan Pusztai (as wxFormBuilder)
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

#include "geninhertclass_gui.h"

/** Holds the details of the class to generate.
*/
class GenClassDetails {
public:
    /** Default Constructor.
    */
    GenClassDetails() { }

    /** Constructor.

        @param form Form object.
        @param className Name of the class to generate.
        @param fileName File name of the output files the at generated class will be in.
        @param isSelected If true then the class is selected, else it is not.
    */
    GenClassDetails(PObjectBase form, const wxString& className,
                    const wxString& fileName, bool isSelected = false)
        : m_form(form)
        , m_className(className)
        , m_fileName(fileName)
        , m_isSelected(isSelected)
    {
    }

    PObjectBase m_form;   ///< Form object.
    wxString m_className; ///< Name of the class to generate.
    wxString m_fileName;  ///< File name to generate the class in.
    bool m_isSelected;    ///< Holds if the checkbox is selected for the form.
};

class GenInheritedClassDlg : public GenInheritedClassDlgBase {
public:
    GenInheritedClassDlg(wxWindow* parent, PObjectBase project);
    void GetFormsSelected(std::vector<GenClassDetails>* forms);

private:
    void OnFormsSelected(wxCommandEvent& event) override;
    void OnFormsToggle(wxCommandEvent& event) override;
    void OnClassNameChange(wxCommandEvent& event) override;
    void OnFileNameChange(wxCommandEvent& event) override;

    std::vector<GenClassDetails> m_classDetails;
};
