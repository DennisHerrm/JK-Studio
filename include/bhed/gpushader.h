// gpushader.h - Shaderquelltext aus den Merkmalsbits erzeugen
//
// Warum erzeugen statt schreiben
// ------------------------------
// Ein fester Shader je Kombination waere bei zwoelf Merkmalsbits eine
// unuebersehbare Zahl von Dateien. Ein einziger Shader mit allen Zweigen
// waere langsam und wuerde die Bits nur verstecken.
//
// Der eigentliche Grund ist aber ein anderer: Shaderquelltext laesst sich
// auf diesem Rechner weder uebersetzen noch ausfuehren - es gibt keine
// Grafikkarte und kein Direct3D. Erzeugt man ihn dagegen aus den Bits, dann
// ist die ERZEUGUNG gewoehnlicher C++-Code, und den kann man pruefen:
//
//   - hat jedes gesetzte Bit auch eine Zeile im Quelltext?
//   - steht in einem Shader OHNE das Bit die Zeile wirklich nicht drin?
//   - erzeugen gleiche Bits denselben Quelltext (sonst kein Wiederverwenden)?
//
// Das faengt nicht jeden Fehler - ein Tippfehler im HLSL faellt erst beim
// Uebersetzen auf. Es faengt aber die Klasse von Fehlern, die in diesem
// Projekt am haeufigsten war: ein Merkmal, das irgendwo vergessen wurde.
// Genau so entstanden rc383 (deformVertexes nur im Basiseintrag) und rc385
// (Alphatest ohne Bedingung).
//
// Sprache
// -------
// Erzeugt wird HLSL fuer Direct3D 11. Der OpenGL-Weg behaelt vorerst den
// vorhandenen Rasterer; zwei Sprachen gleichzeitig einzufuehren waere
// doppelte Angriffsflaeche, und der D3D-Weg ist der, den shanks Rechner
// benutzt (siehe Protokoll: "Grafikschnittstelle starten: Direct3D 11").
#ifndef BHED_GPUSHADER_H
#define BHED_GPUSHADER_H

#include <cstdint>
#include <string>

