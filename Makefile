BASE = test
BISON = bison
FLEX = flex
CXX = clang++-12

CXXFLAGS = `llvm-config --cxxflags`
CXXFLAGS += -g -fsanitize=leak -fsanitize=address

all: $(BASE)

%.cc %.hh: %.yy
	$(BISON) $(BISONFLAGS) -o $*.cc $<

%.cc: %.ll
	$(FLEX) $(FLEXFLAGS) -o $@ $<

%.o: %.cc
	$(CXX) $(CXXFLAGS) -c -o $@ $<

$(BASE): parser.o scanner.o ast.o CodeGenVisitor.o JITCodeGenVisitor.o NativeCodeGenVisitor.o driver.o $(BASE).o
	$(CXX) -fuse-ld=lld $(CXXFLAGS) `llvm-config --ldflags` -o $@ $^ `llvm-config --libs` -pthread -lz -ldl -lncurses

$(BASE).o: parser.hh
parser.o: parser.hh
scanner.o: parser.hh

run: $(BASE)
	@echo "Quit with ctrl-d."
	./$< -

CLEANFILES = $(BASE) *.o parser.hh parser.cc location.hh scanner.cc
clean:
	$(RM) $(CLEANFILES)
