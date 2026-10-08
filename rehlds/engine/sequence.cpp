/*
*
*    This program is free software; you can redistribute it and/or modify it
*    under the terms of the GNU General Public License as published by the
*    Free Software Foundation; either version 2 of the License, or (at
*    your option) any later version.
*
*    This program is distributed in the hope that it will be useful, but
*    WITHOUT ANY WARRANTY; without even the implied warranty of
*    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
*    General Public License for more details.
*
*    You should have received a copy of the GNU General Public License
*    along with this program; if not, write to the Free Software Foundation,
*    Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*
*    In addition, as a special exception, the author gives permission to
*    link the code of this program with the Half-Life Game Engine ("HL
*    Engine") and Modified Game Libraries ("MODs") developed by Valve,
*    L.L.C ("Valve").  You must obey the GNU General Public License in all
*    respects for all of the code used other than the HL Engine and MODs
*    from Valve.  If you modify this file, you may extend this exception
*    to your version of the file, but you are not obligated to do so.  If
*    you do not wish to do so, delete this exception statement from your
*    version.
*
*/

#include "precompiled.h"

// Scripted sequences (sequences/*.seq), used by CS:CZ Deleted Scenes through pfnSequenceGet / pfnSequencePickSentence.
// Valve shipped this only in the client engine (parsed from CL_Init), so the dedicated server never had it.

sequenceCommandMapping_s g_sequenceCommandMappingTable[] =
{
	{ SEQUENCE_COMMAND_PAUSE,			"pause",		SEQUENCE_TYPE_COMMAND },
	{ SEQUENCE_COMMAND_TEXT,			"text",			SEQUENCE_TYPE_COMMAND },
	{ SEQUENCE_COMMAND_SOUND,			"sound",		SEQUENCE_TYPE_COMMAND },
	{ SEQUENCE_COMMAND_FIRETARGETS,		"firetargets",	SEQUENCE_TYPE_COMMAND },
	{ SEQUENCE_COMMAND_KILLTARGETS,		"killtargets",	SEQUENCE_TYPE_COMMAND },
	{ SEQUENCE_COMMAND_GOSUB,			"gosub",		SEQUENCE_TYPE_COMMAND },
	{ SEQUENCE_COMMAND_SENTENCE,		"sentence",		SEQUENCE_TYPE_COMMAND },
	{ SEQUENCE_COMMAND_REPEAT,			"repeat",		SEQUENCE_TYPE_COMMAND },
	{ SEQUENCE_COMMAND_SETDEFAULTS,		"setdefaults",	SEQUENCE_TYPE_COMMAND },
	{ SEQUENCE_COMMAND_MODIFIER,		"modifier",		SEQUENCE_TYPE_COMMAND },
	{ SEQUENCE_COMMAND_POSTMODIFIER,	"postmodifier",	SEQUENCE_TYPE_COMMAND },
	{ SEQUENCE_COMMAND_NOOP,			"noop",			SEQUENCE_TYPE_COMMAND },
	{ SEQUENCE_MODIFIER_EFFECT,			"effect",		SEQUENCE_TYPE_MODIFIER },
	{ SEQUENCE_MODIFIER_POSITION,		"position",		SEQUENCE_TYPE_MODIFIER },
	{ SEQUENCE_MODIFIER_COLOR,			"color",		SEQUENCE_TYPE_MODIFIER },
	{ SEQUENCE_MODIFIER_COLOR2,			"color2",		SEQUENCE_TYPE_MODIFIER },
	{ SEQUENCE_MODIFIER_FADEIN,			"fadein",		SEQUENCE_TYPE_MODIFIER },
	{ SEQUENCE_MODIFIER_FADEOUT,		"fadeout",		SEQUENCE_TYPE_MODIFIER },
	{ SEQUENCE_MODIFIER_HOLDTIME,		"holdtime",		SEQUENCE_TYPE_MODIFIER },
	{ SEQUENCE_MODIFIER_FXTIME,			"fxtime",		SEQUENCE_TYPE_MODIFIER },
	{ SEQUENCE_MODIFIER_SPEAKER,		"speaker",		SEQUENCE_TYPE_MODIFIER },
	{ SEQUENCE_MODIFIER_LISTENER,		"listener",		SEQUENCE_TYPE_MODIFIER },
	{ SEQUENCE_MODIFIER_TEXTCHANNEL,	"channel",		SEQUENCE_TYPE_MODIFIER },
};

char *g_scan;
char *g_lineScan;
int g_lineNum;
char g_sequenceParseFileName[256];
qboolean g_sequenceParseFileIsGlobal;

sequenceCommandLine_s g_fileScopeDefaults;
sequenceCommandLine_s g_blockScopeDefaults;

sequenceEntry_s *g_sequenceList;
sentenceGroupEntry_s *g_sentenceGroupList;
unsigned int g_nonGlobalSentences;

