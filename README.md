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
* Monochrome, 8 ANSI colors or 256 color support, depending on terminal.
* Accepts TAS input in the FM2 format.
* Famicom Disk System (FDS) support to load Super Mario Bros 2.
* HVC-007 keyboard and HVC-008 data recorder support in "BASIC mode".

Screenshot from SMB1:
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

Screenshot from SMB2:
```

   MARIO          WORLD  TIME
   000000  $x00    1-1

     /\|||\/||\
  /\ \\|||/|-|<
 /   \/\/| \|||
 \--
     /v\/\|\|/\ |\|\/\/\  /\
     |||||||||| ||||||||  ||
     ||||||<||| |<|<||\\   /
     |||||||||| ||||||||  /
     ||||||||\/ |/||\/\/. ||

               c1986 NINTENDO
  %%%%%%
  %%%%%%
  % %% %   * MARIO GAME
  % %% %
%%%%%%%%%%   LUIGI GAME   /\
%%%%%%%%%%                ||
%%%%  %%%%            /\  ||
%%%%  %%%% TOP- 000000\/  \/
%%%% @@%%%            ||  ||----
%%%% @@%%%            ||  ||----
################################
################################
################################
################################
```

Information on my blog:
* [lazyboNES Emulator](https://kobolt.github.io/article-194.html)
* [lazyboNES Emulator TAS Support](https://kobolt.github.io/article-206.html)
* [lazyboNES Emulator FDS Support](https://kobolt.github.io/article-216.html)
* [lazyboNES Emulator Telnet Hacks](https://kobolt.github.io/article-240.html)
* [lazyboNES Emulator Famicom BASIC Support](https://kobolt.github.io/article-247.html)

YouTube videos:
* [Text-mode Super Mario Bros](https://www.youtube.com/shorts/0doygelPMto)
* [SMB 4:54.032 TAS](https://www.youtube.com/watch?v=kDsrsHwDWrY)
* [Super Mario Bros 2 (Famicom Disk System)](https://www.youtube.com/watch?v=EG1NTZ5mrTo)
* [256-color terminal mode](https://www.youtube.com/watch?v=1N4DXhKInMk)
* [On a real Commodore PC 50-II](https://www.youtube.com/watch?v=rBhs5TBwTWY)
* [Family BASIC program transfer](https://www.youtube.com/watch?v=r433SpSR-e4)

