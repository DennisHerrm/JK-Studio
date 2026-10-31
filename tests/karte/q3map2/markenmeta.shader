textures/meta/a
{
	{
		map $whiteimage
	}
}

textures/meta/b
{
	q3map_lightmapSampleSize 8
	{
		map $whiteimage
	}
}

textures/meta/punkt
{
	surfaceparm pointlight
	{
		map $whiteimage
	}
}

textures/meta/krumm
{
	q3map_nonplanar
	q3map_shadeAngle 60
	{
		map $whiteimage
	}
}

textures/meta/teil
{
	tessSize 64
	{
		map $whiteimage
	}
}

textures/meta/ohnet
{
	q3map_notjunc
	q3map_lightmapAxis z
	{
		map $whiteimage
	}
}

textures/meta/tc
{
	q3map_tcGen ivector ( 256 0 0 ) ( 0 128 0 )
	q3map_textureSize 256 128
	{
		map $whiteimage
	}
}

textures/meta/innen
{
	q3map_invert
	q3map_globaltexture
	{
		map $whiteimage
	}
}

textures/meta/sprite
{
	deformVertexes autosprite
	{
		map $whiteimage
	}
}
