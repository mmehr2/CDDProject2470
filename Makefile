KDIR := /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)

CDDparm := $(CDDparm)
CDDparm := 35

obj-m := CDD2.o
CDD2-objs := main.o basic_ops.o proc_ops.o proc_seq_ops.o

APPS := testApp_ch3 testApp_ch4

all: 	clean run
	@make -s clean

run: CDD2 apps tests

apps:  $(APPS)

compile: CDD2.o apps

tests: CDD2 apps
	# @ [ -c /dev/CDD2 ] && { echo "Hello World" > /dev/CDD2;};
	# @ [ -c /dev/CDD2 ] && { cat < /dev/CDD2; };
	# @ [ -c /dev/CDD2 ] && { echo "Hello World" > /dev/CDD2;};
	# @ [ -c /dev/CDD2 ] && { echo "Hello World" > /dev/CDD2;};
	# @ [ -c /dev/CDD2 ] && { cat < /dev/CDD2; };
	# Test file O_TRUNC mode
	echo "Hello World" > /dev/CDD2;
	cat < /proc/CDD/myCDD2;
	cat < /dev/CDD2;
	# Test file O_APPEND mode
	echo -n "Testing" > /dev/CDD2;
	echo ", 1, 2, 3 ..." >> /dev/CDD2;
	cat < /proc/CDD/myCDD2;
	cat < /dev/CDD2;
	# Test buffer overrun
	-yes | dd bs=1 count=5000 of=/dev/CDD2
	cat < /proc/CDD/myCDD2;
	cat < /dev/CDD2 > /dev/null;
	# test empty buffer
	echo -n "" > /dev/CDD2
	cat < /proc/CDD/myCDD2;
	cat < /dev/CDD2;
	# run test app
	./testApp_ch3;
	# Test writable proc entry
	cat < /proc/CDD/myCDD2;
	echo "My Word!" > /proc/CDD/myCDD2;
	cat < /proc/CDD/myCDD2;
	cat < /proc/CDD/myCDD2;
	# Test myps (sequence file ops entry)
	# cat /proc/myps
	# show devfs and procfs entries created
	ls -l /proc/CDD/myCDD2 /dev/CDD2 /proc/myps

load: CDD2.o
	-su -c "{ insmod ./CDD2.ko CDDparm=$(CDDparm);} || \
		{ echo CDDparm is not set;} ";

CDD2: load
	-su -c "mknod -m 666 /dev/CDD2 c $(shell grep CDD2 /proc/devices | sed 's/CDD2//') 0;"


CDD2.o:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

###
###  Alternatively, you may want to use the early 2.6 syntax of
###  $(MAKE) -C $(KDIR) SUBDIRS=$(PWD) modules
###
# Targets for various test apps go here
testApp_ch3: testApp_ch3.c
	-gcc -o testApp_ch3 testApp_ch3.c;

testApp_ch4: testApp_ch4.c
	-gcc -pthread -o testApp_ch4 testApp_ch4.c;

unload:
	-su -c "rmmod CDD2; rm -fr /dev/CDD2;"

clean: unload
	-@rm -fr *.o CDD2*.o CDD2*.ko .CDD2*.* CDD2*.*.* $(APPS) .tmp_versions .[mM]odule* [mM]o*
