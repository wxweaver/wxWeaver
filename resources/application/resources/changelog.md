# wxWeaver changelog

## v0.1.0
- Forked wxFormBuilder 3.9.0 master branch at commit [c9bdbd0].
- Using [WebKit] code style via `clang-format`.
- Removed `wxT` and incorrect use of `_()` functions
- Removed wx version checks for old 2.9 wxWidgets development version;
  support only wxWidgets 3.0.5.1+.
- Replaced `NULL` with `nullptr`, `unsigned int` with `size_t`.
- Minimized the use of `wxString` helpers in TypeConv (`_STDSTR`, `_WXSTR`).
- Replaced almost all `Connect()` functions and static event tables with `Bind()`,
  this should avoid some conflicts between plugins and editor event handlers.
- Fixed dark mode when using `wxGTK` < v3.1.4.
- Modern UI using `wxAUI` docking panels, better icons.
- `DebugWindow` logger docked pane.
- Using `CMake` build system, archived `Premake 4` and `Meson` scripts.
- File tree reorganziation.
- Use native `freedesktop` icons via `wxArtProvider`.
- New preference dialog.

[c9bdbd0]: https://github.com/wxFormBuilder/wxFormBuilder/commit/c9bdbd0a83b6fa7c22f407d8766e942d25b31e4d
[WebKit]:  https://webkit.org/code-style-guidelines/
