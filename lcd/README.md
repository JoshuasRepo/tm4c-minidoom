# Display-Treiber (EDUMKII 128x128 TFT, ST7735S)

Herkunft: TI Crystalfontz128x128_ST7735 (BSD-Lizenz, Header bleiben drin), auf
TivaWare-grlib + EK-TM4C1294XL portiert von Gene Bogdanov (WPI ECE 3849), Quelle
https://github.com/hoangminh2408/ECE-3849-lab3 (Stand 2019). Unveraendert uebernommen.
Der EK-TM4C129EXL hat dieselbe BoosterPack-2-Belegung wie der EK-TM4C1294XL.

Annahmen des Treibers:
- MKII steckt auf BoosterPack 2.
- Systemtakt 120 MHz (LCD_SYSTEM_CLOCK und HAL_LCD_delay im HAL-Header).
- Pins: SSI3 CLK PQ0 (J1.7), MOSI PQ2 (J2.15), CS PP3 (J2.13), DC PK7 (J4.31), RST PA7 (J2.17).
- Framebuffer im RAM (32 KB): grlib-Befehle zeichnen nur in den Puffer,
  sichtbar wird es erst mit GrFlush(&context).

Benutzung: g_sCrystalfontz128x128 als Display an GrContextInit() uebergeben,
vorher einmal Crystalfontz128x128_Init().

Aenderung (26.10.26): In Crystalfontz128x128_Init() setzt der Treiber jetzt
Crystalfontz128x128_SetOrientation(LCD_ORIENTATION_UP) statt nur MADCTL=BGR.
Vorher stand das Bild auf dem Board um 180 Grad gedreht (und die Offset-Variable
passte nicht zur gesetzten Ausrichtung). Original: Crystalfontz128x128_ST7735.c.orig
