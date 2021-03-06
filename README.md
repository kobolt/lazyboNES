# lazyboNES
Special NES emulator for playing Super Mario Bros with text-mode graphics in a terminal.

Some features:
* Heavily tied to SMB, other games may or may not work or just look plain ugly.
* SDL2 graphical output also available and can run in parallel.
* Use a joystick/gamepad (as detected by SDL2) to play.
* Keyboard input on the terminal kind of works, but it is very hard to use.
* PPU (video) emulation supports horizontal scrolling only.
* APU (audio) emulation supports pulse/triangle/noise channels, but not DMC.
* Save/Load state supported but only one slot and only in memory.
* Ctrl+C in the terminal breaks into a debugger for dumping data.
* Uses the 8 standard ANSI colors, or monochrome if selected.

How it typically looks:
```

   MARIO          WORLD  TIME
   000000  $x00    1-1

      /\|||\/||\
      \\|||/|-|<
      \/\/| \|||

      /v\/\|\|/\ |\|\/\/\
      |||||||||| ||||||||
      ||||||<||| |<|<||\\
      |||||||||| ||||||||
      ||||||||\/ |/||\/\/.

             c1985 NINTENDO


         * 1 PLAYER GAME

           2 PLAYER GAME
    /\
   / .\
  /    \    TOP- 000000
 / . @@ \               /\/\/\
/    @@  \             /      \
################################
################################
################################
################################
```

