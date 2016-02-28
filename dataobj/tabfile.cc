/*
 * Copyright (c) 1997 - 2001 Hansj�rg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#ifdef MAKEOBJ
#include "../besch/writer/obj_writer.h"
#endif

#include "../simdebug.h"
#include "../display/simgraph.h"
#include "koord.h"
#include "tabfile.h"


bool tabfile_t::open(const char *filename)
{
	close();
	file = fopen(filename, "r");
	return file != NULL;
}



void tabfile_t::close()
{
	if(file) {
		fclose(file);
		file  = NULL;
	}
}


const char *tabfileobj_t::get(const char *key)
{
	obj_info_t *result = objinfo.access(key);
	if(  result  ) {
		result->retrieved = true;
		return result->str;
	}
	return "";
}


/**
 * Get the string value for a key - key must be lowercase
 * @return def if key isn't found, value otherwise
 * @author Hj. Malthaner
 */
const char *tabfileobj_t::get_string(const char *key, const char * def)
{
	obj_info_t *result = objinfo.access(key);
	if(  result  ) {
		result->retrieved = true;
		return result->str;
	}
	return def;
}



bool tabfileobj_t::put(const char *key, const char *value)
{
	if(objinfo.get(key).str) {
		return false;
	}
	objinfo.put(strdup(key), obj_info_t(false,strdup(value)) );
	return true;
}



void tabfileobj_t::clear()
{
	FOR(stringhashtable_tpl<obj_info_t>, const& i, objinfo) {
		free(const_cast<char*>(i.key));
		free(const_cast<char*>(i.value.str));
	}
	objinfo.clear();
}


// private helps to get x y value pairs needed for koord etc.
bool tabfileobj_t::get_x_y( const char *key, sint16 &x, sint16 &y )
{
	const char *value = get_string(key,NULL);
	const char *tmp;

	if(!value) {
		return false;
	}
	// 2. Determine value
	for(tmp = value; *tmp != ','; tmp++) {
		if(!*tmp) {
			return false;
		}
	}
	x = atoi(value);
	y = atoi(tmp + 1);
	return true;
}

const koord &tabfileobj_t::get_koord(const char *key, koord def)
{
	static koord ret;
	ret = def;
	get_x_y( key, ret.x, ret.y );
	return ret;
}

const scr_coord &tabfileobj_t::get_scr_coord(const char *key, scr_coord def)
{
	static scr_coord ret;
	ret = def;
	get_x_y( key, ret.x, ret.y );
	return ret;
}

const scr_size &tabfileobj_t::get_scr_size(const char *key, scr_size def)
{
	static scr_size ret;
	ret = def;
	get_x_y( key, ret.w, ret.h );
	return ret;
}

