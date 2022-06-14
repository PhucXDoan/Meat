#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "unified.h"

#define IS_ALPHA(C)         (IN_RANGE((C), 'a', 'z' + 1) || IN_RANGE((C), 'A', 'Z' + 1))
#define IS_DIGIT(C)         (IN_RANGE((C), '0', '9' + 1))
#define PASS_STRING(STRING) (STRING).size, (STRING).data
#define STRLIT(STR)         (String { sizeof(STR) - 1, STR });

struct String
{
	i32   size;
	char* data;
};

internal constexpr bool32 operator==(String& a, String& b) { return a.size  == b.size  && memcmp(a.data , b.data , a.size ) == 0; }
internal constexpr bool32 operator!=(String& a, String& b) { return !(a == b); }

enum struct SymbolKind : u8
{
	null,
	semicolon,
	plus,
	minus,
	asterisk,
	forward_slash,
	period,
	caret,
	exclamation_point,
	equal,
	comma,
	parenthesis_start,
	parenthesis_end,

	parenthetical_application,
	assert
};

enum struct TokenKind : u8
{
	null,
	symbol,
	number,
	identifier
};

struct Token
{
	TokenKind kind;
	String    string;
	union
	{
		struct
		{
			SymbolKind kind;
		} symbol;
	};
};

struct Tokenizer
{
	String stream;
	i32    current_index;
};

struct SyntaxTree
{
	Token       token;
	SyntaxTree* left;
	SyntaxTree* right;
};

struct FunctionArgumentNode
{
	FunctionArgumentNode* next_node;
	String                name;
	f32                   value;
};

enum struct VariableDeclarationStatus : u8
{
	yet_calculated,
	currently_calculating,
	cached
};

enum struct StatementType : u8
{
	null,
	assertion,
	expression,
	variable_declaration,
	function_declaration
};

struct Statement
{
	SyntaxTree*   tree;
	StatementType type;

	union
	{
		struct
		{
			Statement* corresponding_statement;
		} assertion;

		struct
		{
			bool32 is_cached;
			f32    cached_evaluation;
		} expression;

		struct
		{
			VariableDeclarationStatus status;
			f32                       cached_evaluation;
		} variable_declaration;

		struct
		{
			FunctionArgumentNode* args;
		} function_declaration;
	};
};

struct Ledger
{
	MemoryArena*          arena;
	i32                   allocated_syntax_tree_count;
	SyntaxTree*           available_syntax_tree;
	i32                   allocated_function_argument_node_count;
	FunctionArgumentNode* available_function_argument_node;
	i32                   statement_count;
	Statement             statement_buffer[64];
};

// @TODO@ Handle non-existing files.
internal Tokenizer init_tokenizer_from_file(strlit file_path)
{
	FILE* file;
	errno_t file_errno = fopen_s(&file, file_path, "rb");
	ASSERT(file_errno == 0);
	DEFER { fclose(file); };

	Tokenizer tokenizer;
	tokenizer.current_index = 0;

	fseek(file, 0, SEEK_END);
	tokenizer.stream.size = ftell(file);
	tokenizer.stream.data = reinterpret_cast<char*>(malloc(tokenizer.stream.size));
	fseek(file, 0, SEEK_SET);
	fread(tokenizer.stream.data, sizeof(char), tokenizer.stream.size, file);

	return tokenizer;
}

internal void deinit_tokenizer_from_file(Tokenizer tokenizer)
{
	free(tokenizer.stream.data);
}

internal SyntaxTree* init_single_syntax_tree(Ledger* ledger, Token token, SyntaxTree* left, SyntaxTree* right)
{
	ledger->allocated_syntax_tree_count += 1;
	SyntaxTree* allocation;

	if (ledger->available_syntax_tree)
	{
		allocation                       = ledger->available_syntax_tree;
		ledger->available_syntax_tree = ledger->available_syntax_tree->left;
	}
	else
	{
		allocation = memory_arena_allocate<SyntaxTree>(ledger->arena);
	}

	*allocation       = {};
	allocation->token = token;
	allocation->left  = left;
	allocation->right = right;

	return allocation;
}