void Sequence_ResetDefaults(sequenceCommandLine_s *destination, sequenceCommandLine_s *source)
{
	if (!source)
	{
		destination->clientMessage.effect = 0;
		destination->clientMessage.r1 = 255;
		destination->clientMessage.g1 = 255;
		destination->clientMessage.b1 = 255;
		destination->clientMessage.a1 = 255;
		destination->clientMessage.r2 = 255;
		destination->clientMessage.g2 = 255;
		destination->clientMessage.b2 = 255;
		destination->clientMessage.a2 = 255;
		destination->clientMessage.x = 0.5f;
		destination->clientMessage.y = 0.5f;
		destination->clientMessage.fadein = 0.2f;
		destination->clientMessage.fadeout = 0.2f;
		destination->clientMessage.holdtime = 1.6f;
		destination->clientMessage.fxtime = 1.0f;
		destination->clientMessage.pName = NULL;
		destination->clientMessage.pMessage = NULL;
		destination->textChannel = 0;
		destination->delay = 0.0f;
		destination->repeatCount = 0;
		destination->nextCommandLine = NULL;
		destination->soundFileName = NULL;

#ifdef REHLDS_FIXES
		// fixed memory leak
		Mem_Free(destination->speakerName);
		Mem_Free(destination->listenerName);
#endif
		destination->speakerName = NULL;
		destination->listenerName = NULL;
		return;
	}

	destination->clientMessage = source->clientMessage;
	destination->clientMessage.pName = NULL;
	destination->clientMessage.pMessage = NULL;
	destination->textChannel = source->textChannel;
	destination->delay = source->delay;
	destination->repeatCount = source->repeatCount;
	destination->nextCommandLine = NULL;
	destination->soundFileName = NULL;

	Mem_Free(destination->speakerName);
	destination->speakerName = Mem_Strdup(source->speakerName);

	Mem_Free(destination->listenerName);
	destination->listenerName = Mem_Strdup(source->listenerName);
}

void Sequence_WriteDefaults(sequenceCommandLine_s *source, sequenceCommandLine_s *destination)
{
	if (!destination)
		Sys_Error("Attempt to bake defaults into a non-existant command.");

	if (!source)
		Sys_Error("Attempt to bake defaults from a non-existant command.");

	if (source->modifierBitField & SEQUENCE_MODIFIER_EFFECT_BIT)
	{
		destination->clientMessage.effect = source->clientMessage.effect;
	}

	if (source->modifierBitField & SEQUENCE_MODIFIER_COLOR_BIT)
	{
		destination->clientMessage.r1 = source->clientMessage.r1;
		destination->clientMessage.g1 = source->clientMessage.g1;
		destination->clientMessage.b1 = source->clientMessage.b1;
		destination->clientMessage.a1 = source->clientMessage.a1;
	}

	if (source->modifierBitField & SEQUENCE_MODIFIER_COLOR2_BIT)
	{
		destination->clientMessage.r2 = source->clientMessage.r2;
		destination->clientMessage.g2 = source->clientMessage.g2;
		destination->clientMessage.b2 = source->clientMessage.b2;
		destination->clientMessage.a2 = source->clientMessage.a2;
	}

	if (source->modifierBitField & SEQUENCE_MODIFIER_POSITION_BIT)
	{
		destination->clientMessage.x = source->clientMessage.x;
		destination->clientMessage.y = source->clientMessage.y;
	}

	if (source->modifierBitField & SEQUENCE_MODIFIER_FADEIN_BIT)
	{
		destination->clientMessage.fadein = source->clientMessage.fadein;
	}

	if (source->modifierBitField & SEQUENCE_MODIFIER_FADEOUT_BIT)
	{
		destination->clientMessage.fadeout = source->clientMessage.fadeout;
	}

	if (source->modifierBitField & SEQUENCE_MODIFIER_HOLDTIME_BIT)
	{
		destination->clientMessage.holdtime = source->clientMessage.holdtime;
	}

	if (source->modifierBitField & SEQUENCE_MODIFIER_FXTIME_BIT)
	{
		destination->clientMessage.fxtime = source->clientMessage.fxtime;
	}

	if (source->modifierBitField & SEQUENCE_MODIFIER_TEXTCHANNEL_BIT)
	{
		destination->textChannel = source->textChannel;
	}

	if (source->modifierBitField & SEQUENCE_MODIFIER_SPEAKER_BIT)
	{
		Mem_Free(destination->speakerName);
		destination->speakerName = Mem_Strdup(source->speakerName);
	}

	if (source->modifierBitField & SEQUENCE_MODIFIER_LISTENER_BIT)
	{
		Mem_Free(destination->listenerName);
		destination->listenerName = Mem_Strdup(source->listenerName);
	}
}

void Sequence_BakeDefaults(sequenceCommandLine_s *destination, sequenceCommandLine_s *source)
{
	if (!destination)
		Sys_Error("Attempt to bake defaults into a non-existant command.");

	if (!source)
		Sys_Error("Attempt to bake defaults from a non-existant command.");

	const char *pName = destination->clientMessage.pName;
	const char *pMessage = destination->clientMessage.pMessage;
	destination->clientMessage = source->clientMessage;
	destination->clientMessage.pName = pName;
	destination->clientMessage.pMessage = pMessage;

	destination->textChannel = source->textChannel;

	Mem_Free(destination->speakerName);
	destination->speakerName = Mem_Strdup(source->speakerName);

	Mem_Free(destination->listenerName);
	destination->listenerName = Mem_Strdup(source->listenerName);
}

qboolean Sequence_IsNameValueChar(char ch)
{
	if (isalnum((unsigned char)ch))
		return TRUE;

	switch (ch)
	{
	case '-':
	case '.':
	case '/':
	case '_':
	case '\\':
		return TRUE;
	}

	return FALSE;
}

qboolean Sequence_IsSymbol(char ch)
{
	switch (ch)
	{
	case '"':
	case '#':
	case '$':
	case '%':
	case ',':
	case '=':
	case '@':
	case '{':
	case '}':
		return TRUE;
	}

	return FALSE;
}

// Returns TRUE if a line break was skipped
qboolean Sequence_SkipWhitespace()
{
	qboolean newLine = FALSE;

	while (isspace((unsigned char)*g_scan))
	{
		if (*g_scan == '\n')
		{
			g_lineScan = g_scan + 1;
			g_lineNum++;
			newLine = TRUE;
		}

		g_scan++;
	}

	return newLine;
}

