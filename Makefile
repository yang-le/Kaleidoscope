BASE = test
BISON = bison
FLEX = flex

CXXFLAGS = `llvm-config-12 --cxxflags` -std=c++17
CXXFLAGS += -g -fsanitize=leak -fsanitize=address

all: $(BASE)

%.cc %.hh: %.yy
	$(BISON) $(BISONFLAGS) -o $*.cc $<

%.cc: %.ll
	$(FLEX) $(FLEXFLAGS) -o $@ $<

%.o: %.cc
	$(CXX) $(CXXFLAGS) -c -o $@ $<

$(BASE): parser.o scanner.o ast.o CodeGenVisitor.o JITCodeGenVisitor.o NativeCodeGenVisitor.o driver.o $(BASE).o
	$(CXX) $(CXXFLAGS) `llvm-config-12 --ldflags` -o $@ $^ `llvm-config-12 --libs`

$(BASE).o: parser.hh
parser.o: parser.hh
scanner.o: parser.hh

run: $(BASE)
	@echo "Quit with ctrl-d."
	./$< -

CLEANFILES = $(BASE) *.o parser.hh parser.cc location.hh scanner.cc
clean:
	$(RM) $(CLEANFILES)
