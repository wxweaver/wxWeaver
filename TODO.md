# TODO

## Linux

- Handle own project files, adding the related mimetype, including icons.
- Debian and other distro packages on OBS.

## macOS

- Fix the build, see:
  https://github.com/wxFormBuilder/wxFormBuilder/issues/247
  https://github.com/wxFormBuilder/wxFormBuilder/issues/665
  https://github.com/wxFormBuilder/wxFormBuilder/issues/677

## Windows

- Rewrite wxWeaver.iss script.

## CMake

- wxWidgets custom script, find Arch wx-config-gtk3.
- Replace macOS `postbuild.sh`.

## Misc

- CI stuff.
- Handle API and XRC based on wx version also in plugins.
- Use snake case instead camel case in some plugins properties
  (toolSeparator -> tool_separator).
- Trailing newlines in codegen templates (e.g. additional plugin).
- Deal with `wxversion` XML property.
- Some click event on a palette tab is ignored, is it an AuiTabArt issue?

## i18n

- Extract help strings from wxWidgets Doxygen comments in interface files,
  to apply on XML files.
  Auto (Google?) translate script, all eventually automated with CI.
- Rewrite XML files to use Text nodes instead `help` attributes to have more
  control on translation filters and avoid missing translations due a problem
  with new lines.

## Controls

- Check the controls returned by plugins with `OnCreated()` and its log errors.
- Adding an expanded wxButton doesn't updates until hovering with mouse in
  XRC preview.
- Controls seems not to be in sync with the treeview, sizers are not highlighted
  in red as usual, in some circumstances trying to make an operation on a widget
  ends to switch to the parent, might cause segfaults.

- Check components with:
    - No `public` access specifier.
    - `CleanUp()` disabled.
    - Exceptions "handling".

- wxAUI
    - Replace current AUI properties management with new objects as
      `wxAuiManager` and `wxAuiPaneInfo`.
      This will break with wxFB compatibility, so a chance for adding wxFB
      projects as imported and save with own new format.
      See <https://github.com/wxWidgets/wxWidgets/blob/master/samples/xrc/rc/aui.xrc>.

- wxBookCtrls
    - Rename `select` property same as wxXRC `selected` and update the project.
      format
    - Reset to 0 all other `select` pages when setting to 1 one of them.

- wxImageList
    - Refactor to make work the XRC import, see `src/xrcloader.cpp` line 162.
    - Missing code generation.

- wxPropertyGrid
    - Add and use values instead labels for `wxWeaverBitmapProperty` and all
      other properties, with a snake case enum (E.g. `load_from_file`),
      then update the project file version. This to enable translation.
    - Replace the wxPropertyGridManager with a notebook with 2 splitters, each
      containing wxPropertyGrid above and a wxHtmlWindow below.
      This way will be possible to use a vertical scrollbar for the content and
      enable hypertextual links.

- wxSizers
    - wxBagGridSizer layout doesn't work correctly.
    - Widgets are not added.

- wxStyledTextCtrl
    - Proper use of StyleClearAll().
    - View always/only whitespaces for indenting doesn't work (Editor settings).

- wxWebView

- wxWizard
    - Add a wxWizard 2-steps creation (with Create() function) in Python code to add
    the Help button on Python code generation like in C++ one
    (see wxWizard doc's webpage under 'Extended styles').

    - Only wxWizardPageSimple class is supported: we need a change to customcontrol
    in additional component plugin (change it from widget type to a standalone)
    in order to use it to create custom wxWizardPages.
