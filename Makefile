KDIR := /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)

CDDparm := $(CDDparm)
CDDparm := 35

obj-m := CDD2.o
CDD2-objs := main.o basic_ops.o proc_ops.o proc_seq_ops.o proc_log_marker.o

APPS := testApp_ch3 testApp_ch4

# NOTE: Ubuntu versions of /var/log/messages
KERN-LOG := /var/log/kern.log
KERN-MARKER := "UNIQUE KERNEL MARKER"

all: 	clean run
	@make -s clean

run: CDD2 apps tests

apps:  $(APPS) testApp_ch1

compile: CDD2.o apps

tests: test3 test4 test5
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
## See here for how to do clever line grabs from files like the kernel log:
# http://unix.stackexchange.com/questions/56429/how-to-print-all-lines-after-a-match-up-to-the-end-of-the-file
## NOTE: The Outputs directory will contain test output products for project checkin.
CH01_OUTFILE := ./Outputs/Chapter01.txt

testApp_ch1:
	@echo "HOMEWORK TEST OUTPUT FOR CHAPTER 01"   > $(CH01_OUTFILE)
	@echo ""  >> $(CH01_OUTFILE)
	@echo "1.a: Output of 'uname -a':" >> $(CH01_OUTFILE)
	-su -c "uname -a" >> $(CH01_OUTFILE)
	@echo ""  >> $(CH01_OUTFILE)
	@echo "1.b: Output of 'cat /etc/*-release':" >> $(CH01_OUTFILE)
	-su -c "cat /etc/*-release" >> $(CH01_OUTFILE)

CH03_OUTFILE := ./Outputs/Chapter03.txt

testApp_ch3: testApp_ch3.c
	-gcc -o testApp_ch3 testApp_ch3.c;

test3: CDD2 testApp_ch3
	@echo "HOMEWORK TEST OUTPUT FOR CHAPTER 03"   > $(CH03_OUTFILE)
	@echo ""  >> $(CH03_OUTFILE)
	@echo "# Ch.3: Test file O_TRUNC mode " >> $(CH03_OUTFILE)
	echo "Hello World" > /dev/CDD2
	cat /dev/CDD2 >> $(CH03_OUTFILE)
	@echo ""  >> $(CH03_OUTFILE)
	@echo "# Ch.3: Test file O_APPEND mode"  >> $(CH03_OUTFILE)
	echo -n "Testing" > /dev/CDD2
	echo ", 1, 2, 3 ..." >> /dev/CDD2
	cat /dev/CDD2 >> $(CH03_OUTFILE)
	@echo ""  >> $(CH03_OUTFILE)
	@echo "# Ch.3: Test buffer overrun"  >> $(CH03_OUTFILE)
	-yes | dd bs=1 count=5000 of=/dev/CDD2
	cat /dev/CDD2 > /dev/null
	@echo ""  >> $(CH03_OUTFILE)
	@echo "# Ch.3: test empty buffer"  >> $(CH03_OUTFILE)
	echo -n "" > /dev/CDD2
	cat /dev/CDD2 >> $(CH03_OUTFILE)
	@echo ""  >> $(CH03_OUTFILE)
	@echo "# Ch.3: run test app"  >> $(CH03_OUTFILE)
	./testApp_ch3  >> $(CH03_OUTFILE)

CH04_OUTFILE := ./Outputs/Chapter04.txt

testApp_ch4: testApp_ch4.c
	-gcc -pthread -o testApp_ch4 testApp_ch4.c;

