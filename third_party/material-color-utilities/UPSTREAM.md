# Material Color Utilities

Source: https://github.com/material-foundation/material-color-utilities

Pinned commit: `5b3618b16fdc3825e21d5679bafd144662088ea1`.

Only the C++ source closure required by SchemeTonalSpot and MaterialDynamicColors is vendored. Apache-2.0 license retained in LICENSE and each source header.

Local adaptation: cpp/utils/utils.cc replaces Abseil StrCat/Hex with std::ostringstream in HexFromArgb; the color algorithms are unchanged. CMakeLists.txt is project-provided.
