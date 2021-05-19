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
#include "innerframe.h"
#include "window_buttons.h"

#include <wx/dcbuffer.h>
#include <wx/event.h>

wxDEFINE_EVENT(wxEVT_WVR_INNER_FRAME_RESIZED, wxCommandEvent);

class InnerFrame::TitleBar : public wxPanel {
public:
    TitleBar(wxWindow* parent, wxWindowID id,
             const wxPoint& pos = wxDefaultPosition,
             const wxSize& size = wxDefaultSize,
             long style = 0);

    void OnPaint(wxPaintEvent& event);

    void OnLeftClick(wxMouseEvent& event);
    void SetTitle(const wxString& title) { m_titleText = title; }
    wxString GetTitle() { return m_titleText; }
    void SetStyle(long style) { m_style = style; }

protected:
    wxSize DoGetBestSize() const override { return wxSize(100, 19); }

private:
    void DrawTitleBar(wxDC& dc);

    wxBitmap m_minimize;
    wxBitmap m_minimizeDisabled;
    wxBitmap m_maximize;
    wxBitmap m_maximizeDisabled;
    wxBitmap m_close;
    wxBitmap m_closeDisabled;
    wxColour m_colour1, m_colour2;
    wxString m_titleText;
    long m_style;
};

InnerFrame::TitleBar::TitleBar(wxWindow* parent, wxWindowID id,
                               const wxPoint& pos, const wxSize& size,
                               long style)
    : wxPanel(parent, id, pos, size, wxFULL_REPAINT_ON_RESIZE)
    , m_minimize(minimize_xpm)
    , m_minimizeDisabled(minimize_disabled_xpm)
    , m_maximize(maximize_xpm)
    , m_maximizeDisabled(maximize_disabled_xpm)
    , m_close(close_xpm)
    , m_closeDisabled(close_disabled_xpm)
    , m_style(style)
{
    m_colour1 = wxSystemSettings::GetColour(wxSYS_COLOUR_ACTIVECAPTION);

    int r, g, b;
    r = wxMin(255, m_colour1.Red() + 30);
    g = wxMin(255, m_colour1.Green() + 30);
    b = wxMin(255, m_colour1.Blue() + 30);

    m_colour2 = wxColour(r, g, b);
    m_titleText = "wxWeaver rocks!";
    SetMinSize(wxSize(100, 19));

    Bind(wxEVT_LEFT_DOWN, &InnerFrame::TitleBar::OnLeftClick, this);
    Bind(wxEVT_PAINT, &InnerFrame::TitleBar::OnPaint, this);
}

void InnerFrame::TitleBar::OnLeftClick(wxMouseEvent& event)
{
    LogDebug("OnLeftClick");
    GetParent()->GetEventHandler()->ProcessEvent(event);
}

void InnerFrame::TitleBar::OnPaint(wxPaintEvent&)
{
    wxPaintDC dc(this);
    wxBufferedDC bdc(&dc, GetClientSize());
    DrawTitleBar(bdc);
}