test4: CDD2 testApp_ch4
	@echo "HOMEWORK TEST OUTPUT FOR CHAPTER 04"   > $(CH04_OUTFILE)
	@echo ""  >> $(CH04_OUTFILE)
	@echo "# UTIL - mark kernel log for later retrieval..."  >> $(CH04_OUTFILE)
	echo $(KERN-MARKER) > /proc/CDD/marker
	@echo ""  >> $(CH04_OUTFILE)
	@echo "# Ch.4.1: Test file O_TRUNC mode report"  >> $(CH04_OUTFILE)
	echo "Hello World" > /dev/CDD2
	cat /proc/CDD/myCDD2  >> $(CH04_OUTFILE)
	cat /dev/CDD2  >> $(CH04_OUTFILE)
	@echo ""  >> $(CH04_OUTFILE)
	@echo "# Ch.4.1: Test file O_APPEND mode report"  >> $(CH04_OUTFILE)
	echo -n "Testing" > /dev/CDD2
	echo ", 1, 2, 3 ..." >> /dev/CDD2
	cat /proc/CDD/myCDD2  >> $(CH04_OUTFILE)
	cat /dev/CDD2  >> $(CH04_OUTFILE)
	@echo ""  >> $(CH04_OUTFILE)
	@echo "# Ch.4.1: Test buffer overrun report"  >> $(CH04_OUTFILE)
	-yes | dd bs=1 count=5000 of=/dev/CDD2
	cat /proc/CDD/myCDD2  >> $(CH04_OUTFILE)
	cat /dev/CDD2 > /dev/null
	@echo ""  >> $(CH04_OUTFILE)
	@echo "# Ch.4.1: test empty buffer report"  >> $(CH04_OUTFILE)
	echo -n "" > /dev/CDD2
	cat /proc/CDD/myCDD2  >> $(CH04_OUTFILE)
	cat /dev/CDD2  >> $(CH04_OUTFILE)
	@echo ""  >> $(CH04_OUTFILE)
	@echo "# Ch.4.1: Test writable proc entry (changes name of 2nd team member)"  >> $(CH04_OUTFILE)
	cat /proc/CDD/myCDD2  >> $(CH04_OUTFILE)
	echo "My Word!" > /proc/CDD/myCDD2
	cat /proc/CDD/myCDD2  >> $(CH04_OUTFILE)
	cat /proc/CDD/myCDD2  >> $(CH04_OUTFILE)
	@echo ""  >> $(CH04_OUTFILE)
	@echo "# Ch.4.2 Test /proc/myps on current task"  >> $(CH04_OUTFILE)
	cat /proc/myps  >> $(CH04_OUTFILE)
	@echo ""  >> $(CH04_OUTFILE)
	@echo "# Ch.4: run test app with a few threads"  >> $(CH04_OUTFILE)
	./testApp_ch4 5  >> $(CH04_OUTFILE)
	@echo ""  >> $(CH04_OUTFILE)
	@echo "# Grab the recent output of kernel message log too"  >> $(CH04_OUTFILE)
	tac $(KERN-LOG) | grep "$(shell /bin/cat /proc/CDD/marker)" -B5000 -m1 | tac  >> $(CH04_OUTFILE)

CH05_OUTFILE := ./Outputs/Chapter05.txt

test5: CDD2
	@echo "HOMEWORK TEST OUTPUT FOR CHAPTER 05"   > $(CH05_OUTFILE)
	@echo ""  >> $(CH05_OUTFILE)
	@echo "# UTIL - mark kernel log for later retrieval..."  >> $(CH05_OUTFILE)
	echo $(KERN-MARKER) > /proc/CDD/marker
	#echo "*** KERNEL MARKER ***" > /proc/CDD/marker
	@echo ""  >> $(CH05_OUTFILE)
	@echo "# Ch.5.2 Test /proc/myps on pid provided by writing to /proc/myps"  >> $(CH05_OUTFILE)
	echo 1 > /proc/myps
	cat /proc/myps  >> $(CH05_OUTFILE)
	@echo ""  >> $(CH05_OUTFILE)
	@echo "# Grab the recent output of kernel message log too"  >> $(CH05_OUTFILE)
	tac $(KERN-LOG) | grep "$(shell /bin/cat /proc/CDD/marker)" -B5000 -m1 | tac  >> $(CH05_OUTFILE)

unload:
	-su -c "rmmod CDD2; rm -fr /dev/CDD2;"

clean: unload
	-@rm -fr *.o CDD2*.o CDD2*.ko .CDD2*.* CDD2*.*.* $(APPS) .tmp_versions .[mM]odule* [mM]o*
