KDIR := /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)
# this is for modprobe use
LDD := /lib/modules/$(shell uname -r)/kernel/drivers/ldd/

CDDparm := $(CDDparm)
CDDparm := 35

# Modules:
CH2_1 := ch21
CH2_2 := ch22
DRIVER := CDD2

# NOTE: The kernel module targets (Next 3 lines) MUST be separated by lblank lines.
obj-m := $(DRIVER).o $(CH2_1).o

$(CH2_1)-objs := hello.o

$(DRIVER)-objs := main.o basic_ops.o proc_ops.o proc_seq_ops.o proc_log_marker.o

APPS := testApp_ch3 testApp_ch4 testApp_ch5

# NOTE: Ubuntu versions require /var/log/kern.log, others use /var/log/messages
KERN-LOG := /var/log/kern.log
KERN-MARKER := "UNIQUE KERNEL MARKER"

all: 	clean run
	@make -s clean

run: $(DRIVER) apps tests

apps:  $(APPS) testApp_ch1

compile: $(DRIVER).o $(CH2_1).o apps

tests: test2 test3 test4 test5

load: $(DRIVER).o
	-su -c "{ insmod ./$(DRIVER).ko CDDparm=$(CDDparm);} || \
		{ echo CDDparm is not set;} ";

$(DRIVER): load
	-su -c "mknod -m 666 /dev/CDD2 c $(shell grep CDD2 /proc/devices | sed 's/CDD2//') 0;"
	# show devfs and procfs entries created
	ls -l /proc/CDD/* /dev/CDD* /proc/myps


$(CH2_1).o $(DRIVER).o:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

unload:
	-su -c "rmmod $(DRIVER); rm -fr /dev/$(DRIVER);"
#	-su -c "rmmod $(CH2_1);"

clean: unload
	-@rm -fr *.o $(APPS) .tmp_versions .[mM]odule* [mM]o*
	-@rm -fr $(DRIVER)*.o $(DRIVER)*.ko .$(DRIVER)*.* $(DRIVER)*.*.*
	-@rm -fr $(CH2_1)*.o $(CH2_1)*.ko .$(CH2_1)*.* $(CH2_1)*.*.*
	-@su -c "rm -f $(LDD)/*; [ -d $(LDD) ] && { rmdir $(LDD); };"

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
	-cat /etc/*-release >> $(CH01_OUTFILE)

CH02_OUTFILE := ./Outputs/Chapter02.txt

test2: compile
	@echo "HOMEWORK TEST OUTPUT FOR CHAPTER 02"   > $(CH02_OUTFILE)
	@echo ""  >> $(CH02_OUTFILE)
	@echo "2.1: Output version info (single module via insmod):" >> $(CH02_OUTFILE)
	@echo ".. load:"  >> $(CH02_OUTFILE)
	su -c "insmod ./$(CH2_1).ko howmany=3 whom=Michael" >> $(CH02_OUTFILE)
	tac $(KERN-LOG) | grep "(0) Hello," -B5000 -m1 | tac  >> $(CH02_OUTFILE)
	@echo ".. lsmod"  >> $(CH02_OUTFILE)
	lsmod | egrep "$(CH2_1)" >> $(CH02_OUTFILE)
	@echo ".. symbols:"  >> $(CH02_OUTFILE)
	-cat /proc/kallsyms | egrep "$(CH2_1)"  >> $(CH02_OUTFILE)
	@echo ".. symbols deps:"  >> $(CH02_OUTFILE)
	-tail /lib/modules/`uname -r`/modules.dep | egrep "$(CH2_1)" >> $(CH02_OUTFILE)
	@echo ".. unload:"  >> $(CH02_OUTFILE)
	su -c "rmmod $(CH2_1)" >> $(CH02_OUTFILE)
	tac $(KERN-LOG) | grep "Goodbye," -B5000 -m1 | tac  >> $(CH02_OUTFILE)
	@echo ""  >> $(CH02_OUTFILE)
	@echo "2.1: Output version info (single module via modprobe):" >> $(CH02_OUTFILE)
	@echo ".. load:"  >> $(CH02_OUTFILE)
	-su -c "mkdir -p $(LDD); cp $(CH2_1).ko $(LDD); depmod -A;"; >> $(CH02_OUTFILE)
	su -c "modprobe $(CH2_1) howmany=3 whom=Michael"  >> $(CH02_OUTFILE)
	tac $(KERN-LOG) | grep "(0) Hello," -B5000 -m1 | tac  >> $(CH02_OUTFILE)
	@echo ".. lsmod"  >> $(CH02_OUTFILE)
	lsmod | egrep "$(CH2_1)" >> $(CH02_OUTFILE)
	@echo ".. symbols:"  >> $(CH02_OUTFILE)
	-cat /proc/kallsyms | egrep "$(CH2_1)"  >> $(CH02_OUTFILE)
	@echo ".. symbols deps:"  >> $(CH02_OUTFILE)
	-tail /lib/modules/`uname -r`/modules.dep | egrep "$(CH2_1)" >> $(CH02_OUTFILE)
	@echo ".. unload:"  >> $(CH02_OUTFILE)
	su -c "modprobe -r $(CH2_1)" >> $(CH02_OUTFILE)
	tac $(KERN-LOG) | grep "Goodbye," -B5000 -m1 | tac  >> $(CH02_OUTFILE)
	@echo ""  >> $(CH02_OUTFILE)
	@echo "2.2+3: Output version info (2 stacked modules via insmod):" >> $(CH02_OUTFILE)
	#	-su -c "insmod ./$(CH2_1).ko"  >> $(CH02_OUTFILE)
	@echo ""  >> $(CH02_OUTFILE)
	@echo "2.2+3: Output version info (2 stacked modules via modprobe):" >> $(CH02_OUTFILE)
	#	-su -c "insmod ./$(CH2_1).ko"  >> $(CH02_OUTFILE)

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

testApp_ch5: testApp_ch5.c
	-gcc -o testApp_ch5 testApp_ch5.c;

test5: CDD2 testApp_ch5
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
	@echo "# Ch.5.2b,c: run test app with process prio and open file changes"  >> $(CH05_OUTFILE)
	@echo "NOTE: Since fork() copies open files, the redirected output is duplicated each time."  >> $(CH05_OUTFILE)
	./testApp_ch5 30  >> $(CH05_OUTFILE)
	@echo ""  >> $(CH05_OUTFILE)
	@echo "# Grab the recent output of kernel message log too"  >> $(CH05_OUTFILE)
	tac $(KERN-LOG) | grep "$(shell /bin/cat /proc/CDD/marker)" -B5000 -m1 | tac  >> $(CH05_OUTFILE)
