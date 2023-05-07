BASE = test
BISON = bison
FLEX = flex

CXXFLAGS += -I/usr/lib/llvm-12/include

all: $(BASE)

%.cc %.hh: %.yy
	$(BISON) $(BISONFLAGS) -o $*.cc $<

%.cc: %.ll
	$(FLEX) $(FLEXFLAGS) -o $@ $<

%.o: %.cc
	$(CXX) $(CXXFLAGS) -c -o $@ $<

$(BASE): $(BASE).o driver.o parser.o scanner.o ast.o
	$(CXX) `llvm-config-12 --ldflags` -o $@ $^ `llvm-config-12 --libs`

$(BASE).o: parser.hh
parser.o: parser.hh
scanner.o: parser.hh

run: $(BASE)
	@echo "Quit with ctrl-d."
	./$< -

CLEANFILES = $(BASE) *.o parser.hh parser.cc location.hh scanner.cc
clean:
	$(RM) $(CLEANFILES)