#ifdef MAKEOBJ
// returns next matching color to an rgb
uint8 display_get_index_from_rgb( uint8 r, uint8 g, uint8 b )
{
	// the players colors and colors for simple drawing operations
	// each eight colors are corresponding to a player color
	static const uint8 special_pal[224*3]=
	{
		36, 75, 103,
		57, 94, 124,
		76, 113, 145,
		96, 132, 167,
		116, 151, 189,
		136, 171, 211,
		156, 190, 233,
		176, 210, 255,
		88, 88, 88,
		107, 107, 107,
		125, 125, 125,
		144, 144, 144,
		162, 162, 162,
		181, 181, 181,
		200, 200, 200,
		219, 219, 219,
		17, 55, 133,
		27, 71, 150,
		37, 86, 167,
		48, 102, 185,
		58, 117, 202,
		69, 133, 220,
		79, 149, 237,
		90, 165, 255,
		123, 88, 3,
		142, 111, 4,
		161, 134, 5,
		180, 157, 7,
		198, 180, 8,
		217, 203, 10,
		236, 226, 11,
		255, 249, 13,
		86, 32, 14,
		110, 40, 16,
		134, 48, 18,
		158, 57, 20,
		182, 65, 22,
		206, 74, 24,
		230, 82, 26,
		255, 91, 28,
		34, 59, 10,
		44, 80, 14,
		53, 101, 18,
		63, 122, 22,
		77, 143, 29,
		92, 164, 37,
		106, 185, 44,
		121, 207, 52,
		0, 86, 78,
		0, 108, 98,
		0, 130, 118,
		0, 152, 138,
		0, 174, 158,
		0, 196, 178,
		0, 218, 198,
		0, 241, 219,
		74, 7, 122,
		95, 21, 139,
		116, 37, 156,
		138, 53, 173,
		160, 69, 191,
		181, 85, 208,
		203, 101, 225,
		225, 117, 243,
		59, 41, 0,
		83, 55, 0,
		107, 69, 0,
		131, 84, 0,
		155, 98, 0,
		179, 113, 0,
		203, 128, 0,
		227, 143, 0,
		87, 0, 43,
		111, 11, 69,
		135, 28, 92,
		159, 45, 115,
		183, 62, 138,
		230, 74, 174,
		245, 121, 194,
		255, 156, 209,
		20, 48, 10,
		44, 74, 28,
		68, 99, 45,
		93, 124, 62,
		118, 149, 79,
		143, 174, 96,
		168, 199, 113,
		193, 225, 130,
		54, 19, 29,
		82, 44, 44,
		110, 69, 58,
		139, 95, 72,
		168, 121, 86,
		197, 147, 101,
		226, 173, 115,
		255, 199, 130,
		8, 11, 100,
		14, 22, 116,
		20, 33, 139,
		26, 44, 162,
		41, 74, 185,
		57, 104, 208,
		76, 132, 231,
		96, 160, 255,
		43, 30, 46,
		68, 50, 85,
		93, 70, 110,
		118, 91, 130,
		143, 111, 170,
		168, 132, 190,
		193, 153, 210,
		219, 174, 230,
		63, 18, 12,
		90, 38, 30,
		117, 58, 42,
		145, 78, 55,
		172, 98, 67,
		200, 118, 80,
		227, 138, 92,
		255, 159, 105,
		11, 68, 30,
		33, 94, 56,
		54, 120, 81,
		76, 147, 106,
		98, 174, 131,
		120, 201, 156,
		142, 228, 181,
		164, 255, 207,
		64, 0, 0,
		96, 0, 0,
		128, 0, 0,
		192, 0, 0,
		255, 0, 0,
		255, 64, 64,
		255, 96, 96,
		255, 128, 128,
		0, 128, 0,
		0, 196, 0,
		0, 225, 0,
		0, 240, 0,
		0, 255, 0,
		64, 255, 64,
		94, 255, 94,
		128, 255, 128,
		0, 0, 128,
		0, 0, 192,
		0, 0, 224,
		0, 0, 255,
		0, 64, 255,
		0, 94, 255,
		0, 106, 255,
		0, 128, 255,
		128, 64, 0,
		193, 97, 0,
		215, 107, 0,
		255, 128, 0,
		255, 128, 0,
		255, 149, 43,
		255, 170, 85,
		255, 193, 132,
		8, 52, 0,
		16, 64, 0,
		32, 80, 4,
		48, 96, 4,
		64, 112, 12,
		84, 132, 20,
		104, 148, 28,
		128, 168, 44,
		164, 164, 0,
		193, 193, 0,
		215, 215, 0,
		255, 255, 0,
		255, 255, 32,
		255, 255, 64,
		255, 255, 128,
		255, 255, 172,
		32, 4, 0,
		64, 20, 8,
		84, 28, 16,
		108, 44, 28,
		128, 56, 40,
		148, 72, 56,
		168, 92, 76,
		184, 108, 88,
		64, 0, 0,
		96, 8, 0,
		112, 16, 0,
		120, 32, 8,
		138, 64, 16,
		156, 72, 32,
		174, 96, 48,
		192, 128, 64,
		32, 32, 0,
		64, 64, 0,
		96, 96, 0,
		128, 128, 0,
		144, 144, 0,
		172, 172, 0,
		192, 192, 0,
		224, 224, 0,
		64, 96, 8,
		80, 108, 32,
		96, 120, 48,
		112, 144, 56,
		128, 172, 64,
		150, 210, 68,
		172, 238, 80,
		192, 255, 96,
		32, 32, 32,
		48, 48, 48,
		64, 64, 64,
		80, 80, 80,
		96, 96, 96,
		172, 172, 172,
		236, 236, 236,
		255, 255, 255,
		41, 41, 54,
		60, 45, 70,
		75, 62, 108,
		95, 77, 136,
		113, 105, 150,
		135, 120, 176,
		165, 145, 218,
		198, 191, 232,
	};

	uint8 result = 0;
	unsigned diff = 256*3;
	for(  unsigned i=0;  i<lengthof(special_pal);  i+=3  ) {
		unsigned cur_diff = abs(r-special_pal[i+0]) + abs(g-special_pal[i+1]) + abs(b-special_pal[i+2]);
		if(  cur_diff < diff  ) {
			result = i/3;
			diff = cur_diff;
		}
	}
	return result;
}
#endif