// Returns (and consumes) the next non-whitespace character
char Sequence_GetSymbol()
{
	Sequence_SkipWhitespace();

	char ch = *g_scan;
	if (ch)
		g_scan++;

	return ch;
}

void Sequence_GetNameValueString(char *token, int tokenLen)
{
	if (tokenLen < 1)
		return;

	Sequence_SkipWhitespace();

	char ch = *g_scan;
	if (!Sequence_IsNameValueChar(ch))
	{
		if (ch == '#' || ch == '$')
			Sys_Error("Parsing error on line %d of %s.seq: cannot have more than one '%c' per line; '%c' must be at the beginning of the line ONLY\n", g_lineNum, g_sequenceParseFileName, ch, ch);

		Sys_Error("Parsing error on line %d of %s.seq: expected name/value, found illegal character '%c'\n", g_lineNum, g_sequenceParseFileName, ch);
	}

	char *p = token;
	while (Sequence_IsNameValueChar(*g_scan) && p - token < tokenLen - 1)
		*p++ = *g_scan++;

	*p = '\0';
}

void Sequence_GetToken(char *token, int tokenLen)
{
	Sequence_SkipWhitespace();

	char ch = *g_scan;
	if (Sequence_IsNameValueChar(ch))
	{
		Sequence_GetNameValueString(token, tokenLen);
		return;
	}

	if (!Sequence_IsSymbol(ch))
		Sys_Error("Parsing error on line %d of %s.seq: expected token, found '%c' instead\n", g_lineNum, g_sequenceParseFileName, ch);

	Q_snprintf(token, tokenLen, "%c", ch);
	g_scan++;
}

void Sequence_GetLine(char *line, int lineMaxLen)
{
	Sequence_SkipWhitespace();

	char *lineEnd = Q_strchr(g_scan, '\n');
	if (!lineEnd)
		Sys_Error("Syntax Error on line %d of %s.seq: expected sentence definition or '}', found End-Of-File!\n", g_lineNum, g_sequenceParseFileName);

	int lineLen = (int)(lineEnd - g_scan);
	if (lineLen >= lineMaxLen)
		Sys_Error("Syntax Error on line %d of %s.seq: line was too long (was %d chars; max is %d chars)\n", g_lineNum, g_sequenceParseFileName, lineLen, lineMaxLen - 1);

	Q_memcpy(line, g_scan, lineLen);
	line[lineLen] = '\0';
	g_scan = lineEnd;
}

// Removes // and /* */ comments in place, keeping the line breaks so line numbers stay right
void Sequence_StripComments(char *buffer, int *pBufSize)
{
	char *eof = buffer + *pBufSize;
	char *read = buffer;
	char *write = buffer;

	while (read < eof && *read)
	{
		if (read[0] == '/' && read[1] == '/')
		{
			while (*read && *read != '\n' && *read != '\r')
				read++;
		}
		else if (read[0] == '/' && read[1] == '*')
		{
			read += 2;
			while (*read && !(read[0] == '*' && read[1] == '/'))
			{
				if (*read == '\n' || *read == '\r')
					*write++ = *read;

				read++;
			}

			if (*read)
				read += 2;
		}
		else
		{
			*write++ = *read++;
		}
	}

	*write = '\0';
}

int Sequence_ReadInt()
{
	char intString[256];
	Sequence_GetNameValueString(intString, sizeof(intString));
	return strtol(intString, NULL, 10);
}

float Sequence_ReadFloat()
{
	char floatString[256];
	Sequence_GetNameValueString(floatString, sizeof(floatString));
	return (float)strtod(floatString, NULL);
}

void Sequence_ReadQuotedString(char *string, int stringLen)
{
	if (stringLen < 1)
		return;

	char ch = Sequence_GetSymbol();
	if (ch != '"')
		Sys_Error("Parsing error on or before line %d of %s.seq: expected quote (\"), found '%c' instead\n", g_lineNum, g_sequenceParseFileName, ch);

	char *p = string;
	while (*g_scan && *g_scan != '"' && p - string < stringLen - 1)
	{
		if (*g_scan == '\n')
			g_lineNum++;

		*p++ = *g_scan++;
	}

	*p = '\0';

	// skip the closing quote
	if (*g_scan)
		g_scan++;
}

sequenceCommandEnum_e Sequence_GetCommandEnumForName(const char *commandName)
{
	for (int i = 0; i < ARRAYSIZE(g_sequenceCommandMappingTable); i++)
	{
		if (!Q_stricmp(commandName, g_sequenceCommandMappingTable[i].commandName))
			return g_sequenceCommandMappingTable[i].commandEnum;
	}

	return SEQUENCE_COMMAND_ERROR;
}

qboolean Sequence_IsCommandAModifier(sequenceCommandEnum_e commandEnum)
{
	for (int i = 0; i < ARRAYSIZE(g_sequenceCommandMappingTable); i++)
	{
		if (g_sequenceCommandMappingTable[i].commandEnum == commandEnum)
			return g_sequenceCommandMappingTable[i].commandType == SEQUENCE_TYPE_MODIFIER;
	}

	Sys_Error("Internal error caused by line %d of %s.seq: unknown command enum = %d\n", g_lineNum, g_sequenceParseFileName, commandEnum);
	return FALSE;
}

