#ifdef JAY_IMPLEMENTATION

#include <ctype.h>
#include <stdio.h>
#include <string.h>

typedef enum {
	TOKEN_LEFT_BRACE,
	TOKEN_RIGHT_BRACE,
	TOKEN_LEFT_BRACKET,
	TOKEN_RIGHT_BRACKET,
	TOKEN_COMMA,
	TOKEN_COLON,
	TOKEN_STRING,
	TOKEN_NUMBER,
	TOKEN_TRUE,
	TOKEN_FALSE,
	TOKEN_NULL,
	TOKEN_EOF,
	TOKEN_UNKNOWN
} token_type;

typedef struct {
	token_type type;
	const char *start;
	int length;
	int line;
} token;

typedef enum {
	JSON_OBJECT,
	JSON_ARRAY,
	JSON_STRING,
	JSON_NUMBER,
	JSON_TRUE,
	JSON_FALSE,
	JSON_NULL
} json_type;

typedef struct json_value json_value;

typedef struct {
	char *key;
	json_value *value;
} json_keypair;

struct json_value {
	json_type type;
	union {
		struct {
			json_keypair *pairs;
			int count;
		} object;
		struct {
			json_value *values;
			int count;
		} array;
		char *string;
		double number;
	} as;
};

static const char *start;
static const char *lexer_current;
static int line;

void initLexer(const char *source)
{
	start = source;
	lexer_current = source;
	line = 1;
}

char lexer_advance()
{
	lexer_current++;
	return lexer_current[-1];
}

char peek()
{
	return *lexer_current;
}

int end()
{
	return *lexer_current == '\0';
}

char peek_next()
{
	if (end()) return '\0';
	return lexer_current[1];
}

token gen_token(token_type type)
{
	token token;
	token.type = type;
	token.start = start;
	token.length = lexer_current - start;
	token.line = line;
	return token;
}

void skip_whitespace()
{
	while (1) {
		char c = peek();
		switch (c) {
			case ' ':
			case '\r':
			case '\t':
				lexer_advance();
				break;

			case '\n':
				line++;
				lexer_advance();
				break;

			default:
				return;
		}
	}
}

token string()
{
	while (peek() != '"' && !end()) {
		if (peek() == '\n')
			line++;
		if (peek() == '\\')
			lexer_advance();
		lexer_advance();
	}

	if (end()) {
		fprintf(stderr, "jay: Unterminated string");
		exit(1);
	}

	lexer_advance();
	return gen_token(TOKEN_STRING);
}

token number()
{
	while (isdigit(peek())) lexer_advance();

	if (peek() == '.' && isdigit(peek_next())) {
		lexer_advance();

		while (isdigit(peek())) lexer_advance();
	}

	return gen_token(TOKEN_NUMBER);
}

token_type check_keyword(int length, const char *rest, token_type type)
{
	if ((lexer_current - start) == 5 && memcmp(start + 1, rest, length - 1) == 0) {
		return type;
	}
	fprintf(stderr, "jay: Unknown keyword \"%.*s\" at line %d\n", length, start, line);
	exit(1);
	return TOKEN_UNKNOWN;
}

token_type check_identifier_type()
{
	switch (start[0]) {
		case 'f': return check_keyword(5, "alse", TOKEN_FALSE);
		case 'n': return check_keyword(4, "ull", TOKEN_NULL);
		case 't': return check_keyword(4, "rue", TOKEN_TRUE);
		default: return TOKEN_UNKNOWN;
	}
}

token identifier()
{
	while (isalpha(peek())) lexer_advance();
	token_type type = check_identifier_type();
	if (type == TOKEN_UNKNOWN) {
		char *identifier = strndup(start, lexer_current - start);
		fprintf(stderr, "jay: Unknown identifier \"%s\" at line %d\n", identifier, line);
		exit(1);
	}
	return gen_token(type);
}

token get_next_token()
{
	skip_whitespace();
	start = lexer_current;

	if (end())
		return gen_token(TOKEN_EOF);

	char c = lexer_advance();

	if (isdigit(c))
		return number();
	if (isalpha(c))
		return identifier();

	switch (c) {
		case '{': return gen_token(TOKEN_LEFT_BRACE);
		case '}': return gen_token(TOKEN_RIGHT_BRACE);
		case '[': return gen_token(TOKEN_LEFT_BRACKET);
		case ']': return gen_token(TOKEN_RIGHT_BRACKET);
		case ',': return gen_token(TOKEN_COMMA);
		case ':': return gen_token(TOKEN_COLON);
		case '"': return string();
	}

	fprintf(stderr, "jay: Unexpected character \"%c\" at line %d\n", c, line);
	exit(1);
}

token parser_current;
token parser_previous;

void parser_advance()
{
	parser_previous = parser_current;
	parser_current = get_next_token();
}

