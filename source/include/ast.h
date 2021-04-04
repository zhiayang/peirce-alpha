// ast.h
// Copyright (c) 2021, zhiayang
// Licensed under the Apache License Version 2.0.

#pragma once

#include <unordered_map>

#include "defs.h"
#include "result.h"

namespace alpha
{
	struct Item;
}

namespace ast
{
	constexpr int EXPR_VAR          = 1;
	constexpr int EXPR_LIT          = 2;
	constexpr int EXPR_AND          = 3;
	constexpr int EXPR_NOT          = 4;
	constexpr int EXPR_OR           = 5;
	constexpr int EXPR_IMPLIES      = 6;
	constexpr int EXPR_BIDIRIMPLIES = 7;

	struct Expr
	{
		Expr(int t) : type(t) { }
		virtual ~Expr();

		virtual Expr* evaluate(const std::unordered_map<std::string, bool>& syms) const = 0;

		const int type;
		const alpha::Item* original = nullptr;
	};

	struct Var : Expr
	{
		Var(std::string s) : Expr(TYPE), name(std::move(s)) { }
		virtual ~Var() override;
		virtual Expr* evaluate(const std::unordered_map<std::string, bool>& syms) const override;

		static constexpr int TYPE = EXPR_VAR;

		std::string name;
	};

	struct Lit : Expr
	{
		Lit(bool v) : Expr(TYPE), value(v) { }
		virtual ~Lit() override;
		virtual Expr* evaluate(const std::unordered_map<std::string, bool>& syms) const override;

		static constexpr int TYPE = EXPR_LIT;

		bool value;
	};

	struct And : Expr
	{
		And(Expr* l, Expr* r) : Expr(TYPE), left(l), right(r) { }
		virtual ~And() override;
		virtual Expr* evaluate(const std::unordered_map<std::string, bool>& syms) const override;

		static constexpr int TYPE = EXPR_AND;

		Expr* left = 0;
		Expr* right = 0;
	};

	struct Not : Expr
	{
		Not(Expr* e) : Expr(TYPE), e(e) { }
		virtual ~Not() override;
		virtual Expr* evaluate(const std::unordered_map<std::string, bool>& syms) const override;

		static constexpr int TYPE = EXPR_NOT;

		Expr* e = 0;
	};

	struct Or : Expr
	{
		Or(Expr* l, Expr* r) : Expr(TYPE), left(l), right(r) { }
		virtual ~Or() override;
		virtual Expr* evaluate(const std::unordered_map<std::string, bool>& syms) const override;

		static constexpr int TYPE = EXPR_OR;

		Expr* left = 0;
		Expr* right = 0;
	};

	struct Implies : Expr
	{
		Implies(Expr* l, Expr* r) : Expr(TYPE), left(l), right(r) { }

		virtual ~Implies() override;
		virtual Expr* evaluate(const std::unordered_map<std::string, bool>& syms) const override;

		static constexpr int TYPE = EXPR_IMPLIES;

		Expr* left = 0;
		Expr* right = 0;
	};

	struct BidirImplies : Expr
	{
		BidirImplies(Expr* l, Expr* r) : Expr(TYPE), left(l), right(r) { }

		virtual ~BidirImplies() override;
		virtual Expr* evaluate(const std::unordered_map<std::string, bool>& syms) const override;

		static constexpr int TYPE = EXPR_BIDIRIMPLIES;

		Expr* left = 0;
		Expr* right = 0;
	};
}

namespace parser
{
	struct Location
	{
		size_t begin;
		size_t length;
	};

	struct Error
	{
		std::string msg;
		Location loc;
	};

	enum class TokenType
	{
		Invalid,

		LParen,
		RParen,
		LBrace,
		RBrace,
		RightArrow,
		LeftArrow,
		DoubleArrow,
		Backslash,

		Not,
		And,
		Or,

		Top,
		Bottom,
		Identifier,

		EndOfFile,
	};

	struct Token
	{
		Token() { }
		Token(TokenType t, Location loc, zbuf::str_view s) : loc(loc), text(s), type(t) { }

		Location loc;
		zbuf::str_view text;
		TokenType type = TokenType::Invalid;

		operator TokenType() const { return this->type; }
	};

	zst::Result<ast::Expr*, Error> parse(zbuf::str_view input);
	zst::Result<std::vector<Token>, Error> lex(zbuf::str_view input);
}
