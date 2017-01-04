all: build_wo_contrib

clean_build:
	rm -rf build
	mkdir -p build

clean_contrib:
	rm -rf contrib/yaml-0.1.4
	rm -rf contrib/hiredis
	rm -rf contrib/libevent

build_contrib_libyaml: 
	cd contrib; tar -xvzf yaml-0.1.4.tar.gz; cd yaml-0.1.4; ./configure; make;

build_contrib_libevent:
	cd contrib; git clone --branch 2.0.23-stable-rc https://github.com/libevent/libevent.git; \
	cd libevent; autoreconf --install; ./configure; make;

build_contrib_hiredis: build_contrib_libevent
	cd contrib; git clone https://github.com/redis/hiredis.git; \
	cd hiredis; git checkout 33152ad163a21f568fb40eeeb88b79365886b4ea; make;

build_wo_contrib: clean_build
	cd build; cmake ..
	make -C build

build: clean_build clean_contrib build_contrib_libyaml build_contrib_hiredis build_wo_contrib

run:
	cd build; ./twemproxy_sentinel