namespace bhed::gpu {

// rc608 Teil 11: Das Licht im Spielbild ist Bild mal Lightmap mal diesem Faktor. Im Spiel gemessen (Teil 5 bis 8,
// Movie Duels, screenshot_tga gegen die Vorschau: Boden und Waende 1,00) - r_mapOverBrightBits verschiebt die
// Lightmap beim Laden (R_ColorShiftLightingBytes), das Bild am Schirm traegt danach keinen weiteren Faktor 2. Die
// Vorschau (App::mapBrightness als Vorgabe) und der Karten-Editor (Echtzeit-Licht, gebaute Lightmaps) rechnen damit.
inline constexpr float kSpielLightmapFaktor = 1.0F;

// Der Vertex-Shader fuer diese Merkmalskombination.
//
// `features` ist das Bitfeld aus PipelineState (siehe gpustate.h).
// `numTexMods` begrenzt die tcMod-Schleife; 0 heisst: keine.
[[nodiscard]] std::string vertexShaderHlsl(std::uint32_t features,
                                           int numTexMods);

// Der Vertex-Shader fuer FIGUREN.
//
// Eigener Shader und keine Merkmalsvariante des Karten-Shaders: die Eingabe
// ist eine andere (Knochen statt Lightmap), und die Verformung passiert VOR
// der Projektion. Beides in einen Shader zu zwingen hiesse, in jedem
// Kartenstapel einen ungenutzten Knochenzweig mitzuschleppen.
[[nodiscard]] std::string figurVertexShaderHlsl(std::uint32_t features);

// Der Pixel-Shader fuer dieselbe Kombination.
[[nodiscard]] std::string pixelShaderHlsl(std::uint32_t features);

// --- Die Durchgaenge, die nur Bildpunkte verarbeiten -------------------
//
// Ein Vollbild-Dreieck als Vertex-Shader, dazu zwei Pixel-Shader fuer das
// Weichzeichnen und das Auflegen des Gluehens. Beide sind woertlich aus
// src/glow.cpp uebernommen - zwei Fassungen derselben Formel waeren zwei
// verschieden aussehende Gluehbilder.
[[nodiscard]] std::string vollbildVertexShaderHlsl();
// Die Klingen der Lichtschwerter: fertige Weltecken, Textur oder Farbe.
[[nodiscard]] std::string klingeVertexShaderHlsl();
[[nodiscard]] std::string klingePixelShaderHlsl();
// Dynamisches Licht und Blobschatten auf der Karte: ein zweiter Durchgang
// ueber die deckenden Kartenstapel (ProjectDlightTexture2 und die
// markShadow-Abziehbilder aus CG_PlayerShadow). Konstanten in JeLicht (b4),
// Aufbau in kLichtKonstanten - der Packer steht in gpumap_win32.cpp.
[[nodiscard]] std::string lichtVertexShaderHlsl();
[[nodiscard]] std::string lichtPixelShaderHlsl();
[[nodiscard]] std::string schattenPixelShaderHlsl();
// Die Schattenarten 2 (Volumen, Stencil) und 3 (flach auf den Boden) an
// der Figur - Konstanten in JeSchatten (b5).
[[nodiscard]] std::string schattenVolumenVsHlsl();
[[nodiscard]] std::string schattenVolumenGsHlsl();
[[nodiscard]] std::string schattenFlachVsHlsl();
[[nodiscard]] std::string schattenSchwarzPsHlsl();
[[nodiscard]] std::string schattenAbdunkelnPsHlsl();
// Nebel (RB_FogPass): Karte, Figur, und der gemeinsame Pixel-Shader -
// Konstanten in JeNebel (b6).
[[nodiscard]] std::string nebelVertexShaderHlsl();
[[nodiscard]] std::string nebelFigurVertexShaderHlsl();
[[nodiscard]] std::string nebelPixelShaderHlsl();
// Der Verlauf hinter dem Modellfenster (zu vollbildVertexShaderHlsl).
[[nodiscard]] std::string modellHintergrundPixelShaderHlsl();
[[nodiscard]] std::string glowBlurPixelShaderHlsl();
[[nodiscard]] std::string glowCompositePixelShaderHlsl();
// Movie Duels, r_gammaShaders 1 (Vorgabe, im Protokoll "GAMMA: shader"): am Ende jedes Bilds
// "TEX r0, texcoord, texture[0], RECT; TEX result.color, r0, texture[1], 3D" mit einer 3D-Tabelle
// aus 64 Stufen, GL_LINEAR, GL_CLAMP_TO_EDGE, ohne Halbtexel-Ausgleich. Die Tabelle steht in der DLL
// (MovieDuels-rdsp_x86_64.dll, R_SetColorMappings bei 0x180053FF0): (int)(pow(i/63, 1/r_gamma)*255 + 0.5)
// mal 2^overbright, auf 0..255 geklemmt. Bei r_gamma 1 fast die Einheit, zieht aber die dunklen Werte
// herunter: 3 -> 1, 5 -> 3, 13 -> 11.
[[nodiscard]] float spielGammaWert(float wert);
[[nodiscard]] std::string spielGammaPixelShaderHlsl();
// rc608 Teil 9b: Mit r_ext_multisample hat der Tiefenpuffer Proben, und Direct3D kopiert ihn nicht in einen
// Lesepuffer. Dieser Durchgang schreibt die Tiefe der Probe 0 je Bildpunkt in ein R32_FLOAT-Ziel (fuer die Gizmos).
[[nodiscard]] std::string tiefeProbePixelShaderHlsl();

// Ein kurzer Name fuer diese Kombination - fuer Protokolle und um erzeugte
// Shader wiederzuerkennen.
[[nodiscard]] std::string shaderName(std::uint32_t features, int numTexMods);

}  // namespace bhed::gpu

#endif  // BHED_GPUSHADER_H
