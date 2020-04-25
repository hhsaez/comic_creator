add_executable(
	ComicCreator
	main.cpp
)

target_include_directories(
	ComicCreator
	PUBLIC ${PROJECT_SOURCE_DIR}/third-party/stb
)

install(
	TARGETS ComicCreator
	RUNTIME
		DESTINATION bin
		PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE
)

