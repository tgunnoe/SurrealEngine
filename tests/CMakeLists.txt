add_executable(SurrealTests
	main.cpp
	UtilsTests.cpp
	PackageTests.cpp
	VMTests.cpp
)
target_link_libraries(SurrealTests SurrealCommon ${SURREALCOMMON_LIBS})
set_target_properties(SurrealTests PROPERTIES CXX_STANDARD 20)

# Each TEST(name) becomes its own ctest entry so a crash cannot hide other tests.
# Names on xfail.txt are registered as XFAIL.<name>; the runner itself reports
# XFAIL/XPASS using the same file.
set(SURREALTESTS_XFAIL_FILE ${CMAKE_CURRENT_SOURCE_DIR}/xfail.txt)
file(STRINGS ${SURREALTESTS_XFAIL_FILE} XFAIL_LINES)
set(XFAIL_NAMES)
foreach(line IN LISTS XFAIL_LINES)
	string(REGEX REPLACE "#.*$" "" line "${line}")
	string(STRIP "${line}" line)
	if(NOT line STREQUAL "")
		list(APPEND XFAIL_NAMES ${line})
	endif()
endforeach()

set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${SURREALTESTS_XFAIL_FILE})
foreach(src UtilsTests.cpp PackageTests.cpp VMTests.cpp)
	set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${src})
	file(STRINGS ${CMAKE_CURRENT_SOURCE_DIR}/${src} TEST_LINES REGEX "^TEST\\(")
	foreach(line IN LISTS TEST_LINES)
		string(REGEX REPLACE "^TEST\\(([A-Za-z0-9_]+)\\).*$" "\\1" test_name "${line}")
		set(ctest_name ${test_name})
		if(test_name IN_LIST XFAIL_NAMES)
			set(ctest_name XFAIL.${test_name})
		endif()
		add_test(NAME ${ctest_name}
			COMMAND SurrealTests --test ${test_name} --xfail-file ${SURREALTESTS_XFAIL_FILE})
	endforeach()
endforeach()
