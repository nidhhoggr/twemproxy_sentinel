all: build

mkdir_build:
	rm -rf build
	mkdir -p build
	rm -rf configure/yaml-0.1.4
build: mkdir_build
	cd contrib; tar -xvzf yaml-0.1.4.tar.gz; cd yaml-0.1.4; ./configure; make; make install;
	cd ..;
	cd build; cmake \
	-DLIBYAML_INCLUDE_DIR=../contrib/yaml-0.1.4/include \
	..
	make -C build

run:
	cd build; ./twemproxy_sentinel
