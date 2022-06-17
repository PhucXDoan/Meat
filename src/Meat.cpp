#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "unified.h"

enum struct TokenKind : u8
{
	eof,
	assertion,

	identifier,
	number,

	semicolon,
	parenthesis_start,
	parenthesis_end,

	parenthetical_application,
	equal,
	comma,
	plus,
	minus,
	asterisk,
	forward_slash,
	caret,
	exclamation_point
};

struct Token
{
	TokenKind  kind;
	StringView string;
};

struct TokenBufferNode
{
	TokenBufferNode* next_node;
	i32              count;
	Token            buffer[64];
};

struct Tokenizer
{
	i32              file_size;
	char*            file_data;
	i32              index_in_current_token_buffer_node;
	TokenBufferNode* head_token_buffer_node;
	TokenBufferNode* current_token_buffer_node; // @TODO@ If not too restrictive, could deinitialize a buffer node before moving onto the next.
};

struct SyntaxTree
{
	union
	{
		SyntaxTree* next_node;
		SyntaxTree* left;
	};
	SyntaxTree* right;
	Token       token;
};

struct FunctionArgumentNode
{
	FunctionArgumentNode* next_node;
	StringView            name;
	f32                   value;
};

struct Allocator
{
	MemoryArena           arena;
	i32                   allocated_token_buffer_node_count;
	i32                   allocated_syntax_tree_count;
	i32                   allocated_function_argument_node_count;
	TokenBufferNode*      available_token_buffer_node;
	SyntaxTree*           available_syntax_tree;
	FunctionArgumentNode* available_function_argument_node;
};

struct Value
{
	f32 number;
};

typedef Value Function(FunctionArgumentNode*);

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
	i32       statement_count;
	Statement statement_buffer[64];
};

#include "predefined.cpp"
#include "meta/predefined.h"

internal SyntaxTree* init_single_syntax_tree(Allocator* allocator, Token token, SyntaxTree* left, SyntaxTree* right)
{
	allocator->allocated_syntax_tree_count += 1;

	SyntaxTree* allocation = memory_arena_allocate_from_available(&allocator->available_syntax_tree, &allocator->arena);
	*allocation       = {};
	allocation->token = token;
	allocation->left  = left;
	allocation->right = right;
	return allocation;
}

internal void deinit_entire_syntax_tree(Allocator* allocator, SyntaxTree* tree)
{
	if (tree)
	{
		if (tree->left)
		{
			deinit_entire_syntax_tree(allocator, tree->left);
		}
		if (tree->right)
		{
			deinit_entire_syntax_tree(allocator, tree->right);
		}

		tree->left                       = allocator->available_syntax_tree;
		allocator->available_syntax_tree = tree;

		allocator->allocated_syntax_tree_count -= 1;
		ASSERT(allocator->allocated_syntax_tree_count >= 0);

	}
}

internal FunctionArgumentNode* init_function_argument_node(Allocator* allocator, StringView name, f32 value = 0.0f)
{
	allocator->allocated_function_argument_node_count += 1;

	FunctionArgumentNode* allocation = memory_arena_allocate_from_available(&allocator->available_function_argument_node, &allocator->arena);
	*allocation       = {};
	allocation->name  = name;
	allocation->value = value;
	return allocation;
}

internal void deinit_entire_function_argument_node(Allocator* allocator, FunctionArgumentNode* node)
{
	while (node)
	{
		push_single_node(pop_node(&node), &allocator->available_function_argument_node);

		allocator->allocated_function_argument_node_count -= 1;
		ASSERT(allocator->allocated_function_argument_node_count >= 0);
	}
}

internal TokenBufferNode* init_token_buffer_node(Allocator* allocator)
{
	allocator->allocated_token_buffer_node_count += 1;

	TokenBufferNode* allocation = memory_arena_allocate_from_available(&allocator->available_token_buffer_node, &allocator->arena);
	*allocation = {};
	return allocation;
}

internal void deinit_entire_token_buffer_node(Allocator* allocator, TokenBufferNode* node)
{
	while (node)
	{
		push_single_node(pop_node(&node), &allocator->available_token_buffer_node);

		allocator->allocated_token_buffer_node_count -= 1;
		ASSERT(allocator->allocated_token_buffer_node_count >= 0);
	}
}

internal void deinit_tokenizer(Allocator* allocator, Tokenizer* tokenizer)
{
	free(tokenizer->file_data);
	deinit_entire_token_buffer_node(allocator, tokenizer->head_token_buffer_node);
}