internal void deinit_entire_syntax_tree(Ledger* ledger, SyntaxTree* tree)
{
	if (tree)
	{
		if (tree->left)
		{
			deinit_entire_syntax_tree(ledger, tree->left);
		}
		if (tree->right)
		{
			deinit_entire_syntax_tree(ledger, tree->right);
		}

		ledger->allocated_syntax_tree_count -= 1;
		ASSERT(ledger->allocated_syntax_tree_count >= 0);

		tree->left                    = ledger->available_syntax_tree;
		ledger->available_syntax_tree = tree;
	}
}

internal FunctionArgumentNode* init_function_argument_node(Ledger* ledger, String name, f32 value = 0.0f)
{
	ledger->allocated_function_argument_node_count += 1;
	FunctionArgumentNode* allocation;

	if (ledger->available_function_argument_node)
	{
		allocation                               = ledger->available_function_argument_node;
		ledger->available_function_argument_node = ledger->available_function_argument_node->next_node;
	}
	else
	{
		allocation = memory_arena_allocate<FunctionArgumentNode>(ledger->arena);
	}

	*allocation       = {};
	allocation->name  = name;
	allocation->value = value;

	return allocation;
}

internal void deinit_entire_function_argument_node(Ledger* ledger, FunctionArgumentNode* node)
{
	while (node)
	{
		ledger->allocated_function_argument_node_count -= 1;
		ASSERT(ledger->allocated_function_argument_node_count >= 0);

		FunctionArgumentNode* tail = node->next_node;
		node->next_node                          = ledger->available_function_argument_node;
		ledger->available_function_argument_node = node;
		node                                     = tail;
	}
}

internal void eat_white_space(Tokenizer* tokenizer)
{
	while (tokenizer->current_index < tokenizer->stream.size)
	{
		switch (tokenizer->stream.data[tokenizer->current_index])
		{
			case ' ':
			case '\t':
			case '\r':
			case '\n':
			{
				tokenizer->current_index += 1;
			} break;

			default:
			{
				return;
			} break;
		}
	}
}

// @TODO@ Leading decimal support.
internal Token eat_token(Tokenizer* tokenizer)
{
	eat_white_space(tokenizer);

	if (tokenizer->current_index < tokenizer->stream.size)
	{
		if (IS_DIGIT(tokenizer->stream.data[tokenizer->current_index]))
		{
			Token token;
			token.kind        = TokenKind::number;
			token.string.size = 1;
			token.string.data = tokenizer->stream.data + tokenizer->current_index;

			bool32 has_decimal = false;
			for (tokenizer->current_index += 1; tokenizer->current_index < tokenizer->stream.size; tokenizer->current_index += 1)
			{
				if (IS_DIGIT(tokenizer->stream.data[tokenizer->current_index]))
				{
					token.string.size += 1;
				}
				else if (tokenizer->stream.data[tokenizer->current_index] == '.' && !has_decimal)
				{
					token.string.size += 1;
					has_decimal        = true;
				}
				else
				{
					if (tokenizer->stream.data[tokenizer->current_index] == '.' && has_decimal)
					{
						ASSERT(false); // A decimal has already been used in the number.
						return {};
					}

					break;
				}
			}

			return token;
		}
		else
		{
			constexpr struct { strlit cstring; SymbolKind kind; } SYMBOLS[] =
				{
					{ ";"     , SymbolKind::semicolon         },
					{ "+"     , SymbolKind::plus              },
					{ "-"     , SymbolKind::minus             },
					{ "*"     , SymbolKind::asterisk          },
					{ "/"     , SymbolKind::forward_slash     },
					{ "."     , SymbolKind::period            },
					{ "^"     , SymbolKind::caret             },
					{ "!"     , SymbolKind::exclamation_point },
					{ "="     , SymbolKind::equal             },
					{ ","     , SymbolKind::comma             },
					{ "("     , SymbolKind::parenthesis_start },
					{ ")"     , SymbolKind::parenthesis_end   },
					{ "ASSERT", SymbolKind::assert            }
				};

			FOR_ELEMS(SYMBOLS)
			{
				Token token;
				token.kind        = TokenKind::symbol;
				token.string.size = 0;
				token.string.data = tokenizer->stream.data + tokenizer->current_index;
				token.symbol.kind = it->kind;

				while (tokenizer->current_index + token.string.size < tokenizer->stream.size)
				{
					if (tokenizer->stream.data[tokenizer->current_index + token.string.size] == it->cstring[token.string.size])
					{
						token.string.size += 1;
						if (it->cstring[token.string.size] == '\0')
						{
							tokenizer->current_index += token.string.size;
							return token;
						}
					}
					else
					{
						break;
					}
				}
			}

			if (IS_ALPHA(tokenizer->stream.data[tokenizer->current_index]) || tokenizer->stream.data[tokenizer->current_index] == '_')
			{
				Token token;
				token.kind        = TokenKind::identifier;
				token.string.size = 0;
				token.string.data = tokenizer->stream.data + tokenizer->current_index;

				while (IS_ALPHA(tokenizer->stream.data[tokenizer->current_index]) || tokenizer->stream.data[tokenizer->current_index] == '_')
				{
					tokenizer->current_index += 1;
					token.string.size        += 1;
				}

				return token;
			}

			ASSERT(false); // Couldn't parse for a token.
			return {};
		}
	}
	else
	{
		return {};
	}
}

