#ifndef COMIC_CREATOR_LOG_
#define COMIC_CREATOR_LOG_

#include <string>
#include <sstream>
#include <thread>
#include <iostream>

namespace comic_creator {

	class Log {
	public:
		template< typename ... Args >
		static void info( Args &&... args ) noexcept
		{
			getInstance().log( "I", std::forward< Args >( args )... );
		}
		
		template< typename ... Args >
		static void trace( Args &&... args ) noexcept
		{
			getInstance().log( "T", std::forward< Args >( args )... );
		}
		
		template< typename ... Args >
		static void debug( Args &&... args ) noexcept
		{
			getInstance().log( "D", std::forward< Args >( args )... );
		}
		
		template< typename ... Args >
		static void error( Args &&... args ) noexcept
		{
			getInstance().log( "E", std::forward< Args >( args )... );
		}
		
		template< typename ... Args >
		static void warning( Args &&... args ) noexcept
		{
			getInstance().log( "W", std::forward< Args >( args )... );
		}
		
	private:
		~Log( void ) = default;
		
		static Log &getInstance()
		{
			static Log instance;
			return instance;
		}

	private:
		template< typename ... Args >
		void log( std::string tag, Args &&... args )
		{
			auto lock = std::lock_guard< std::mutex >( logMutex );
			
			auto tp = std::chrono::system_clock::now();
			auto s = std::chrono::duration_cast< std::chrono::microseconds >( tp.time_since_epoch() );
			auto t = ( time_t )( s.count() );
			
			auto line = []( Args &&... args ) {
				std::stringstream ss;
				( void ) std::initializer_list< int > {
					(
						ss << args,
						0
					)...
					};
				return ss.str();
			}( std::forward< Args >( args )... );
			
			std::cout << t
					  << " " << std::this_thread::get_id()
					  << " " << tag
					  << " - "
					  << line
					  << "\n";
		}
		
		template< typename ... Args >
		void log_i( Args &&... args )
		{
			log( "I", std::forward< Args >( args )... );
		}
		
		template< typename ... Args >
		void log_t( Args &&... args )
		{
			log( "T", std::forward< Args >( args )... );
		}
		
		template< typename ... Args >
		void log_d( Args &&... args )
		{
			log( "D", std::forward< Args >( args )... );
		}
		
		template< typename ... Args >
		void log_w( Args &&... args )
		{
			log( "W", std::forward< Args >( args )... );
		}
		
		template< typename ... Args >
		void log_e( Args &&... args )
		{
			log( "E", std::forward< Args >( args )... );
		}
		
	private:
		std::mutex logMutex;
	};

}

#endif
	
