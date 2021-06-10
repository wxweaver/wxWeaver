#pragma once

#include <wx/textctrl.h>

class DebugWindow : public wxTextCtrl {
public:
    DebugWindow(wxWindow* parent = nullptr);

private:
    void OnTextUpdated(wxCommandEvent& event);
};
