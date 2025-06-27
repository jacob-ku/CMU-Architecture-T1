//---------------------------------------------------------------------------


#pragma hdrstop
#define _CRT_SECURE_NO_WARNINGS
#include "Texture.h"

//---------------------------------------------------------------------------

#pragma package(smart_init)


Texture::Texture() {
	m_Pixels = 0;
	m_ID = 0;

	m_glInternalFormat = 0;
	m_glFormat = 0;
	m_Width = 0;
	m_Height = 0;
}

Texture::~Texture() {
	if (m_Pixels)
		free(m_Pixels);

	if (m_ID)
		glDeleteTextures(1, &m_ID);
}

void Texture::Unload() {
	if (m_ID) {
		glDeleteTextures(1, &m_ID);
		m_ID = 0;
	}
}

void Texture::SetTexture() {
	if (!m_ID && !m_Pixels)
		throw Exception("attempt to use empty texture");

	if (!m_ID) {
		/* we have pixels, but we need to load these into OpenGL */
		GLuint newtexture;
		glGenTextures(1, &newtexture);
		glBindTexture(GL_TEXTURE_2D, newtexture);
		glTexImage2D(GL_TEXTURE_2D, 0, m_glInternalFormat, m_Width, m_Height, 0, m_glFormat, GL_UNSIGNED_BYTE, m_Pixels);

		/* some default texture parameters */
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

		/* ok, done */
		m_ID = newtexture;

		/* we don't need this memory buffer any more */
		free(m_Pixels);
		m_Pixels = 0;
	} else {
		glBindTexture(GL_TEXTURE_2D, m_ID);
	}
}

/********************************************************************\
 * Image format detection                                            *
\********************************************************************/

ImageFormat Texture::DetectFormat(const void* data, size_t size) {
	if (size < 8) return IMAGE_FORMAT_UNKNOWN;

	const unsigned char* bytes = (const unsigned char*)data;

	// Check PNG signature
	if (bytes[0] == 0x89 && bytes[1] == 0x50 && bytes[2] == 0x4E && bytes[3] == 0x47 &&
		bytes[4] == 0x0D && bytes[5] == 0x0A && bytes[6] == 0x1A && bytes[7] == 0x0A) {
		return IMAGE_FORMAT_PNG;
	}

	// Check JPEG signature
	if (bytes[0] == 0xFF && bytes[1] == 0xD8 && bytes[2] == 0xFF) {
		return IMAGE_FORMAT_JPEG;
	}

	return IMAGE_FORMAT_UNKNOWN;
}

/********************************************************************\
 * JPEG loading stuff                                               *
\********************************************************************/

void Texture::my_jpeg_noop(j_decompress_ptr cinfo) {
}

boolean Texture::my_jpeg_fill_input_buffer(j_decompress_ptr cinfo) {
	return 1;
}

void Texture::my_jpeg_skip_input_data(j_decompress_ptr cinfo, long num_bytes) {
	cinfo->src->next_input_byte += (size_t)num_bytes;
	cinfo->src->bytes_in_buffer -= (size_t)num_bytes;
}

void Texture::my_jpeg_mem_src(j_decompress_ptr cinfo, void *mem, int len) {
	cinfo->src = (struct jpeg_source_mgr *)
		(*cinfo->mem->alloc_small)((j_common_ptr)cinfo,
		JPOOL_PERMANENT,
		sizeof(struct jpeg_source_mgr));
	cinfo->src->init_source = my_jpeg_noop;
	cinfo->src->fill_input_buffer = (wxjpeg_boolean (__cdecl *)(j_decompress_ptr))my_jpeg_fill_input_buffer;
	cinfo->src->skip_input_data = my_jpeg_skip_input_data;
	cinfo->src->resync_to_restart = jpeg_resync_to_restart;
	cinfo->src->term_source = my_jpeg_noop;
	cinfo->src->bytes_in_buffer = len;
	cinfo->src->next_input_byte = (const JOCTET*)mem;
}

void Texture::my_jpeg_error_exit(j_common_ptr cinfo) {
	/* on internal JPEG error, we'll find ourselves here */
	my_jpeg_error_ptr *myerr = (my_jpeg_error_ptr*)cinfo->err;

	char errbuf[JMSG_LENGTH_MAX];

	/* format the error message */
	(*cinfo->err->format_message)(cinfo, errbuf);

	/* create an exteption object to throw it later */
	myerr->err = Exception_("JPEG error");

	/* return to LoadJPEG */
	longjmp(myerr->jmpbuf, 1);
}