void InnerFrame::TitleBar::DrawTitleBar(wxDC& dc)
{
    static const int margin = 2;

    int tbPosX, tbPosY; // title bar
    int tbWidth, tbHeight;

    int wbPosX, wbPosY; // window buttons
    int wbWidth /*, wbHeight*/;

    int txtPosX, txtPosY; // title text position
    int /*txtWidth,*/ txtHeight;

    // setup all variables
    wxSize clientSize(GetClientSize());
    //wxSize clientSize(500,100);

    tbPosX = tbPosY = 0;
    tbHeight = m_close.GetHeight() + margin * 2;
    tbWidth = clientSize.GetWidth();

    /*wbHeight = m_close.GetHeight();*/
    wbWidth = m_close.GetWidth();
    wbPosX = tbPosX + tbWidth - wbWidth - 2 * margin;
    wbPosY = tbPosX + margin;

    txtPosY = tbPosY + margin;
    txtPosX = tbPosX + 15 + 2 * margin;
    txtHeight = tbHeight - 2 * margin + 1;
    /*txtWidth = wbPosX - 2 * margin - txtPosX;*/
#if 1
    // Draw title background with vertical gradient.
    float incR = (float)(m_colour2.Red() - m_colour1.Red()) / tbWidth;
    float incG = (float)(m_colour2.Green() - m_colour1.Green()) / tbWidth;
    float incB = (float)(m_colour2.Blue() - m_colour1.Blue()) / tbWidth;

    float colourR = m_colour1.Red();
    float colourG = m_colour1.Green();
    float colourB = m_colour1.Blue();

    wxColour colour;
    wxPen pen;
    for (int i = 0; i < tbWidth; i++) {
        colour.Set((unsigned char)colourR,
                   (unsigned char)colourG,
                   (unsigned char)colourB);
        pen.SetColour(colour);
        dc.SetPen(pen);
        dc.DrawLine(tbPosX + i, tbPosY, tbPosX + i, tbPosY + tbHeight);

        colourR += incR;
        colourG += incG;
        colourB += incB;
    }
#else
    // Draw title background with horizontal gradient.
    float incR = (float)(m_colour2.Red() - m_colour1.Red()) / tbHeight;
    float incG = (float)(m_colour2.Green() - m_colour1.Green()) / tbHeight;
    float incB = (float)(m_colour2.Blue() - m_colour1.Blue()) / tbHeight;

    float colourR = m_colour1.Red();
    float colourG = m_colour1.Green();
    float colourB = m_colour1.Blue();

    wxColour colour;
    wxPen pen;
    for (int i = 0; i < tbHeight; i++) {
        colour.Set((unsigned char)colourR,
                   (unsigned char)colourG,
                   (unsigned char)colourB);
        pen.SetColour(colour);
        dc.SetPen(pen);
        dc.DrawLine(tbPosX, tbPosY + i, tbPosX + tbWidth, tbPosY + i);

        colourR += incR;
        colourG += incG;
        colourB += incB;
    }
#endif
    // Draw title text
    wxFont font = dc.GetFont();
    wxSize ppi = dc.GetPPI();

    int fontSize = 72 * txtHeight / ppi.GetHeight();

    font.SetWeight(wxFONTWEIGHT_BOLD);
    dc.SetTextForeground(*wxWHITE);

    // text vertical adjustment
    wxCoord tw, th;
    do {
        font.SetPointSize(fontSize--);
        dc.SetFont(font);
        dc.GetTextExtent(m_titleText, &tw, &th);
    } while (th > txtHeight);

    dc.DrawLabel(m_titleText, wxRect(txtPosX, txtPosY, tw, th));

    // Draw Buttons
    bool hasClose = (m_style & wxCLOSE_BOX);
    bool hasMinimize = (m_style & wxMINIMIZE_BOX);
    bool hasMaximize = (m_style & wxMAXIMIZE_BOX);

#ifdef __WXMSW__
    if (!(m_style & wxSYSTEM_MENU))
        return; // On Windows, no buttons are drawn without System Menu

    dc.DrawBitmap(hasClose ? m_close : m_closeDisabled, wbPosX, wbPosY, true);
    wbPosX -= m_close.GetWidth();

    if (hasMaximize)
        dc.DrawBitmap(m_maximize, wbPosX, wbPosY, true);
    else if (hasMinimize)
        dc.DrawBitmap(m_maximizeDisabled, wbPosX, wbPosY, true);

    wbPosX -= m_maximize.GetWidth();

    if (hasMinimize)
        dc.DrawBitmap(m_minimize, wbPosX, wbPosY, true);
    else if (hasMaximize)
        dc.DrawBitmap(m_minimizeDisabled, wbPosX, wbPosY, true);
#else
    if (hasClose) {
        dc.DrawBitmap(m_close, wbPosX, wbPosY, true);
        wbPosX -= m_close.GetWidth();
    }
    bool hasResizeBorder = (m_style & wxRESIZE_BORDER);
    if (hasMaximize && hasResizeBorder) {
        dc.DrawBitmap(m_maximize, wbPosX, wbPosY, true);
        wbPosX -= m_maximize.GetWidth();
    }
    if (hasMinimize)
        dc.DrawBitmap(m_minimize, wbPosX, wbPosY, true);
#endif
}

InnerFrame::InnerFrame(wxWindow* parent, wxWindowID id,
                       const wxPoint& pos, const wxSize& size, long style)
#ifdef __WXGTK__
    : wxPanel(parent, id, pos, size, wxNO_BORDER | wxFULL_REPAINT_ON_RESIZE)
