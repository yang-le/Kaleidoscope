#include "Toy.hh"

using namespace mlir;
using namespace mlir::toy;

#include "ToyDialect.cpp.inc"

void ToyDialect::initialize()
{
    addOperations<
#define GET_OP_LIST
#include "Toy.cpp.inc"
        >();
}
