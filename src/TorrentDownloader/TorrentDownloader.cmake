# find_package(LibtorrentRasterbar REQUIRED)
# find_package(GLog REQUIRED)

# file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS
#     "${CMAKE_CURRENT_LIST_DIR}/*.cpp"
#     "${CMAKE_CURRENT_LIST_DIR}/*.h"
# )
# add_library(TorrentDownloader ${SOURCES})

# target_link_libraries(TorrentDownloader
#     LibtorrentRasterbar::torrent-rasterbar
#     glog::glog
# )