#else
    : wxPanel(parent, id, pos, size, wxRAISED_BORDER | wxFULL_REPAINT_ON_RESIZE)
#endif
    , m_titleBar(new TitleBar(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, style))
    , m_frameContent(new wxPanel(this, wxID_ANY, wxDefaultPosition, wxDefaultSize))
    , m_sizing(NONE)
    , m_minSize(m_titleBar->GetMinSize())
    , m_curX(-1)
    , m_curY(-1)
    , m_resizeBorder(10)
{
    /*
        Use spacers to create a 1 pixel border on left and top of content panel,
        this is for drawing the selection box.
        Use borders to create a 2 pixel border on right and bottom,
        this is so the back panel can catch mouse events for resizing
    */
    wxBoxSizer* hSizer = new wxBoxSizer(wxHORIZONTAL);
    wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);

    hSizer->AddSpacer(1);
    hSizer->Add(m_frameContent, 1, wxGROW);

    sizer->Add(m_titleBar, 0, wxGROW | wxRIGHT, 2);
    sizer->AddSpacer(1);
    sizer->Add(hSizer, 1, wxGROW | wxBOTTOM | wxRIGHT, 2);

    SetSizer(sizer);
    SetAutoLayout(true);
    Layout();

    m_minSize.x += 8;
    m_minSize.y += 10;
    m_baseMinSize = m_minSize;

    if (size == wxDefaultSize)
        SetSize(GetBestSize());

    Bind(wxEVT_MOTION, &InnerFrame::OnMouseMotion, this);
    Bind(wxEVT_LEFT_DOWN, &InnerFrame::OnLeftDown, this);
    Bind(wxEVT_LEFT_UP, &InnerFrame::OnLeftUp, this);
}

wxSize InnerFrame::DoGetBestSize() const
{
    wxSize best = m_titleBar->GetBestSize();
    wxSize content = m_frameContent->GetBestSize();
    best.IncBy(0, content.GetHeight());
    int border = wxSystemSettings::GetMetric(wxSYS_BORDER_X);
    best.SetWidth((content.GetWidth() + 1 > best.GetWidth()
                       ? content.GetWidth() + 1
                       : best.GetWidth())
                  + 2 + 2 * (border > 0 ? border : 2));

    best.IncBy(0, 3); // spacers and borders
    return best;
}

void InnerFrame::OnMouseMotion(wxMouseEvent& e)
{
    if (m_sizing != NONE) {
        wxScreenDC dc;
        wxPen pen(*wxBLACK, 1, wxPENSTYLE_DOT);

        dc.SetPen(pen);
        dc.SetBrush(*wxTRANSPARENT_BRUSH);
        dc.SetLogicalFunction(wxINVERT);

        //wxPoint pos = ClientToScreen(wxPoint(0, 0));
        wxPoint pos = GetParent()->ClientToScreen(GetPosition());

        if (m_curX >= 0 && m_curY >= 0)
            dc.DrawRectangle(pos.x, pos.y, m_curX, m_curY);

        if (m_sizing == RIGHT || m_sizing == RIGHTBOTTOM)
            m_curX = e.GetX() + m_difX;
        else
            m_curX = GetSize().x;

        if (m_sizing == BOTTOM || m_sizing == RIGHTBOTTOM)
            m_curY = e.GetY() + m_difY;
        else
            m_curY = GetSize().y;

        // User min size
        wxSize minSize = GetMinSize();
        if (m_curX < minSize.x)
            m_curX = minSize.x;
        if (m_curY < minSize.y)
            m_curY = minSize.y;

        // Internal min size
        if (m_curX < m_minSize.x)
            m_curX = m_minSize.x;
        if (m_curY < m_minSize.y)
            m_curY = m_minSize.y;

        wxSize maxSize = GetMaxSize();
        if (m_curX > maxSize.x && maxSize.x != wxDefaultCoord)
            m_curX = maxSize.x;
        if (m_curY > maxSize.y && maxSize.y != wxDefaultCoord)
            m_curY = maxSize.y;

        dc.DrawRectangle(pos.x, pos.y, m_curX, m_curY);

        dc.SetLogicalFunction(wxCOPY);
        dc.SetPen(wxNullPen);
        dc.SetBrush(wxNullBrush);
    } else {
        int x, y;
        GetClientSize(&x, &y);

        if ((e.GetX() >= x - m_resizeBorder && e.GetY() >= y - m_resizeBorder)
            || (e.GetX() < m_resizeBorder && e.GetY() < m_resizeBorder)) {
            SetCursor(wxCursor(wxCURSOR_SIZENWSE));
        } else if ((e.GetX() < m_resizeBorder && e.GetY() >= y - m_resizeBorder)
                   || (e.GetX() > x - m_resizeBorder && e.GetY() < m_resizeBorder)) {
            SetCursor(wxCursor(wxCURSOR_SIZENESW));
        } else if (e.GetX() >= x - m_resizeBorder
                   || e.GetX() < m_resizeBorder) {
            SetCursor(wxCursor(wxCURSOR_SIZEWE));
        } else if (e.GetY() >= y - m_resizeBorder
                   || e.GetY() < m_resizeBorder) {
            SetCursor(wxCursor(wxCURSOR_SIZENS));
        } else {
            SetCursor(*wxSTANDARD_CURSOR);
        }
        m_titleBar->SetCursor(*wxSTANDARD_CURSOR);
        m_frameContent->SetCursor(*wxSTANDARD_CURSOR);
    }
}