void Sequence_ReadCommandData(sequenceCommandEnum_e commandEnum, sequenceCommandLine_s *defaults)
{
	char tempString[1024];

	switch (commandEnum)
	{
	case SEQUENCE_COMMAND_PAUSE:
		defaults->delay = Sequence_ReadFloat();
		break;

	case SEQUENCE_COMMAND_FIRETARGETS:
		Sequence_ReadQuotedString(tempString, sizeof(tempString));
		defaults->fireTargetNames = Mem_Strdup(tempString);
		break;

	case SEQUENCE_COMMAND_KILLTARGETS:
		Sequence_ReadQuotedString(tempString, sizeof(tempString));
		defaults->killTargetNames = Mem_Strdup(tempString);
		break;

	case SEQUENCE_COMMAND_TEXT:
		Sequence_ReadQuotedString(tempString, sizeof(tempString));
		defaults->clientMessage.pMessage = Mem_Strdup(tempString);
		break;

	case SEQUENCE_COMMAND_SOUND:
		Sequence_GetNameValueString(tempString, sizeof(tempString));
		defaults->soundFileName = Mem_Strdup(tempString);
		break;

	case SEQUENCE_COMMAND_GOSUB:
		// the name of the entry to gosub is kept in pName until the gosubs get expanded
		Sequence_GetNameValueString(tempString, sizeof(tempString));
		defaults->clientMessage.pName = Mem_Strdup(tempString);
		break;

	case SEQUENCE_COMMAND_SENTENCE:
		Sequence_GetNameValueString(tempString, sizeof(tempString));
		defaults->sentenceName = Mem_Strdup(tempString);
		break;

	case SEQUENCE_COMMAND_REPEAT:
		defaults->repeatCount = Sequence_ReadInt();
		break;

	case SEQUENCE_MODIFIER_EFFECT:
		defaults->clientMessage.effect = Sequence_ReadInt();
		defaults->modifierBitField |= SEQUENCE_MODIFIER_EFFECT_BIT;
		break;

	case SEQUENCE_MODIFIER_POSITION:
		defaults->clientMessage.x = Sequence_ReadFloat();
		defaults->clientMessage.y = Sequence_ReadFloat();
		defaults->modifierBitField |= SEQUENCE_MODIFIER_POSITION_BIT;
		break;

	case SEQUENCE_MODIFIER_COLOR:
		defaults->clientMessage.r1 = Sequence_ReadInt();
		defaults->clientMessage.g1 = Sequence_ReadInt();
		defaults->clientMessage.b1 = Sequence_ReadInt();
		defaults->clientMessage.a1 = 255;
		defaults->modifierBitField |= SEQUENCE_MODIFIER_COLOR_BIT;
		break;

	case SEQUENCE_MODIFIER_COLOR2:
		defaults->clientMessage.r2 = Sequence_ReadInt();
		defaults->clientMessage.g2 = Sequence_ReadInt();
		defaults->clientMessage.b2 = Sequence_ReadInt();
		defaults->clientMessage.a2 = 255;
		defaults->modifierBitField |= SEQUENCE_MODIFIER_COLOR2_BIT;
		break;

	case SEQUENCE_MODIFIER_FADEIN:
		defaults->clientMessage.fadein = Sequence_ReadFloat();
		defaults->modifierBitField |= SEQUENCE_MODIFIER_FADEIN_BIT;
		break;

	case SEQUENCE_MODIFIER_FADEOUT:
		defaults->clientMessage.fadeout = Sequence_ReadFloat();
		defaults->modifierBitField |= SEQUENCE_MODIFIER_FADEOUT_BIT;
		break;

	case SEQUENCE_MODIFIER_HOLDTIME:
		defaults->clientMessage.holdtime = Sequence_ReadFloat();
		defaults->modifierBitField |= SEQUENCE_MODIFIER_HOLDTIME_BIT;
		break;

	case SEQUENCE_MODIFIER_FXTIME:
		defaults->clientMessage.fxtime = Sequence_ReadFloat();
		defaults->modifierBitField |= SEQUENCE_MODIFIER_FXTIME_BIT;
		break;

	case SEQUENCE_MODIFIER_SPEAKER:
		Sequence_GetNameValueString(tempString, sizeof(tempString));
		defaults->speakerName = Mem_Strdup(tempString);
		defaults->modifierBitField |= SEQUENCE_MODIFIER_SPEAKER_BIT;
		break;

	case SEQUENCE_MODIFIER_LISTENER:
		Sequence_GetNameValueString(tempString, sizeof(tempString));
		defaults->listenerName = Mem_Strdup(tempString);
		defaults->modifierBitField |= SEQUENCE_MODIFIER_LISTENER_BIT;
		break;

	case SEQUENCE_MODIFIER_TEXTCHANNEL:
		defaults->textChannel = Sequence_ReadInt();
		defaults->modifierBitField |= SEQUENCE_MODIFIER_TEXTCHANNEL_BIT;
		break;

	default:
		Sys_Error("Internal error caused by line %d of %s.seq: unknown command enum = %d\n", g_lineNum, g_sequenceParseFileName, commandEnum);
	}
}

// Parses "name = value(s)" of a $modifier, returns the symbol that follows it (',' or the start of the next line)
char Sequence_ParseModifier(sequenceCommandLine_s *defaults)
{
	char modifierName[256];
	Sequence_GetNameValueString(modifierName, sizeof(modifierName));

	sequenceCommandEnum_e modifierEnum = Sequence_GetCommandEnumForName(modifierName);
	if (modifierEnum == SEQUENCE_COMMAND_ERROR)
		Sys_Error("Parsing error on line %d of %s.seq: unknown modifier \"%s\"\n", g_lineNum, g_sequenceParseFileName, modifierName);

	if (!Sequence_IsCommandAModifier(modifierEnum))
		Sys_Error("Parsing error on line %d of %s.seq: \"%s\" is a #command, not a $modifier\n", g_lineNum, g_sequenceParseFileName, modifierName);

	char ch = Sequence_GetSymbol();
	if (ch != '=')
		Sys_Error("Parsing error on or after line %d of %s.seq: after modifier \"%s\", expected '=', found '%c'\n", g_lineNum, g_sequenceParseFileName, modifierName, ch);

	Sequence_ReadCommandData(modifierEnum, defaults);

	qboolean newLine = Sequence_SkipWhitespace();
	ch = *g_scan;
	if (ch != ',' && !newLine)
		Sys_Error("Parsing error on line %d of %s.seq: after value(s) for modifier \"%s\", expected ',' or End-Of-Line; found '%c'\n", g_lineNum, g_sequenceParseFileName, modifierName, ch);

	if (ch)
		g_scan++;

	return ch;
}

