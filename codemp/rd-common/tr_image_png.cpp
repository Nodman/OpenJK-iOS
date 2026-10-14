/*
===========================================================================
Copyright (C) 1999 - 2005, Id Software, Inc.
Copyright (C) 2000 - 2013, Raven Software, Inc.
Copyright (C) 2001 - 2013, Activision, Inc.
Copyright (C) 2005 - 2015, ioquake3 contributors
Copyright (C) 2013 - 2015, OpenJK contributors

This file is part of the OpenJK source code.

OpenJK is free software; you can redistribute it and/or modify it
under the terms of the GNU General Public License version 2 as
published by the Free Software Foundation.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, see <http://www.gnu.org/licenses/>.
===========================================================================
*/

#include "tr_common.h"
#include <png.h>

void user_write_data( png_structp png_ptr, png_bytep data, png_size_t length ) {
	fileHandle_t fp = *(fileHandle_t*)png_get_io_ptr( png_ptr );
	ri.FS_Write( data, length, fp );
}
void user_flush_data( png_structp png_ptr ) {
	//TODO: ri.FS_Flush?
}

int RE_SavePNG( const char *filename, byte *buf, size_t width, size_t height, int byteDepth ) {
	fileHandle_t fp;
	png_structp png_ptr = NULL;
	png_infop info_ptr = NULL;
	unsigned int x, y;
	png_byte ** row_pointers = NULL;
	/* "status" contains the return value of this function. At first
	it is set to a value which means 'failure'. When the routine
	has finished its work, it is set to a value which means
	'success'. */
	int status = -1;
	/* The following number is set by trial and error only. I cannot
	see where it it is documented in the libpng manual.
	*/
	int depth = 8;

	fp = ri.FS_FOpenFileWrite( filename, qtrue );
	if ( !fp ) {
		goto fopen_failed;
	}

	png_ptr = png_create_write_struct (PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	if (png_ptr == NULL) {
		goto png_create_write_struct_failed;
	}

	info_ptr = png_create_info_struct (png_ptr);
	if (info_ptr == NULL) {
		goto png_create_info_struct_failed;
	}

	/* Set up error handling. */

	if (setjmp (png_jmpbuf (png_ptr))) {
		goto png_failure;
	}

	/* Set image attributes. */

	png_set_IHDR (png_ptr,
		info_ptr,
		width,
		height,
		depth,
		PNG_COLOR_TYPE_RGB,
		PNG_INTERLACE_NONE,
		PNG_COMPRESSION_TYPE_DEFAULT,
		PNG_FILTER_TYPE_DEFAULT);

	/* Initialize rows of PNG. */

	row_pointers = (png_byte **)png_malloc (png_ptr, height * sizeof (png_byte *));
	for ( y=0; y<height; ++y ) {
		png_byte *row = (png_byte *)png_malloc (png_ptr, sizeof (uint8_t) * width * byteDepth);
		row_pointers[height-y-1] = row;
		for (x = 0; x < width; ++x) {
			byte *px = buf + (width * y + x)*3;
			*row++ = px[0];
			*row++ = px[1];
			*row++ = px[2];
		}
	}

	/* Write the image data to "fp". */

//	png_init_io (png_ptr, fp);
	png_set_write_fn( png_ptr, (png_voidp)&fp, user_write_data, user_flush_data );
	png_set_rows (png_ptr, info_ptr, row_pointers);
	png_write_png (png_ptr, info_ptr, PNG_TRANSFORM_IDENTITY, NULL);

	/* The routine has successfully written the file, so we set
	"status" to a value which indicates success. */

	status = 0;

	for (y = 0; y < height; y++) {
		png_free (png_ptr, row_pointers[y]);
	}
	png_free (png_ptr, row_pointers);

png_failure:
png_create_info_struct_failed:
	png_destroy_write_struct (&png_ptr, &info_ptr);
png_create_write_struct_failed:
	ri.FS_FCloseFile( fp );
fopen_failed:
	return status;
}

void user_read_data( png_structp png_ptr, png_bytep data, png_size_t length );
void png_print_error ( png_structp png_ptr, png_const_charp err )
{
	ri.Printf (PRINT_ERROR, "%s\n", err);
}

void png_print_warning ( png_structp png_ptr, png_const_charp warning )
{
	ri.Printf (PRINT_WARNING, "%s\n", warning);
}

bool IsPowerOfTwo ( int i ) { return (i & (i - 1)) == 0; }

struct PNGFileReader
{
	PNGFileReader ( char *buf ) : buf(buf), offset(0), png_ptr(NULL), info_ptr(NULL) {}
	~PNGFileReader()
	{
		ri.FS_FreeFile (buf);
		png_destroy_read_struct (&png_ptr, &info_ptr, NULL);
	}

	int Read ( byte **data, int *width, int *height )
	{
		// Setup the pointers
		*data = NULL;
		*width = 0;
		*height = 0;

		// Make sure we're actually reading PNG data.
		const int SIGNATURE_LEN = 8;

		byte ident[SIGNATURE_LEN];
		memcpy (ident, buf, SIGNATURE_LEN);

		if ( !png_check_sig (ident, SIGNATURE_LEN) )
		{
			ri.Printf (PRINT_ERROR, "PNG signature not found in given image.");
			return 0;
		}

		png_ptr = png_create_read_struct (PNG_LIBPNG_VER_STRING, NULL, png_print_error, png_print_warning);
		if ( png_ptr == NULL )
		{
			ri.Printf (PRINT_ERROR, "Could not allocate enough memory to load the image.");
			return 0;
		}

		info_ptr = png_create_info_struct (png_ptr);
		if ( setjmp (png_jmpbuf (png_ptr)) )
		{
			return 0;
		}

		// We've read the signature
		offset += SIGNATURE_LEN;

		// Setup reading information, and read header
		png_set_read_fn (png_ptr, (png_voidp)this, &user_read_data);
#ifdef PNG_HANDLE_AS_UNKNOWN_SUPPORTED
		// This generic "ignore all, except required chunks" requires 1.6.0 or newer"
		png_set_keep_unknown_chunks (png_ptr, PNG_HANDLE_CHUNK_NEVER, NULL, -1);
#endif
		png_set_sig_bytes (png_ptr, SIGNATURE_LEN);
		png_read_info (png_ptr, info_ptr);

		png_uint_32 width_;
		png_uint_32 height_;
		int depth;
		int colortype;

		png_get_IHDR (png_ptr, info_ptr, &width_, &height_, &depth, &colortype, NULL, NULL, NULL);

		// While modern OpenGL can handle non-PoT textures, it's faster to handle only PoT
		// so that the graphics driver doesn't have to fiddle about with the texture when uploading.
		if ( !IsPowerOfTwo (width_) || !IsPowerOfTwo (height_) )
		{
			ri.Printf (PRINT_ERROR, "Width or height is not a power-of-two.\n");
			return 0;
		}

		// This function is equivalent to using what used to be LoadPNG32. LoadPNG8 also existed,
		// but this only seemed to be used by the RMG system which does not work in JKA. If this
		// does need to be re-implemented, then colortype should be PNG_COLOR_TYPE_PALETTE or
		// PNG_COLOR_TYPE_GRAY.
		if ( colortype != PNG_COLOR_TYPE_RGB && colortype != PNG_COLOR_TYPE_RGBA )
		{
			ri.Printf (PRINT_ERROR, "Image is not 24-bit or 32-bit.");
			return 0;
		}

		// Read the png data
		if ( colortype == PNG_COLOR_TYPE_RGB )
		{
			// Expand RGB -> RGBA
			png_set_add_alpha (png_ptr, 0xff, PNG_FILLER_AFTER);
		}

		png_read_update_info (png_ptr, info_ptr);

		// We always assume there are 4 channels. RGB channels are expanded to RGBA when read.
		byte *tempData = (byte *)ri.Z_Malloc (width_ * height_ * 4, TAG_TEMP_PNG, qfalse, 4);
		if ( !tempData )
		{
			ri.Printf (PRINT_ERROR, "Could not allocate enough memory to load the image.");
			return 0;
		}

		// Dynamic array of row pointers, with 'height' elements, initialized to NULL.
		byte **row_pointers = (byte **)ri.Hunk_AllocateTempMemory (sizeof (byte *) * height_);
		if ( !row_pointers )
		{
			ri.Printf (PRINT_ERROR, "Could not allocate enough memory to load the image.");

			ri.Z_Free (tempData);

			return 0;
		}

		// Re-set the jmp so that these new memory allocations can be reclaimed
		if ( setjmp (png_jmpbuf (png_ptr)) )
		{
			ri.Hunk_FreeTempMemory (row_pointers);
			ri.Z_Free (tempData);
			return 0;
		}

		for ( unsigned int i = 0, j = 0; i < height_; i++, j += 4 )
		{
			row_pointers[i] = tempData + j * width_;
		}

		png_read_image (png_ptr, row_pointers);

		// Finish reading
		png_read_end (png_ptr, NULL);

		ri.Hunk_FreeTempMemory (row_pointers);

		// Finally assign all the parameters
		*data = tempData;
		*width = width_;
		*height = height_;

		return 1;
	}

	void ReadBytes ( void *dest, size_t len )
	{
		memcpy (dest, buf + offset, len);
		offset += len;
	}

private:
	char *buf;
	size_t offset;
	png_structp png_ptr;
	png_infop info_ptr;
};

void user_read_data( png_structp png_ptr, png_bytep data, png_size_t length ) {
	png_voidp r = png_get_io_ptr (png_ptr);
	PNGFileReader *reader = (PNGFileReader *)r;
	reader->ReadBytes (data, length);
}

// Loads a PNG image from file.
void LoadPNG ( const char *filename, byte **data, int *width, int *height )
{
	char *buf = NULL;
	int len = ri.FS_ReadFile (filename, (void **)&buf);
	if ( len < 0 || buf == NULL )
	{
		return;
	}

	PNGFileReader reader (buf);
	reader.Read (data, width, height);
}


#ifdef OPENJK_IOS
#include <limits.h>

struct PNGMemoryState {
	const byte *bytes;
	size_t length, offset;
	byte *pixels;
	png_bytep *rows;
};

static void PNGMemoryRead( png_structp png, png_bytep output, png_size_t count ) {
	PNGMemoryState *state = (PNGMemoryState *)png_get_io_ptr( png );
	if ( state->offset > state->length || count > state->length - state->offset )
		png_error( png, "Truncated memory PNG" );
	memcpy( output, state->bytes + state->offset, count );
	state->offset += count;
}

static void PNGMemoryFree( PNGMemoryState *state ) {
	if ( state->rows ) ri.Z_Free( state->rows );
	if ( state->pixels ) ri.Z_Free( state->pixels );
	ri.Z_Free( state );
}

int LoadPNGFromMemory( const byte *bytes, size_t length, byte **data, int *width, int *height ) {
	if ( data ) *data = NULL;
	if ( width ) *width = 0;
	if ( height ) *height = 0;
	if ( !data || !width || !height || !bytes || length < 8 || png_sig_cmp( bytes, 0, 8 ) )
		return 0;
	// Cleanup storage must survive libpng's longjmp.
	PNGMemoryState *state = (PNGMemoryState *)ri.Z_Malloc( (int)(sizeof( PNGMemoryState )), TAG_TEMP_PNG, qtrue, 4 );
	if ( !state ) return 0;
	state->bytes = bytes;
	state->length = length;
	state->offset = 8;
	png_structp png = png_create_read_struct( PNG_LIBPNG_VER_STRING, NULL, png_print_error, png_print_warning );
	if ( !png ) {
		PNGMemoryFree( state );
		return 0;
	}
	png_infop info = png_create_info_struct( png );
	if ( !info ) {
		png_destroy_read_struct( &png, NULL, NULL );
		PNGMemoryFree( state );
		return 0;
	}
	if ( setjmp( png_jmpbuf( png ) ) ) {
		png_destroy_read_struct( &png, &info, NULL );
		PNGMemoryFree( state );
		return 0;
	}
	png_set_read_fn( png, state, PNGMemoryRead );
	png_set_sig_bytes( png, 8 );
	png_set_crc_action( png, PNG_CRC_ERROR_QUIT, PNG_CRC_ERROR_QUIT );
#ifdef PNG_SET_USER_LIMITS_SUPPORTED
	png_set_user_limits( png, 4096, 4096 );
	png_set_chunk_malloc_max( png, 1024 * 1024 );
#endif
	png_read_info( png, info );
	png_uint_32 w, h;
	int depth, colour, interlace;
	png_get_IHDR( png, info, &w, &h, &depth, &colour, &interlace, NULL, NULL );
	if ( !w || !h || w > 4096 || h > 4096 || ( w & ( w - 1 ) ) || ( h & ( h - 1 ) ) ||
		depth != 8 || ( colour != PNG_COLOR_TYPE_RGB && colour != PNG_COLOR_TYPE_RGBA ) || interlace != PNG_INTERLACE_NONE )
		png_error( png, "Unsupported memory PNG format" );
	if ( (size_t)w > (size_t)INT_MAX / 4 / h || (size_t)h > (size_t)INT_MAX / sizeof( png_bytep ) )
		png_error( png, "Memory PNG allocation overflow" );
	if ( colour == PNG_COLOR_TYPE_RGB )
		png_set_add_alpha( png, 255, PNG_FILLER_AFTER );
	png_read_update_info( png, info );
	if ( png_get_bit_depth( png, info ) != 8 || png_get_channels( png, info ) != 4 || png_get_rowbytes( png, info ) != (size_t)w * 4 )
		png_error( png, "Invalid memory PNG rows" );
	state->pixels = (byte *)ri.Z_Malloc( (int)((size_t)w * h * 4), TAG_TEMP_PNG, qfalse, 4 );
	if ( !state->pixels ) png_error( png, "Memory PNG pixel allocation failed" );
	state->rows = (png_bytep *)ri.Z_Malloc( (int)(sizeof( png_bytep ) * h), TAG_TEMP_PNG, qfalse, 4 );
	if ( !state->rows ) png_error( png, "Memory PNG row allocation failed" );
	for ( png_uint_32 row = 0; row < h; ++row )
		state->rows[row] = state->pixels + (size_t)row * w * 4;
	png_read_image( png, state->rows );
	png_read_end( png, NULL );
	*data = state->pixels;
	*width = (int)w;
	*height = (int)h;
	state->pixels = NULL;
	png_destroy_read_struct( &png, &info, NULL );
	PNGMemoryFree( state );
	return 1;
}
#endif
