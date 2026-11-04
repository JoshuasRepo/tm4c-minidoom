# MiniDoom – Raycaster-Shooter auf EK-TM4C129EXL + MKII (BoosterPack 2)

Steuerung: Stick hoch/runter = laufen, links/rechts = drehen, S1 = schießen
(Titelbild: Start), S2 = Tür vor dir öffnen/schließen, nach dem Ende Neustart.
Ziel: alle 5 Gegner abschießen (je 2 Treffer). Gegner greifen auf <0,9 Kacheln an (12 HP).

Dateien:
- game.c/.h: Karte 16x16, Raycasting (DDA, z-Buffer), Gegner-KI, Waffe, HUD, Bildschirme
- sound.c/.h: Soundeffekte ohne Warten, Schritt alle 1 ms aus dem SysTick-ISR (clock.c)
- joystick.c: + joystick_raw() (analoge Werte, Mitte = 0)
- buzzer.c (Ton per Timer0A-Interrupt), buttons.c, lcd/ (Display-Treiber)

Anpassen: Stick vorwärts/rückwärts vertauscht -> JOY_Y_SIGN in game.c auf -1.
Tempo: MOVE_SPEED / TURN_SPEED / ENEMY_SPEED, Schaden ATTACK_DMG.

Bauen: Code Composer Studio (TI-Compiler), TivaWare 2.2.0.295 mit driverlib und grlib.
Den Pfad zu TivaWare in den Projekteigenschaften als Build-Variable `SW_ROOT` setzen.
