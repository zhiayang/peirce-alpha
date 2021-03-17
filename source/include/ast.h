// ast.h
// Copyright (c) 2021, zhiayang
// Licensed under the Apache License Version 2.0.

#pragma once

#include <unordered_map>

#include "defs.h"
#include "result.h"

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
		virtual ~Expr() { }
		virtual Expr* evaluate(const std::unordered_map<std::string, bool>& syms) const = 0;

		int type;
	};

	struct Var : Expr
	{
		static constexpr int TYPE = EXPR_VAR;
		virtual Expr* evaluate(const std::unordered_map<std::string, bool>& syms) const override;

		std::string name;
	};

	struct Lit : Expr
	{
		static constexpr int TYPE = EXPR_LIT;
		virtual Expr* evaluate(const std::unordered_map<std::string, bool>& syms) const override;

		bool value;
	};

	struct And : Expr
	{
		static constexpr int TYPE = EXPR_AND;
		virtual Expr* evaluate(const std::unordered_map<std::string, bool>& syms) const override;

		Expr* left = 0;
		Expr* right = 0;
	};

	struct Not : Expr
	{
		static constexpr int TYPE = EXPR_NOT;
		virtual Expr* evaluate(const std::unordered_map<std::string, bool>& syms) const override;

		Expr* e = 0;
	};

	struct Or : Expr
	{
		static constexpr int TYPE = EXPR_OR;
		virtual Expr* evaluate(const std::unordered_map<std::string, bool>& syms) const override;

		Expr* left = 0;
		Expr* right = 0;
	};

	struct Implies : Expr
	{
		static constexpr int TYPE = EXPR_IMPLIES;
		virtual Expr* evaluate(const std::unordered_map<std::string, bool>& syms) const override;

		Expr* left = 0;
		Expr* right = 0;
	};

	struct BidirImplies : Expr
	{
		static constexpr int TYPE = EXPR_BIDIRIMPLIES;
		virtual Expr* evaluate(const std::unordered_map<std::string, bool>& syms) const override;

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
		std::string message;
		Location loc;
	};

	enum class TokenType
	{
		Invalid,

		Pipe,
		Ampersand,
		Asterisk,
		Exclamation,
		Plus,
		LParen,
		RParen,
		LBrace,
		RBrace,
		RightArrow,
		LeftArrow,
		DoubleArrow,
		Backslash,
		Tilde,
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
		Token(TokenType t, Location loc, zbuf::str_view s) : text(s), type(t) { }

		Location loc;
		zbuf::str_view text;
		TokenType type = TokenType::Invalid;

		operator TokenType() const { return this->type; }
	};

	zst::Result<ast::Expr*, Error> parse(zbuf::str_view input);
	zst::Result<std::vector<Token>, Error> lex(zbuf::str_view input);
}
