# This project existed only to build the bzip2/lzma/ppmd/zlib subprojects under
# 3rdparty/. Those sources are now Algos amalgamations compiled directly by
# xarchive.pri, so there are no subdirectories left to build.
#
# The file is kept because it is the conventional entry point for the module;
# consumers include xarchive.pri, which carries the sources.
TEMPLATE      = subdirs