internal Token peek_token(Tokenizer tokenizer)
{
	return eat_token(&tokenizer);
}

enum struct OperatorType : u8
{
	binary_left_associative,
	binary_right_associative,
	postfix,
	prefix
};

struct OperatorInfo
{
	OperatorType type;
	i32          precedence;
};

internal bool32 get_operator_info(OperatorInfo* operator_info, SymbolKind kind)
{
	switch (kind)
	{
		case SymbolKind::exclamation_point:
		operator_info->type       = OperatorType::postfix;
		operator_info->precedence = 7;
		return true;

		case SymbolKind::caret:
		operator_info->type       = OperatorType::binary_right_associative;
		operator_info->precedence = 6;
		return true;

		case SymbolKind::parenthetical_application:
		operator_info->type       = OperatorType::binary_left_associative;
		operator_info->precedence = 5;
		return true;

		case SymbolKind::asterisk:
		case SymbolKind::forward_slash:
		operator_info->type       = OperatorType::binary_left_associative;
		operator_info->precedence = 4;
		return true;

		case SymbolKind::plus:
		case SymbolKind::minus:
		operator_info->type       = OperatorType::binary_left_associative;
		operator_info->precedence = 3;
		return true;

		case SymbolKind::comma:
		operator_info->type       = OperatorType::binary_right_associative;
		operator_info->precedence = 2;
		return true;

		case SymbolKind::equal:
		operator_info->type       = OperatorType::binary_left_associative;
		operator_info->precedence = 1;
		return true;

		case SymbolKind::assert:
		operator_info->type       = OperatorType::prefix;
		operator_info->precedence = 0;
		return true;
	}

	return false;
}

