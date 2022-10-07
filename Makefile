all:
	arm-linux-gnueabihf-gcc src/main.c src/getstream.c -o build/getstreamAPP \
	-I include \
	-L lib \
	-lavcodec -lavdevice -lavfilter -lavformat -lavutil -lpostproc -lswresample -lswscale

clean:
	rm build/*

copy:
	sudo cp build/getstreamAPP /home/ubuntu20/workdir/nfsdir/rootfs/home/greenqueen/ -f