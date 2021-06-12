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
#include "codeeditor.h"
#include "event.h"
#include "settings.h"

#include <wx/fdrepdlg.h>
#include <wx/msgdlg.h>
#include <wx/sizer.h>

#include <wx/stc/stc.h>

CodeEditor::CodeEditor(wxWindow* parent, int id)
    : wxPanel(parent, id)
    , m_code(new wxStyledTextCtrl(this))
{
    // Line Numbers
    m_code->SetMarginType(0, wxSTC_MARGIN_NUMBER);
    m_code->SetMarginWidth(0, m_code->TextWidth(wxSTC_STYLE_LINENUMBER, "_99999"));

    // Markers
    m_code->MarkerDefine(wxSTC_MARKNUM_FOLDER, wxSTC_MARK_BOXPLUS);
    m_code->MarkerSetBackground(wxSTC_MARKNUM_FOLDER, wxColour("BLACK"));
    m_code->MarkerSetForeground(wxSTC_MARKNUM_FOLDER, wxColour("WHITE"));
    m_code->MarkerDefine(wxSTC_MARKNUM_FOLDEROPEN, wxSTC_MARK_BOXMINUS);
    m_code->MarkerSetBackground(wxSTC_MARKNUM_FOLDEROPEN, wxColour("BLACK"));
    m_code->MarkerSetForeground(wxSTC_MARKNUM_FOLDEROPEN, wxColour("WHITE"));
    m_code->MarkerDefine(wxSTC_MARKNUM_FOLDERSUB, wxSTC_MARK_EMPTY);
    m_code->MarkerDefine(wxSTC_MARKNUM_FOLDEREND, wxSTC_MARK_BOXPLUS);
    m_code->MarkerSetBackground(wxSTC_MARKNUM_FOLDEREND, wxColour("BLACK"));
    m_code->MarkerSetForeground(wxSTC_MARKNUM_FOLDEREND, wxColour("WHITE"));
    m_code->MarkerDefine(wxSTC_MARKNUM_FOLDEROPENMID, wxSTC_MARK_BOXMINUS);
    m_code->MarkerSetBackground(wxSTC_MARKNUM_FOLDEROPENMID, wxColour("BLACK"));
    m_code->MarkerSetForeground(wxSTC_MARKNUM_FOLDEROPENMID, wxColour("WHITE"));
    m_code->MarkerDefine(wxSTC_MARKNUM_FOLDERMIDTAIL, wxSTC_MARK_EMPTY);
    m_code->MarkerDefine(wxSTC_MARKNUM_FOLDERTAIL, wxSTC_MARK_EMPTY);

    // Folding
    m_code->SetMarginType(1, wxSTC_MARGIN_SYMBOL);
    m_code->SetMarginMask(1, wxSTC_MASK_FOLDERS);
    m_code->SetMarginWidth(1, 16);
    m_code->SetMarginSensitive(1, true);

    m_code->SetProperty("fold", "1");
    m_code->SetProperty("fold.comment", "1");
    m_code->SetProperty("fold.compact", "1");
    m_code->SetProperty("fold.preprocessor", "1");
    m_code->SetProperty("fold.html", "1");
    m_code->SetProperty("fold.html.preprocessor", "1");

    m_code->SetFoldFlags(wxSTC_FOLDFLAG_LINEBEFORE_CONTRACTED
                         | wxSTC_FOLDFLAG_LINEAFTER_CONTRACTED);

    m_code->SetMarginWidth(2, 0);

    LoadSettings();
    SetupTheme();

    wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
    sizer->Add(m_code, 1, wxEXPAND | wxALL);
    SetSizer(sizer);

    AppData()->AddHandler(this->GetEventHandler());

    Bind(wxEVT_FIND, &CodeEditor::OnFind, this);
    Bind(wxEVT_FIND_NEXT, &CodeEditor::OnFind, this);

    Bind(wxEVT_STC_MARGINCLICK, &CodeEditor::OnMarginClick, this);

    Bind(wxEVT_WVR_PREFS_EDITOR_CHANGED, &CodeEditor::OnPrefsEditorChanged, this);
}

CodeEditor::~CodeEditor()
{
    AppData()->RemoveHandler(this->GetEventHandler());
}

void CodeEditor::OnMarginClick(wxStyledTextEvent& event)
{
    if (event.GetMargin() != 1)
        return;

    int lineClick = m_code->LineFromPosition(event.GetPosition());
    int levelClick = m_code->GetFoldLevel(lineClick);

    if ((levelClick & wxSTC_FOLDLEVELHEADERFLAG) > 0)
        m_code->ToggleFold(lineClick);
}

wxStyledTextCtrl* CodeEditor::GetTextCtrl() const
{
    return m_code;
}

void CodeEditor::LoadSettings()
{
    PrefsEditor prefs;

    wxFont font(prefs.fontSize, wxFONTFAMILY_MODERN, wxFONTSTYLE_NORMAL,
                wxFONTWEIGHT_NORMAL, false, prefs.fontFace); // Default font
    m_font = font;

    SetupTheme(); // TODO: Understand how to use StyleClearAll() properly

    m_code->SetIndent(prefs.indentSize);
    m_code->SetIndentationGuides(prefs.showIndentationGuides);
    m_code->SetUseTabs(prefs.useTabs);
    m_code->SetTabWidth(prefs.tabsWidth);
    m_code->SetTabIndents(prefs.tabIndents);
    m_code->SetCaretWidth(prefs.caretWidth);
    m_code->SetViewWhiteSpace(prefs.showWhiteSpace);
    m_code->SetViewEOL(prefs.showEOL);
}

