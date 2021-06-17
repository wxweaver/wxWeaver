/*
    wxWeaver - A GUI Designer Editor for wxWidgets.
    Copyright (C) 2005 José Antonio Hurtado
    Copyright (C) 2005 Juan Antonio Ortega (as wxFormBuilder)
    Copyright (C) 2012-2021 Andrea Zanellato <redtid3@gmail.com>

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
#include "dockart.h"

#include <wx/dc.h>
#include <wx/renderer.h>
#include <wx/settings.h>

#ifdef __WXOSX__
#include <wx/osx/private.h>
#endif

namespace wxw {
static bool IsDark(const wxColour& colour)
{
    int average = (colour.Red() + colour.Green() + colour.Blue()) / 3;
    if (average < 128)
        return true;
    return false;
}

wxBitmap BitmapFromBits(const unsigned char bits[], int w, int h,
                        const wxColour& color)
{
    wxImage img = wxBitmap((const char*)bits, w, h).ConvertToImage();
    if (color.Alpha() == wxALPHA_OPAQUE) {
        img.Replace(0, 0, 0, 123, 123, 123);
        img.Replace(255, 255, 255, color.Red(), color.Green(), color.Blue());
        img.SetMaskColour(123, 123, 123);
    } else {
        img.InitAlpha();
        const int newr = color.Red();
        const int newg = color.Green();
        const int newb = color.Blue();
        const int newa = color.Alpha();
        for (int x = 0; x < w; x++) {
            for (int y = 0; y < h; y++) {
                int r = img.GetRed(x, y);
                int g = img.GetGreen(x, y);
                int b = img.GetBlue(x, y);
                if (!r && !g && !b) {
                    img.SetAlpha(x, y, wxALPHA_TRANSPARENT);
                } else {
                    img.SetRGB(x, y, newr, newg, newb);
                    img.SetAlpha(x, y, newa);
                }
            }
        }
    }
    return wxBitmap(img);
}

wxColor LightContrastColour(const wxColour& c)
{
    int amount = 120;
    if (c.Red() < 128 && c.Green() < 128 && c.Blue() < 128)
        amount = 160;
    return c.ChangeLightness(amount);
}
} // namespace wxw

wxString DockArt::EllipsizeText(wxDC& dc, const wxString& text, int maxSize)
{
    wxCoord x, y;
    dc.GetTextExtent(text, &x, &y);
    if (x <= maxSize)
        return text;
    size_t i, len = text.Length();
    size_t lastGoodLength = 0;
    for (i = 0; i < len; ++i) {
        wxString s = text.Left(i);
        s += "...";
        dc.GetTextExtent(s, &x, &y);
        if (x > maxSize)
            break;
        lastGoodLength = i;
    }
    wxString ret = text.Left(lastGoodLength);
    ret += "...";
    return ret;
}

DockArt::DockArt()
{
    UpdateColoursFromSystem();
#ifndef __WXOSX__
    m_captionFont = wxFont(8, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL,
                           wxFONTWEIGHT_NORMAL);
#endif
#ifdef __WXOSX__
    m_captionFont = *wxSMALL_FONT;
    SInt32 height;
    GetThemeMetric(kThemeMetricSmallPaneSplitterHeight, &height);
    m_sashSize = height;
#elif defined(__WXGTK__)
    m_sashSize = wxRendererNative::Get().GetSplitterParams(nullptr).widthSash;
#else
    m_sashSize = 4;
#endif
    m_captionSize = 17;
    m_borderSize = 1;
    m_buttonSize = 14;
    m_gripperSize = 9;
    m_gradientType = wxAUI_GRADIENT_VERTICAL;
    InitBitmaps();
}

void DockArt::UpdateColoursFromSystem()
{
    wxColor baseColour = wxSystemSettings::GetColour(wxSYS_COLOUR_3DFACE);
    if ((255 - baseColour.Red()) + (255 - baseColour.Green()) + (255 - baseColour.Blue()) < 60) {
        baseColour = baseColour.ChangeLightness(92);
    }
    m_baseColour = baseColour;
    wxColor darker1Colour = baseColour.ChangeLightness(85);
    wxColor darker2Colour = baseColour.ChangeLightness(75);
    wxColor darker3Colour = baseColour.ChangeLightness(60);
    //wxColor darker4Colour = baseColour.ChangeLightness(50);
    wxColor darker5Colour = baseColour.ChangeLightness(40);
    m_activeCaptionColour = wxSystemSettings::GetColour(wxSYS_COLOUR_HIGHLIGHT);
    m_activeCaptionGradientColour = wxw::LightContrastColour(wxSystemSettings::GetColour(wxSYS_COLOUR_HIGHLIGHT));
    m_activeCaptionTextColour = wxSystemSettings::GetColour(wxSYS_COLOUR_HIGHLIGHTTEXT);
    m_inactiveCaptionColour = darker1Colour;
    m_inactiveCaptionGradientColour = baseColour.ChangeLightness(97);
    m_inactiveCaptionTextColour = wxSystemSettings::GetColour(wxSYS_COLOUR_INACTIVECAPTIONTEXT);
    m_sashBrush = wxBrush(baseColour);
    m_backgroundBrush = wxBrush(baseColour);
    m_gripperBrush = wxBrush(baseColour);
    m_borderPen = wxPen(darker2Colour);
    int pen_width = 1;
    m_gripperPen1 = wxPen(darker5Colour, pen_width);
    m_gripperPen2 = wxPen(darker3Colour, pen_width);
    m_gripperPen3 = wxPen(*wxStockGDI::GetColour(wxStockGDI::COLOUR_WHITE), pen_width);
    InitBitmaps();
}

void DockArt::InitBitmaps()
{
#if defined(__WXOSX__)
    static const unsigned char close_bits[] = {
        0xFF, 0xFF, 0xFF, 0xFF, 0x0F, 0xFE, 0x03, 0xF8, 0x01, 0xF0, 0x19, 0xF3,
        0xB8, 0xE3, 0xF0, 0xE1, 0xE0, 0xE0, 0xF0, 0xE1, 0xB8, 0xE3, 0x19, 0xF3,
        0x01, 0xF0, 0x03, 0xF8, 0x0F, 0xFE, 0xFF, 0xFF
    };
#elif defined(__WXGTK__)
    static const unsigned char close_bits[] = {
        0xff, 0xff, 0xff, 0xff, 0x07, 0xf0, 0xfb, 0xef, 0xdb, 0xed, 0x8b, 0xe8,
        0x1b, 0xec, 0x3b, 0xee, 0x1b, 0xec, 0x8b, 0xe8, 0xdb, 0xed, 0xfb, 0xef,
        0x07, 0xf0, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff
    };
#else
    static const unsigned char close_bits[] = {
        // reduced height, symmetric
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xcf, 0xf3, 0x9f, 0xf9,
        0x3f, 0xfc, 0x7f, 0xfe, 0x3f, 0xfc, 0x9f, 0xf9, 0xcf, 0xf3, 0xff, 0xff,
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff
    };
    /*
         // same height as maximize/restore
         0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xe7, 0xe7, 0xcf, 0xf3, 0x9f, 0xf9,
         0x3f, 0xfc, 0x7f, 0xfe, 0x3f, 0xfc, 0x9f, 0xf9, 0xcf, 0xf3, 0xe7, 0xe7,
         0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };
      */