void Texture::LoadJPEG(int source, ...) {
	if (m_Pixels || m_ID)
		throw Exception("texture is already loaded");

	va_list va;

	struct jpeg_decompress_struct	cinfo;
	struct my_jpeg_error_ptr	jerr;
	JSAMPARRAY		scanlinebuffer;

	FILE		*infile = 0;

	/* variable arguments stuff */
	va_start(va, source);

	/* establish error handling */
	cinfo.err = jpeg_std_error(&jerr.mgr);

	jerr.mgr.error_exit = my_jpeg_error_exit;

	try {
		if (setjmp(jerr.jmpbuf)) {
			throw jerr.err;
		}

		/* initialize decompression */
		jpeg_create_decompress(&cinfo);

		if (source == TEXTURE_SOURCE_FILE) {
			if ((infile = fopen(va_arg(va, char*), "rb")) == 0)
				throw SysException("fopen() failed", errno);

			jpeg_stdio_src(&cinfo, infile);
		} else if (source == TEXTURE_SOURCE_MEM) {
			void *data = va_arg(va, void*);
			size_t len = va_arg(va, size_t);
			my_jpeg_mem_src(&cinfo, data, (int)len);
		} else {
			throw Exception("unknown texture source in JPEG loading");
		}
	
		/* read header */
		jpeg_read_header(&cinfo, 1);

		if (cinfo.jpeg_color_space == JCS_GRAYSCALE) {
			cinfo.out_color_space = JCS_GRAYSCALE;
			m_glInternalFormat = GL_LUMINANCE8;
			m_glFormat = GL_LUMINANCE;
		} else {
			cinfo.out_color_space = JCS_RGB;
			m_glInternalFormat = GL_RGB8;
			m_glFormat = GL_RGB;
		}

		jpeg_calc_output_dimensions(&cinfo);

		m_Width = cinfo.output_width;
		m_Height = cinfo.output_height;

		/* initialize buffers */
		if ((m_Pixels = (unsigned char *)malloc(cinfo.output_width*cinfo.output_components*cinfo.output_height)) == 0)
			throw std::bad_alloc();

		scanlinebuffer = (*cinfo.mem->alloc_sarray)((j_common_ptr)&cinfo, JPOOL_IMAGE, cinfo.output_width*cinfo.output_components, 1);

		/* start decompression */
		jpeg_start_decompress(&cinfo);

		/* read scanlines */
		unsigned int sl = 0;
		while (sl < cinfo.output_height) {
			jpeg_read_scanlines(&cinfo, scanlinebuffer, 1);
			memcpy(&m_Pixels[cinfo.output_width*cinfo.output_components*sl], scanlinebuffer[0], cinfo.output_width*cinfo.output_components);
			sl++;
			SleepEx (0,0); 
		}

		/* finish decompress */
		jpeg_finish_decompress(&cinfo);

	} catch (...) {
		/* error encountered - cleanup and rethrow */
		jpeg_destroy_decompress(&cinfo);

		if (infile)
			fclose(infile);

		if (m_Pixels)
			free(m_Pixels);

		m_Pixels = 0;

		va_end(va);

		throw;
	}

	/* successfully loaded - cleanup */
	jpeg_destroy_decompress(&cinfo);

	if (infile)
		fclose(infile);

	va_end(va);
}

/********************************************************************\
 * PNG loading stuff                                                *
\********************************************************************/

void Texture::my_png_read_fn(png_structp png_ptr, png_bytep data, png_size_t length) {
	FILE* f = (FILE*)png_get_io_ptr(png_ptr);
	fread(data, length, 1, f);
}

void Texture::my_png_mem_read_fn(png_structp png_ptr, png_bytep data, png_size_t length) {
	// Memory source structure
	png_mem_source* mem_source = (png_mem_source*)png_get_io_ptr(png_ptr);

	if (mem_source->offset + length > mem_source->size) {
		png_error(png_ptr, "Read beyond end of memory buffer");
		return;
	}

	memcpy(data, mem_source->data + mem_source->offset, length);
	mem_source->offset += length;
}

void Texture::my_png_error_fn(png_structp png_ptr, png_const_charp error_msg) {
	/* on internal PNG error, we'll find ourselves here */
	my_png_error_ptr *myerr = (my_png_error_ptr*)png_get_error_ptr(png_ptr);

	/* create an exteption object to throw it later */
	myerr->err = Exception_("PNG error");

	/* return to LoadPNG */
	longjmp(myerr->jmpbuf, 1);
}

