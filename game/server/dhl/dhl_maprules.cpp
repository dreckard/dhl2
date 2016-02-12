//-----------------------------------------------------------------------------
// Distraction Half-Life 2:
// Logical entity controlling some map specific variables
//-----------------------------------------------------------------------------
#include "cbase.h"
#include "dhl_maprules.h"

LINK_ENTITY_TO_CLASS( dhl_maprules, CDHLMapRules );

// Start of our data description for the class
BEGIN_DATADESC( CDHLMapRules )
	DEFINE_KEYFIELD( m_bForceGrayscale, FIELD_BOOLEAN, "ForceGrayscale" ),
END_DATADESC()

//-----------------------------------------------------------------------------
// Purpose: Change forced shader value
//-----------------------------------------------------------------------------
void CDHLMapRules::SetForceGrayscale( bool bForce )
{
	if ( bForce )
	{
		m_bForceGrayscale = true;
	}
	else
	{
		m_bForceGrayscale = false;
	}
}

bool CDHLMapRules::GetForceGrayscale( void )
{
	return m_bForceGrayscale;
}