void Sequence_AddCommandLineToEntry(sequenceCommandLine_s *commandLine, sequenceEntry_s *entry)
{
	commandLine->nextCommandLine = NULL;

	if (!entry->firstCommand)
	{
		entry->firstCommand = commandLine;
		return;
	}

	sequenceCommandLine_s *last = entry->firstCommand;
	while (last->nextCommandLine)
		last = last->nextCommandLine;

	last->nextCommandLine = commandLine;
}

sequenceCommandLine_s *Sequence_CreateCommand(sequenceEntry_s *entry, sequenceCommandLine_s *defaults)
{
	sequenceCommandLine_s *newCommand = (sequenceCommandLine_s *)Mem_ZeroMalloc(sizeof(sequenceCommandLine_s));
	Sequence_ResetDefaults(newCommand, defaults);
	Sequence_AddCommandLineToEntry(newCommand, entry);
	return newCommand;
}

// Parses "name = value(s)" of a #command, returns the symbol that follows it
char Sequence_ParseCommand(sequenceCommandLine_s *newCommandLine)
{
	char commandName[256];
	Sequence_GetNameValueString(commandName, sizeof(commandName));

	sequenceCommandEnum_e commandEnum = Sequence_GetCommandEnumForName(commandName);
	if (commandEnum == SEQUENCE_COMMAND_ERROR)
		Sys_Error("Parsing error on line %d of %s.seq: unknown command \"%s\"\n", g_lineNum, g_sequenceParseFileName, commandName);

	// a modifier on a command line only applies to that command
	if (Sequence_IsCommandAModifier(commandEnum))
	{
		sequenceCommandLine_s *modifierCommand = (sequenceCommandLine_s *)Mem_ZeroMalloc(sizeof(sequenceCommandLine_s));
		modifierCommand->commandType = SEQUENCE_COMMAND_POSTMODIFIER;

		sequenceCommandLine_s *last = newCommandLine;
		while (last->nextCommandLine)
			last = last->nextCommandLine;

		last->nextCommandLine = modifierCommand;
		newCommandLine = modifierCommand;
	}

	char ch = Sequence_GetSymbol();
	if (ch != '=')
		Sys_Error("Parsing error on or before line %d of %s.seq: after command \"%s\", expected '=', found '%c'\n", g_lineNum, g_sequenceParseFileName, commandName, ch);

	Sequence_ReadCommandData(commandEnum, newCommandLine);

	return Sequence_GetSymbol();
}

// $modifier line: every modifier gets its own command
char Sequence_ParseModifierLine(sequenceEntry_s *entry)
{
	char endToken;

	do
	{
		sequenceCommandLine_s *newCommand = (sequenceCommandLine_s *)Mem_ZeroMalloc(sizeof(sequenceCommandLine_s));
		newCommand->commandType = SEQUENCE_COMMAND_MODIFIER;
		Sequence_AddCommandLineToEntry(newCommand, entry);

		endToken = Sequence_ParseModifier(newCommand);
	}
	while (endToken == ',');

	return endToken;
}

// #command line
char Sequence_ParseCommandLine(sequenceEntry_s *entry)
{
	sequenceCommandLine_s *newCommand = Sequence_CreateCommand(entry, &g_blockScopeDefaults);

	char endToken;
	do
	{
		endToken = Sequence_ParseCommand(newCommand);
	}
	while (endToken == ',');

	return endToken;
}

// @macro line: gosub to another entry
char Sequence_ParseMacro(sequenceEntry_s *entry)
{
	sequenceCommandLine_s *newCommand = Sequence_CreateCommand(entry, &g_blockScopeDefaults);
	Sequence_ReadCommandData(SEQUENCE_COMMAND_GOSUB, newCommand);

	char endToken = Sequence_GetSymbol();
	while (endToken == ',')
		endToken = Sequence_ParseCommand(newCommand);

	return endToken;
}

char Sequence_ParseLine(char startToken, sequenceEntry_s *entry)
{
	switch (startToken)
	{
	case '$':
		return Sequence_ParseModifierLine(entry);
	case '#':
		return Sequence_ParseCommandLine(entry);
	case '@':
		return Sequence_ParseMacro(entry);
	}

	Sys_Error("Parsing error on line %d of %s.seq: line must begin with either '#' (command) or '$' (modifier); found '%c'\n", g_lineNum, g_sequenceParseFileName, startToken);
	return '\0';
}

qboolean Sequence_IsEntrySafe(sequenceEntry_s *entry)
{
	float duration = 0.0f;

	for (sequenceCommandLine_s *cmd = entry->firstCommand; cmd; cmd = cmd->nextCommandLine)
	{
		duration += cmd->delay;

		// infinite repeat without any delay
		if (cmd->repeatCount < 0 && duration <= 0.0f)
			return FALSE;
	}

	return TRUE;
}

void Sequence_CreateDefaultsCommand(sequenceEntry_s *entry)
{
	sequenceCommandLine_s *setDefaults = Sequence_CreateCommand(entry, &g_fileScopeDefaults);
	setDefaults->commandType = SEQUENCE_COMMAND_SETDEFAULTS;
	setDefaults->modifierBitField = 0xFF;
}