struct InitTokenizerStatus
{
	StringView message;
};

internal bool32 init_tokenizer(InitTokenizerStatus* status, Tokenizer* tokenizer, Allocator* allocator, strlit file_path)
{
	FILE* file;
	if (fopen_s(&file, file_path, "rb"))
	{
		status->message = string_builder_quick(&allocator->arena, "You received an error in attempting to open `%s`.", file_path);
		return true;
	}
	DEFER { fclose(file); };

	fseek(file, 0, SEEK_END);
	tokenizer->file_size = ftell(file);
	tokenizer->file_data = reinterpret_cast<char*>(malloc(tokenizer->file_size));
	fseek(file, 0, SEEK_SET);
	fread(tokenizer->file_data, sizeof(char), tokenizer->file_size, file);

	tokenizer->index_in_current_token_buffer_node = 0;
	tokenizer->head_token_buffer_node             = init_token_buffer_node(allocator);
	tokenizer->current_token_buffer_node          = tokenizer->head_token_buffer_node;

	for (i32 current_index = 0;;)
	{
		while (current_index < tokenizer->file_size)
		{
			if
			(
				tokenizer->file_data[current_index] == ' '  ||
				tokenizer->file_data[current_index] == '\t' ||
				tokenizer->file_data[current_index] == '\r' ||
				tokenizer->file_data[current_index] == '\n'
			)
			{
				current_index += 1;
			}
			else if (tokenizer->file_data[current_index] == '/' && current_index + 1 < tokenizer->file_size && tokenizer->file_data[current_index + 1] == '/')
			{
				while (current_index < tokenizer->file_size && tokenizer->file_data[current_index] != '\n')
				{
					current_index += 1;
				}
			}
			else
			{
				break;
			}
		}

		if (current_index < tokenizer->file_size)
		{
			if (tokenizer->current_token_buffer_node->count == ARRAY_CAPACITY(tokenizer->current_token_buffer_node->buffer))
			{
				tokenizer->current_token_buffer_node->next_node = init_token_buffer_node(allocator);
				tokenizer->current_token_buffer_node            = tokenizer->current_token_buffer_node->next_node;
			}

			Token* token       = &tokenizer->current_token_buffer_node->buffer[tokenizer->current_token_buffer_node->count];
			token->string.data = tokenizer->file_data + current_index;

			if (is_digit(tokenizer->file_data[current_index]) || tokenizer->file_data[current_index] == '.')
			{
				token->kind        = TokenKind::number;
				token->string.size = 1;

				bool32 has_decimal = tokenizer->file_data[current_index] == '.';
				while (current_index + token->string.size < tokenizer->file_size)
				{
					if (!is_digit(tokenizer->file_data[current_index + token->string.size]))
					{
						if (tokenizer->file_data[current_index + token->string.size] == '.')
						{
							if (has_decimal)
							{
								status->message = STRING_VIEW_OF("There is already a decimal in the number.");
								goto ABORT;
							}
							else
							{
								has_decimal = true;
							}
						}
						else
						{
							if (has_decimal && token->string.size <= 1)
							{
								status->message = STRING_VIEW_OF("I'm not sure how to interpret the single decimal.");
								goto ABORT;
							}

							goto PROCESS_TOKEN;
						}
					}

					token->string.size += 1;
				}
			}
			else
			{
				constexpr struct { strlit cstring; TokenKind kind; } RESERVED[] =
					{
						{ "ASSERT", TokenKind::assertion         },
						{ ";"     , TokenKind::semicolon         },
						{ "="     , TokenKind::equal             },
						{ ","     , TokenKind::comma             },
						{ "+"     , TokenKind::plus              },
						{ "-"     , TokenKind::minus             },
						{ "*"     , TokenKind::asterisk          },
						{ "/"     , TokenKind::forward_slash     },
						{ "^"     , TokenKind::caret             },
						{ "!"     , TokenKind::exclamation_point },
						{ "("     , TokenKind::parenthesis_start },
						{ ")"     , TokenKind::parenthesis_end   }
					};

				FOR_ELEMS(RESERVED)
				{
					token->kind        = it->kind;
					token->string.size = 0;

					while (current_index + token->string.size < tokenizer->file_size)
					{
						if (tokenizer->file_data[current_index + token->string.size] == it->cstring[token->string.size])
						{
							token->string.size += 1;
							if (it->cstring[token->string.size] == '\0')
							{
								goto PROCESS_TOKEN;
							}
						}
						else
						{
							break;
						}
					}
				}

				if (is_alpha(tokenizer->file_data[current_index]) || tokenizer->file_data[current_index] == '_')
				{
					token->kind        = TokenKind::identifier;
					token->string.size = 1;

					while (is_alpha(tokenizer->file_data[current_index + token->string.size]) || is_digit(tokenizer->file_data[current_index + token->string.size]) || tokenizer->file_data[current_index + token->string.size] == '_')
					{
						token->string.size += 1;
					}
				}
				else
				{
					status->message = string_builder_quick(&allocator->arena, "You typed the unknown token `%c`.", tokenizer->file_data[current_index]);
					goto ABORT;
				}

				goto PROCESS_TOKEN;
			}

			PROCESS_TOKEN:;
			tokenizer->current_token_buffer_node->count += 1;
			current_index                               += token->string.size;
			continue;

			ABORT:;
			deinit_tokenizer(allocator, tokenizer);
			return true;
		}
		else
		{
			break;
		}
	}

	tokenizer->current_token_buffer_node = tokenizer->head_token_buffer_node;

	return false;
}

