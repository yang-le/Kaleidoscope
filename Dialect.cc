#include "Dialect.hh"

#include "mlir/IR/Builders.h"
#include "mlir/IR/BuiltinTypes.h"
#include "mlir/IR/FunctionImplementation.h"
#include "mlir/IR/OpImplementation.h"

using namespace mlir;
using namespace mlir::toy;

#include "Dialect.cc.inc"

void ToyDialect::initialize()
{
    addOperations<
#define GET_OP_LIST
#include "Ops.cc.inc"
        >();
}
