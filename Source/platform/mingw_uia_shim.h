#pragma once

// MinGW-w64 ships the UI Automation headers, but its uiautomationcore.h is
// missing the CaretPosition enum that JUCE's text provider uses. This is force
// included (see build.sh) so the MinGW build sees it, exactly as the MSVC SDK
// would.
#if defined (_WIN32) && defined (__MINGW32__)
  #ifndef CaretPosition_Unknown
    enum CaretPosition
    {
        CaretPosition_Unknown = 0,
        CaretPosition_EndOfLine = 1,
        CaretPosition_BeginningOfLine = 2
    };
  #endif
#endif