// @TODO@ Avoid copy.
internal Token peek_token(Tokenizer* tokenizer)
{
	if (tokenizer->current_token_buffer_node)
	{
		ASSERT(IN_RANGE(tokenizer->index_in_current_token_buffer_node, 0, tokenizer->current_token_buffer_node->count));
		return tokenizer->current_token_buffer_node->buffer[tokenizer->index_in_current_token_buffer_node];
	}
	else
	{
		return {};
	}
}

internal Token eat_token(Tokenizer* tokenizer)
{
	if (tokenizer->current_token_buffer_node)
	{
		ASSERT(IN_RANGE(tokenizer->index_in_current_token_buffer_node, 0, tokenizer->current_token_buffer_node->count));
		Token token = tokenizer->current_token_buffer_node->buffer[tokenizer->index_in_current_token_buffer_node];
		tokenizer->index_in_current_token_buffer_node += 1;
		if (tokenizer->index_in_current_token_buffer_node == tokenizer->current_token_buffer_node->count)
		{
			ASSERT(IFF(tokenizer->current_token_buffer_node->count == ARRAY_CAPACITY(tokenizer->current_token_buffer_node->buffer), tokenizer->current_token_buffer_node->next_node));
			tokenizer->index_in_current_token_buffer_node = 0;
			tokenizer->current_token_buffer_node          = tokenizer->current_token_buffer_node->next_node;
		}
		return token;
	}
	else
	{
		return {};
	}
}

enum struct Associativity : u8
{
	prefix,
	postfix,
	binary_left_associative,
	binary_right_associative
};

struct TokenOrder
{
	Associativity associativity;
	i32           precedence;
};

internal bool32 try_get_token_order(TokenOrder* token_order, TokenKind kind)
{
	switch (kind)
	{
		case TokenKind::exclamation_point:
		token_order->associativity = Associativity::postfix;
		token_order->precedence    = 7;
		return true;

		case TokenKind::caret:
		token_order->associativity = Associativity::binary_right_associative;
		token_order->precedence    = 6;
		return true;

		case TokenKind::parenthetical_application:
		token_order->associativity = Associativity::binary_left_associative;
		token_order->precedence    = 5;
		return true;

		case TokenKind::asterisk:
		case TokenKind::forward_slash:
		token_order->associativity = Associativity::binary_left_associative;
		token_order->precedence    = 4;
		return true;

		case TokenKind::plus:
		case TokenKind::minus:
		token_order->associativity = Associativity::binary_left_associative;
		token_order->precedence    = 3;
		return true;

		case TokenKind::comma:
		token_order->associativity = Associativity::binary_right_associative;
		token_order->precedence    = 2;
		return true;

		case TokenKind::equal:
		token_order->associativity = Associativity::binary_left_associative;
		token_order->precedence    = 1;
		return true;

		case TokenKind::assertion:
		token_order->associativity = Associativity::prefix;
		token_order->precedence    = 0;
		return true;
	}

	return false;
}

