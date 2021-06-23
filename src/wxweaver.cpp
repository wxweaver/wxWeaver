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
#include "wxweaver.h"

#include "gui/mainframe.h"
#include "rtti/objectbase.h"
#include "utils/exception.h"
#include "utils/typeconv.h"
#include "appdata.h"

#include <wx/clipbrd.h>
#include <wx/cmdline.h>
#include <wx/config.h>
#include <wx/stdpaths.h>
#include <wx/sysopt.h>

#if wxVERSION_NUMBER >= 2905 && wxVERSION_NUMBER <= 3100
#include <wx/xrc/xh_auinotbk.h>
#elif wxVERSION_NUMBER > 3100
#include <wx/xrc/xh_aui.h>
#endif

// Abnormal Termination Handling
#if wxUSE_ON_FATAL_EXCEPTION && wxUSE_STACKWALKER
#include <wx/stackwalk.h>
#elif defined(_WIN32) && defined(__MINGW32__)
#include "stacktrace/stack.hpp"
#include <sstream>
#include <excpt.h>

#if defined __MINGW64_VERSION_MAJOR && defined __MINGW64_VERSION_MINOR /* MinGW-w64 required */
__stdcall EXCEPTION_DISPOSITION StructuredExceptionHandler(struct _EXCEPTION_RECORD* ExceptionRecord, /* breaks build with MinGW 32 */
                                                           void* EstablisherFrame,
                                                           struct _CONTEXT* ContextRecord,
                                                           void* DispatcherContext);
#else
EXCEPTION_DISPOSITION StructuredExceptionHandler(struct _EXCEPTION_RECORD* ExceptionRecord,
                                                 void* EstablisherFrame,
                                                 struct _CONTEXT* ContextRecord,
                                                 void* DispatcherContext);
#endif
#endif

void LogStack();

static const wxCmdLineEntryDesc s_cmdLineDesc[] = {
    { wxCMD_LINE_SWITCH, "g", "generate", "Generate code from passed file.", wxCMD_LINE_VAL_STRING,
      0 },
    { wxCMD_LINE_OPTION, "l", "language",
      "Override the code_generation property from the passed file and generate the passed "
      "languages. Separate multiple languages with commas.",
      wxCMD_LINE_VAL_STRING, 0 },
    { wxCMD_LINE_SWITCH, "h", "help", "Show this help message.", wxCMD_LINE_VAL_STRING,
      wxCMD_LINE_OPTION_HELP },
    { wxCMD_LINE_SWITCH, "v", "version", "Print version information.", wxCMD_LINE_VAL_STRING, 0 },
    { wxCMD_LINE_PARAM, nullptr, nullptr, "File to open.", wxCMD_LINE_VAL_STRING,
      wxCMD_LINE_PARAM_OPTIONAL },
    { wxCMD_LINE_NONE, nullptr, nullptr, nullptr, wxCMD_LINE_VAL_NONE, 0 }
};

wxIMPLEMENT_APP(wxWeaver);

