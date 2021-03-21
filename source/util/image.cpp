// image.cpp
// Copyright (c) 2021, zhiayang
// Licensed under the Apache License Version 2.0.

#include <errno.h>
#include <string.h>
#include <stdlib.h>

#include <GL/gl3w.h>

#include "defs.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

namespace util
{
	uint32_t loadImageFromFile(zbuf::str_view path)
	{
		int width = 0;
		int height = 0;
		int chans = 0;

		auto pixels = stbi_load(path.str().c_str(), &width, &height, &chans, 4);
		if(pixels == nullptr)
		{
			lg::error("img", "failed to open '{}': {}", path, strerror(errno));
			return 0;
		}

		GLuint textureId = 0;
		GLint last = 0;

		glGetIntegerv(GL_TEXTURE_BINDING_2D, &last);
		glGenTextures(1, &textureId);
		glBindTexture(GL_TEXTURE_2D, textureId);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);

		glBindTexture(GL_TEXTURE_2D, last);

		stbi_image_free(pixels);

		return textureId;
	}
}
