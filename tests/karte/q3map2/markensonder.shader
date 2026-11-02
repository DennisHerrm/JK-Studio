textures/sonder/a
{
	{
		map $whiteimage
	}
}

textures/sonder/nebel
{
	surfaceparm fog
	surfaceparm nonsolid
	surfaceparm trans
	surfaceparm nolightmap
	fogparms ( 0.5 0.5 0.5 ) 256
	{
		map $whiteimage
	}
}

textures/sonder/klon
{
	q3map_cloneShader textures/sonder/kopie
	{
		map $whiteimage
	}
}

textures/sonder/kopie
{
	surfaceparm nomarks
	{
		map $whiteimage
	}
}

textures/sonder/rueck
{
	q3map_backShader textures/sonder/hinten
	{
		map $whiteimage
	}
}

textures/sonder/hinten
{
	q3map_lightmapSampleSize 8
	{
		map $whiteimage
	}
}

textures/sonder/nichts
{
	surfaceparm nodraw
	surfaceparm nonsolid
	{
		map $whiteimage
	}
}

textures/sonder/gelaende
{
	q3map_terrain
	{
		map $whiteimage
	}
}

textures/sonder/fleck
{
	polygonOffset
	surfaceparm nonsolid
	{
		map $whiteimage
	}
}

textures/sonder/ohnenebel
{
	q3map_noFog
	{
		map $whiteimage
	}
}