void CodeEditor::OnFind(wxFindDialogEvent& event)
{
    int wxflags = event.GetFlags();
    int sciflags = 0;
    if ((wxflags & wxFR_WHOLEWORD))
        sciflags |= wxSTC_FIND_WHOLEWORD;

    if ((wxflags & wxFR_MATCHCASE))
        sciflags |= wxSTC_FIND_MATCHCASE;

    int result;
    if ((wxflags & wxFR_DOWN)) {
        m_code->SetSelectionStart(m_code->GetSelectionEnd());
        m_code->SearchAnchor();
        result = m_code->SearchNext(sciflags, event.GetFindString());
    } else {
        m_code->SetSelectionEnd(m_code->GetSelectionStart());
        m_code->SearchAnchor();
        result = m_code->SearchPrev(sciflags, event.GetFindString());
    }
    if (wxSTC_INVALID_POSITION == result) {
        wxMessageBox(
            wxString::Format(_("\"%s\" not found!"), event.GetFindString().c_str()),
            _("Not Found!"), wxICON_ERROR, (wxWindow*)event.GetClientData());
    } else {
        m_code->EnsureCaretVisible();
    }
}

void CodeEditor::OnPrefsEditorChanged(wxWeaverPrefsEditorEvent&)
{
    LoadSettings();
}

void CodeEditor::SetupTheme()
{
    bool darkMode = AppData()->IsDarkMode();
    if (darkMode) {
        m_code->StyleSetBackground(wxSTC_STYLE_DEFAULT, wxColour(30, 30, 30));
        m_code->StyleSetForeground(wxSTC_STYLE_DEFAULT, wxColour(170, 180, 190));
    } else {
        m_code->StyleSetBackground(wxSTC_STYLE_DEFAULT,
                                   wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOW));
        m_code->StyleSetForeground(wxSTC_STYLE_DEFAULT,
                                   wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOWTEXT));
    }
    m_code->StyleSetFont(wxSTC_STYLE_DEFAULT, m_font);
    m_code->StyleClearAll();
    m_code->StyleSetBold(wxSTC_C_WORD, true);
    if (!darkMode) {
        m_code->StyleSetForeground(wxSTC_C_WORD, wxColour(0, 0, 128));
        m_code->StyleSetForeground(wxSTC_C_STRING, wxColour(0, 128, 0));
        m_code->StyleSetForeground(wxSTC_C_STRINGEOL, wxColour(0, 128, 0));
        m_code->StyleSetForeground(wxSTC_C_PREPROCESSOR, wxColour(0, 0, 80));
        m_code->StyleSetForeground(wxSTC_C_COMMENT, wxColour(0, 128, 0));
        m_code->StyleSetForeground(wxSTC_C_COMMENTLINE, wxColour(0, 128, 0));
        m_code->StyleSetForeground(wxSTC_C_COMMENTDOC, wxColour(0, 128, 0));
        m_code->StyleSetForeground(wxSTC_C_COMMENTLINEDOC, wxColour(0, 128, 0));
        m_code->StyleSetForeground(wxSTC_C_NUMBER, wxColour(0, 0, 128));
        m_code->StyleSetForeground(wxSTC_H_DOUBLESTRING, *wxRED);
        m_code->StyleSetForeground(wxSTC_H_TAG, wxColour(0, 0, 128));
        m_code->StyleSetForeground(wxSTC_H_ATTRIBUTE, wxColour(128, 0, 128));
        m_code->SetSelBackground(true, wxSystemSettings::GetColour(wxSYS_COLOUR_HIGHLIGHT));
        m_code->SetSelForeground(true, wxSystemSettings::GetColour(wxSYS_COLOUR_HIGHLIGHTTEXT));
    } else {
        m_code->StyleSetForeground(wxSTC_C_WORD, wxColour(200, 120, 230));
        m_code->StyleSetForeground(wxSTC_C_WORD2, wxColour(235, 100, 115));
        m_code->StyleSetForeground(wxSTC_C_GLOBALCLASS, wxColour(235, 100, 115));
        //m_code->StyleSetForeground(wxSTC_C_IDENTIFIER, wxColour(90, 180, 250));
        m_code->StyleSetForeground(wxSTC_C_CHARACTER, wxColour(150, 200, 120));
        m_code->StyleSetForeground(wxSTC_C_STRING, wxColour(150, 200, 120));
        m_code->StyleSetForeground(wxSTC_C_STRINGEOL, wxColour(150, 200, 120));
        m_code->StyleSetForeground(wxSTC_C_PREPROCESSOR, wxColour(200, 120, 230));
        m_code->StyleSetForeground(wxSTC_C_PREPROCESSORCOMMENT, wxColour(90, 100, 120));
        m_code->StyleSetForeground(wxSTC_C_COMMENT, wxColour(90, 100, 120));
        m_code->StyleSetForeground(wxSTC_C_COMMENTLINE, wxColour(90, 100, 120));
        m_code->StyleSetForeground(wxSTC_C_COMMENTDOC, wxColour(90, 100, 120));
        m_code->StyleSetForeground(wxSTC_C_COMMENTLINEDOC, wxColour(90, 100, 120));
        m_code->StyleSetForeground(wxSTC_C_NUMBER, wxColour(220, 160, 100));
        m_code->StyleSetForeground(wxSTC_H_DOUBLESTRING, wxColour(23, 198, 163));
        m_code->StyleSetForeground(wxSTC_H_TAG, wxColour(18, 144, 195));
        m_code->StyleSetForeground(wxSTC_H_ATTRIBUTE, wxColour(221, 40, 103));
        m_code->SetSelBackground(true, wxColour(45, 50, 60));
    }
    m_code->SetCaretForeground(wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOWTEXT));
}