// @NOTE@ https://eli.thegreenplace.net/2012/08/02/parsing-expressions-by-precedence-climbing
internal SyntaxTree* eat_syntax_tree(Tokenizer* tokenizer, Ledger* ledger, Allocator* allocator, i32 min_precedence = 0)
{
	// @TODO@ Clean up.
	Token parenthetical_application_token;
	parenthetical_application_token.kind   = TokenKind::parenthetical_application;
	parenthetical_application_token.string = STRING_VIEW_OF("()");
	TokenOrder parenthetical_application_order;
	bool32 parenthetical_application_has_order = try_get_token_order(&parenthetical_application_order, TokenKind::parenthetical_application);
	ASSERT(parenthetical_application_has_order);

	Token multiplication_token;
	multiplication_token.kind   = TokenKind::asterisk;
	multiplication_token.string = STRING_VIEW_OF("*");
	TokenOrder multiplication_order;
	bool32 multiplication_has_order = try_get_token_order(&multiplication_order, TokenKind::asterisk);
	ASSERT(multiplication_has_order);

	SyntaxTree* current_tree;

	{
		Token      token = peek_token(tokenizer);
		TokenOrder token_order;
		if (token.kind == TokenKind::minus)
		{
			eat_token(tokenizer);
			current_tree = init_single_syntax_tree(allocator, token, 0, eat_syntax_tree(tokenizer, ledger, allocator, multiplication_order.precedence + (multiplication_order.associativity == Associativity::binary_right_associative ? 0 : 1)));
			ASSERT(current_tree->right);
		}
		else if (try_get_token_order(&token_order, token.kind) && token_order.associativity == Associativity::prefix && token_order.precedence >= min_precedence)
		{
			eat_token(tokenizer);
			current_tree = init_single_syntax_tree(allocator, token, 0, eat_syntax_tree(tokenizer, ledger, allocator, token_order.precedence + 1));
			ASSERT(current_tree->right);
		}
		else if (token.kind == TokenKind::parenthesis_start)
		{
			eat_token(tokenizer);
			current_tree = eat_syntax_tree(tokenizer, ledger, allocator, 0);
			ASSERT(current_tree);
			token = eat_token(tokenizer);
			ASSERT(token.kind == TokenKind::parenthesis_end);
			current_tree = init_single_syntax_tree(allocator, parenthetical_application_token, 0, current_tree);
		}
		else if (token.kind == TokenKind::number || token.kind == TokenKind::identifier)
		{
			eat_token(tokenizer);
			current_tree = init_single_syntax_tree(allocator, token, 0, 0);
		}
		else
		{
			return 0;
		}
	}

	while (true)
	{
		Token      token = peek_token(tokenizer);
		TokenOrder token_order;
		if (try_get_token_order(&token_order, token.kind) && token_order.precedence >= min_precedence)
		{
			eat_token(tokenizer);

			switch (token_order.associativity)
			{
				case Associativity::binary_left_associative:
				{
					current_tree = init_single_syntax_tree(allocator, token, current_tree, eat_syntax_tree(tokenizer, ledger, allocator, token_order.precedence + 1));
					ASSERT(current_tree->right);
				} break;

				case Associativity::binary_right_associative:
				{
					current_tree = init_single_syntax_tree(allocator, token, current_tree, eat_syntax_tree(tokenizer, ledger, allocator, token_order.precedence));
					ASSERT(current_tree->right);
				} break;

				case Associativity::postfix:
				{
					current_tree = init_single_syntax_tree(allocator, token, current_tree, 0);
				} break;

				case Associativity::prefix:
				{
					ASSERT(false); // Prefix operator encountered after atom.
				} break;
			}
		}
		else if (token.kind == TokenKind::parenthesis_start && parenthetical_application_order.precedence >= min_precedence)
		{
			eat_token(tokenizer);
			SyntaxTree* right_hand_side = eat_syntax_tree(tokenizer, ledger, allocator, 0);
			token = eat_token(tokenizer);
			ASSERT(token.kind == TokenKind::parenthesis_end);
			current_tree = init_single_syntax_tree(allocator, parenthetical_application_token, current_tree, right_hand_side);
		}
		else if // @TODO@ Might be bugged?
		(
			(token.kind == TokenKind::number && (current_tree->token.kind == TokenKind::identifier || current_tree->token.kind == TokenKind::parenthetical_application))
			|| token.kind == TokenKind::identifier && parenthetical_application_order.precedence >= min_precedence
		)
		{
			current_tree = init_single_syntax_tree(allocator, multiplication_token, current_tree, eat_syntax_tree(tokenizer, ledger, allocator, multiplication_order.precedence + (multiplication_order.associativity == Associativity::binary_right_associative ? 0 : 1)));
		}
		else
		{
			break;
		}
	}

	return current_tree;
}

