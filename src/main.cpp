#include "log.hpp"

#include <iostream>
#include <filesystem>
#include <vector>
#include <memory>
#include <string>
#include <sstream>
#include <algorithm>
#include <thread>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

using namespace comic_creator;

struct Image {
	std::string name;
	std::vector< unsigned char > data;
	int width;
	int height;
	int channels;
};

std::filesystem::path getRootDirectory( void ) noexcept
{
	return std::filesystem::current_path();
}

std::filesystem::path createDirectory( const std::filesystem::path &dir )
{
	if ( std::filesystem::exists( dir ) ) {
		std::filesystem::remove_all( dir );
	}

	std::filesystem::create_directories( dir );

	return dir;
}

std::shared_ptr< Image > loadImage( const std::filesystem::path &path ) noexcept
{
	if ( !std::filesystem::exists( path ) ) {
		Log::error( path, " does not exist" );
		return nullptr;
	}

	auto image = std::make_shared< Image >();
	image->name = path.filename();

	stbi_uc *pixels = stbi_load(
		path.c_str(),
		&image->width,
		&image->height,
		&image->channels,
		STBI_rgb_alpha
	);

	if ( pixels == nullptr ) {
		Log::error( "Failed to load image ", path );
		return nullptr;
	}

	image->data.resize( image->width * image->height * image->channels );
	memcpy( image->data.data(), pixels, sizeof( unsigned char ) * image->data.size() );

	stbi_image_free( pixels );

	Log::debug( "Loaded ", path );

	return image;
}

void saveImage( std::shared_ptr< Image > const &image, std::filesystem::path const &path ) noexcept
{
	stbi_write_png(
		path.c_str(),
		image->width,
		image->height,
		image->channels,
		image->data.data(),
		image->width * image->channels
	);

	Log::debug( "Saved ", path );
}

void cutImage( const std::filesystem::path &path, const std::filesystem::path &outDir )
{
	auto src = loadImage( path );
	saveImage( src, outDir / path.filename() );
}

std::filesystem::path createOnlineImages( const std::filesystem::path &src ) noexcept
{
	Log::trace( "Creating images for online publishing" );

	auto outDir = createDirectory( getRootDirectory() / "online" );
	for ( const auto &entry : std::filesystem::directory_iterator( src ) ) {
		if ( std::filesystem::is_directory( entry ) ) {
			continue;
		}
		cutImage( entry.path(), outDir );
	}

	return outDir;
}

std::filesystem::path createPrintingImages( const std::filesystem::path &src )
{
	Log::trace( "Creating images for printing" );

	auto dst = createDirectory( getRootDirectory() / "print" );

	return dst;
}

int main( int argc, char **argv )
{
	Log::info( "ComicCreator ", COMIC_CREATOR_VERSION );

	auto currentDir = std::filesystem::current_path();
	auto pagesDir = currentDir / "pages";
	if ( !std::filesystem::exists( pagesDir ) ) {
		Log::error( "Cannot find ", pagesDir );
		return -1;
	}

	createOnlineImages( pagesDir );
	createPrintingImages( pagesDir );
	
	return 0;
}

