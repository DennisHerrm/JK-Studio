// texturtags.h - Karten-Editor: die Textur-Tags (shadertags.xml) des
// Texturbrowsers.
//
// Portiert aus NetRadiant-Custom 20260114: libs/xml/xmltextags.cpp/.h
// (XmlTagBuilder). NRC haelt die Datei als libxml2-Dokument und fragt sie mit
// festen XPath-Ausdruecken ab ("/root/*/*[@path='X']", "...[child::tag='T']",
// "/root/*/*/tag", "/root/*/*[not(child::tag)]", "[tag='A' and tag='B']");
// hier ein kleines XML-Dokument mit denselben Knoten (auch den Leerraum-
// Textknoten, die libxml2 beim Einfuegen zusammenlegt: xmlAddChild /
// xmlAddNextSibling) und diese Abfragen direkt. Gespeichert wird wie
// xmlSaveFile mit xmlSaveNoEmptyTags = 1.
//
//   Copyright (C) 2006, Stefan Greven. All Rights Reserved.
//   This file is part of GtkRadiant.
//
//   GtkRadiant is free software; you can redistribute it and/or modify it under
//   the terms of the GNU General Public License as published by the Free
//   Software Foundation; either version 2 of the License, or (at your option)
//   any later version. GtkRadiant is distributed in the hope that it will be
//   useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
//   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General
//   Public License for more details (LICENSE).
#ifndef BHED_KARTE_TEXTURTAGS_H
#define BHED_KARTE_TEXTURTAGS_H

#include <memory>
#include <set>
#include <string>
#include <vector>

namespace bhed::karte {

constexpr const char* kTagDatei = "shadertags.xml";   // SHADERTAG_FILE

enum class TagTexturArt { Stock, Eigen };     // TextureType::STOCK / CUSTOM
enum class TagKnotenArt { Shader, Textur };   // NodeShaderType::SHADER / TEXTURE

class TexturTags {
public:
    TexturTags();
    ~TexturTags();
    TexturTags(const TexturTags&) = delete;
    TexturTags& operator=(const TexturTags&) = delete;

    bool neu(const std::string& speicherDatei = std::string());   // CreateXmlDocument
    bool oeffnen(const std::string& datei, const std::string& speicherDatei = std::string());   // OpenXmlDoc
    bool speichern() const;                                         // SaveXmlDoc
    bool speichern(const std::string& datei) const;
    [[nodiscard]] std::string text() const;                         // was xmlSaveFile schreibt
    bool lesen(const std::string& text);                            // aus einem Text (xmlParseMemory)

    bool shaderDazu(const std::string& shader, TagTexturArt art, TagKnotenArt knoten);   // AddShaderNode
    bool shaderWeg(const std::string& shader);                                           // DeleteShaderNode
    [[nodiscard]] bool hatShader(const std::string& shader) const;                       // CheckShaderTag( shader )
    [[nodiscard]] bool hatTag(const std::string& shader, const std::string& tag) const;  // CheckShaderTag( shader, content )
    bool tagDazu(const std::string& shader, const std::string& tag);                     // AddShaderTag
    int tagUmbenennen(const std::string& alt, const std::string& neu);                   // RenameShaderTag
    bool tagWeg(const std::string& shader, const std::string& tag);                      // DeleteShaderTag
    bool tagUeberallWeg(const std::string& tag);                                          // DeleteTag
    // GetShaderTags: leer = alle Tags aller Shader
    [[nodiscard]] std::vector<std::string> shaderTags(const std::string& shader) const;
    [[nodiscard]] std::set<std::string> ohneTags() const;                                 // GetUntagged
    [[nodiscard]] std::set<std::string> alleTags() const;                                 // GetAllTags
    // TagSearch mit "/root/*/*[tag='A' and tag='B']" (TextureBrowser_searchTags)
    [[nodiscard]] std::set<std::string> suchen(const std::vector<std::string>& tags) const;

    struct Knoten;

private:
    std::unique_ptr<Knoten> m_wurzel;   // das Dokument (Kinder: root)
    std::string m_speicherDatei;        // m_savefilename
};

}  // namespace bhed::karte

#endif