uint8 tabfileobj_t::get_color(const char *key, uint8 def)
{
	const char *value = get_string(key,NULL);

	if(!value) {
		return def;
	}
	else {
		// skip spaces/tabs
		while ( *value>0  &&  *value<=32  ) {
			value ++;
		}
		if(  *value=='#'  ) {
			uint32 rgb = strtoul( value+1, NULL, 16 ) & 0XFFFFFFul;
			return display_get_index_from_rgb( rgb>>16, (rgb>>8)&0xFF, rgb&0xFF );
		}
		else {
			// this inputs also hex correct
			return (uint8)strtol( value, NULL, 0 );
		}
	}
}

int tabfileobj_t::get_int(const char *key, int def)
{
	const char *value = get_string(key,NULL);

	if(!value) {
		return def;
	}
	else {
		// skip spaces/tabs
		while ( *value>0  &&  *value<=32  ) {
			value ++;
		}
		// this inputs also hex correct
		return strtol( value, NULL, 0 );
	}
}


sint64 atosint64(const char* a)
{
	return (sint64)(atof(a)+0.5);
}


sint64 tabfileobj_t::get_int64(const char *key, sint64 def)
{
	const char *value = get_string(key,NULL);

	if(!value) {
		return def;
	}
	else {
		return atosint64(value);
	}
}


int *tabfileobj_t::get_ints(const char *key)
{
	const char *value = get_string(key,NULL);
	const char *tmp;
	int         count = 1;
	int         *result;

	if(!value) {
		result = new int[1];
		result[0] = 0;
		return result;
	}
	// Determine number
	for(tmp = value; *tmp; tmp++) {
		if(*tmp == ',') {
			count++;
		}
	}
	// Create result vector and fill
	result = new int[count + 1];

	result[0] = count;
	count = 1;
	result[count++] = strtol( value, NULL, 0 );
	for(tmp = value; *tmp; tmp++) {
		if(*tmp == ',') {
			// skip spaces/tabs
			do {
				tmp ++;
			} while ( *tmp>0  &&  *tmp<=32  );
			// this inputs also hex correct
			result[count++] = strtol( tmp, NULL, 0 );
		}
	}
	return result;
}


sint64 *tabfileobj_t::get_sint64s(const char *key)
{
	const char *value = get_string(key,NULL);
	const char *tmp;
	int         count = 1;
	sint64         *result;

	if(!value) {
		result = new sint64[1];
		result[0] = 0;
		return result;
	}
	// Determine number
	for(tmp = value; *tmp; tmp++) {
		if(*tmp == ',') {
			count++;
		}
	}
	// Create result vector and fill
	result = new sint64[count + 1];

	result[0] = count;
	count = 1;
	result[count++] = atosint64(value);
	for(tmp = value; *tmp; tmp++) {
		if(*tmp == ',') {
			result[count++] = atosint64(tmp + 1);
		}
	}
	return result;
}


