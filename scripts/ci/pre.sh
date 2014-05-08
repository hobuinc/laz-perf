#!/bin/bash -e
# Installs requirements for las-perf
# Borrowed from PDAL's pre-script.

sudo apt-key adv --recv-keys --keyserver keyserver.ubuntu.com 16126D3A3E5C1192
sudo apt-get update -y
sudo apt-get install software-properties-common -y
sudo apt-get install python-software-properties -y
sudo add-apt-repository ppa:boost-latest/ppa -y

# g++4.8.1
if [ "$CXX" == "g++" ]; then sudo add-apt-repository -y ppa:ubuntu-toolchain-r/test; fi

# clang 3.4
if [ "$CXX" == "clang++" ]; then sudo add-apt-repository -y ppa:h-rayflood/llvm; fi

sudo apt-get update -qq
sudo apt-get install \
	cmake \
	libboost-test1.55-dev

# g++4.8.1
if [ "$CXX" = "g++" ]; then 
	sudo apt-get install -qq g++-4.8;
	export CXX="g++-4.8";
fi

# clang 3.4
if [ "$CXX" == "clang++" ]; then 
	sudo apt-get install --allow-unauthenticated -qq clang-3.4;
	export CXX="clang++-3.4";

	# Download and install libc++
	export CXXFLAGS="-std=c++0x -stdlib=libc++" ;
	svn co --quiet http://llvm.org/svn/llvm-project/libcxx/trunk libcxx ;

	cd libcxx/lib && bash buildit;
	sudo cp ./libc++.so.1.0 /usr/lib/;
	sudo mkdir /usr/include/c++/v1;
	cd .. && sudo cp -r include/* /usr/include/c++/v1/;
	cd /usr/lib && sudo ln -sf libc++.so.1.0 libc++.so;
	sudo ln -sf libc++.so.1.0 libc++.so.1 && cd $cwd;
fi

cd $TRAVIS_BUILD_DIR