// @TODO@ Make this more robust.
internal f32 parse_number(StringView string)
{
	char buffer[64];
	sprintf_s(buffer, sizeof(buffer), "%.*s", PASS_STRING_VIEW(string));
	f32 result;
	sscanf_s(buffer, "%f", &result);
	return result;
}

internal bool32 is_name_defined(StringView name, Ledger* ledger)
{
	FOR_ELEMS(PREDEFINED_CONSTANTS)
	{
		if (it->name == name)
		{
			return true;
		}
	}

	FOR_ELEMS(PREDEFINED_FUNCTIONS)
	{
		if (it->name == name)
		{
			return true;
		}
	}

	FOR_ELEMS(it, ledger->statement_buffer, ledger->statement_count)
	{
		if (it->type == StatementType::variable_declaration && it->tree->left->token.string == name)
		{
			return true;
		}
		else if (it->type == StatementType::function_declaration && it->tree->left->left->token.string == name)
		{
			return true;
		}
	}

	return false;
};

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

			printf(" : `%.*s`\n", PASS_STRING_VIEW(tree->token.string));
		}
		else
		{
			printf("Branch : ");
			FOR_RANGE(i, depth)
			{
				printf("%llu", (path >> i) & 0b1);
			}
			printf(" : `%.*s`\n", PASS_STRING_VIEW(tree->token.string));

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
			case TokenKind::number:
			case TokenKind::identifier:
			{
				printf("%.*s", PASS_STRING_VIEW(tree->token.string));
			} break;

			case TokenKind::plus:
			{
				DEBUG_print_serialized_syntax_tree(tree->left);
				printf(" + ");
				DEBUG_print_serialized_syntax_tree(tree->right);
			} break;

			case TokenKind::minus:
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

			case TokenKind::asterisk:
			{
				if (tree->left->token.kind == TokenKind::parenthetical_application || tree->right->token.kind == TokenKind::parenthetical_application)
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

			case TokenKind::forward_slash:
			{
				DEBUG_print_serialized_syntax_tree(tree->left);
				printf("/");
				DEBUG_print_serialized_syntax_tree(tree->right);
			} break;

			case TokenKind::caret:
			{
				DEBUG_print_serialized_syntax_tree(tree->left);
				printf("^");
				DEBUG_print_serialized_syntax_tree(tree->right);
			} break;

			case TokenKind::exclamation_point:
			{
				DEBUG_print_serialized_syntax_tree(tree->left);
				printf("!");
			} break;

			case TokenKind::parenthetical_application:
			{
				DEBUG_print_serialized_syntax_tree(tree->left);
				printf("(");
				DEBUG_print_serialized_syntax_tree(tree->right);
				printf(")");
			} break;

			case TokenKind::equal:
			{
				DEBUG_print_serialized_syntax_tree(tree->left);
				printf(" = ");
				DEBUG_print_serialized_syntax_tree(tree->right);
			} break;

			case TokenKind::comma:
			{
				DEBUG_print_serialized_syntax_tree(tree->left);
				printf(", ");
				DEBUG_print_serialized_syntax_tree(tree->right);
			} break;
		}
	}
}

