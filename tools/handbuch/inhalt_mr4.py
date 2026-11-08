"""MR4/MR5: Hinweis auf der Seite "Die Karte" zur Texturkompression der Vorschau unter Jedi Academy und Jedi Outcast (S3TC wie
r_ext_compress_textures 1, src/dxt.cpp).

Jeder Text einmal in allen sechs Sprachen (en, de, zh, ja, es, cs) wie in inhalt_paint.py.
"""
from inhalt_basis import T

DXT = (
    "Under the rules of Jedi Academy and Jedi Outcast the textures look as compressed as in the game: like `r_ext_compress_textures 1` "
    "the game uploads every mipmap level as S3TC (DXT5), which shifts the colours slightly (a grey 32 becomes 33 32 33 - the faint "
    "magenta tint of the original). Shaders with `noTC` and the lightmaps stay uncompressed, as in the game. Movie Duels switches the "
    "compression off, so there the textures stay as they are.",
    "Unter den Regeln von Jedi Academy und Jedi Outcast sehen die Texturen so komprimiert aus wie im Spiel: Wie mit "
    "`r_ext_compress_textures 1` lädt das Spiel jede Mip-Stufe als S3TC (DXT5) hoch, das verschiebt die Farben leicht (aus Grau 32 "
    "wird 33 32 33 - der leichte Magentastich des Originals). Shader mit `noTC` und die Lightmaps bleiben wie im Spiel unkomprimiert. "
    "Movie Duels schaltet die Kompression ab, dort bleiben die Texturen, wie sie sind.",
    "在 Jedi Academy 和 Jedi Outcast 的规则下,纹理看起来与游戏中一样经过压缩:与 `r_ext_compress_textures 1` 一样,游戏将每一级 "
    "mipmap 以 S3TC (DXT5) 上传,颜色因此略有偏移(灰度 32 变为 33 32 33——即原版中淡淡的洋红色调)。带有 `noTC` 的着色器和光照贴图"
    "与游戏中一样不压缩。Movie Duels 关闭了压缩,因此那里的纹理保持原样。",
    "Jedi Academy と Jedi Outcast のルールでは、テクスチャはゲームと同じように圧縮されて見えます。`r_ext_compress_textures 1` "
    "と同じく、ゲームは各ミップマップ段階を S3TC (DXT5) としてアップロードするため、色がわずかにずれます(グレー 32 が 33 32 33 "
    "になります。オリジナルのかすかなマゼンタがかった色合いです)。`noTC` のシェーダーとライトマップはゲームと同じく非圧縮のままです。"
    "Movie Duels は圧縮をオフにしているので、そこではテクスチャはそのままです。",
    "Con las reglas de Jedi Academy y Jedi Outcast las texturas se ven tan comprimidas como en el juego: como con "
    "`r_ext_compress_textures 1`, el juego sube cada nivel de mipmap como S3TC (DXT5), lo que desplaza un poco los colores (un gris 32 "
    "pasa a 33 32 33 - el leve tono magenta del original). Los shaders con `noTC` y los lightmaps quedan sin comprimir, como en el "
    "juego. Movie Duels desactiva la compresión, así que allí las texturas quedan como están.",
    "Podle pravidel Jedi Academy a Jedi Outcast vypadají textury stejně komprimované jako ve hře: stejně jako s "
    "`r_ext_compress_textures 1` hra nahraje každou úroveň mipmapy jako S3TC (DXT5), což barvy mírně posune (šedá 32 se změní na "
    "33 32 33 - slabý purpurový nádech originálu). Shadery s `noTC` a lightmapy zůstávají jako ve hře nekomprimované. Movie Duels "
    "kompresi vypíná, takže tam textury zůstávají, jak jsou.",
)


def dxt_notiz():
    return ("note", T(*DXT[:4]))


ES = {DXT[0]: DXT[4]}
CS = {DXT[0]: DXT[5]}