// @NOTE@ https://eli.thegreenplace.net/2012/08/02/parsing-expressions-by-precedence-climbing
internal SyntaxTree* eat_syntax_tree(Tokenizer* tokenizer, Ledger* ledger, i32 min_precedence = 0)
{
	// @TODO@ Clean up.
	Token parenthetical_application_token;
	parenthetical_application_token.kind        = TokenKind::symbol;
	parenthetical_application_token.string      = STRLIT("()");
	parenthetical_application_token.symbol.kind = SymbolKind::parenthetical_application;
	OperatorInfo parenthetical_application_info;
	bool32 is_parenthetical_application_operator = get_operator_info(&parenthetical_application_info, SymbolKind::parenthetical_application);
	ASSERT(is_parenthetical_application_operator);
	Token multiplication_token;
	multiplication_token.kind        = TokenKind::symbol;
	multiplication_token.string      = STRLIT("*");
	multiplication_token.symbol.kind = SymbolKind::asterisk;
	OperatorInfo multiplication_info;
	bool32 is_multiplication_operator = get_operator_info(&multiplication_info, SymbolKind::asterisk);
	ASSERT(is_multiplication_operator);

	SyntaxTree* current_tree;

	{
		OperatorInfo operator_info;
		Token        token = peek_token(*tokenizer);

		if (token.kind == TokenKind::symbol && token.symbol.kind == SymbolKind::parenthesis_start)
		{
			eat_token(tokenizer);
			current_tree = eat_syntax_tree(tokenizer, ledger, 0);
			ASSERT(current_tree);
			token = eat_token(tokenizer);
			ASSERT(token.kind == TokenKind::symbol && token.symbol.kind == SymbolKind::parenthesis_end);
			current_tree = init_single_syntax_tree(ledger, parenthetical_application_token, 0, current_tree);
		}
		else if (token.kind == TokenKind::symbol && token.symbol.kind == SymbolKind::minus)
		{
			eat_token(tokenizer);
			current_tree = init_single_syntax_tree(ledger, token, 0, eat_syntax_tree(tokenizer, ledger, multiplication_info.precedence + (multiplication_info.type == OperatorType::binary_right_associative ? 0 : 1)));
			ASSERT(current_tree->right);
		}
		else if (token.kind == TokenKind::symbol && get_operator_info(&operator_info, token.symbol.kind) && operator_info.type == OperatorType::prefix && operator_info.precedence >= min_precedence)
		{
			eat_token(tokenizer);
			current_tree = init_single_syntax_tree(ledger, token, 0, eat_syntax_tree(tokenizer, ledger, operator_info.precedence + 1));
			ASSERT(current_tree->right);
		}
		else if (token.kind == TokenKind::number || token.kind == TokenKind::identifier)
		{
			eat_token(tokenizer);
			current_tree = init_single_syntax_tree(ledger, token, 0, 0);
		}
		else
		{
			return 0;
		}
	}

	while (true)
	{
		Token        token = peek_token(*tokenizer);
		OperatorInfo operator_info;
		if (token.kind == TokenKind::symbol && get_operator_info(&operator_info, token.symbol.kind) && operator_info.precedence >= min_precedence)
		{
			eat_token(tokenizer);

			switch (operator_info.type)
			{
				case OperatorType::binary_left_associative:
				{
					current_tree = init_single_syntax_tree(ledger, token, current_tree, eat_syntax_tree(tokenizer, ledger, operator_info.precedence + 1));
					ASSERT(current_tree->right);
				} break;

				case OperatorType::binary_right_associative:
				{
					current_tree = init_single_syntax_tree(ledger, token, current_tree, eat_syntax_tree(tokenizer, ledger, operator_info.precedence));
					ASSERT(current_tree->right);
				} break;

				case OperatorType::postfix:
				{
					current_tree = init_single_syntax_tree(ledger, token, current_tree, 0);
				} break;

				case OperatorType::prefix:
				{
					ASSERT(false); // Prefix operator encountered after atom.
				} break;
			}
		}
		else if (token.kind == TokenKind::symbol && token.symbol.kind == SymbolKind::parenthesis_start && parenthetical_application_info.precedence >= min_precedence)
		{
			eat_token(tokenizer);
			SyntaxTree* right_hand_side = eat_syntax_tree(tokenizer, ledger, 0);
			token = eat_token(tokenizer);
			ASSERT(token.kind == TokenKind::symbol && token.symbol.kind == SymbolKind::parenthesis_end);
			current_tree = init_single_syntax_tree(ledger, parenthetical_application_token, current_tree, right_hand_side);
		}
		else if
		(
			(token.kind == TokenKind::number && (current_tree->token.kind == TokenKind::identifier || current_tree->token.kind == TokenKind::symbol && current_tree->token.symbol.kind == SymbolKind::parenthetical_application)) ||
			token.kind == TokenKind::identifier && parenthetical_application_info.precedence >= min_precedence
		)
		{
			current_tree = init_single_syntax_tree(ledger, multiplication_token, current_tree, eat_syntax_tree(tokenizer, ledger, multiplication_info.precedence + (multiplication_info.type == OperatorType::binary_right_associative ? 0 : 1)));
		}
		else
		{
			break;
		}
	}

	return current_tree;
}

// @TODO@ Make this more robust.
internal f32 parse_number(String string)
{
	char buffer[64];
	sprintf_s(buffer, sizeof(buffer), "%.*s", PASS_STRING(string));
	f32 result;
	sscanf_s(buffer, "%f", &result);
	return result;
}

#if DEBUG
internal void DEBUG_print_syntax_tree(SyntaxTree* tree, i32 depth = 0, u64 path = 0)
{
	if (tree)
	{
		if (!tree->left && !tree->right)
		{
			printf("Leaf : ");
			FOR_RANGE(i, depth)
			{
				printf("%llu", (path >> i) & 0b1);
			}

			printf(" : `%.*s`\n", PASS_STRING(tree->token.string));
		}
		else
		{
			printf("Branch : ");
			FOR_RANGE(i, depth)
			{
				printf("%llu", (path >> i) & 0b1);
			}
			printf(" : `%.*s`\n", PASS_STRING(tree->token.string));

			if (tree->left)
			{
				DEBUG_print_syntax_tree(tree->left, depth + 1, path & ~(1ULL << depth));
			}
			if (tree->right)
			{
				DEBUG_print_syntax_tree(tree->right, depth + 1, path | (1ULL << depth));
			}
		}
	}
}