// %entry block, returns the symbol that follows it
char Sequence_ParseEntry()
{
	char token[256];
	Sequence_GetNameValueString(token, sizeof(token));

	char symbol = Sequence_GetSymbol();
	if (symbol != '{')
		Sys_Error("Parsing error on line %d of %s.seq: expected '{' to start a\n new entry block; found '%c' instead!", g_lineNum, g_sequenceParseFileName, symbol);

	Sequence_ResetDefaults(&g_blockScopeDefaults, &g_fileScopeDefaults);

	sequenceEntry_s *entry = (sequenceEntry_s *)Mem_Malloc(sizeof(sequenceEntry_s));
	entry->entryName = Mem_Strdup(token);
	entry->fileName = Mem_Strdup(g_sequenceParseFileName);
	entry->isGlobal = g_sequenceParseFileIsGlobal;
	entry->firstCommand = NULL;
	Sequence_CreateDefaultsCommand(entry);

	symbol = Sequence_GetSymbol();
	while (symbol != '}')
		symbol = Sequence_ParseLine(symbol, entry);

	if (!Sequence_IsEntrySafe(entry))
		Sys_Error("Logic error in file %s.seq before line %d: execution of entry \"%%%s\" would cause an infinite loop!", g_sequenceParseFileName, g_lineNum, entry->entryName);

	entry->nextEntry = g_sequenceList;
	g_sequenceList = entry;

	return Sequence_GetSymbol();
}

sentenceGroupEntry_s *Sequence_FindSentenceGroup(const char *groupName)
{
	for (sentenceGroupEntry_s *groupEntry = g_sentenceGroupList; groupEntry; groupEntry = groupEntry->nextEntry)
	{
		if (!Q_stricmp(groupEntry->groupName, groupName))
			return groupEntry;
	}

	return NULL;
}

sentenceGroupEntry_s *Sequence_AddSentenceGroup(const char *groupName)
{
	sentenceGroupEntry_s *groupEntry = (sentenceGroupEntry_s *)Mem_Malloc(sizeof(sentenceGroupEntry_s));
	groupEntry->groupName = Mem_Strdup(groupName);
	groupEntry->numSentences = 0;
	groupEntry->firstSentence = NULL;
	groupEntry->nextEntry = NULL;

	if (!g_sentenceGroupList)
	{
		g_sentenceGroupList = groupEntry;
		return groupEntry;
	}

	sentenceGroupEntry_s *last = g_sentenceGroupList;
	while (last->nextEntry)
		last = last->nextEntry;

	last->nextEntry = groupEntry;
	return groupEntry;
}

void Sequence_AddSentenceToGroup(const char *groupName, const char *data)
{
	sentenceGroupEntry_s *groupEntry = Sequence_FindSentenceGroup(groupName);
	if (!groupEntry)
		groupEntry = Sequence_AddSentenceGroup(groupName);

	sentenceEntry_s *sentenceEntry = (sentenceEntry_s *)Mem_Malloc(sizeof(sentenceEntry_s));
	sentenceEntry->data = Mem_Strdup(data);
	sentenceEntry->nextEntry = NULL;
	sentenceEntry->isGlobal = g_sequenceParseFileIsGlobal;

	// the client resolves "#index" sounds against its own copy of the same .seq files, so the numbering must match
	sentenceEntry->index = g_nonGlobalSentences++;

	groupEntry->numSentences++;

	if (!groupEntry->firstSentence)
	{
		groupEntry->firstSentence = sentenceEntry;
		return;
	}

	sentenceEntry_s *last = groupEntry->firstSentence;
	while (last->nextEntry)
		last = last->nextEntry;

	last->nextEntry = sentenceEntry;
}

// "GROUPNAME0 sentence data" line, returns TRUE at the closing '}'
qboolean Sequence_ParseSentenceLine()
{
	char groupName[64];
	char data[1024];

	Sequence_GetToken(groupName, sizeof(groupName));
	if (groupName[0] == '}')
		return TRUE;

	// strip the trailing index from the group name (e.g. "CT_ALERT0" -> "CT_ALERT")
	char *groupEnd = &groupName[Q_strlen(groupName) - 1];
	while (groupEnd >= groupName && !isalpha((unsigned char)*groupEnd) && *groupEnd != '_')
		groupEnd--;

	if (groupEnd >= groupName)
		groupEnd[1] = '\0';

	Sequence_GetLine(data, sizeof(data));

	int dataLen = Q_strlen(data);
	if (dataLen > 0 && (data[dataLen - 1] == '\n' || data[dataLen - 1] == '\r'))
		data[dataLen - 1] = '\0';

	Sequence_AddSentenceToGroup(groupName, data);
	return FALSE;
}

char Sequence_ParseSentenceBlock()
{
	char symbol = Sequence_GetSymbol();
	if (symbol != '{')
		Sys_Error("Parsing error on line %d of %s.seq: expected '{' to start a\n new sentence block; found '%c' instead!", g_lineNum, g_sequenceParseFileName, symbol);

	while (!Sequence_ParseSentenceLine())
		;

	return Sequence_GetSymbol();
}

// !data block, returns the symbol that follows it
char Sequence_ParseGlobalDataBlock()
{
	char token[256];
	Sequence_GetNameValueString(token, sizeof(token));

	if (Q_stricmp(token, "Sentences"))
		Sys_Error("Syntax error in file %s.seq on line %d: found global data block symbol '!' with unknown data type \"%s\"", g_sequenceParseFileName, g_lineNum, token);

	return Sequence_ParseSentenceBlock();
}

