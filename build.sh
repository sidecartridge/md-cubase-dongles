#!/bin/bash

# Get the absolute path of the current script
SCRIPT_DIR=$(dirname "$(realpath "$0")") 

# Ensure all required arguments are provided
if [ -z "$1" ] || [ -z "$2" ] || [ -z "$3" ]; then
    echo "Usage: $0 <board_type> <build_type> <app_uuid_key>"
    echo "Example: $0 pico|pico_w debug|release 123e4567-e89b-12d3-a456-426614174000"
    exit 1
fi

# Copy the version.txt to each project
echo "Copy version.txt to each project"
cp version.txt rp/
cp version.txt target/

# Display the version information
export VERSION=$(cat version.txt)
echo "Version: $VERSION"

# Set the board type to be used for building
export BOARD_TYPE=$1
echo "Board type: $BOARD_TYPE"

# Set the release or debug build type
export BUILD_TYPE=$2
echo "Build type: $BUILD_TYPE"

# Set the APP_UUID_KEY of the app to be built
export APP_UUID_KEY=$3
echo "App UUID Key: $APP_UUID_KEY"

# Set the dist directory. Delete previous contents if any
echo "Delete previous dist directory"
rm -rf dist
mkdir dist

# Build the project in the target architecture.
# Pin the atarist-toolkit-docker image tag to a published one for the m68k build
# only. The stcmd wrappers derive the tag from $VERSION (older CI wrapper) or
# $STCMD_IMAGE_TAG (newer), and our $VERSION is the *app* version (e.g. v1.2.1),
# which is not a published toolkit image — so `stcmd make` fails to pull it.
# Overriding both for this subprocess is safe: the toolkit's own Makefile reads
# target/atarist/version.txt, not this $VERSION.
echo "Building target project"
cd target/atarist
VERSION=latest STCMD_IMAGE_TAG=latest ./build.sh "$SCRIPT_DIR/target/atarist" release
target_status=$?
cd ../..
if [ "$target_status" -ne 0 ]; then
    echo "ERROR: Atari ST (m68k) build failed (status $target_status)."
    echo "       Aborting before the RP build so a stale target_firmware.h is"
    echo "       never embedded into the firmware."
    exit "$target_status"
fi
echo "Done building target project"

# Build the rp project in the RP architecture
echo "Building rp project"
cd rp
./build.sh "$BOARD_TYPE" "$BUILD_TYPE"
if [ "$BUILD_TYPE" = "release" ]; then
    cp  ./dist/rp-$BOARD_TYPE.uf2 ../dist/rp.uf2
else
    cp  ./dist/rp-$BOARD_TYPE-$BUILD_TYPE.uf2 ../dist/rp.uf2
fi
cd ..
echo "Done building rp project"

# Calculate the md5sum of the generated rp.uf2 file
md5sum dist/rp.uf2 > dist/rp.uf2.md5sum

# Show the md5sum of the generated rp.uf2 file
echo "md5sum of the generated rp.uf2 file:"
cat dist/rp.uf2.md5sum

# Now inform the user that the build is complet and must
# modify the app.json file with the new md5sum and the UUID
echo "Build completed successfully. Please update the app.json file with the new md5sum and the UUID"

# Rename the file to the standard name <APP_UUID>.uf2
mv dist/rp.uf2 dist/$APP_UUID_KEY.uf2

# Check that there is a app.json file in the dist directory
if [ ! -f desc/app.json ]; then
    echo "app.json file not found in the 'desc'' directory. Please create one."
    exit 1
fi

# Copy the app.json file to the dist directory
cp desc/app.json dist/

# Use portable sed for Linux and macOS
if [ "$(uname)" = "Darwin" ]; then
    sed -i '' "s/<APP_UUID>/$APP_UUID_KEY/g" dist/app.json
    sed -i '' "s/<BINARY_MD5_HASH>/$(cat dist/rp.uf2.md5sum | cut -d ' ' -f 1)/g" dist/app.json
    sed -i '' "s/<APP_VERSION>/$VERSION/g" dist/app.json
else
    sed -i "s/<APP_UUID>/$APP_UUID_KEY/g" dist/app.json
    sed -i "s/<BINARY_MD5_HASH>/$(cat dist/rp.uf2.md5sum | cut -d ' ' -f 1)/g" dist/app.json
    sed -i "s/<APP_VERSION>/$VERSION/g" dist/app.json
fi

mv dist/$APP_UUID_KEY.uf2 dist/$APP_UUID_KEY-$VERSION.uf2

# Show the content of the $APP_UUID_KEY.json file
echo "Content of the $APP_UUID_KEY.json file:"
mv dist/app.json dist/$APP_UUID_KEY.json
cat dist/$APP_UUID_KEY.json

# Done
exit 0
