// kartetexturtags.cpp - Karten-Editor Stufe 2, Punkt 7: die Textur-Tags
// (shadertags.xml) wie NetRadiant-Custom 20260114 (libs/xml/xmltextags.cpp).
//
// Sollwerte: was libxml2 aus NRCs Aufrufen macht. CreateXmlDocument schreibt
// "\n  " vor stock/custom und "\n" am Ende; AddShaderNode/AddShaderTag haengen
// Leerraum mit xmlAddChild an den ersten Textknoten (zusammengelegt) und
// setzen den neuen Knoten mit xmlAddNextSibling an den Anfang;
// xmlSaveNoEmptyTags = 1.
#include "bhed/karte/texturtags.h"

#include <cstdio>
#include <string>

using namespace bhed::karte;

namespace {

int g_fehler = 0;
void pruefe(bool ok, const std::string& was) {
    if (!ok) {
        ++g_fehler;
        std::printf("FEHL  %s\n", was.c_str());
    }
}

const char* const kLeer = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n<root>\n  <stock>\n  </stock>\n  <custom>\n  </custom>\n</root>\n";

void dokument() {
    TexturTags t;
    t.neu();
    pruefe(t.text() == kLeer, "CreateXmlDocument:\n" + t.text());
    t.shaderDazu("textures/a/b", TagTexturArt::Eigen, TagKnotenArt::Textur);
    const std::string erwartet1 =
        "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n<root>\n  <stock>\n  </stock>\n  <custom>\n    <texture path=\"textures/a/b\">\n    </texture>\n  "
        "</custom>\n</root>\n";
    pruefe(t.text() == erwartet1, "AddShaderNode (erster):\n" + t.text());
    t.tagDazu("textures/a/b", "Holz");
    const std::string erwartet2 =
        "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n<root>\n  <stock>\n  </stock>\n  <custom>\n    <texture path=\"textures/a/b\">\n      <tag>Holz</tag>\n    "
        "</texture>\n  </custom>\n</root>\n";
    pruefe(t.text() == erwartet2, "AddShaderTag (erster):\n" + t.text());
    t.tagDazu("textures/a/b", "Metall");
    t.shaderDazu("textures/a/c", TagTexturArt::Eigen, TagKnotenArt::Shader);
    const std::string erwartet3 =
        "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n<root>\n  <stock>\n  </stock>\n  <custom>\n    <shader path=\"textures/a/c\">\n    </shader>\n    "
        "<texture path=\"textures/a/b\">\n      <tag>Metall</tag>\n      <tag>Holz</tag>\n    </texture>\n  </custom>\n</root>\n";
    pruefe(t.text() == erwartet3, "zweiter Tag und zweiter Shader kommen nach vorn:\n" + t.text());
    pruefe(t.hatShader("textures/a/b") && t.hatShader("textures/a/c") && !t.hatShader("textures/a/d"), "CheckShaderTag( shader )");
    pruefe(t.hatTag("textures/a/b", "Holz") && !t.hatTag("textures/a/c", "Holz"), "CheckShaderTag( shader, tag )");
    pruefe(t.alleTags() == std::set<std::string>{"Holz", "Metall"}, "GetAllTags");
    pruefe(t.ohneTags() == std::set<std::string>{"textures/a/c"}, "GetUntagged");
    pruefe(t.suchen({"Holz", "Metall"}) == std::set<std::string>{"textures/a/b"} && t.suchen({"Holz", "Glas"}).empty(), "TagSearch mit and");
    const auto tags = t.shaderTags("textures/a/b");
    pruefe(tags.size() == 2 && tags[0] == "Metall" && tags[1] == "Holz", "GetShaderTags in Dokumentreihenfolge");
    // Rundlauf
    TexturTags u;
    pruefe(u.lesen(t.text()) && u.text() == t.text(), "Lesen und Schreiben ergeben dasselbe");
    // Umbenennen, Loeschen
    pruefe(t.tagUmbenennen("Holz", "Eiche") == 1 && t.hatTag("textures/a/b", "Eiche") && !t.hatTag("textures/a/b", "Holz"), "RenameShaderTag");
    t.tagWeg("textures/a/b", "Metall");
    const std::string erwartet4 =
        "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n<root>\n  <stock>\n  </stock>\n  <custom>\n    <shader path=\"textures/a/c\">\n    </shader>\n    "
        "<texture path=\"textures/a/b\">\n      <tag>Eiche</tag>\n    </texture>\n  </custom>\n</root>\n";
    pruefe(t.text() == erwartet4, "DeleteShaderTag: der Tag und der Leerraum davor:\n" + t.text());
    t.tagDazu("textures/a/c", "Eiche");
    t.tagUeberallWeg("Eiche");
    pruefe(t.alleTags().empty(), "DeleteTag: aus allen Shadern");
    pruefe(t.shaderWeg("textures/a/c") && !t.hatShader("textures/a/c"), "DeleteShaderNode");
    // Maskierung
    TexturTags m;
    m.neu();
    m.shaderDazu("textures/a&b", TagTexturArt::Stock, TagKnotenArt::Textur);
    m.tagDazu("textures/a&b", "<x>");
    pruefe(m.text().find("path=\"textures/a&amp;b\"") != std::string::npos && m.text().find("<tag>&lt;x&gt;</tag>") != std::string::npos,
           "Maskierung von & < >");
    TexturTags n;
    pruefe(n.lesen(m.text()) && n.hatTag("textures/a&b", "<x>"), "Entitaeten beim Lesen aufgeloest");
}

}  // namespace

int main() {
    dokument();
    std::printf("kartetexturtags: %d Fehlschlaege\n", g_fehler);
    return g_fehler == 0 ? 0 : 1;
}
