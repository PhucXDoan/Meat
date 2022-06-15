#include <stdio.h>
#include "unified.h"

#define META_printf(FSTR, ...)\
do\
{\
	printf((FSTR), __VA_ARGS__);\
	DEBUG_printf((FSTR), __VA_ARGS__);\
}\
while (false)

#define META_ASSERT_(CONDITION)\
do\
{\
	if (!(CONDITION))\
	{\
		META_printf("Metaprogram failed assertion (" MACRO_STRINGIFY_(__LINE__) ") :: " #CONDITION "\n");\
		*((i32*)0) = 0;\
		exit(-1);\
	}\
}\
while (false)

#undef  ASSERT
#define ASSERT META_ASSERT_

// @TODO@ Multiple character tokens (e.g. `<=`) are not handled.
enum struct TokenKind : u8
{
	null,

	// ...
	// @NOTE@ Single character ASCII
	// ...

	identifier = 128,
	number,
};

struct Token
{
	TokenKind  kind;
	StringView string;
};

struct Tokenizer
{
	i32   stream_size;
	char* stream_data;
	i32   current_index;
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
	tokenizer.stream_size = ftell(file);
	tokenizer.stream_data = reinterpret_cast<char*>(malloc(tokenizer.stream_size));
	fseek(file, 0, SEEK_SET);
	fread(tokenizer.stream_data, sizeof(char), tokenizer.stream_size, file);

	return tokenizer;
}

internal void deinit_tokenizer_from_file(Tokenizer tokenizer)
{
	free(tokenizer.stream_data);
}

// @TODO@ Does not handle cases like escaped newlines.
internal void eat_white_space(Tokenizer* tokenizer)
{
	while (tokenizer->current_index < tokenizer->stream_size)
	{
		switch (tokenizer->stream_data[tokenizer->current_index])
		{
			case ' ':
			case '\t':
			case '\r':
			case '\n':
			{
				tokenizer->current_index += 1;
			} break;

			case '/':
			{
				if (tokenizer->current_index + 1 < tokenizer->stream_size && tokenizer->stream_data[tokenizer->current_index + 1] == '/')
				{
					while (tokenizer->current_index < tokenizer->stream_size && tokenizer->stream_data[tokenizer->current_index] != '\n')
					{
						tokenizer->current_index += 1;
					}
				}
				else if (tokenizer->current_index + 1 < tokenizer->stream_size && tokenizer->stream_data[tokenizer->current_index + 1] == '*')
				{
					while (tokenizer->current_index + 1 < tokenizer->stream_size && !(tokenizer->stream_data[tokenizer->current_index] == '*' && tokenizer->stream_data[tokenizer->current_index + 1] == '/'))
					{
						tokenizer->current_index += 1;
					}
					tokenizer->current_index += 2;
				}
				else
				{
					return;
				}
			} break;

			default:
			{
				return;
			} break;
		}
	}
}

internal Token eat_token(Tokenizer* tokenizer)
{
	eat_white_space(tokenizer);

	if (tokenizer->current_index < tokenizer->stream_size)
	{
		if (is_alpha(tokenizer->stream_data[tokenizer->current_index]) || tokenizer->stream_data[tokenizer->current_index] == '_')
		{
			Token token;
			token.kind        = TokenKind::identifier;
			token.string.size = 0;
			token.string.data = tokenizer->stream_data + tokenizer->current_index;

			while (is_alpha(tokenizer->stream_data[tokenizer->current_index]) || is_digit(tokenizer->stream_data[tokenizer->current_index]) || tokenizer->stream_data[tokenizer->current_index] == '_')
			{
				tokenizer->current_index += 1;
				token.string.size        += 1;
			}

			return token;
		}
		else if (is_digit(tokenizer->stream_data[tokenizer->current_index]) || tokenizer->stream_data[tokenizer->current_index] == '.')
		{
			Token token;
			token.kind        = TokenKind::number;
			token.string.size = 1;
			token.string.data = tokenizer->stream_data + tokenizer->current_index;

			bool32 has_decimal = tokenizer->stream_data[tokenizer->current_index] == '.';
			bool32 has_suffix  = false;
			for (tokenizer->current_index += 1; tokenizer->current_index < tokenizer->stream_size; tokenizer->current_index += 1)
			{
				if (is_digit(tokenizer->stream_data[tokenizer->current_index]) && !has_suffix)
				{
					token.string.size += 1;
				}
				else if (tokenizer->stream_data[tokenizer->current_index] == '.' && !has_decimal && !has_suffix)
				{
					token.string.size += 1;
					has_decimal        = true;
				}
				else if (is_alpha(tokenizer->stream_data[tokenizer->current_index]))
				{
					token.string.size += 1;
					has_suffix         = true;
				}
				else
				{
					if (tokenizer->stream_data[tokenizer->current_index] == '.' && has_decimal)
					{
						ASSERT(false);
						return {};
					}

					break;
				}
			}

			ASSERT(!has_decimal || (token.string.size > 1 && token.string.data[token.string.size - 1] != '.'));

			return token;
		}
		else
		{
			Token token;
			token.kind   = static_cast<TokenKind>(tokenizer->stream_data[tokenizer->current_index]);
			token.string = { 1, tokenizer->stream_data + tokenizer->current_index };

			tokenizer->current_index += 1;
			return token;
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

int main(void)
{
	Tokenizer tokenizer = init_tokenizer_from_file(SRC_DIR "predefined.cpp");
	DEFER { deinit_tokenizer_from_file(tokenizer); };

	FILE* output_file;
	errno_t output_file_errno = fopen_s(&output_file, SRC_DIR "meta/predefined.h", "wb");
	ASSERT(output_file_errno == 0);
	DEFER { fclose(output_file); };

	constexpr StringView CONSTANT_PREFIX = STRING_VIEW_OF("constant_");
	constexpr StringView FUNCTION_PREFIX = STRING_VIEW_OF("function_");

	i32        predefined_constant_count = 0;
	StringView predefined_constant_buffer[64];

	i32        predefined_function_count = 0;
	StringView predefined_function_buffer[64];

	while (tokenizer.current_index < tokenizer.stream_size)
	{
		Token token = eat_token(&tokenizer);

		switch (token.kind)
		{
			case TokenKind::null:
			{
				ASSERT(tokenizer.current_index == tokenizer.stream_size);
			} break;

			case TokenKind::identifier:
			{
				if (starts_with(CONSTANT_PREFIX, token.string))
				{
					ASSERT(token.string.size > CONSTANT_PREFIX.size);
					predefined_constant_buffer[predefined_constant_count]  = token.string;
					predefined_constant_count                             += 1;
				}
				else if (starts_with(FUNCTION_PREFIX, token.string))
				{
					ASSERT(token.string.size > FUNCTION_PREFIX.size);
					predefined_function_buffer[predefined_function_count]  = token.string;
					predefined_function_count                             += 1;
				}
			} break;
		}
	}

	fprintf
	(
		output_file,
		"#pragma once\n"
		"\n"
		"global constexpr struct { StringView name; Value value; } PREDEFINED_CONSTANTS[] =\n"
		"\t{\n"
	);

	FOR_ELEMS(it, predefined_constant_buffer, predefined_constant_count)
	{
		fprintf(output_file, "\t\t{ STRING_VIEW_OF(\"%.*s\"), %.*s },\n", it->size - CONSTANT_PREFIX.size, it->data + CONSTANT_PREFIX.size, PASS_STRING_VIEW(*it));
	}

	fprintf
	(
		output_file,
		"\t};\n"
		"\n"
		"global constexpr struct { StringView name; Function* function; } PREDEFINED_FUNCTIONS[] =\n"
		"\t{\n"
	);

	FOR_ELEMS(it, predefined_function_buffer, predefined_function_count)
	{
		fprintf(output_file, "\t\t{ STRING_VIEW_OF(\"%.*s\"), %.*s },\n", it->size - FUNCTION_PREFIX.size, it->data + FUNCTION_PREFIX.size, PASS_STRING_VIEW(*it));
	}

	fprintf
	(
		output_file,
		"\t};\n"
	);

	return 0;
}
