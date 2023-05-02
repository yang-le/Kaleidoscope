D=0
V=0

all: test

test: lex.yy.c y.tab.c ast.cc
	$(CXX) -O3 -DYYDEBUG=$D -DYYERROR_VERBOSE=$V $^ `llvm-config --cxxflags --ldflags --libs` -o $@

lex.yy.c: lex.l
	$(LEX) $<

y.tab.c y.tab.h: parser.y
	$(YACC) -d $<

clean:
	$(RM) test lex.yy.c y.tab.c y.tab.h