sequenceEntry_s *Sequence_GetEntryForName(const char *entryName)
{
	for (sequenceEntry_s *entry = g_sequenceList; entry; entry = entry->nextEntry)
	{
		if (!Q_stricmp(entryName, entry->entryName))
			return entry;
	}

	return NULL;
}

sequenceCommandLine_s *Sequence_CopyCommand(sequenceCommandLine_s *commandOrig)
{
	sequenceCommandLine_s *command = (sequenceCommandLine_s *)Mem_ZeroMalloc(sizeof(sequenceCommandLine_s));

	command->commandType = commandOrig->commandType;
	command->modifierBitField = commandOrig->modifierBitField;
	command->repeatCount = commandOrig->repeatCount;
	command->delay = commandOrig->delay;
	command->textChannel = commandOrig->textChannel;
	command->fireTargetNames = Mem_Strdup(commandOrig->fireTargetNames);
	command->killTargetNames = Mem_Strdup(commandOrig->killTargetNames);
	command->listenerName = Mem_Strdup(commandOrig->listenerName);
	command->sentenceName = Mem_Strdup(commandOrig->sentenceName);
	command->soundFileName = Mem_Strdup(commandOrig->soundFileName);
	command->speakerName = Mem_Strdup(commandOrig->speakerName);
	command->clientMessage = commandOrig->clientMessage;
	command->clientMessage.pMessage = Mem_Strdup(commandOrig->clientMessage.pMessage);
	command->clientMessage.pName = Mem_Strdup(commandOrig->clientMessage.pName);
	command->nextCommandLine = NULL;

	return command;
}

sequenceCommandLine_s *Sequence_CopyCommandList(sequenceCommandLine_s *list)
{
	sequenceCommandLine_s *head = NULL;
	sequenceCommandLine_s *tail = NULL;

	for (; list; list = list->nextCommandLine)
	{
		// the gosub'd entry's defaults don't apply to the caller
		if (list->commandType == SEQUENCE_COMMAND_SETDEFAULTS)
			continue;

		sequenceCommandLine_s *command = Sequence_CopyCommand(list);
		if (tail)
			tail->nextCommandLine = command;
		else
			head = command;

		tail = command;
	}

	return head;
}

// Inlines one level of gosubs, returns TRUE if the entry had none left
qboolean Sequence_ExpandGosubsForEntry(sequenceEntry_s *entry)
{
	qboolean foundGosub = FALSE;

	for (sequenceCommandLine_s *cmd = entry->firstCommand; cmd; cmd = cmd->nextCommandLine)
	{
		const char *gosubName = cmd->clientMessage.pName;
		if (!gosubName)
			continue;

		if (!Q_stricmp(gosubName, entry->entryName))
			Sys_Error("Error in %s.seq: entry \"%s\" gosubs itself!\n", entry->fileName, entry->entryName);

		sequenceEntry_s *gosubEntry = Sequence_GetEntryForName(gosubName);
		if (!gosubEntry)
			Sys_Error("Error in %s.seq: Gosub in entry \"%s\" specified unknown entry \"%s\"\n", entry->fileName, entry->entryName, gosubName);

		sequenceCommandLine_s *copyList = Sequence_CopyCommandList(gosubEntry->firstCommand);

		Mem_Free((void *)cmd->clientMessage.pName);
		cmd->clientMessage.pName = NULL;
		foundGosub = TRUE;

		if (copyList)
		{
			sequenceCommandLine_s *copyTail = copyList;
			while (copyTail->nextCommandLine)
				copyTail = copyTail->nextCommandLine;

			// splice the copy in after the gosub command and continue after it
			copyTail->nextCommandLine = cmd->nextCommandLine;
			cmd->nextCommandLine = copyList;
			cmd = copyTail;
		}
	}

	return !foundGosub;
}

void Sequence_ExpandAllGosubs()
{
	qboolean isComplete;

	do
	{
		isComplete = TRUE;

		for (sequenceEntry_s *entry = g_sequenceList; entry; entry = entry->nextEntry)
		{
			if (!Sequence_ExpandGosubsForEntry(entry))
				isComplete = FALSE;
		}
	}
	while (!isComplete);
}

// Applies the modifiers to the commands, so the game dll can just walk the list
void Sequence_FlattenEntry(sequenceEntry_s *entry)
{
	sequenceCommandLine_s *lastCommand = NULL;

	for (sequenceCommandLine_s *cmd = entry->firstCommand; cmd; cmd = cmd->nextCommandLine)
	{
		switch (cmd->commandType)
		{
		case SEQUENCE_COMMAND_SETDEFAULTS:
			Sequence_WriteDefaults(cmd, &g_blockScopeDefaults);
			cmd->commandType = SEQUENCE_COMMAND_NOOP;
			break;

		case SEQUENCE_COMMAND_MODIFIER:
			Sequence_WriteDefaults(cmd, &g_blockScopeDefaults);
			break;

		case SEQUENCE_COMMAND_POSTMODIFIER:
			Sequence_WriteDefaults(cmd, lastCommand);
			break;

		default:
			Sequence_BakeDefaults(cmd, &g_blockScopeDefaults);
			lastCommand = cmd;
			break;
		}
	}
}

void Sequence_FlattenAllEntries()
{
	for (sequenceEntry_s *entry = g_sequenceList; entry; entry = entry->nextEntry)
		Sequence_FlattenEntry(entry);
}

