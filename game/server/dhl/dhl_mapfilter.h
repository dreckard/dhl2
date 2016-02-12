//-----------------------------------------------------------------------------------------
//Copied from SourceWiki
//All credit goes to author of http://www.sourcewiki.org/wiki/index.php/Resetting_the_Map
//-----------------------------------------------------------------------------------------
#include "cbase.h"
#include "mapentities.h"
#include "utlvector.h"
 
#ifndef CMAPENTITYFILTER_H
#define CMAPENTITYFILTER_H
 
class CMapEntityFilter : public IMapEntityFilter
{
public:
	// constructor
	CMapEntityFilter();
	// deconstructor
	~CMapEntityFilter();
	
	// used to check if we should reset an entity or not
	virtual bool ShouldCreateEntity( const char *pClassname );
	// creates the next entity in our stored list.
	virtual CBaseEntity* CreateNextEntity( const char *pClassname );
	// add an entity to our list
	void AddKeep( const char*);
 
private:
	// our list of entities to keep
	CUtlVector< const char* >*keepList;
};
 
#endif 
