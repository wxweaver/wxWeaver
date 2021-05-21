# TODO

## Linux

- Handle own project files, adding the related mimetype, including icons.
- Debian and other distro packages on OBS.

## macOS

- Replace `postbuild.sh` with a CMake script
- Fix the build, see:
  https://github.com/wxFormBuilder/wxFormBuilder/issues/247
  https://github.com/wxFormBuilder/wxFormBuilder/issues/665
  https://github.com/wxFormBuilder/wxFormBuilder/issues/677

## Windows

- Rewrite wxWeaver.iss script.

## Misc

- CI stuff.
- Handle API and XRC based on wx version also in plugins.
- Add removed names to copyright headers.
- Use snake case instead camel case in some plugins properties
  (toolSeparator -> tool_separator)

## Controls

- Check the controls returned by plugins with `OnCreated()` and its log errors.

- Replace static event tables and `Connect()` with `Bind()`.

- Check components with:
    - no `public` access specifier
    - `CleanUp()` disabled
    - exceptions "handling"

- wxWizard
    - Add a wxWizard 2-steps creation (with Create() function) in Python code to add
    the Help button on Python code generation like in C++ one
    (see wxWizard doc's webpage under 'Extended styles').

    - Only wxWizardPageSimple class is supported: we need a change to customcontrol
    in additional component plugin (change it from widget type to a standalone)
    in order to use it to create custom wxWizardPages.

- wxTreebook and wxToolbook
- wxWebView
- wxImageList
- Make a common base class for codeeditor panels