void tabfileobj_t::unused( const char *exclude_start_chars )
{
	FOR(stringhashtable_tpl<obj_info_t>, const& i, objinfo) {
		if(  !i.value.retrieved  ) {
			// never retrieved
			if(  !exclude_start_chars  ||  !strchr( exclude_start_chars, *i.value.str )  ) {
#ifdef MAKEOBJ
				dbg->warning( obj_writer_t::last_name, "Entry \"%s=%s\" ignored (check spelling)", i.key, i.value.str );
#else
				dbg->warning( "tabfile_t", "Entry \"%s=%s\" ignored (check spelling)", i.key, i.value.str );
#endif
			}
			objinfo.access( i.key )->retrieved = true;
		}
	}
}


bool tabfile_t::read(tabfileobj_t &objinfo)
{
	bool lines = false;
	char line[4096];

	char line_expand[4096];
	char delim_expand[4096];
	char buffer[4096];
	char *param[10];
	char *expansion[10];

	objinfo.clear();

	do {
		while(read_line(line, sizeof(line)) && *line != '-') {
			char *delim = strchr(line, '=');

			if(delim) {
				*delim++ = '\0';
				format_key(line);

				int parameters = 0;
				int expansions = 0;
				if(find_parameter_expansion(line, delim, &parameters, &expansions, param, expansion) > 0) {
					int parameter_value[10][256];
					int parameter_length[10];
					int parameter_values[10];
					int combinations=1;

					for(int i=0; i<parameters; i++) {
						char *token_ptr;
						parameter_length[i] = strcspn(param[i],"]");
						parameter_values[i] = 0;
						sprintf(buffer, "%.*s", parameter_length[i], param[i]);

						parameter_value[i][parameter_values[i]++] = atoi(buffer);

						token_ptr = strtok(buffer,"-,");
						while (token_ptr != NULL && parameter_values[i]<256) {
							switch(param[i][token_ptr-buffer-1]) {
								case ',':
									parameter_value[i][parameter_values[i]++] = atoi(token_ptr);
								break;
								case '-':
									int start_range = parameter_value[i][parameter_values[i]-1];
									int end_range = atoi(token_ptr);
									for(int range=start_range; range<end_range; range++) {
										parameter_value[i][parameter_values[i]++] = range+1;
									}
							}
							token_ptr = strtok(NULL, "-,");
						}
						combinations*=parameter_values[i];
					}

					for(int i=0; i<combinations; i++) {
						int combination[10];
						if(parameters>0) {
							for(int j=0; j<parameters; j++) {
								combination[j] = i;
								for(int k=0; k<j; k++) {
									combination[j]/=parameter_values[k];
								}
								combination[j]%=parameter_values[j];
							}
							sprintf(line_expand, "%.*s%d", param[0]-line, line, parameter_value[0][combination[0]]);
							for(int i=1; i<parameters; i++) {
								char *prev_end = param[i-1]+parameter_length[i-1];
								sprintf(buffer, "%.*s%d", param[i]-prev_end, prev_end, parameter_value[i][combination[i]]);
								strcat(line_expand, buffer);
							}
							strcat(line_expand, param[parameters-1]+parameter_length[parameters-1]);
						}
						else {
							strcpy(line_expand, line);
						}

						if(expansions>0) {
							int expansion_length[10];
							int expansion_value[10];

							for(int i=0; i<expansions; i++) {
								expansion_length[i] = strcspn(expansion[i],">")-1;
								sprintf(buffer, "%.*s", expansion_length[i]+1, expansion[i]);

								expansion_value[i] = calculate(buffer, parameter_value, parameters, combination);
							}

							sprintf(delim_expand, "%.*s%d", expansion[0]-delim, delim, expansion_value[0]);
							for(int i=1; i<expansions; i++) {
								char *prev_end = expansion[i-1]+expansion_length[i-1]+2;
								sprintf(buffer, "%.*s%d", expansion[i]-prev_end, prev_end, expansion_value[i]);
								strcat(delim_expand, buffer);
							}
							strcat(delim_expand, expansion[expansions-1]+expansion_length[expansions-1]+2);
						}
						else {
							strcpy(delim_expand, delim);
						}

						objinfo.put(line_expand, delim_expand);
					}
				}
				else {
					objinfo.put(line, delim);
				}
				lines = true;
			}
			else if(  *line  ) {
				dbg->warning( "tabfile_t::read", "No data in \"%s\"", line );
			}
		}
	} while(!lines && !feof(file)); // skip empty objects

	return lines;
}



