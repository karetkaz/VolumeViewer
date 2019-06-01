#include "volume.h"
#include "voxel_float1.h"

#include <png.h>
#include <itkImage.h>
#include <itkIOCommon.h>
#include <itkImageFileReader.h>

#include <itkNiftiImageIO.h>
#include <itkNiftiImageIOFactory.h>

// read 16 bit png image
void readPng(const string &file, Volume<float1> &volume, unsigned z) {
	FILE *fp = fopen(file.c_str(), "rb");
	if (fp == NULL) {
		throw runtime_error("Failed to open file: " + file);
	}

	png_structp png = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	if (png == NULL) {
		fclose(fp);
		throw runtime_error("Failed to create_read_struct: " + file);
	}
	
	png_infop info = png_create_info_struct(png);
	if (info == NULL) {
		fclose(fp);
		png_destroy_read_struct(&png, &info, NULL);
		throw runtime_error("Failed to create_info_struct: " + file);
	}
	
	if (setjmp(png_jmpbuf(png))) {
		fclose(fp);
		png_destroy_read_struct(&png, &info, NULL);
		throw runtime_error("Failed to read png image: " + file);
	}

	png_init_io(png, fp);
	png_read_info(png, info);

	int width      = png_get_image_width(png, info);
	int height     = png_get_image_height(png, info);
	int color_type = png_get_color_type(png, info);
	int bit_depth  = png_get_bit_depth(png, info);
	
	// Read any color_type into 8bit depth, RGBA format.
	// See http://www.libpng.org/pub/png/libpng-manual.txt
	
	if (bit_depth != 16 || color_type != PNG_COLOR_TYPE_GRAY) {
		fclose(fp);
		png_destroy_read_struct(&png, &info, NULL);
		throw runtime_error("only 16 bit gray png is supported: " + file);
	}

	png_bytep *row_pointers = (png_bytep*)malloc(sizeof(png_bytep) * height);
	for(int y = 0; y < height; y++) {
		row_pointers[y] = (png_byte*)malloc(png_get_rowbytes(png,info));
	}

	png_set_swap(png);
	png_read_image(png, row_pointers);
	png_destroy_read_struct(&png, &info, NULL);

	for(int y = 0; y < height; y++) {
		png_uint_16p buff = (png_uint_16p)row_pointers[y];
		for (int x = 0; x < width; x++) {
			volume.set(x, y, z, float1(buff[x]));
		}
		free(row_pointers[y]);
	}
	free(row_pointers);
	fclose(fp);
}
// write 16 bit png image
void writePng(const string &file, const Volume<float1> &volume, unsigned z) {
	FILE *fp = fopen(file.c_str(), "wb");
	if (fp == NULL) {
		throw runtime_error("Failed to open file: " + file);
	}

	png_structp png = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	if (png == NULL) {
		fclose(fp);
		throw runtime_error("Failed to create_write_struct: " + file);
	}

	png_infop info = png_create_info_struct(png);
	if (info == NULL) {
		fclose(fp);
		png_destroy_write_struct(&png, &info);
		throw runtime_error("Failed to create_write_struct: " + file);
	}

	int width = volume.width();
	int height = volume.height();
	if (setjmp(png_jmpbuf(png))) {
		fclose(fp);
		png_destroy_write_struct(&png, &info);
		throw runtime_error("Failed to write png image: " + file);
	}

	png_init_io(png, fp);
	png_set_IHDR(png, info,
		width, height,
		16, PNG_COLOR_TYPE_GRAY,
		PNG_INTERLACE_NONE,
		PNG_COMPRESSION_TYPE_BASE,
		PNG_FILTER_TYPE_BASE
	);

	png_set_swap(png);

	png_bytepp row_pointers = (png_bytepp)malloc(sizeof(png_uint_16p) * height);
	for(int y = 0; y < height; y++) {
		row_pointers[y] = (png_bytep)malloc(png_get_rowbytes(png, info));
		png_uint_16p buff = (png_uint_16p)row_pointers[y];
		for (int x = 0; x < width; x++) {
			buff[x] = 65535 * volume.get(x, y, z).value;
		}
	}

	png_set_rows(png, info, row_pointers);
	png_write_png(png, info, PNG_TRANSFORM_SWAP_ENDIAN, NULL);
	png_destroy_write_struct(&png, &info);

	for(int y = 0; y < height; y++) {
		free(row_pointers[y]);
	}
	free(row_pointers);
	fclose(fp);
}

// read NIFTI volume file using itk library
void readNIFTI(const string &file, Volume<float1> &volume) {
	static int initialized = 0;
	if (initialized == 0) {
		itk::NiftiImageIOFactory::RegisterOneFactory();
		initialized = 1;
	}

	typedef itk::Image< short, 3 > ImageType;
	typedef itk::ImageFileReader<ImageType> ReaderType;
	ReaderType::Pointer reader = ReaderType::New();
	reader->SetFileName(file);
	reader->Update();
	ImageType::Pointer image=reader->GetOutput();
	ImageType::RegionType region = image->GetLargestPossibleRegion();
	ImageType::SizeType size = region.GetSize();
	ImageType::IndexType index;
	int xmid = (volume.width() - size[0]) / 2;
	int ymid = (volume.height() - size[1]) / 2;
	int zmid = (volume.depth() - size[2]) / 2;
	for (int z = 0; z < size[2]; z++) {
		index[2] = z;
		for (int y = 0; y < size[1]; y++) {
			index[1] = y;
			for (int x = 0; x < size[0]; x++) {
				index[0] = x;
				float value = image->GetPixel(index);
				volume.set(x + xmid, y + ymid, z + zmid, float1(value));
			}
		}
	}
}
