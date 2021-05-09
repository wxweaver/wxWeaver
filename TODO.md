# TODO

## macOS

- Fix the build, see:
  https://github.com/wxFormBuilder/wxFormBuilder/issues/247
  https://github.com/wxFormBuilder/wxFormBuilder/issues/665

## Windows

- Rewrite wxWeaver.iss script.

## Misc

- Handle own project files, adding the related mimetype.
- CI stuff.
- Handle API and XRC based on wx version also in plugins.

## Controls

- Check the controls returned by plugins with `OnCreated()` and its log errors.
- Try to replace static event tables with `Bind()`.
  Note that using `clang-format` with `wx{BEGIN|END}_EVENT_TABLE` screws up the
  indentation of next elements for an unknown reason, it doesn't happens with
  the old deprecated versions.
- Check components with:
    - no `public` access specifier
    - `CleanUp()` disabled
    - exceptions "handling"

- Replace static event tables with dynamic ones.

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