bool tabfile_t::read_line(char *s, int size)
{
	char *r;
	size_t l;

	do {
		r = fgets(s, size, file);
	} while(r != NULL  &&  (*s == '#' || *s == ' ')  );

	if(r) {
		l = strlen(r);
		while(  l  &&  (r[l-1] == '\n' || r[l-1] == '\r')  ) {
			r[--l] = '\0';
		}
	}
	return r != NULL;
}



int tabfile_t::find_parameter_expansion(char *key, char *data, int *parameters, int *expansions, char *param_ptr[10], char *expansion_ptr[10])
{
	char *s;

	// find params in key
	for(s = key; *s; s++) {
		if(*s == '[') {
			bool parameter = false;
			s++;

			char *t = s;

			while(*s && *s != ']') {
				if( (*s == ',' || *s == '-') && *(s-1) != '[' ) {
					parameter = true;
				}
				s++;
			}
			if(parameter) {
				param_ptr[*parameters] = t;
				(*parameters)++;
			}
		}
	}


	// find expansions in data
	for(s = data; *s; s++) {
		if(*s == '<') {
			expansion_ptr[*expansions] = s;
			(*expansions)++;
		}
	}

	return (*parameters)+(*expansions);
}



int tabfile_t::calculate(char *expression, int parameter_value[10][256], int parameters, int combination[10])
{
	char processed[4096];
	add_operator_brackets(expression, processed);
	return calculate_internal(processed, parameter_value, parameters, combination);
}



void tabfile_t::add_operator_brackets(char *expression, char *processed)
{
	char buffer[4096];
	char operators[5] = { '%','/','*','-','+' };

	// first remove any spaces
	int j = 0;
	for(uint16 i = 0; i<=strlen(expression); i++) {
		if(expression[i]!=' ') {
			buffer[j++]=expression[i];
		}
	}
	strcpy(processed, buffer);

	// add brackets around each operator to ensure processed in correct order
	// e.g. v+w*x+y/z becomes (((v+(w*x))+(y/z)) and x/y/z = ((x/y)/z)

	// for each operator take 'a operator b' and turn into '(a operator b)'

	for(int operator_loop = 0 ; operator_loop<5; operator_loop++) {
		int operator_count = 0;

		char *expression_ptr = strchr(processed, operators[operator_loop]);
		while(expression_ptr!=NULL) {
			// find a
			char *expression_pos = expression_ptr-1;
			char *expression_start = NULL;
			while(expression_pos>=processed && !expression_start) {
				switch(expression_pos[0]) {
					case ')': {
						int bracket_level = 1;
						expression_start = expression_pos-1;
						while(expression_start>processed && bracket_level>0) {
							switch(expression_start[0]) {
								case ')':
									bracket_level++;
								break;
								case '(':
									bracket_level--;
								break;
							}
							expression_start--;
						}
						if(bracket_level>0) dbg->fatal( "tabfile_t::read", "mismatched brackets" );
					}
					break;
					case '%':
					case '/':
					case '*':
					case '+':
					case '-':
					case '(':
					case '<':
						expression_start = expression_pos;
					break;
				}
				expression_pos--;
			}
			if(expression_start==NULL) expression_start=expression_pos;

			// find b
			expression_pos = expression_ptr+1;
			char *expression_end = NULL;
			while(expression_pos[0]!='\0' && !expression_end) {
				switch(expression_pos[0]) {
					case '(': {
						int bracket_level = 1;
						expression_end = expression_pos+1;
						while(expression_end[0]!='\0' && bracket_level>0) {
							switch(expression_end[0]) {
								case '(':
									bracket_level++;
								break;
								case ')':
									bracket_level--;
								break;
							}
							expression_end++;
						}
						if(bracket_level>0) dbg->fatal( "tabfile_t::read", "mismatched brackets" );
					}
					break;
					case '%':
					case '/':
					case '*':
					case '+':
					case '-':
						expression_end = expression_pos;
					break;
				}
				expression_pos++;
			}
			if(expression_end==NULL) expression_end = expression_pos;

			// construct expression with brackets around 'a operator b'
			sprintf(buffer,"%.*s(%.*s%.*s)%s",	expression_start-processed+1, processed,
								expression_ptr-expression_start-1, expression_start+1,
								expression_end-expression_ptr, expression_ptr,
								expression_end);

			strcpy(processed, buffer);
			operator_count++;

			expression_ptr = strchr(processed, operators[operator_loop]);
			for(int i=0; i<operator_count; i++) {
				expression_ptr = strchr(expression_ptr+1, operators[operator_loop]);
			}
		}
	}
}



