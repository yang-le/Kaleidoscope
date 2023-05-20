#ifndef TOY
#define TOY

include "mlir/IR/OpBase.td"
include "mlir/IR/FunctionInterfaces.td"
include "mlir/IR/SymbolInterfaces.td"
include "mlir/Interfaces/SideEffectInterfaces.td"

def Toy_Dialect : Dialect {
    let name = "toy";
    let cppNamespace = "::mlir::toy";
}

class Toy_Op<string mnemonic, list<Trait> traits = []> :
    Op<Toy_Dialect, mnemonic, traits>;

def ConstantOp : Toy_Op<"constant", [Pure]> {
    let arguments = (ins F64ElementsAttr:$value);
    let results = (outs F64Tensor);
    let hasCustomAssemblyFormat = 1;
    let builders = [
        OpBuilder<(ins "DenseElementsAttr":$value), [{
            build($_builder, $_state, value.getType(), value);
        }]>,
        OpBuilder<(ins "double":$value)>
    ];
    let hasVerifier = 1;
}

def AddOp : Toy_Op<"add"> {
    let arguments = (ins F64Tensor:$lhs, F64Tensor:$rhs);
    let results = (outs F64Tensor);
    let hasCustomAssemblyFormat = 1;
    let builders = [
        OpBuilder<(ins "Value":$lhs, "Value":$rhs)>
    ];
}

def FuncOp : Toy_Op<"func", [FunctionOpInterface, IsolatedFromAbove]> {
    let arguments = (ins
        SymbolNameAttr:$sym_name,
        TypeAttrOf<FunctionType>:$function_type,
        OptionalAttr<DictArrayAttr>:$arg_attrs,
        OptionalAttr<DictArrayAttr>:$res_attrs
    );
    let regions = (region AnyRegion:$body);
    let builders = [OpBuilder<(ins
        "StringRef":$name,
        "FunctionType":$type,
        CArg<"ArrayRef<NamedAttribute>", "{}">:$attrs
    )>];
    let extraClassDeclaration = [{
        ArrayRef<Type> getArgumentTypes() { return getFunctionType().getInputs(); }
        ArrayRef<Type> getResultTypes() { return getFunctionType().getResults(); }
    }];
    let hasCustomAssemblyFormat = 1;
    let skipDefaultBuilders = 1;
}

def GenericCallOp : Toy_Op<"generic_call"> {
    let arguments = (ins FlatSymbolRefAttr:$callee, Variadic<F64Tensor>:$inputs);
    let results = (outs F64Tensor);
    let assemblyFormat = [{
        $callee `(` $inputs `)` attr-dict `:` functional-type($inputs, results)
    }];
    let builders = [
        OpBuilder<(ins "StringRef":$callee, "ArrayRef<Value>":$arguments)>
    ];
}

def MulOp : Toy_Op<"mul"> {
    let arguments = (ins F64Tensor:$lhs, F64Tensor:$rhs);
    let results = (outs F64Tensor);
    let hasCustomAssemblyFormat = 1;
    let builders = [
        OpBuilder<(ins "Value":$lhs, "Value":$rhs)>
    ];
}

#endif // TOY
