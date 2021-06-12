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

- wxWidgets custom script, find Arch wx-config-gtk3
- Replace macOS `postbuild.sh`

## Misc

- CI stuff.
- Handle API and XRC based on wx version also in plugins.
- Use snake case instead camel case in some plugins properties
  (toolSeparator -> tool_separator)
- Trailing newlines in codegen templates (e.g. additional plugin)
- Deal with `wxversion` XML property

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

- wxBookCtrls
    - Rename `select` property same as wxXRC `selected` and update the project.
      format
    - Reset to 0 all other `select` pages when setting to 1 one of them.

- wxImageList
    - Refactor to make work the XRC import, see `src/model/xrcfilter.cpp` line 162.
    - Missing code generation.

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
