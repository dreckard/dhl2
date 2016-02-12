#ifndef DHL_MAPRULES_H
#define DHL_MAPRULES_H

class CDHLMapRules : public CLogicalEntity
{
public:
	DECLARE_CLASS( CDHLMapRules, CLogicalEntity );
	DECLARE_DATADESC();

	// Constructor
	CDHLMapRules ( void ) {}

	void SetForceGrayscale( bool bForce );
	bool GetForceGrayscale( void );

private:
	bool m_bForceGrayscale;
};
#endif