#!/bin/sh
cd "$(dirname "$0")"

dataDir=$1

mkdir -p $dataDir/icons

echo "Clone breeze iconset"
cp -fr /system/data/icons/breeze $dataDir/icons/haiku
cp -f --remove-destination index.theme $dataDir/icons/haiku

echo "Install actions icons"
cp -f --remove-destination actions/*.svg $dataDir/icons/haiku/actions/22
rm -rf $dataDir/icons/haiku/actions/12 \
	$dataDir/icons/haiku/actions/16 \
	$dataDir/icons/haiku/actions/24 \
	$dataDir/icons/haiku/actions/32 \
	$dataDir/icons/haiku/actions/symbolic
ln -rfs $dataDir/icons/haiku/actions/22 $dataDir/icons/haiku/actions/12
ln -rfs $dataDir/icons/haiku/actions/22 $dataDir/icons/haiku/actions/16
ln -rfs $dataDir/icons/haiku/actions/22 $dataDir/icons/haiku/actions/24
ln -rfs $dataDir/icons/haiku/actions/22 $dataDir/icons/haiku/actions/32

echo "Install categories icons"
cp -f --remove-destination categories/*.svg $dataDir/icons/haiku/categories/32

echo "Install devices icons"
cp -f --remove-destination devices/*.svg $dataDir/icons/haiku/devices/22
rm -rf $dataDir/icons/haiku/devices/16 \
	$dataDir/icons/haiku/devices/64 \
	$dataDir/icons/haiku/devices/symbolic
ln -rfs $dataDir/icons/haiku/devices/22 $dataDir/icons/haiku/devices/16
ln -rfs $dataDir/icons/haiku/devices/22 $dataDir/icons/haiku/devices/64

echo "Install mimetypes icons"
cp -f --remove-destination mimetypes/*.svg $dataDir/icons/haiku/mimetypes/22
rm -rf $dataDir/icons/haiku/mimetypes/16 \
	$dataDir/icons/haiku/mimetypes/32 \
	$dataDir/icons/haiku/mimetypes/64
ln -rfs $dataDir/icons/haiku/mimetypes/22 $dataDir/icons/haiku/mimetypes/16
ln -rfs $dataDir/icons/haiku/mimetypes/22 $dataDir/icons/haiku/mimetypes/32
ln -rfs $dataDir/icons/haiku/mimetypes/22 $dataDir/icons/haiku/mimetypes/64

echo "Install places icons"
cp -f --remove-destination places/*.svg $dataDir/icons/haiku/places/22
rm -rf $dataDir/icons/haiku/places/16 \
	$dataDir/icons/haiku/places/32 \
	$dataDir/icons/haiku/places/64 \
	$dataDir/icons/haiku/places/symbolic
ln -rfs $dataDir/icons/haiku/places/22 $dataDir/icons/haiku/places/16
ln -rfs $dataDir/icons/haiku/places/22 $dataDir/icons/haiku/places/32
ln -rfs $dataDir/icons/haiku/places/22 $dataDir/icons/haiku/places/64

echo "Install status icons"
cp -f --remove-destination status/*.svg $dataDir/icons/haiku/status/16
cp -f --remove-destination status/*.svg $dataDir/icons/haiku/status/22
cp -f --remove-destination status/*.svg $dataDir/icons/haiku/status/64
rm -rf $dataDir/icons/haiku/status/24 \
	$dataDir/icons/haiku/status/symbolic
ln -rfs $dataDir/icons/haiku/status/22 $dataDir/icons/haiku/status/24

echo "Install emblems icons"
cp -f --remove-destination emblems/*.svg $dataDir/icons/haiku/emblems/8
cp -f --remove-destination emblems/*.svg $dataDir/icons/haiku/emblems/16
cp -f --remove-destination emblems/*.svg $dataDir/icons/haiku/emblems/22
