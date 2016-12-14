C0=/dev/ttyUSB1

# only works for the both upper usb ports

DEV_C0=`/bin/udevadm info --name=$C0 | grep DEVPATH | sed 's/.*usb.\/\(.\)-\(.\).*/\1:\2/g'`

if [ -z "$DEV_C0" ]
then
    echo "Not connected"
    exit 1
fi

echo $DEV_C0

cat >Dockerfile << EOL
FROM ubuntu:xenial
RUN apt-get update && apt-get install -y wget git bzip2 build-essential libtool automake autotools-dev pkg-config
RUN git clone http://repo.or.cz/openocd.git /openocd
RUN cd / && wget -O /libusb.tar.bz2 "http://downloads.sourceforge.net/project/libusb/libusb-1.0/libusb-1.0.20/libusb-1.0.20.tar.bz2" && tar -xjf /libusb.tar.bz2 
RUN apt-get install -y libudev-dev
RUN cd /libusb* && ./configure && make && make install && ldconfig
RUN cd /openocd && ./bootstrap && ./configure && make && make install
RUN echo "interface ftdi\nftdi_vid_pid 0x0403 0x6010\nftdi_layout_init 0x0c08 0x0c2b\nftdi_layout_signal nTRST -data 0x0800\nftdi_layout_signal nSRST -data 0x0400\nsource [find target/stm32f1x.cfg]\nadapter_khz 1500\ngdb_port 3456\ntelnet_port 0\ntcl_port 0\nftdi_location $DEV_C0" > /openocdA.cfg
EOL

docker build -t openocd .

docker stop $(docker ps -a -q)
docker rm $(docker ps -a -q)
docker run -it --privileged -p127.0.0.1:3456:3456 -d openocd openocd -f openocdA.cfg

