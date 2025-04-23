/***
 *
 *	Copyright (c) 2009, Valve LLC. All rights reserved.
 *	
 *	This product contains software technology licensed from Id 
 *	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc. 
 *	All Rights Reserved.
 *
 *   Use, distribution, and modification of this source code and/or resulting
 *   object code is restricted to non-commercial enhancements to products from
 *   Valve LLC.  All other use, distribution, or modification is prohibited
 *   without written permission from Valve LLC.
 *
 ****/

#ifndef ENUMS_H
#define ENUMS_H

#define MAX_EXTRA_GAMES 8

// Used as array indexer
typedef enum netsrc_s
{
	NS_CLIENT = 0,
	NS_SERVER,
	NS_MULTICAST,	// xxxMO
	NS_EXTRA,
	NS_MAX = NS_EXTRA + MAX_EXTRA_GAMES
} netsrc_t;

#endif