internal void DEBUG_print_serialized_syntax_tree(SyntaxTree* tree)
{
	if (tree)
	{
		switch (tree->token.kind)
		{
			case TokenKind::symbol:
			{
				switch (tree->token.symbol.kind)
				{
					case SymbolKind::plus:
					{
						DEBUG_print_serialized_syntax_tree(tree->left);
						printf(" + ");
						DEBUG_print_serialized_syntax_tree(tree->right);
					} break;

					case SymbolKind::minus:
					{
						if (tree->left)
						{
							DEBUG_print_serialized_syntax_tree(tree->left);
							printf(" - ");
							DEBUG_print_serialized_syntax_tree(tree->right);
						}
						else
						{
							printf("-");
							DEBUG_print_serialized_syntax_tree(tree->right);
						}
					} break;

					case SymbolKind::asterisk:
					{
						if (tree->left->token.kind == TokenKind::symbol && tree->left->token.symbol.kind == SymbolKind::parenthetical_application || tree->right->token.kind == TokenKind::symbol && tree->right->token.symbol.kind == SymbolKind::parenthetical_application)
						{
							DEBUG_print_serialized_syntax_tree(tree->left);
							DEBUG_print_serialized_syntax_tree(tree->right);
						}
						else
						{
							DEBUG_print_serialized_syntax_tree(tree->left);
							printf(" * ");
							DEBUG_print_serialized_syntax_tree(tree->right);
						}
					} break;

					case SymbolKind::forward_slash:
					{
						DEBUG_print_serialized_syntax_tree(tree->left);
						printf("/");
						DEBUG_print_serialized_syntax_tree(tree->right);
					} break;

					case SymbolKind::caret:
					{
						DEBUG_print_serialized_syntax_tree(tree->left);
						printf("^");
						DEBUG_print_serialized_syntax_tree(tree->right);
					} break;

					case SymbolKind::exclamation_point:
					{
						DEBUG_print_serialized_syntax_tree(tree->left);
						printf("!");
					} break;

					case SymbolKind::parenthetical_application:
					{
						DEBUG_print_serialized_syntax_tree(tree->left);
						printf("(");
						DEBUG_print_serialized_syntax_tree(tree->right);
						printf(")");
					} break;

					case SymbolKind::equal:
					{
						DEBUG_print_serialized_syntax_tree(tree->left);
						printf(" = ");
						DEBUG_print_serialized_syntax_tree(tree->right);
					} break;

					case SymbolKind::comma:
					{
						DEBUG_print_serialized_syntax_tree(tree->left);
						printf(", ");
						DEBUG_print_serialized_syntax_tree(tree->right);
					} break;
				}
			} break;

			case TokenKind::number:
			case TokenKind::identifier:
			{
				printf("%.*s", PASS_STRING(tree->token.string));
			} break;
		}
	}
}

internal bool32 DEBUG_token_eq(Token a, Token b)
{
	if (a.kind == TokenKind::symbol && b.kind == TokenKind::symbol && a.symbol.kind != b.symbol.kind)
	{
		return false;
	}

	return a.kind == b.kind && memcmp(a.string.data, b.string.data, a.string.size) == 0;
}

internal bool32 DEBUG_syntax_tree_eq(SyntaxTree* a, SyntaxTree* b)
{
	if (a == b)
	{
		return true;
	}
	else if (a && b)
	{
		return DEBUG_token_eq(a->token, b->token) && DEBUG_syntax_tree_eq(a->left, b->left) && DEBUG_syntax_tree_eq(a->right, b->right);
	}
	else
	{
		return false;
	}
}
#endif

internal bool32 is_token_symbol(Token token, SymbolKind kind)
{
	return token.kind == TokenKind::symbol && token.symbol.kind == kind;
}