void InnerFrame::OnLeftDown(wxMouseEvent& e)
{
    LogDebug("OnLeftDown");
    if (m_sizing == NONE) {
        if (e.GetX() >= GetSize().x - m_resizeBorder
            && e.GetY() >= GetSize().y - m_resizeBorder)
            m_sizing = RIGHTBOTTOM;
        else if (e.GetX() >= GetSize().x - m_resizeBorder)
            m_sizing = RIGHT;
        else if (e.GetY() >= GetSize().y - m_resizeBorder)
            m_sizing = BOTTOM;

        if (m_sizing != NONE) {
            m_difX = GetSize().x - e.GetX();
            m_difY = GetSize().y - e.GetY();
            CaptureMouse();
            OnMouseMotion(e);
        }
    }
}

void InnerFrame::OnLeftUp(wxMouseEvent&)
{
    if (m_sizing == NONE)
        return;

    m_sizing = NONE;
    ReleaseMouse();

    wxScreenDC dc;
    wxPen pen(*wxBLACK, 1, wxPENSTYLE_DOT);

    dc.SetPen(pen);
    dc.SetBrush(*wxTRANSPARENT_BRUSH);
    dc.SetLogicalFunction(wxINVERT);

    //wxPoint pos = ClientToScreen(wxPoint(0, 0));
    wxPoint pos = GetParent()->ClientToScreen(GetPosition());

    dc.DrawRectangle(pos.x, pos.y, m_curX, m_curY);

    dc.SetLogicalFunction(wxCOPY);
    dc.SetPen(wxNullPen);
    dc.SetBrush(wxNullBrush);

    wxScrolledWindow* VEditor = (wxScrolledWindow*)GetParent();
    int scrolledposX = 0;
    int scrolledposY = 0;
    VEditor->GetViewStart(&scrolledposX, &scrolledposY);
    SetSize(m_curX, m_curY);
    Freeze();
    VEditor->FitInside();
    VEditor->SetVirtualSize(GetSize().x + 20, GetSize().y + 20);

    wxCommandEvent event(wxEVT_WVR_INNER_FRAME_RESIZED, GetId());
    event.SetEventObject(this);
    GetEventHandler()->AddPendingEvent(event);

    VEditor->Scroll(scrolledposX, scrolledposY);
    Thaw();
    Update();

    m_curX = m_curY = -1;
}

void InnerFrame::ShowTitleBar(bool show)
{
    m_titleBar->Show(show);
    m_minSize = (show ? m_baseMinSize : wxSize(10, 10));
    Layout();
}

void InnerFrame::SetToBaseSize()
{
    if (m_titleBar->IsShown())
        SetSize(m_baseMinSize);
    else
        SetSize(wxSize(10, 10));
}

bool InnerFrame::IsTitleBarShown()
{
    return m_titleBar->IsShown();
}

void InnerFrame::SetTitle(const wxString& title)
{
    m_titleBar->SetTitle(title);
}

wxString InnerFrame::GetTitle()
{
    return m_titleBar->GetTitle();
}

void InnerFrame::SetTitleStyle(long style)
{
    m_titleBar->SetStyle(style);
}