int wxWeaver::OnRun()
{
// Abnormal Termination Handling
#if wxUSE_ON_FATAL_EXCEPTION && wxUSE_STACKWALKER
    ::wxHandleFatalExceptions(true);
#elif defined(_WIN32) && defined(__MINGW32__)
    // Structured Exception handlers are stored in a linked list at FS:[0] for 32-bit and GS:[0] for 64-bit
    // https://github.com/wine-mirror/wine/blob/1aff1e6a370ee8c0213a0fd4b220d121da8527aa/include/winternl.h#L347
    // THIS MUST BE A LOCAL VARIABLE - windows won't use an object outside of the thread's stack frame
    EXCEPTION_REGISTRATION ex;
    ex.handler = StructuredExceptionHandler;

#if defined(__amd64__) || defined(__x86_64__) // 64-bit
    asm volatile("movq %%gs:0, %0"
                 : "=r"(ex.prev));
    asm volatile("movq %0, %%gs:0"
                 :
                 : "r"(&ex));
#elif defined(__i386__) || defined(_X86_)     // 32-bit
    asm volatile("movl %%fs:0, %0"
                 : "=r"(ex.prev));
    asm volatile("movl %0, %%fs:0"
                 :
                 : "r"(&ex));
#endif
#endif
    // Using a space so the initial 'w' will not be capitalized in dialogs
    wxApp::SetAppName(" wxWeaver");

    // Creating the wxConfig manually so there will be no space
    // The old config (if any) is returned, delete it
    delete wxConfigBase::Set(new wxConfig("wxWeaver"));

    // Get the data directory
    wxStandardPathsBase& stdPaths = wxStandardPaths::Get();
    wxString dataDir = stdPaths.GetDataDir();
    dataDir.Replace(GetAppName().c_str(), "wxweaver");

    // Log to stderr while working on the command line
    delete wxLog::SetActiveTarget(new wxLogStderr);
#if 0
    // Message output to the same as the log target
    delete wxMessageOutput::Set(new wxMessageOutputLog);
#endif
    // Help to load locale if wxWeaver is not installed in system
    ::wxSetWorkingDirectory(dataDir);

    bool enabled;
    int selection;
    int language = 0;
    wxConfigBase* config = wxConfigBase::Get();

    config->Read("/Locale/Enabled", &enabled, 0);
    config->Read("/Locale/Language", &selection, 0);

    if (enabled) {
        switch (selection) {
        default:
        case 0:
            language = wxLANGUAGE_DEFAULT;
            break;
        case 1:
            language = wxLANGUAGE_ENGLISH;
            break;
#if 0
        case 2:
            language = wxLANGUAGE_ENGLISH_US;
            break;
        case 3:
            language = wxLANGUAGE_GERMAN;
            break;
        case 4:
#else
        case 2:
#endif
            language = wxLANGUAGE_ITALIAN;
            break;
        }
        SelectLanguage(language);
    }
    // Parse command line
    wxCmdLineParser parser(s_cmdLineDesc, argc, argv);
    if (parser.Parse())
        return 1;

    if (parser.Found("v")) {
        std::cout << "wxWeaver " << VERSION << REVISION << '\n';
        return EXIT_SUCCESS;
    }

    // Get project to load
    wxString projectToLoad = wxEmptyString;
    if (parser.GetParamCount() > 0)
        projectToLoad = parser.GetParam();

    bool justGenerate = false;
    wxString codeLanguage;
    bool hasLanguage = parser.Found("l", &codeLanguage);
    if (parser.Found("g")) {
        if (projectToLoad.empty()) {
            wxLogError("You must pass a path to a project file. Nothing to generate.");
            return 2;
        }
        if (hasLanguage) {
            if (codeLanguage.empty()) {
                wxLogError("Empty language option. Nothing generated.");
                return 3;
            }
            codeLanguage.Replace(",", "|", true);
        }
        // generate code
        justGenerate = true;
#if 0
    } else {
        delete wxLog::SetActiveTarget(new wxLogGui);
#endif
    }
    // Create singleton AppData, wait to initialize until sure
    // that this is not the second instance of a project file.
    AppDataCreate(dataDir);

    // Make passed project name absolute
    try {
        if (!projectToLoad.empty()) {
            wxFileName projectPath(projectToLoad);
            if (!projectPath.IsOk()) {
                wxWEAVER_THROW_EX("This path is invalid: " << projectToLoad);
            }

            if (!projectPath.IsAbsolute()) {
                if (!projectPath.MakeAbsolute()) {
                    wxWEAVER_THROW_EX("Could not make path absolute: " << projectToLoad);
                }
            }
            projectToLoad = projectPath.GetFullPath();
        }
    } catch (wxWeaverException& ex) {
        wxLogError(ex.what());
    }
    // If the project is already loaded in another instance, switch to that instance and quit
    if (!projectToLoad.empty() && !justGenerate) {
        if (::wxFileExists(projectToLoad)) {
            if (!AppData()->VerifySingleInstance(projectToLoad))
                return 4;
        }
    }
    // Init handlers
    wxInitAllImageHandlers();
    wxXmlResource::Get()->InitAllHandlers();
#if wxVERSION_NUMBER >= 2905 && wxVERSION_NUMBER <= 3100
    wxXmlResource::Get()->AddHandler(new wxAuiNotebookXmlHandler);
#elif wxVERSION_NUMBER > 3100
    wxXmlResource::Get()->AddHandler(new wxAuiXmlHandler);
#endif
    // Init AppData
    try {
        AppDataInit();
    } catch (wxWeaverException& ex) {
        wxLogError("Error loading application: %s\n cannot continue.", ex.what());
        wxLog::FlushActive();
        return 5;
    }
    wxSystemOptions::SetOption("msw.remap", 0);
    wxSystemOptions::SetOption("msw.staticbox.optimized-paint", 0);

    m_frame = nullptr;
    wxYield();

    m_frame = new MainFrame();
    if (!justGenerate) {
        m_frame->Show();
        SetTopWindow(m_frame);
    }
/*
    This is not necessary for wxWeaver to work.
    However, Windows sets the Current Working Directory to the directory
    from which a .fbp file was opened, if opened from Windows Explorer.
    This puts an unneccessary lock on the directory.
    This changes the CWD to the already locked app directory as a workaround
*/
#ifdef __WXMSW__
    ::wxSetWorkingDirectory(dataDir);
#endif
    if (!projectToLoad.empty()) {
        if (AppData()->LoadProject(projectToLoad, justGenerate)) {
            if (justGenerate) {
                if (hasLanguage) {
                    PObjectBase project = AppData()->GetProjectData();
                    PProperty codeGen = project->GetProperty("code_generation");
                    if (codeGen)
                        codeGen->SetValue(language);
                }
                AppData()->GenerateCode(false, true);
                return 0;
            } else {
                m_frame->InsertRecentProject(projectToLoad);
                return wxApp::OnRun();
            }
        } else {
            wxLogError("Unable to load project: %s", projectToLoad.c_str());
        }
    }
    if (justGenerate)
        return 6;

    AppData()->NewProject();

#ifdef __WXOSX__
    // document to open on startup
    if (!m_mac_file_name.IsEmpty()) {
        if (AppData()->LoadProject(m_mac_file_name))
            m_frame->InsertRecentProject(m_mac_file_name);
    }
#endif

    return wxApp::OnRun();
}