internal void evaluate_statement(Statement* statement, Ledger* ledger, FunctionArgumentNode* binded_args = 0)
{
	lambda evaluate_expression =
		[&](SyntaxTree* tree)
		{
			Statement exp = {};
			exp.tree = tree;
			exp.type = StatementType::expression;
			evaluate_statement(&exp, ledger, binded_args);
			ASSERT(exp.expression.is_cached);
			return exp.expression.cached_evaluation;
		};

	switch (statement->type)
	{
		case StatementType::assertion:
		{
			evaluate_statement(statement->assertion.corresponding_statement, ledger);
			f32 expectant_value = parse_number(statement->tree->right->token.string);
			f32 resultant_value = NAN;

			switch (statement->assertion.corresponding_statement->type)
			{
				case StatementType::expression:
				{
					ASSERT(statement->assertion.corresponding_statement->expression.is_cached);
					resultant_value = statement->assertion.corresponding_statement->expression.cached_evaluation;
				} break;

				case StatementType::variable_declaration:
				{
					ASSERT(statement->assertion.corresponding_statement->variable_declaration.status == VariableDeclarationStatus::cached);
					resultant_value = statement->assertion.corresponding_statement->variable_declaration.cached_evaluation;
				} break;

				default:
				{
					ASSERT(false); // Unknown assertion case.
				} break;
			}

			if (fabsf(resultant_value - expectant_value) < 0.000001f)
			{
				printf("Passed assertion :: %f :: ", expectant_value);
				DEBUG_print_serialized_syntax_tree(statement->assertion.corresponding_statement->tree);
				printf("\n");
			}
			else
			{
				printf("Failed assertion :: %f :: resultant value :: %f :: ", expectant_value, resultant_value);
				DEBUG_print_serialized_syntax_tree(statement->assertion.corresponding_statement->tree);
				printf("\n");
				ASSERT(false); // Failed meat assertion.
			}
		} break;

		case StatementType::expression:
		{
			if (!statement->expression.is_cached)
			{
				switch (statement->tree->token.kind)
				{
					case TokenKind::symbol:
					{
						switch (statement->tree->token.symbol.kind)
						{
							case SymbolKind::plus:
							{
								statement->expression.cached_evaluation = evaluate_expression(statement->tree->left) + evaluate_expression(statement->tree->right);
								statement->expression.is_cached         = true;
							} break;

							case SymbolKind::minus:
							{
								if (statement->tree->left)
								{
									statement->expression.cached_evaluation = evaluate_expression(statement->tree->left) - evaluate_expression(statement->tree->right);
								}
								else
								{
									statement->expression.cached_evaluation = -evaluate_expression(statement->tree->right);
								}
								statement->expression.is_cached = true;
							} break;

							case SymbolKind::asterisk:
							{
								statement->expression.cached_evaluation = evaluate_expression(statement->tree->left) * evaluate_expression(statement->tree->right);
								statement->expression.is_cached         = true;
							} break;

							case SymbolKind::forward_slash:
							{
								statement->expression.cached_evaluation = evaluate_expression(statement->tree->left) / evaluate_expression(statement->tree->right);
								statement->expression.is_cached         = true;
							} break;

							case SymbolKind::caret:
							{
								statement->expression.cached_evaluation = powf(evaluate_expression(statement->tree->left), evaluate_expression(statement->tree->right));
								statement->expression.is_cached         = true;
							} break;

							case SymbolKind::exclamation_point:
							{
								ASSERT(!statement->tree->right);
								statement->expression.cached_evaluation = static_cast<f32>(tgamma(evaluate_expression(statement->tree->left) + 1.0));
								statement->expression.is_cached         = true;
							} break;

							case SymbolKind::parenthetical_application:
							{
								if (statement->tree->left)
								{
									if (statement->tree->left->token.kind == TokenKind::identifier)
									{
										FOR_ELEMS(function, ledger->statement_buffer, ledger->statement_count)
										{
											if (function->type == StatementType::function_declaration && function->tree->left->left->token.string == statement->tree->left->token.string)
											{
												FunctionArgumentNode* new_binded_args = 0;
												DEFER { deinit_entire_function_argument_node(ledger, new_binded_args); };

												FunctionArgumentNode** new_binded_args_nil    = &new_binded_args;
												FunctionArgumentNode*  current_function_arg   = function->function_declaration.args;
												SyntaxTree*            current_parameter_tree = statement->tree->right;
												while (true)
												{
													if (is_token_symbol(current_parameter_tree->token, SymbolKind::comma))
													{
														*new_binded_args_nil = init_function_argument_node(ledger, current_function_arg->name, evaluate_expression(current_parameter_tree->left));
														current_function_arg = current_function_arg->next_node;
													}
													else
													{
														*new_binded_args_nil = init_function_argument_node(ledger, current_function_arg->name, evaluate_expression(current_parameter_tree));
														current_function_arg = current_function_arg->next_node;
														break;
													}

													current_parameter_tree = current_parameter_tree->right;
													new_binded_args_nil = &(*new_binded_args_nil)->next_node;
												}

												ASSERT(current_function_arg == 0);

												Statement exp = {};
												exp.tree = function->tree->right;
												exp.type = StatementType::expression;
												evaluate_statement(&exp, ledger, new_binded_args);
												ASSERT(exp.expression.is_cached);
												statement->expression.cached_evaluation = exp.expression.cached_evaluation;
												statement->expression.is_cached         = true;
												return;
											}
										}
									}

									statement->expression.cached_evaluation = evaluate_expression(statement->tree->left) * evaluate_expression(statement->tree->right);
								}
								else
								{
									statement->expression.cached_evaluation = evaluate_expression(statement->tree->right);
								}
								statement->expression.is_cached = true;
							} break;

							default:
							{
								ASSERT(false); // Unknown symbol.
							} break;
						}
					} break;

					case TokenKind::number:
					{
						ASSERT(!statement->tree->left);
						ASSERT(!statement->tree->right);
						statement->expression.cached_evaluation = parse_number(statement->tree->token.string);
						statement->expression.is_cached         = true;
					} break;

					case TokenKind::identifier:
					{
						ASSERT(!statement->tree->left);
						ASSERT(!statement->tree->right);

						FOR_NODES(binded_args)
						{
							if (it->name == statement->tree->token.string)
							{
								statement->expression.cached_evaluation = it->value;
								statement->expression.is_cached         = true;
								return;
							}
						}

						FOR_ELEMS(it, ledger->statement_buffer, ledger->statement_count)
						{
							if (it->type == StatementType::variable_declaration && it->tree->left->token.string == statement->tree->token.string)
							{
								evaluate_statement(it, ledger);
								statement->expression.cached_evaluation = it->variable_declaration.cached_evaluation;
								statement->expression.is_cached         = true;
								return;
							}
						}

						ASSERT(false); // Couldn't find declaration.
					} break;

					default:
					{
						ASSERT(false); // Unknown token.
					} break;
				}
			}
		} break;

		case StatementType::variable_declaration:
		{
			switch (statement->variable_declaration.status)
			{
				case VariableDeclarationStatus::yet_calculated:
				{
					statement->variable_declaration.status            = VariableDeclarationStatus::currently_calculating;
					statement->variable_declaration.cached_evaluation = evaluate_expression(statement->tree->right);
					statement->variable_declaration.status            = VariableDeclarationStatus::cached;
				} break;

				case VariableDeclarationStatus::currently_calculating:
				{
					ASSERT(false); // Circlar definition.
				} break;

				case VariableDeclarationStatus::cached:
				{
				} break;

				default:
				{
					ASSERT(false); // Unknown variable declaration status.
				} break;
			};
		} break;

		case StatementType::function_declaration:
		{
		} break;

		default:
		{
			ASSERT(false); // Unknown statement type.
		} break;
	}
}

