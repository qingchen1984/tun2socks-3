echo "build 3rd library ..."

export ROOT_DIR=`pwd`
INSTALL_DIR="$ROOT_DIR/pkg"
PKG_DIR="pkg/packages"

if [ ! -d $PKG_DIR ]; then
  mkdir -p $PKG_DIR
fi

if [ $# -ne 1 ]; then
    echo "build_deps.sh <libuv|libnet>"
    exit -1
fi

if [ $1 == 'libuv' ]; then

	# build libevent
	#
	echo ">>>>>--------> build libuv ..."
	UV_VERSION="1.41.0"
	if [ ! -f $PKG_DIR/libuv-v$UV_VERSION.tar.gz ]; then 
		wget https://dist.libuv.org/dist/v$UV_VERSION/libuv-v$UV_VERSION.tar.gz --output-document=$PKG_DIR/libuv-v$UV_VERSION.tar.gz
	fi
	
	cd $PKG_DIR/
	tar -zxvf libuv-v$UV_VERSION.tar.gz

	cd libuv-v$UV_VERSION/

	echo "make..."
	sh autogen.sh
	./configure --prefix=$INSTALL_DIR
	make
	
	echo "install..."
	make install

	echo "clean..."
	cd ../
	rm -rf libuv-v$UV_VERSION/

elif [ $1 == 'libnet' ]; then

	# build libnet
	#
	echo ">>>>>--------> build libnet ..."

	if [ ! -f $PKG_DIR/libnet-1.2.tar.gz ]; then 
		wget https://github.com/libnet/libnet/releases/download/v1.2/libnet-1.2.tar.gz --output-document=$PKG_DIR/libnet-1.2.tar.gz
	fi

	cd $PKG_DIR/
	tar -zxvf libnet-1.2.tar.gz

	cd libnet-1.2/

	echo "configure..."
	./configure --prefix=$INSTALL_DIR

	echo "make install ..."
	make install

	echo "clean..."
	cd ..
	echo $INSTALL_DIR
	# rm -rf zlog-1.2.15/

fi