int tabfile_t::calculate_internal(char *expression, int parameter_value[10][256], int parameters, int combination[10])
{
	int answer = 0;

	char original[4096];
	strcpy(original, expression);

	char *token_ptr = strtok(expression,"<-+*/%");

	if(token_ptr[0]=='(') {
		token_ptr++;
	}

	while (token_ptr<expression+strlen(original)) {
		int value = 0;
		char operator_char = original[token_ptr-expression-1];

		if(token_ptr[0]=='$') {
			int parameter = atoi((char *)(token_ptr+1));
			if(parameter<=parameters) {
				value = parameter_value[parameter][combination[parameter]];
			}
		}
		else {
			if(token_ptr[0]=='(') {
				uint16 bracket_length;
				int bracket_level=1;
				for(bracket_length=1; bracket_length<strlen(original)-(token_ptr-expression) && bracket_level>0; bracket_length++) {
					switch(original[token_ptr-expression+bracket_length]) {
						case '(':
							bracket_level++;
						break;
						case ')':
							bracket_level--;
						break;
					}
				}
				char buffer[4096];
				sprintf(buffer, "%.*s", bracket_length, original+(token_ptr-expression)-1);
				value = calculate_internal(buffer+1, parameter_value, parameters, combination);
				token_ptr+=bracket_length;
			}
			else {
				value = atoi(token_ptr);
			}
		}
		switch(operator_char) {
			case '+':
				answer+= value;
			break;
			case '-':
				answer-= value;
			break;
			case '*':
				answer*= value;
			break;
			case '/':
				answer/= value;
			break;
			case '%':
				answer%= value;
			break;
			case '<':
			case '(':
				answer = value;
			break;
		}
		token_ptr += strcspn(token_ptr, "<-+*/%")+1;
	}
	return answer;
}



void tabfile_t::format_key(char *key)
{
	char *s = key + strlen(key);
	char *t;

	// trim right
	while(s > key && s[-1] == ' ') {
		*--s = '\0';
	}
	// make lowercase
	for(s = key; *s; s++) {
		*s = tolower(*s);
	}
	// skip spaces inside []
	for(s = t = key; *s; s++) {
		if(*s == '[') {
			*t++ = *s++;

			while(*s && *s != ']') {
				if(*s == ' ') {
					s++;
				}
				else {
					*t++ = *s++;
				}
			}
			s--;
		}
		else {
			*t++ = *s;
		}
	}
	*t = '\0';
}


void tabfile_t::format_value(char *value)
{
	size_t len = strlen(value);

	// trim right
	while(len && value[len - 1] == ' ') {
		value[--len] = '\0';
	}
	// trim left
	if(*value == ' ') {
		char *from;
		for(from = value; *from == ' '; from++) {}
		while(*value) {
			*value++ = *from++;
		}
	}
}
