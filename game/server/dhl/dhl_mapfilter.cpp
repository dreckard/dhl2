//-----------------------------------------------------------------------------------------
//Copied from SourceWiki
//All credit goes to author of http://www.sourcewiki.org/wiki/index.php/Resetting_the_Map
//-----------------------------------------------------------------------------------------
#include "cbase.h"
#include "dhl_mapfilter.h"
 
// Constructor
CMapEntityFilter::CMapEntityFilter() 
{
	keepList = new CUtlVector< const char *>;
}
 
// Deconstructor
CMapEntityFilter::~CMapEntityFilter() 
{
	delete keepList;
}
 
// [bool] ShouldCreateEntity [char]
// Purpose   : Used to check if the passed in entity is on our stored list
// Arguments : The classname of an entity 
// Returns   : Boolean value - if we have it stored, we return false.
 
bool CMapEntityFilter::ShouldCreateEntity( const char *pClassname ) 
{
	//Note: CUtlVector::Find() does not work because the == operator for const char* is not the same as strcmp() (what we want to do)
	for ( int i = 0; i < keepList->Count(); i++ )
	{
		if ( Q_strcmp( keepList->Element( i ), pClassname ) == 0 )
			return false;
	}

	return true;
}
 
// [CBaseEntity] CreateNextEntity [char]
// Purpose   : Creates an entity
// Arguments : The classname of an entity
// Returns   : A pointer to the new entity
 
CBaseEntity* CMapEntityFilter::CreateNextEntity( const char *pClassname ) 
{
	return CreateEntityByName( pClassname );
}
 
// [void] AddKeep [char]
// Purpose   : Adds the passed in value to our list of items to keep
// Arguments : The class name of an entity
// Returns   : Void
 
void CMapEntityFilter::AddKeep( const char *sz ) 
{
	keepList->AddToTail(sz);
}