//------------------------------------------------------------------------------
// ScriptSession.cpp
// High-level interface to the compiler tools to evaluate snippets of code
//
// SPDX-FileCopyrightText: Michael Popoloski
// SPDX-License-Identifier: MIT
//------------------------------------------------------------------------------
#include "slang/ast/ScriptSession.h"

#include "slang/ast/Expression.h"
#include "slang/ast/Statements.h"
#include "slang/ast/symbols/BlockSymbols.h"
#include "slang/ast/symbols/CompilationUnitSymbols.h"
#include "slang/ast/symbols/VariableSymbols.h"
#include "slang/syntax/AllSyntax.h"

namespace slang::ast {

static CompilationOptions createOptions() {
    CompilationOptions options;
    options.allowHierarchicalConst = true;
    return options;
}

ScriptSession::ScriptSession() :
    compilation(createOptions()), scope(compilation.createScriptScope()),
    evalContext(compilation, EvalFlags::IsScript) {
    evalContext.pushEmptyFrame();
}

ConstantValue ScriptSession::eval(string_view text) {
    syntaxTrees.emplace_back(SyntaxTree::fromText(text));

    const auto& node = syntaxTrees.back()->root();
    switch (node.kind) {
        case SyntaxKind::ParameterDeclarationStatement:
        case SyntaxKind::FunctionDeclaration:
        case SyntaxKind::TaskDeclaration:
        case SyntaxKind::InterfaceDeclaration:
        case SyntaxKind::ModuleDeclaration:
        case SyntaxKind::HierarchyInstantiation:
        case SyntaxKind::TypedefDeclaration:
            scope.addMembers(node);
            return nullptr;
        case SyntaxKind::DataDeclaration: {
            SmallVectorSized<const ValueSymbol*, 2> symbols;
            VariableSymbol::fromSyntax(compilation, node.as<DataDeclarationSyntax>(), scope,
                                       symbols);

            for (auto symbol : symbols) {
                scope.addMember(*symbol);

                ConstantValue initial;
                if (auto initializer = symbol->getInitializer())
                    initial = initializer->eval(evalContext);

                evalContext.createLocal(symbol, initial);
            }
            return nullptr;
        }
        case SyntaxKind::CompilationUnit:
            for (auto member : node.as<CompilationUnitSyntax>().members)
                scope.addMembers(*member);
            return nullptr;
        default:
            if (ExpressionSyntax::isKind(node.kind)) {
                return evalExpression(node.as<ExpressionSyntax>());
            }
            else if (StatementSyntax::isKind(node.kind)) {
                evalStatement(node.as<StatementSyntax>());
                return nullptr;
            }
            else {
                // If this throws, ScriptSession doesn't currently support whatever construct
                // you were trying to evaluate. Add support to the case above.
                ASSUME_UNREACHABLE;
            }
    }
}

ConstantValue ScriptSession::evalExpression(const ExpressionSyntax& expr) {
    ASTContext context(scope, LookupLocation::max);
    auto& bound = Expression::bind(expr, context, ASTFlags::AssignmentAllowed);
    return bound.eval(evalContext);
}

void ScriptSession::evalStatement(const StatementSyntax& stmt) {
    auto& block = StatementBlockSymbol::fromLabeledStmt(scope, stmt);
    scope.addMember(block);

    ASTContext context(scope, LookupLocation::max);
    Statement::StatementContext stmtCtx(context);
    block.getStatement(context, stmtCtx).eval(evalContext);
}

Diagnostics ScriptSession::getDiagnostics() {
    Diagnostics result;
    for (auto& tree : syntaxTrees)
        result.appendRange(tree->diagnostics());

    result.appendRange(compilation.getAllDiagnostics());
    result.appendRange(evalContext.getDiagnostics());
    result.sort(SyntaxTree::getDefaultSourceManager());
    return result;
}

} // namespace slang::ast