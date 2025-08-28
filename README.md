LazyMansClang 2.0 (LMC) is a simple cross-platform GUI wrapper for clang++, designed to take the pain out of quick C++ builds.
LMC began as a personal tool to eliminate the need for constantly typing long clang++ commands in Terminal, but it has since evolved into a fully featured desktop application for macOS and Windows.

New in 2.0!
Cross-platform support:


• macOS .dmg build, drag-and-drop install with custom branding.

• Auto-detects **SDL2**, **SDL_ttf**, **SDL_image**, **GLFW**, **SFML** includes

• Detects multiple **main()** functions

• Custom compiler flags, defines and libs

• Windows .exe build with deployable DLLs (via windeployqt).

• Compiler path persistence

• LMC now remembers your chosen compiler path between sessions.

• Architecture mismatch detection (Windows)

• Warns you if you try compiling with the wrong toolchain (e.g. ARM64 vs x86_64).

• UI overhaul

• Cleaner build output window.

• Removed arbitrary 2-file limit from LMC 1.0 — now supports as many .cpp files as you want.

• C++ standard selection: Build with C++11 → C++23.



💡 Notes

• On macOS, you may need to right-click → Open the first time, since the app is unsigned.

• On Windows, make sure you select the proper clang++.exe for your system architecture.

• Custom icon & gradient background designed under Stardust Softworks.

Running source directly will require Qt6.x.x from The Qt Company

LazyMansClang 2.0 is complete. If you want more features, fork it and make it yours.

Special thanks to Hedge-dev for Sonic Unleashed Recompiled, which literally acted as the critical load bearing MVP that kept my unstable, dying Ryzen PC alive long enough to compile and ship the Windows Release of LazyMansClang.
[Hedge-dev's GitHub](https://github.com/hedge-dev/UnleashedRecomp)

P.S Sonic Unleashed clears Sonic Colours that game stinks.

<img src="https://static.wixstatic.com/media/beada9_d8d17b01c1154e3898a07d481f665ad0~mv2.png" alt="LMC Logo" width="750"/>