#endif
    static const unsigned char maximize_bits[] = {
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x07, 0xf0, 0xf7, 0xf7, 0x07, 0xf0,
        0xf7, 0xf7, 0xf7, 0xf7, 0xf7, 0xf7, 0xf7, 0xf7, 0xf7, 0xf7, 0x07, 0xf0,
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff
    };
    static const unsigned char restore_bits[] = {
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x1f, 0xf0, 0x1f, 0xf0, 0xdf, 0xf7,
        0x07, 0xf4, 0x07, 0xf4, 0xf7, 0xf5, 0xf7, 0xf1, 0xf7, 0xfd, 0xf7, 0xfd,
        0x07, 0xfc, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff
    };
    static const unsigned char pin_bits[] = {
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x1f, 0xfc, 0xdf, 0xfc, 0xdf, 0xfc,
        0xdf, 0xfc, 0xdf, 0xfc, 0xdf, 0xfc, 0x0f, 0xf8, 0x7f, 0xff, 0x7f, 0xff,
        0x7f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff
    };
#ifdef __WXOSX__
    const wxColour inactive = wxSystemSettings::GetColour(wxSYS_COLOUR_INACTIVECAPTION);
    const wxColour active = wxSystemSettings::GetColour(wxSYS_COLOUR_CAPTIONTEXT);
#else
    const wxColor inactive = m_inactiveCaptionTextColour;
    const wxColor active = m_activeCaptionTextColour;
#endif
    m_inactiveCloseBitmap = wxw::BitmapFromBits(close_bits, 16, 16, inactive);
    m_activeCloseBitmap = wxw::BitmapFromBits(close_bits, 16, 16, active);
    m_inactiveMaximizeBitmap = wxw::BitmapFromBits(maximize_bits, 16, 16, inactive);
    m_activeMaximizeBitmap = wxw::BitmapFromBits(maximize_bits, 16, 16, active);
    m_inactiveRestoreBitmap = wxw::BitmapFromBits(restore_bits, 16, 16, inactive);
    m_activeRestoreBitmap = wxw::BitmapFromBits(restore_bits, 16, 16, active);
    m_inactivePinBitmap = wxw::BitmapFromBits(pin_bits, 16, 16, inactive);
    m_activePinBitmap = wxw::BitmapFromBits(pin_bits, 16, 16, active);
}

