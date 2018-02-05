#target and objects
NEED_ENV= GROUTE_LD_DIR GROUTE_INC_DIR JHASH_H_DIR GLIST_H_DIR
TARGET_SRV = libgroute.a
PUBLIC_HEADER = groute.h

TARGET_C_FILE = $(shell ls *.c)
TARGET_OBJ = $(TARGET_C_FILE:.c=.o)
TARGET_DEP = $(TARGET_OBJ:.o=.d)
#CFLAGS += -g -Wall -I./ -I../include/
CFLAGS += -fPIC -Wall $(DEBFLAGS)
CFLAGS += -I$(JHASH_H_DIR) -I$(GLIST_H_DIR)
TEST_FOLDER = test
# make rules

%/check:
	@if [ -z ${${@D}} ]; then\
		echo "Undefine $(@D)";\
		echo "Please source Build environment then make again";\
		echo "cd ../Script; source Bashrc;";\
		exit 1;\
	fi

all: $(addsuffix /check, $(NEED_ENV)) $(TARGET_SRV)
	cp $(TARGET_SRV) $(GROUTE_LD_DIR)
	cp $(PUBLIC_HEADER) $(GROUTE_INC_DIR)

$(TARGET_SRV): $(TARGET_OBJ)
	$(AR) -r $@ $^

%/ut:
	cd $(@D);make unit-test;
	
unit-test: $(addsuffix /ut, $(TEST_FOLDER))
	#gcovr -r ../ -g -x -o groute_cov.xml 

doc:
	cd doc_cfg; doxygen Doxygen.cfg 
	mv GROUTE $(GROUTE_DOC_DIR)

%.o: %.c
	$(CC) $(CFLAGS) -c -MD $< -o $@ 

%/clean:
	cd $(@D); make clean;
	
clean: $(addsuffix /check, $(NEED_ENV)) $(addsuffix /clean, $(TEST_FOLDER)) 
	rm -f $(GROUTE_LD_DIR)/$(TARGET_SRV)
	rm -f $(GROUTE_INC_DIR)/$(PUBLIC_HEADER)
	rm -f $(TARGET_SRV) *.d *.o

