# TODO

## macOS

- Fix the build, see:
  https://github.com/wxFormBuilder/wxFormBuilder/issues/247
  https://github.com/wxFormBuilder/wxFormBuilder/issues/665

## Windows

- Rewrite wxWeaver.iss script.

## Misc

- Get rid and eliminate the `output` directory.
- Move `resources` directory in the root and add various better paths.
- Move  `plugins`, `sdk` and `wxFbTest` in `src`.

## Controls

### wxWizard

- Add a wxWizard 2-steps creation (with Create() function) in Python code to add
  the Help button on Python code generation like in C++ one
  (see wxWizard doc's webpage under 'Extended styles').

- Only wxWizardPageSimple class is supported: we need a change to customcontrol
  in additional component plugin (change it from widget type to a standalone)
  in order to use it to create custom wxWizardPages.
