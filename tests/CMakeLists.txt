# Every .cpp under tests/ is part of the test executable, and every xfail.txt under
# tests/ contributes expected failures, so a new test file needs no edit here.
file(GLOB_RECURSE SURREALTESTS_SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)
file(GLOB_RECURSE SURREALTESTS_XFAIL_FILES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/xfail.txt)

add_executable(SurrealTests ${SURREALTESTS_SOURCES})
target_include_directories(SurrealTests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(SurrealTests SurrealCommon ${SURREALCOMMON_LIBS})
set_target_properties(SurrealTests PROPERTIES CXX_STANDARD 20)

set(XFAIL_NAMES)
set(XFAIL_ARGS)
foreach(xfail_file IN LISTS SURREALTESTS_XFAIL_FILES)
	list(APPEND XFAIL_ARGS --xfail-file ${xfail_file})
	file(STRINGS ${xfail_file} XFAIL_LINES)
	foreach(line IN LISTS XFAIL_LINES)
		string(REGEX REPLACE "#.*$" "" line "${line}")
		string(STRIP "${line}" line)
		if(NOT line STREQUAL "")
			list(APPEND XFAIL_NAMES ${line})
		endif()
	endforeach()
endforeach()

# Each TEST(name) becomes its own ctest entry so a crash cannot hide other tests.
# Names listed in an xfail.txt are registered as XFAIL.<name>; the runner itself reports
# XFAIL/XPASS from the same files.
set(TEST_COUNT 0)
foreach(src IN LISTS SURREALTESTS_SOURCES)
	file(STRINGS ${src} TEST_LINES REGEX "^TEST\\(")
	foreach(line IN LISTS TEST_LINES)
		string(REGEX REPLACE "^TEST\\(([A-Za-z0-9_]+)\\).*$" "\\1" test_name "${line}")
		set(ctest_name ${test_name})
		if(test_name IN_LIST XFAIL_NAMES)
			set(ctest_name XFAIL.${test_name})
		endif()
		add_test(NAME ${ctest_name}
			COMMAND SurrealTests --test ${test_name} ${XFAIL_ARGS})
		math(EXPR TEST_COUNT "${TEST_COUNT} + 1")
	endforeach()
endforeach()

# A test that compiles but never registers (or an xfail entry naming no test) goes red here.
add_test(NAME RegistryMatchesSources
	COMMAND SurrealTests --expect-count ${TEST_COUNT} ${XFAIL_ARGS})