void Sequence_ParseBuffer(byte *buffer, int bufferSize)
{
	g_lineNum = 1;
	g_scan = (char *)buffer;
	g_lineScan = (char *)buffer;

	Sequence_StripComments((char *)buffer, &bufferSize);
	Sequence_ResetDefaults(&g_fileScopeDefaults, NULL);

	char symbol = Sequence_GetSymbol();
	while (symbol)
	{
		switch (symbol)
		{
		case '$':
			do
			{
				symbol = Sequence_ParseModifier(&g_fileScopeDefaults);
			}
			while (symbol == ',');
			break;

		case '%':
			symbol = Sequence_ParseEntry();
			break;

		case '!':
			symbol = Sequence_ParseGlobalDataBlock();
			break;

		default:
			Sys_Error("Parsing error on line %d of %s.seq: At file scope, lines must begin with '$' (modifier) or '%%' (entry block) or '!' (sentence / global data block); found '%c'\n", g_lineNum, g_sequenceParseFileName, symbol);
		}
	}

	Sequence_ExpandAllGosubs();
	Sequence_FlattenAllEntries();
}

void Sequence_ParseFile(const char *fileName, qboolean isGlobal)
{
	char filePathName[256];
	int bufSize;

	Q_strlcpy(g_sequenceParseFileName, fileName, sizeof(g_sequenceParseFileName));
	g_sequenceParseFileIsGlobal = isGlobal;

	Q_snprintf(filePathName, sizeof(filePathName), "sequences/%s.seq", fileName);

	byte *buffer = COM_LoadTempFile(filePathName, &bufSize);
	if (!buffer)
	{
		Con_DPrintf("WARNING: failed to locate sequence file %s\n", fileName);
		return;
	}

	Sequence_ParseBuffer(buffer, bufSize);
}

void Sequence_Init()
{
	Sequence_ParseFile("global", TRUE);
}

sequenceEntry_s *EXT_FUNC SequenceGet(const char *fileName, const char *entryName)
{
	// fileName is ignored, entry names are global
	return Sequence_GetEntryForName(entryName);
}

sentenceEntry_s *EXT_FUNC SequencePickSentence(const char *groupName, int pickMethod, int *picked)
{
	// pickMethod is ignored, the pick is always random
	sentenceEntry_s *sentenceEntry = NULL;
	int pick = 0;

	sentenceGroupEntry_s *groupEntry = Sequence_FindSentenceGroup(groupName);
	if (groupEntry)
	{
		pick = RandomLong(0, groupEntry->numSentences - 1);

		sentenceEntry = groupEntry->firstSentence;
		for (int i = 0; i < pick; i++)
			sentenceEntry = sentenceEntry->nextEntry;
	}

	if (picked)
		*picked = pick;

	return sentenceEntry;
}

void Sequence_FreeCommand(sequenceCommandLine_s *kill)
{
	Mem_Free(kill->fireTargetNames);
	Mem_Free(kill->speakerName);
	Mem_Free(kill->listenerName);
	Mem_Free(kill->soundFileName);
	Mem_Free(kill->sentenceName);
	Mem_Free((void *)kill->clientMessage.pName);
	Mem_Free((void *)kill->clientMessage.pMessage);

#ifdef REHLDS_FIXES
	// fixed memory leak
	Mem_Free(kill->killTargetNames);
	Mem_Free(kill);
#endif
}

void Sequence_FreeEntry(sequenceEntry_s *kill)
{
	Mem_Free(kill->entryName);
	Mem_Free(kill->fileName);

	while (kill->firstCommand)
	{
		sequenceCommandLine_s *cmd = kill->firstCommand;
		kill->firstCommand = cmd->nextCommandLine;
		Sequence_FreeCommand(cmd);
	}

	Mem_Free(kill);
}

void Sequence_FreeSentence(sentenceEntry_s *sentenceEntry)
{
	Mem_Free(sentenceEntry->data);
	Mem_Free(sentenceEntry);
}

void Sequence_FreeSentenceGroup(sentenceGroupEntry_s *groupEntry)
{
	Mem_Free(groupEntry->groupName);
	Mem_Free(groupEntry);
}

void Sequence_FreeSentenceGroupEntries(sentenceGroupEntry_s *groupEntry, qboolean purgeGlobals)
{
	sentenceEntry_s **link = &groupEntry->firstSentence;

	while (*link)
	{
		sentenceEntry_s *sentenceEntry = *link;
		if (sentenceEntry->isGlobal && !purgeGlobals)
		{
			link = &sentenceEntry->nextEntry;
			continue;
		}

		*link = sentenceEntry->nextEntry;
		groupEntry->numSentences--;
		g_nonGlobalSentences--;
		Sequence_FreeSentence(sentenceEntry);
	}
}

void Sequence_PurgeEntries(qboolean purgeGlobals)
{
	sequenceEntry_s **entryLink = &g_sequenceList;

	while (*entryLink)
	{
		sequenceEntry_s *entry = *entryLink;
		if (entry->isGlobal && !purgeGlobals)
		{
			entryLink = &entry->nextEntry;
			continue;
		}

		*entryLink = entry->nextEntry;
		Sequence_FreeEntry(entry);
	}

	sentenceGroupEntry_s **groupLink = &g_sentenceGroupList;

	while (*groupLink)
	{
		sentenceGroupEntry_s *groupEntry = *groupLink;
		Sequence_FreeSentenceGroupEntries(groupEntry, purgeGlobals);

		if (groupEntry->numSentences)
		{
			groupLink = &groupEntry->nextEntry;
			continue;
		}

		*groupLink = groupEntry->nextEntry;
		Sequence_FreeSentenceGroup(groupEntry);
	}
}

void Sequence_OnLevelLoad(const char *mapName)
{
	Sequence_PurgeEntries(FALSE);
	Sequence_ParseFile(mapName, FALSE);
}