int match(token_type type)
{
	if (parser_current.type == type) {
		parser_advance();
		return 1;
	}
	return 0;
}

void consume(token_type type, const char *message)
{
	if (parser_current.type == type) {
		parser_advance();
		return;
	}
	fprintf(stderr, "jay: %s at line %d\n", message, parser_current.line);
	exit(1);
}

json_value* parse_value();

json_value* parse_object()
{
	json_value *object = malloc(sizeof(json_value));
	object->type = JSON_OBJECT;
	object->as.object.pairs = NULL;
	object->as.object.count = 0;

	while (!match(TOKEN_RIGHT_BRACE)) {
		consume(TOKEN_STRING, "Expected string");
		char *key = strndup(parser_previous.start + 1, parser_previous.length - 2);

		consume(TOKEN_COLON, "Expected ':' after key");
		json_value* value = parse_value();

		object->as.object.pairs = realloc(object->as.object.pairs, sizeof(json_keypair) * (object->as.object.count + 1));
		object->as.object.pairs[object->as.object.count].key = key;
		object->as.object.pairs[object->as.object.count++].value = value;

		if (!match(TOKEN_COMMA) && parser_current.type != TOKEN_RIGHT_BRACE) {
			fprintf(stderr, "jay: Expected ',' or '}' after value.\n");
			exit(1);
		}
	}

	return object;
}

json_value *parse_array()
{
	json_value *array = malloc(sizeof(json_value));
	array->type = JSON_ARRAY;
	array->as.array.values = NULL;
	array->as.array.count = 0;

	while (!match(TOKEN_RIGHT_BRACKET)) {
		json_value *value = parse_value();
		array->as.array.values = realloc(array->as.array.values, sizeof(json_value) * (array->as.array.count + 1));
		array->as.array.values[array->as.array.count++] = *value;

		if (!match(TOKEN_COMMA) && parser_current.type != TOKEN_RIGHT_BRACKET) {
			fprintf(stderr, "jay: Expected ',' or ']' after value.\n");
			exit(1);
		}
	}

	return array;
}

json_value *parse_value()
{
	if (match(TOKEN_LEFT_BRACE)) return parse_object();
	if (match(TOKEN_LEFT_BRACKET)) return parse_array();
	if (match(TOKEN_STRING)) {
		json_value *string = malloc(sizeof(json_value));
		string->type = JSON_STRING;
		string->as.string = strndup(parser_previous.start + 1, parser_previous.length - 2);
		return string;
	}
	if (match(TOKEN_NUMBER)) {
		json_value *val = malloc(sizeof(json_value));
		val->type = JSON_NUMBER;
		val->as.number = strtod(parser_previous.start, NULL);
		return val;
	}
	if (match(TOKEN_TRUE)) {
		json_value *val = malloc(sizeof(json_value));
		val->type = JSON_TRUE;
		return val;
	}
	if (match(TOKEN_FALSE)) {
		json_value *val = malloc(sizeof(json_value));
		val->type = JSON_FALSE;
		return val;
	}
	if (match(TOKEN_NULL)) {
		json_value *val = malloc(sizeof(json_value));
		val->type = JSON_NULL;
		return val;
	}

	fprintf(stderr, "jay: Unexpected value at line %d\n", parser_current.line);
	exit(1);
	return NULL;
}

json_value *jay_parse(const char *source)
{
	initLexer(source);
	parser_advance();
	return parse_value();
}

void jay_print(json_value *value, int indent)
{
	if (value == NULL) return;

	switch (value->type) {
		case JSON_OBJECT:
			printf("{\n");
			for (int i = 0; i < value->as.object.count; i++) {
				for (int j = 0; j < indent + 1; j++) printf("  ");
				printf("\033[34m\"%s\"\033[m: ", value->as.object.pairs[i].key);
				jay_print(value->as.object.pairs[i].value, indent + 1);
				if (i < value->as.object.count - 1) printf(",");
				printf("\n");
			}
			for (int i = 0; i < indent; i++) printf("  ");
			printf("}");
			break;
		case JSON_ARRAY:
			printf("[\n");
			for (int i = 0; i < value->as.array.count; i++) {
				for (int j = 0; j < indent + 1; j++) printf("  ");
				jay_print(&value->as.array.values[i], indent + 1);
				if (i < value->as.array.count - 1) printf(",\n");
			}
			printf("\n");
			for (int i = 0; i < indent; i++) printf("  ");
			printf("]");
			break;
		case JSON_STRING:
			printf("\033[32m\"%s\"\033[m", value->as.string);
			break;
		case JSON_NUMBER:
			printf("%f", value->as.number);
			break;
		case JSON_TRUE:
			printf("true");
			break;
		case JSON_FALSE:
			printf("false");
			break;
		case JSON_NULL:
			printf("null");
			break;
	}
}

#endif
