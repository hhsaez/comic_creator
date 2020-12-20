#include "log.hpp"

#include <algorithm>
#include <filesystem>
#include <future>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>
#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include <stb_image_resize.h>

using namespace comic_creator;

using Path = std::filesystem::path;
using PathArray = std::vector< Path >;

struct PageSize {
    float width;
    float height;
};

struct Settings {
    PageSize pageSize;
    PageSize trimSize;
};

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

std::vector< Path > fetchPages( void )
{
    std::vector< Path > ret;

    auto pagesDir = getRootDirectory() / "pages";
    for ( const auto &entry : std::filesystem::directory_iterator( pagesDir ) ) {
        if ( std::filesystem::is_directory( entry ) ) {
            continue;
        }
        ret.push_back( entry.path() );
    }

    std::sort( std::begin( ret ), std::end( ret ) );

    return ret;
}

std::shared_ptr< Image > loadImage( const std::filesystem::path &path ) noexcept
{
    if ( !std::filesystem::exists( path ) ) {
        Log::error( path, " does not exist" );
        return nullptr;
    }

    auto image = std::make_shared< Image >();
    image->name = path.filename();

    stbi_uc *pixels = stbi_load( path.c_str(), &image->width, &image->height, &image->channels, STBI_rgb_alpha );

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

void saveImage( std::shared_ptr< Image > const &image,
                std::filesystem::path const &path ) noexcept
{
    stbi_write_png( path.c_str(), image->width, image->height, image->channels, image->data.data(), image->width * image->channels );

    Log::debug( "Saved ", path );
}

void cutImage( const std::filesystem::path &path,
               const std::filesystem::path &outDir,
               const Settings &settings )
{
    auto src = loadImage( path );

    auto pageSize = settings.pageSize;
    auto trimSize = settings.trimSize;

    auto w0 = int( src->width );
    auto w1 = int( w0 * trimSize.width / pageSize.width );
    auto h0 = int( src->height );
    auto h1 = int( h0 * trimSize.height / pageSize.height );
    auto startX = int( 0.5f * ( w0 - w1 ) );
    auto startY = int( 0.5f * ( h0 - h1 ) );

    auto dst = std::make_shared< Image >();
    dst->name = src->name;
    dst->width = w1;
    dst->height = h1;
    dst->channels = src->channels;
    dst->data.resize( dst->width * dst->height * dst->channels );

    for ( auto y1 = 0; y1 < h1; y1++ ) {
        for ( auto x1 = 0; x1 < w1; x1++ ) {
            for ( auto c = 0; c < dst->channels; c++ ) {
                auto x0 = startX + x1;
                auto y0 = startY + y1;
                auto idx0 = ( y0 * w0 + x0 ) * src->channels + c;
                auto idx1 = ( y1 * w1 + x1 ) * dst->channels + c;
                dst->data[ idx1 ] = src->data[ idx0 ];
            }
        }
    }

    saveImage( dst, outDir / path.filename() );
}

void resizeImage( const std::filesystem::path &path,
                  const std::filesystem::path &outDir,
                  const Settings &settings )
{
    auto src = loadImage( path );

    auto w0 = int( src->width );
    auto w1 = int( 1024 );
    auto h0 = int( src->height );
    auto h1 = w1 * h0 / w0;

    auto dst = std::make_shared< Image >();
    dst->name = src->name;
    dst->width = w1;
    dst->height = h1;
    dst->channels = src->channels;
    dst->data.resize( dst->width * dst->height * dst->channels );

    stbir_resize_uint8( src->data.data(), w0, h0, 0, dst->data.data(), w1, h1, 0, dst->channels );

    saveImage( dst, outDir / path.filename() );
}

Path createOnlineImages( const PathArray &pages,
                         const Settings &settings ) noexcept
{
    Log::trace( "Creating images for online publishing" );

    auto dst = createDirectory( getRootDirectory() / "online" );

    std::vector< std::future< void > > tasks;
    std::transform(
        std::begin( pages ),
        std::end( pages ),
        std::back_inserter( tasks ),
        [ dst, settings ]( const Path &path ) {
            return std::async( std::launch::async, resizeImage, path, dst, settings );
        } );

    return dst;
}

void printPages( const Path &left, const Path &right, const Path &dst, const Settings &settings )
{
    Log::trace( "Printing ", left.filename(), " and ", right.filename() );

    auto pageSize = settings.pageSize;
    auto trimSize = settings.trimSize;

    auto leftImage = loadImage( left );
    auto rightImage = loadImage( right );

    auto w0 = int( leftImage->width );
    auto w1 = int( w0 * trimSize.width / pageSize.width );
    auto h0 = int( leftImage->height );
    auto h1 = int( h0 * trimSize.height / pageSize.height );
    auto marginX = int( 0.5 * ( w0 - w1 ) );
    auto marginY = int( 0.5 * ( h0 - h1 ) );

    auto page = std::make_shared< Image >();
    page->name = dst.filename();
    page->width = 2 * leftImage->width;
    page->height = leftImage->height;
    page->channels = leftImage->channels;
    page->data.resize( page->width * page->height * page->channels );

    memset( page->data.data(), 0xff, page->data.size() );

    auto pageStride = page->width * page->channels;
    auto imageStride = leftImage->width * leftImage->channels;

    for ( auto y = 0l; y < page->height; y++ ) {
        for ( auto x = 0l; x < leftImage->width; x++ ) {
            for ( auto c = 0; c < leftImage->channels - 1; c++ ) {
                auto idx0 = y * pageStride + x * leftImage->channels + c;
                auto idx1 = y * imageStride + x * leftImage->channels + c;
                page->data[ idx0 ] = leftImage->data[ idx1 ];
            }
        }
        for ( auto x = 0l; x < rightImage->width; x++ ) {
            for ( auto c = 0; c < rightImage->channels - 1; c++ ) {
                auto idx0 = y * pageStride + imageStride + x * page->channels + c;
                auto idx1 = y * imageStride + x * page->channels + c;
                page->data[ idx0 ] = rightImage->data[ idx1 ];
            }
        }
    }

    saveImage( page, dst );
}

Path createPrintingImages( const PathArray &pages,
                           const Settings &settings ) noexcept
{
    if ( pages.size() % 4 != 0 ) {
        Log::error( "Cannot create printing layout. Incorrect number of pages" );
        return Path();
    }

    Log::trace( "Creating images for printing" );

    auto dst = createDirectory( getRootDirectory() / "print" );

    std::vector< std::future< void > > tasks;

    for ( auto i = 0; i < pages.size() / 2; i++ ) {
        auto left = pages[ pages.size() - 1 - i ];
        auto right = pages[ i ];

        std::stringstream ss;
        ss << "page_" << i << ".png";

        tasks.push_back( std::async( std::launch::async, printPages, left, right, dst / ss.str(), settings ) );
    }

    return dst;
}

int main( int argc, char **argv )
{
    Log::info( "ComicCreator ", COMIC_CREATOR_VERSION );

    auto settings = Settings {
        .pageSize = { .width = 29.70f, .height = 42.00f },
        .trimSize = { .width = 29.70f, .height = 42.00f },
    };

    auto pages = fetchPages();
    if ( pages.size() == 0 ) {
        Log::error( "Cannot fetch pages" );
        return -1;
    }

    createOnlineImages( pages, settings );
    // createPrintingImages( pages, settings );

    return 0;
}
