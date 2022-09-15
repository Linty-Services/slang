//------------------------------------------------------------------------------
//! @file Definition.h
//! @brief Module / interface / program definitions
//
// SPDX-FileCopyrightText: Michael Popoloski
// SPDX-License-Identifier: MIT
//------------------------------------------------------------------------------
#pragma once

#include "slang/ast/Lookup.h"
#include "slang/ast/SemanticFacts.h"
#include "slang/syntax/SyntaxFwd.h"
#include "slang/util/Hash.h"
#include "slang/util/SmallVector.h"
#include "slang/util/Util.h"

namespace slang {

class SyntaxTree;

}

namespace slang::ast {

class AttributeSymbol;
class Compilation;
class Expression;
class NetType;
class Scope;
class Type;

class Definition {
public:
    struct ParameterDecl {
        union {
            const ParameterDeclarationSyntax* valueSyntax;
            const TypeParameterDeclarationSyntax* typeSyntax;
            const Type* givenType;
        };

        union {
            const DeclaratorSyntax* valueDecl;
            const TypeAssignmentSyntax* typeDecl;
            const Expression* givenInitializer;
        };

        string_view name;
        SourceLocation location;
        bool isTypeParam;
        bool isLocalParam;
        bool isPortParam;
        bool hasSyntax;

        ParameterDecl(const Scope& scope, const ParameterDeclarationSyntax& syntax,
                      const DeclaratorSyntax& decl, bool isLocal, bool isPort);
        ParameterDecl(const Scope& scope, const TypeParameterDeclarationSyntax& syntax,
                      const TypeAssignmentSyntax& decl, bool isLocal, bool isPort);
        ParameterDecl(string_view name, SourceLocation location, const Type& givenType,
                      bool isLocal, bool isPort, const Expression* givenInitializer);
        ParameterDecl(string_view name, SourceLocation location, bool isLocal, bool isPort,
                      const Type* defaultType);

        bool hasDefault() const;
    };

    string_view name;
    SourceLocation location;
    const ModuleDeclarationSyntax& syntax;
    const NetType& defaultNetType;
    const Scope& scope;
    SymbolIndex indexInScope;
    DefinitionKind definitionKind;
    VariableLifetime defaultLifetime;
    UnconnectedDrive unconnectedDrive;
    TimeScale timeScale;
    SmallVectorSized<ParameterDecl, 8> parameters;
    flat_hash_set<string_view> modports;
    span<const AttributeSymbol* const> attributes;
    const SyntaxTree* syntaxTree;
    bool hasNonAnsiPorts;

    Definition(const Scope& scope, LookupLocation lookupLocation,
               const ModuleDeclarationSyntax& syntax, const NetType& defaultNetType,
               UnconnectedDrive unconnectedDrive, optional<TimeScale> directiveTimeScale,
               const SyntaxTree* syntaxTree);

    /// Returns a string description of the definition kind, such as "module",
    /// "interface", or "program".
    string_view getKindString() const;

    /// Returns a string description of the definition kind, including an
    /// indefinite article. e.g. "a module", "an interface".
    string_view getArticleKindString() const;

    bool isInstantiated() const { return instantiated; }
    void noteInstantiated() const { instantiated = true; }

    void getHierarchicalPath(std::string& buffer) const;

private:
    mutable bool instantiated = false;
};

} // namespace slang::ast