int main(void)
{
	//
	// Initialization.
	//

	Tokenizer tokenizer = init_tokenizer_from_file(DATA_DIR "meat.meat");
	DEFER { deinit_tokenizer_from_file(tokenizer); };

	MemoryArena memory_arena;
	memory_arena.size = MEBIBYTES_OF(1);
	memory_arena.base = reinterpret_cast<byte*>(malloc(memory_arena.size));
	memory_arena.used = 0;
	DEFER { free(memory_arena.base); };

	Ledger ledger = {};
	ledger.arena = &memory_arena;
	DEFER
	{
		FOR_ELEMS(it, ledger.statement_buffer, ledger.statement_count)
		{
			deinit_entire_syntax_tree(&ledger, it->tree);

			if (it->type == StatementType::function_declaration)
			{
				deinit_entire_function_argument_node(&ledger, it->function_declaration.args);
			}
		}

		ASSERT(ledger.allocated_syntax_tree_count == 0);
		ASSERT(ledger.allocated_function_argument_node_count == 0);
	};

	////
	//// Interpreting.
	////

	while (peek_token(tokenizer).kind != TokenKind::null)
	{
		ASSERT(IN_RANGE(ledger.statement_count, 0, ARRAY_CAPACITY(ledger.statement_buffer)));
		aliasing statement = ledger.statement_buffer[ledger.statement_count];
		ledger.statement_count += 1;

		statement = {};
		statement.tree = eat_syntax_tree(&tokenizer, &ledger);
		ASSERT(statement.tree);

		if (is_token_symbol(statement.tree->token, SymbolKind::assert))
		{
			ASSERT(IN_RANGE(ledger.statement_count - 2, 0, ledger.statement_count));
			statement.type                              = StatementType::assertion;
			statement.assertion.corresponding_statement = &ledger.statement_buffer[ledger.statement_count - 2];
		}
		else if (is_token_symbol(statement.tree->token, SymbolKind::equal))
		{
			if (is_token_symbol(statement.tree->left->token, SymbolKind::parenthetical_application))
			{
				ASSERT(statement.tree->left->left);
				ASSERT(statement.tree->left->left->token.kind == TokenKind::identifier);
				FOR_ELEMS(it, ledger.statement_buffer, ledger.statement_count - 1)
				{
					ASSERT(it->type != StatementType::variable_declaration || it->tree->left->token.string != statement.tree->left->left->token.string);
					ASSERT(it->type != StatementType::function_declaration || it->tree->left->left->token.string != statement.tree->left->left->token.string);
				}

				statement.type = StatementType::function_declaration;

				FunctionArgumentNode** args_nil = &statement.function_declaration.args;
				for (SyntaxTree* tree = statement.tree->left->right; tree; tree = tree->right)
				{
					if (is_token_symbol(tree->token, SymbolKind::comma))
					{
						ASSERT(tree->left->token.kind == TokenKind::identifier);
						*args_nil = init_function_argument_node(&ledger, tree->left->token.string);
					}
					else
					{
						ASSERT(tree->token.kind == TokenKind::identifier);
						*args_nil = init_function_argument_node(&ledger, tree->token.string);
						break;
					}

					args_nil  = &(*args_nil)->next_node;
				}
			}
			else
			{
				// @TODO@ `x = x` is not noticed as ill-formed.

				ASSERT(statement.tree->left->token.kind == TokenKind::identifier);
				ASSERT(!statement.tree->left->left);
				ASSERT(!statement.tree->left->right);
				FOR_ELEMS(it, ledger.statement_buffer, ledger.statement_count - 1)
				{
					ASSERT(it->type != StatementType::variable_declaration || it->tree->left->token.string != statement.tree->left->token.string);
					ASSERT(it->type != StatementType::function_declaration || it->tree->left->left->token.string != statement.tree->left->token.string);
				}

				statement.type = StatementType::variable_declaration;
			}
		}
		else
		{
			statement.type = StatementType::expression;
		}

		Token terminating_token = eat_token(&tokenizer);
		ASSERT(is_token_symbol(terminating_token, SymbolKind::semicolon));

		DEBUG_print_syntax_tree(statement.tree);
		printf("===================\n");
	}

	FOR_ELEMS(it, ledger.statement_buffer, ledger.statement_count)
	{
		evaluate_statement(it, &ledger);

		switch (it->type)
		{
			case StatementType::assertion:
			{
			} break;

			case StatementType::variable_declaration:
			{
				ASSERT(it->variable_declaration.status == VariableDeclarationStatus::cached);
				printf("%f :: ", it->variable_declaration.cached_evaluation);
				DEBUG_print_serialized_syntax_tree(it->tree);
				printf("\n");
			} break;

			case StatementType::expression:
			{
				ASSERT(it->expression.is_cached);
				printf("%f :: ", it->expression.cached_evaluation);
				DEBUG_print_serialized_syntax_tree(it->tree);
				printf("\n");
			} break;

			case StatementType::function_declaration:
			{
			} break;

			default:
			{
				ASSERT(false); // Unknown statement type.
			} break;
		}
	}

	//while (true)
	//{
	//	Token token = eat_token(&tokenizer);

	//	if (token.kind == TokenKind::null)
	//	{
	//		ASSERT(tokenizer.current_index == tokenizer.stream.size); // Tokenizer must finish the stream.
	//		break;
	//	}
	//	else
	//	{
	//		if (token.kind == TokenKind::number)
	//		{
	//			printf("Token : `%.*s` : %f\n", PASS_STRING(token.string), parse_number(token.string));
	//		}
	//		else
	//		{
	//			printf("Token : `%.*s`\n", PASS_STRING(token.string));
	//		}
	//	}
	//}

	DEBUG_STDOUT_HALT();

	return 0;
}