void Texture::LoadPNG(int source, ...) {
	if (m_Pixels || m_ID)
		throw Exception("texture already loaded");

	va_list va;
	png_structp	png_ptr = 0;
	png_infop	info_ptr = 0;
	png_bytep	*row_pointers = 0;

	FILE		*infile = 0;
	const unsigned char* mem_data = 0;
	size_t mem_size = 0;

	// Memory source structure for PNG
	png_mem_source mem_source = {0, 0, 0};

	/* variable arguments stuff */
	va_start(va, source);

	my_png_error_ptr	perr;

	try {
		if (setjmp(perr.jmpbuf)) {
			throw perr.err;
		}

		if (source == TEXTURE_SOURCE_FILE) {
			if ((infile = fopen(va_arg(va, char*), "rb")) == 0)
				throw SysException("fopen() failed", errno);
		} else if (source == TEXTURE_SOURCE_MEM) {
			mem_data = (const unsigned char*)va_arg(va, void*);
			mem_size = va_arg(va, size_t);
			mem_source.data = mem_data;
			mem_source.size = mem_size;
			mem_source.offset = 0;
		} else {
			throw Exception("bad source in png loading");
		}
	
		/* read signature */
		png_byte sig[8] = {0};

		if (source == TEXTURE_SOURCE_FILE) {
			fread(sig, 8, 1, infile);
		} else {
			if (mem_size < 8) throw Exception("PNG data too small");
			memcpy(sig, mem_data, 8);
			mem_source.offset = 8;
		}

		if (!png_check_sig(sig, 8))
			throw Exception("bad PNG signature");

		/* create structures */
		if ((png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, (png_voidp)&perr, my_png_error_fn, 0)) == 0)
			throw Exception("cannot create PNG read struct");

		if ((info_ptr = png_create_info_struct(png_ptr)) == 0)
			throw Exception("cannot create PNG info struct");

		/* init read function */
		if (source == TEXTURE_SOURCE_FILE) {
			png_set_read_fn(png_ptr, (void*)infile, my_png_read_fn);
		} else {
			png_set_read_fn(png_ptr, (void*)&mem_source, my_png_mem_read_fn);
		}

		/* read header */
		png_set_sig_bytes(png_ptr, 8);

		png_read_info(png_ptr, info_ptr);

		png_uint_32 width = 0, height = 0;
		int bit_depth = 0;
		int color_type = 0;

		png_get_IHDR(png_ptr, info_ptr, &width, &height, &bit_depth, &color_type, 0, 0, 0);

		/* adjust some color transformations */
		if (bit_depth == 16)
			png_set_strip_16(png_ptr);

		switch (color_type) {
		case PNG_COLOR_TYPE_GRAY:
			if (bit_depth < 8)
				png_set_gray_1_2_4_to_8(png_ptr);
			m_glInternalFormat = GL_LUMINANCE8;
			m_glFormat = GL_LUMINANCE;
			break;
		case PNG_COLOR_TYPE_PALETTE:
			png_set_palette_to_rgb(png_ptr);
			m_glInternalFormat = GL_RGB8;
			m_glFormat = GL_RGB;
			break;
		case PNG_COLOR_TYPE_RGB:
			m_glInternalFormat = GL_RGB8;
			m_glFormat = GL_RGB;
			break;
		case PNG_COLOR_TYPE_RGB_ALPHA:
			m_glInternalFormat = GL_RGBA8;
			m_glFormat = GL_RGBA;
			break;
		case PNG_COLOR_TYPE_GRAY_ALPHA:
			m_glInternalFormat = GL_LUMINANCE8_ALPHA8;
			m_glFormat = GL_LUMINANCE_ALPHA;
			break;
		default:
			throw Exception("PNG color_type confusion");
		}

		if (png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS))
			png_set_tRNS_to_alpha(png_ptr);

		/* setup gamma */
		double gamma = 0.0f;
		if (png_get_gAMA(png_ptr, info_ptr, &gamma))
			png_set_gamma(png_ptr, 2.2, gamma);
		else
			png_set_gamma(png_ptr, 2.2, 0.45455);

		/* update info */
		png_read_update_info(png_ptr, info_ptr);
		png_get_IHDR(png_ptr, info_ptr, &width, &height, &bit_depth, &color_type, 0, 0, 0);

		m_Width = width;
		m_Height = height;

		/* initialize buffers */
		png_uint_32 row_bytes = png_get_rowbytes(png_ptr, info_ptr);

		if ((m_Pixels = (unsigned char *)malloc(row_bytes*height)) == 0)
			throw std::bad_alloc();

		if ((row_pointers = (png_bytep*)malloc(sizeof(png_bytep)*height)) == 0)
			throw std::bad_alloc();

		for (unsigned int i = 0; i < height; i++)
			row_pointers[i] = m_Pixels + i*row_bytes;

		/* read image */
		png_read_image(png_ptr, row_pointers);
	} catch (...) {
		/* error encountered - cleanup and rethrow */
		if (png_ptr && info_ptr)
			png_destroy_read_struct(&png_ptr, &info_ptr, 0);
		else if (png_ptr)
			png_destroy_read_struct(&png_ptr, 0, 0);

		if (row_pointers)
			free(row_pointers);

		if (infile)
			fclose(infile);

		if (m_Pixels)
			free(m_Pixels);

		m_Pixels = 0;

		va_end(va);

		throw;
	}

	/* successfully loaded - cleanup */
	png_destroy_read_struct(&png_ptr, &info_ptr, 0);

	free(row_pointers);

	if (infile)
		fclose(infile);

	va_end(va);
}
