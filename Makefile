KDIR := /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)
# this is for modprobe use
LDD := /lib/modules/$(shell uname -r)/kernel/drivers/ldd/

CDDparm := $(CDDparm)
CDDparm := 0

# Modules:
CH2_1 := ch21
CH2_2 := ch22main
CH2_2s := ch22sub
DRIVER := CDD2

# NOTE: The kernel module targets (Next 3 lines) MUST be separated by lblank lines.
obj-m := $(DRIVER).o $(CH2_1).o $(CH2_2).o $(CH2_2s).o

$(CH2_1)-objs := hello.o hello_sub.o

$(CH2_2)-objs := hello2.o

$(CH2_2s)-objs := hello2_mod.o hello_sub.o

$(DRIVER)-objs := main.o basic_ops.o proc_ops.o proc_seq_ops.o proc_log_marker.o

APPS := testApp_ch3 testApp_ch4 testApp_ch5 testApp_ch6

# NOTE: Ubuntu versions require /var/log/kern.log, others use /var/log/messages
KERN-LOG := /var/log/kern.log
KERN-MARKER := "UNIQUE KERNEL MARKER"

all: 	clean run
	@make -s clean

run: $(DRIVER) apps tests

apps:  $(APPS) testApp_ch1

compile: $(DRIVER).o $(CH2_1).o $(CH2_2).o $(CH2_2s).o apps

tests: test2 test3 test4 test5 test6

load: $(DRIVER).o
	su -c "{ insmod ./$(DRIVER).ko CDDparm=$(CDDparm);} || \
		{ echo CDDparm is not set;} ";