internal bool32 DEBUG_token_eq(Token a, Token b)
{
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

internal void evaluate_statement(Statement* statement, Ledger* ledger, Allocator* allocator, FunctionArgumentNode* binded_args = 0)
{
	lambda evaluate_expression =
		[&](SyntaxTree* tree)
		{
			Statement exp = {};
			exp.tree = tree;
			exp.type = StatementType::expression;
			evaluate_statement(&exp, ledger, allocator, binded_args);
			ASSERT(exp.expression.is_cached);
			return exp.expression.cached_evaluation;
		};

	switch (statement->type)
	{
		case StatementType::assertion:
		{
			evaluate_statement(statement->assertion.corresponding_statement, ledger, allocator);
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

						FOR_ELEMS(PREDEFINED_CONSTANTS)
						{
							if (it->name == statement->tree->token.string)
							{
								statement->expression.cached_evaluation = it->value.number;
								statement->expression.is_cached         = true;
								return;
							}
						}

						FOR_ELEMS(it, ledger->statement_buffer, ledger->statement_count)
						{
							if (it->type == StatementType::variable_declaration && it->tree->left->token.string == statement->tree->token.string)
							{
								evaluate_statement(it, ledger, allocator);
								statement->expression.cached_evaluation = it->variable_declaration.cached_evaluation;
								statement->expression.is_cached         = true;
								return;
							}
						}

						ASSERT(false); // Couldn't find declaration.
					} break;

					case TokenKind::number:
					{
						ASSERT(!statement->tree->left);
						ASSERT(!statement->tree->right);
						statement->expression.cached_evaluation = parse_number(statement->tree->token.string);
						statement->expression.is_cached         = true;
					} break;

					case TokenKind::plus:
					{
						statement->expression.cached_evaluation = evaluate_expression(statement->tree->left) + evaluate_expression(statement->tree->right);
						statement->expression.is_cached         = true;
					} break;

					case TokenKind::minus:
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

					case TokenKind::asterisk:
					{
						statement->expression.cached_evaluation = evaluate_expression(statement->tree->left) * evaluate_expression(statement->tree->right);
						statement->expression.is_cached         = true;
					} break;

					case TokenKind::forward_slash:
					{
						statement->expression.cached_evaluation = evaluate_expression(statement->tree->left) / evaluate_expression(statement->tree->right);
						statement->expression.is_cached         = true;
					} break;

					case TokenKind::caret:
					{
						statement->expression.cached_evaluation = powf(evaluate_expression(statement->tree->left), evaluate_expression(statement->tree->right));
						statement->expression.is_cached         = true;
					} break;

					case TokenKind::exclamation_point:
					{
						ASSERT(!statement->tree->right);
						statement->expression.cached_evaluation = static_cast<f32>(tgamma(evaluate_expression(statement->tree->left) + 1.0));
						statement->expression.is_cached         = true;
					} break;

					case TokenKind::parenthetical_application:
					{
						if (statement->tree->left)
						{
							if (statement->tree->left->token.kind == TokenKind::identifier)
							{
								FOR_ELEMS(PREDEFINED_FUNCTIONS)
								{
									if (it->name == statement->tree->left->token.string)
									{
										FunctionArgumentNode* arguments = 0;
										DEFER { deinit_entire_function_argument_node(allocator, arguments); };

										FunctionArgumentNode** arguments_nil = &arguments;
										for (SyntaxTree* current_parameter_tree = statement->tree->right; current_parameter_tree; current_parameter_tree = current_parameter_tree->right)
										{
											if (current_parameter_tree->token.kind == TokenKind::comma)
											{
												*arguments_nil = init_function_argument_node(allocator, {}, evaluate_expression(current_parameter_tree->left));
											}
											else
											{
												*arguments_nil = init_function_argument_node(allocator, {}, evaluate_expression(current_parameter_tree));
												break;
											}

											arguments_nil = &(*arguments_nil)->next_node;
										}

										statement->expression.cached_evaluation = it->function(arguments).number; // @TODO@ Assumes all values are numbers.
										statement->expression.is_cached         = true;
										return;
									}
								}

								FOR_ELEMS(function, ledger->statement_buffer, ledger->statement_count)
								{
									if (function->type == StatementType::function_declaration && function->tree->left->left->token.string == statement->tree->left->token.string)
									{
										FunctionArgumentNode* new_binded_args = 0;
										DEFER { deinit_entire_function_argument_node(allocator, new_binded_args); };

										FunctionArgumentNode** new_binded_args_nil    = &new_binded_args;
										FunctionArgumentNode*  current_function_arg   = function->function_declaration.args;
										SyntaxTree*            current_parameter_tree = statement->tree->right;
										while (true)
										{
											if (current_parameter_tree->token.kind == TokenKind::comma)
											{
												*new_binded_args_nil = init_function_argument_node(allocator, current_function_arg->name, evaluate_expression(current_parameter_tree->left));
												current_function_arg = current_function_arg->next_node;
											}
											else
											{
												*new_binded_args_nil = init_function_argument_node(allocator, current_function_arg->name, evaluate_expression(current_parameter_tree));
												current_function_arg = current_function_arg->next_node;
												break;
											}

											current_parameter_tree = current_parameter_tree->right;
											new_binded_args_nil    = &(*new_binded_args_nil)->next_node;
										}

										ASSERT(current_function_arg == 0);

										Statement exp = {};
										exp.tree = function->tree->right;
										exp.type = StatementType::expression;
										evaluate_statement(&exp, ledger, allocator, new_binded_args);
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
	DEFER { DEBUG_STDOUT_HALT(); };

	//
	// Initialization.
	//

	Ledger ledger = {};

	Allocator allocator  = {};
	allocator.arena.size = MEBIBYTES_OF(1);
	allocator.arena.base = reinterpret_cast<byte*>(malloc(allocator.arena.size));
	allocator.arena.used = 0;
	DEFER
	{
		// @NOTE@ Makes sure every initialization has been deinitialized.
		FOR_ELEMS(it, ledger.statement_buffer, ledger.statement_count)
		{
			deinit_entire_syntax_tree(&allocator, it->tree);

			if (it->type == StatementType::function_declaration)
			{
				deinit_entire_function_argument_node(&allocator, it->function_declaration.args);
			}
		}

		ASSERT(allocator.allocated_token_buffer_node_count      == 0);
		ASSERT(allocator.allocated_syntax_tree_count            == 0);
		ASSERT(allocator.allocated_function_argument_node_count == 0);

		free(allocator.arena.base);
	};

	Tokenizer tokenizer;
	{
		InitTokenizerStatus status;
		if (init_tokenizer(&status, &tokenizer, &allocator, DATA_DIR "meat.meat"))
		{
			printf("%.*s\n", PASS_STRING_VIEW(status.message));
			return -1;
		}
	}
	DEFER { deinit_tokenizer(&allocator, &tokenizer); };

	//
	// Interpreting.
	//

	while (peek_token(&tokenizer).kind != TokenKind::eof)
	{
		ASSERT(IN_RANGE(ledger.statement_count, 0, ARRAY_CAPACITY(ledger.statement_buffer)));
		aliasing statement = ledger.statement_buffer[ledger.statement_count];
		ledger.statement_count += 1;

		statement = {};
		statement.tree = eat_syntax_tree(&tokenizer, &ledger, &allocator);
		ASSERT(statement.tree);

		if (statement.tree->token.kind == TokenKind::assertion)
		{
			ASSERT(IN_RANGE(ledger.statement_count - 2, 0, ledger.statement_count));
			statement.type                              = StatementType::assertion;
			statement.assertion.corresponding_statement = &ledger.statement_buffer[ledger.statement_count - 2];
		}
		else if (statement.tree->token.kind == TokenKind::equal)
		{
			if (statement.tree->left->token.kind == TokenKind::parenthetical_application)
			{
				ASSERT(statement.tree->left->left);
				ASSERT(statement.tree->left->left->token.kind == TokenKind::identifier);
				ASSERT(!is_name_defined(statement.tree->left->left->token.string, &ledger));

				statement.type = StatementType::function_declaration;

				FunctionArgumentNode** args_nil = &statement.function_declaration.args;
				for (SyntaxTree* tree = statement.tree->left->right; tree; tree = tree->right)
				{
					if (tree->token.kind == TokenKind::comma)
					{
						ASSERT(tree->left->token.kind == TokenKind::identifier);
						*args_nil = init_function_argument_node(&allocator, tree->left->token.string);
					}
					else
					{
						ASSERT(tree->token.kind == TokenKind::identifier);
						*args_nil = init_function_argument_node(&allocator, tree->token.string);
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
				ASSERT(!is_name_defined(statement.tree->left->token.string, &ledger));

				statement.type = StatementType::variable_declaration;
			}
		}
		else
		{
			statement.type = StatementType::expression;
		}

		Token terminating_token = eat_token(&tokenizer);
		ASSERT(terminating_token.kind == TokenKind::semicolon);

		DEBUG_print_syntax_tree(statement.tree);
		printf("===================\n");
	}

	FOR_ELEMS(it, ledger.statement_buffer, ledger.statement_count)
	{
		evaluate_statement(it, &ledger, &allocator);

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

	#if 0
	FOR_NODES(node, tokenizer.head_token_buffer_node)
	{
		printf(":: Token buffer node\n");
		FOR_ELEMS(it, node->buffer, node->count)
		{
			printf("Token : `%.*s`\n", PASS_STRING_VIEW(it->string));
		}
	}
	#elif 0
	while (true)
	{
		Token token = eat_token(&tokenizer);

		if (token.kind == TokenKind::eof)
		{
			break;
		}
		else
		{
			printf("Token : `%.*s`\n", PASS_STRING_VIEW(token.string));
		}
	}
	#endif

	return 0;
}
