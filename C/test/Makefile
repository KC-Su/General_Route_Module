CC        = gcc 
SUBDIRS  = 
CC_DEFINE  += -DGROUTE_UNIT_TEST -DUNIT_TEST_FAST 
TARGET    = test

CC_FLAGS   += -Wall -g3 -I../include 
CC_FLAGS   += -ftest-coverage -fprofile-arcs

LDFLAGS += -lcunit -lgcov

TEST_C_FILE = groute_test.c groute_stub.c
TEST_OBJ    = $(TEST_C_FILE:.c=.o)

FUT_PATH    = ../
FUT         = groute.c groute.h
SRC_PATH    = $(shell cd ../../;pwd)

CURR_DIR    = $(shell pwd)
LCOV_FILTER_PREFIX = "*
LCOV_FILTER_SUFFIX = "
FUT_LCOV_TEMP = $(addprefix $(LCOV_FILTER_PREFIX), $(FUT))
FUT_LCOV_FILTER = $(addsuffix $(LCOV_FILTER_SUFFIX), $(FUT_LCOV_TEMP))

FUT_C_FILE  = $(filter %.c,$(FUT))
FUT_OBJ     = $(FUT_C_FILE:.c=.o)


all: $(FUT) $(TARGET) 

$(FUT): 
	cp $(addprefix $(FUT_PATH), $(FUT)) ./
	-covselect --no-banner -a $(addprefix $(CURR_DIR)/, $(FUT))
	#unmark upper line when using Bullseye

$(TARGET): $(TEST_OBJ) $(FUT_OBJ)
	$(CC) -g -o $(TARGET) $(TEST_OBJ) $(FUT_OBJ) -L. $(LDFLAGS)

%.o : %.c *.h 
	$(CC) $(CC_FLAGS) $(CC_DEFINE) -c $<

unit-test : $(FUT) $(TARGET)
	-./$(TARGET)						#Execute Binary
	lcov -c -d . -o temp.info			#Generate temp info
	lcov -e temp.info $(FUT_LCOV_FILTER) -o groute_cov.info #Filter we want
	rm temp.info
	lcov_cober groute_cov.info -b $(SRC_PATH) -o groute_cov.xml #Gernerate cobertura XML
	
clean:
	-ipcrm -M 0x47525400 
	-ipcrm -M 0x47525401 
	rm -f $(TARGET)
	rm -f $(FUT)
	rm -f *.o
	rm -f *.gc*
	rm -f *.xml
	rm -f *.txt
	rm -f test.cov							#unmark this when using Bullseye