$(DRIVER): load
	-su -c "mknod -m 666 /dev/CDD2 c $(shell grep CDD2 /proc/devices | sed 's/CDD2//') 0;"
	-su -c "mknod -m 666 /dev/CDD16 c $(shell grep CDD2 /proc/devices | sed 's/CDD2//') 1;"
	-su -c "mknod -m 666 /dev/CDD64 c $(shell grep CDD2 /proc/devices | sed 's/CDD2//') 2;"
	-su -c "mknod -m 666 /dev/CDD128 c $(shell grep CDD2 /proc/devices | sed 's/CDD2//') 3;"
	-su -c "mknod -m 666 /dev/CDD256 c $(shell grep CDD2 /proc/devices | sed 's/CDD2//') 4;"
	# show devfs and procfs entries created
	ls -l /dev/CDD* /proc/CDD/* /proc/myps

# Debug version of the above, to make /dev/ nodes for any major dvc (hwk.6+)
altnodes:
	-su -c "mknod -m 666 /dev/CDD2 c $(MAJOR) 0;"
	-su -c "mknod -m 666 /dev/CDD16 c $(MAJOR) 1;"
	-su -c "mknod -m 666 /dev/CDD64 c $(MAJOR) 2;"
	-su -c "mknod -m 666 /dev/CDD128 c $(MAJOR) 3;"
	-su -c "mknod -m 666 /dev/CDD256 c $(MAJOR) 4;"
	# show devfs and procfs entries created
	ls -l /dev/CDD* /proc/CDD/* /proc/myps

$(CH2_1).o $(CH2_2).o $(CH2_2s).o $(DRIVER).o:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

unload:
	-su -c "rm -fr /dev/CDD*; rmmod $(DRIVER);"
#	-su -c "rmmod $(CH2_1);"

clean: unload
	-@rm -fr *.o $(APPS) .tmp_versions .[mM]odule* [mM]o* .*.o.d .*.cmd
	-@rm -fr $(DRIVER)*.o $(DRIVER)*.ko .$(DRIVER)*.* $(DRIVER)*.*.*
	-@rm -fr $(CH2_1)*.o $(CH2_1)*.ko .$(CH2_1)*.* $(CH2_1)*.*.*
	-@rm -fr $(CH2_2)*.o $(CH2_2)*.ko .$(CH2_2)*.* $(CH2_2)*.*.*
	-@rm -fr $(CH2_2s)*.o $(CH2_2s)*.ko .$(CH2_2s)*.* $(CH2_2s)*.*.*
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
	@echo ".. load (sub+main):"  >> $(CH02_OUTFILE)
	su -c "insmod ./$(CH2_2s).ko" >> $(CH02_OUTFILE)
	su -c "insmod ./$(CH2_2).ko howmany=3 whom=Michael" >> $(CH02_OUTFILE)
	sleep 1
	tac $(KERN-LOG) | grep "hello2" -B5000 -m1 | tac  >> $(CH02_OUTFILE)
	@echo ".. lsmod"  >> $(CH02_OUTFILE)
	lsmod | egrep "$(CH2_2)" >> $(CH02_OUTFILE)
	@echo ".. symbols:"  >> $(CH02_OUTFILE)
	-cat /proc/kallsyms | egrep "$(CH2_2)"  >> $(CH02_OUTFILE)
	@echo ".. symbols deps:"  >> $(CH02_OUTFILE)
	-tail /lib/modules/`uname -r`/modules.dep | egrep "$(CH2_2)" >> $(CH02_OUTFILE)
	@echo ".. unload(main+sub):"  >> $(CH02_OUTFILE)
	su -c "rmmod $(CH2_2)" >> $(CH02_OUTFILE)
	su -c "rmmod $(CH2_2s)" >> $(CH02_OUTFILE)
	tac $(KERN-LOG) | grep "Goodbye," -B5000 -m1 | tac  >> $(CH02_OUTFILE)
	@echo ""  >> $(CH02_OUTFILE)
	@echo "2.2+3: Output version info (2 stacked modules via modprobe):" >> $(CH02_OUTFILE)
	@echo ".. load (sub+main):"  >> $(CH02_OUTFILE)
	-su -c "mkdir -p $(LDD); cp $(CH2_2).ko  $(CH2_2s).ko $(LDD); depmod -A;"; >> $(CH02_OUTFILE)
	su -c "modprobe $(CH2_2) howmany=3 whom=Michael"  >> $(CH02_OUTFILE)
	sleep 1
	tac $(KERN-LOG) | grep "hello2" -B5000 -m1 | tac  >> $(CH02_OUTFILE)
	@echo ".. lsmod"  >> $(CH02_OUTFILE)
	lsmod | egrep "$(CH2_2)" >> $(CH02_OUTFILE)
	@echo ".. symbols:"  >> $(CH02_OUTFILE)
	-cat /proc/kallsyms | egrep "$(CH2_2)"  >> $(CH02_OUTFILE)
	@echo ".. symbols deps:"  >> $(CH02_OUTFILE)
	-tail /lib/modules/`uname -r`/modules.dep | egrep "$(CH2_2)" >> $(CH02_OUTFILE)
	@echo ".. unload(main+sub):"  >> $(CH02_OUTFILE)
	su -c "modprobe -r $(CH2_2)" >> $(CH02_OUTFILE)
	tac $(KERN-LOG) | grep "Goodbye," -B5000 -m1 | tac  >> $(CH02_OUTFILE)

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
	cat /proc/CDD/CDD2  >> $(CH04_OUTFILE)
	cat /dev/CDD2  >> $(CH04_OUTFILE)
	@echo ""  >> $(CH04_OUTFILE)
	@echo "# Ch.4.1: Test file O_APPEND mode report"  >> $(CH04_OUTFILE)
	echo -n "Testing" > /dev/CDD2
	echo ", 1, 2, 3 ..." >> /dev/CDD2
	cat /proc/CDD/CDD2  >> $(CH04_OUTFILE)
	cat /dev/CDD2  >> $(CH04_OUTFILE)
	@echo ""  >> $(CH04_OUTFILE)
	@echo "# Ch.4.1: Test buffer overrun report"  >> $(CH04_OUTFILE)
	-yes | dd bs=1 count=5000 of=/dev/CDD2
	cat /proc/CDD/CDD2  >> $(CH04_OUTFILE)
	cat /dev/CDD2 > /dev/null
	@echo ""  >> $(CH04_OUTFILE)
	@echo "# Ch.4.1: test empty buffer report"  >> $(CH04_OUTFILE)
	echo -n "" > /dev/CDD2
	cat /proc/CDD/CDD2  >> $(CH04_OUTFILE)
	cat /dev/CDD2  >> $(CH04_OUTFILE)
	@echo ""  >> $(CH04_OUTFILE)
	@echo "# Ch.4.1: Test writable proc entry (changes name of 2nd team member)"  >> $(CH04_OUTFILE)
	cat /proc/CDD/CDD2  >> $(CH04_OUTFILE)
	echo "My Word!" > /proc/CDD/CDD2
	cat /proc/CDD/CDD2  >> $(CH04_OUTFILE)
	cat /proc/CDD/CDD2  >> $(CH04_OUTFILE)
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

CH06_OUTFILE := ./Outputs/Chapter06.txt

testApp_ch6: testApp_ch6.c
	-gcc -o testApp_ch6 testApp_ch6.c;

test6: CDD2 testApp_ch6
	@echo "HOMEWORK TEST OUTPUT FOR CHAPTER 06"   > $(CH06_OUTFILE)
	@echo ""  >> $(CH06_OUTFILE)
	@echo "# UTIL - mark kernel log for later retrieval..."  >> $(CH06_OUTFILE)
	echo $(KERN-MARKER) > /proc/CDD/marker
	#echo "*** KERNEL MARKER ***" > /proc/CDD/marker
	@echo ""  >> $(CH06_OUTFILE)
	@echo "# Ch.6.1 Test 1 - multi-minor read-write"  >> $(CH06_OUTFILE)
	./testApp_ch6 0  >> $(CH06_OUTFILE)
	@echo "Read /dev and /proc/CDD entries"  >> $(CH06_OUTFILE)
	@echo "cat /dev/CDD2:"  >> $(CH06_OUTFILE)
	cat /dev/CDD2  >> $(CH06_OUTFILE)
	@echo ""  >> $(CH06_OUTFILE)
	cat /proc/CDD/CDD2  >> $(CH06_OUTFILE)
	@echo "cat /dev/CDD16:"  >> $(CH06_OUTFILE)
	cat /dev/CDD16  >> $(CH06_OUTFILE)
	@echo ""  >> $(CH06_OUTFILE)
	cat /proc/CDD/CDD16  >> $(CH06_OUTFILE)
	@echo "cat /dev/CDD64:"  >> $(CH06_OUTFILE)
	cat /dev/CDD64  >> $(CH06_OUTFILE)
	@echo ""  >> $(CH06_OUTFILE)
	cat /proc/CDD/CDD64  >> $(CH06_OUTFILE)
	@echo "cat /dev/CDD128:"  >> $(CH06_OUTFILE)
	cat /dev/CDD128  >> $(CH06_OUTFILE)
	@echo ""  >> $(CH06_OUTFILE)
	cat /proc/CDD/CDD128  >> $(CH06_OUTFILE)
	@echo "cat /dev/CDD256:"  >> $(CH06_OUTFILE)
	cat /dev/CDD256  >> $(CH06_OUTFILE)
	@echo ""  >> $(CH06_OUTFILE)
	cat /proc/CDD/CDD256  >> $(CH06_OUTFILE)
	@echo ""  >> $(CH06_OUTFILE)
	@echo "# Ch.6.1 Test 1 - output of kernel message log"  >> $(CH06_OUTFILE)
	tac $(KERN-LOG) | grep "$(shell /bin/cat /proc/CDD/marker)" -B6000 -m1 | tac  >> $(CH06_OUTFILE)
	@echo ""  >> $(CH06_OUTFILE)
	@echo "# Ch.6.1 Test 2 - multi-minor seek"  >> $(CH06_OUTFILE)
	echo $(KERN-MARKER) > /proc/CDD/marker
	./testApp_ch6 1  >> $(CH06_OUTFILE)
	@echo "Read /dev and /proc/CDD entries"  >> $(CH06_OUTFILE)
	@echo "cat /dev/CDD2:"  >> $(CH06_OUTFILE)
	cat /dev/CDD2  >> $(CH06_OUTFILE)
	@echo ""  >> $(CH06_OUTFILE)
	cat /proc/CDD/CDD2  >> $(CH06_OUTFILE)
	@echo "cat /dev/CDD16:"  >> $(CH06_OUTFILE)
	cat /dev/CDD16  >> $(CH06_OUTFILE)
	@echo ""  >> $(CH06_OUTFILE)
	cat /proc/CDD/CDD16  >> $(CH06_OUTFILE)
	@echo "cat /dev/CDD64:"  >> $(CH06_OUTFILE)
	cat /dev/CDD64  >> $(CH06_OUTFILE)
	@echo ""  >> $(CH06_OUTFILE)
	cat /proc/CDD/CDD64  >> $(CH06_OUTFILE)
	@echo "cat /dev/CDD128:"  >> $(CH06_OUTFILE)
	cat /dev/CDD128  >> $(CH06_OUTFILE)
	@echo ""  >> $(CH06_OUTFILE)
	cat /proc/CDD/CDD128  >> $(CH06_OUTFILE)
	@echo "cat /dev/CDD256:"  >> $(CH06_OUTFILE)
	cat /dev/CDD256  >> $(CH06_OUTFILE)
	@echo ""  >> $(CH06_OUTFILE)
	cat /proc/CDD/CDD256  >> $(CH06_OUTFILE)
	@echo ""  >> $(CH06_OUTFILE)
	@echo "# Ch.6.1 Test 2 - output of kernel message log"  >> $(CH06_OUTFILE)
	tac $(KERN-LOG) | grep "$(shell /bin/cat /proc/CDD/marker)" -B6000 -m1 | tac  >> $(CH06_OUTFILE)
	@echo ""  >> $(CH06_OUTFILE)
	@echo "# Ch.6.2 Test - ioctl"  >> $(CH06_OUTFILE)
	echo $(KERN-MARKER) > /proc/CDD/marker
	./testApp_ch6 2  >> $(CH06_OUTFILE)
	@echo "# Ch.6.2 Test - output of kernel message log"  >> $(CH06_OUTFILE)
	tac $(KERN-LOG) | grep "$(shell /bin/cat /proc/CDD/marker)" -B6000 -m1 | tac  >> $(CH06_OUTFILE)
