#include <iostream>
#include <filesystem>
#include <vector>
#include <string>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

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
		std::cerr << path << "Does not exists\n";
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
		std::cerr << "Failed to load image " << path << "\n";
		return nullptr;
	}

	image->data.resize( image->width * image->height * image->channels );
	memcpy( image->data.data(), pixels, sizeof( unsigned char ) * image->data.size() );

	stbi_image_free( pixels );

	std::cout << "Loaded " << path << "\n";

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

	std::cout << "Saved " << path.c_str() << "\n";
}

void cutImage( const std::filesystem::path &path, const std::filesystem::path &outDir )
{
	auto src = loadImage( path );
	saveImage( src, outDir / path.filename() );
}

std::filesystem::path createOnlineImages( const std::filesystem::path &src ) noexcept
{
	std::cout << "Creating images for ONLINE publishing\n";

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
	std::cout << "Creating images for PRINTING\n";
	auto dst = createDirectory( getRootDirectory() / "print" );

	return dst;
}

int main( int argc, char **argv )
{
	std::cout << "ComicCreator " << COMIC_CREATOR_VERSION << "\n";

	auto currentDir = std::filesystem::current_path();
	auto pagesDir = currentDir / "pages";
	if ( !std::filesystem::exists( pagesDir ) ) {
		std::cerr << "Cannot find " << pagesDir << "\n";
		return -1;
	}

	createOnlineImages( pagesDir );
	createPrintingImages( pagesDir );
	
	return 0;
}