void DockArt::DrawCaption(wxDC& dc, wxWindow*, const wxString& text,
                          const wxRect& rect, wxAuiPaneInfo& pane)
{
    // Draw caption glossy gradient
    wxColour topStart, topEnd, bottomStart, bottomEnd;
    int r, g, b;
    topEnd = wxSystemSettings::GetColour(wxSYS_COLOUR_ACTIVECAPTION);
    r = topEnd.Red() + 45;
    if (r > 255)
        r = 225;
    g = topEnd.Green() + 45;
    if (g > 255)
        g = 225;
    b = topEnd.Blue() + 45;
    if (b > 255)
        b = 225;
    topStart = wxColour((unsigned char)r, (unsigned char)g, (unsigned char)b);
    r = topEnd.Red() - 75;
    if (r < 0)
        r = 15;
    g = topEnd.Green() - 75;
    if (g < 0)
        g = 15;
    b = topEnd.Blue() - 75;
    if (b < 0)
        b = 15;
    bottomStart = wxColour((unsigned char)r, (unsigned char)g, (unsigned char)b);
    r = topEnd.Red() - 15;
    if (r < 0)
        r = 30;
    g = topEnd.Green() - 15;
    if (g < 0)
        g = 30;
    b = topEnd.Blue() - 15;
    if (b < 0)
        b = 30;
    bottomEnd = wxColour((unsigned char)r, (unsigned char)g, (unsigned char)b);
    wxRect gradientRect = rect;
    gradientRect.SetHeight(gradientRect.GetHeight() / 2);
    dc.GradientFillLinear(gradientRect, topStart, topEnd, wxSOUTH);
    gradientRect.Offset(0, gradientRect.GetHeight());
    dc.GradientFillLinear(gradientRect, bottomStart, bottomEnd, wxSOUTH);

    // Draw caption label
    wxCoord w, h;
    wxColour labelColour = wxw::IsDark(bottomStart) ? *wxWHITE : *wxBLACK;
    dc.GetTextExtent("ABCDEFHXfgkj", &w, &h);
    dc.SetFont(wxFont(9, 70, 90, 90, false, wxEmptyString));
    dc.SetTextForeground(labelColour);
    wxRect clipRect = rect;
    clipRect.width -= 3; // Text offset
    clipRect.width -= 2; // Button padding
    if (pane.HasCloseButton())
        clipRect.width -= m_buttonSize;
    if (pane.HasPinButton())
        clipRect.width -= m_buttonSize;
    if (pane.HasMaximizeButton())
        clipRect.width -= m_buttonSize;
    wxString drawText = EllipsizeText(dc, text, clipRect.width);
    dc.SetClippingRegion(clipRect);
    dc.DrawText(drawText, rect.x + 3, rect.y + (rect.height / 2) - (h / 2) - 1);
    dc.DestroyClippingRegion();
}

void DockArt::DrawButton(wxDC& dc, wxWindow*, int button, int button_state,
                         const wxRect& _rect, wxAuiPaneInfo& pane)
{
    wxBitmap bmp;
    switch (button) {
    default:
    case wxAUI_BUTTON_CLOSE:
        if (pane.state & wxAuiPaneInfo::optionActive)
            bmp = m_activeCloseBitmap;
        else
            bmp = m_inactiveCloseBitmap;
        break;
    case wxAUI_BUTTON_PIN:
        if (pane.state & wxAuiPaneInfo::optionActive)
            bmp = m_activePinBitmap;
        else
            bmp = m_inactivePinBitmap;
        break;
    case wxAUI_BUTTON_MAXIMIZE_RESTORE:
        if (pane.IsMaximized()) {
            if (pane.state & wxAuiPaneInfo::optionActive)
                bmp = m_activeRestoreBitmap;
            else
                bmp = m_inactiveRestoreBitmap;
        } else {
            if (pane.state & wxAuiPaneInfo::optionActive)
                bmp = m_activeMaximizeBitmap;
            else
                bmp = m_inactiveMaximizeBitmap;
        }
        break;
    }
    wxRect rect = _rect;
    rect.y = rect.y + (rect.height / 2) - (bmp.GetScaledHeight() / 2);
    if (button_state == wxAUI_BUTTON_STATE_PRESSED) {
        rect.x += 1;
        rect.y += 1;
    }
    if (button_state == wxAUI_BUTTON_STATE_HOVER
        || button_state == wxAUI_BUTTON_STATE_PRESSED) {
        if (pane.state & wxAuiPaneInfo::optionActive) {
            dc.SetBrush(wxBrush(m_activeCaptionColour.ChangeLightness(120)));
            dc.SetPen(wxPen(m_activeCaptionColour.ChangeLightness(70)));
        } else {
            dc.SetBrush(wxBrush(m_inactiveCaptionColour.ChangeLightness(120)));
            dc.SetPen(wxPen(m_inactiveCaptionColour.ChangeLightness(70)));
        }
        dc.DrawRectangle(rect.x, rect.y, // button background
                         bmp.GetScaledWidth() - 1,
                         bmp.GetScaledHeight() - 1);
    }
    dc.DrawBitmap(bmp, rect.x, rect.y, true);
}