bool wxWeaver::OnInit()
{
    // Initialization is done in OnRun, so MinGW SEH works for all code
    // (it needs a local variable, OnInit is called before OnRun)
    return true;
}

bool wxWeaver::AddPluginLocaleCatalog(const wxString& catalog)
{
    return m_locale.AddCatalog(catalog);
}

int wxWeaver::OnExit()
{
    MacroDictionary::Destroy();
    AppDataDestroy();

    if (!wxTheClipboard->IsOpened()) {
        if (!wxTheClipboard->Open())
            return wxApp::OnExit();
    }
    // Allow clipboard data to persist after close
    wxTheClipboard->Flush();
    wxTheClipboard->Close();

    return wxApp::OnExit();
}

void wxWeaver::SelectLanguage(int language)
{
    if (!m_locale.Init(language))
        wxLogWarning("This language is not supported by the system.");

#ifdef __WXGTK__
    wxString prefix = wxStandardPaths::Get().GetInstallPrefix() + "/share/wxweaver/locale";
    m_locale.AddCatalogLookupPathPrefix(prefix);

#elif defined(__WXMSW__)
    wxLocale::AddCatalogLookupPathPrefix(
        wxStandardPaths::Get().GetDataDir() + "\\locale");
#endif
    if (!m_locale.AddCatalog("wxweaver"))
        wxLogWarning("Can't load wxLocale main catalog");
#if 1
    // FIXME: Plugin catalogs should be added dynamically on database.cpp@680
    m_locale.AddCatalog("libadditional");
    m_locale.AddCatalog("libcommon");
    m_locale.AddCatalog("libcontainers");
    m_locale.AddCatalog("libforms");
    m_locale.AddCatalog("liblayout");
#endif
#ifdef __LINUX__
    /*
        This catalog is installed in standard location on Linux systems and
        shows that you may make use of the standard message catalogs as well.
        If it's not installed on your system, it is just silently ignored,
    */
    if (!m_locale.AddCatalog("coreutils"))
        wxLogWarning("Can't load wxLocale coreutils catalog");
#endif
}

#ifdef __WXOSX__
void wxWeaver::MacOpenFile(const wxString& fileName)
{
    if (!m_frame)
        m_mac_file_name = fileName;
    else {
        if (!m_frame->SaveWarning())
            return;

        if (AppData()->LoadProject(fileName))
            m_frame->InsertRecentProject(fileName);
    }
}
#endif

#if wxUSE_ON_FATAL_EXCEPTION && wxUSE_STACKWALKER
class StackLogger : public wxStackWalker {
protected:
    void OnStackFrame(const wxStackFrame& frame) override
    {
        // Build param string
        wxString params;
        size_t paramCount = frame.GetParamCount();
        if (paramCount > 0) {
            params << "( ";
            for (size_t i = 0; i < paramCount; ++i) {
                wxString type, name, value;
                if (frame.GetParam(i, &type, &name, &value)) {
                    params << type << " " << name << " = " << value << ", ";
                }
            }
            params << ")";
        }
        wxString source;
        if (frame.HasSourceLocation())
            source.Printf("%s@%i"), frame.GetFileName().c_str(), frame.GetLine();

        wxLogError("%03i %i %s %s %s %s"),
            frame.GetLevel(),
            frame.GetAddress(),
            frame.GetModule().c_str(),
            frame.GetName().c_str(),
            params.c_str(),
            source.c_str();
    }
};

void wxWeaver::OnFatalException()
{
    LogStack();
}
#elif defined(_WIN32) && defined(__MINGW32__)
static _CONTEXT* context = 0;
EXCEPTION_DISPOSITION StructuredExceptionHandler(struct _EXCEPTION_RECORD* ExceptionRecord,
                                                 void* EstablisherFrame,
                                                 struct _CONTEXT* ContextRecord,
                                                 void* DispatcherContext)
{
    context = ContextRecord;
    LogStack();
    return ExceptionContinueSearch;
}

class StackLogger {
public:
    virtual ~StackLogger() = default;

    void WalkFromException()
    {
        try {
            std::stringstream output;
            dbg::stack s(0, context);
            dbg::stack::const_iterator frame;
            for (frame = s.begin(); frame != s.end(); ++frame) {
                output << *frame;
                wxLogError(wxString(output.str().c_str(), *wxConvCurrent));
                output.str("");
            }
        } catch (std::exception& ex) {
            wxLogError(wxString(ex.what(), *wxConvCurrent));
        }
    }
};
#endif

#if (wxUSE_ON_FATAL_EXCEPTION && wxUSE_STACKWALKER) || (defined(_WIN32) && defined(__MINGW32__))
class LoggingStackWalker : public StackLogger {
public:
    LoggingStackWalker() { wxLog::Suspend(); }
    ~LoggingStackWalker() override
    {
        wxLogError("A Fatal Error Occurred. Click Details for a backtrace.");
        wxLog::Resume();
        wxLog* logger = wxLog::GetActiveTarget();
        if (logger)
            logger->Flush();

        exit(1);
    }
};

void LogStack()
{
    LoggingStackWalker walker;
    walker.WalkFromException();
}
#endif
